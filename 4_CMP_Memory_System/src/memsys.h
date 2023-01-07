///////////////////////////////////////////////////////////////////////////////
// You shouldn't need to modify this file.                                   //
///////////////////////////////////////////////////////////////////////////////

// memsys.h
// Declares the memory system structure and related functions.

#ifndef __MEMSYS_H__
#define __MEMSYS_H__

#include "types.h"
#include "cache.h"
#include "dram.h"

///////////////////////////////////////////////////////////////////////////////
//                              DATA STRUCTURES                              //
///////////////////////////////////////////////////////////////////////////////

typedef struct MemorySystem
{
    /** A cache for data accesses. Used in parts A, B, and C. */
    Cache *dcache;
    /** A cache for instruction fetches. Used in parts A, B, and C. */
    Cache *icache;

    /**
     * The data caches for each core in a multicore system. Used in parts D,
     * E, and F.
     */
    Cache *dcache_coreid[2];
    /**
     * The instruction caches for each core in a multicore system. Used in
     * parts D, E, and F.
     */
    Cache *icache_coreid[2];

    /** The shared L2 cache. Used in parts B, C, D, E, and F. */
    Cache *l2cache;
    /** The DRAM module. Used in parts B, C, D, E, and F. */
    DRAM *dram;

    /**
     * The total number of times the memory system was accessed for an
     * instruction fetch. This is updated for you in memsys_access().
     */
    unsigned long long stat_ifetch_access;
    /**
     * The total number of times the memory system was accessed for a data
     * load. This is updated for you in memsys_access().
     */
    unsigned long long stat_load_access;
    /**
     * The total number of times the memory system was accessed for a data
     * store. This is updated for you in memsys_access().
     */
    unsigned long long stat_store_access;
    /**
     * The total number of cycles spent on instruction fetches. This is updated
     * for you in memsys_access().
     */
    uint64_t stat_ifetch_delay;
    /**
     * The total number of cycles spent on data loads. This is updated for you
     * in memsys_access().
     */
    uint64_t stat_load_delay;
    /**
     * The total number of cycles spent on data stores. This is updated for you
     * in memsys_access().
     */
    uint64_t stat_store_delay;
} MemorySystem;

///////////////////////////////////////////////////////////////////////////////
//                            FUNCTION PROTOTYPES                            //
///////////////////////////////////////////////////////////////////////////////

/**
 * Allocate and initialize the memory system.
 * 
 * This is implemented for you, but you may modify it as needed.
 * 
 * @return A pointer to the memory system.
 */
MemorySystem *memsys_new();

/**
 * Access the given memory address from an instruction fetch or load/store.
 * 
 * Return the delay in cycles incurred by this memory access. Also update the
 * statistics accordingly.
 * 
 * This is implemented for you, but you may modify it as needed.
 * 
 * @param sys The memory system to use for the access.
 * @param addr The address to access (in bytes).
 * @param type The type of memory access.
 * @param core_id The CPU core ID that requested this access.
 * @return The delay in cycles incurred by this memory access.
 */
uint64_t memsys_access(MemorySystem *sys, uint64_t addr, AccessType type,
                       unsigned int core_id);

/**
 * In mode A, access the given memory address from a load or store.
 * 
 * Return the simulated delay in cycles for this memory access, which is 0
 * because timing is not simulated in this mode.
 * 
 * This is implemented for you, but you may modify it as needed.
 * 
 * @param sys The memory system to use for the access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param type The type of memory access.
 * @param core_id The CPU core ID that requested this access.
 * @return Always 0 in this mode.
 */
uint64_t memsys_access_modeA(MemorySystem *sys, uint64_t line_addr,
                             AccessType type, unsigned int core_id);

/**
 * In mode B or C, access the given memory address from an instruction fetch or
 * load/store.
 * 
 * Return the delay in cycles incurred by this memory access. It is intended
 * that the calling function will be responsible for updating the statistics
 * accordingly.
 * 
 * This is intended to be implemented in part B.
 * 
 * @param sys The memory system to use for the access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param type The type of memory access.
 * @param core_id The CPU core ID that requested this access.
 * @return The delay in cycles incurred by this memory access.
 */
uint64_t memsys_access_modeBC(MemorySystem *sys, uint64_t line_addr,
                              AccessType type, unsigned int core_id);

/**
 * Access the given address through the shared L2 cache.
 * 
 * Return the delay in cycles incurred by the L2 (and possibly DRAM) access.
 * 
 * This is intended to be implemented in part B and used in parts B through F
 * for icache misses, dcache misses, and dcache writebacks.
 * 
 * @param sys The memory system to use for the access.
 * @param line_addr The (physical) address of the cache line to access (in
 *                  units of the cache line size, i.e., excluding the line
 *                  offset bits).
 * @param is_writeback Whether this access is a writeback from an L1 cache.
 * @param core_id The CPU core ID that requested this access.
 * @return The delay in cycles incurred by this access.
 */
uint64_t memsys_l2_access(MemorySystem *sys, uint64_t line_addr,
                          bool is_writeback, unsigned int core_id);

/**
 * In mode D, E, or F, access the given virtual address from an instruction
 * fetch or load/store.
 * 
 * Note that you will need to access the per-core icache and dcache. Also note
 * that all caches are physically indexed and physically tagged.
 * 
 * Return the delay in cycles incurred by this memory access. It is intended
 * that the calling function will be responsible for updating the statistics
 * accordingly.
 * 
 * This is intended to be implemented in part D.
 * 
 * @param sys The memory system to use for the access.
 * @param v_line_addr The virtual address of the cache line to access (in units
 *                    of the cache line size, i.e., excluding the line offset
 *                    bits).
 * @param type The type of memory access.
 * @param core_id The CPU core ID that requested this access.
 * @return The delay in cycles incurred by this memory access.
 */
uint64_t memsys_access_modeDEF(MemorySystem *sys, uint64_t v_line_addr,
                               AccessType type, unsigned int core_id);

/**
 * Convert the given virtual page number (VPN) to its corresponding physical
 * frame number (PFN; also known as physical page number, or PPN).
 * 
 * This is implemented for you and shouldn't need to be modified.
 * 
 * Note that you will need additional operations to obtain the VPN from the
 * v_line_addr and to get the physical line_addr using the PFN.
 * 
 * @param sys The memory system being used.
 * @param vpn The virtual page number to convert.
 * @param core_id The CPU core ID that requested this access.
 * @return The physical frame number corresponding to the given VPN.
 */
uint64_t memsys_convert_vpn_to_pfn(MemorySystem *sys, uint64_t vpn,
                                   unsigned int core_id);

/**
 * Print the statistics of the memory system.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param dram The memory system to print the statistics of.
 */
void memsys_print_stats(MemorySystem *sys);

#endif // __MEMSYS_H__
