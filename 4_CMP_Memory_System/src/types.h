///////////////////////////////////////////////////////////////////////////////
// You shouldn't need to modify this file.                                   //
///////////////////////////////////////////////////////////////////////////////

// types.h
// Contains type definitions used throughout the code.

#ifndef __TYPES_H__
#define __TYPES_H__

#include <inttypes.h>

/** Possible types of instructions. */
typedef enum InstTypeEnum
{
    INST_TYPE_ALU = 0,      // An ALU instruction, e.g., ADD or SUB.
    INST_TYPE_LOAD = 1,     // A load instruction, i.e., LD.
    INST_TYPE_STORE = 2,    // A store instruction, i.e., ST.
    INST_TYPE_OTHER = 3,    // An non-ALU instruction that is not an LD or ST.
} InstType;

/** Possible reasons for an access to memory. */
typedef enum AccessTypeEnum
{
    ACCESS_TYPE_IFETCH = 0, // An instruction fetch.
    ACCESS_TYPE_LOAD = 1,   // A data load caused by a load instruction.
    ACCESS_TYPE_STORE = 2,  // A data store caused by a store instruction.
} AccessType;

/** Possible ways in which the simulation can be run. */
typedef enum ModeEnum
{
    SIM_MODE_A = 1,     // Simulate the standalone cache only.
    SIM_MODE_B = 2,     // Simulate a multi-level cache.
    SIM_MODE_C = 3,     // Simulate a multi-level cache and a DRAM module.
    SIM_MODE_DEF = 4,   // Simulate a multicore system (in parts D, E, and F).
} Mode;

#endif // __TYPES_H__
