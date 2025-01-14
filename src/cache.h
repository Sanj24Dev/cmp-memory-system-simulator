// cache.h
// Contains declarations of data structures and functions used to implement a cache.

#ifndef __CACHE_H__
#define __CACHE_H__

#include "types.h"

///////////////////////////////////////////////////////////////////////////////
//                                 CONSTANTS                                 //
///////////////////////////////////////////////////////////////////////////////

/**
 * The maximum allowed number of ways in a cache set.
 * 
 * At runtime, the actual number of ways in each cache set is guaranteed to be
 * less than or equal to this value.
 */
#define MAX_WAYS_PER_CACHE_SET 16

///////////////////////////////////////////////////////////////////////////////
//                              DATA STRUCTURES                              //
///////////////////////////////////////////////////////////////////////////////

/** Possible replacement policies for the cache. */
typedef enum ReplacementPolicyEnum
{
    LRU = 0,    // Evict the least recently used line.
    RANDOM = 1, // Evict a random line.

    /**
     * Evict according to a static way partitioning policy.
     */
    SWP = 2,

    /**
     * Evict according to a dynamic way partitioning policy.
     */
    DWP = 3,
} ReplacementPolicy;

/** A single Cache line */
typedef struct CacheLine
{
    bool valid;
    bool dirty;
    unsigned long tag;
    unsigned coreID;
    uint64_t lastAccessTime;
} CacheLine;

// for DWP
struct UMON {
    unsigned totalHits[MAX_WAYS_PER_CACHE_SET]; // Tracks hits for each way
    unsigned totalMisses; // Tracks total misses
};

/** Cache set */
typedef struct CacheSet 
{
    CacheLine* row;
    unsigned ways_per_core[2];

    // for DWP
    UMON umon;
} CacheSet;


/** A single cache module. */
typedef struct Cache
{
    /**
     * The entire cache structure
     */
    CacheSet* cacheGrid;

    /** 
     * Total ways in the cache 
     */
    unsigned ways;

    /**
     * Total sets in the cache
     */
    unsigned sets;

    /**
     * The replacement policy used to kick out any entry when cache is full
     */
    ReplacementPolicy policy;

    /**
     * Information about the last evicted line from the cache
     */
    CacheLine lastEvictedLine;

    // Access bits
    unsigned index_mask;
    unsigned index_bits;

    /**
     * The total number of times this cache was accessed for a read.
     */
    unsigned long long stat_read_access;

    /**
     * The total number of read accesses that missed this cache.
     */
    unsigned long long stat_read_miss;

    /**
     * The total number of times this cache was accessed for a write.
     */
    unsigned long long stat_write_access;

    /**
     * The total number of write accesses that missed this cache.
     */
    unsigned long long stat_write_miss;

    /**
     * The total number of times a dirty line was evicted from this cache.
     */
    unsigned long long stat_dirty_evicts;
} Cache;

/** Whether a cache access is a hit or a miss. */
typedef enum CacheResultEnum
{
    HIT = 1,  // The access hit the cache.
    MISS = 0, // The access missed the cache.
} CacheResult;



///////////////////////////////////////////////////////////////////////////////
//                            FUNCTION PROTOTYPES                            //
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
                 ReplacementPolicy replacement_policy);

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
                         unsigned int core_id);

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
                   unsigned int core_id);

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
                               unsigned int core_id);

/**
 * Print the statistics of the given cache.
 * 
 * @param c The cache to print the statistics of.
 * @param label A label for the cache, which is used as a prefix for each
 *              statistic.
 */
void cache_print_stats(Cache *c, const char *label);

#endif // __CACHE_H__
