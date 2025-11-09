#include "string_pool.h"

namespace Tick {

int StringPool::add(const String& str) {
    for (size_t i = 0; i < _strings.size(); i++) {
        if (_strings[i] == str) {
            return i;
        }
    }
    _strings.push(str);
    return _strings.size() - 1;
}

const char* StringPool::get(int index) {
    if (index >= 0 && (size_t)index < _strings.size()) {
        return _strings[index].c_str();
    }
    return "";
}

}
