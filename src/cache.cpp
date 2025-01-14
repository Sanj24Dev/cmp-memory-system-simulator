// cache.cpp
// Defines the functions used to implement the cache.

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <numeric>

///////////////////////////////////////////////////////////////////////////////
//                    EXTERNALLY DEFINED GLOBAL VARIABLES                    //
///////////////////////////////////////////////////////////////////////////////

/**
 * The current clock cycle number.
 */
extern uint64_t current_cycle;

/**
 * For static way partitioning, the quota of ways in each set that can be
 * assigned to core 0.
 * 
 * The remaining number of ways is the quota for core 1.
 */
extern unsigned int SWP_CORE0_WAYS;
unsigned int DWP_CORE0_WAYS = 0;

///////////////////////////////////////////////////////////////////////////////
//                           FUNCTION DEFINITIONS                            //
///////////////////////////////////////////////////////////////////////////////

/**
 * Allocate and initialize a cache.
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
    // Allocate memory to the data structures and initialize 
    Cache *c = (Cache*)malloc(sizeof(Cache));
    c->ways = associativity;
    c->sets = (size / line_size) / associativity;
    c->policy = replacement_policy;
    c->cacheGrid = (CacheSet*)calloc(c->sets, sizeof(CacheSet));
    for (unsigned i = 0; i < c->sets; i++) {
        c->cacheGrid[i].row = (CacheLine*)calloc(c->ways, sizeof(CacheLine));
        c->cacheGrid[i].ways_per_core[0] = 0;
        c->cacheGrid[i].ways_per_core[1] = 0;
        c->cacheGrid[i].umon.totalHits[MAX_WAYS_PER_CACHE_SET] = {0};
        c->cacheGrid[i].umon.totalMisses = 0;
    }

    // Access info
    c->index_bits = (unsigned)(std::log2(c->sets));
    c->index_mask = ((1U << c->index_bits) - 1);

    c->stat_read_access = 0;
    c->stat_read_miss = 0;
    c->stat_write_access = 0;
    c->stat_write_miss = 0;
    c->stat_dirty_evicts = 0;

    return c;
}

/**
 * Access the cache at the given address.
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
    // Get the access information
    int set_to_check = (line_addr & c->index_mask);
    unsigned long tag_to_check = line_addr >> c->index_bits;
    
    // Update the appropriate cache statistics
    if(is_write) 
        c->stat_write_access++;
    else
        c->stat_read_access++;

    // Check if the entry exists in cache
    for (unsigned i = 0; i < c->ways; i++)
    {
        if (c->cacheGrid[set_to_check].row[i].valid == true && c->cacheGrid[set_to_check].row[i].coreID == core_id && c->cacheGrid[set_to_check].row[i].tag == tag_to_check) 
        {
            // if write, then tag the row as dirty
            if(is_write)
            {
                c->cacheGrid[set_to_check].row[i].dirty = true;
            }
            c->cacheGrid[set_to_check].row[i].lastAccessTime = current_cycle;
            
            // for DWP
            c->cacheGrid[set_to_check].umon.totalHits[i]++;
            return HIT;
        }
    }
    
    // Update the appropriate cache statistics
    if(is_write) 
        c->stat_write_miss++;
    else
        c->stat_read_miss++;

    // for DWP
    c->cacheGrid[set_to_check].umon.totalMisses++;
    
    return MISS;
}

/**
 * Install the cache line with the given address.
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
    unsigned set_to_add = (line_addr & c->index_mask) % c->sets;
    unsigned i = cache_find_victim(c, set_to_add, core_id);
    c->lastEvictedLine = c->cacheGrid[set_to_add].row[i];

    // If the evicted line is dirty, update stats
    if (c->lastEvictedLine.valid == true && c->lastEvictedLine.dirty == true) {
        c->stat_dirty_evicts++;
    }
    if (c->lastEvictedLine.valid == true)
        c->cacheGrid[set_to_add].ways_per_core[c->lastEvictedLine.coreID]--;

    // Install the line
    c->cacheGrid[set_to_add].row[i].valid = true;
    c->cacheGrid[set_to_add].row[i].dirty = false;
    c->cacheGrid[set_to_add].row[i].tag = line_addr >> c->index_bits;
    c->cacheGrid[set_to_add].row[i].coreID = core_id;
    c->cacheGrid[set_to_add].ways_per_core[core_id]++;
    c->cacheGrid[set_to_add].row[i].lastAccessTime = current_cycle;
    if(is_write)
    {
        c->cacheGrid[set_to_add].row[i].dirty = true;
    }
}

/**
 * Find which way in a given cache set to replace when a new cache line needs
 * to be installed. This should be chosen according to the cache's replacement
 * policy.
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
    // TODO: In part A, implement the LRU and random replacement policies.
    // TODO: In part E, for extra credit, implement static way partitioning.
    // TODO: In part F, for extra credit, implement dynamic way partitioning.
    unsigned index = 0;

    if (c->policy == LRU) 
    {
        // Check for empty slot
        for (unsigned i = 0; i < c->ways; i++) 
        {
            if (c->cacheGrid[set_index].row[i].valid == false) 
            {
                return i;
            }
        }

        // If all are valid, then we need to evict a line
        uint64_t oldestTime = c->cacheGrid[set_index].row[0].lastAccessTime;

        for (unsigned i = 0; i < c->ways; i++)
        {
            if(c->cacheGrid[set_index].row[i].lastAccessTime < oldestTime)
            {
                index = i;
                oldestTime = c->cacheGrid[set_index].row[i].lastAccessTime;
            }
        }
    }
    else if (c->policy == RANDOM)
    {
        // Check for empty slot
        for (unsigned i = 0; i < c->ways; i++) 
        {
            if (c->cacheGrid[set_index].row[i].valid == false) 
            {
                return i;
            }
        }
        // If all are valid, then we need to evict a line
        index = (unsigned) (rand() % c->ways);
    }
    else if (c->policy == SWP) 
    {
        // Check for empty slot
        for (unsigned i = 0; i < c->ways; i++) 
        {
            if (c->cacheGrid[set_index].row[i].valid == false) 
            {
                return i;
            }
        }

        // If all are valid, then we need to evict a line
        uint64_t oldestTime = 0;
        unsigned core_to_assign = core_id; 
        if (c->cacheGrid[set_index].ways_per_core[0] < SWP_CORE0_WAYS)
            core_to_assign = 1;

        // Find the first row with the respective core id
        unsigned i = 0;
        for (; i < c->ways; i++)
        {
            if (c->cacheGrid[set_index].row[i].coreID == core_to_assign)
            {
                index = i;
                oldestTime = c->cacheGrid[set_index].row[i].lastAccessTime;
                break;
            }
        }
        // Find the oldest row with the respective core id
        for (; i < c->ways; i++)
        {
            if (c->cacheGrid[set_index].row[i].coreID == core_to_assign && c->cacheGrid[set_index].row[i].lastAccessTime < oldestTime)
            {
                index = i;
                oldestTime = c->cacheGrid[set_index].row[i].lastAccessTime;
            }
        }
    }
    else if (c->policy == DWP)
    {
        // Check for empty slot
        for (unsigned i = 0; i < c->ways; i++) 
        {
            if (c->cacheGrid[set_index].row[i].valid == false) 
            {
                return i;
            }
        }

        // If all are valid, then we need to evict a line
        unsigned totalUtility[2] = {0};
        unsigned totalHitbasedUtility[2] = {0};
        unsigned totalMissbasedUtility[2] = {0};

        // Compute the totla hits and misses for each core
        for (unsigned core = 0; core < 2; ++core) {
            for (unsigned way = 0; way < c->ways; ++way) {
                if (c->cacheGrid[set_index].row[way].coreID == core) {
                    totalHitbasedUtility[core] += c->cacheGrid[set_index].umon.totalHits[way];
                }
            }
            totalMissbasedUtility[core] += c->cacheGrid[set_index].umon.totalMisses;
            totalUtility[core] = (0.7 * totalHitbasedUtility[core]) + (0.3 * totalMissbasedUtility[core]);
        }

        // Dynamic way allocation for Core 0 based on utility
        unsigned totalUtilitySum = std::fmax(totalUtility[0] + totalUtility[1], 1U); // Avoid division by zero
        if ((totalUtility[0] * c->ways) / totalUtilitySum >= 0)
            DWP_CORE0_WAYS = (totalUtility[0] * c->ways) / totalUtilitySum;
        

        uint64_t oldestTime = 0;
        unsigned core_to_assign = core_id; 
        if (c->cacheGrid[set_index].ways_per_core[0] < DWP_CORE0_WAYS)
            core_to_assign = 1;

        unsigned i = 0;
        // Find the first row with the respective core id
        for (; i < c->ways; i++)
        {
            if (c->cacheGrid[set_index].row[i].coreID == core_to_assign)
            {
                index = i;
                oldestTime = c->cacheGrid[set_index].row[i].lastAccessTime;
                break;
            }
        }
        // Find the oldest row with the respective core id
        for (; i < c->ways; i++)
        {
            if (c->cacheGrid[set_index].row[i].coreID == core_to_assign && c->cacheGrid[set_index].row[i].lastAccessTime < oldestTime)
            {
                index = i;
                oldestTime = c->cacheGrid[set_index].row[i].lastAccessTime;
            }
        }
    }
    return index;
}

/**
 * Print the statistics of the given cache.
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
