/*
 * MIT License
 *
 * Copyright (c) 2025 heather7283
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This is a single-header library that provides simple event loop abstraction built on epoll.
 * To use this library, do this in one C file:
 *   #define EVENT_LOOP_IMPLEMENTATION
 *   #include "event_loop.h"
 *
 * COMPILE-TIME TUNABLES:
 *   EVENT_LOOP_CALLOC(n, size) - calloc()-like function that will be used to allocate memory.
 *     Default: #define EVENT_LOOP_CALLOC(n, size) calloc(n, size)
 *   EVENT_LOOP_FREE(ptr) - free()-like function that will be used to free memory.
 *     Default: #define EVENT_LOOP_FREE(ptr) free(ptr)
 *   EVENT_LOOP_ENABLE_LOGGING - enables debug logging if defined.
 *     Default: not defined
 *   EVENT_LOOP_DEBUG(fmt, ...) - printf()-like function that will be used for debug logging.
 *     Default: fprintf(stderr, "event loop: " fmt "\n", ##__VA_ARGS__)
 *   EVENT_LOOP_ERR(fmt, ...) - printf()-like function that will be used for logging errors.
 *     Default: fprintf(stderr, "event loop: ERR: " fmt "\n", ##__VA_ARGS__)
 *   EVENT_LOOP_WARN(fmt, ...) - printf()-like function that will be used for logging warnings.
 *     Default: fprintf(stderr, "event loop: WARN: " fmt "\n", ##__VA_ARGS__)
 */

#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#if !defined(EVENT_LOOP_CALLOC) || !defined(EVENT_LOOP_FREE)
    #include <stdlib.h>
#endif

#if !defined(EVENT_LOOP_CALLOC)
    #define EVENT_LOOP_CALLOC(n, size) calloc(n, size)
#endif

#if !defined(EVENT_LOOP_FREE)
    #define EVENT_LOOP_FREE(ptr) free(ptr)
#endif

