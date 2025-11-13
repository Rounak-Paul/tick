#pragma once

#include "../compiler/ast.h"
#include "../core/string.h"
#include <cstdio>

class Compiler {
public:
    static bool compile_to_native(const char* source_file, const char* output_file, bool keep_c = false);
    
private:
    static Tick::String generate_c_code(Tick::Program* program);
    static int generate_process(char* buffer, int pos, Tick::ProcessDecl* proc, Tick::Program* program);
    static int generate_function(char* buffer, int pos, Tick::FunctionDecl* func, Tick::Program* program);
    static int generate_statement(char* buffer, int pos, Tick::StmtNode* stmt, int indent, Tick::Program* program);
    static int generate_expression(char* buffer, int pos, Tick::ExprNode* expr, Tick::Program* program);
    static void write_to_file(const char* filename, const char* content);
    static bool invoke_gcc(const char* c_file, const char* output_file);
    static const char* tick_type_to_c_type(const Tick::String& tick_type, Tick::Program* program);
};
