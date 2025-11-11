#pragma once

#include "../core/string.h"
#include "../core/dynamic_array.h"
#include "../core/hash_map.h"
#include "bytecode.h"
#include "runtime.h"
#include <sys/stat.h>
#include <cstdio>
#include <cstring>

namespace Tick {

struct CacheHeader {
    uint32_t magic;
    uint32_t version;
    uint64_t source_mtime;
    uint64_t source_size;
    uint32_t num_functions;
    uint32_t num_processes;
    uint32_t num_events;
    uint32_t num_signals;
    uint32_t num_classes;
    uint32_t string_pool_size;
    uint32_t constants_size;
};

class BytecodeCache {
private:
    static constexpr uint32_t CACHE_MAGIC = 0x5449434B;
    static constexpr uint32_t CACHE_VERSION = 1;
    
    static bool get_file_stats(const char* path, uint64_t& mtime, uint64_t& size) {
        struct stat st;
        if (stat(path, &st) != 0) {
            return false;
        }
        mtime = static_cast<uint64_t>(st.st_mtime);
        size = static_cast<uint64_t>(st.st_size);
        return true;
    }
    
    static String get_cache_path(const char* source_path) {
        const char* last_slash = nullptr;
        const char* ptr = source_path;
        while (*ptr) {
            if (*ptr == '/') last_slash = ptr;
            ptr++;
        }
        
        String dir;
        String filename;
        
        if (last_slash) {
            size_t dir_len = last_slash - source_path;
            char* dir_buf = new char[dir_len + 1];
            memcpy(dir_buf, source_path, dir_len);
            dir_buf[dir_len] = '\0';
            dir = String(dir_buf);
            delete[] dir_buf;
            
            filename = String(last_slash + 1);
        } else {
            dir = String(".");
            filename = String(source_path);
        }
        
        const char* dot = nullptr;
        ptr = filename.c_str();
        while (*ptr) {
            if (*ptr == '.') dot = ptr;
            ptr++;
        }
        
        String basename;
        if (dot) {
            size_t base_len = dot - filename.c_str();
            char* base_buf = new char[base_len + 1];
            memcpy(base_buf, filename.c_str(), base_len);
            base_buf[base_len] = '\0';
            basename = String(base_buf);
            delete[] base_buf;
        } else {
            basename = filename;
        }
        
        char cache_path[1024];
        snprintf(cache_path, sizeof(cache_path), "%s/.tickcache/%s.tickc", 
                 dir.c_str(), basename.c_str());
        return String(cache_path);
    }
    
