//////////////////////////////////////////////////////////////////////
// In part B, you must modify this file to implement the following: //
// - void pipe_cycle_issue(Pipeline *p)                             //
// - void pipe_cycle_schedule(Pipeline *p)                          //
// - void pipe_cycle_writeback(Pipeline *p)                         //
// - void pipe_cycle_commit(Pipeline *p)                            //
//////////////////////////////////////////////////////////////////////

// pipeline.cpp
// Implements the out-of-order pipeline.

#include "pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// #define DEBUG

/**
 * The width of the pipeline; that is, the maximum number of instructions that
 * can be processed during any given cycle in each of the issue, schedule, and
 * commit stages of the pipeline.
 * 
 * (Note that this does not apply to the writeback stage: as many as
 * MAX_WRITEBACKS instructions can be written back to the ROB in a single
 * cycle!)
 * 
 * When the width is 1, the pipeline is scalar.
 * When the width is greater than 1, the pipeline is superscalar.
 */
extern uint32_t PIPE_WIDTH;

/**
 * The number of entries in the ROB; that is, the maximum number of
 * instructions that can be stored in the ROB at any given time.
 * 
 * You should use only this many entries of the ROB::entries array.
 */
extern uint32_t NUM_ROB_ENTRIES;

/**
 * Whether to use in-order scheduling or out-of-order scheduling.
 * 
 * The possible values are SCHED_IN_ORDER for in-order scheduling and
 * SCHED_OUT_OF_ORDER for out-of-order scheduling.
 * 
 * Your implementation of pipe_cycle_sched() should check this value and
 * implement scheduling of instructions accordingly.
 */
extern SchedulingPolicy SCHED_POLICY;

/**
 * The number of cycles an LD instruction should take to execute.
 * 
 * This is used by the code in exeq.cpp to determine how long to wait before
 * considering the execution of an LD instruction done.
 */
extern uint32_t LOAD_EXE_CYCLES;

int cur_exe_idx;

/**
 * Read a single trace record from the trace file and use it to populate the
 * given fe_latch.
 * 
 * You should not modify this function.
 * 
 * @param p the pipeline whose trace file should be read
 * @param fe_latch the PipelineLatch struct to populate
 */
void pipe_fetch_inst(Pipeline *p, PipelineLatch *fe_latch)
{
    InstInfo *inst = &fe_latch->inst;
    TraceRec trace_rec;
    uint8_t *trace_rec_buf = (uint8_t *)&trace_rec;
    size_t bytes_read_total = 0;
    ssize_t bytes_read_last = 0;
    size_t bytes_left = sizeof(TraceRec);

    // Read a total of sizeof(TraceRec) bytes from the trace file.
    while (bytes_left > 0)
    {
        bytes_read_last = read(p->trace_fd, trace_rec_buf, bytes_left);
        if (bytes_read_last <= 0)
        {
            // EOF or error
            break;
        }

        trace_rec_buf += bytes_read_last;
        bytes_read_total += bytes_read_last;
        bytes_left -= bytes_read_last;
    }

    // Check for error conditions.
    if (bytes_left > 0 || trace_rec.op_type >= NUM_OP_TYPES)
    {
        fe_latch->valid = false;
        p->halt_inst_num = p->last_inst_num;

        if (p->stat_retired_inst >= p->halt_inst_num)
        {
            p->halt = true;
        }

        if (bytes_read_last == -1)
        {
            fprintf(stderr, "\n");
            perror("Couldn't read from pipe");
            return;
        }

        if (bytes_read_total == 0)
        {
            // No more trace records to read
            return;
        }

        // Too few bytes read or invalid op_type
        fprintf(stderr, "\n");
        fprintf(stderr, "Error: Invalid trace file\n");
        return;
    }

    // Got a valid trace record!
    fe_latch->valid = true;
    fe_latch->stall = false;
    inst->inst_num = ++p->last_inst_num;
    inst->op_type = (OpType)trace_rec.op_type;

    inst->dest_reg = trace_rec.dest_needed ? trace_rec.dest_reg : -1;
    inst->src1_reg = trace_rec.src1_needed ? trace_rec.src1_reg : -1;
    inst->src2_reg = trace_rec.src2_needed ? trace_rec.src2_reg : -1;

    inst->dr_tag = -1;
    inst->src1_tag = -1;
    inst->src2_tag = -1;
    inst->src1_ready = false;
    inst->src2_ready = false;
    inst->exe_wait_cycles = 0;
}

