#include <stdio.h>                         
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "queue.h"

#define BUFFER_SIZE (getpagesize())
#define NUM_THREADS (1)
#define MESSAGES_PER_THREAD (getpagesize() * 2)

uint32_t comp(const void *elem1, const void *elem2)
{
    uint32_t f = *((uint32_t *) elem1);
    uint32_t s = *((uint32_t *) elem2);
    if (f > s)
        return 1;
    if (f < s)
        return -1;
    return 0;
}

typedef struct {
    queue_t q;
    uint32_t messages_per_thread;
    uint32_t num_threads;
} rbuf_t;

size_t in[65536];
size_t out[65536];

/**
 * @brief Get timestamp
 * @return timestamp now
 */
uint64_t get_time()
{
    struct timespec ts;
    clock_gettime(0, &ts);
    return (uint64_t)(ts.tv_sec * 1e6 + ts.tv_nsec / 1e3);
}

static void *publisher_loop(void *arg)
{
    rbuf_t *r = (rbuf_t *) arg;
    size_t i;
    size_t **publisher_ptr = malloc(sizeof(size_t *));
    *publisher_ptr = in;
    for (i = 0; i < r->messages_per_thread * r->num_threads; i++)
        queue_put(&r->q, (uint8_t **) publisher_ptr, sizeof(size_t));
    return (void *) i;
}

static void *consumer_loop(void *arg)
{
    rbuf_t *r = (rbuf_t *) arg;
    size_t i;
    size_t **consumer_ptr = malloc(sizeof(size_t *));
    *consumer_ptr = out;
    for (i = 0; i < r->messages_per_thread; i++) {
        queue_get(&r->q, (uint8_t **) consumer_ptr, sizeof(size_t));
        // printf("%ld\n", x);
    }
    return (void *) i;
}

int main(int argc, char *argv[])
{
    uint32_t time[100];
    for (int i = 0; i < 100; i++) {
        for(size_t i = 0; i < 65536UL; i++) {
            in[i] = i;
            out[i] = 0UL;
        }

        rbuf_t r;
        r.num_threads = 1U;
        if (argc > 1)
            r.messages_per_thread = (uint32_t) atoi(argv[1]);
        else
            r.messages_per_thread = 65536;
        
        queue_init(&r.q, BUFFER_SIZE);

        uint64_t start = get_time();

        pthread_t publisher;
        pthread_t consumer;

        pthread_attr_t attr;
        pthread_attr_init(&attr);

        pthread_create(&publisher, &attr, &publisher_loop, (void *) &r);

        pthread_create(&consumer, &attr, &consumer_loop, (void *) &r);

        intptr_t sent;
        pthread_join(publisher, (void **) &sent);

        intptr_t recd;
        pthread_join(consumer, (void **) &recd);

        uint64_t end = get_time();
        // printf("\npublisher sent %ld messages\n", sent);
        // printf("consumer received %ld messages\n", recd);

        time[i] = end - start;

        pthread_attr_destroy(&attr);

        queue_destroy(&r.q);
    }

    // for(size_t i = 0; i < 65536UL; i++)
    //     printf("%ld\n", out[i]);

    qsort(time, 100U, sizeof(uint32_t), comp);
    long long avg = 0UL;
    for (int num = 16; num < 84; num++) {
        avg += time[num];
    }
    avg /= 68;
    // printf("average time : %lldus\n", avg);
    printf("%lld\n", avg);

    return 0;
}