//#include "proc_stat.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

struct cpu_stat {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
    uint64_t guest;
    uint64_t guest_nice;
};
    
struct proc_stat {
    struct cpu_stat total_cpu_stat;
    struct cpu_stat* cpu_stat;
    size_t cpu_statlen;
    
    uint64_t total_interrupt;
    uint64_t* interrupt;
    size_t interruptlen;

    uint64_t total_context;

    uint64_t total_softirq;
    uint64_t* softirq;
    size_t softirqlen;
};
/*
static const char CPU[] = "cpu";
static const char IRQ[] = "intr";
static const char SOFTIRQ[] = "softirq";
static const char CONTEXT[] = "ctxt";
*/

int parse_cpu_stat(char *s, struct cpu_stat *stat) {
    assert(s != NULL);
    errno = 0;
    if(stat == NULL) {
        return 0;
    }

    uint64_t *offset = &stat->user;
    for(char *stash = NULL, *token = strtok_r(s, " ", &stash);
        token != NULL;
        token = strtok_r(NULL, " ", &stash), ++offset) {
        char *err = NULL;
        *offset = strtoull(token, &err, 10); 
        if(errno || *err != 0) {
            errno = errno ? errno : EINVAL;
            return -1;
        }
    }
    return 0;
}

int parse_interrupts(char *s,
                     uint64_t *total,
                     uint64_t *interrupts, size_t *interruptslen) {
    assert(s != NULL);
    errno = 0;
    if(total == NULL && interrupts == NULL && interruptslen == NULL) {
        return 0;
    }
    char *stash = NULL;
    char *token = strtok_r(s, " ", &stash);

    if(total != NULL) {
        char *err = NULL;
        *total = strtoull(token, &err, 10);
        if(errno || *err != 0) {
            errno = errno ? errno : EINVAL;
            return -1;
        }
    }
    size_t idx = 0;
    for(token = strtok_r(NULL, " ", &stash);
        token != NULL;
        token = strtok_r(NULL, " ", &stash), ++idx) {
        if(interrupts != NULL && interruptslen != NULL && idx < *interruptslen) {
            char *err = NULL;
            *(interrupts + idx) = strtoull(token, &err, 10);
            if(errno || *err) {
                errno = errno ? errno : EINVAL;
                return -1;
            }
        }
    }
    if(interruptslen != NULL) {
        if(*interruptslen < idx) {
            errno = ERANGE;
        }
        *interruptslen = idx;
    }
    return errno ? -1 : 0;
}

int parse_context(char *s, uint64_t *context) {
    assert(s != NULL);
    errno = 0;
    if(context != NULL) {
        char *err = NULL;
        *context = strtoull(s, &err, 10);
        if(errno || *err != 0) {
            errno = errno ? errno : EINVAL;
            return -1;
        }
    }
    return 0;
}

int parse_stat(char *s, struct proc_stat *stat) {
    assert(s != NULL);
    errno = 0;
    if(stat == NULL) return 0;
    return 0;
}

/* int scale_cpu_stat(struct cpu_stat *stats, int64_t p, int64_t q) { */
/*     assert(stats != NULL); */
/*     stats->user *= p; stats->user /= q; */
/*     stats->nice *= p; stats->nice /= q; */
/*     stats->system *= p; stats->system /= q; */
/*     stats->idle *= p; stats->idle /= q; */
/*     stats->iowait *= p; stats->iowait /= q; */
/*     stats->irq *= p; stats->irq /= q; */
/*     stats->softirq *= p; stats->softirq /= q; */
/*     stats->steal *= p; stats->steal /= q; */
/*     stats->guest *= p; stats->guest /= q; */
/*     stats->guest_nice *= p; stats->guest_nice /= q; */
/*     return 0; */
/* } */

