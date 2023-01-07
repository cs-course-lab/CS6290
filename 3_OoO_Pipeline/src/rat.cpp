///////////////////////////////////////////////////////////
// You must modify this file to implement the following: //
// - int rat_get_remap(RAT *rat, int arf_id)             //
// - void rat_set_remap(RAT *t, int arf_id, int prf_id)  //
// - void rat_reset_entry(RAT *t, int arf_id)            //
///////////////////////////////////////////////////////////

// rat.cpp
// Implements the register alias table.

#include "rat.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Allocate and initialize a new RAT.
 * 
 * This function has been implemented for you.
 * 
 * @return a pointer to a newly allocated RAT
 */
RAT *rat_init()
{
    RAT *rat = (RAT *)calloc(1, sizeof(RAT));
    for (int i = 0; i < MAX_ARF_REGS; i++)
    {
        rat->entries[i].valid = false;
    }
    return rat;
}

/**
 * Print out the state of the RAT for debugging purposes.
 * 
 * This function is called automatically in pipe_print_state(), but you may
 * also use it to help debug your RAT implementation. If you choose to do so,
 * please remove calls to this function before submitting the lab.
 * 
 * @param rat the RAT
 */
void rat_print_state(RAT *rat)
{
    int ii = 0;
    printf("Current RAT state:\n");
    printf("Entry  Valid  prf_id\n");
    for (ii = 0; ii < MAX_ARF_REGS; ii++)
    {
        if(rat->entries[ii].valid){
            printf("%5d ::  %d \t\t", ii, rat->entries[ii].valid);
            printf("%5d \n", (int)rat->entries[ii].prf_id);

        }
    }
    printf("\n");
}

/**
 * Get the PRF ID (i.e., ID of ROB entry) of the latest value of a register.
 * 
 * If the register is not currently aliased (i.e., its latest value is already
 * committed and thus resides in the ARF), return -1.
 * 
 * You must implement this function in part A of the assignment.
 * 
 * @param rat the RAT
 * @param arf_id the ID of the architectural register to get the alias of
 * @return the ID of the ROB entry whose output this register is aliased to, or
 *         -1 if the register is not aliased
 */
int rat_get_remap(RAT *rat, int arf_id)
{
    // fprintf(stderr, "rat_get_remap:: %d\n", arf_id);
    if (rat->entries[arf_id].valid){
        return rat->entries[arf_id].prf_id;
    }
    return -1;
    // TODO: Access the RAT entry at the correct index.
    // TODO: Return the PRF ID if the register is aliased or -1 otherwise.
}

/**
 * Set the PRF ID (i.e., ID of ROB entry) that a register should be aliased to.
 * 
 * In part B, you will call this to remap the destination register of each
 * newly-issued instruction to the ROB entry where that instruction resides.
 * 
 * You must implement this function in part A of the assignment.
 * 
 * @param rat the RAT
 * @param arf_id the ID of the architectural register to set the alias of
 * @param prf_id the ID of the ROB entry whose output this register should be
 *               aliased to
 */
void rat_set_remap(RAT *rat, int arf_id, int prf_id)
{
    // fprintf(stderr, "rat_set_remap:: %d\n", arf_id);
    rat->entries[arf_id].prf_id = prf_id;
    rat->entries[arf_id].valid = true;
    // TODO: Access the RAT entry at the correct index.
    // TODO: Set the correct values on that entry.
}

/**
 * Reset the alias of a register.
 * 
 * In part B, you will call this to indicate that the instruction that a
 * register was previously aliased to has been committed. This means that the
 * register should no longer be aliased to any ROB entry, as its latest value
 * now resides in the ARF.
 * 
 * @param rat the RAT
 * @param arf_id the ID of the architectural register to reset the alias of
 */
void rat_reset_entry(RAT *rat, int arf_id)
{
    // fprintf(stderr, "rat_reset_entry:: %d\n", arf_id);
    
    rat->entries[arf_id].valid = false;
    // TODO: Access the RAT entry at the correct index.
    // TODO: Make it invalid.
}
