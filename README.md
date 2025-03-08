# event_loop.h
This is a [stb]-style single-header library providing event loop abstraction on top of [epoll].

## Usage:
To use this library, simply copy it into your project. Then, in one (only one!) C file:
```C
#define EVENT_LOOP_IMPLEMENTATION
#include "event_loop.h"
```
For documentation on each function, see the source code.

## Example:
Usage example that monitors fd 0 (stdin) and echoes read data to stdout:
```C
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

int main(void) {
    int ret = 0;

    /* set O_NONBLOCK flag on stdin so callback doesn't block forever */
    int flags = fcntl(0 /* stdin */, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(0 /* stdin */, F_SETFL, flags);

    struct event_loop *loop = event_loop_create();

    char *sus = "amogus"; /* you can pass a pointer to callback */
    event_loop_add_callback(loop, 0 /* stdin */, EPOLLIN, stdin_callback, sus);

    ret = event_loop_run(loop); /* this will block until the loop is stopped */

    event_loop_cleanup(loop);
    return ret;
}
```

[stb]: https://github.com/nothings/stb
[epoll]: https://www.man7.org/linux/man-pages/man7/epoll.7.html
