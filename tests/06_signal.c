#include <stdio.h>
#include <assert.h>

#define POLLEN_LOG_DEBUG(fmt, ...) fprintf(stderr, "DEBUG: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_INFO(fmt, ...) fprintf(stderr, "INFO: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_WARN(fmt, ...) fprintf(stderr, "WARN: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_ERR(fmt, ...) fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_IMPLEMENTATION
#include "pollen.h"

int signal_callback(struct pollen_callback *callback, int signum, void *data) {
    return -69;
}

int main(void) {
    struct pollen_loop *loop;

    assert((loop = pollen_loop_create()));
    assert(pollen_loop_add_signal(loop, SIGUSR1, signal_callback, NULL));

    assert(raise(SIGUSR1) == 0);
    assert(pollen_loop_run(loop) == -69);

    pollen_loop_cleanup(loop);
}

