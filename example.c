#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define POLLEN_IMPLEMENTATION
#include "pollen.h"

static int stdin_callback(struct pollen_callback *callback, int fd, uint32_t events, void *data) {
    printf("stdin_callback: fired\n");

    /* retrieve and use pointer to user data */
    char *message = data;
    printf("stdin_callback: user data: %s\n", message);

    /* echo read data to stdout */
    static char buf[4096];
    int ret;
    while ((ret = read(fd, buf, sizeof(buf))) > 0) {
        write(1 /* stdout */, buf, ret);
    }

    if (ret == 0) {
        /* end of file, quit event loop */
        printf("stdin_callback: EOF on stdin, quitting event loop\n");
        pollen_loop_quit(pollen_callback_get_loop(callback), 0);
        return 0;
    } else { /* ret < 0 */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* no more data available to read for now. just return */
            return 0;
        } else {
            /* something went wrong, return error */
            int save_errno = errno;
            printf("stdin_callback: read() error: %s\n", strerror(errno));
            return -save_errno;
        }
    }
}

static int idle_callback(struct pollen_callback *loop_item, void *data) {
    printf("idle_callback: fired\n");
    printf("idle_callback: %s\n", (char *)data);

    return 0;
}

static int idle_callback_important(struct pollen_callback *loop_item, void *data) {
    printf("idle_callback_important: fired\n");
    printf("idle_callback_important: %s\n", (char *)data);

    return 0;
}

static int signals_callback(struct pollen_callback *loop_item, int signum, void *data) {
    printf("signals_callback: fired\n");
    printf("signals_callback: caught signal %d, exiting main loop\n", signum);

    pollen_loop_quit(pollen_callback_get_loop(loop_item), 0);

    return 0;
}

int main(void) {
    int ret = 0;

    /* set O_NONBLOCK flag on stdin so callback doesn't block forever */
    int flags = fcntl(0 /* stdin */, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(0 /* stdin */, F_SETFL, flags);

    struct pollen_loop *loop = pollen_loop_create();

    char *sus = "amogus"; /* you can pass any arbitrary pointer to callback */

    /* this callback will run when stdin (0) becomes available for reading (EPOLLIN) */
    pollen_loop_add_fd(loop, 0 /* stdin */, EPOLLIN, false, stdin_callback, sus);

    /*
     * Those callbacks will run on every event loop iteration after all other callback types
     * have been processed. Callbacks with higher priority will run before those with lower
     * priority.
     */
    pollen_loop_add_idle(loop, 0, idle_callback, "this callback has priority 0");
    pollen_loop_add_idle(loop, 5, idle_callback_important, "this callback has priority 5");

    /* Those callbacks will run on reception of specified signal. */
    pollen_loop_add_signal(loop, SIGINT, signals_callback, NULL);
    pollen_loop_add_signal(loop, SIGTERM, signals_callback, NULL);

    /* this will block until the loop is stopped */
    ret = pollen_loop_run(loop);

    pollen_loop_cleanup(loop);
    return ret;
}
