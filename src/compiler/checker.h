#ifndef TICK_CHECKER_H
#define TICK_CHECKER_H

#include "ast.h"
#include "../core/hash_map.h"
#include "../core/dynamic_array.h"

namespace Tick {

/// Semantic kind of a name in scope.
enum class SymKind { LOCAL, PARAM, GLOBAL, FUNC, STRUCT, ENUM, INTERFACE, SIGNAL, EVENT };

struct Symbol {
    SymKind kind;
    String name;
    TypeRef* type;        // owned by the symbol
    bool is_mutable;  // true unless binding was declared with `const` qualifier
    SymKind decl;
    Symbol() : kind(SymKind::LOCAL), type(nullptr), is_mutable(true), decl(SymKind::LOCAL) {}
};

/// Per-binding ownership decision computed by the analyzer, consumed by codegen.
struct OwnershipInfo {
    bool needs_reclaim;   // an owned heap value -> insert free/release at scope end
    bool auto_shared;     // promoted to shared because ownership wasn't provable
    OwnershipInfo() : needs_reclaim(false), auto_shared(false) {}
};

/// Type checking + borrow/ownership analysis. Annotates the AST in place:
///   - every expression node gets `resolved_type`
///   - last-use expression nodes are recorded as moves (so codegen elides the copy)
///   - owned locals needing reclamation are recorded per declaration node
class Checker {
public:
    explicit Checker(Program* program);
    ~Checker();

    /// Returns true if the program is well-typed and memory-sound.
    bool check();

    /// Codegen queries: was this expression node the last use of an owned value?
    bool is_move(Node* node) const;
    /// Codegen queries: does this VarDecl/GlobalDecl own a heap value to reclaim?
    bool needs_reclaim(Node* decl) const;

private:
    Program* _program;
    bool _errors;

    // Flat, scoped symbol table. Each entry records the scope depth at which it
    // was declared; pop_scope drops the tail. Lookup scans newest-first.
    struct ScopeVar {
        String name;
        Symbol* sym;     // owned here
        int depth;
    };
    DynamicArray<ScopeVar> _stack;
    int _depth;

    // analysis side-tables (keyed by node pointer)
    HashMap<const void*, bool> _moves;
    HashMap<const void*, bool> _reclaims;

    FuncDecl* _current_func;
    String _current_self_type;

    void error(int line, const char* fmt, ...);
    void push_scope();
    void pop_scope();
    void declare(const String& name, Symbol* sym);
    Symbol* lookup(const String& name);

    // declarations
    void collect_globals();
    void check_func(FuncDecl* fn);
    void check_impl(ImplDecl* impl);
    void check_interfaces();

    // statements
    void check_block(Block* block);
    void check_stmt(Node* stmt);
    void check_var(VarDecl* d);

    // expressions: returns the resolved type (owned by the node)
    TypeRef* check_expr(Node* expr);
    TypeRef* check_call(Call* call);
    TypeRef* check_method_call(MethodCall* mc);
    TypeRef* check_field(Field* f);
    TypeRef* check_index(Index* ix);
    TypeRef* check_struct_lit(StructLit* lit);
    TypeRef* check_array_lit(ArrayLit* lit);
    TypeRef* check_match(MatchExpr* m);
    TypeRef* check_binary(Binary* b);

    // helpers
    StructDecl* find_struct(const String& name);
    EnumDecl* find_enum(const String& name);
    InterfaceDecl* find_interface(const String& name);
    FuncDecl* find_method(const String& type_name, const String& method);
    FuncDecl* find_function(const String& name);
    ExternDecl* find_extern(const String& name);
    SignalDecl* find_signal(const String& name);
    EnumVariant* find_variant(EnumDecl* en, const String& variant);

    bool types_equal(const TypeRef* a, const TypeRef* b) const;
    bool assignable(const TypeRef* target, const TypeRef* value) const;
    bool is_numeric(const TypeRef* t) const;
    TypeRef* builtin_call_type(const String& name, Call* call);
    bool is_builtin(const String& name) const;
    void mark_move(Node* node);
    void mark_reclaim(Node* decl);
};

}

#endif
