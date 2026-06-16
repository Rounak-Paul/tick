#ifndef TICK_RUNTIME_H
#define TICK_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

/* ------------------------------------------------------------------ */
/* Validation traps. Compiled in for dev/validate, out for release.   */
/* The codegen emits calls to these only in the active build mode.    */
/* ------------------------------------------------------------------ */

void tick_trap_bounds(const char* where, int64_t index, int64_t len);
void tick_trap_unwrap(const char* where);
void tick_trap_overflow(const char* where);
void tick_trap_use_after_move(const char* where);
void tick_trap_weak_dead(const char* where);

/* ------------------------------------------------------------------ */
/* Strings: immutable, heap-owned, NUL-terminated.                    */
/* ------------------------------------------------------------------ */

char* tick_str_concat(const char* a, const char* b);
int32_t tick_str_len(const char* s);
int32_t tick_str_order(const char* a, const char* b);
bool tick_str_eq(const char* a, const char* b);
char* tick_str_from_i64(int64_t v);
char* tick_str_from_u64(uint64_t v);
char* tick_str_from_f64(double v);
char* tick_str_from_bool(bool v);
char* tick_str_dup(const char* s);

/* ------------------------------------------------------------------ */
/* Dynamic arrays of value elements.                                  */
/* ------------------------------------------------------------------ */

typedef struct {
    void* data;
    int32_t len;
    int32_t cap;
    int32_t elem_size;
} TickArray;

TickArray tick_array_new(int32_t elem_size);
TickArray tick_array_copy(TickArray src);   /* deep value copy */
void  tick_array_reserve(TickArray* a, int32_t needed);
void* tick_array_at(TickArray* a, int32_t index);   /* checked in validate/dev */
void* tick_array_at_unchecked(TickArray* a, int32_t index);
void  tick_array_push(TickArray* a, const void* elem);
void  tick_array_free(TickArray* a);

/* ------------------------------------------------------------------ */
/* Shared (reference counted) heap objects.                           */
/* Layout: [strong][weak][payload...]. Pointers handed to the program */
/* point at the payload; the header sits just before it.              */
/* ------------------------------------------------------------------ */

typedef struct {
    int32_t strong;
    int32_t weak;
} TickShared;

void* tick_shared_new(int32_t payload_size);
void* tick_shared_retain(void* payload);
void  tick_shared_release(void* payload);           /* frees payload at strong==0 */
void* tick_weak_from(void* payload);                /* +weak, returns payload */
void  tick_weak_drop(void* payload);
bool  tick_weak_alive(void* payload);

/* ------------------------------------------------------------------ */
/* Signals & events (concurrency).                                    */
/* ------------------------------------------------------------------ */

#define TICK_SIGNAL_CAP 1024

typedef struct {
    void* data[TICK_SIGNAL_CAP];
    int head, tail, count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TickSignal;

typedef void* (*TickProcessFn)(void*);

typedef struct {
    TickProcessFn* fns;
    pthread_t* threads;
    int count;
    int cap;
} TickEvent;

void  tick_signal_init(TickSignal* s);
void  tick_signal_emit(TickSignal* s, void* value);
void* tick_signal_recv(TickSignal* s);

void  tick_event_init(TickEvent* e, int cap);
void  tick_event_bind(TickEvent* e, TickProcessFn fn);
void  tick_event_fire(TickEvent* e);

#endif
