#ifndef TICK_STRING_POOL_H
#define TICK_STRING_POOL_H

#include "../core/dynamic_array.h"
#include "../core/string.h"

namespace Tick {

class StringPool {
public:
    int add(const String& str);
    const char* get(int index);
    
private:
    DynamicArray<String> _strings;
};

}

#endif
