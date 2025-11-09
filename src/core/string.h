#ifndef TICK_STRING_H
#define TICK_STRING_H

#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace Tick {

class String {
public:
    String() : _data(nullptr), _length(0) {}
    
    String(const char* str) {
        _length = strlen(str);
        _data = (char*)malloc(_length + 1);
        memcpy(_data, str, _length + 1);
    }
    
    String(const char* str, size_t len) {
        _length = len;
        _data = (char*)malloc(_length + 1);
        memcpy(_data, str, _length);
        _data[_length] = '\0';
    }
    
    String(const String& other) {
        _length = other._length;
        _data = (char*)malloc(_length + 1);
        memcpy(_data, other._data, _length + 1);
    }
    
    ~String() {
        if (_data) {
            free(_data);
        }
    }
    
    String& operator=(const String& other) {
        if (this != &other) {
            if (_data) free(_data);
            _length = other._length;
            _data = (char*)malloc(_length + 1);
            memcpy(_data, other._data, _length + 1);
        }
        return *this;
    }
    
    bool operator==(const String& other) const {
        if (_length != other._length) return false;
        return memcmp(_data, other._data, _length) == 0;
    }
    
    bool operator==(const char* str) const {
        size_t len = strlen(str);
        if (_length != len) return false;
        return memcmp(_data, str, len) == 0;
    }
    
    const char* c_str() const { return _data ? _data : ""; }
    size_t length() const { return _length; }
    
    char operator[](size_t index) const {
        return _data[index];
    }

private:
    char* _data;
    size_t _length;
};

}

#endif
