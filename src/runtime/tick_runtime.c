#include "tick_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

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
    } else {
        fprintf(stderr, "tick: signal queue full, value dropped\n");
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

int32_t tick_str_length(const char* s) {
    if (!s) return 0;
    return (int32_t)strlen(s);
}

char* tick_str_concat(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    size_t la = strlen(a);
    size_t lb = strlen(b);
    char* result = (char*)tick_gc_alloc(la + lb + 1);
    memcpy(result, a, la);
    memcpy(result + la, b, lb);
    result[la + lb] = '\0';
    return result;
}

char* tick_str_substring(const char* s, int32_t start, int32_t end) {
    if (!s) { char* r = (char*)tick_gc_alloc(1); r[0] = '\0'; return r; }
    int32_t len = (int32_t)strlen(s);
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end) { char* r = (char*)tick_gc_alloc(1); r[0] = '\0'; return r; }
    int32_t sub_len = end - start;
    char* result = (char*)tick_gc_alloc(sub_len + 1);
    memcpy(result, s + start, sub_len);
    result[sub_len] = '\0';
    return result;
}

int32_t tick_str_index_of(const char* s, const char* substr) {
    if (!s || !substr) return -1;
    const char* found = strstr(s, substr);
    if (!found) return -1;
    return (int32_t)(found - s);
}

int32_t tick_str_compare(const char* a, const char* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return (int32_t)strcmp(a, b);
}

char tick_str_char_at(const char* s, int32_t index) {
    if (!s || index < 0 || index >= (int32_t)strlen(s)) return '\0';
    return s[index];
}

char* tick_str_from_i64(int64_t val) {
    char* buf = (char*)tick_gc_alloc(32);
    snprintf(buf, 32, "%lld", (long long)val);
    return buf;
}

int64_t tick_str_to_i64(const char* s) {
    if (!s) return 0;
    return (int64_t)atoll(s);
}

char* tick_str_from_u64(uint64_t val) {
    char* buf = (char*)tick_gc_alloc(32);
    snprintf(buf, 32, "%llu", (unsigned long long)val);
    return buf;
}

char* tick_str_from_f64(double val) {
    char* buf = (char*)tick_gc_alloc(64);
    snprintf(buf, 64, "%f", val);
    return buf;
}

double tick_str_to_f64(const char* s) {
    if (!s) return 0.0;
    return atof(s);
}

char* tick_str_from_b8(bool val) {
    char* buf = (char*)tick_gc_alloc(6);
    snprintf(buf, 6, "%s", val ? "true" : "false");
    return buf;
}

bool tick_str_to_b8(const char* s) {
    if (!s) return false;
    return strcmp(s, "true") == 0 || strcmp(s, "1") == 0;
}

char* tick_input_readline(const char* prompt) {
    if (prompt && prompt[0] != '\0') {
        fputs(prompt, stdout);
        fflush(stdout);
    }
    size_t cap = 128;
    size_t len = 0;
    char* buf = (char*)tick_gc_alloc(cap);
    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n') {
        if (len + 1 >= cap) {
            cap *= 2;
            buf = (char*)tick_gc_realloc(buf, cap);
        }
        buf[len++] = (char)c;
    }
    buf[len] = '\0';
    return buf;
}

TickFile* tick_file_open(const char* path, const char* mode) {
    if (!path || !mode) return NULL;
    FILE* h = fopen(path, mode);
    if (!h) return NULL;
    TickFile* f = (TickFile*)malloc(sizeof(TickFile));
    f->handle = h;
    return f;
}

char* tick_file_read(TickFile* f) {
    if (!f || !f->handle) { char* r = (char*)tick_gc_alloc(1); r[0] = '\0'; return r; }
    fseek(f->handle, 0, SEEK_END);
    long size = ftell(f->handle);
    fseek(f->handle, 0, SEEK_SET);
    char* buf = (char*)tick_gc_alloc(size + 1);
    fread(buf, 1, size, f->handle);
    buf[size] = '\0';
    return buf;
}

void tick_file_write(TickFile* f, const char* data) {
    if (!f || !f->handle || !data) return;
    fputs(data, f->handle);
}

void tick_file_close(TickFile* f) {
    if (!f) return;
    if (f->handle) fclose(f->handle);
    free(f);
}

bool tick_file_exists(const char* path) {
    if (!path) return false;
    struct stat st;
    return stat(path, &st) == 0;
}

void tick_array_push(TickArray* arr, size_t elem_size) {
    if (arr->len >= arr->cap) {
        int32_t new_cap = (arr->cap == 0) ? 4 : (arr->cap * 2);
        arr->ptr = tick_gc_realloc(arr->ptr, (size_t)new_cap * elem_size);
        arr->cap = new_cap;
    }
    arr->len++;
}

void tick_array_pop(TickArray* arr) {
    if (arr->len > 0) arr->len--;
}

static TickGC _tick_gc = {{NULL}, 0};

void tick_gc_init(void) {
    _tick_gc.count = 0;
}

void* tick_gc_realloc(void* old_ptr, size_t new_size) {
    void* new_ptr = realloc(old_ptr, new_size);
    if (!new_ptr) return NULL;
    if (new_ptr == old_ptr) return new_ptr;
    for (int i = 0; i < _tick_gc.count; i++) {
        if (_tick_gc.ptrs[i] == old_ptr) {
            _tick_gc.ptrs[i] = new_ptr;
            return new_ptr;
        }
    }
    if (_tick_gc.count < TICK_GC_MAX_OBJECTS) {
        _tick_gc.ptrs[_tick_gc.count++] = new_ptr;
    }
    return new_ptr;
}

void* tick_gc_alloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr && _tick_gc.count < TICK_GC_MAX_OBJECTS) {
        _tick_gc.ptrs[_tick_gc.count++] = ptr;
    }
    return ptr;
}

void tick_gc_free(void* ptr) {
    if (!ptr) return;
    for (int i = 0; i < _tick_gc.count; i++) {
        if (_tick_gc.ptrs[i] == ptr) {
            _tick_gc.ptrs[i] = _tick_gc.ptrs[_tick_gc.count - 1];
            _tick_gc.count--;
            free(ptr);
            return;
        }
    }
}

void tick_gc_collect(void) {
    tick_gc_cleanup();
}

void tick_gc_cleanup(void) {
    for (int i = 0; i < _tick_gc.count; i++) {
        if (_tick_gc.ptrs[i]) {
            free(_tick_gc.ptrs[i]);
            _tick_gc.ptrs[i] = NULL;
        }
    }
    _tick_gc.count = 0;
}
