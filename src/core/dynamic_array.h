#ifndef TICK_DYNAMIC_ARRAY_H
#define TICK_DYNAMIC_ARRAY_H

#include <cstddef>
#include <cstdlib>
#include <new>

namespace Tick {

template<typename T>
class DynamicArray {
public:
    DynamicArray() : _data(nullptr), _size(0), _capacity(0) {}

    DynamicArray(const DynamicArray& other) : _data(nullptr), _size(0), _capacity(0) {
        if (other._size > 0) {
            _capacity = other._size;
            _data = (T*)malloc(_capacity * sizeof(T));
            for (size_t i = 0; i < other._size; i++) {
                new (&_data[i]) T(other._data[i]);
            }
            _size = other._size;
        }
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            destroy_elements();
            if (_data) free(_data);
            _data = nullptr;
            _size = 0;
            _capacity = 0;
            if (other._size > 0) {
                _capacity = other._size;
                _data = (T*)malloc(_capacity * sizeof(T));
                for (size_t i = 0; i < other._size; i++) {
                    new (&_data[i]) T(other._data[i]);
                }
                _size = other._size;
            }
        }
        return *this;
    }

    ~DynamicArray() {
        destroy_elements();
        if (_data) free(_data);
    }

    void push(const T& value) {
        if (_size >= _capacity) {
            size_t new_capacity = _capacity == 0 ? 8 : _capacity * 2;
            grow(new_capacity);
        }
        new (&_data[_size]) T(value);
        _size++;
    }

    void pop_back() {
        if (_size > 0) {
            _size--;
            _data[_size].~T();
        }
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

    void clear() {
        destroy_elements();
        _size = 0;
    }

    T* begin() { return _data; }
    T* end() { return _data + _size; }
    const T* begin() const { return _data; }
    const T* end() const { return _data + _size; }

private:
    T* _data;
    size_t _size;
    size_t _capacity;

    void destroy_elements() {
        for (size_t i = 0; i < _size; i++) {
            _data[i].~T();
        }
    }

    void grow(size_t new_capacity) {
        if (new_capacity <= _capacity) return;
        T* new_data = (T*)malloc(new_capacity * sizeof(T));
        for (size_t i = 0; i < _size; i++) {
            new (&new_data[i]) T(_data[i]);
            _data[i].~T();
        }
        if (_data) free(_data);
        _data = new_data;
        _capacity = new_capacity;
    }
};

}

#endif
