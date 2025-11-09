#ifndef TICK_RUNTIME_H
#define TICK_RUNTIME_H

#include "../core/dynamic_array.h"
#include "../core/hash_map.h"
#include "../core/string.h"
#include <pthread.h>

namespace Tick {

class StringPool;

struct Value {
    enum Type {
        INT,
        BOOL,
        STRING,
        NONE
    } type;
    
    union {
        int int_val;
        bool bool_val;
        int string_id;
    };
    
    Value() : type(NONE), int_val(0) {}
    Value(int v) : type(INT), int_val(v) {}
    Value(bool v) : type(BOOL), bool_val(v) {}
    Value(int sid, bool is_string) : type(STRING), string_id(sid) {}
};

class SignalQueue {
public:
    SignalQueue();
    ~SignalQueue();
    
    void emit(Value value);
    Value recv();
    bool has_value();

private:
    DynamicArray<Value> _queue;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    size_t _read_pos;
};

struct ProcessContext {
    void* bytecode;
    size_t bytecode_size;
    HashMap<const char*, Value>* local_vars;
    
    ProcessContext() : bytecode(nullptr), bytecode_size(0), local_vars(nullptr) {}
};

class ThreadPool {
public:
    ThreadPool(int num_threads);
    ~ThreadPool();
    
    void submit_task(void* (*func)(void*), void* arg);
    void wait_all();

private:
    struct Task {
        void* (*function)(void*);
        void* argument;
        Task* next;
        
        Task() : function(nullptr), argument(nullptr), next(nullptr) {}
    };
    
    pthread_t* _threads;
    int _num_threads;
    Task* _task_queue_head;
    Task* _task_queue_tail;
    pthread_mutex_t _queue_mutex;
    pthread_cond_t _queue_cond;
    bool _shutdown;
    int _active_tasks;
    
    static void* worker_thread(void* arg);
    Task* get_task();
};

class EventScheduler {
public:
    EventScheduler(int num_threads);
    ~EventScheduler();
    
    void register_process(const char* event_name, ProcessContext* process);
    void execute_event(const char* event_name);
    void wait_completion();

private:
    ThreadPool* _thread_pool;
    HashMap<const char*, DynamicArray<ProcessContext*>*> _event_processes;
    
    static void* execute_process(void* arg);
};

class Runtime {
public:
    Runtime();
    ~Runtime();
    
    void register_signal(const char* name);
    void register_event(const char* name);
    void register_process(const char* event_name, ProcessContext* process);
    void register_function(const char* name, Value (*func)(DynamicArray<Value>&));
    
    SignalQueue* get_signal(const char* name);
    void execute_event(const char* event_name);
    void wait_event_completion();
    
    Value call_function(const char* name, DynamicArray<Value>& args);
    
    void set_global(const char* name, Value value);
    Value get_global(const char* name);
    
    void set_string_pool(StringPool* pool);
    StringPool* get_string_pool();

private:
    HashMap<const char*, SignalQueue*> _signals;
    HashMap<const char*, bool> _events;
    HashMap<const char*, Value (*)(DynamicArray<Value>&)> _functions;
    HashMap<const char*, Value> _globals;
    EventScheduler* _scheduler;
    StringPool* _string_pool;
};

}

#endif
