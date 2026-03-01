#ifndef TICK_RUNTIME_H
#define TICK_RUNTIME_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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

typedef struct {
    FILE* handle;
} TickFile;

void tick_signal_init(TickSignal* sig);
void tick_signal_emit(TickSignal* sig, void* value);
void* tick_signal_recv(TickSignal* sig);
void tick_signal_destroy(TickSignal* sig);

void tick_event_init(TickEvent* evt, int capacity);
void tick_event_add_process(TickEvent* evt, TickProcessFunc func);
void tick_event_execute(TickEvent* evt);
void tick_event_wait(TickEvent* evt);
void tick_event_destroy(TickEvent* evt);

int32_t tick_str_length(const char* s);
char* tick_str_concat(const char* a, const char* b);
char* tick_str_substring(const char* s, int32_t start, int32_t end);
int32_t tick_str_index_of(const char* s, const char* substr);
int32_t tick_str_compare(const char* a, const char* b);
char tick_str_char_at(const char* s, int32_t index);
char* tick_str_from_i64(int64_t val);
int64_t tick_str_to_i64(const char* s);
char* tick_str_from_u64(uint64_t val);
char* tick_str_from_f64(double val);
double tick_str_to_f64(const char* s);
char* tick_str_from_b8(bool val);
bool tick_str_to_b8(const char* s);

char* tick_input_readline(const char* prompt);

TickFile* tick_file_open(const char* path, const char* mode);
char* tick_file_read(TickFile* f);
void tick_file_write(TickFile* f, const char* data);
void tick_file_close(TickFile* f);
bool tick_file_exists(const char* path);

typedef struct {
    void* ptr;
    int32_t len;
    int32_t cap;
} TickArray;

void tick_array_push(TickArray* arr, size_t elem_size);
void tick_array_pop(TickArray* arr);

#define TICK_GC_MAX_OBJECTS 65536

typedef struct {
    void* ptrs[TICK_GC_MAX_OBJECTS];
    int count;
} TickGC;

void tick_gc_init(void);
void* tick_gc_alloc(size_t size);
void* tick_gc_realloc(void* ptr, size_t new_size);
void tick_gc_free(void* ptr);
void tick_gc_collect(void);
void tick_gc_cleanup(void);

#endif
