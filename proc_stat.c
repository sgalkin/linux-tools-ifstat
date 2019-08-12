#include "proc_stat.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

int parse_cpu_stats(char *s, struct cpu_stats *stats) {
    assert(s != NULL);
    assert(stats != NULL);

    char *stash = NULL;
    char *token = strtok_r(s, " ", &stash);
    if(token == NULL) {
        errno = EINVAL;
        return -1;
    }
    assert(strlen(token) < sizeof(stats->id));
    strncpy(stats->id, token, sizeof(stats->id) - 1);
    stats->id[sizeof(stats->id) - 1] = 0;

    token = strtok_r(NULL, " ", &stash);
    for(int64_t *offset = &stats->user;
        token != NULL;
        token = strtok_r(NULL, " ", &stash), ++offset) {
        char *err = NULL;
        int64_t value = strtoll(token, &err, 10);
        if(errno || *err != 0) continue;
        *offset = value;
    }
    return 0;
}


int scale_cpu_stats(struct cpu_stats *stats, int64_t p, int64_t q) {
    assert(stats != NULL);
    stats->user *= p; stats->user /= q;
    stats->nice *= p; stats->nice /= q;
    stats->system *= p; stats->system /= q;
    stats->idle *= p; stats->idle /= q;
    stats->iowait *= p; stats->iowait /= q;
    stats->irq *= p; stats->irq /= q;
    stats->softirq *= p; stats->softirq /= q;
    stats->steal *= p; stats->steal /= q;
    stats->guest *= p; stats->guest /= q;
    stats->guest_nice *= p; stats->guest_nice /= q;
    return 0;
}

int percentage_cpu_stats(struct cpu_stats *stats) {
    uint64_t sum = stats->user + stats->nice + stats->system + stats->idle
        + stats->iowait
        + stats->irq + stats->softirq
        + stats->steal + stats->guest + stats->guest_nice;
    return scale_cpu_stats(stats, 100, sum);
}

int substract_cpu_stats(struct cpu_stats *lhs,
                        const struct cpu_stats *rhs) {
    assert(lhs != NULL);
    assert(rhs != NULL);
    if(strncmp(lhs->id, rhs->id, sizeof(lhs->id)) != 0) {
        errno = EINVAL;
        return -1;
    }
    
    lhs->user -= rhs->user;
    lhs->nice -= rhs->nice;
    lhs->system -= rhs->system;
    lhs->idle -= rhs->idle;
    lhs->iowait -= rhs->iowait;
    lhs->irq -= rhs->irq;
    lhs->softirq -= rhs->softirq;
    lhs->steal -= rhs->steal;
    lhs->guest -= rhs->guest;
    lhs->guest_nice -= rhs->guest_nice;
    
    return 0;
}
                    
void dump_cpu_stats(const struct cpu_stats *stats) {
    assert(stats != NULL);
    printf("%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
           stats->id,
           stats->user, stats->nice, stats->system, stats->idle,
           stats->iowait,
           stats->irq, stats->softirq,
           stats->steal, stats->guest, stats->guest_nice);
}

#ifdef PROC_STAT_MAIN

static const char CPU[] = "cpu";

char cpu[] =
    "cpu  402169 361 152592 549511651 78243 0 5376 0 0 0\n"
    "cpu0 95611 96 42268 137311411 22173 0 4263 0 0 0\n"
    "cpu1 103449 68 37454 137416820 11902 0 429 0 0 0\n"
    "cpu2 95004 104 35994 137375795 21099 0 360 0 0 0\n"
    "cpu3 108105 93 36876 137407624 23067 0 324 0 0 0";

/* char */
/* intr 74296033 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 26423167 0 0 0 0 112938 0 0 0 0 0 0 0 677048 0 0 6411 0 0 0 0 0 6228895 0 0 0 0 0 0 0 6667493 0 0 0 0 0 0 36 0 398132 */
/* ctxt 105244433 */
/* btime 1564258665 */
/* processes 861575 */
/* procs_running 1 */
/* procs_blocked 0 */
/* softirq 50438794 7 16904130 89568 720988 0 0 4965277 17320789 2269 10435766 */


int main() {
    struct cpu_stats stats;
    for(char *stash = NULL, *token = strtok_r(cpu, "\n", &stash);
        token != NULL;
        token = strtok_r(NULL, "\n", &stash)) {
        if(strncmp(token, CPU, strlen(CPU)) == 0) {
            parse_cpu_stats(token, &stats);
        } else {
            //
        }
    }
    return 0;
}

#endif /* PROC_STAT_MAIN */
