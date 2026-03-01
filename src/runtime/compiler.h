#pragma once

#include "../compiler/ast.h"
#include "../core/string.h"
#include <cstdio>

#define MAX_DEFER_SCOPES 64
#define MAX_DEFERS_PER_SCOPE 64
#define MAX_RAII_PER_SCOPE 32

struct RaiiEntry {
    Tick::String var_name;
    Tick::String class_name;
};

struct CodeBuffer {
    char* data;
    int pos;
    int capacity;

    CodeBuffer();
    ~CodeBuffer();
    void ensure(int needed);
    int append(const char* fmt, ...);
};

class Compiler {
public:
    static bool compile_to_native(const char* source_file, const char* output_file, bool keep_c = false);
    
private:
    static Tick::FunctionDecl* _current_func;
    static Tick::ClassDecl* _current_class;
    static Tick::StmtNode* _defer_scopes[MAX_DEFER_SCOPES][MAX_DEFERS_PER_SCOPE];
    static int _defer_counts[MAX_DEFER_SCOPES];
    static int _defer_depth;
    static Tick::String _expected_type;
    static RaiiEntry _raii_scopes[MAX_DEFER_SCOPES][MAX_RAII_PER_SCOPE];
    static int _raii_counts[MAX_DEFER_SCOPES];

    static Tick::String generate_c_code(Tick::Program* program);
    static void generate_process(CodeBuffer& buf, Tick::ProcessDecl* proc, Tick::Program* program);
    static void generate_function(CodeBuffer& buf, Tick::FunctionDecl* func, Tick::Program* program);
    static void generate_statement(CodeBuffer& buf, Tick::StmtNode* stmt, int indent, Tick::Program* program);
    static void generate_expression(CodeBuffer& buf, Tick::ExprNode* expr, Tick::Program* program);
    static void generate_print_arg(CodeBuffer& buf, Tick::ExprNode* arg, Tick::Program* program);
    static void generate_deferred(CodeBuffer& buf, int indent, Tick::Program* program);
    static void generate_all_deferred(CodeBuffer& buf, int indent, Tick::Program* program);
    static void generate_raii_cleanup(CodeBuffer& buf, int indent, Tick::Program* program);
    static void generate_all_raii_cleanup(CodeBuffer& buf, int indent, Tick::Program* program);
    static void push_defer_scope();
    static void pop_defer_scope();
    static bool class_has_destructor(const Tick::String& class_name, Tick::Program* program);
    static Tick::ClassDecl* find_class(const Tick::String& name, Tick::Program* program);
    static Tick::FunctionDecl* find_method(const Tick::String& class_name, const Tick::String& method_name, Tick::Program* program);
    static void write_to_file(const char* filename, const char* content);
    static bool invoke_gcc(const char* c_file, const char* output_file, const char* extra_flags);
    static void tick_type_to_c_type(const Tick::String& tick_type, Tick::Program* program, char* out, size_t out_size);
    static bool is_typed_ptr_type(const Tick::String& t);
    static Tick::String typed_ptr_base_type(const Tick::String& t);
    static bool is_func_ptr_type(const Tick::String& t);
    static void generate_typed_decl(CodeBuffer& buf, const Tick::String& tick_type, const char* name, Tick::Program* program);
    static Tick::String lookup_var_type(const Tick::String& name, Tick::Program* program);
    static void lookup_var_type_in_block(const Tick::String& name, Tick::BlockStmt* block, Tick::String& result);
    static Tick::String infer_expr_type(Tick::ExprNode* expr, Tick::Program* program);
    static bool is_string_type(Tick::ExprNode* expr, Tick::Program* program);
    static bool is_array_type_str(const Tick::String& t);
};
