#ifndef TICK_HASH_MAP_H
#define TICK_HASH_MAP_H

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>

namespace Tick {

template<typename K, typename V>
class HashMap {
private:
    struct Entry {
        K key;
        V value;
        bool occupied;
        Entry() : occupied(false) {}
    };
    
    Entry* _buckets;
    size_t _capacity;
    size_t _size;
    
    bool keys_equal(const K& a, const K& b) const {
        return a == b;
    }
    
    size_t hash(const K& key) const {
        size_t k = 0;
        const unsigned char* p = (const unsigned char*)&key;
        for (size_t i = 0; i < sizeof(K); i++) {
            k = k * 31 + p[i];
        }
        return k % _capacity;
    }
    
    void rehash() {
        size_t old_capacity = _capacity;
        Entry* old_buckets = _buckets;
        
        _capacity *= 2;
        _buckets = (Entry*)malloc(_capacity * sizeof(Entry));
        for (size_t i = 0; i < _capacity; i++) {
            new (&_buckets[i]) Entry();
        }
        _size = 0;
        
        for (size_t i = 0; i < old_capacity; i++) {
            if (old_buckets[i].occupied) {
                insert(old_buckets[i].key, old_buckets[i].value);
            }
        }
        
        free(old_buckets);
    }

public:
    HashMap() : _capacity(16), _size(0) {
        _buckets = (Entry*)malloc(_capacity * sizeof(Entry));
        for (size_t i = 0; i < _capacity; i++) {
            new (&_buckets[i]) Entry();
        }
    }
    
    ~HashMap() {
        if (_buckets) {
            free(_buckets);
        }
    }
    
    void insert(const K& key, const V& value) {
        if (_size >= _capacity * 0.7) {
            rehash();
        }
        
        size_t index = hash(key);
        while (_buckets[index].occupied && !keys_equal(_buckets[index].key, key)) {
            index = (index + 1) % _capacity;
        }
        
        if (!_buckets[index].occupied) {
            _size++;
        }
        _buckets[index].key = key;
        _buckets[index].value = value;
        _buckets[index].occupied = true;
    }
    
    V* find(const K& key) {
        size_t index = hash(key);
        size_t start = index;
        
        while (_buckets[index].occupied) {
            if (keys_equal(_buckets[index].key, key)) {
                return &_buckets[index].value;
            }
            index = (index + 1) % _capacity;
            if (index == start) break;
        }
        return nullptr;
    }
    
    bool contains(const K& key) const {
        size_t index = hash(key);
        size_t start = index;
        
        while (_buckets[index].occupied) {
            if (_buckets[index].key == key) {
                return true;
            }
            index = (index + 1) % _capacity;
            if (index == start) break;
        }
        return false;
    }
    
    void remove(const K& key) {
        size_t index = hash(key);
        size_t start = index;
        
        while (_buckets[index].occupied) {
            if (keys_equal(_buckets[index].key, key)) {
                _buckets[index].occupied = false;
                _size--;
                return;
            }
            index = (index + 1) % _capacity;
            if (index == start) break;
        }
    }
    
    size_t size() const { return _size; }
    
    template<typename Func>
    void for_each(Func func) const {
        for (size_t i = 0; i < _capacity; i++) {
            if (_buckets[i].occupied) {
                func(_buckets[i].key, _buckets[i].value);
            }
        }
    }
};

template<typename V>
class HashMap<const char*, V> {
private:
    struct Entry {
        const char* key;
        V value;
        bool occupied;
        Entry() : key(nullptr), occupied(false) {}
    };
    
    Entry* _buckets;
    size_t _capacity;
    size_t _size;
    
    bool keys_equal(const char* a, const char* b) const {
        return strcmp(a, b) == 0;
    }
    
    size_t hash(const char* key) const {
        size_t h = 0;
        while (*key) {
            h = h * 31 + *key++;
        }
        return h % _capacity;
    }
    
    void rehash() {
        size_t old_capacity = _capacity;
        Entry* old_buckets = _buckets;
        
        _capacity *= 2;
        _buckets = (Entry*)malloc(_capacity * sizeof(Entry));
        for (size_t i = 0; i < _capacity; i++) {
            new (&_buckets[i]) Entry();
        }
        _size = 0;
        
        for (size_t i = 0; i < old_capacity; i++) {
            if (old_buckets[i].occupied) {
                insert(old_buckets[i].key, old_buckets[i].value);
            }
        }
        
        free(old_buckets);
    }

public:
    HashMap() : _capacity(16), _size(0) {
        _buckets = (Entry*)malloc(_capacity * sizeof(Entry));
        for (size_t i = 0; i < _capacity; i++) {
            new (&_buckets[i]) Entry();
        }
    }
    
    ~HashMap() {
        if (_buckets) {
            free(_buckets);
        }
    }
    
    void insert(const char* key, const V& value) {
        if (_size >= _capacity * 0.7) {
            rehash();
        }
        
        size_t index = hash(key);
        while (_buckets[index].occupied && !keys_equal(_buckets[index].key, key)) {
            index = (index + 1) % _capacity;
        }
        
        if (!_buckets[index].occupied) {
            _size++;
        }
        _buckets[index].key = key;
        _buckets[index].value = value;
        _buckets[index].occupied = true;
    }
    
    V* find(const char* key) {
        size_t index = hash(key);
        size_t start = index;
        
        while (_buckets[index].occupied) {
            if (keys_equal(_buckets[index].key, key)) {
                return &_buckets[index].value;
            }
            index = (index + 1) % _capacity;
            if (index == start) break;
        }
        return nullptr;
    }
    
    bool contains(const char* key) const {
        size_t index = hash(key);
        size_t start = index;
        
        while (_buckets[index].occupied) {
            if (keys_equal(_buckets[index].key, key)) {
                return true;
            }
            index = (index + 1) % _capacity;
            if (index == start) break;
        }
        return false;
    }
    
    void remove(const char* key) {
        size_t index = hash(key);
        size_t start = index;
        
        while (_buckets[index].occupied) {
            if (keys_equal(_buckets[index].key, key)) {
                _buckets[index].occupied = false;
                _size--;
                return;
            }
            index = (index + 1) % _capacity;
            if (index == start) break;
        }
    }
    
    size_t size() const { return _size; }
    
    template<typename Func>
    void for_each(Func func) const {
        for (size_t i = 0; i < _capacity; i++) {
            if (_buckets[i].occupied) {
                func(_buckets[i].key, _buckets[i].value);
            }
        }
    }
};

}

#endif
