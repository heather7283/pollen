#include <sys/eventfd.h>
#include <stdio.h>
#include <assert.h>

#define POLLEN_LOG_DEBUG(fmt, ...) fprintf(stderr, "DEBUG: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_INFO(fmt, ...) fprintf(stderr, "INFO: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_WARN(fmt, ...) fprintf(stderr, "WARN: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_ERR(fmt, ...) fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_IMPLEMENTATION
#include "pollen.h"

bool callback_fired = false;

int efd_callback(struct pollen_callback *callback, int fd, uint32_t events, void *data) {
    return -69;
}

int main(void) {
    struct pollen_loop *loop;
    int efd;

    assert((efd = eventfd(0, EFD_NONBLOCK)) > 0);

    assert((loop = pollen_loop_create()));
    assert(pollen_loop_add_fd(loop, efd, EPOLLIN, true, efd_callback, NULL));

    uint64_t n = 1;
    assert(write(efd, &n, sizeof(n)) == sizeof(n));
    assert(pollen_loop_run(loop) == -69);

    pollen_loop_cleanup(loop);
}