/* int percentage_cpu_stat(struct cpu_stat *stats) { */
/*     uint64_t sum = stats->user + stats->nice + stats->system + stats->idle */
/*         + stats->iowait */
/*         + stats->irq + stats->softirq */
/*         + stats->steal + stats->guest + stats->guest_nice; */
/*     return scale_cpu_stat(stats, 100, sum); */
/* } */

/* int substract_cpu_stat(struct cpu_stat *lhs, */
/*                         const struct cpu_stat *rhs) { */
/*     assert(lhs != NULL); */
/*     assert(rhs != NULL); */
/*     if(strncmp(lhs->id, rhs->id, sizeof(lhs->id)) != 0) { */
/*         errno = EINVAL; */
/*         return -1; */
/*     } */
    
/*     lhs->user -= rhs->user; */
/*     lhs->nice -= rhs->nice; */
/*     lhs->system -= rhs->system; */
/*     lhs->idle -= rhs->idle; */
/*     lhs->iowait -= rhs->iowait; */
/*     lhs->irq -= rhs->irq; */
/*     lhs->softirq -= rhs->softirq; */
/*     lhs->steal -= rhs->steal; */
/*     lhs->guest -= rhs->guest; */
/*     lhs->guest_nice -= rhs->guest_nice; */
    
/*     return 0; */
/* } */
                    
void dump_cpu_stat(const char* tag, const struct cpu_stat *stat) {
    assert(tag != NULL);
    assert(stat != NULL);
    printf("%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
           tag,
           stat->user, stat->nice, stat->system, stat->idle,
           stat->iowait,
           stat->irq, stat->softirq,
           stat->steal, stat->guest, stat->guest_nice);
}

#ifdef PROC_STAT_MAIN

