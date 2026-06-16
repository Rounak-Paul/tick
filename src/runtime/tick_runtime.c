#include "tick_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- validation traps ---- */

void tick_trap_bounds(const char* where, int64_t index, int64_t len) {
    fflush(stdout);
    fprintf(stderr, "tick: out-of-bounds access at %s: index %lld, length %lld\n",
            where, (long long)index, (long long)len);
    abort();
}
void tick_trap_unwrap(const char* where) {
    fprintf(stderr, "tick: unwrap of empty value at %s\n", where);
    abort();
}
void tick_trap_overflow(const char* where) {
    fprintf(stderr, "tick: integer overflow at %s\n", where);
    abort();
}
void tick_trap_use_after_move(const char* where) {
    fprintf(stderr, "tick: use after move at %s\n", where);
    abort();
}
void tick_trap_weak_dead(const char* where) {
    fprintf(stderr, "tick: upgrade of dead weak reference at %s\n", where);
    abort();
}

/* ---- strings ---- */

char* tick_str_concat(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    size_t la = strlen(a), lb = strlen(b);
    char* r = (char*)malloc(la + lb + 1);
    memcpy(r, a, la);
    memcpy(r + la, b, lb);
    r[la + lb] = '\0';
    return r;
}
int32_t tick_str_len(const char* s) { return s ? (int32_t)strlen(s) : 0; }
int32_t tick_str_order(const char* a, const char* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return (int32_t)strcmp(a, b);
}
bool tick_str_eq(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}
char* tick_str_from_i64(int64_t v) {
    char* b = (char*)malloc(24); snprintf(b, 24, "%lld", (long long)v); return b;
}
char* tick_str_from_u64(uint64_t v) {
    char* b = (char*)malloc(24); snprintf(b, 24, "%llu", (unsigned long long)v); return b;
}
char* tick_str_from_f64(double v) {
    char* b = (char*)malloc(40); snprintf(b, 40, "%g", v); return b;
}
char* tick_str_from_bool(bool v) {
    return tick_str_dup(v ? "true" : "false");
}
char* tick_str_dup(const char* s) {
    if (!s) s = "";
    size_t n = strlen(s);
    char* r = (char*)malloc(n + 1);
    memcpy(r, s, n + 1);
    return r;
}

/* ---- dynamic arrays ---- */

TickArray tick_array_new(int32_t elem_size) {
    TickArray a;
    a.data = NULL; a.len = 0; a.cap = 0; a.elem_size = elem_size;
    return a;
}
TickArray tick_array_copy(TickArray src) {
    TickArray a = tick_array_new(src.elem_size);
    if (src.len > 0) {
        tick_array_reserve(&a, src.len);
        memcpy(a.data, src.data, (size_t)src.len * src.elem_size);
        a.len = src.len;
    }
    return a;
}
void tick_array_reserve(TickArray* a, int32_t needed) {
    if (needed <= a->cap) return;
    int32_t cap = a->cap == 0 ? 4 : a->cap;
    while (cap < needed) cap *= 2;
    a->data = realloc(a->data, (size_t)cap * a->elem_size);
    a->cap = cap;
}
void* tick_array_at(TickArray* a, int32_t index) {
    if (index < 0 || index >= a->len) tick_trap_bounds("array index", index, a->len);
    return (char*)a->data + (size_t)index * a->elem_size;
}
void* tick_array_at_unchecked(TickArray* a, int32_t index) {
    return (char*)a->data + (size_t)index * a->elem_size;
}
void tick_array_push(TickArray* a, const void* elem) {
    tick_array_reserve(a, a->len + 1);
    memcpy((char*)a->data + (size_t)a->len * a->elem_size, elem, a->elem_size);
    a->len++;
}
void tick_array_free(TickArray* a) {
    if (a->data) { free(a->data); a->data = NULL; }
    a->len = 0; a->cap = 0;
}

/* ---- shared / weak ---- */

void* tick_shared_new(int32_t payload_size) {
    TickShared* h = (TickShared*)malloc(sizeof(TickShared) + payload_size);
    h->strong = 1;
    h->weak = 0;
    return (void*)(h + 1);
}
static TickShared* header_of(void* payload) {
    return ((TickShared*)payload) - 1;
}
void* tick_shared_retain(void* payload) {
    if (payload) header_of(payload)->strong++;
    return payload;
}
void tick_shared_release(void* payload) {
    if (!payload) return;
    TickShared* h = header_of(payload);
    h->strong--;
    if (h->strong == 0 && h->weak == 0) free(h);
}
void* tick_weak_from(void* payload) {
    if (payload) header_of(payload)->weak++;
    return payload;
}
void tick_weak_drop(void* payload) {
    if (!payload) return;
    TickShared* h = header_of(payload);
    h->weak--;
    if (h->strong == 0 && h->weak == 0) free(h);
}
bool tick_weak_alive(void* payload) {
    return payload && header_of(payload)->strong > 0;
}

/* ---- signals & events ---- */

void tick_signal_init(TickSignal* s) {
    s->head = s->tail = s->count = 0;
    pthread_mutex_init(&s->mutex, NULL);
    pthread_cond_init(&s->cond, NULL);
}
void tick_signal_emit(TickSignal* s, void* value) {
    pthread_mutex_lock(&s->mutex);
    if (s->count < TICK_SIGNAL_CAP) {
        s->data[s->tail] = value;
        s->tail = (s->tail + 1) % TICK_SIGNAL_CAP;
        s->count++;
        pthread_cond_signal(&s->cond);
    } else {
        fprintf(stderr, "tick: signal queue full, value dropped\n");
    }
    pthread_mutex_unlock(&s->mutex);
}
void* tick_signal_recv(TickSignal* s) {
    pthread_mutex_lock(&s->mutex);
    while (s->count == 0) pthread_cond_wait(&s->cond, &s->mutex);
    void* v = s->data[s->head];
    s->head = (s->head + 1) % TICK_SIGNAL_CAP;
    s->count--;
    pthread_mutex_unlock(&s->mutex);
    return v;
}
void tick_event_init(TickEvent* e, int cap) {
    e->fns = (TickProcessFn*)malloc(sizeof(TickProcessFn) * (cap > 0 ? cap : 1));
    e->threads = (pthread_t*)malloc(sizeof(pthread_t) * (cap > 0 ? cap : 1));
    e->count = 0;
    e->cap = cap;
}
void tick_event_bind(TickEvent* e, TickProcessFn fn) {
    if (e->count < e->cap) e->fns[e->count++] = fn;
}
void tick_event_fire(TickEvent* e) {
    for (int i = 0; i < e->count; i++)
        pthread_create(&e->threads[i], NULL, e->fns[i], NULL);
    for (int i = 0; i < e->count; i++)
        pthread_join(e->threads[i], NULL);
}
