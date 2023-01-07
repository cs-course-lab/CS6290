///////////////////////////////////////////////////////////////////////////////
// You will need to modify this file to implement part A and, for extra      //
// credit, parts E and F.                                                    //
///////////////////////////////////////////////////////////////////////////////

// cache.cpp
// Defines the functions used to implement the cache.

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <utility>
// You may add any other #include directives you need here, but make sure they
// compile on the reference machine!

///////////////////////////////////////////////////////////////////////////////
//                    EXTERNALLY DEFINED GLOBAL VARIABLES                    //
///////////////////////////////////////////////////////////////////////////////

/**
 * The current clock cycle number.
 * 
 * This can be used as a timestamp for implementing the LRU replacement policy.
 */
extern uint64_t current_cycle;

/**
 * For static way partitioning, the quota of ways in each set that can be
 * assigned to core 0.
 * 
 * The remaining number of ways is the quota for core 1.
 * 
 * This is used to implement extra credit part E.
 */
extern unsigned int SWP_CORE0_WAYS;

extern Mode SIM_MODE;
///////////////////////////////////////////////////////////////////////////////
//                           FUNCTION DEFINITIONS                            //
///////////////////////////////////////////////////////////////////////////////

uint64_t DWP_access_total_per_core[2] = {0, 0};
uint64_t DWP_access_count_per_core[2] = {0, 0};

float DWP_prior_miss_rate[2] = {0, 0};
int DWP_prior_partition = 0;



int DWP_partition;

int DWP_monitor_count = 0;

// int DWP_prior_miss_count = 0;
// int DWP_current_miss_count = 0;

// As described in cache.h, you are free to deviate from the suggested
// implementation as you see fit.


// The only restriction is that you must not remove cache_print_stats() or
// modify its output format, since its output will be used for grading.

/**
 * Allocate and initialize a cache.
 * 
 * This is intended to be implemented in part A.
 *
 * @param size The size of the cache in bytes.
 * @param associativity The associativity of the cache.
 * @param line_size The size of a cache line in bytes.
 * @param replacement_policy The replacement policy of the cache.
 * @return A pointer to the cache.
 */
Cache *cache_new(uint64_t size, uint64_t associativity, uint64_t line_size,
                 ReplacementPolicy replacement_policy)
{
    Cache* newCache = (Cache *)calloc(1, sizeof(Cache));

    newCache->numOfSets = size / associativity / line_size;
    newCache->numOfWays = associativity;
    // newCache->numOfWays = 1;
    newCache->sizeOfLine = line_size;
    newCache->replPolicy = replacement_policy;
    newCache->last_evicted_line.valid = 0;
    newCache->last_evicted_line_addr = 0;
    // fprintf(stderr, "totalsize:%d, numOfSets:%d, numOfWays:%d, sizeOfLine:%d\n", size, newCache->numOfSets, newCache->numOfWays, newCache->sizeOfLine);
    // total cache size: size(B) 
    // total cache line size: size(B) / associativity / line_size
    newCache->set= (CacheSet *) calloc(newCache->numOfSets, sizeof(CacheSet));
    for(int i = 0; i < newCache->numOfSets; i ++){
        newCache->set[i].line = (CacheLine *) calloc(associativity, sizeof(CacheLine));
    }
    if (replacement_policy == DWP){
        DWP_partition = newCache->numOfWays / 2;
    }
    // total: size
    // per asso: size / associativity
    // # of sets: size / associativity / line_size
    // # of Ways: associativity

    newCache->tagMask = (0xffffffffffffffff << (int) (log2(newCache->numOfSets))) % 0xffffffffffffffff;
    newCache->indexMask = (0xffffffffffffffff) & ~(newCache->tagMask);
    

    // fprintf(stderr, "tagMask:%016lx, indexMask:%016lx\n", newCache->tagMask, newCache->indexMask);

    newCache->stat_dirty_evicts = 0;
    newCache->stat_read_access = 0;
    newCache->stat_read_miss = 0;
    newCache->stat_write_access = 0;
    newCache->stat_write_miss = 0;

    // fprintf(stderr, "Allocated CacheSet, tagMask:%016lx, indexMask:%016lx, offsetMask:%016lx\n", (newCache->tagMask), newCache->indexMask, newCache->offsetMask);
    return newCache;
    // TODO: Allocate memory to the data structures and initialize the required
    //       fields. (You might want to use calloc() for this.)
}

