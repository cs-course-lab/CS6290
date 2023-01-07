// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// DO NOT MODIFY OR SUBMIT THIS FILE. //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

#ifndef _TRACE_H_
#define _TRACE_H_

#include <inttypes.h>

/** The type of operation performed by an instruction in the CPU trace file. */
typedef enum OpTypeEnum
{
    OP_ALU,   // Any ALU operation, e.g. add, sub, mul, div
    OP_LD,    // Load from memory
    OP_ST,    // Store to memory
    OP_CBR,   // Conditional branch
    OP_OTHER, // Anything else
    NUM_OP_TYPES
} OpType;

/**
 * An individual record from the CPU trace file.
 * This corresponds to a single execution of a single instruction in the trace.
 */
typedef struct TraceRecStruct
{
    /**
     * The address (PC) of the instruction.
     *
     * This may not be unique among all trace records, as the same instruction
     * may be executed multiple times in the trace (e.g. in a loop or in a
     * function called multiple times).
     */
    uint64_t inst_addr;

    /**
     * The type of operation performed by this instruction, as indicated by the
     * OpType enum.
     */
    uint8_t optype;
} TraceRec;

/**
 * Updates the global variables stat_num_cycle, stat_optype_dyn, and
 * stat_unique_pc according to the given trace record.
 *
 * You must implement this function in studentwork.cpp.
 *
 * @param t the trace record to process
 */
void analyze_trace_record(TraceRec *t);

#endif
