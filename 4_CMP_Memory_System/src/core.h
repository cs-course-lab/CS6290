///////////////////////////////////////////////////////////////////////////////
// You shouldn't need to modify this file.                                   //
///////////////////////////////////////////////////////////////////////////////

// core.h
// Declares a CPU core structure and related functions.

#ifndef __CORE_H__
#define __CORE_H__

#include "types.h"
#include "memsys.h"
#include <sys/types.h>

typedef struct Core
{
    unsigned int core_id;

    MemorySystem *memsys;

    int trace_fd;
    pid_t pid;
    uint8_t read_buf[32 * 1024];
    size_t read_buf_offset;
    ssize_t read_buf_left;

    bool done;

    uint64_t trace_inst_addr;
    uint64_t trace_inst_type;
    uint64_t trace_ldst_addr;

    // Used to stall when waiting for data to return from memory.
    uint64_t snooze_end_cycle;

    unsigned long long inst_count;
    unsigned long long done_inst_count;
    unsigned long long done_cycle_count;
} Core;

Core *core_new(MemorySystem *memsys, const char *trace_filename,
               unsigned int core_id);
void core_cycle(Core *core);
void core_print_stats(Core *core);
void core_read_trace(Core *core);

#endif // __CORE_H__
