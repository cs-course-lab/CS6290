// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// You should not modify this file. //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

// rob.h
// Declares the struct for the re-order buffer.

#ifndef _ROB_H_
#define _ROB_H_

#include "trace.h"
#include <inttypes.h>

/**
 * [Internal] The maximum allowed number of ROB entries.
 * 
 * This is an implementation detail that defines the array size of the
 * ROB::entries array; you should not have to use this value directly.
 * 
 * You may need to use the global variable NUM_ROB_ENTRIES instead.
 */
#define MAX_ROB_ENTRIES 256

/** A single entry of the ROB that can hold one instruction. */
typedef struct ROBEntryStruct
{
    /**
     * Does this entry contain a valid instruction?
     * 
     * If false, this entry is empty and should be ignored.
     */
    bool valid;

    /** Has this instruction started executing? */
    bool exec;

    /**
     * Is this instruction's output ready? (i.e., is it finished executing?)
     * Equivalently, is this instruction ready to commit?
     */
    bool ready;

    /**
     * The instruction that this entry holds.
     * 
     * This holds the tags that must be used for register renaming and the
     * ready bits that determine whether the instruction is ready to be
     * executed. See the definition of the InstInfo structure in trace.h for
     * more information.
     */
    InstInfo inst;
} ROBEntry;

/**
 * The re-order buffer.
 * 
 * The ROB should be used as a circular buffer: when the head or tail pointers
 * reach NUM_ROB_ENTRIES, they should be wrapped around to 0.
 */
typedef struct ROB
{
    /**
     * An array of entries in the ROB. Each can hold one instruction.
     * 
     * Not all MAX_ROB_ENTRIES entries of this array will be used. Refer to the
     * global variable NUM_ROB_ENTRIES to see how many of these should be used.
     */
    ROBEntry entries[MAX_ROB_ENTRIES];

    /**
     * The index of the head entry of the ROB; that is, the entry that is "next
     * to commit." This is always the entry containing the oldest instruction.
     * 
     * This value should wrap around at NUM_ROB_ENTRIES.
     */
    int head_ptr;

    /**
     * The index of the tail entry of the ROB; that is, the entry that is "next
     * available." This is always just past the entry containing the youngest
     * instruction.
     * 
     * This value should wrap around at NUM_ROB_ENTRIES.
     */
    int tail_ptr;
} ROB;

/**
 * Allocate and initialize a new ROB.
 * 
 * This function has been implemented for you.
 * 
 * @return a pointer to a newly allocated ROB
 */
ROB *rob_init();

/**
 * Print out the state of the ROB for debugging purposes.
 * 
 * This function is called automatically in pipe_print_state(), but you may
 * also use it to help debug your ROB implementation. If you choose to do so,
 * please remove calls to this function before submitting the lab.
 * 
 * @param rob the ROB
 */
void rob_print_state(ROB *rob);

/**
 * Check if there is space available to insert another instruction into the
 * ROB.
 * 
 * You must implement this function in part A of the assignment.
 * 
 * @param rob the ROB
 * @return true if the ROB has space for another instruction, false otherwise
 */
bool rob_check_space(ROB *rob);

/**
 * Insert an instruction into the ROB at the tail pointer.
 * 
 * You must implement this function in part A of the assignment.
 * 
 * @param rob the ROB
 * @param inst the instruction to insert
 * @return the ID (index) of the newly inserted instruction in the ROB, or -1
 *         if there is no more space in the ROB
 */
int rob_insert(ROB *rob, InstInfo inst);

/**
 * Find the given instruction in the ROB and mark it as executing.
 * 
 * In part B, you will call this function when an instruction is scheduled for
 * execution.
 * 
 * You must implement this function in part A of the assignment.
 * 
 * @param rob the ROB
 * @param inst the instruction that is now executing
 */
void rob_mark_exec(ROB *rob, InstInfo inst);

/**
 * Find the given instruction in the ROB and mark it as having its output
 * ready (i.e., being ready to commit).
 * 
 * In part B, you will call this function when an instruction is finished
 * executing and is being written back.
 * 
 * You must implement this function in part A of the assignment.
 * 
 * @param rob the ROB
 * @param inst the instruction whose output is ready
 */
void rob_mark_ready(ROB *rob, InstInfo inst);

/**
 * Check if the instruction with the given tag (ID/index) has its output ready.
 * 
 * This is the same as asking if that instruction is ready to commit.
 * 
 * You must implement this function in part A of the assignment.
 * 
 * @param rob the ROB
 * @param tag the tag (ID/index) of the instruction to check
 * @return true if the instruction has its output ready, false if it is not or
 *         if there is no valid instruction at this tag in the ROB
 */
bool rob_check_ready(ROB *rob, int tag);

/**
 * Check if the instruction at the head of the ROB is ready to commit.
 * 
 * You must implement this function in part A of the assignment.
 * 
 * @param rob the ROB
 * @return true if the instruction at the head of the ROB is ready to commit,
 *         false if it is not or if there is no valid instruction at the head
 *         of the ROB
 */
bool rob_check_head(ROB *rob);

/**
 * Wake up instructions that are dependent on the instruction with the given
 * tag.
 * 
 * In part B, you will call this function during the writeback stage of the
 * pipeline: for each instruction that has finished executing, you will call
 * this function with its destination tag to indicate that the data with that
 * tag is now ready.
 * 
 * Then, for each source operand for each valid instruction in the ROB, if the
 * tag of the source operand matches the given tag, this function should mark
 * that source operand as ready.
 * 
 * You must implement this function in part A of the assignment.
 * 
 * @param rob the ROB
 * @param tag the tag of the instruction that has finished executing
 */
void rob_wakeup(ROB *rob, int tag);

/**
 * If the head entry of the ROB is ready to commit, remove that entry and
 * return the instruction contained there.
 * 
 * You must implement this function in part A of the assignment.
 * 
 * @param rob the ROB
 * @return the instruction that was previously at the head of the ROB
 */
InstInfo rob_remove_head(ROB *rob);

#endif
