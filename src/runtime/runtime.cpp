#include "runtime.h"
#include "interpreter.h"
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

namespace Tick {

SignalQueue::SignalQueue() : _write_pos(0), _read_pos(0) {
    pthread_mutex_init(&_mutex, nullptr);
    pthread_cond_init(&_cond, nullptr);
}

SignalQueue::~SignalQueue() {
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
}

void SignalQueue::emit(Value value) {
    pthread_mutex_lock(&_mutex);
    
    if (_write_pos >= _queue.size()) {
        _queue.push(value);
    } else {
        _queue[_write_pos] = value;
    }
    _write_pos++;
    
    pthread_cond_signal(&_cond);
    pthread_mutex_unlock(&_mutex);
}

Value SignalQueue::recv() {
    pthread_mutex_lock(&_mutex);
    
    while (_read_pos >= _write_pos) {
        pthread_cond_wait(&_cond, &_mutex);
    }
    
    Value value = _queue[_read_pos];
    _read_pos++;
    
    if (_read_pos == _write_pos) {
        _read_pos = 0;
        _write_pos = 0;
    }
    
    pthread_mutex_unlock(&_mutex);
    return value;
}

bool SignalQueue::has_value() {
    pthread_mutex_lock(&_mutex);
    bool has = _read_pos < _write_pos;
    pthread_mutex_unlock(&_mutex);
    return has;
}

ThreadPool::ThreadPool(int num_threads) 
    : _num_threads(num_threads), _task_queue_head(nullptr), 
      _task_queue_tail(nullptr), _shutdown(false), _active_tasks(0) {
    
    pthread_mutex_init(&_queue_mutex, nullptr);
    pthread_cond_init(&_queue_cond, nullptr);
    
    _threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&_threads[i], nullptr, worker_thread, this);
    }
}

ThreadPool::~ThreadPool() {
    pthread_mutex_lock(&_queue_mutex);
    _shutdown = true;
    pthread_cond_broadcast(&_queue_cond);
    pthread_mutex_unlock(&_queue_mutex);
    
    for (int i = 0; i < _num_threads; i++) {
        pthread_join(_threads[i], nullptr);
    }
    
    free(_threads);
    pthread_mutex_destroy(&_queue_mutex);
    pthread_cond_destroy(&_queue_cond);
}

void ThreadPool::submit_task(void* (*func)(void*), void* arg) {
    Task* task = new Task();
    task->function = func;
    task->argument = arg;
    task->next = nullptr;
    
    pthread_mutex_lock(&_queue_mutex);
    
    if (_task_queue_tail) {
        _task_queue_tail->next = task;
    } else {
        _task_queue_head = task;
    }
    _task_queue_tail = task;
    _active_tasks++;
    
    pthread_cond_signal(&_queue_cond);
    pthread_mutex_unlock(&_queue_mutex);
}

void ThreadPool::wait_all() {
    pthread_mutex_lock(&_queue_mutex);
    while (_active_tasks > 0) {
        pthread_mutex_unlock(&_queue_mutex);
        usleep(1000);
        pthread_mutex_lock(&_queue_mutex);
    }
    pthread_mutex_unlock(&_queue_mutex);
}

void* ThreadPool::worker_thread(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    
    while (true) {
        Task* task = pool->get_task();
        
        if (!task) {
            break;
        }
        
        task->function(task->argument);
        
        pthread_mutex_lock(&pool->_queue_mutex);
        pool->_active_tasks--;
        pthread_mutex_unlock(&pool->_queue_mutex);
        
        delete task;
    }
    
    return nullptr;
}

ThreadPool::Task* ThreadPool::get_task() {
    pthread_mutex_lock(&_queue_mutex);
    
    while (!_task_queue_head && !_shutdown) {
        pthread_cond_wait(&_queue_cond, &_queue_mutex);
    }
    
    if (_shutdown) {
        pthread_mutex_unlock(&_queue_mutex);
        return nullptr;
    }
    
    Task* task = _task_queue_head;
    _task_queue_head = task->next;
    
    if (!_task_queue_head) {
        _task_queue_tail = nullptr;
    }
    
    pthread_mutex_unlock(&_queue_mutex);
    return task;
}

