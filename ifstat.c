#include "proc_stat.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/inotify.h>

#include <net/if.h>

#include <linux/if_link.h>

#define CHECK_RESULT(statement, message, action) \
    if((statement) == -1) { \
        perror((message));  \
        action;             \
    }

#define MAX_EVENTS 8

typedef void(*timer_callback)();

int timer_handler(int timer, timer_callback callback) {
    assert(timer != -1);
    assert(callback != NULL);

    struct timespec ts;
    CHECK_RESULT(clock_gettime(CLOCK_MONOTONIC, &ts),
                 "timer_handler: clock_gettime", return -1);
    printf("%ld.%09ld: timer triggered\n", ts.tv_sec, ts.tv_nsec);
    uint64_t data = 0;
    int result = read(timer, &data, sizeof(data));
    CHECK_RESULT(result, "timer_handler: read", return -1);
    if(result != sizeof(data) || data != 1) {
        fprintf(stderr,
                "Unepexcted content while reading timer: "
                "read %d bytes: %" PRIu64 "\n", result, data);
    } else {
        struct timespec b, e;
        CHECK_RESULT(clock_gettime(CLOCK_MONOTONIC, &b), "clock_gettime", return -1);
        callback();
        CHECK_RESULT(clock_gettime(CLOCK_MONOTONIC, &e), "clock_gettime", return -1);
        time_t sec = e.tv_sec - b.tv_sec;
        long nsec = e.tv_nsec - b.tv_nsec;
        if(nsec < 0) {
            nsec += 1000000000;
            sec -= 1;
        }
        printf("took: %ld.%09ld\n", sec, nsec);
    }
    return 0;
}


static struct cpu_stats data[2];
static struct cpu_stats *base = NULL;
static struct cpu_stats *current = data;

void parse_proc_stat() {
    int fd = open("/proc/stat", O_CLOEXEC | O_NONBLOCK, O_RDONLY);
    CHECK_RESULT(fd, "open", abort());
    char buf[4096];
    ssize_t size = read(fd, &buf, sizeof(buf));
    CHECK_RESULT(size, "read", abort());
    buf[size] = 0;
    for(char *stash, *token = strtok_r(buf, "\n", &stash);
        token != NULL;
        token = strtok_r(NULL, "\n", &stash)) {
        if(strncmp(token, "cpu ", strlen("cpu ")) == 0) {
            CHECK_RESULT(parse_cpu_stats(token, current), "parse_cpu_stats", abort());
            if(base == NULL) {
                base = current;
                current = data + 1;
            } else {
                substract_cpu_stats(base, current);
                scale_cpu_stats(base, -1, 1);
                percentage_cpu_stats(base);
                struct cpu_stats *tmp = current;
                current = base;
                base = tmp;
                
                dump_cpu_stats(current);
            }
        }
    }
    CHECK_RESULT(close(fd), "close", abort());
}

