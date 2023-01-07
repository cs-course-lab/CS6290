///////////////////////////////////////////////////////////////////////////////
// You will need to modify this file to implement parts B through F.         //
//                                                                           //
// In part B, it is intended that you implement the following functions:     //
// - memsys_access_modeBC() (used in parts B and C)                          //
// - memsys_l2_access() (used in parts B through F)                          //
//                                                                           //
// In part D, it is intended that you implement the following function:      //
// - memsys_access_modeDEF() (used in parts D, E, and F)                     //
///////////////////////////////////////////////////////////////////////////////

// memsys.cpp
// Defines the functions for the memory system.

#include "memsys.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
// You may add any other #include directives you need here, but make sure they
// compile on the reference machine!

///////////////////////////////////////////////////////////////////////////////
//                                 CONSTANTS                                 //
///////////////////////////////////////////////////////////////////////////////

/** The number of bytes in a page. */
#define PAGE_SIZE 4096

/** The hit time of the data cache in cycles. */
#define DCACHE_HIT_LATENCY 1

/** The hit time of the instruction cache in cycles. */
#define ICACHE_HIT_LATENCY 1

/** The hit time of the L2 cache in cycles. */
#define L2CACHE_HIT_LATENCY 10

///////////////////////////////////////////////////////////////////////////////
//                    EXTERNALLY DEFINED GLOBAL VARIABLES                    //
///////////////////////////////////////////////////////////////////////////////

/**
 * The current mode under which the simulation is running, corresponding to
 * which part of the lab is being evaluated.
 */
extern Mode SIM_MODE;

/** The number of bytes in a cache line. */
extern uint64_t CACHE_LINESIZE;

/** The replacement policy to use for the L1 data and instruction caches. */
extern ReplacementPolicy REPL_POLICY;

/** The size of the data cache in bytes. */
extern uint64_t DCACHE_SIZE;

/** The associativity of the data cache. */
extern uint64_t DCACHE_ASSOC;

/** The size of the instruction cache in bytes. */
extern uint64_t ICACHE_SIZE;

/** The associativity of the instruction cache. */
extern uint64_t ICACHE_ASSOC;

/** The size of the L2 cache in bytes. */
extern uint64_t L2CACHE_SIZE;

/** The associativity of the L2 cache. */
extern uint64_t L2CACHE_ASSOC;

/** The replacement policy to use for the L2 cache. */
extern ReplacementPolicy L2CACHE_REPL;

/** The number of cores being simulated. */
extern unsigned int NUM_CORES;

/**
 * The current clock cycle number.
 * 
 * This can be used as a timestamp for implementing the LRU replacement policy.
 */
extern uint64_t current_cycle;

///////////////////////////////////////////////////////////////////////////////
//                           FUNCTION DEFINITIONS                            //
///////////////////////////////////////////////////////////////////////////////

/**
 * Allocate and initialize the memory system.
 * 
 * This is implemented for you, but you may modify it as needed.
 * 
 * @return A pointer to the memory system.
 */
