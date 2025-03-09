#include <sys/eventfd.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#define EVENT_LOOP_LOG_DEBUG(fmt, ...) fprintf(stderr, "event loop DEBUG: "fmt"\n", ##__VA_ARGS__)
#define EVENT_LOOP_LOG_WARN(fmt, ...) fprintf(stderr, "event loop WARN: "fmt"\n", ##__VA_ARGS__)
#define EVENT_LOOP_LOG_ERR(fmt, ...) fprintf(stderr, "event loop ERROR: "fmt"\n", ##__VA_ARGS__)
//#define EVENT_LOOP_IMPLEMENTATION
#include "event_loop.h"

int eventfd_callback(struct event_loop_item *loop_item, uint32_t events) {
    printf("eventfd_callback: fired\n");

    int efd = *(int *)event_loop_item_get_data(loop_item);

    uint64_t n;
    while (read(efd, &n, sizeof(n)) > 0) {
        printf("eventfd_callback: read %lu\n", n);
    }

    write(efd, &n, sizeof(n));

    return 0;
}

int unconditional_callback(struct event_loop_item *loop_item) {
    static int foo = 0;

    printf("unconditional_callback: fired, foo = %d\n", foo);

    if (foo++ == 5) {
        event_loop_quit(event_loop_item_get_loop(loop_item), 0);
    }

    return 0;
}

int unconditional_callback_low_prio(struct event_loop_item *loop_item) {
    printf("unconditional_callback_low_prio: fired, message %s\n",
           (char *)event_loop_item_get_data(loop_item));

    event_loop_remove_callback(loop_item);

    return 0;
}

int unconditional_callback_super_low_prio(struct event_loop_item *loop_item) {
    printf("unconditional_callback_low_prio: fired, message %s\n",
           (char *)event_loop_item_get_data(loop_item));

    event_loop_remove_callback(loop_item);

    return 0;
}

int main(void) {
    int efd;
    efd = eventfd(0, EFD_NONBLOCK);
    assert(!(efd < 0));

    struct event_loop *loop = event_loop_create();
    assert(loop != NULL);

    uint64_t n;

    n = 1;
    write(efd, &n, sizeof(n));

    assert(event_loop_add_pollable(loop, efd, EPOLLIN, eventfd_callback, &efd));
    assert(event_loop_add_unconditional(loop, -100, unconditional_callback_super_low_prio, "bwaa"));
    assert(event_loop_add_unconditional(loop, 100, unconditional_callback, NULL));
    assert(event_loop_add_unconditional(loop, 1, unconditional_callback_low_prio, "AMOGUS"));
    assert(event_loop_run(loop) == 0);

    event_loop_cleanup(loop);
}

