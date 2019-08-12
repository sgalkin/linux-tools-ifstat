#ifndef PROC_STAT_H_
#define PROC_STAT_H_

#include <stdint.h>

/*
user   (1) Time spent in user mode.
nice   (2) Time spent in user mode with low priority (nice).
system (3) Time spent in system mode.
idle   (4) Time spent in the idle task.  This value should be USER_HZ times the second entry in the /proc/uptime pseudo-file.
iowait (since Linux 2.5.41)
       (5) Time waiting for I/O to complete.  This value is not reliable, for the following reasons:
    1. The CPU will not wait for I/O to complete; iowait is the time that a task is waiting for I/O to complete.  When a CPU goes into idle state for
       outstanding task I/O, another task will be scheduled on this CPU.
    2. On a multi-core CPU, the task waiting for I/O to complete is not running on any CPU, so the iowait of each CPU is difficult to calculate.
    3. The value in this field may decrease in certain conditions.
irq (since Linux 2.6.0-test4)
       (6) Time servicing interrupts.
softirq (since Linux 2.6.0-test4)
       (7) Time servicing softirqs.
steal (since Linux 2.6.11)
       (8) Stolen time, which is the time spent in other operating systems when running in a virtualized environment
guest (since Linux 2.6.24)
       (9) Time spent running a virtual CPU for guest operating systems under the control of the Linux kernel.
guest_nice (since Linux 2.6.33)
       (10) Time spent running a niced guest (virtual CPU for guest operating systems under the control of the Linux kernel).
*/
struct cpu_stats {
    char id[8];
    int64_t user;
    int64_t nice;
    int64_t system;
    int64_t idle;
    int64_t iowait;
    int64_t irq;
    int64_t softirq;
    int64_t steal;
    int64_t guest;
    int64_t guest_nice;
};

int parse_cpu_stats(char *s, struct cpu_stats *stats);

int substract_cpu_stats(struct cpu_stats *lhs, const struct cpu_stats *rhs);
int scale_cpu_stats(struct cpu_stats *stats, int64_t p, int64_t q);
int percentage_cpu_stats(struct cpu_stats *stats);

void dump_cpu_stats(const struct cpu_stats *stats);

#endif /* PROC_STAT_H_ */
