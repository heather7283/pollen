#include <sys/eventfd.h>
#include <stdio.h>
#include <assert.h>

#define POLLEN_LOG_DEBUG(fmt, ...) fprintf(stderr, "DEBUG: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_INFO(fmt, ...) fprintf(stderr, "INFO: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_WARN(fmt, ...) fprintf(stderr, "WARN: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_ERR(fmt, ...) fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_IMPLEMENTATION
#include "pollen.h"

int counter = 0;

int idle_callback_100(struct pollen_callback *callback, void *data) {
    assert(counter == 1000);
    counter = 100;
    return -69;
}

int idle_callback_1000(struct pollen_callback *callback, void *data) {
    assert(counter == 10000);
    counter = 1000;
    return 0;
}

int idle_callback_10000(struct pollen_callback *callback, void *data) {
    assert(counter == 0);
    counter = 10000;
    return 0;
}

int efd_callback(struct pollen_callback *callback, int fd, uint32_t events, void *data) {
    return 0;
}

int main(void) {
    struct pollen_loop *loop;
    int efd;

    assert((efd = eventfd(0, EFD_NONBLOCK)) > 0);

    assert((loop = pollen_loop_create()));
    assert(pollen_loop_add_fd(loop, efd, EPOLLIN, true, efd_callback, NULL));
    assert(pollen_loop_add_idle(loop, 100, idle_callback_100, NULL));
    assert(pollen_loop_add_idle(loop, 1000, idle_callback_1000, NULL));
    assert(pollen_loop_add_idle(loop, 10000, idle_callback_10000, NULL));

    uint64_t n = 1;
    assert(write(efd, &n, sizeof(n)) == sizeof(n));
    assert(pollen_loop_run(loop) == -69);

    assert(counter == 100);

    pollen_loop_cleanup(loop);
}