    static bool ensure_cache_dir(const char* cache_path) {
        const char* last_slash = nullptr;
        const char* ptr = cache_path;
        while (*ptr) {
            if (*ptr == '/') last_slash = ptr;
            ptr++;
        }
        
        if (!last_slash) return true;
        
        size_t dir_len = last_slash - cache_path;
        char* dir_buf = new char[dir_len + 1];
        memcpy(dir_buf, cache_path, dir_len);
        dir_buf[dir_len] = '\0';
        
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", dir_buf);
        int result = system(cmd);
        delete[] dir_buf;
        
        return result == 0;
    }

public:
    static bool write_cache(const char* source_path,
                           const DynamicArray<Instruction>* main_code,
                           const HashMap<String, DynamicArray<Instruction>*>& function_codes,
                           const HashMap<String, DynamicArray<Instruction>*>& process_codes,
                           const DynamicArray<String>& events,
                           const DynamicArray<String>& signals,
                           const DynamicArray<String>& classes,
                           const DynamicArray<String>& string_pool,
                           const DynamicArray<Value>& constants) {
        
        uint64_t mtime, size;
        if (!get_file_stats(source_path, mtime, size)) {
            return false;
        }
        
        String cache_path = get_cache_path(source_path);
        if (!ensure_cache_dir(cache_path.c_str())) {
            return false;
        }
        
        FILE* f = fopen(cache_path.c_str(), "wb");
        if (!f) return false;
        
        CacheHeader header;
        header.magic = CACHE_MAGIC;
        header.version = CACHE_VERSION;
        header.source_mtime = mtime;
        header.source_size = size;
        header.num_functions = function_codes.size();
        header.num_processes = process_codes.size();
        header.num_events = events.size();
        header.num_signals = signals.size();
        header.num_classes = classes.size();
        header.string_pool_size = string_pool.size();
        header.constants_size = constants.size();
        
        fwrite(&header, sizeof(CacheHeader), 1, f);
        
        if (main_code) {
            uint32_t main_size = main_code->size();
            fwrite(&main_size, sizeof(uint32_t), 1, f);
            fwrite(main_code->data(), sizeof(Instruction), main_size, f);
        } else {
            uint32_t main_size = 0;
            fwrite(&main_size, sizeof(uint32_t), 1, f);
        }
        
        function_codes.for_each([&](const String& name, DynamicArray<Instruction>* code) {
            uint32_t name_len = strlen(name.c_str());
            fwrite(&name_len, sizeof(uint32_t), 1, f);
            fwrite(name.c_str(), 1, name_len, f);
            
            uint32_t code_size = code->size();
            fwrite(&code_size, sizeof(uint32_t), 1, f);
            fwrite(code->data(), sizeof(Instruction), code_size, f);
        });
        
        process_codes.for_each([&](const String& name, DynamicArray<Instruction>* code) {
            uint32_t name_len = strlen(name.c_str());
            fwrite(&name_len, sizeof(uint32_t), 1, f);
            fwrite(name.c_str(), 1, name_len, f);
            
            uint32_t code_size = code->size();
            fwrite(&code_size, sizeof(uint32_t), 1, f);
            fwrite(code->data(), sizeof(Instruction), code_size, f);
        });
        
        for (size_t i = 0; i < events.size(); i++) {
            uint32_t len = strlen(events[i].c_str());
            fwrite(&len, sizeof(uint32_t), 1, f);
            fwrite(events[i].c_str(), 1, len, f);
        }
        
        for (size_t i = 0; i < signals.size(); i++) {
            uint32_t len = strlen(signals[i].c_str());
            fwrite(&len, sizeof(uint32_t), 1, f);
            fwrite(signals[i].c_str(), 1, len, f);
        }
        
        for (size_t i = 0; i < classes.size(); i++) {
            uint32_t len = strlen(classes[i].c_str());
            fwrite(&len, sizeof(uint32_t), 1, f);
            fwrite(classes[i].c_str(), 1, len, f);
        }
        
        for (size_t i = 0; i < string_pool.size(); i++) {
            uint32_t len = strlen(string_pool[i].c_str());
            fwrite(&len, sizeof(uint32_t), 1, f);
            fwrite(string_pool[i].c_str(), 1, len, f);
        }
        
        for (size_t i = 0; i < constants.size(); i++) {
            fwrite(&constants[i], sizeof(Value), 1, f);
        }
        
        fclose(f);
        return true;
    }
    
    static bool is_cache_valid(const char* source_path) {
        uint64_t source_mtime, source_size;
        if (!get_file_stats(source_path, source_mtime, source_size)) {
            return false;
        }
        
        String cache_path = get_cache_path(source_path);
        FILE* f = fopen(cache_path.c_str(), "rb");
        if (!f) return false;
        
        CacheHeader header;
        size_t read = fread(&header, sizeof(CacheHeader), 1, f);
        fclose(f);
        
        if (read != 1) return false;
        if (header.magic != CACHE_MAGIC) return false;
        if (header.version != CACHE_VERSION) return false;
        if (header.source_mtime != source_mtime) return false;
        if (header.source_size != source_size) return false;
        
        return true;
    }
    
