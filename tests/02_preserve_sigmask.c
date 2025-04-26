#include <assert.h>

#define POLLEN_LOG_DEBUG(fmt, ...) fprintf(stderr, "DEBUG: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_INFO(fmt, ...) fprintf(stderr, "INFO: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_WARN(fmt, ...) fprintf(stderr, "WARN: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_LOG_ERR(fmt, ...) fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)
#define POLLEN_IMPLEMENTATION
#include "pollen.h"

int signal_callback(struct pollen_callback *callback, int signum, void *data) {
    return 0;
}

int main(void) {
    struct pollen_loop *loop;
    sigset_t old_sigset, new_sigset;

    memset(&old_sigset, '\0', sizeof(sigset_t));
    memset(&new_sigset, '\0', sizeof(sigset_t));

    sigaddset(&old_sigset, SIGUSR1);
    assert(sigprocmask(SIG_BLOCK, &old_sigset, NULL) == 0);

    /* first, save original sigset */
    assert(sigprocmask(SIG_BLOCK /* ignored */, NULL, &old_sigset) == 0);

    assert((loop = pollen_loop_create()));

    struct pollen_callback *callback;
    callback = pollen_loop_add_signal(loop, SIGUSR2, signal_callback, NULL);
    assert(callback != NULL);
    pollen_loop_remove_callback(callback);

    /* sigset should not be modified in any way */
    assert(sigprocmask(SIG_BLOCK /* ignored */, NULL, &new_sigset) == 0);
    assert(memcmp(&old_sigset, &new_sigset, sizeof(sigset_t)) == 0);

    pollen_loop_cleanup(loop);
}