/**
 * Allocate and initialize a new pipeline.
 * 
 * You should not need to modify this function.
 * 
 * @param trace_fd the file descriptor from which to read trace records
 * @return a pointer to a newly allocated pipeline
 */
Pipeline *pipe_init(int trace_fd)
{
    printf("\n** PIPELINE IS %d WIDE **\n\n", PIPE_WIDTH);

    // Allocate pipeline.
    Pipeline *p = (Pipeline *)calloc(1, sizeof(Pipeline));

    // Initialize pipeline.
    p->rat = rat_init();
    p->rob = rob_init();
    p->exeq = exeq_init();
    p->trace_fd = trace_fd;
    // #ifdef DEBUG
    // p->halt_inst_num = 1000;
    // #endif
    #ifndef DEBUG
    p->halt_inst_num = (uint64_t)(-1) - 3;
    #endif
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        p->FE_latch[i].valid = false;
        p->ID_latch[i].valid = false;
        p->SC_latch[i].valid = false;
    }
    for (unsigned int i = 0; i < MAX_WRITEBACKS; i++)
    {
        p->EX_latch[i].valid = false;
    }
    cur_exe_idx = 0;
    return p;
}

/**
 * Commit the given instruction.
 * 
 * This updates counters and flags on the pipeline.
 * 
 * This function is implemented for you. You should not modify it.
 * 
 * @param p the pipeline to update.
 * @param inst the instruction to commit.
 */
void pipe_commit_inst(Pipeline *p, InstInfo inst)
{
    p->stat_retired_inst++;

    if (inst.inst_num >= p->halt_inst_num)
    {
        p->halt = true;
    }
}

/**
 * Print out the state of the pipeline for debugging purposes.
 * 
 * You may use this function to help debug your pipeline implementation, but
 * please remove calls to this function before submitting the lab.
 * 
 * @param p the pipeline
 */
void pipe_print_state(Pipeline *p)
{
    printf("\n--------------------------------------------\n");
    printf("Cycle count: %lu, retired instructions: %lu\n",
           (unsigned long)p->stat_num_cycle,
           (unsigned long)p->stat_retired_inst);

    // Print table header
    for (unsigned int latch_type = 0; latch_type < 4; latch_type++)
    {
        switch (latch_type)
        {
        case 0:
            printf(" FE:    ");
            break;
        case 1:
            printf(" ID:    ");
            break;
        case 2:
            printf(" SCH:   ");
            break;
        case 3:
            printf(" EX:    ");
            break;
        default:
            printf(" ------ ");
        }
    }
    printf("\n");

    // Print row for each lane in pipeline width
    unsigned int ex_i = 0;
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (p->FE_latch[i].valid)
        {
            printf(" %6lu ",
                   (unsigned long)p->FE_latch[i].inst.inst_num);
        }
        else
        {
            printf(" ------ ");
        }
        if (p->ID_latch[i].valid)
        {
            printf(" %6lu ",
                   (unsigned long)p->ID_latch[i].inst.inst_num);
        }
        else
        {
            printf(" ------ ");
        }
        if (p->SC_latch[i].valid)
        {
            printf(" %6lu ",
                   (unsigned long)p->SC_latch[i].inst.inst_num);
        }
        else
        {
            printf(" ------ ");
        }
        for (; ex_i < MAX_WRITEBACKS; ex_i++)
        {
            if (p->EX_latch[ex_i].valid)
            {
                printf(" %6lu ",
                       (unsigned long)p->EX_latch[ex_i].inst.inst_num);
                ex_i++;
                break;
            }
        }
        printf("\n");
    }
    printf("\n");

    rat_print_state(p->rat);
    exeq_print_state(p->exeq);
    rob_print_state(p->rob);
}

