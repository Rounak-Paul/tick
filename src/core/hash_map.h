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
    enum SlotState : unsigned char { EMPTY, OCCUPIED, TOMBSTONE };

    struct Entry {
        K key;
        V value;
        SlotState state;
        Entry() : state(EMPTY) {}
    };

    Entry* _buckets;
    size_t _capacity;
    size_t _size;
    size_t _tombstones;

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
        _tombstones = 0;

        for (size_t i = 0; i < old_capacity; i++) {
            if (old_buckets[i].state == OCCUPIED) {
                insert(old_buckets[i].key, old_buckets[i].value);
                old_buckets[i].key.~K();
                old_buckets[i].value.~V();
            }
        }

        free(old_buckets);
    }

public:
    HashMap() : _capacity(16), _size(0), _tombstones(0) {
        _buckets = (Entry*)malloc(_capacity * sizeof(Entry));
        for (size_t i = 0; i < _capacity; i++) {
            new (&_buckets[i]) Entry();
        }
    }

    ~HashMap() {
        for (size_t i = 0; i < _capacity; i++) {
            if (_buckets[i].state == OCCUPIED) {
                _buckets[i].key.~K();
                _buckets[i].value.~V();
            }
        }
        free(_buckets);
    }

    void insert(const K& key, const V& value) {
        if ((_size + _tombstones) >= _capacity * 7 / 10) {
            rehash();
        }

        size_t index = hash(key);
        size_t first_tombstone = _capacity;

        for (size_t i = 0; i < _capacity; i++) {
            size_t idx = (index + i) % _capacity;
            if (_buckets[idx].state == OCCUPIED && _buckets[idx].key == key) {
                _buckets[idx].value = value;
                return;
            }
            if (_buckets[idx].state == TOMBSTONE && first_tombstone == _capacity) {
                first_tombstone = idx;
            }
            if (_buckets[idx].state == EMPTY) {
                size_t target = (first_tombstone < _capacity) ? first_tombstone : idx;
                if (target == first_tombstone) _tombstones--;
                new (&_buckets[target].key) K(key);
                new (&_buckets[target].value) V(value);
                _buckets[target].state = OCCUPIED;
                _size++;
                return;
            }
        }

        if (first_tombstone < _capacity) {
            _tombstones--;
            new (&_buckets[first_tombstone].key) K(key);
            new (&_buckets[first_tombstone].value) V(value);
            _buckets[first_tombstone].state = OCCUPIED;
            _size++;
        }
    }

    V* find(const K& key) {
        size_t index = hash(key);
        for (size_t i = 0; i < _capacity; i++) {
            size_t idx = (index + i) % _capacity;
            if (_buckets[idx].state == EMPTY) return nullptr;
            if (_buckets[idx].state == OCCUPIED && _buckets[idx].key == key) {
                return &_buckets[idx].value;
            }
        }
        return nullptr;
    }

    bool contains(const K& key) const {
        size_t index = hash(key);
        for (size_t i = 0; i < _capacity; i++) {
            size_t idx = (index + i) % _capacity;
            if (_buckets[idx].state == EMPTY) return false;
            if (_buckets[idx].state == OCCUPIED && _buckets[idx].key == key) {
                return true;
            }
        }
        return false;
    }

    void remove(const K& key) {
        size_t index = hash(key);
        for (size_t i = 0; i < _capacity; i++) {
            size_t idx = (index + i) % _capacity;
            if (_buckets[idx].state == EMPTY) return;
            if (_buckets[idx].state == OCCUPIED && _buckets[idx].key == key) {
                _buckets[idx].key.~K();
                _buckets[idx].value.~V();
                _buckets[idx].state = TOMBSTONE;
                _size--;
                _tombstones++;
                return;
            }
        }
    }

    size_t size() const { return _size; }

    template<typename Func>
    void for_each(Func func) const {
        for (size_t i = 0; i < _capacity; i++) {
            if (_buckets[i].state == OCCUPIED) {
                func(_buckets[i].key, _buckets[i].value);
            }
        }
    }
};

