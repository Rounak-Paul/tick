#ifndef TICK_CODEGEN_H
#define TICK_CODEGEN_H

#include "../compiler/ast.h"
#include "../compiler/checker.h"
#include "../core/string.h"
#include "build_mode.h"
#include <cstdio>
#include <cstdarg>

namespace Tick {

/// Growable output buffer for generated C.
struct CBuffer {
    char* data;
    int len;
    int cap;
    CBuffer();
    ~CBuffer();
    void put(const char* fmt, ...);
    void indent(int n);
};

/// Lowers a checked program to C source. Validation-layer checks are emitted
/// according to the build mode; release strips them entirely.
class CodeGen {
public:
    CodeGen(Program* program, Checker* checker, BuildMode mode);
    String generate();

private:
    Program* _program;
    Checker* _checker;
    BuildMode _mode;
    CBuffer _out;

    // reclaim tracking: stack of scopes, each a list of (c_name, kind) to free
    enum class Reclaim { ARRAY, STRING, SHARED, WEAK };
    struct Owned { String name; Reclaim kind; };
    static const int MAX_SCOPES = 128;
    DynamicArray<Owned> _scopes[MAX_SCOPES];
    // deferred statements per scope, run LIFO at scope exit
    DynamicArray<Node*> _defers[MAX_SCOPES];
    int _scope;
    TypeRef* _return_type;   // current function's return type (not owned)

    bool checks_on() const { return _mode != BuildMode::RELEASE; }

    void emit_prelude();
    void emit_struct(StructDecl* st);
    void emit_enum(EnumDecl* en);
    void emit_extern(ExternDecl* ex);
    void emit_func_proto(FuncDecl* fn);
    void emit_func(FuncDecl* fn);
    void emit_process(ProcessDecl* pr);
    void emit_main_init(FuncDecl* main_fn);

    void c_type(const TypeRef* t, CBuffer& b);
    String c_type_str(const TypeRef* t);
    String mangle_func(const String& name);
    String mangle_method(const String& type, const String& method);

    void gen_block(Block* block, int indent, bool new_scope);
    void gen_stmt(Node* stmt, int indent);
    void gen_var(VarDecl* d, int indent);
    void gen_expr(Node* expr);
    void gen_call(Call* call);
    void gen_method_call(MethodCall* mc);
    void gen_match(MatchExpr* m, int indent);

    void push_scope();
    void pop_scope_reclaims(int indent);   // emits defers (LIFO) then reclaims
    void pop_scope();
    void emit_all_reclaims(int indent);    // for early return: all live scopes
    void track_owned(const String& name, const TypeRef* t);
    static Reclaim reclaim_kind(const TypeRef* t);
    static bool is_reclaimable(const TypeRef* t);
};

}

#endif
