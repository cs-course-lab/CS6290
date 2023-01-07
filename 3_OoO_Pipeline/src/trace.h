// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// You should not modify this file. //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

// trace.h
// Declares the TraceRec and InstInfo structs, structures containing
// information about each instruction executed in the CPU trace. Also declares
// related enums.
//
// TraceRec defines the raw form of instructions present in the trace file,
// while InstInfo is what moves through the pipeline and ROB.
//
// You should never have to use TraceRec directly; TraceRec is only used
// internally by pipe_fetch_inst(). You will only directly read and modify
// InstInfo structures in the pipeline and ROB.

#ifndef _TRACE_H_
#define _TRACE_H_

#include <inttypes.h>

/** The type of operation performed by an instruction in the CPU trace. */
typedef enum OpTypeEnum
{
    OP_ALU,   // Any ALU operation, e.g. add, sub, mul, div
    OP_LD,    // Load from memory
    OP_ST,    // Store to memory
    OP_CBR,   // Conditional branch
    OP_OTHER, // Anything else
    NUM_OP_TYPES
} OpType;

/** [Internal] A raw individual record read from the CPU trace file. */
typedef struct TraceRecStruct
{
    /** [Internal] The address (PC) of the instruction. */
    uint64_t inst_addr;
    /** [Internal] The type of operation performed by this instruction. */
    uint8_t op_type;
    /** [Internal] If dest_needed, this instruction's destination register. */
    uint8_t dest_reg;
    /** [Internal] Whether this instruction has a valid dest_reg. */
    uint8_t dest_needed;
    /** [Internal] If src1_needed, this instruction's 1st source register. */
    uint8_t src1_reg;
    /** [Internal] If src2_needed, this instruction's 2nd source register. */
    uint8_t src2_reg;
    /** [Internal] Whether this instruction has a valid src1_reg. */
    uint8_t src1_needed;
    /** [Internal] Whether this instruction has a valid src2_reg. */
    uint8_t src2_needed;
    /** [Internal] Whether this instruction reads cc. Ignored in this lab. */
    uint8_t cc_read;
    /** [Internal] Whether this instruction writes cc. Ignored in this lab. */
    uint8_t cc_write;
    /** [Internal] If mem_read/mem_write, the memory address to read/write. */
    uint64_t mem_addr;
    /** [Internal] Whether this instruction writes the memory at mem_addr. */
    uint8_t mem_write;
    /** [Internal] Whether this instruction reads the memory at mem_addr. */
    uint8_t mem_read;
    /** [Internal] If op_type == OP_CBR, whether this branch is taken. */
    uint8_t br_dir;
    /** [Internal] If op_type == OP_CBR, the target address of the branch. */
    uint64_t br_target;
} TraceRec;

/**
 * An in-flight instruction in the out-of-order processor.
 * 
 * This structure is passed through the pipeline latches and used in each ROB
 * entry.
 * 
 * You are expected to modify some of the fields to implement register renaming
 * and to determine when the instruction is ready to execute.
 */
typedef struct InstInfoStruct
{
    /**
     * A unique, monotonically increasing ID for this instruction.
     * 
     * This is guaranteed to be unique for each operation in the trace file.
     * 
     * Additionally, it is monotonically increasing, which allows it to be used
     * for ordering instructions: if A's inst_num is less than B's inst_num,
     * then A is older than B.
     */
    uint64_t inst_num;

    /**
     * The type of operation performed by this instruction, as indicated by the
     * OpType enum.
     */
    OpType op_type;

    /**
     * The destination register this instruction writes to.
     * 
     * This is set to -1 if no destination register is used.
     */
    int dest_reg;

    /**
     * The first source register this instruction reads from.
     * 
     * This is set to -1 if no first source register is used.
     */
    int src1_reg;

    /**
     * The second source register this instruction reads from.
     * 
     * This is set to -1 if no second source register is used.
     */
    int src2_reg;

    /**
     * The tag of this instruction's destination register after renaming.
     * 
     * This is the same as the ID of the ROB entry where this instruction is
     * stored.
     * 
     * (Since, in hardware, the ROB stores the data value of the destination
     * register until it is committed, it doubles as a PRF, so the tag of the
     * destination register can be considered synonymous with the tag (ID) of
     * the instruction that produces it.)
     * 
     * The issue stage of your pipeline should set this field during register
     * renaming.
     */
    int dr_tag;

    /**
     * The tag of this instruction's first source register after renaming.
     * 
     * This is the ID of the ROB entry corresponding to the instruction that
     * will produce this instruction's first operand.
     * 
     * If this instruction's first operand is not needed or already ready, this
     * field should be set to -1.
     * 
     * The issue stage of your pipeline should set this field during register
     * renaming.
     */
    int src1_tag;

    /**
     * The tag of this instruction's second source register after renaming.
     * 
     * This is the ID of the ROB entry corresponding to the instruction that
     * will produce this instruction's second operand.
     * 
     * If this instruction's second operand is not needed or already ready,
     * this field should be set to -1.
     * 
     * The issue stage of your pipeline should set this field during register
     * renaming.
     */
    int src2_tag;

    /**
     * Whether this instruction's first operand is ready or not needed.
     * 
     * You will need to update this field as needed and use it in determining
     * when the instruction is ready to execute.
     */
    bool src1_ready;

    /**
     * Whether this instruction's second operand is ready or not needed.
     * 
     * You will need to update this field as needed and use it in determining
     * when the instruction is ready to execute.
     */
    bool src2_ready;

    /**
     * The number of cycles this instruction will take to execute.
     * 
     * You shouldn't have to use this field directly. It is used by the EXEQ
     * structure for multi-cycle execution.
     */
    int exe_wait_cycles;
} InstInfo;

#endif
