#include "perf_collector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>

static int perf_fds[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
static bool initialized = false;

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

int perf_collector_init(void) {
    if (initialized) {
        return 0;
    }
    
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(pe);
    pe.disabled = 1;
    pe.exclude_kernel = 0;
    pe.exclude_hv = 1;
    
    // Open performance counters
    uint32_t configs[] = {
        PERF_COUNT_HW_CPU_CYCLES,
        PERF_COUNT_HW_INSTRUCTIONS,
        PERF_COUNT_HW_CACHE_REFERENCES,
        PERF_COUNT_HW_CACHE_MISSES,
        PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
        PERF_COUNT_HW_BRANCH_MISSES,
    };
    
    for (int i = 0; i < 6; i++) {
        pe.config = configs[i];
        perf_fds[i] = perf_event_open(&pe, 0, -1, -1, 0);
        if (perf_fds[i] < 0) {
            fprintf(stderr, "Failed to open perf counter %d\n", i);
        }
    }
    
    // Software counters
    pe.type = PERF_TYPE_SOFTWARE;
    pe.config = PERF_COUNT_SW_PAGE_FAULTS;
    perf_fds[6] = perf_event_open(&pe, 0, -1, -1, 0);
    
    pe.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
    perf_fds[7] = perf_event_open(&pe, 0, -1, -1, 0);
    
    initialized = true;
    return 0;
}

void perf_collector_shutdown(void) {
    for (int i = 0; i < 8; i++) {
        if (perf_fds[i] >= 0) {
            close(perf_fds[i]);
            perf_fds[i] = -1;
        }
    }
    initialized = false;
}

int perf_collector_start(void) {
    if (!initialized) {
        return -1;
    }
    
    for (int i = 0; i < 8; i++) {
        if (perf_fds[i] >= 0) {
            ioctl(perf_fds[i], PERF_EVENT_IOC_RESET, 0);
            ioctl(perf_fds[i], PERF_EVENT_IOC_ENABLE, 0);
        }
    }
    
    return 0;
}

int perf_collector_stop(perf_counters_t* counters) {
    if (!initialized || !counters) {
        return -1;
    }
    
    for (int i = 0; i < 8; i++) {
        if (perf_fds[i] >= 0) {
            ioctl(perf_fds[i], PERF_EVENT_IOC_DISABLE, 0);
        }
    }
    
    return perf_collector_read(counters);
}

int perf_collector_read(perf_counters_t* counters) {
    if (!initialized || !counters) {
        return -1;
    }
    
    memset(counters, 0, sizeof(*counters));
    
    uint64_t values[8];
    for (int i = 0; i < 8; i++) {
        if (perf_fds[i] >= 0) {
            read(perf_fds[i], &values[i], sizeof(uint64_t));
        } else {
            values[i] = 0;
        }
    }
    
    counters->cycles = values[0];
    counters->instructions = values[1];
    counters->cache_references = values[2];
    counters->cache_misses = values[3];
    counters->branch_instructions = values[4];
    counters->branch_misses = values[5];
    counters->page_faults = values[6];
    counters->context_switches = values[7];
    
    // Calculate derived metrics
    if (counters->cycles > 0) {
        counters->ipc = (double)counters->instructions / counters->cycles;
    }
    if (counters->cache_references > 0) {
        counters->cache_miss_rate = (double)counters->cache_misses / 
                                    counters->cache_references;
    }
    if (counters->branch_instructions > 0) {
        counters->branch_miss_rate = (double)counters->branch_misses / 
                                     counters->branch_instructions;
    }
    
    return 0;
}
