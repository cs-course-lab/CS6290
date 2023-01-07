///////////////////////////////////////////////////////////////////////////////
// You shouldn't need to modify this file.                                   //
///////////////////////////////////////////////////////////////////////////////

// core.cpp
// Defines the functions for the CPU cores.

#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern uint64_t current_cycle;

int open_gunzip_pipe(const char *filename, int *fd, pid_t *pid);
ssize_t trace_read(Core *core, void *buf, size_t size);

Core *core_new(MemorySystem *memsys, const char *trace_filename,
               unsigned int core_id)
{
    int trace_fd;
    pid_t pid;
    if (open_gunzip_pipe(trace_filename, &trace_fd, &pid) != 0)
    {
        return NULL;
    }

    Core *core = (Core *)calloc(1, sizeof(Core));
    core->core_id = core_id;
    core->memsys = memsys;
    core->trace_fd = trace_fd;
    core->pid = pid;
    core->read_buf_offset = 0;
    core->read_buf_left = 0;

    core_read_trace(core);
    return core;
}

void core_cycle(Core *core)
{
    if (core->done)
    {
        return;
    }

    // If core is snoozing on DRAM hits, return.
    if (current_cycle <= core->snooze_end_cycle)
    {
        return;
    }

    core->inst_count++;

    uint64_t ifetch_delay = 0;
    uint64_t ld_delay = 0;
    uint64_t bubble_cycles = 0;

    ifetch_delay = memsys_access(core->memsys, core->trace_inst_addr,
                                 ACCESS_TYPE_IFETCH, core->core_id);
    if (ifetch_delay > 1)
    {
        bubble_cycles += (ifetch_delay - 1);
    }

    if (core->trace_inst_type == INST_TYPE_LOAD)
    {
        ld_delay = memsys_access(core->memsys, core->trace_ldst_addr,
                                 ACCESS_TYPE_LOAD, core->core_id);
    }
    if (ld_delay > 1)
    {
        bubble_cycles += (ld_delay - 1);
    }

    if (core->trace_inst_type == INST_TYPE_STORE)
    {
        memsys_access(core->memsys, core->trace_ldst_addr, ACCESS_TYPE_STORE,
                      core->core_id);
    }
    // We don't incur bubbles for store misses.

    if (bubble_cycles)
    {
        core->snooze_end_cycle = current_cycle + bubble_cycles;
    }

    core_read_trace(core);
}

void core_read_trace(Core *core)
{
    uint32_t inst_addr;
    uint8_t inst_type;
    uint32_t ldst_addr;

    if (trace_read(core, &inst_addr, sizeof(inst_addr)) !=
            sizeof(inst_addr) ||
        trace_read(core, &inst_type, sizeof(inst_type)) !=
            sizeof(inst_type) ||
        trace_read(core, &ldst_addr, sizeof(ldst_addr)) !=
            sizeof(ldst_addr))
    {
        core->done = true;
        core->done_inst_count = core->inst_count;
        core->done_cycle_count = current_cycle;
    }

    core->trace_inst_addr = inst_addr;
    core->trace_inst_type = inst_type;
    core->trace_ldst_addr = ldst_addr;
}

void core_print_stats(Core *core)
{
    double ipc = 0.0;
    if (core->done_cycle_count)
    {
        ipc = (double)(core->done_inst_count) /
              (double)(core->done_cycle_count);
    }

    printf("\n");
    printf("CORE_%01d_INST         \t\t : %10llu\n", core->core_id,
           core->done_inst_count);
    printf("CORE_%01d_CYCLES       \t\t : %10llu\n", core->core_id,
           core->done_cycle_count);
    printf("CORE_%01d_IPC          \t\t : %10.3f\n", core->core_id, ipc);

    close(core->trace_fd);
    waitpid(core->pid, NULL, 0);
}

int open_gunzip_pipe(const char *filename, int *fd, pid_t *pid)
{
    int status;
    int pipefd[2];

    status = pipe(pipefd);
    if (status != 0)
    {
        perror("Couldn't create pipe");
        return 1;
    }

    *pid = fork();
    if (*pid == -1)
    {
        perror("Couldn't fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (*pid == 0)
    {
        // Child process: exec gunzip.
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execlp("gunzip", "gunzip", "-c", filename, NULL);
        perror("Couldn't exec gunzip");
        fprintf(stderr, "Is gunzip installed?\n");
        return 127;
    }

    // Parent process: return the read end of the pipe.
    *fd = pipefd[0];
    close(pipefd[1]);
    return 0;
}

ssize_t trace_read(Core *core, void *buf, size_t size)
{
    uint8_t *bytes = (uint8_t *)buf;
    size_t bytes_read_total = 0;
    size_t bytes_left = size;

    // Read a total of size bytes from the file descriptor.
    while (bytes_left > 0)
    {
        if (core->read_buf_left < 0)
        {
            // Error
            return -1;
        }

        if (core->read_buf_left == 0)
        {
            // Refill the read buffer.
            core->read_buf_left = read(core->trace_fd, core->read_buf,
                                       sizeof(core->read_buf));
            if (core->read_buf_left < 0)
            {
                perror("Couldn't read from trace file");
                return -1;
            }
            if (core->read_buf_left == 0)
            {
                // EOF
                break;
            }
            core->read_buf_offset = 0;
        }

        // Copy bytes from the read buffer to the caller's buffer.
        uint8_t *read_buf_ptr = core->read_buf + core->read_buf_offset;
        size_t bytes_to_copy = ((ssize_t)bytes_left < core->read_buf_left)
                                   ? bytes_left
                                   : core->read_buf_left;
        memcpy(bytes, read_buf_ptr, bytes_to_copy);
        bytes += bytes_to_copy;
        bytes_read_total += bytes_to_copy;
        bytes_left -= bytes_to_copy;
        core->read_buf_left -= bytes_to_copy;
        core->read_buf_offset += bytes_to_copy;
    }

    return bytes_read_total;
}