template<typename V>
class HashMap<const char*, V> {
private:
    enum SlotState : unsigned char { EMPTY, OCCUPIED, TOMBSTONE };

    struct Entry {
        const char* key;
        V value;
        SlotState state;
        Entry() : key(nullptr), state(EMPTY) {}
    };

    Entry* _buckets;
    size_t _capacity;
    size_t _size;
    size_t _tombstones;

    size_t hash(const char* key) const {
        size_t h = 0;
        while (*key) {
            h = h * 31 + (unsigned char)*key++;
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
        _tombstones = 0;

        for (size_t i = 0; i < old_capacity; i++) {
            if (old_buckets[i].state == OCCUPIED) {
                insert(old_buckets[i].key, old_buckets[i].value);
            }
        }

        free(old_buckets);
    }

public:
    HashMap() : _capacity(16), _size(0), _tombstones(0) {
        _buckets = (Entry*)malloc(_capacity * sizeof(Entry));
        for (size_t i = 0; i < _capacity; i++) {
            new (&_buckets[i]) Entry();
        }
    }

    ~HashMap() {
        free(_buckets);
    }

    void insert(const char* key, const V& value) {
        if ((_size + _tombstones) >= _capacity * 7 / 10) {
            rehash();
        }

        size_t index = hash(key);
        size_t first_tombstone = _capacity;

        for (size_t i = 0; i < _capacity; i++) {
            size_t idx = (index + i) % _capacity;
            if (_buckets[idx].state == OCCUPIED && strcmp(_buckets[idx].key, key) == 0) {
                _buckets[idx].value = value;
                return;
            }
            if (_buckets[idx].state == TOMBSTONE && first_tombstone == _capacity) {
                first_tombstone = idx;
            }
            if (_buckets[idx].state == EMPTY) {
                size_t target = (first_tombstone < _capacity) ? first_tombstone : idx;
                if (target == first_tombstone) _tombstones--;
                _buckets[target].key = key;
                _buckets[target].value = value;
                _buckets[target].state = OCCUPIED;
                _size++;
                return;
            }
        }

        if (first_tombstone < _capacity) {
            _tombstones--;
            _buckets[first_tombstone].key = key;
            _buckets[first_tombstone].value = value;
            _buckets[first_tombstone].state = OCCUPIED;
            _size++;
        }
    }

    V* find(const char* key) {
        size_t index = hash(key);
        for (size_t i = 0; i < _capacity; i++) {
            size_t idx = (index + i) % _capacity;
            if (_buckets[idx].state == EMPTY) return nullptr;
            if (_buckets[idx].state == OCCUPIED && strcmp(_buckets[idx].key, key) == 0) {
                return &_buckets[idx].value;
            }
        }
        return nullptr;
    }

    bool contains(const char* key) const {
        size_t index = hash(key);
        for (size_t i = 0; i < _capacity; i++) {
            size_t idx = (index + i) % _capacity;
            if (_buckets[idx].state == EMPTY) return false;
            if (_buckets[idx].state == OCCUPIED && strcmp(_buckets[idx].key, key) == 0) {
                return true;
            }
        }
        return false;
    }

    void remove(const char* key) {
        size_t index = hash(key);
        for (size_t i = 0; i < _capacity; i++) {
            size_t idx = (index + i) % _capacity;
            if (_buckets[idx].state == EMPTY) return;
            if (_buckets[idx].state == OCCUPIED && strcmp(_buckets[idx].key, key) == 0) {
                _buckets[idx].state = TOMBSTONE;
                _size--;
                _tombstones++;
                return;
            }
        }
    }

    size_t size() const { return _size; }

    template<typename Func>
    void for_each(Func func) const {
        for (size_t i = 0; i < _capacity; i++) {
            if (_buckets[i].state == OCCUPIED) {
                func(_buckets[i].key, _buckets[i].value);
            }
        }
    }
};

}

#endif
