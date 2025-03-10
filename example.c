#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define EVENT_LOOP_IMPLEMENTATION
#include "event_loop.h"

static int stdin_callback(struct event_loop_item *loop_item, uint32_t events) {
    printf("stdin_callback: fired\n");

    /* retrieve and use pointer to user data */
    char *message = event_loop_item_get_data(loop_item);
    printf("stdin_callback: user data: %s\n", message);

    /* get fd associated with this callback */
    int fd = event_loop_item_get_fd(loop_item);

    /* echo read data to stdout */
    static char buf[4096];
    int ret;
    while ((ret = read(fd, buf, sizeof(buf))) > 0) {
        write(1 /* stdout */, buf, ret);
    }

    if (ret == 0) {
        /* end of file, quit event loop */
        printf("stdin_callback: EOF on stdin, quitting event loop\n");
        event_loop_quit(event_loop_item_get_loop(loop_item), 0);
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

static int unconditional_callback(struct event_loop_item *loop_item) {
    printf("unconditional_callback: fired\n");
    printf("unconditional_callback: %s\n", (char *)event_loop_item_get_data(loop_item));

    return 0;
}

static int unconditional_callback2(struct event_loop_item *loop_item) {
    printf("unconditional_callback2: fired\n");
    printf("unconditional_callback2: %s\n", (char *)event_loop_item_get_data(loop_item));

    return 0;
}

static int signals_callback(struct event_loop_item *loop_item, int signal) {
    printf("signals_callback: fired\n");
    printf("signals_callback: caught signal %d, exiting main loop\n", signal);

    event_loop_quit(event_loop_item_get_loop(loop_item), 0);

    return 0;
}

int main(void) {
    int ret = 0;

    /* set O_NONBLOCK flag on stdin so callback doesn't block forever */
    int flags = fcntl(0 /* stdin */, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(0 /* stdin */, F_SETFL, flags);

    struct event_loop *loop = event_loop_create();

    char *sus = "amogus"; /* you can pass any arbitrary pointer to callback */

    /* this callback will run when stdin (0) becomes available for reading (EPOLLIN) */
    event_loop_add_pollable(loop, 0 /* stdin */, EPOLLIN, stdin_callback, sus);

    /*
     * Those callbacks will run on every event loop iteration after all other callback types
     * have been processed. Callbacks with higher priority will run before those with lower
     * priority.
     */
    event_loop_add_unconditional(loop, 0, unconditional_callback, "this callback has priority 0");
    event_loop_add_unconditional(loop, 5, unconditional_callback2, "this callback has priority 5");

    /* Those callbacks will run on reception of specified signal. */
    event_loop_add_signal(loop, SIGINT, signals_callback, NULL);
    event_loop_add_signal(loop, SIGTERM, signals_callback, NULL);

    /* this will block until the loop is stopped */
    ret = event_loop_run(loop);

    event_loop_cleanup(loop);
    return ret;
}