EventScheduler::EventScheduler(int num_threads) {
    _thread_pool = new ThreadPool(num_threads);
}

EventScheduler::~EventScheduler() {
    delete _thread_pool;
}

void EventScheduler::register_process(const char* event_name, ProcessContext* process) {
    DynamicArray<ProcessContext*>** processes = _event_processes.find(event_name);
    
    if (!processes) {
        DynamicArray<ProcessContext*>* new_array = new DynamicArray<ProcessContext*>();
        _event_processes.insert(event_name, new_array);
        processes = _event_processes.find(event_name);
    }
    
    (*processes)->push(process);
}

void EventScheduler::execute_event(const char* event_name) {
    DynamicArray<ProcessContext*>** processes = _event_processes.find(event_name);
    
    if (!processes) {
        return;
    }
    
    for (size_t i = 0; i < (*processes)->size(); i++) {
        _thread_pool->submit_task(execute_process, (*processes)->data()[i]);
    }
}

void EventScheduler::wait_completion() {
    _thread_pool->wait_all();
}

void* EventScheduler::execute_process(void* arg) {
    ProcessContext* ctx = static_cast<ProcessContext*>(arg);
    if (ctx && ctx->bytecode && ctx->runtime && ctx->string_pool && ctx->constants) {
        Interpreter interpreter(ctx->runtime, ctx->string_pool);
        interpreter.execute(ctx->bytecode, ctx->constants);
    }
    return nullptr;
}

Runtime::Runtime() {
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores <= 0) num_cores = 4;
    _scheduler = new EventScheduler(num_cores);
    _string_pool = nullptr;
    _constants = nullptr;
}

Runtime::~Runtime() {
    _signals.for_each([](const char*, SignalQueue* queue) {
        delete queue;
    });
    _user_functions.for_each([](const char*, DynamicArray<Instruction>* code) {
        delete code;
    });
    delete _scheduler;
}

void Runtime::set_string_pool(StringPool* pool) {
    _string_pool = pool;
}

StringPool* Runtime::get_string_pool() {
    return _string_pool;
}

void Runtime::set_constants(DynamicArray<Value>* constants) {
    _constants = constants;
}

DynamicArray<Value>* Runtime::get_constants() {
    return _constants;
}

void Runtime::register_signal(const char* name) {
    if (!_signals.contains(name)) {
        _signals.insert(name, new SignalQueue());
    }
}

void Runtime::register_event(const char* name) {
    _events.insert(name, true);
}

void Runtime::register_process(const char* event_name, ProcessContext* process) {
    _scheduler->register_process(event_name, process);
}

void Runtime::register_function(const char* name, Value (*func)(DynamicArray<Value>&)) {
    _functions.insert(name, func);
}

SignalQueue* Runtime::get_signal(const char* name) {
    SignalQueue** queue = _signals.find(name);
    return queue ? *queue : nullptr;
}

void Runtime::execute_event(const char* event_name) {
    _scheduler->execute_event(event_name);
}

void Runtime::wait_event_completion() {
    _scheduler->wait_completion();
}

void Runtime::register_user_function(const char* name, DynamicArray<Instruction>* code) {
    _user_functions.insert(name, code);
}

Value Runtime::call_function(const char* name, DynamicArray<Value>& args) {
    
    Value (**func)(DynamicArray<Value>&) = _functions.find(name);
    if (func) {
        return (*func)(args);
    }
    
    DynamicArray<Instruction>** user_func = _user_functions.find(name);
    if (user_func && _string_pool && _constants) {
        Interpreter interpreter(this, _string_pool);
        return interpreter.execute_function(*user_func, _constants, args);
    }
    
    return Value();
}

void Runtime::set_global(const char* name, Value value) {
    _globals.insert(name, value);
}

Value Runtime::get_global(const char* name) {
    Value* val = _globals.find(name);
    return val ? *val : Value();
}

}
