#ifndef TICK_AST_H
#define TICK_AST_H

#include "type.h"
#include "../core/string.h"
#include "../core/dynamic_array.h"

namespace Tick {

enum class NodeKind {
    // expressions
    INT_LIT, FLOAT_LIT, BOOL_LIT, STRING_LIT,
    IDENT, SELF,
    BINARY, UNARY, ASSIGN, COMPOUND_ASSIGN,
    CALL, METHOD_CALL, FIELD, INDEX,
    STRUCT_LIT, ARRAY_LIT, RANGE,
    REF_EXPR, SHARED_EXPR, WEAK_EXPR, CAST, SIZEOF,
    MATCH,

    // statements
    BLOCK, LET_DECL, EXPR_STMT,
    IF, WHILE, FOR, RETURN, BREAK, CONTINUE, DEFER, UNSAFE_BLOCK,

    // declarations
    PROGRAM, FUNC_DECL, STRUCT_DECL, ENUM_DECL, INTERFACE_DECL,
    IMPL_DECL, EXTERN_DECL,
    SIGNAL_DECL, EVENT_DECL, PROCESS_DECL, GLOBAL_DECL
};

struct Node {
    NodeKind kind;
    int line;
    /// Filled in by the type checker. Owned by the node.
    TypeRef* resolved_type;

