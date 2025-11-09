#ifndef TICK_DYNAMIC_ARRAY_H
#define TICK_DYNAMIC_ARRAY_H

#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace Tick {

template<typename T>
class DynamicArray {
public:
    DynamicArray() : _data(nullptr), _size(0), _capacity(0) {}
    
    ~DynamicArray() {
        if (_data) {
            free(_data);
        }
    }
    
    void push(const T& value) {
        if (_size >= _capacity) {
            size_t new_capacity = _capacity == 0 ? 8 : _capacity * 2;
            reserve(new_capacity);
        }
        _data[_size++] = value;
    }
    
    void pop_back() {
        if (_size > 0) {
            _size--;
        }
    }
    
    void reserve(size_t new_capacity) {
        if (new_capacity <= _capacity) return;
        
        T* new_data = (T*)malloc(new_capacity * sizeof(T));
        if (_data) {
            memcpy(new_data, _data, _size * sizeof(T));
            free(_data);
        }
        _data = new_data;
        _capacity = new_capacity;
    }
    
    T& operator[](size_t index) {
        return _data[index];
    }
    
    const T& operator[](size_t index) const {
        return _data[index];
    }
    
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    
    T* data() { return _data; }
    const T* data() const { return _data; }
    
    void clear() { _size = 0; }
    
    T* begin() { return _data; }
    T* end() { return _data + _size; }
    const T* begin() const { return _data; }
    const T* end() const { return _data + _size; }

private:
    T* _data;
    size_t _size;
    size_t _capacity;
};

}

#endif
