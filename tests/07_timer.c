#include <stdio.h>
#include <time.h>
#include <assert.h>

#define POLLEN_LOG_DEBUG(fmt, ...) fprintf(stderr, "DEBUG: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_INFO(fmt, ...) fprintf(stderr, "INFO: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_WARN(fmt, ...) fprintf(stderr, "WARN: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_ERR(fmt, ...) fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_IMPLEMENTATION
#include "pollen.h"

struct timespec timespec_sub(const struct timespec *lhs, const struct timespec *rhs) {
	struct timespec zero = {
		.tv_sec = 0,
		.tv_nsec = 0,
	};
	struct timespec ret;

	if (lhs->tv_sec < rhs->tv_sec) {
		return zero;
    }

	ret.tv_sec = lhs->tv_sec - rhs->tv_sec;

	if (lhs->tv_nsec < rhs->tv_nsec) {
		if (ret.tv_sec == 0) {
			return zero;
        }

		ret.tv_sec--;
		ret.tv_nsec = 1000000000L - rhs->tv_nsec + lhs->tv_nsec;
	} else {
		ret.tv_nsec = lhs->tv_nsec - rhs->tv_nsec;
    }

	return ret;
}

int timer_callback(struct pollen_callback *callback, void *data) {
    static int counter = 0;

    struct timespec *ts_start = data;

    struct timespec ts_now;
    assert(clock_gettime(CLOCK_MONOTONIC, &ts_now) == 0);
    fprintf(stderr, "time: %lus%lums\n", ts_now.tv_sec, ts_now.tv_nsec / 1000000L);
    struct timespec ts_elapsed = timespec_sub(&ts_now, ts_start);
    fprintf(stderr, "elapsed: %lus%lums\n", ts_elapsed.tv_sec, ts_elapsed.tv_nsec / 1000000L);

    if (++counter == 5) {
        return -69;
    } else {
        return 0;
    }
}

int main(void) {
    struct pollen_loop *loop;
    struct timespec timespec_first, timespec_second, timespec_diff;

    assert(clock_gettime(CLOCK_MONOTONIC, &timespec_first) == 0);

    assert((loop = pollen_loop_create()));
    assert(pollen_loop_add_timer(loop, 100, timer_callback, &timespec_first));

    assert(pollen_loop_run(loop) == -69);

    assert(clock_gettime(CLOCK_MONOTONIC, &timespec_second) == 0);

    timespec_diff = timespec_sub(&timespec_second, &timespec_first);
    fprintf(stderr, "%lus%lums\n", timespec_diff.tv_sec, timespec_diff.tv_nsec / 1000000L);
    assert(timespec_diff.tv_nsec / 100000000L == 5);

    pollen_loop_cleanup(loop);
}