uint64_t get_tag(Cache *c, uint64_t line_addr){
    // return line_addr / c->numOfSets;
    return (c->tagMask & line_addr) >> __builtin_ctzll(c->tagMask);
}

uint64_t get_index(Cache *c, uint64_t line_addr){
    // return line_addr % c->numOfSets;
    // fprintf(stderr, "index:%lu\n", (c->indexMask & line_addr) >> __builtin_ctzll(c->indexMask));
    return (c->indexMask & line_addr) >> __builtin_ctzll(c->indexMask);
}

uint64_t get_line_addr(Cache *c, int setNo, int lineNo){
    return ((c->set[setNo].line[lineNo].tag << __builtin_ctzll(c->tagMask)) & c->tagMask) | ((setNo << __builtin_ctzll(c->indexMask)) & c->indexMask);
}

/**
 * Access the cache at the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this access is a write.
 * @param core_id The CPU core ID that requested this access.
 * @return Whether the cache access was a hit or a miss.
 */
CacheResult cache_access(Cache *c, uint64_t line_addr, bool is_write,
                         unsigned int core_id)
{
    // fprintf(stderr, "cache::access ");
    // fprintf(stderr, "tag:%d, set:%d, offset:%d, core:%u\n", get_tag(c, line_addr), get_index(c, line_addr), get_offset(c, line_addr), core_id);
    CacheResult result = MISS;
    int lineNo = -1;
    int setNo = get_index(c, line_addr);
    for(int line = 0; line < c->numOfWays; line++){
        if(c->set[setNo].line[line].valid && (c->set[setNo].line[line].tag == get_tag(c, line_addr))){
            lineNo = line;
            break;
        }
    }

    if (lineNo != -1)
    { // Cache Hit
        // fprintf(stderr, "cache::hit, setNo:%ld, lineNo:%ld\n", setNo, lineNo);
        c->set[setNo].line[lineNo].lastAccessTime = current_cycle;
        // fprintf(stderr, "cache::hit\n");

        // todo: What to do with core_id? for D, E, F
        if(c->set[setNo].line[lineNo].coreID != core_id){
            c->set[setNo].line[lineNo].dirty = true;
        }
        
        c->set[setNo].line[lineNo].coreID = core_id;
        // todo: What to do with core_id? for D, E, F
        // if(c->set[setNo].line[lineNo].dirty){
        //     c->stat_dirty_evicts;
        // }

        if (is_write)
        {   
            c->set[setNo].line[lineNo].dirty = true;
            // TODO: If is_write is true, mark the resident line as dirty.
            c->stat_write_access ++;
        }
        else
        {
            c->stat_read_access ++;
        }
        // fprintf(stderr, "cache::hit\n");
    // TODO: Return HIT if the access hits in the cache, and MISS otherwise.
        result = HIT;
    }
    else
    {
        if (!is_write){
            c->stat_read_miss ++;
            c->stat_read_access ++;
        }
        else{
            c->stat_write_miss ++;
            c->stat_write_access ++;
        }
    }
    // TODO: Update the appropriate cache statistics.
    if(c->replPolicy == DWP){
        DWP_access_total_per_core[core_id] ++;
    }

    return result;


}

/**
 * Install the cache line with the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to install the line into.
 * @param line_addr The address of the cache line to install (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this install is triggered by a write.
 * @param core_id The CPU core ID that requested this access.
 */