    static bool read_cache(const char* source_path,
                          DynamicArray<Instruction>** main_code,
                          HashMap<String, DynamicArray<Instruction>*>& function_codes,
                          HashMap<String, DynamicArray<Instruction>*>& process_codes,
                          DynamicArray<String>& events,
                          DynamicArray<String>& signals,
                          DynamicArray<String>& classes,
                          DynamicArray<String>& string_pool,
                          DynamicArray<Value>& constants) {
        
        if (!is_cache_valid(source_path)) {
            return false;
        }
        
        String cache_path = get_cache_path(source_path);
        FILE* f = fopen(cache_path.c_str(), "rb");
        if (!f) return false;
        
        CacheHeader header;
        if (fread(&header, sizeof(CacheHeader), 1, f) != 1) {
            fclose(f);
            return false;
        }
        
        uint32_t main_size;
        if (fread(&main_size, sizeof(uint32_t), 1, f) != 1) {
            fclose(f);
            return false;
        }
        
        if (main_size > 0) {
            *main_code = new DynamicArray<Instruction>();
            (*main_code)->reserve(main_size);
            for (uint32_t i = 0; i < main_size; i++) {
                Instruction inst;
                if (fread(&inst, sizeof(Instruction), 1, f) != 1) {
                    fclose(f);
                    return false;
                }
                (*main_code)->push(inst);
            }
        } else {
            *main_code = nullptr;
        }
        
        for (uint32_t i = 0; i < header.num_functions; i++) {
            uint32_t name_len;
            if (fread(&name_len, sizeof(uint32_t), 1, f) != 1) {
                fclose(f);
                return false;
            }
            
            char* name_buf = new char[name_len + 1];
            if (fread(name_buf, 1, name_len, f) != name_len) {
                delete[] name_buf;
                fclose(f);
                return false;
            }
            name_buf[name_len] = '\0';
            String name(name_buf);
            delete[] name_buf;
            
            uint32_t code_size;
            if (fread(&code_size, sizeof(uint32_t), 1, f) != 1) {
                fclose(f);
                return false;
            }
            
            DynamicArray<Instruction>* code = new DynamicArray<Instruction>();
            code->reserve(code_size);
            for (uint32_t j = 0; j < code_size; j++) {
                Instruction inst;
                if (fread(&inst, sizeof(Instruction), 1, f) != 1) {
                    delete code;
                    fclose(f);
                    return false;
                }
                code->push(inst);
            }
            
            function_codes.insert(name, code);
        }
        
        for (uint32_t i = 0; i < header.num_processes; i++) {
            uint32_t name_len;
            if (fread(&name_len, sizeof(uint32_t), 1, f) != 1) {
                fclose(f);
                return false;
            }
            
            char* name_buf = new char[name_len + 1];
            if (fread(name_buf, 1, name_len, f) != name_len) {
                delete[] name_buf;
                fclose(f);
                return false;
            }
            name_buf[name_len] = '\0';
            String name(name_buf);
            delete[] name_buf;
            
            uint32_t code_size;
            if (fread(&code_size, sizeof(uint32_t), 1, f) != 1) {
                fclose(f);
                return false;
            }
            
            DynamicArray<Instruction>* code = new DynamicArray<Instruction>();
            code->reserve(code_size);
            for (uint32_t j = 0; j < code_size; j++) {
                Instruction inst;
                if (fread(&inst, sizeof(Instruction), 1, f) != 1) {
                    delete code;
                    fclose(f);
                    return false;
                }
                code->push(inst);
            }
            
            process_codes.insert(name, code);
        }
        
        for (uint32_t i = 0; i < header.num_events; i++) {
            uint32_t len;
            if (fread(&len, sizeof(uint32_t), 1, f) != 1) {
                fclose(f);
                return false;
            }
            char* buf = new char[len + 1];
            if (fread(buf, 1, len, f) != len) {
                delete[] buf;
                fclose(f);
                return false;
            }
            buf[len] = '\0';
            events.push(String(buf));
            delete[] buf;
        }
        
        for (uint32_t i = 0; i < header.num_signals; i++) {
            uint32_t len;
            if (fread(&len, sizeof(uint32_t), 1, f) != 1) {
                fclose(f);
                return false;
            }
            char* buf = new char[len + 1];
            if (fread(buf, 1, len, f) != len) {
                delete[] buf;
                fclose(f);
                return false;
            }
            buf[len] = '\0';
            signals.push(String(buf));
            delete[] buf;
        }
        
        for (uint32_t i = 0; i < header.num_classes; i++) {
            uint32_t len;
            if (fread(&len, sizeof(uint32_t), 1, f) != 1) {
                fclose(f);
                return false;
            }
            char* buf = new char[len + 1];
            if (fread(buf, 1, len, f) != len) {
                delete[] buf;
                fclose(f);
                return false;
            }
            buf[len] = '\0';
            classes.push(String(buf));
            delete[] buf;
        }
        
        for (uint32_t i = 0; i < header.string_pool_size; i++) {
            uint32_t len;
            if (fread(&len, sizeof(uint32_t), 1, f) != 1) {
                fclose(f);
                return false;
            }
            char* buf = new char[len + 1];
            if (fread(buf, 1, len, f) != len) {
                delete[] buf;
                fclose(f);
                return false;
            }
            buf[len] = '\0';
            string_pool.push(String(buf));
            delete[] buf;
        }
        
        for (uint32_t i = 0; i < header.constants_size; i++) {
            Value val;
            if (fread(&val, sizeof(Value), 1, f) != 1) {
                fclose(f);
                return false;
            }
            constants.push(val);
        }
        
        fclose(f);
        return true;
    }
};

}
