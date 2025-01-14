// dram.h
// Contains declarations of data structures and functions used to implement DRAM.

#ifndef __DRAM_H__
#define __DRAM_H__

#include "types.h"

///////////////////////////////////////////////////////////////////////////////
//                              DATA STRUCTURES                              //
///////////////////////////////////////////////////////////////////////////////

/** Row buffer */
typedef struct RowBuffer 
{
    bool valid;
    unsigned rowID;
} RowBuffer;

/** A DRAM module. */
typedef struct DRAM
{
    // Row buffer 
    RowBuffer* rowbuf;

    // access info
    unsigned bank_bits;
    
    /**
     * The total number of times DRAM was accessed for a read.
     */
    unsigned long long stat_read_access;

    /**
     * The total number of cycles spent on DRAM reads.
     */
    uint64_t stat_read_delay;

    /**
     * The total number of times DRAM was accessed for a write.
     */
    unsigned long long stat_write_access;

    /**
     * The total number of cycles spent on DRAM writes.
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

/**
 * Allocate and initialize a DRAM module.
 * 
 * @return A pointer to the DRAM module.
 */
DRAM *dram_new();

/**
 * Access the DRAM at the given cache line address.
 * 
 * @param dram The DRAM module to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size).
 * @param is_dram_write Whether this access writes to DRAM.
 * @return The delay in cycles incurred by this DRAM access.
 */
uint64_t dram_access(DRAM *dram, uint64_t line_addr, bool is_dram_write);

/**
 * For modes C through F, access the DRAM at the given cache line address.
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
 * @param dram The DRAM module to print the statistics of.
 */
void dram_print_stats(DRAM *dram);

#endif // __DRAM_H__
