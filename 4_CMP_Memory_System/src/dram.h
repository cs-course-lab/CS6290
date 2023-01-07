///////////////////////////////////////////////////////////////////////////////
// You will need to modify this file to implement part C.                    //
///////////////////////////////////////////////////////////////////////////////

// dram.h
// Contains declarations of data structures and functions used to implement
// DRAM.

#ifndef __DRAM_H__
#define __DRAM_H__
#define NUM_BANKS 16

#include "types.h"
// You may add any other #include directives you need here, but make sure they
// compile on the reference machine!

///////////////////////////////////////////////////////////////////////////////
//                              DATA STRUCTURES                              //
///////////////////////////////////////////////////////////////////////////////

// TODO: Define any other data structures you need here.
// Refer to Appendix B for details on data structures you will need here.
typedef struct RowBuffer
{
    bool valid;
    uint64_t rowID;

}RowBuffer;


/** A DRAM module. */
typedef struct DRAM
{
    // TODO: Define any other fields you need here.
    // Refer to Appendix B for details on other fields you will need here.
    RowBuffer rowBuffer[NUM_BANKS];
    /**
     * The total number of times DRAM was accessed for a read.
     * You should initialize this to 0 and update it for every DRAM read!
     */
    unsigned long long stat_read_access;

    /**
     * The total number of cycles spent on DRAM reads.
     * This only includes the latency incurred at the DRAM module itself.
     * You should initialize this to 0 and update it for every DRAM read!
     */
    uint64_t stat_read_delay;

    /**
     * The total number of times DRAM was accessed for a write.
     * You should initialize this to 0 and update it for every DRAM write!
     */
    unsigned long long stat_write_access;

    /**
     * The total number of cycles spent on DRAM writes.
     * This only includes the latency incurred at the DRAM module itself.
     * You should initialize this to 0 and update it for every DRAM write!
     */
    uint64_t stat_write_delay;
} DRAM;

/** Possible page policies for DRAM. */
typedef enum DRAMPolicyEnum
{
    OPEN_PAGE = 0,  // The DRAM uses an open-page policy.
    CLOSE_PAGE = 1, // The DRAM uses a close-page policy.
} DRAMPolicy;

///////////////////////////////////////////////////////////////////////////////
//                            FUNCTION PROTOTYPES                            //
///////////////////////////////////////////////////////////////////////////////

// Please note:
// Implementing the following functions as described will be useful in
// completing the lab.

// However, if you would like to deviate from the suggested implementation,
// you are free to do so by adding, removing, or modifying declarations as you
// see fit.

// The only restriction is that you must not remove dram_print_stats() or
// modify its output format, since its output will be used for grading.

/**
 * Allocate and initialize a DRAM module.
 * 
 * This is intended to be implemented in part B.
 * 
 * @return A pointer to the DRAM module.
 */
DRAM *dram_new();

/**
 * Access the DRAM at the given cache line address.
 * 
 * Return the delay in cycles incurred by this DRAM access. Also update the
 * DRAM statistics accordingly.
 * 
 * Note that the address is given in units of the cache line size!
 * 
 * This is intended to be implemented in parts B and C. In parts C through F,
 * you may delegate logic to the dram_access_mode_CDEF() functions.
 * 
 * @param dram The DRAM module to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size).
 * @param is_dram_write Whether this access writes to DRAM.
 * @return The delay in cycles incurred by this DRAM access.
 */
uint64_t dram_access(DRAM *dram, uint64_t line_addr, bool is_dram_write);

/**
 * For parts C through F, access the DRAM at the given cache line address.
 * 
 * Return the delay in cycles incurred by this DRAM access. It is intended that
 * the calling function will be responsible for updating DRAM statistics
 * accordingly.
 * 
 * Note that the address is given in units of the cache line size!
 * 
 * This is intended to be implemented in part C.
 * 
 * @param dram The DRAM module to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size).
 * @param is_dram_write Whether this access writes to DRAM.
 * @return The delay in cycles incurred by this DRAM access.
 */
uint64_t dram_access_mode_CDEF(DRAM *dram, uint64_t line_addr,
                               bool is_dram_write);

/**
 * Print the statistics of the DRAM module.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param dram The DRAM module to print the statistics of.
 */
void dram_print_stats(DRAM *dram);

#endif // __DRAM_H__
