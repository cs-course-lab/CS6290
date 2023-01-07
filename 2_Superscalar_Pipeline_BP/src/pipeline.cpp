// ------------------------------------------------------------------------ //
// This is a reference implementation for part A.                           //
// To implement part B, you may add any code you need, as long as you       //
// correctly implement the required pipe_cycle_*() functions already listed //
// in this file, as well as pipe_check_bpred().                             //
// ------------------------------------------------------------------------ //

// pipeline.cpp
// Implements functions to simulate a pipelined processor.

#include "pipeline.h"
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

/**
 * Read a single trace record from the trace file and use it to populate the
 * given fetch_op.
 * 
 * You should not modify this function.
 * 
 * @param p the pipeline whose trace file should be read
 * @param fetch_op the PipelineLatch struct to populate
 */
void pipe_get_fetch_op(Pipeline *p, PipelineLatch *fetch_op)
{
    TraceRec *trace_rec = &fetch_op->trace_rec;
    uint8_t *trace_rec_buf = (uint8_t *)trace_rec;
    size_t bytes_read_total = 0;
    ssize_t bytes_read_last = 0;
    size_t bytes_left = sizeof(*trace_rec);

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
    if (bytes_left > 0 || trace_rec->op_type >= NUM_OP_TYPES)
    {
        fetch_op->valid = false;
        p->halt_op_id = p->last_op_id;

        if (p->last_op_id == 0)
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
    fetch_op->valid = true;
    fetch_op->stall = false;
    fetch_op->is_mispred_cbr = false;
    fetch_op->op_id = ++p->last_op_id;
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
    p->trace_fd = trace_fd;
    p->halt_op_id = (uint64_t)(-1) - 3;

    // Allocate and initialize a branch predictor if needed.
    if (BPRED_POLICY != BPRED_PERFECT)
    {
        p->b_pred = new BPred(BPRED_POLICY);
    }

    return p;
}

/**
 * Print out the state of the pipeline latches for debugging purposes.
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
    for (uint8_t latch_type = 0; latch_type < NUM_LATCH_TYPES; latch_type++)
    {
        switch (latch_type)
        {
        case IF_LATCH:
            printf(" IF:    ");
            break;
        case ID_LATCH:
            printf(" ID:    ");
            break;
        case EX_LATCH:
            printf(" EX:    ");
            break;
        case MA_LATCH:
            printf(" MA:    ");
            break;
        default:
            printf(" ------ ");
        }
    }
    printf("\n");

    // Print row for each lane in pipeline width
    for (uint8_t i = 0; i < PIPE_WIDTH; i++)
    {
        for (uint8_t latch_type = 0; latch_type < NUM_LATCH_TYPES;
             latch_type++)
        {
            if (p->pipe_latch[latch_type][i].valid)
            {
                printf(" %6lu ",
                       (unsigned long)p->pipe_latch[latch_type][i].op_id);
            }
            else
            {
                printf(" ------ ");
            }
        }
        printf("\n");
    }
    printf("\n");
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

    // In hardware, all pipeline stages execute in parallel, and each pipeline
    // latch is populated at the start of the next clock cycle.

    // In our simulator, we simulate the pipeline stages one at a time in
    // reverse order, from the Write Back stage (WB) to the Fetch stage (IF).
    // We do this so that each stage can read from the latch before it and
    // write to the latch after it without needing to "double-buffer" the
    // latches.

    // Additionally, it means that earlier pipeline stages can know about
    // stalls triggered in later pipeline stages in the same cycle, as would be
    // the case with hardware stall signals asserted by combinational logic.

    pipe_cycle_WB(p);
    pipe_cycle_MA(p);
    pipe_cycle_EX(p);
    pipe_cycle_ID(p);
    pipe_cycle_IF(p);

    // You can uncomment the following line to print out the pipeline state
    // after each clock cycle for debugging purposes.
    // Make sure you comment it out or remove it before you submit the lab.
    //pipe_print_state(p);
}

/**
 * Simulate one cycle of the Write Back stage (WB) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_WB(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (p->pipe_latch[MA_LATCH][i].valid)
        {
            p->stat_retired_inst++;

            if (p->pipe_latch[MA_LATCH][i].op_id >= p->halt_op_id)
            {
                // Halt the pipeline if we've reached the end of the trace.
                p->halt = true;
            }
            if ((p->pipe_latch[MA_LATCH][i].trace_rec.op_type == 3) && p->pipe_latch[MA_LATCH][i].is_mispred_cbr){
                for( unsigned int j = 0; j < PIPE_WIDTH; j ++){
                    p->pipe_latch[IF_LATCH][j].is_mispred_cbr = false;
                }
                #ifdef VERBOSE
                    std::cout << "WB_STAGE: CBR Release\n";
                #endif
            }

        }
    }
}

/**
 * Simulate one cycle of the Memory Access stage (MA) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_MA(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        // Copy each instruction from the EX latch to the MA latch.
        p->pipe_latch[MA_LATCH][i] = p->pipe_latch[EX_LATCH][i];
        
    }
}

/**
 * Simulate one cycle of the Execute stage (EX) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_EX(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        // Copy each instruction from the ID latch to the EX latch.
        p->pipe_latch[EX_LATCH][i] = p->pipe_latch[ID_LATCH][i];
    }
}

/**
 * Simulate one cycle of the Instruction Decode stage (ID) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_ID(Pipeline *p)
{
    // Bookkeeping information:
    // Is any instruction in the ID latch being stalled this cycle?
    bool is_instruction_stalled_this_cycle = false;
    // What is the op_id of the oldest instruction stalled this cycle?
    uint64_t oldest_op_id_stalled = 0;

    // For each lane of the superscalar pipeline:
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        // Copy each instruction from the IF latch to the ID latch.
        p->pipe_latch[ID_LATCH][i] = p->pipe_latch[IF_LATCH][i];

        // If this lane of IF was previously stalled, clear its stall flag.
        // We will re-stall if needed according to the stall logic below.
        p->pipe_latch[IF_LATCH][i].stall = false;
    }

    // Check for stall conditions for each instruction in the ID latch.
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (!p->pipe_latch[ID_LATCH][i].valid)
        {
            // There is no instruction in this lane. Skip stall checks.
            continue;
        }

        // Identify each dependency of this instruction that could potentially
        // cause a RAW hazard.

        bool found_src1_dependency = false;
        LatchType src1_dependency_in_latch = (LatchType)(0);
        uint64_t src1_dependency_op_id = 0;
        uint8_t src1_dependency_op_type = 0;

        bool found_src2_dependency = false;
        LatchType src2_dependency_in_latch = (LatchType)(0);
        uint64_t src2_dependency_op_id = 0;
        uint8_t src2_dependency_op_type = 0;

        bool found_cc_dependency = false;
        LatchType cc_dependency_in_latch = (LatchType)(0);
        uint64_t cc_dependency_op_id = 0;
        uint8_t cc_dependency_op_type = 0;

        // Check each lane of MA_LATCH, EX_LATCH, and ID_LATCH for potential
        // dependencies.
        for (unsigned int j = 0; j < PIPE_WIDTH; j++)
        {
            if (p->pipe_latch[MA_LATCH][j].valid)
            {
                // There is an instruction in lane j of the MA_LATCH.
                // Could it cause a RAW hazard?

                if (p->pipe_latch[ID_LATCH][i].trace_rec.src1_needed &&
                    p->pipe_latch[MA_LATCH][j].trace_rec.dest_needed &&
                    p->pipe_latch[ID_LATCH][i].trace_rec.src1_reg == p->pipe_latch[MA_LATCH][j].trace_rec.dest_reg)
                {
                    // We found a dependency for this instruction's src1.
                    // Is it the youngest dependency we've found so far?
                    if (!found_src1_dependency || p->pipe_latch[MA_LATCH][j].op_id > src1_dependency_op_id)
                    {
                        // Yes, it is!
                        found_src1_dependency = true;
                        src1_dependency_in_latch = MA_LATCH;
                        src1_dependency_op_id = p->pipe_latch[MA_LATCH][j].op_id;
                        src1_dependency_op_type = p->pipe_latch[MA_LATCH][j].trace_rec.op_type;
                    }
                }

                if (p->pipe_latch[ID_LATCH][i].trace_rec.src2_needed &&
                    p->pipe_latch[MA_LATCH][j].trace_rec.dest_needed &&
                    p->pipe_latch[ID_LATCH][i].trace_rec.src2_reg == p->pipe_latch[MA_LATCH][j].trace_rec.dest_reg)
                {
                    // We found a dependency for this instruction's src2.
                    // Is it the youngest dependency we've found so far?
                    if (!found_src2_dependency || p->pipe_latch[MA_LATCH][j].op_id > src2_dependency_op_id)
                    {
                        // Yes, it is!
                        found_src2_dependency = true;
                        src2_dependency_in_latch = MA_LATCH;
                        src2_dependency_op_id = p->pipe_latch[MA_LATCH][j].op_id;
                        src2_dependency_op_type = p->pipe_latch[MA_LATCH][j].trace_rec.op_type;
                    }
                }

                if (p->pipe_latch[ID_LATCH][i].trace_rec.cc_read &&
                    p->pipe_latch[MA_LATCH][j].trace_rec.cc_write)
                {
                    // We found a dependency for this instruction's cc.
                    // Is it the youngest dependency we've found so far?
                    if (!found_cc_dependency || p->pipe_latch[MA_LATCH][j].op_id > cc_dependency_op_id)
                    {
                        // Yes, it is!
                        found_cc_dependency = true;
                        cc_dependency_in_latch = MA_LATCH;
                        cc_dependency_op_id = p->pipe_latch[MA_LATCH][j].op_id;
                        cc_dependency_op_type = p->pipe_latch[MA_LATCH][j].trace_rec.op_type;
                    }
                }
            }

            if (p->pipe_latch[EX_LATCH][j].valid)
            {
                // There is an instruction in lane j of the EX_LATCH.
                // Could it cause a RAW hazard?

                if (p->pipe_latch[ID_LATCH][i].trace_rec.src1_needed &&
                    p->pipe_latch[EX_LATCH][j].trace_rec.dest_needed &&
                    p->pipe_latch[ID_LATCH][i].trace_rec.src1_reg == p->pipe_latch[EX_LATCH][j].trace_rec.dest_reg)
                {
                    // We found a dependency for this instruction's src1.
                    // Is it the youngest dependency we've found so far?
                    if (!found_src1_dependency || p->pipe_latch[EX_LATCH][j].op_id > src1_dependency_op_id)
                    {
                        // Yes, it is!
                        found_src1_dependency = true;
                        src1_dependency_in_latch = EX_LATCH;
                        src1_dependency_op_id = p->pipe_latch[EX_LATCH][j].op_id;
                        src1_dependency_op_type = p->pipe_latch[EX_LATCH][j].trace_rec.op_type;
                    }
                }

                if (p->pipe_latch[ID_LATCH][i].trace_rec.src2_needed &&
                    p->pipe_latch[EX_LATCH][j].trace_rec.dest_needed &&
                    p->pipe_latch[ID_LATCH][i].trace_rec.src2_reg == p->pipe_latch[EX_LATCH][j].trace_rec.dest_reg)
                {
                    // We found a dependency for this instruction's src2.
                    // Is it the youngest dependency we've found so far?
                    if (!found_src2_dependency || p->pipe_latch[EX_LATCH][j].op_id > src2_dependency_op_id)
                    {
                        // Yes, it is!
                        found_src2_dependency = true;
                        src2_dependency_in_latch = EX_LATCH;
                        src2_dependency_op_id = p->pipe_latch[EX_LATCH][j].op_id;
                        src2_dependency_op_type = p->pipe_latch[EX_LATCH][j].trace_rec.op_type;
                    }
                }

                if (p->pipe_latch[ID_LATCH][i].trace_rec.cc_read &&
                    p->pipe_latch[EX_LATCH][j].trace_rec.cc_write)
                {
                    // We found a dependency for this instruction's cc.
                    // Is it the youngest dependency we've found so far?
                    if (!found_cc_dependency || p->pipe_latch[EX_LATCH][j].op_id > cc_dependency_op_id)
                    {
                        // Yes, it is!
                        found_cc_dependency = true;
                        cc_dependency_in_latch = EX_LATCH;
                        cc_dependency_op_id = p->pipe_latch[EX_LATCH][j].op_id;
                        cc_dependency_op_type = p->pipe_latch[EX_LATCH][j].trace_rec.op_type;
                    }
                }
            }

            if (p->pipe_latch[ID_LATCH][j].valid &&
                p->pipe_latch[ID_LATCH][j].op_id < p->pipe_latch[ID_LATCH][i].op_id)
            {
                // There is an older instruction in lane j of the ID_LATCH.
                // Could it cause a RAW hazard?

                if (p->pipe_latch[ID_LATCH][i].trace_rec.src1_needed &&
                    p->pipe_latch[ID_LATCH][j].trace_rec.dest_needed &&
                    p->pipe_latch[ID_LATCH][i].trace_rec.src1_reg == p->pipe_latch[ID_LATCH][j].trace_rec.dest_reg)
                {
                    // We found a dependency for this instruction's src1.
                    // Is it the youngest dependency we've found so far?
                    if (!found_src1_dependency || p->pipe_latch[ID_LATCH][j].op_id > src1_dependency_op_id)
                    {
                        // Yes, it is!
                        found_src1_dependency = true;
                        src1_dependency_in_latch = ID_LATCH;
                        src1_dependency_op_id = p->pipe_latch[ID_LATCH][j].op_id;
                        src1_dependency_op_type = p->pipe_latch[ID_LATCH][j].trace_rec.op_type;
                    }
                }

                if (p->pipe_latch[ID_LATCH][i].trace_rec.src2_needed &&
                    p->pipe_latch[ID_LATCH][j].trace_rec.dest_needed &&
                    p->pipe_latch[ID_LATCH][i].trace_rec.src2_reg == p->pipe_latch[ID_LATCH][j].trace_rec.dest_reg)
                {
                    // We found a dependency for this instruction's src2.
                    // Is it the youngest dependency we've found so far?
                    if (!found_src2_dependency || p->pipe_latch[ID_LATCH][j].op_id > src2_dependency_op_id)
                    {
                        // Yes, it is!
                        found_src2_dependency = true;
                        src2_dependency_in_latch = ID_LATCH;
                        src2_dependency_op_id = p->pipe_latch[ID_LATCH][j].op_id;
                        src2_dependency_op_type = p->pipe_latch[ID_LATCH][j].trace_rec.op_type;
                    }
                }

                if (p->pipe_latch[ID_LATCH][i].trace_rec.cc_read &&
                    p->pipe_latch[ID_LATCH][j].trace_rec.cc_write)
                {
                    // We found a dependency for this instruction's cc.
                    // Is it the youngest dependency we've found so far?
                    if (!found_cc_dependency || p->pipe_latch[ID_LATCH][j].op_id > cc_dependency_op_id)
                    {
                        // Yes, it is!
                        found_cc_dependency = true;
                        cc_dependency_in_latch = ID_LATCH;
                        cc_dependency_op_id = p->pipe_latch[ID_LATCH][j].op_id;
                        cc_dependency_op_type = p->pipe_latch[ID_LATCH][j].trace_rec.op_type;
                    }
                }
            }
        }

        // We have now identified the dependencies for this instruction.
        // Do any of them require us to stall?

        bool should_stall_for_src1 = false;
        bool should_stall_for_src2 = false;
        bool should_stall_for_cc = false;

        if (found_src1_dependency)
        {
            if (src1_dependency_in_latch == MA_LATCH)
            {
                if (ENABLE_MEM_FWD)
                {
                    // We can forward any dependency from the MA_LATCH.
                    // No need to stall for this dependency.
                }
                else
                {
                    should_stall_for_src1 = true;
                }
            }

            if (src1_dependency_in_latch == EX_LATCH)
            {
                if (ENABLE_EXE_FWD)
                {
                    // We can only forward dependencies from the EX_LATCH if
                    // the dependency is not a load instruction.
                    should_stall_for_src1 = (src1_dependency_op_type == OP_LD);
                }
                else
                {
                    should_stall_for_src1 = true;
                }
            }

            if (src1_dependency_in_latch == ID_LATCH)
            {
                // We can never forward dependencies from the ID_LATCH.
                // We must always stall.
                should_stall_for_src1 = true;
            }
        }

        if (found_src2_dependency)
        {
            if (src2_dependency_in_latch == MA_LATCH)
            {
                if (ENABLE_MEM_FWD)
                {
                    // We can forward any dependency from the MA_LATCH.
                    // No need to stall for this dependency.
                }
                else
                {
                    should_stall_for_src2 = true;
                }
            }

            if (src2_dependency_in_latch == EX_LATCH)
            {
                if (ENABLE_EXE_FWD)
                {
                    // We can only forward dependencies from the EX_LATCH if
                    // the dependency is not a load instruction.
                    should_stall_for_src2 = (src2_dependency_op_type == OP_LD);
                }
                else
                {
                    should_stall_for_src2 = true;
                }
            }

            if (src2_dependency_in_latch == ID_LATCH)
            {
                // We can never forward dependencies from the ID_LATCH.
                // We must always stall.
                should_stall_for_src2 = true;
            }
        }

        if (found_cc_dependency)
        {
            if (cc_dependency_in_latch == MA_LATCH)
            {
                if (ENABLE_MEM_FWD)
                { 
                    // We can forward any dependency from the MA_LATCH.
                    // No need to stall for this dependency.
                }
                else
                {
                    should_stall_for_cc = true;
                }
            }

            if (cc_dependency_in_latch == EX_LATCH)
            {
                if (ENABLE_EXE_FWD)
                {
                    // We can only forward dependencies from the EX_LATCH if
                    // the dependency is not a load instruction.
                    should_stall_for_cc = (cc_dependency_op_type == OP_LD);
                }
                else
                {
                    should_stall_for_cc = true;
                }
            }

            if (cc_dependency_in_latch == ID_LATCH)
            {
                // We can never forward dependencies from the ID_LATCH.
                // We must always stall.
                should_stall_for_cc = true;
            }
        }

        // Finally, we know if we need to stall this instruction due to any of
        // its dependencies.
        if (should_stall_for_src1 || should_stall_for_src2 || should_stall_for_cc)
        {
            // Update bookkeeping information.
            if (!is_instruction_stalled_this_cycle ||
                p->pipe_latch[ID_LATCH][i].op_id < oldest_op_id_stalled)
            {
                oldest_op_id_stalled = p->pipe_latch[ID_LATCH][i].op_id;
            }
            is_instruction_stalled_this_cycle = true;

            // Insert a bubble into the ID/EX latch.
            p->pipe_latch[ID_LATCH][i].valid = false;

            // Tell the IF stage to stall this lane.
            p->pipe_latch[IF_LATCH][i].stall = true;
        }
    }

    if (is_instruction_stalled_this_cycle)
    {
        // Enforce in-order execution by stalling any remaining instructions
        // younger than the oldest one stalled.
        for (unsigned int i = 0; i < PIPE_WIDTH; i++)
        {
            if (p->pipe_latch[ID_LATCH][i].valid &&
                p->pipe_latch[ID_LATCH][i].op_id > oldest_op_id_stalled)
            {
                // Insert a bubble into the ID/EX latch.
                p->pipe_latch[ID_LATCH][i].valid = false;

                // Tell the IF stage to stall this lane.
                p->pipe_latch[IF_LATCH][i].stall = true;
            }
        }
    }
}

/**
 * Simulate one cycle of the Instruction Fetch stage (IF) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_IF(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (p->pipe_latch[IF_LATCH][i].stall)
        {
            // If ID has told this lane to stall, then do nothing.
            // This will leave the instruction in this IF_LATCH alone, thereby
            // keeping it available for ID to process again in the next cycle.
            continue;
        }

        // Read an instruction from the trace file.
        PipelineLatch fetch_op;
        if (p->pipe_latch[IF_LATCH][i].is_mispred_cbr){//&& (p->pipe_latch[IF_LATCH][i].needed_bubble != 0)){
            fetch_op = p->pipe_latch[IF_LATCH][i];
            fetch_op.valid = false;
            #ifdef VERBOSE
                fprintf(stdout, "NOT FETCH, cbr: %d \n", fetch_op.is_mispred_cbr);
            #endif
        }
        else{
            pipe_get_fetch_op(p, &fetch_op);
            #ifdef VERBOSE
                std::cout << "FETCH\n";
            #endif
            // Handle branch (mis)prediction.
            if (BPRED_POLICY != BPRED_PERFECT && fetch_op.valid && (fetch_op.trace_rec.op_type == 3))
            {
                pipe_check_bpred(p, &fetch_op, i);
            }
        }
        // Copy the instruction to the IF latch.
        p->pipe_latch[IF_LATCH][i] = fetch_op;
    }
}

/**
 * If the instruction just fetched is a conditional branch, check for a branch
 * misprediction, update the branch predictor, and set appropriate flags in the
 * pipeline.
 * 
 * You must implement this function in part B of the lab.
 * 
 * @param p the pipeline
 * @param fetch_op the pipeline latch containing the operation fetched
 */
void pipe_check_bpred(Pipeline *p, PipelineLatch *fetch_op, unsigned int i)
{
    BranchDirection predict = p->b_pred->predict(fetch_op->trace_rec.inst_addr); // why taking pc?
    // TODO: For a conditional branch instruction, get a prediction from the
    // branch predictor.
    
    BranchDirection resolution = predict;
    // std::cout << "Pipe check bpred " << predict << ", " <<fetch_op->trace_rec.br_dir << std::endl; 
    #ifdef VERBOSE
        fprintf(stdout, "Pipe check bpred %d, %d\n", predict,fetch_op->trace_rec.br_dir);
    #endif
    if (predict != (fetch_op->trace_rec.br_dir)){
        
        fetch_op->is_mispred_cbr = true;
        for (unsigned int j = 0; j < PIPE_WIDTH; j ++){
            p->pipe_latch[IF_LATCH][j].is_mispred_cbr = true;
            // p->pipe_latch[IF_LATCH][j].needed_bubble = 3;
        }
        if (predict == TAKEN)
            resolution = NOT_TAKEN;
        else
            resolution = TAKEN;
        
    }
    // TODO: If the branch predictor mispredicted, mark the fetch_op
    // accordingly.
    // TODO: If needed, stall the IF stage by setting the flag
    // p->fetch_cbr_stall.

    p->b_pred->update(fetch_op->trace_rec.inst_addr, predict, resolution);
    // TODO: Immediately update the branch predictor.
    
}