MemorySystem *memsys_new()
{
    MemorySystem *sys = (MemorySystem *)calloc(1, sizeof(MemorySystem));

    if (SIM_MODE == SIM_MODE_A)
    {
        sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE,
                                REPL_POLICY);
    }

    if (SIM_MODE == SIM_MODE_B || SIM_MODE == SIM_MODE_C)
    {
        sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE,
                                REPL_POLICY);
        sys->icache = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE,
                                REPL_POLICY);
        sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE,
                                 REPL_POLICY);
        sys->dram = dram_new();
    }

    if (SIM_MODE == SIM_MODE_DEF)
    {
        sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE,
                                 L2CACHE_REPL);
        sys->dram = dram_new();
        for (unsigned int i = 0; i < NUM_CORES; i++)
        {
            sys->dcache_coreid[i] = cache_new(DCACHE_SIZE, DCACHE_ASSOC,
                                              CACHE_LINESIZE, REPL_POLICY);
            sys->icache_coreid[i] = cache_new(ICACHE_SIZE, ICACHE_ASSOC,
                                              CACHE_LINESIZE, REPL_POLICY);
        }
    }

    return sys;
}

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
                       unsigned int core_id)
{
    uint64_t delay = 0;

    // All cache transactions happen at line granularity, so we convert the
    // byte address to a cache line address.
    uint64_t line_addr = addr / CACHE_LINESIZE;

    if (SIM_MODE == SIM_MODE_A)
    {
        // fprintf(stderr, "memsys::_access:: addr:%ld, type:%d, \n", addr, type);
        delay = memsys_access_modeA(sys, line_addr, type, core_id);

    }

    if (SIM_MODE == SIM_MODE_B || SIM_MODE == SIM_MODE_C)
    {
        delay = memsys_access_modeBC(sys, line_addr, type, core_id);
    }

    if (SIM_MODE == SIM_MODE_DEF)
    {
        delay = memsys_access_modeDEF(sys, line_addr, type, core_id);
    }

    // Update the statistics.
    if (type == ACCESS_TYPE_IFETCH)
    {
        sys->stat_ifetch_access++;
        sys->stat_ifetch_delay += delay;
    }

    if (type == ACCESS_TYPE_LOAD)
    {
        sys->stat_load_access++;
        sys->stat_load_delay += delay;
    }

    if (type == ACCESS_TYPE_STORE)
    {
        sys->stat_store_access++;
        sys->stat_store_delay += delay;
    }

    return delay;
}

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
                             AccessType type, unsigned int core_id)
{
    bool needs_dcache_access = false;
    bool is_write = false;

    if (type == ACCESS_TYPE_IFETCH)
    {
        // Ignore instruction fetches because there is no instruction cache in
        // this mode.
    }

    if (type == ACCESS_TYPE_LOAD)
    {
        needs_dcache_access = true;
        is_write = false;
    }

    if (type == ACCESS_TYPE_STORE)
    {
        needs_dcache_access = true;
        is_write = true;
    }

    if (needs_dcache_access)
    {
        CacheResult outcome = cache_access(sys->dcache, line_addr, is_write,
                                           core_id);
        if (outcome == MISS)
        {
            cache_install(sys->dcache, line_addr, is_write, core_id);
        }
    }

    // Timing is not simulated in Part A.
    return 0;
}

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
                              AccessType type, unsigned int core_id)
{
    uint64_t delay = 0;
    bool need_icache_access = false;
    bool need_dcache_access = false;
    bool is_write = false;
    if (type == ACCESS_TYPE_IFETCH)
    {
        need_icache_access = true;
        is_write = false;
        // TODO: Simulate the instruction fetch and update delay accordingly.
    }

    if (type == ACCESS_TYPE_LOAD)
    {
        need_dcache_access = true;
        is_write = false;
        // TODO: Simulate the data load and update delay accordingly.
    }

    if (type == ACCESS_TYPE_STORE)
    {
        need_dcache_access = true;
        is_write = true;
        // TODO: Simulate the data store and update delay accordingly.
    }

    if(need_icache_access)
    {
        CacheResult outcome_l1 = cache_access(sys->icache, line_addr, is_write,
                                           core_id);
        delay += ICACHE_HIT_LATENCY;
        if (outcome_l1 == MISS)
        {
            delay += memsys_l2_access(sys, line_addr, false, core_id);
            cache_install(sys->icache, line_addr, is_write, core_id);
            if (sys->icache->last_evicted_line.dirty){
                memsys_l2_access(sys, sys->icache->last_evicted_line_addr, true, core_id);
            }
        }   
    }

    if(need_dcache_access)
    {
        CacheResult outcome_l1 = cache_access(sys->dcache, line_addr, is_write,
                                           core_id);
        delay += DCACHE_HIT_LATENCY;
        if (outcome_l1 == MISS)
        {
            delay += memsys_l2_access(sys, line_addr, false, core_id);
            cache_install(sys->dcache, line_addr, is_write, core_id);
            if (sys->dcache->last_evicted_line.dirty){
                memsys_l2_access(sys, sys->dcache->last_evicted_line_addr, true, core_id);
            }
        }
    }
    return delay;
}

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
                          bool is_writeback, unsigned int core_id)
{
    uint64_t delay = L2CACHE_HIT_LATENCY;
    CacheResult outcome_l2 = cache_access(sys->l2cache, line_addr, is_writeback, core_id);
    if (outcome_l2 == MISS)
    {
        delay += dram_access(sys->dram, line_addr, false);
        cache_install(sys->l2cache, line_addr, is_writeback, core_id);
        if (sys->l2cache->last_evicted_line.dirty){
            dram_access(sys->dram, sys->l2cache->last_evicted_line_addr, true);
        }
    }

    // if (is_writeback){
    //     CacheResult outcome_l2 = cache_access(sys->l2cache, line_addr, is_writeback, core_id);
    //     if (outcome_l2 == MISS)
    //     {
    //         cache_install(sys->l2cache, line_addr, is_writeback, core_id);
    //         if (sys->l2cache->last_evicted_line.dirty){
    //             dram_access(sys->dram, sys->l2cache->last_evicted_line_addr, true);
    //         }

    //     }
    //      // no delay for writeback path
    // }
    // TODO: Perform the L2 cache access.

    // TODO: Use the dram_access() function to get the delay of an L2 miss.
    // TODO: Use the dram_access() function to perform writebacks to memory.
    //       Note that writebacks are done off the critical path.
    // This will help us track your memory reads and memory writes.

    return delay;
}

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
                               AccessType type, unsigned int core_id)
{
    uint64_t p_line_addr = 0;
    uint64_t vpn = (v_line_addr * CACHE_LINESIZE) / PAGE_SIZE;
    p_line_addr = (memsys_convert_vpn_to_pfn(sys, vpn, core_id) * PAGE_SIZE | (v_line_addr * CACHE_LINESIZE) % PAGE_SIZE) / CACHE_LINESIZE;
    // fprintf(stderr, "vpn:%016lx, ppn:%016lx, line:%016lx, phys_line:%016lx, core_id:%d\n", vpn, memsys_convert_vpn_to_pfn(sys, vpn, core_id), v_line_addr, p_line_addr, core_id);

    // TODO: First convert lineaddr from virtual (v) to physical (p) using the
    //       function memsys_convert_vpn_to_pfn(). Page size is defined to be
    //       4 KB, as indicated by the PAGE_SIZE constant.
    // Note: memsys_convert_vpn_to_pfn() operates at page granularity and
    //       returns a page number.

    uint64_t delay = 0;
    bool need_icache_access = false;
    bool need_dcache_access = false;
    bool is_write = false;
    if (type == ACCESS_TYPE_IFETCH)
    {
        need_icache_access = true;
        is_write = false;
        // TODO: Simulate the instruction fetch and update delay accordingly.
    }

    if (type == ACCESS_TYPE_LOAD)
    {
        need_dcache_access = true;
        is_write = false;
        // TODO: Simulate the data load and update delay accordingly.
    }

    if (type == ACCESS_TYPE_STORE)
    {
        need_dcache_access = true;
        is_write = true;
        // TODO: Simulate the data store and update delay accordingly.
    }

    if(need_icache_access)
    {
        CacheResult outcome_l1 = cache_access(sys->icache_coreid[core_id], p_line_addr, is_write, core_id);
        delay += ICACHE_HIT_LATENCY;
        if (outcome_l1 == MISS)
        {
            delay += memsys_l2_access(sys, p_line_addr, false, core_id);
            cache_install(sys->icache_coreid[core_id], p_line_addr, is_write, core_id);
            if (sys->icache_coreid[core_id]->last_evicted_line.dirty){
                memsys_l2_access(sys, sys->icache_coreid[core_id]->last_evicted_line_addr, true, core_id);
            }
        }
    }

    if(need_dcache_access)
    {
        CacheResult outcome_l1 = cache_access(sys->dcache_coreid[core_id], p_line_addr, is_write, core_id);
        delay += DCACHE_HIT_LATENCY;
        if (outcome_l1 == MISS)
        {
            delay += memsys_l2_access(sys, p_line_addr, false, core_id);
            cache_install(sys->dcache_coreid[core_id], p_line_addr, is_write, core_id);
            if (sys->dcache_coreid[core_id]->last_evicted_line.dirty){
                memsys_l2_access(sys, sys->dcache_coreid[core_id]->last_evicted_line_addr, true, core_id);
            }
        }
    }

    // if(need_dcache_access)
    // {
    //     CacheResult outcome_l1 = cache_access(sys->dcache, line_addr, is_write,
    //                                        core_id);
    //     delay += DCACHE_HIT_LATENCY;
    //     if (outcome_l1 == MISS)
    //     {
    //         delay += memsys_l2_access(sys, line_addr, false, core_id);
    //         cache_install(sys->dcache, line_addr, is_write, core_id);
    //         if (sys->dcache->last_evicted_line.dirty){
    //             memsys_l2_access(sys, sys->dcache->last_evicted_line_addr, true, core_id);
    //         }
    //     }
    // }

    return delay;
}

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
                                   unsigned int core_id)
{
    assert(NUM_CORES == 2);
    uint64_t tail = vpn & 0x000fffff;
    uint64_t head = vpn >> 20;
    uint64_t pfn = tail + (core_id << 21) + (head << 21);
    return pfn;
}