void gather_if_stats() {
    struct ifaddrs *ifa = NULL;
    CHECK_RESULT(getifaddrs(&ifa), "getifaddrs", goto IFADDRS_CLEANUP);
    assert(ifa != NULL);
    for (struct ifaddrs *cifa = ifa; cifa != NULL; cifa = cifa->ifa_next) {
        assert(cifa != NULL);
        if(cifa->ifa_flags & IFF_LOOPBACK) continue;
        if(!(cifa->ifa_flags & IFF_UP && cifa->ifa_flags & IFF_RUNNING)) continue;
        if(cifa->ifa_addr == NULL || cifa->ifa_addr->sa_family != AF_PACKET) continue;
        if(cifa->ifa_data == NULL) continue;
        struct rtnl_link_stats *stats = cifa->ifa_data;
/*
        struct rtnl_link_stats {
        __u32   rx_packets;             / * total packets received       * /
        __u32   tx_packets;             / * total packets transmitted    * / 
        __u32   rx_bytes;               / * total bytes received         * /
        __u32   tx_bytes;               / * total bytes transmitted      * /
        __u32   rx_errors;              / * bad packets received         * /
        __u32   tx_errors;              / * packet transmit problems     * /
        __u32   rx_dropped;             / * no space in linux buffers    * /
        __u32   tx_dropped;             / * no space available in linux  * /
        __u32   multicast;              / * multicast packets received   * /
        __u32   collisions;

        / * detailed rx_errors: * /
        __u32   rx_length_errors;
        __u32   rx_over_errors;         / * receiver ring buff overflow  * /
        __u32   rx_crc_errors;          / * recved pkt with crc error    * /
        __u32   rx_frame_errors;        / * recv'd frame alignment error * /
        __u32   rx_fifo_errors;         / * recv'r fifo overrun          * /
        __u32   rx_missed_errors;       / * receiver missed packet       * /

        / * detailed tx_errors * /
        __u32   tx_aborted_errors;
        __u32   tx_carrier_errors;
        __u32   tx_fifo_errors;
        __u32   tx_heartbeat_errors;
        __u32   tx_window_errors;

        / * for cslip etc * /
        __u32   rx_compressed;
        __u32   tx_compressed;

        __u32   rx_nohandler;           / * dropped, no handler found    * /
};
*/
        printf("%s:\t"
               "tx: p: %u b: %u e: %u d: %u\t"
               "rx: p: %u b: %u e: %u d: %u\n",
               cifa->ifa_name,
               stats->tx_packets, stats->tx_bytes, stats->tx_errors, stats->tx_dropped,
               stats->rx_packets, stats->rx_bytes, stats->rx_errors, stats->rx_dropped
            );
    }
    
IFADDRS_CLEANUP:
    freeifaddrs(ifa);
}

int main() {
    int epoll = epoll_create1(EPOLL_CLOEXEC);
    CHECK_RESULT(epoll, "epoll_create1", goto EPOLL_CREATE_CLEANUP);
    assert(epoll != -1);
    
    
    int timer = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    CHECK_RESULT(timer, "timerfd_create", goto TIMER_CLEANUP);
    assert(timer != -1);

    struct itimerspec timeout;
    timeout.it_interval.tv_sec = 1;
    timeout.it_interval.tv_nsec = 0;

    CHECK_RESULT(clock_gettime(CLOCK_MONOTONIC, &timeout.it_value),
                 "clock_gettime", goto TIMER_CLEANUP);
    timeout.it_value.tv_sec += 2;
    timeout.it_value.tv_nsec = 0;
    CHECK_RESULT(timerfd_settime(timer, TFD_TIMER_ABSTIME, &timeout, NULL),
                 "timerfd_settime", goto TIMER_CLEANUP);

    struct epoll_event timer_event;
    timer_event.events = EPOLLIN | EPOLLERR;
    timer_event.data.fd = timer;
    CHECK_RESULT(epoll_ctl(epoll, EPOLL_CTL_ADD, timer, &timer_event),
                 "epoll_ctl: timer", goto TIMER_CLEANUP);
        

    struct epoll_event events[MAX_EVENTS];
    for(;;) {
        int nfds = epoll_wait(epoll, events, MAX_EVENTS, -1);
        CHECK_RESULT(nfds, "epoll_wait", goto EPOLL_WAIT_CLEANUP);
        for(int i = 0; i < nfds; ++i) {
            if(events[i].data.fd == timer_event.data.fd) {
                CHECK_RESULT(timer_handler(timer_event.data.fd, &parse_proc_stat),
                             "timer_handler", goto EPOLL_WAIT_CLEANUP);
            }
        }
    }

EPOLL_WAIT_CLEANUP:

TIMER_CLEANUP:
    CHECK_RESULT(close(timer), "close: timer", abort());

EPOLL_CREATE_CLEANUP:
    CHECK_RESULT(close(epoll), "close: epoll", abort());
    return 0;
}