    explicit Node(NodeKind k) : kind(k), line(0), resolved_type(nullptr) {}
    virtual ~Node() { delete resolved_type; }
};

// ---- Expressions ----

struct IntLit : Node {
    long long value;
    explicit IntLit(long long v) : Node(NodeKind::INT_LIT), value(v) {}
};

struct FloatLit : Node {
    double value;
    explicit FloatLit(double v) : Node(NodeKind::FLOAT_LIT), value(v) {}
};

struct BoolLit : Node {
    bool value;
    explicit BoolLit(bool v) : Node(NodeKind::BOOL_LIT), value(v) {}
};

struct StringLit : Node {
    String value;
    explicit StringLit(const String& v) : Node(NodeKind::STRING_LIT), value(v) {}
};

struct Ident : Node {
    String name;
    explicit Ident(const String& n) : Node(NodeKind::IDENT), name(n) {}
};

struct SelfExpr : Node {
    SelfExpr() : Node(NodeKind::SELF) {}
};

struct Binary : Node {
    String op;
    Node* left;
    Node* right;
    Binary(Node* l, const String& o, Node* r)
        : Node(NodeKind::BINARY), op(o), left(l), right(r) {}
    ~Binary() { delete left; delete right; }
};

struct Unary : Node {
    String op;
    Node* operand;
    Unary(const String& o, Node* x) : Node(NodeKind::UNARY), op(o), operand(x) {}
    ~Unary() { delete operand; }
};

struct Assign : Node {
    Node* target;
    Node* value;
    Assign(Node* t, Node* v) : Node(NodeKind::ASSIGN), target(t), value(v) {}
    ~Assign() { delete target; delete value; }
};

struct CompoundAssign : Node {
    Node* target;
    String op;
    Node* value;
    CompoundAssign(Node* t, const String& o, Node* v)
        : Node(NodeKind::COMPOUND_ASSIGN), target(t), op(o), value(v) {}
    ~CompoundAssign() { delete target; delete value; }
};

/// Argument with optional `ref` marker at the call site.
struct Arg {
    Node* value;
    bool is_ref;
    bool ref_mutable;
    Arg() : value(nullptr), is_ref(false), ref_mutable(false) {}
};

struct Call : Node {
    String callee;             // free function or struct/enum constructor name
    DynamicArray<Arg> args;
    explicit Call(const String& c) : Node(NodeKind::CALL), callee(c) {}
    ~Call() { for (size_t i = 0; i < args.size(); i++) delete args[i].value; }
};

struct MethodCall : Node {
    Node* receiver;
    String method;
    DynamicArray<Arg> args;
    MethodCall(Node* r, const String& m)
        : Node(NodeKind::METHOD_CALL), receiver(r), method(m) {}
    ~MethodCall() { delete receiver; for (size_t i = 0; i < args.size(); i++) delete args[i].value; }
};

struct Field : Node {
    Node* object;
    String name;
    Field(Node* o, const String& n) : Node(NodeKind::FIELD), object(o), name(n) {}
    ~Field() { delete object; }
};

struct Index : Node {
    Node* array;
    Node* index;
    Index(Node* a, Node* i) : Node(NodeKind::INDEX), array(a), index(i) {}
    ~Index() { delete array; delete index; }
};

struct FieldInit {
    String name;
    Node* value;
    FieldInit() : value(nullptr) {}
};

struct StructLit : Node {
    String type_name;
    DynamicArray<FieldInit> fields;
    explicit StructLit(const String& n) : Node(NodeKind::STRUCT_LIT), type_name(n) {}
    ~StructLit() { for (size_t i = 0; i < fields.size(); i++) delete fields[i].value; }
};

struct ArrayLit : Node {
    DynamicArray<Node*> elements;
    ArrayLit() : Node(NodeKind::ARRAY_LIT) {}
    ~ArrayLit() { for (size_t i = 0; i < elements.size(); i++) delete elements[i]; }
};

struct RangeExpr : Node {
    Node* start;
    Node* end;
    RangeExpr(Node* s, Node* e) : Node(NodeKind::RANGE), start(s), end(e) {}
    ~RangeExpr() { delete start; delete end; }
};

struct RefExpr : Node {
    Node* operand;
    bool mutable_ref;
    RefExpr(Node* x, bool m) : Node(NodeKind::REF_EXPR), operand(x), mutable_ref(m) {}
    ~RefExpr() { delete operand; }
};

struct SharedExpr : Node {
    Node* operand;
    explicit SharedExpr(Node* x) : Node(NodeKind::SHARED_EXPR), operand(x) {}
    ~SharedExpr() { delete operand; }
};

struct WeakExpr : Node {
    Node* operand;
    explicit WeakExpr(Node* x) : Node(NodeKind::WEAK_EXPR), operand(x) {}
    ~WeakExpr() { delete operand; }
};

struct CastExpr : Node {
    Node* operand;
    TypeRef* target;
    CastExpr(Node* x, TypeRef* t) : Node(NodeKind::CAST), operand(x), target(t) {}
    ~CastExpr() { delete operand; delete target; }
};

struct SizeofExpr : Node {
    TypeRef* target;
    explicit SizeofExpr(TypeRef* t) : Node(NodeKind::SIZEOF), target(t) {}
    ~SizeofExpr() { delete target; }
};

struct MatchArm {
    String variant;       // variant name, or empty for wildcard "_"
    bool is_wildcard;
    Node* body;           // block
    MatchArm() : is_wildcard(false), body(nullptr) {}
    ~MatchArm() { delete body; }
};

struct MatchExpr : Node {
    Node* subject;
    DynamicArray<MatchArm*> arms;
    explicit MatchExpr(Node* s) : Node(NodeKind::MATCH), subject(s) {}
    ~MatchExpr() { delete subject; for (size_t i = 0; i < arms.size(); i++) delete arms[i]; }
};

// ---- Statements ----

struct Block : Node {
    DynamicArray<Node*> statements;
    Block() : Node(NodeKind::BLOCK) {}
    ~Block() { for (size_t i = 0; i < statements.size(); i++) delete statements[i]; }
};

struct LetDecl : Node {
    bool is_mutable;        // let vs var
    String name;
    TypeRef* declared_type; // may be null -> inferred
    Node* init;             // may be null
    LetDecl(bool m, const String& n)
        : Node(NodeKind::LET_DECL), is_mutable(m), name(n), declared_type(nullptr), init(nullptr) {}
    ~LetDecl() { delete declared_type; delete init; }
};

struct ExprStmt : Node {
    Node* expr;
    explicit ExprStmt(Node* e) : Node(NodeKind::EXPR_STMT), expr(e) {}
    ~ExprStmt() { delete expr; }
};

struct IfStmt : Node {
    Node* cond;
    Block* then_branch;
    Node* else_branch;   // Block or IfStmt or null
    IfStmt(Node* c, Block* t, Node* e)
        : Node(NodeKind::IF), cond(c), then_branch(t), else_branch(e) {}
    ~IfStmt() { delete cond; delete then_branch; delete else_branch; }
};

struct WhileStmt : Node {
    Node* cond;
    Block* body;
    WhileStmt(Node* c, Block* b) : Node(NodeKind::WHILE), cond(c), body(b) {}
    ~WhileStmt() { delete cond; delete body; }
};

struct ForStmt : Node {
    String var_name;
    bool bind_ref;        // for ref x in ...
    Node* iterable;       // RangeExpr or array expression
    Block* body;
    ForStmt(const String& v) : Node(NodeKind::FOR), var_name(v), bind_ref(false), iterable(nullptr), body(nullptr) {}
    ~ForStmt() { delete iterable; delete body; }
};

struct ReturnStmt : Node {
    Node* value;   // may be null
    explicit ReturnStmt(Node* v) : Node(NodeKind::RETURN), value(v) {}
    ~ReturnStmt() { delete value; }
};

struct BreakStmt : Node { BreakStmt() : Node(NodeKind::BREAK) {} };
struct ContinueStmt : Node { ContinueStmt() : Node(NodeKind::CONTINUE) {} };

struct DeferStmt : Node {
    Node* stmt;
    explicit DeferStmt(Node* s) : Node(NodeKind::DEFER), stmt(s) {}
    ~DeferStmt() { delete stmt; }
};

struct UnsafeBlock : Node {
    Block* body;
    explicit UnsafeBlock(Block* b) : Node(NodeKind::UNSAFE_BLOCK), body(b) {}
    ~UnsafeBlock() { delete body; }
};

// ---- Declarations ----

struct Param {
    String name;
    TypeRef* type;
    Param() : type(nullptr) {}
    ~Param() { delete type; }
};

enum class SelfKind { NONE, VALUE, REF, REF_MUT };

struct FuncDecl : Node {
    bool is_pub;
    String name;
    String impl_type;        // non-empty when this is a method in an impl block
    SelfKind self_kind;
    DynamicArray<Param*> params;
    TypeRef* return_type;
    Block* body;             // null for interface method signatures
    FuncDecl(const String& n)
        : Node(NodeKind::FUNC_DECL), is_pub(false), name(n),
          self_kind(SelfKind::NONE), return_type(nullptr), body(nullptr) {}
    ~FuncDecl() {
        for (size_t i = 0; i < params.size(); i++) delete params[i];
        delete return_type; delete body;
    }
};

struct FieldDecl {
    String name;
    TypeRef* type;
    FieldDecl() : type(nullptr) {}
    ~FieldDecl() { delete type; }
};

struct StructDecl : Node {
    bool is_pub;
    String name;
    DynamicArray<FieldDecl*> fields;
    explicit StructDecl(const String& n) : Node(NodeKind::STRUCT_DECL), is_pub(false), name(n) {}
    ~StructDecl() { for (size_t i = 0; i < fields.size(); i++) delete fields[i]; }
};

struct EnumVariant {
    String name;
    bool has_int_value;
    long long int_value;
    EnumVariant() : has_int_value(false), int_value(0) {}
};

struct EnumDecl : Node {
    bool is_pub;
    String name;
    DynamicArray<EnumVariant*> variants;
    explicit EnumDecl(const String& n) : Node(NodeKind::ENUM_DECL), is_pub(false), name(n) {}
    ~EnumDecl() { for (size_t i = 0; i < variants.size(); i++) delete variants[i]; }
};

struct InterfaceDecl : Node {
    bool is_pub;
    String name;
    DynamicArray<FuncDecl*> methods;   // bodies are null
    explicit InterfaceDecl(const String& n) : Node(NodeKind::INTERFACE_DECL), is_pub(false), name(n) {}
    ~InterfaceDecl() { for (size_t i = 0; i < methods.size(); i++) delete methods[i]; }
};

struct ImplDecl : Node {
    String type_name;
    String interface_name;   // empty for inherent impl
    DynamicArray<FuncDecl*> methods;
    explicit ImplDecl(const String& t) : Node(NodeKind::IMPL_DECL), type_name(t) {}
    ~ImplDecl() { for (size_t i = 0; i < methods.size(); i++) delete methods[i]; }
};

struct ExternDecl : Node {
    String name;
    DynamicArray<Param*> params;
    TypeRef* return_type;
    explicit ExternDecl(const String& n) : Node(NodeKind::EXTERN_DECL), name(n), return_type(nullptr) {}
    ~ExternDecl() {
        for (size_t i = 0; i < params.size(); i++) delete params[i];
        delete return_type;
    }
};

struct SignalDecl : Node {
    String name;
    TypeRef* payload_type;
    explicit SignalDecl(const String& n) : Node(NodeKind::SIGNAL_DECL), name(n), payload_type(nullptr) {}
    ~SignalDecl() { delete payload_type; }
};

struct EventDecl : Node {
    String name;
    explicit EventDecl(const String& n) : Node(NodeKind::EVENT_DECL), name(n) {}
};

struct ProcessDecl : Node {
    String name;
    String event_name;
    Block* body;
    ProcessDecl(const String& n, const String& e)
        : Node(NodeKind::PROCESS_DECL), name(n), event_name(e), body(nullptr) {}
    ~ProcessDecl() { delete body; }
};

struct GlobalDecl : Node {
    bool is_mutable;
    String name;
    TypeRef* type;
    Node* init;
    GlobalDecl(bool m, const String& n)
        : Node(NodeKind::GLOBAL_DECL), is_mutable(m), name(n), type(nullptr), init(nullptr) {}
    ~GlobalDecl() { delete type; delete init; }
};

struct Program : Node {
    DynamicArray<GlobalDecl*> globals;
    DynamicArray<StructDecl*> structs;
    DynamicArray<EnumDecl*> enums;
    DynamicArray<InterfaceDecl*> interfaces;
    DynamicArray<ImplDecl*> impls;
    DynamicArray<ExternDecl*> externs;
    DynamicArray<FuncDecl*> functions;
    DynamicArray<SignalDecl*> signals;
    DynamicArray<EventDecl*> events;
    DynamicArray<ProcessDecl*> processes;
    DynamicArray<String> link_flags;

    Program() : Node(NodeKind::PROGRAM) {}
    ~Program() {
        for (size_t i = 0; i < globals.size(); i++) delete globals[i];
        for (size_t i = 0; i < structs.size(); i++) delete structs[i];
        for (size_t i = 0; i < enums.size(); i++) delete enums[i];
        for (size_t i = 0; i < interfaces.size(); i++) delete interfaces[i];
        for (size_t i = 0; i < impls.size(); i++) delete impls[i];
        for (size_t i = 0; i < externs.size(); i++) delete externs[i];
        for (size_t i = 0; i < functions.size(); i++) delete functions[i];
        for (size_t i = 0; i < signals.size(); i++) delete signals[i];
        for (size_t i = 0; i < events.size(); i++) delete events[i];
        for (size_t i = 0; i < processes.size(); i++) delete processes[i];
    }
};

}

#endif
