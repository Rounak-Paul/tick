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
        if (str) {
            _length = strlen(str);
            _data = (char*)malloc(_length + 1);
            memcpy(_data, str, _length + 1);
        } else {
            _data = nullptr;
            _length = 0;
        }
    }

    String(const char* str, size_t len) {
        _length = len;
        _data = (char*)malloc(_length + 1);
        memcpy(_data, str, _length);
        _data[_length] = '\0';
    }

    String(const String& other) {
        _length = other._length;
        if (other._data) {
            _data = (char*)malloc(_length + 1);
            memcpy(_data, other._data, _length + 1);
        } else {
            _data = nullptr;
        }
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
            if (other._data) {
                _data = (char*)malloc(_length + 1);
                memcpy(_data, other._data, _length + 1);
            } else {
                _data = nullptr;
            }
        }
        return *this;
    }

    bool operator==(const String& other) const {
        if (_length != other._length) return false;
        if (_data == nullptr && other._data == nullptr) return true;
        if (_data == nullptr || other._data == nullptr) return false;
        return memcmp(_data, other._data, _length) == 0;
    }

    bool operator==(const char* str) const {
        if (!str) return _data == nullptr;
        size_t len = strlen(str);
        if (_length != len) return false;
        if (!_data) return false;
        return memcmp(_data, str, len) == 0;
    }

    bool operator!=(const String& other) const {
        return !(*this == other);
    }

    bool operator!=(const char* str) const {
        return !(*this == str);
    }

    String operator+(const String& other) const {
        size_t new_len = _length + other._length;
        char* buf = (char*)malloc(new_len + 1);
        if (_data) memcpy(buf, _data, _length);
        if (other._data) memcpy(buf + _length, other._data, other._length);
        buf[new_len] = '\0';
        String result(buf, new_len);
        free(buf);
        return result;
    }

    const char* c_str() const { return _data ? _data : ""; }
    size_t length() const { return _length; }
    bool empty() const { return _length == 0; }

    char operator[](size_t index) const {
        return _data[index];
    }

private:
    char* _data;
    size_t _length;
};

}

#endif
