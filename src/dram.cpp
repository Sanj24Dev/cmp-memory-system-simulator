// dram.cpp
// Defines the functions used to implement DRAM.

#include "dram.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

///////////////////////////////////////////////////////////////////////////////
//                                 CONSTANTS                                 //
///////////////////////////////////////////////////////////////////////////////

/** The fixed latency of a DRAM access assumed for mode B, in cycles. */
#define DELAY_SIM_MODE_B 100

/** The DRAM activation latency (ACT), in cycles. (Also known as RAS.) */
#define DELAY_ACT 45

/** The DRAM column selection latency (CAS), in cycles. */
#define DELAY_CAS 45

/** The DRAM precharge latency (PRE), in cycles. */
#define DELAY_PRE 45

/**
 * The DRAM bus latency, in cycles.
 */
#define DELAY_BUS 10

/** The row buffer size, in bytes. */
#define ROW_BUFFER_SIZE 1024

/** The number of banks in the DRAM module. */
#define NUM_BANKS 16

///////////////////////////////////////////////////////////////////////////////
//                    EXTERNALLY DEFINED GLOBAL VARIABLES                    //
///////////////////////////////////////////////////////////////////////////////

/**
 * The current mode under which the simulation is running.
 */
extern Mode SIM_MODE;

/** The number of bytes in a cache line. */
extern uint64_t CACHE_LINESIZE;

/** Which page policy the DRAM should use. */
extern DRAMPolicy DRAM_PAGE_POLICY;

///////////////////////////////////////////////////////////////////////////////
//                           FUNCTION DEFINITIONS                            //
///////////////////////////////////////////////////////////////////////////////

/**
 * Allocate and initialize a DRAM module.
 * 
 * @return A pointer to the DRAM module.
 */
DRAM *dram_new()
{
    // Allocate memory to the data structures and initialize
    DRAM *d = (DRAM*)malloc(sizeof(DRAM));
    d->rowbuf = (RowBuffer*)calloc(NUM_BANKS, sizeof(RowBuffer)); // per-bank row

    d->stat_read_access = 0;
    d->stat_read_delay = 0;
    d->stat_write_access = 0;
    d->stat_write_delay = 0;

    // Access info
    d->bank_bits = (unsigned)(std::log2(NUM_BANKS));
    return d;
}

/**
 * Access the DRAM at the given cache line address.
 * 
 * @param dram The DRAM module to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size).
 * @param is_dram_write Whether this access writes to DRAM.
 * @return The delay in cycles incurred by this DRAM access.
 */
uint64_t dram_access(DRAM *dram, uint64_t line_addr, bool is_dram_write)
{
    uint64_t delay = 0;
    if (SIM_MODE == SIM_MODE_B)
        delay += DELAY_SIM_MODE_B;
    else 
        delay += dram_access_mode_CDEF(dram, line_addr, is_dram_write);

    // Update dram stats
    if(is_dram_write)
    {
        dram->stat_write_access++;
        dram->stat_write_delay += delay;
    }
    else 
    {
        dram->stat_read_access++;
        dram->stat_read_delay += delay;
    }
    return delay;
}

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
                               bool is_dram_write)
{
    unsigned row_no = (unsigned) (line_addr >> dram->bank_bits);
    unsigned bank_no = (unsigned) row_no % NUM_BANKS;

    uint64_t delay = DELAY_BUS;
    if(DRAM_PAGE_POLICY == OPEN_PAGE)
    {
        // is active
        if (dram->rowbuf[bank_no].valid == true)
        {
            // row hit
            if(dram->rowbuf[bank_no].rowID == row_no)
                delay += DELAY_CAS;
            // row miss
            else 
            {
                delay += DELAY_PRE + DELAY_ACT + DELAY_CAS;
                dram->rowbuf[bank_no].rowID = row_no;
            }
        }    
        // not active
        else
        {
            delay += DELAY_ACT + DELAY_CAS;
            dram->rowbuf[bank_no].rowID = row_no;
            dram->rowbuf[bank_no].valid = true;
        }
    }
    else
    {
        delay += DELAY_ACT + DELAY_CAS;
        dram->rowbuf[bank_no].rowID = row_no;
        dram->rowbuf[bank_no].valid = false;
    }
    return delay;
}

/**
 * Print the statistics of the DRAM module.
 * 
 * @param dram The DRAM module to print the statistics of.
 */
void dram_print_stats(DRAM *dram)
{
    double avg_read_delay = 0.0;
    double avg_write_delay = 0.0;

    if (dram->stat_read_access)
    {
        avg_read_delay = (double)(dram->stat_read_delay) /
                         (double)(dram->stat_read_access);
    }

    if (dram->stat_write_access)
    {
        avg_write_delay = (double)(dram->stat_write_delay) /
                          (double)(dram->stat_write_access);
    }

    printf("\n");
    printf("DRAM_READ_ACCESS     \t\t : %10llu\n", dram->stat_read_access);
    printf("DRAM_WRITE_ACCESS    \t\t : %10llu\n", dram->stat_write_access);
    printf("DRAM_READ_DELAY_AVG  \t\t : %10.3f\n", avg_read_delay);
    printf("DRAM_WRITE_DELAY_AVG \t\t : %10.3f\n", avg_write_delay);
}
