#include "tick_runtime.h"
#include <stdlib.h>
#include <string.h>

void tick_signal_init(TickSignal* sig) {
    sig->head = 0;
    sig->tail = 0;
    sig->count = 0;
    pthread_mutex_init(&sig->mutex, NULL);
    pthread_cond_init(&sig->cond, NULL);
}

void tick_signal_emit(TickSignal* sig, void* value) {
    pthread_mutex_lock(&sig->mutex);
    
    if (sig->count < SIGNAL_QUEUE_SIZE) {
        sig->data[sig->tail] = value;
        sig->tail = (sig->tail + 1) % SIGNAL_QUEUE_SIZE;
        sig->count++;
        pthread_cond_signal(&sig->cond);
    }
    
    pthread_mutex_unlock(&sig->mutex);
}

void* tick_signal_recv(TickSignal* sig) {
    pthread_mutex_lock(&sig->mutex);
    
    while (sig->count == 0) {
        pthread_cond_wait(&sig->cond, &sig->mutex);
    }
    
    void* value = sig->data[sig->head];
    sig->head = (sig->head + 1) % SIGNAL_QUEUE_SIZE;
    sig->count--;
    
    pthread_mutex_unlock(&sig->mutex);
    return value;
}

void tick_signal_destroy(TickSignal* sig) {
    pthread_mutex_destroy(&sig->mutex);
    pthread_cond_destroy(&sig->cond);
}

void tick_event_init(TickEvent* evt, int capacity) {
    evt->process_funcs = (TickProcessFunc*)malloc(sizeof(TickProcessFunc) * capacity);
    evt->threads = (pthread_t*)malloc(sizeof(pthread_t) * capacity);
    evt->process_count = 0;
}

void tick_event_add_process(TickEvent* evt, TickProcessFunc func) {
    evt->process_funcs[evt->process_count++] = func;
}

void tick_event_execute(TickEvent* evt) {
    for (int i = 0; i < evt->process_count; i++) {
        pthread_create(&evt->threads[i], NULL, evt->process_funcs[i], NULL);
    }
}

void tick_event_wait(TickEvent* evt) {
    for (int i = 0; i < evt->process_count; i++) {
        pthread_join(evt->threads[i], NULL);
    }
}

void tick_event_destroy(TickEvent* evt) {
    free(evt->process_funcs);
    free(evt->threads);
}