#if defined(EVENT_LOOP_ENABLE_LOGGING)
    #if !defined(EVENT_LOOP_DEBUG) || !defined(EVENT_LOOP_WARN) || !defined(EVENT_LOOP_ERR)
        #include <stdio.h> /* fprintf() */
        #include <string.h> /* strerror() */
    #endif

    #if !defined(EVENT_LOOP_DEBUG)
        #define EVENT_LOOP_DEBUG(fmt, ...) \
            fprintf(stderr, "event loop: " fmt "\n", ##__VA_ARGS__)
    #endif
    #if !defined(EVENT_LOOP_WARN)
        #define EVENT_LOOP_WARN(fmt, ...) \
            fprintf(stderr, "event loop: WARN: " fmt "\n", ##__VA_ARGS__)
    #endif
    #if !defined(EVENT_LOOP_ERR)
        #define EVENT_LOOP_ERR(fmt, ...) \
            fprintf(stderr, "event loop: ERR: " fmt "\n", ##__VA_ARGS__)
    #endif
#else
    #define EVENT_LOOP_DEBUG(fmt, ...) /* no-op */
    #define EVENT_LOOP_WARN(fmt, ...) /* no-op */
    #define EVENT_LOOP_ERR(fmt, ...) /* no-op */
#endif

struct event_loop_item;
typedef int (*event_loop_callback_t)(void *data, struct event_loop_item *loop_item);

/* Creates a new event_loop instance. Returns NULL and sets errno on failure. */
struct event_loop *event_loop_create(void);
/* Frees all resources associated with the loop. Loop is not valid after this function returns. */
void event_loop_cleanup(struct event_loop *loop);

/*
 * Add new callback to event loop.
 * If fd >= 0, fd is added to epoll interest list.
 * If fd < 0, callback will run unconditionally on every event loop iteration.
 *   In this case, negated value of fd is treated as priority.
 *   Callbacks with higher priority will run before callbacks with lower priority.
 *   If two callbacks have equal priority, the order is undefined.
 */
struct event_loop_item *event_loop_add_callback(struct event_loop *loop, int fd,
                                                event_loop_callback_t callback, void *data);
/* Remove a callback from event loop. */
void event_loop_remove_item(struct event_loop_item *item);

/* Get event_loop instance associated with this event_loop_item. */
struct event_loop *event_loop_item_get_loop(struct event_loop_item *item);
/*
 * Get file descriptor associated with this event_loop_item.
 * If item refers to an unconditional callback, -1 is returned.
 */
int event_loop_item_get_fd(struct event_loop_item *item);

/*
 * Run the event loop. This function blocks until event loop exits.
 * This function returns 0 if no errors occured.
 * If any of the callbacs return negative value, the loop with be stopped and this value returned.
 */
int event_loop_run(struct event_loop *loop);
/*
 * Quit the event loop.
 * Argument retcode specifies the value that will be returned by event_loop_run.
 */
void event_loop_quit(struct event_loop *loop, int retcode);

#endif /* #ifndef EVENT_LOOP_H */

/*
 * ============================================================================
 *                              IMPLEMENTATION
 * ============================================================================
 */
#ifdef EVENT_LOOP_IMPLEMENTATION

#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>
#include <fcntl.h>

#define EVENT_LOOP_EPOLL_MAX_EVENTS 16

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define EVENT_LOOP_TYPEOF(expr) typeof(expr)
#else
    #define EVENT_LOOP_TYPEOF(expr) __typeof__(expr)
#endif

#define EVENT_LOOP_CONTAINER_OF(ptr, sample, member) \
    (EVENT_LOOP_TYPEOF(sample))((char *)(ptr) - offsetof(EVENT_LOOP_TYPEOF(*sample), member))

/*
 * Linked list.
 * In the head, next points to the first list elem, prev points to the last.
 * In the list element, next points to the next elem, prev points to the previous elem.
 * In the last element, next points to the head. In the first element, prev points to the head.
 * If the list is empty, next and prev point to the head itself.
 */
struct event_loop_ll {
    struct event_loop_ll *next;
    struct event_loop_ll *prev;
};

static inline void event_loop_ll_init(struct event_loop_ll *head) {
    head->next = head;
    head->prev = head;
}

static inline bool event_loop_ll_is_empty(struct event_loop_ll *head) {
    return head->next == head && head->prev == head;
}

/* Inserts new after elem. */
static inline void event_loop_ll_insert(struct event_loop_ll *elem, struct event_loop_ll *new) {
    elem->next->prev = new;
    new->next = elem->next;

    elem->next = new;
    new->prev = elem;
}

static inline void event_loop_ll_remove(struct event_loop_ll *elem) {
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
}

#define EVENT_LOOP_LL_FOR_EACH(var, head, member) \
    for (var = EVENT_LOOP_CONTAINER_OF((head)->next, var, member); \
         &var->member != (head); \
         var = EVENT_LOOP_CONTAINER_OF(var->member.next, var, member))

#define EVENT_LOOP_LL_FOR_EACH_REVERSE(var, head, member) \
    for (var = EVENT_LOOP_CONTAINER_OF((head)->prev, var, member); \
         &var->member != (head); \
         var = EVENT_LOOP_CONTAINER_OF(var->member.prev, var, member))

#define EVENT_LOOP_LL_FOR_EACH_SAFE(var, tmp, head, member) \
    for (var = EVENT_LOOP_CONTAINER_OF((head)->next, var, member), \
         tmp = EVENT_LOOP_CONTAINER_OF((var)->member.next, tmp, member); \
         &var->member != (head); \
         var = tmp, \
         tmp = EVENT_LOOP_CONTAINER_OF(var->member.next, tmp, member))

struct event_loop_item {
    struct event_loop *loop;

    int fd;
    int priority;
    void *data;
    event_loop_callback_t callback;

    struct event_loop_ll link;
};

struct event_loop {
    bool should_quit;
    int retcode;
    int epoll_fd;

    struct event_loop_ll items;
    struct event_loop_ll unconditional_items;
};

static bool fd_is_valid(int fd) {
    if ((fcntl(fd, F_GETFD) < 0) && (errno == EBADF)) {
        return false;
    }
    return true;
}

struct event_loop *event_loop_create(void) {
    EVENT_LOOP_DEBUG("create");
    int save_errno = 0;

    struct event_loop *loop = EVENT_LOOP_CALLOC(1, sizeof(*loop));
    if (loop == NULL) {
        save_errno = errno;
        EVENT_LOOP_ERR("failed to allocate memory for event loop: %s", strerror(errno));
        goto err;
    }

    event_loop_ll_init(&loop->items);
    event_loop_ll_init(&loop->unconditional_items);

    loop->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (loop->epoll_fd < 0) {
        save_errno = errno;
        EVENT_LOOP_ERR("failed to create epoll: %s", strerror(errno));
        goto err;
    }

    return loop;

err:
    EVENT_LOOP_FREE(loop);
    errno = save_errno;
    return NULL;
}

void event_loop_cleanup(struct event_loop *loop) {
    EVENT_LOOP_DEBUG("cleanup");

    struct event_loop_item *item, *item_tmp;
    EVENT_LOOP_LL_FOR_EACH_SAFE(item, item_tmp, &loop->items, link) {
        event_loop_remove_item(item);
    }
    EVENT_LOOP_LL_FOR_EACH_SAFE(item, item_tmp, &loop->unconditional_items, link) {
        event_loop_remove_item(item);
    }

    close(loop->epoll_fd);

    EVENT_LOOP_FREE(loop);
}

struct event_loop_item *event_loop_add_callback(struct event_loop *loop, int fd,
                                                event_loop_callback_t callback, void *data) {
    struct event_loop_item *new_item = NULL;
    int save_errno = 0;

    if (fd >= 0) {
        EVENT_LOOP_DEBUG("adding fd %d to event loop", fd);

        new_item = EVENT_LOOP_CALLOC(1, sizeof(*new_item));
        if (new_item == NULL) {
            save_errno = errno;
            EVENT_LOOP_ERR("failed to allocate memory for callback: %s", strerror(errno));
            goto err;
        }
        new_item->loop = loop;
        new_item->fd = fd;
        new_item->priority = -1;
        new_item->callback = callback;
        new_item->data = data;

        struct epoll_event epoll_event;
        epoll_event.events = EPOLLIN;
        epoll_event.data.ptr = new_item;
        if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &epoll_event) < 0) {
            save_errno = errno;
            EVENT_LOOP_ERR("failed to add fd %d to epoll: %s", fd, strerror(errno));
            goto err;
        }

        event_loop_ll_insert(&loop->items, &new_item->link);
    } else {
        int priority = -fd;
        EVENT_LOOP_DEBUG("adding unconditional callback with prio %d to event loop", priority);

        new_item = EVENT_LOOP_CALLOC(1, sizeof(*new_item));
        if (new_item == NULL) {
            save_errno = errno;
            EVENT_LOOP_ERR("failed to allocate memory for callback: %s", strerror(errno));
            goto err;
        }
        new_item->loop = loop;
        new_item->fd = -1;
        new_item->priority = priority;
        new_item->callback = callback;
        new_item->data = data;

        if (event_loop_ll_is_empty(&loop->unconditional_items)) {
            event_loop_ll_insert(&loop->unconditional_items, &new_item->link);
        } else {
            struct event_loop_item *elem;
            EVENT_LOOP_LL_FOR_EACH_REVERSE(elem, &loop->unconditional_items, link) {
                /*         |6|
                 * |9|  |8|\/|4|  |2|
                 * <-----------------
                 * iterate from the end and find the first item with higher prio
                 */
                bool found = false;
                if (elem->priority > priority) {
                    found = true;
                    event_loop_ll_insert(&elem->link, &new_item->link);
                }
                if (!found) {
                    event_loop_ll_insert(&loop->unconditional_items, &new_item->link);
                }
            }
        }
    }

    return new_item;

err:
    EVENT_LOOP_FREE(new_item);
    errno = save_errno;
    return NULL;
}

void event_loop_remove_item(struct event_loop_item *item) {
    if (item->fd >= 0) {
        EVENT_LOOP_DEBUG("removing fd %d from event loop", item->fd);

        if (fd_is_valid(item->fd)) {
            if (epoll_ctl(item->loop->epoll_fd, EPOLL_CTL_DEL, item->fd, NULL) < 0) {
                int ret = -errno;
                EVENT_LOOP_ERR("failed to remove fd %d from epoll (%s)",
                    item->fd, strerror(errno));
                event_loop_quit(item->loop, ret);
            }
            close(item->fd);
        } else {
            EVENT_LOOP_WARN("fd %d is not valid, was it closed somewhere else?", item->fd);
        }
    } else {
        EVENT_LOOP_DEBUG("removing unconditional callback with prio %d from event loop",
              item->priority);
    }

    event_loop_ll_remove(&item->link);

    EVENT_LOOP_FREE(item);
}

struct event_loop *event_loop_item_get_loop(struct event_loop_item *item) {
    return item->loop;
}

int event_loop_item_get_fd(struct event_loop_item *item) {
    return item->fd;
}

int event_loop_run(struct event_loop *loop) {
    EVENT_LOOP_DEBUG("run");

    int ret = 0;
    int number_fds = -1;
    struct epoll_event events[EVENT_LOOP_EPOLL_MAX_EVENTS];

    loop->should_quit = false;
    while (!loop->should_quit) {
        do {
            number_fds = epoll_wait(loop->epoll_fd, events, EVENT_LOOP_EPOLL_MAX_EVENTS, -1);
        } while (number_fds == -1 && errno == EINTR); /* epoll_wait failing with EINTR is normal */

        if (number_fds == -1) {
            ret = errno;
            EVENT_LOOP_ERR("epoll_wait error (%s)", strerror(errno));
            loop->retcode = -ret;
            goto out;
        }

        EVENT_LOOP_DEBUG("received events on %d fds", number_fds);

        for (int n = 0; n < number_fds; n++) {
            struct event_loop_item *item = events[n].data.ptr;
            EVENT_LOOP_DEBUG("processing item with fd %d", item->fd);

            ret = item->callback(item->data, item);
            if (ret < 0) {
                EVENT_LOOP_ERR("callback returned negative, quitting");
                loop->retcode = ret;
                goto out;
            }
        }

        struct event_loop_item *item;
        EVENT_LOOP_LL_FOR_EACH(item, &loop->unconditional_items, link) {
            EVENT_LOOP_DEBUG("running unconditional callback with prio %d", item->priority);
            ret = item->callback(item->data, item);
            if (ret < 0) {
                EVENT_LOOP_ERR("callback returned negative, quitting");
                loop->retcode = ret;
                goto out;
            }
        }
    }

out:
    return loop->retcode;
}

void event_loop_quit(struct event_loop *loop, int retcode) {
    EVENT_LOOP_DEBUG("quit");

    loop->should_quit = true;
    loop->retcode = retcode;
}

#endif /* #ifndef EVENT_LOOP_IMPLEMENTATION */