void cache_install(Cache *c, uint64_t line_addr, bool is_write,
                   unsigned int core_id)
{
    // fprintf(stderr, "cache::install:: \n");
    int victim_way;
    int set_index = (int) get_index(c, line_addr);

    victim_way = cache_find_victim(c, set_index, core_id);
    // fprintf(stderr, "WHERE\n");
    // TODO: Use cache_find_victim() to determine the victim line to evict.
    
    // TODO: Copy it into a last_evicted_line field in the cache in order to
    //       track writebacks.

    c->last_evicted_line.dirty = c->set[set_index].line[victim_way].dirty;
    c->last_evicted_line_addr = get_line_addr(c, set_index, victim_way);
    // TODO: Initialize the victim entry with the line to install.
    c->set[set_index].line[victim_way].coreID = core_id;
    c->set[set_index].line[victim_way].valid = true;
    c->set[set_index].line[victim_way].dirty = false;
    if(is_write)
        c->set[set_index].line[victim_way].dirty = true;
    // fprintf(stderr, "WHERE\n");
    c->set[set_index].line[victim_way].tag = get_tag(c, line_addr);
    c->set[set_index].line[victim_way].lastAccessTime = current_cycle;
    // fprintf(stderr, "cache::install:: end\n");
    // TODO: Update the appropriate cache statistics.
}

/**
 * Find which way in a given cache set to replace when a new cache line needs
 * to be installed. This should be chosen according to the cache's replacement
 * policy.
 * 
 * The returned victim can be valid (non-empty), in which case the calling
 * function is responsible for evicting the cache line from that victim way.
 * 
 * This is intended to be initially implemented in part A and, for extra
 * credit, extended in parts E and F.
 * 
 * @param c The cache to search.
 * @param set_index The index of the cache set to search.
 * @param core_id The CPU core ID that requested this access.
 * @return The index of the victim way.
 */