/**
 * Print the statistics of the memory system.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param dram The memory system to print the statistics of.
 */
void memsys_print_stats(MemorySystem *sys)
{
    double ifetch_delay_avg = 0;
    double load_delay_avg = 0;
    double store_delay_avg = 0;

    if (sys->stat_ifetch_access)
    {
        ifetch_delay_avg = (double)(sys->stat_ifetch_delay) / (double)(sys->stat_ifetch_access);
    }

    if (sys->stat_load_access)
    {
        load_delay_avg = (double)(sys->stat_load_delay) / (double)(sys->stat_load_access);
    }

    if (sys->stat_store_access)
    {
        store_delay_avg = (double)(sys->stat_store_delay) / (double)(sys->stat_store_access);
    }

    printf("\n");
    printf("MEMSYS_IFETCH_ACCESS   \t\t : %10llu\n", sys->stat_ifetch_access);
    printf("MEMSYS_LOAD_ACCESS     \t\t : %10llu\n", sys->stat_load_access);
    printf("MEMSYS_STORE_ACCESS    \t\t : %10llu\n", sys->stat_store_access);
    printf("MEMSYS_IFETCH_AVGDELAY \t\t : %10.3f\n", ifetch_delay_avg);
    printf("MEMSYS_LOAD_AVGDELAY   \t\t : %10.3f\n", load_delay_avg);
    printf("MEMSYS_STORE_AVGDELAY  \t\t : %10.3f\n", store_delay_avg);

    if (SIM_MODE == SIM_MODE_A)
    {
        cache_print_stats(sys->dcache, "DCACHE");
    }

    if ((SIM_MODE == SIM_MODE_B) || (SIM_MODE == SIM_MODE_C))
    {
        cache_print_stats(sys->icache, "ICACHE");
        cache_print_stats(sys->dcache, "DCACHE");
        cache_print_stats(sys->l2cache, "L2CACHE");
        dram_print_stats(sys->dram);
    }

    if (SIM_MODE == SIM_MODE_DEF)
    {
        assert(NUM_CORES == 2);
        cache_print_stats(sys->icache_coreid[0], "ICACHE_0");
        cache_print_stats(sys->dcache_coreid[0], "DCACHE_0");
        cache_print_stats(sys->icache_coreid[1], "ICACHE_1");
        cache_print_stats(sys->dcache_coreid[1], "DCACHE_1");
        cache_print_stats(sys->l2cache, "L2CACHE");
        dram_print_stats(sys->dram);
    }
}
