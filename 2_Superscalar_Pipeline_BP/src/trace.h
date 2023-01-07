// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// You should not modify this file. //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

// trace.h
// Declares the trace record struct, which is a structure containing
// information about each instruction executed in the CPU trace. Also declares
// related enums.

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
 * This corresponds to a single operation, i.e., a single execution of a single
 * instruction in the trace.
 */
typedef struct TraceRecStruct
{
    /**
     * The address (PC) of the instruction.
     * 
     * This may not be unique among all trace records, as the same instruction
     * may be executed multiple times in the trace (e.g., in a loop or in a
     * function called multiple times).
     */
    uint64_t inst_addr;

    /**
     * The type of operation performed by this instruction, as indicated by the
     * OpType enum.
     */
    uint8_t op_type;

    /**
     * The destination register of this instruction.
     * 
     * This is only valid when dest_needed is true.
     */
    uint8_t dest_reg;

    /**
     * Whether this instruction writes to a register.
     * 
     * If this is true, dest_reg indicates which register this instruction
     * writes to.
     */
    uint8_t dest_needed;

    /**
     * The first source register for this instruction.
     * 
     * This is only valid when src1_needed is true.
     */
    uint8_t src1_reg;

    /**
     * The second source register for this instruction.
     * 
     * This is only valid when src2_needed is true.
     */
    uint8_t src2_reg;

    /**
     * Whether this instruction reads from the first source register.
     * 
     * If this is true, src1_reg indicates which register this instruction
     * reads its first source operand from.
     */
    uint8_t src1_needed;

    /**
     * Whether this instruction reads from the second source register.
     * 
     * If this is true, src2_reg indicates which register this instruction
     * reads its second source operand from.
     */
    uint8_t src2_needed;

    /**
     * Whether this instruction reads the conditional code.
     * 
     * The conditional code can be thought of as its own special register
     * separate from the other registers in the register file. It can be
     * treated similarly to any other register for the purposes of determining
     * pipeline stalls and forwarding.
     */
    uint8_t cc_read;

    /**
     * Whether this instruction writes the conditional code.
     * 
     * The conditional code can be thought of as its own special register
     * separate from the other registers in the register file. It can be
     * treated similarly to any other register for the purposes of determining
     * pipeline stalls and forwarding.
     */
    uint8_t cc_write;

    /**
     * The memory address this instruction reads or writes.
     * 
     * This is only valid when mem_read or mem_write is true.
     */
    uint64_t mem_addr;

    /**
     * Whether this instruction writes to memory.
     * 
     * If this is true, mem_addr indicates the address this instruction writes
     * to.
     */
    uint8_t mem_write;

    /**
     * Whether this instruction reads from memory.
     * 
     * If this is true, mem_addr indicates the address this instruction reads
     * from.
     */
    uint8_t mem_read;

    /**
     * If this instruction is a conditional branch, this indicates whether the
     * branch is actually taken or not.
     * 
     * This is only valid when op_type is OP_CBR.
     * 
     * You may find it useful to cast this to the BranchDirection type when
     * accessing it.
     */
    uint8_t br_dir;

    /**
     * If this instruction is a conditional branch, this is the target address
     * the branch jumps to if it is taken.
     * 
     * This is only valid when op_type is OP_CBR.
     */
    uint64_t br_target;
} TraceRec;

#endif
