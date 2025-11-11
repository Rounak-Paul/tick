#ifndef TICK_STRING_POOL_H
#define TICK_STRING_POOL_H

#include "../core/dynamic_array.h"
#include "../core/string.h"

namespace Tick {

class StringPool {
public:
    int add(const String& str);
    const char* get(int index);
    size_t size() const { return _strings.size(); }
    const DynamicArray<String>& get_strings() const { return _strings; }
    void load_from_array(const DynamicArray<String>& strings) {
        for (size_t i = 0; i < strings.size(); i++) {
            _strings.push(strings[i]);
        }
    }
    
private:
    DynamicArray<String> _strings;
};

}

#endif
