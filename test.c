#include <sys/eventfd.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#define EVENT_LOOP_ENABLE_LOGGING
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

int unconditional_callback(struct event_loop_item *loop_item, uint32_t events) {
    static int foo = 0;

    printf("unconditional_callback: fired, foo = %d\n", foo);

    if (foo++ == 5) {
        event_loop_quit(event_loop_item_get_loop(loop_item), 0);
    }

    return 0;
}

int unconditional_callback_low_prio(struct event_loop_item *loop_item, uint32_t events) {
    printf("unconditional_callback_low_prio: fired\n");

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

    assert(event_loop_add_callback(loop, efd, EPOLLIN, eventfd_callback, &efd));
    assert(event_loop_add_callback(loop, -100, EPOLLIN, unconditional_callback, NULL));
    assert(event_loop_add_callback(loop, -1, EPOLLIN, unconditional_callback_low_prio, NULL));
    assert(event_loop_run(loop) == 0);

    event_loop_cleanup(loop);
}