int main() {
    {
        char total[] = "402169 361 152592 549511651 78243 0 5376 0 0 0";
        struct cpu_stat cpu_stat;
        memset(&cpu_stat, 0xff, sizeof(cpu_stat));
        assert(0 == parse_cpu_stat(total, &cpu_stat));
        
        assert(cpu_stat.user == 402169);
        assert(cpu_stat.nice == 361);
        assert(cpu_stat.system == 152592);
        assert(cpu_stat.idle == 549511651);
        assert(cpu_stat.iowait == 78243);
        assert(cpu_stat.irq == 0);
        assert(cpu_stat.softirq == 5376);
        assert(cpu_stat.steal == 0);
        assert(cpu_stat.guest == 0);
        assert(cpu_stat.guest_nice == 0);
    }
    {
        char cpu3[] = "108105 93 36876 137407624 23067 0 324 0 0 0";
        struct cpu_stat cpu_stat;
        memset(&cpu_stat, 0xff, sizeof(cpu_stat));
        assert(0 == parse_cpu_stat(cpu3, &cpu_stat));
        
        assert(cpu_stat.user == 108105);
        assert(cpu_stat.nice == 93);
        assert(cpu_stat.system == 36876);
        assert(cpu_stat.idle == 137407624);
        assert(cpu_stat.iowait == 23067);
        assert(cpu_stat.irq == 0);
        assert(cpu_stat.softirq == 324);
        assert(cpu_stat.steal == 0);
        assert(cpu_stat.guest == 0);
        assert(cpu_stat.guest_nice == 0);
    }
    {
        char bad[] = "10abc8105 93 36876 137407624 23067 0 324 0 0 0";
        struct cpu_stat cpu_stat;
        assert(-1 == parse_cpu_stat(bad, &cpu_stat));
        assert(errno == EINVAL);
    }
    {
        char ignore[] = "xxx";
        assert(0 == parse_cpu_stat(ignore, NULL));
    }
    {
        char intr[] = "74296033 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 26423167 0 0 0 0 112938 0 0 0 0 0 0 0 677048 0 0 6411 0 0 0 0 0 6228895 0 0 0 0 0 0 0 6667493 0 0 0 0 0 0 36 0 398132";
        size_t count = 0;
        assert(-1 == parse_interrupts(intr, NULL, NULL, &count));
        assert(errno == ERANGE);
        assert(count == 57);
    }  
    {
        char intr[] = "74296033 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 26423167 0 0 0 0 112938 0 0 0 0 0 0 0 677048 0 0 6411 0 0 0 0 0 6228895 0 0 0 0 0 0 0 6667493 0 0 0 0 0 0 36 0 398132";
        uint64_t total = 0;
        size_t count = 0;        
        assert(-1 == parse_interrupts(intr, &total, NULL, &count));
        assert(errno == ERANGE);
        assert(count == 57);
        assert(total == 74296033);
    }
    {
        char intr[] = "74296033 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 26423167 0 0 0 0 112938 0 0 0 0 0 0 0 677048 0 0 6411 0 0 0 0 0 6228895 0 0 0 0 0 0 0 6667493 0 0 0 0 0 0 36 0 398132";
        uint64_t values = (uint64_t)(-1);
        size_t count = 1;        
        assert(-1 == parse_interrupts(intr, NULL, &values, &count));
        assert(errno == ERANGE);
        assert(count == 57);
        assert(values == 0);
    }
    {
        char intr[] = "74296033 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 26423167 0 0 0 0 112938 0 0 0 0 0 0 0 677048 0 0 6411 0 0 0 0 0 6228895 0 0 0 0 0 0 0 6667493 0 0 0 0 0 0 36 0 398132";
        uint64_t values = (uint64_t)(-1);
        uint64_t total = 0;
        size_t count = 1;        
        assert(-1 == parse_interrupts(intr, &total, &values, &count));
        assert(errno == ERANGE);
        assert(count == 57);
        assert(values == 0);
        assert(total == 74296033);
    }
    {
        char bad[] = "74296033 abc 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 26423167 0 0 0 0 112938 0 0 0 0 0 0 0 677048 0 0 6411 0 0 0 0 0 6228895 0 0 0 0 0 0 0 6667493 0 0 0 0 0 0 36 0 398132";
        uint64_t values = (uint64_t)(-1);
        size_t count = 1;        
        assert(-1 == parse_interrupts(bad, NULL, &values, &count));
        assert(errno == EINVAL);
    }
    {
        char ignore[] = "xxx";
        assert(0 == parse_interrupts(ignore, NULL, NULL, NULL));
    }
    {
        char intr[] = "74296033 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 26423167 0 0 0 0 112938 0 0 0 0 0 0 0 677048 0 0 6411 0 0 0 0 0 6228895 0 0 0 0 0 0 0 6667493 0 0 0 0 0 0 36 0 398132";
        uint64_t values[64];
        size_t count = 64;
        assert(0 == parse_interrupts(intr, NULL, values, &count));
        assert(count == 57);
        assert(values[17] == 26423167);
        assert(values[22] == 112938);
        assert(values[30] == 677048);
        assert(values[33] == 6411);
        assert(values[39] == 6228895);
        assert(values[47] == 6667493);
        assert(values[54] == 36);
        assert(values[56] == 398132);
    }
    {
        char ctx[] = "105244433";
        assert(0 == parse_context(ctx, NULL));
    }
    {
        char bad[] = "10524vv4433";
        uint64_t ctx;
        assert(-1 == parse_context(bad, &ctx));
        assert(errno == EINVAL);
    }
    {
        char ctx[] = "105244433";
        uint64_t value;
        assert(0 == parse_context(ctx, &value));
        assert(value == 105244433);
    }
    {
        char stat[] =
            "cpu  402169 361 152592 549511651 78243 0 5376 0 0 0\n"
            "cpu0 95611 96 42268 137311411 22173 0 4263 0 0 0\n"
            "cpu1 103449 68 37454 137416820 11902 0 429 0 0 0\n"
            "cpu2 95004 104 35994 137375795 21099 0 360 0 0 0\n"
            "cpu3 108105 93 36876 137407624 23067 0 324 0 0 0\n"
            "intr 74296033 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 26423167 0 0 0 0 112938 0 0 0 0 0 0 0 677048 0 0 6411 0 0 0 0 0 6228895 0 0 0 0 0 0 0 6667493 0 0 0 0 0 0 36 0 398132\n"
            "ctxt 105244433\n"
            "btime 1564258665\n"
            "processes 861575\n"
            "procs_running 1\n"
            "procs_blocked 0\n"
            "softirq 50438794 7 16904130 89568 720988 0 0 4965277 17320789 2269 10435766\n";
        {
            struct proc_stat proc_stat;
            memset(&proc_stat, 0, sizeof(proc_stat));
            assert(-1 == parse_stat(stat, &proc_stat));
            assert(errno == ERANGE);
            assert(proc_stat.cpu_statlen == 4);
            assert(proc_stat.interruptlen == 57);
            assert(proc_stat.softirqlen == 9);
        }
        {
            struct proc_stat proc_stat;
            memset(&proc_stat, 0, sizeof(proc_stat));
            proc_stat.cpu_statlen = 4;
            proc_stat.cpu_stat = (struct cpu_stat*)calloc(proc_stat.cpu_statlen,
                                                          sizeof(struct cpu_stat));
            proc_stat.interruptlen = 57;
            proc_stat.interrupt = (uint64_t*)calloc(proc_stat.interruptlen,
                                                    sizeof(uint64_t));
            proc_stat.softirqlen = 9;
            proc_stat.softirq = (uint64_t*)calloc(proc_stat.softirqlen,
                                                  sizeof(uint64_t));
            assert(0 == parse_stat(stat, &proc_stat));

            assert(proc_stat.total_cpu_stat.user == 402169);
            assert(proc_stat.total_cpu_stat.nice == 361);
            assert(proc_stat.total_cpu_stat.system == 152592);
            assert(proc_stat.total_cpu_stat.idle == 549511651);
            assert(proc_stat.total_cpu_stat.iowait == 78243);
            assert(proc_stat.total_cpu_stat.irq == 0);
            assert(proc_stat.total_cpu_stat.softirq == 5376);
            assert(proc_stat.total_cpu_stat.steal == 0);
            assert(proc_stat.total_cpu_stat.guest == 0);
            assert(proc_stat.total_cpu_stat.guest_nice == 0);

            
        /* assert(cpu_stat.user == 108105); */
        /* assert(cpu_stat.nice == 93); */
        /* assert(cpu_stat.system == 36876); */
        /* assert(cpu_stat.idle == 137407624); */
        /* assert(cpu_stat.iowait == 23067); */
        /* assert(cpu_stat.irq == 0); */
        /* assert(cpu_stat.softirq == 324); */
        /* assert(cpu_stat.steal == 0); */
        /* assert(cpu_stat.guest == 0); */
        /* assert(cpu_stat.guest_nice == 0); */

        /* assert(values[17] == 26423167); */
        /* assert(values[22] == 112938); */
        /* assert(values[30] == 677048); */
        /* assert(values[33] == 6411); */
        /* assert(values[39] == 6228895); */
        /* assert(values[47] == 6667493); */
        /* assert(values[54] == 36); */
        /* assert(values[56] == 398132); */

        /*         assert(value == 105244433); */

            free(proc_stat.softirq);
            free(proc_stat.interrupt);
            free(proc_stat.cpu_stat);
        }
    }

    /* struct cpu_stat stats; */
    /* for(char *stash = NULL, *token = strtok_r(cpu, "\n", &stash); */
    /*     token != NULL; */
    /*     token = strtok_r(NULL, "\n", &stash)) { */
    /*     if(strncmp(token, CPU, strlen(CPU)) == 0) { */
    /*         parse_cpu_stat(token, &stats); */
    /*     } else { */
    /*         // */
    /*     } */
    /* } */
//    return 0;
}

#endif /* PROC_STAT_MAIN */