unsigned int cache_find_victim(Cache *c, unsigned int set_index,
                               unsigned int core_id)
{
    // TODO: Find a victim way in the given cache set according to the cache's
    //       replacement policy.
    // fprintf(stderr, "cache::find_victim\n");
    if (SIM_MODE == SIM_MODE_A || SIM_MODE == SIM_MODE_B || SIM_MODE == SIM_MODE_C){
        if (c->replPolicy == LRU){
            uint64_t min_cycle = current_cycle + 1;
            int min_way = -1;
            for (int i = 0; i < c->numOfWays; i ++){
                if (c->set[set_index].line[i].valid){
                    if (c->set[set_index].line[i].lastAccessTime < min_cycle){
                        min_cycle = c->set[set_index].line[i].lastAccessTime;
                        min_way = i;
                    }
                }else{
                    // return directly with the empty set
                    // fprintf(stderr, "Cache::find_victim::empty victim!\n");
                    return i;
                }
            }
            assert(min_way != -1);
            if (c->set[set_index].line[min_way].dirty){
                c->stat_dirty_evicts ++;
            }
            return min_way;
        }
        else if(c->replPolicy == RANDOM){
            for (int i = 0; i < c->numOfWays; i ++){
                if (!c->set[set_index].line[i].valid){
                    // fprintf(stderr, "Cache::find_victim::empty victim!\n");
                    return i;
                }
            }
            unsigned int i = rand() % c->numOfWays;
            if (c->set[set_index].line[i].dirty){
                c->stat_dirty_evicts ++;
            }
            return i; 
        }else{
            fprintf(stderr, "cache:: WRONG SIM_MODE and REPL_POLICY!\n");
            assert(false);
        }
    // TODO: In part A, implement the LRU and random replacement policies.
    }
    else if(SIM_MODE == SIM_MODE_DEF){
        if (c->replPolicy == LRU){
            uint64_t min_cycle = current_cycle + 1;
            int min_way = -1;
            int partitionOfWayStart = 0;
            int partitionOfWayEnd = c->numOfWays;
            // fprintf(stderr, "core_id:%d, start:%d, end:%d\n", core_id, partitionOfWayStart, partitionOfWayEnd);
            for (int i = partitionOfWayStart; i < partitionOfWayEnd; i ++){
                if (c->set[set_index].line[i].valid){
                    if (c->set[set_index].line[i].lastAccessTime < min_cycle){
                        min_cycle = c->set[set_index].line[i].lastAccessTime;
                        min_way = i;
                    }
                }else{
                    // return directly with the empty set
                    // fprintf(stderr, "Cache::find_victim::empty victim!\n");
                    return i;
                }
            }
            assert(min_way != -1);
            if (c->set[set_index].line[min_way].dirty){
                c->stat_dirty_evicts ++;
            }
            return min_way;
        }
        else if (c->replPolicy == RANDOM){
            for (int i = 0; i < c->numOfWays; i ++){
                if (!c->set[set_index].line[i].valid){
                    // fprintf(stderr, "Cache::find_victim::empty victim!\n");
                    return i;
                }
            }
            unsigned int i = rand() % c->numOfWays;
            if (c->set[set_index].line[i].dirty){
                c->stat_dirty_evicts ++;
            }
            return i; 
        }
        else if (c->replPolicy == SWP){
            uint64_t min_cycle = current_cycle + 1;
            int min_way = -1;
            int partitionOfWayStart = 0;
            int partitionOfWayEnd = 0;
            if (SWP_CORE0_WAYS == 0){
                partitionOfWayStart = 0;
                partitionOfWayEnd = c->numOfWays;
            }else{
                if (core_id == 0){
                    partitionOfWayStart = 0;
                    partitionOfWayEnd = SWP_CORE0_WAYS;
                }else{
                    partitionOfWayStart = SWP_CORE0_WAYS;
                    partitionOfWayEnd = c->numOfWays;
                }
            }
            // fprintf(stderr, "core_id:%d, start:%d, end:%d\n", core_id, partitionOfWayStart, partitionOfWayEnd);
            for (int i = partitionOfWayStart; i < partitionOfWayEnd; i ++){
                if (c->set[set_index].line[i].valid){
                    if (c->set[set_index].line[i].lastAccessTime < min_cycle){
                        min_cycle = c->set[set_index].line[i].lastAccessTime;
                        min_way = i;
                    }
                }else{
                    // return directly with the empty set
                    // fprintf(stderr, "Cache::find_victim::empty victim!\n");
                    return i;
                }
            }
            assert(min_way != -1);
            if (c->set[set_index].line[min_way].dirty){
                c->stat_dirty_evicts ++;
            }
            return min_way;
        }
        else if (c->replPolicy == DWP){
            DWP_access_count_per_core[core_id] ++;
            uint64_t min_cycle = current_cycle + 1;
            int min_way = -1;
            int partitionOfWayStart = 0;
            int partitionOfWayEnd = 0;
            if (DWP_access_count_per_core[0] == 0 || DWP_access_count_per_core[1] == 0){
            }else {
                if (DWP_access_total_per_core[0] + DWP_access_total_per_core[1] > DWP_monitor_count * 10000){
                    // fprintf(stderr, "\ntotal access count:%ld, Miss rate 0: %f, Miss rate 1: %f partition: %d\n", DWP_access_total_per_core[0] + DWP_access_total_per_core[1], ((float)DWP_access_count_per_core[0] / DWP_access_total_per_core[0]) , ((float)DWP_access_count_per_core[1] / DWP_access_total_per_core[1]), DWP_partition);
                    DWP_monitor_count++;
                    if (DWP_prior_miss_rate[0] + DWP_prior_miss_rate[1] < ((float)DWP_access_count_per_core[0] / DWP_access_total_per_core[0]) + ((float)DWP_access_count_per_core[1] / DWP_access_total_per_core[1])){
                        // compare prior miss rate with current miss rate, if it is grown, go back to prior partition direction
                        if (DWP_prior_partition <= DWP_partition){
                            DWP_partition --;
                        }else{
                            DWP_partition ++;
                        }
                    }else if (DWP_prior_miss_rate[0] + DWP_prior_miss_rate[1] > ((float)DWP_access_count_per_core[0] / DWP_access_total_per_core[0]) + ((float)DWP_access_count_per_core[1] / DWP_access_total_per_core[1])){
                        if (DWP_prior_partition >= DWP_partition){
                            DWP_partition --;
                        }else{
                            DWP_partition ++;
                        }
                    }else{

                    }
                    DWP_prior_miss_rate[0] = ((float)DWP_access_count_per_core[0] / DWP_access_total_per_core[0]) ; 
                    DWP_prior_miss_rate[1] = ((float)DWP_access_count_per_core[1] / DWP_access_total_per_core[1]) ; 
                }
                DWP_partition = DWP_partition <= 0 ? 1 : DWP_partition;
                DWP_partition = DWP_partition >= c->numOfWays ? c->numOfWays - 1 : DWP_partition;
            }
            if (core_id == 0){
                partitionOfWayStart = 0;
                partitionOfWayEnd = DWP_partition;
            }else{
                partitionOfWayStart = DWP_partition;
                partitionOfWayEnd = c->numOfWays;
            }
            // fprintf(stderr, "core_id:%d, start:%d, end:%d, accessMissRatio:%f, otherCoreMissRatio:%f\n", core_id, partitionOfWayStart, partitionOfWayEnd, (float)DWP_access_count_per_core[core_id] / DWP_access_total_per_core[core_id], (float)DWP_access_count_per_core[core_id ^ 1] / DWP_access_total_per_core[core_id ^ 1]);
            for (int i = partitionOfWayStart; i < partitionOfWayEnd; i ++){
                if (c->set[set_index].line[i].valid){
                    if (c->set[set_index].line[i].lastAccessTime < min_cycle){
                        min_cycle = c->set[set_index].line[i].lastAccessTime;
                        min_way = i;
                    }
                }else{
                    // return directly with the empty set
                    // fprintf(stderr, "Cache::find_victim::empty victim!\n");
                    return i;
                }
            }
            assert(min_way != -1);
            if (c->set[set_index].line[min_way].dirty){
                c->stat_dirty_evicts ++;
            }
            return min_way;
        }
        else{
            exit(1);
        }

    // TODO: In part E, for extra credit, implement static way partitioning.
    // TODO: In part F, for extra credit, implement dynamic way partitioning.

    }


}

