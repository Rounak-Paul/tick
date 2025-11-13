#ifndef TICK_RUNTIME_H
#define TICK_RUNTIME_H

#include <pthread.h>
#include <stdbool.h>

#define SIGNAL_QUEUE_SIZE 1024

typedef struct {
    void* data[SIGNAL_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TickSignal;

typedef void* (*TickProcessFunc)(void*);

typedef struct {
    TickProcessFunc* process_funcs;
    int process_count;
    pthread_t* threads;
} TickEvent;

void tick_signal_init(TickSignal* sig);
void tick_signal_emit(TickSignal* sig, void* value);
void* tick_signal_recv(TickSignal* sig);
void tick_signal_destroy(TickSignal* sig);

void tick_event_init(TickEvent* evt, int capacity);
void tick_event_add_process(TickEvent* evt, TickProcessFunc func);
void tick_event_execute(TickEvent* evt);
void tick_event_wait(TickEvent* evt);
void tick_event_destroy(TickEvent* evt);

#endif
