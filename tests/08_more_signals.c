#include <assert.h>

#define POLLEN_LOG_DEBUG(fmt, ...) fprintf(stderr, "DEBUG: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_INFO(fmt, ...) fprintf(stderr, "INFO: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_WARN(fmt, ...) fprintf(stderr, "WARN: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_ERR(fmt, ...) fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_IMPLEMENTATION
#include "pollen.h"

int foo = 0;

int sigusr1_callback(struct pollen_callback *callback, int signum, void *data) {
    assert(foo == 0);
    foo = 1;

    assert(raise(SIGUSR2) == 0);

    return 0;
}

int sigusr2_callback(struct pollen_callback *callback, int signum, void *data) {
    assert(foo == 1);
    foo = 2;

    assert(raise(SIGALRM) == 0);

    return 0;
}

int sigalrm_callback(struct pollen_callback *callback, int signum, void *data) {
    assert(foo == 2);
    foo = 3;

    return -69;
}

int main(void) {
    struct pollen_loop *loop;

    assert((loop = pollen_loop_create()));
    assert(pollen_loop_add_signal(loop, SIGUSR1, sigusr1_callback, NULL));
    assert(pollen_loop_add_signal(loop, SIGUSR2, sigusr2_callback, NULL));
    assert(pollen_loop_add_signal(loop, SIGALRM, sigalrm_callback, NULL));

    assert(raise(SIGUSR1) == 0);
    assert(pollen_loop_run(loop) == -69);

    assert(foo == 3);

    pollen_loop_cleanup(loop);
}