/**
 * Print the statistics of the given cache.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param c The cache to print the statistics of.
 * @param label A label for the cache, which is used as a prefix for each
 *              statistic.
 */
void cache_print_stats(Cache *c, const char *header)
{
    double read_miss_percent = 0.0;
    double write_miss_percent = 0.0;

    if (c->stat_read_access)
    {
        read_miss_percent = 100.0 * (double)(c->stat_read_miss) /
                            (double)(c->stat_read_access);
    }

    if (c->stat_write_access)
    {
        write_miss_percent = 100.0 * (double)(c->stat_write_miss) /
                             (double)(c->stat_write_access);
    }

    printf("\n");
    printf("%s_READ_ACCESS     \t\t : %10llu\n", header, c->stat_read_access);
    printf("%s_WRITE_ACCESS    \t\t : %10llu\n", header, c->stat_write_access);
    printf("%s_READ_MISS       \t\t : %10llu\n", header, c->stat_read_miss);
    printf("%s_WRITE_MISS      \t\t : %10llu\n", header, c->stat_write_miss);
    printf("%s_READ_MISS_PERC  \t\t : %10.3f\n", header, read_miss_percent);
    printf("%s_WRITE_MISS_PERC \t\t : %10.3f\n", header, write_miss_percent);
    printf("%s_DIRTY_EVICTS    \t\t : %10llu\n", header, c->stat_dirty_evicts);
}
