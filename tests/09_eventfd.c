#include <stdio.h>
#include <assert.h>

#define POLLEN_LOG_DEBUG(fmt, ...) fprintf(stderr, "DEBUG: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_INFO(fmt, ...) fprintf(stderr, "INFO: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_WARN(fmt, ...) fprintf(stderr, "WARN: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_ERR(fmt, ...) fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_IMPLEMENTATION
#include "pollen.h"

int efd_callback(struct pollen_callback *callback, uint64_t n, void *data) {
    assert((*(int *)data) == 228);

    return -n;
}

int main(void) {
    struct pollen_loop *loop;
    struct pollen_callback *callback;

    assert((loop = pollen_loop_create()));

    int foo = 228;
    assert((callback = pollen_loop_add_efd(loop, efd_callback, &foo)));

    assert(pollen_efd_trigger(callback, 100500));
    assert(pollen_loop_run(loop) == -100500);

    pollen_loop_cleanup(loop);
}