/**
 * Simulate one cycle of all stages of a pipeline.
 * 
 * You should not need to modify this function except for debugging purposes.
 * If you add code to print debug output in this function, remove it or comment
 * it out before you submit the lab.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle(Pipeline *p)
{
    p->stat_num_cycle++;

    // In our simulator, stages are processed in reverse order.
    pipe_cycle_commit(p);
    pipe_cycle_writeback(p);
    pipe_cycle_exe(p);
    pipe_cycle_schedule(p);
    pipe_cycle_issue(p);
    pipe_cycle_decode(p);
    pipe_cycle_fetch(p);

    // You can uncomment the following line to print out the pipeline state
    // after each clock cycle for debugging purposes.
    // Make sure you comment it out or remove it before you submit the lab.
    // pipe_print_state(p);
    // fprintf(stdout, "PRINT END\n");
    // fflush(stdout);
}

/**
 * Simulate one cycle of the fetch stage of a pipeline.
 * 
 * This function is implemented for you. You should not modify it.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_fetch(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (!p->FE_latch[i].stall && !p->FE_latch[i].valid)
        {

            // No stall and latch empty, so fetch a new instruction.
            pipe_fetch_inst(p, &p->FE_latch[i]);
            // fprintf(stdout, "FETCH:: %dth latch, valid:%d, inst_num:%d\n", i, p->FE_latch[i].valid, p->FE_latch[i].inst.inst_num);
        }
    }
}

/**
 * Simulate one cycle of the instruction decode stage of a pipeline.
 * 
 * This function is implemented for you. You should not modify it.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_decode(Pipeline *p)
{
    static uint64_t next_inst_num = 1;
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (!p->ID_latch[i].stall && !p->ID_latch[i].valid)
        {
            // No stall and latch empty, so decode the next instruction.
            // Loop to find the next in-order instruction.
            for (unsigned int j = 0; j < PIPE_WIDTH; j++)
            {
            // fprintf(stdout, "DECODE:: %dth FE_valid: %d, FE_inst_num:%d, next_inst_num:%d \n", j, p->FE_latch[j].valid, p->FE_latch[j].inst.inst_num, next_inst_num);
                if (p->FE_latch[j].valid &&
                    p->FE_latch[j].inst.inst_num == next_inst_num)
                {
                    p->ID_latch[i] = p->FE_latch[j];
                    p->FE_latch[j].valid = false;
                    next_inst_num++;
                    break;
                }
            }
        }
    }
}

/**
 * Simulate one cycle of the execute stage of a pipeline. This handles
 * instructions that take multiple cycles to execute.
 * 
 * This function is implemented for you. You should not modify it.
 * 
 * @param p the pipeline to simulate
 */

void pipe_cycle_exe(Pipeline *p)
{
    // If all operations are single-cycle, just copy SC latches to EX latches.
    if (LOAD_EXE_CYCLES == 1)
    {
        for (unsigned int i = 0; i < PIPE_WIDTH; i++)
        {
            if (p->SC_latch[i].valid)
            {
                p->EX_latch[i] = p->SC_latch[i];
                p->SC_latch[i].valid = false;
            }
        }
        return;
    }

    // Otherwise, we need to handle multi-cycle instructions with EXEQ.

    // All valid entries from the SC latches are inserted into the EXEQ.
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (p->SC_latch[i].valid)
        {
            if (!exeq_insert(p->exeq, p->SC_latch[i].inst))
            {
                fprintf(stderr, "Error: EXEQ full\n");
                p->halt = true;
                return;
            }

            p->SC_latch[i].valid = false;
        }
    }

    // Cycle the EXEQ to reduce wait time for each instruction by 1 cycle.
    exeq_cycle(p->exeq);

    // Transfer all finished entries from the EXEQ to the EX latch.
    for (unsigned int i = 0; i < MAX_WRITEBACKS && exeq_check_done(p->exeq); i++)
    {
        p->EX_latch[i].valid = true;
        p->EX_latch[i].stall = false;
        p->EX_latch[i].inst = exeq_remove(p->exeq);
    }
}
/**
 * Simulate one cycle of the issue stage of a pipeline: insert decoded
 * instructions into the ROB and perform register renaming.
 * 
 * You must implement this function in pipeline.cpp in part B of the
 * assignment.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_issue(Pipeline *p)
{

    // TODO: For each valid instruction from the ID stage:
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (p->ID_latch[i].valid)
        {

    // TODO: If there is space in the ROB, insert the instruction.
    // TODO: Set the entry invalid in the previous latch when you do so.
    // TODO: Then, check RAT for this instruction's source operands:
    // TODO: If src1 is not remapped, mark src1 as ready.
    // TODO: If src1 is remapped, set src1 tag accordingly, and set src1 ready
    //       according to whether the ROB entry with that tag is ready.
    // TODO: If src2 is not remapped, mark src2 as ready.
    // TODO: If src2 is remapped, set src2 tag accordingly, and set src2 ready
    //       according to whether the ROB entry with that tag is ready.
            if(rob_check_space(p->rob)){ // ROB has space
                // p->ID_latch[i];
                // fprintf(stdout, "ISSUE:: src1_reg: %d, src2_reg:%d, dest_reg:%d, inst_num:%d\n", p->ID_latch[i].inst.src1_reg, p->ID_latch[i].inst.src2_reg, p->ID_latch[i].inst.dest_reg, p->ID_latch[i].inst.inst_num);
                //insert instruction into ROB
                int latest_rob_id = rob_insert(p->rob, p->ID_latch[i].inst);
                // Check RAT
                if(p->ID_latch[i].inst.src1_reg == -1){ // src1_reg not use
                    // p->SC_latch[i].inst.src1_tag = -1;
                    p->ID_latch[i].inst.src1_ready = true;
                    p->rob->entries[latest_rob_id].inst.src1_ready = true;
                }else{ // src1_reg use
                    int remap_prf_id = rat_get_remap(p->rat, p->ID_latch[i].inst.src1_reg);
                    if(remap_prf_id == -1){ // src1 not remapped
                        p->ID_latch[i].inst.src1_ready = true;
                        p->rob->entries[latest_rob_id].inst.src1_ready = true;
                    }else{ // src1 remapped
                        p->ID_latch[i].inst.src1_tag = remap_prf_id;
                        p->rob->entries[latest_rob_id].inst.src1_tag = remap_prf_id;
                        if(p->rob->entries[remap_prf_id].ready){
                            p->ID_latch[i].inst.src1_ready = true;
                            p->rob->entries[latest_rob_id].inst.src1_ready = true;
                        }
                    }
                }

                if(p->ID_latch[i].inst.src2_reg == -1){ // src2_reg not use
                    // p->SC_latch[i].inst.src2_tag = -1;
                    p->ID_latch[i].inst.src2_ready = true;
                    p->rob->entries[latest_rob_id].inst.src2_ready = true;
                }else{ // src2_reg use
                    int remap_prf_id = rat_get_remap(p->rat, p->ID_latch[i].inst.src2_reg);
                    if(remap_prf_id == -1){ // src2 not remapped
                        p->ID_latch[i].inst.src2_ready = true;
                        p->rob->entries[latest_rob_id].inst.src2_ready = true;
                    }else{ // src2 remapped
                        p->ID_latch[i].inst.src2_tag = remap_prf_id;
                        p->rob->entries[latest_rob_id].inst.src2_tag = remap_prf_id;
                        if(p->rob->entries[remap_prf_id].ready){
                            p->ID_latch[i].inst.src2_ready = true;
                            p->rob->entries[latest_rob_id].inst.src2_ready = true;
                        }
                    }
                }
                // TODO: Set the tag for this instruction's destination register.
                // TODO: If this instruction writes to a register, update the RAT
                //       accordingly.
                if(p->ID_latch[i].inst.dest_reg != -1){ // dest_reg in use
                    // set the tag (rename dest. reg)
                    rat_set_remap(p->rat, p->ID_latch[i].inst.dest_reg, latest_rob_id);
                    // fprintf(stdout, "ISSUE:: RAT_SET_REMAP dest_reg: %d, rob_id: %d, inst_num:%d\n", p->ID_latch[i].inst.dest_reg, latest_rob_id, p->ID_latch[i].inst.inst_num);
                }


                p->ID_latch[i].inst.dr_tag = latest_rob_id;
                p->ID_latch[i].valid = false;
            }else{ // ROB full, cannot issue

            }
        }
    }
}

/**
 * Simulate one cycle of the scheduling stage of a pipeline: schedule
 * instructions to execute if they are ready.
 * 
 * You must implement this function in pipeline.cpp in part B of the
 * assignment.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_schedule(Pipeline *p)
{
    // TODO: Implement two scheduling policies:
    // TODO: Set the entry invalid in the previous latch when you do so.
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        // if(p->SC_latch[i].valid){
        //     continue;
        // }
            if (SCHED_POLICY == SCHED_IN_ORDER)
            {
                // In-order scheduling:
                // TODO: Find the oldest valid entry in the ROB that is not already
                //       executing.
                // if(!p->rob->entries[p->rob->head_ptr].exec){
                    // fprintf(stdout, "SCHEDULE:: IN_ORDER:: %dth pipe, inst: %d\n", i, p->rob->entries[p->rob->head_ptr].inst.inst_num);
                // TODO: Check if it is stalled, i.e., if at least one source operand
                //       is not ready.
                // TODO: If so, stop scheduling instructions.
                // TODO: Otherwise, mark it as executing in the ROB and send it to the
                //       next latch.
                int ii = p->rob->head_ptr;
                    for(; ii != p->rob->tail_ptr;){
                        if(!p->rob->entries[ii].exec){
                            if(p->rob->entries[ii].exec){
                                continue;
                            }
                            if((p->rob->entries[ii].inst.src1_reg == -1 || p->rob->entries[ii].inst.src1_ready )//|| (p->rob->entries[ii].inst.src1_reg == p->rob->entries[ii].inst.dest_reg)) 
                            && (p->rob->entries[ii].inst.src2_reg == -1 || p->rob->entries[ii].inst.src2_ready )){//|| (p->rob->entries[ii].inst.src2_reg == p->rob->entries[ii].inst.dest_reg))){
                // TODO: Mark it as executing in the ROB and send it to the next latch.
                // fprintf(stdout, "SCHEDULE:: OUT_OF_ORDER:: %dth pipe, valid: %d, inst: %d // head_ptr:%d, tail_ptr:%d\n", i, p->SC_latch[i].valid, p->rob->entries[i].inst.inst_num, p->rob->head_ptr, p->rob->tail_ptr);
                                rob_mark_exec(p->rob, p->rob->entries[ii].inst);
                                p->SC_latch[i].inst = p->rob->entries[ii].inst;
                                p->SC_latch[i].valid = true;
                                break;
                            }else{
                                break;
                            }
                        }
                        ii = (ii + 1) % NUM_ROB_ENTRIES;
                    }
 
                    // if((p->rob->entries[p->rob->head_ptr].inst.src1_reg == -1 || p->rob->entries[p->rob->head_ptr].inst.src1_ready || (p->rob->entries[p->rob->head_ptr].inst.src1_reg == p->rob->entries[p->rob->head_ptr].inst.dest_reg)) 
                    // && (p->rob->entries[p->rob->head_ptr].inst.src2_reg == -1 || p->rob->entries[p->rob->head_ptr].inst.src2_ready || (p->rob->entries[p->rob->head_ptr].inst.src2_reg == p->rob->entries[p->rob->head_ptr].inst.dest_reg))){
                    //     rob_mark_exec(p->rob, p->rob->entries[p->rob->head_ptr].inst);
                    //     p->SC_latch[i].inst = p->rob->entries[p->rob->head_ptr].inst;
                    //     p->SC_latch[i].valid = true;
                    // }
                // }
                // TODO: Repeat for each lane of the pipeline.
            }

            if (SCHED_POLICY == SCHED_OUT_OF_ORDER)
            {
                // Out-of-order scheduling:
                // TODO: Find the oldest valid entry in the ROB that has both source
                //       operands ready but is not already executing.
                int ii = p->rob->head_ptr;
                for(; ii != p->rob->tail_ptr;){
                    if(!p->rob->entries[ii].exec){
                        if((p->rob->entries[ii].inst.src1_reg == -1 || p->rob->entries[ii].inst.src1_ready) //|| (p->rob->entries[ii].inst.src1_reg == p->rob->entries[ii].inst.dest_reg)) 
                        && (p->rob->entries[ii].inst.src2_reg == -1 || p->rob->entries[ii].inst.src2_ready)){ //|| (p->rob->entries[ii].inst.src2_reg == p->rob->entries[ii].inst.dest_reg))){
                // TODO: Mark it as executing in the ROB and send it to the next latch.
                // fprintf(stdout, "SCHEDULE:: OUT_OF_ORDER:: %dth pipe, valid: %d, inst: %d // head_ptr:%d, tail_ptr:%d\n", i, p->SC_latch[i].valid, p->rob->entries[i].inst.inst_num, p->rob->head_ptr, p->rob->tail_ptr);
                            rob_mark_exec(p->rob, p->rob->entries[ii].inst);
                            p->SC_latch[i].inst = p->rob->entries[ii].inst;
                            p->SC_latch[i].valid = true;
                            break;
                        }
                    }
                    ii = (ii + 1) % NUM_ROB_ENTRIES;
                }
                // TODO: Repeat for each lane of the pipeline.
            }
    }
}

/**
 * Simulate one cycle of the writeback stage of a pipeline: update the ROB
 * with information from instructions that have finished executing.
 * 
 * You must implement this function in pipeline.cpp in part B of the
 * assignment.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_writeback(Pipeline *p)
{
    for (unsigned int i = 0; i < MAX_WRITEBACKS; i++)
    {
    // TODO: For each valid instruction from the EX stage:
        if(p->EX_latch[i].valid){
            // if(p->EX_latch[i].inst.dest_reg != -1){
                // fprintf(stdout, "WRITEBACK:: %dth, dest:%d, inst_num:%d\n", i, p->EX_latch[i].inst.dest_reg, p->EX_latch[i].inst.inst_num);

                // if (p->EX_latch[i].inst.inst_num <= p->rob->entries[p->rob->head_ptr].inst.inst_num){

                // }
        // TODO: Broadcast the result to all ROB entries.
                // fprintf(stderr, "W\t");
                rob_wakeup(p->rob, p->EX_latch[i].inst.dr_tag);
        // TODO: Update the ROB: mark the instruction ready to commit.
                // fprintf(stderr, "M\t");
                rob_mark_ready(p->rob, p->EX_latch[i].inst);
            // }
    // TODO: Invalidate the instruction in the previous latch.
            p->EX_latch[i].valid = false;
        }

    // Remember: how many instructions can the EX stage send to the WB stage
    // in one cycle?
    }
}

/**
 * Simulate one cycle of the commit stage of a pipeline: commit instructions
 * in the ROB that are ready to commit.
 * 
 * You must implement this function in pipeline.cpp in part B of the
 * assignment.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_commit(Pipeline *p)
{

    // The following code is DUMMY CODE to ensure that the base code compiles
    // and that the simulation terminates. Replace it with a correct
    // implementation!
    // fprintf(stdout, "COMMIT\n");
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        // if (p->EX_latch[i].valid)
        // {
    // TODO: Check if the instruction at the head of the ROB is ready to
    //       commit.
    // TODO: If so, remove it from the ROB.
            if(rob_check_head(p->rob)){
                InstInfo commit_inst = rob_remove_head(p->rob);
    // TODO: Commit that instruction.
                pipe_commit_inst(p, commit_inst);
                // rob_wakeup(p->rob, p->EX_latch[i].inst.dr_tag);
                // TODO: If a RAT mapping exists and is still relevant, update the RAT.
                if(commit_inst.dr_tag == p->rat->entries[commit_inst.dest_reg].prf_id)
                    rat_reset_entry(p->rat, commit_inst.dest_reg);
                // fprintf(stdout, "COMMIT:: %dth, inst_num:%d, dr_tag:%d, dest_reg:%d\n", i, commit_inst.inst_num, commit_inst.dr_tag, commit_inst.dest_reg);
    // TODO: Repeat for each lane of the pipeline.
            }

            // p->FE_latch[i].valid = false;
        // }
    }
}
