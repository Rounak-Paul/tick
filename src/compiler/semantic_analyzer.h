#ifndef TICK_SEMANTIC_ANALYZER_H
#define TICK_SEMANTIC_ANALYZER_H

#include "ast.h"
#include "module_loader.h"
#include "../core/hash_map.h"
#include "../core/dynamic_array.h"
#include "../core/string.h"

namespace Tick {

enum class SymbolType {
    VARIABLE,
    FUNCTION,
    EVENT,
    SIGNAL,
    PROCESS,
    CLASS
};

struct Symbol {
    SymbolType type;
    String name;
    String data_type;
    int param_count;

    Symbol() : type(SymbolType::VARIABLE), param_count(-1) {}
    Symbol(SymbolType t, const String& n, const String& dt, int pc = -1)
        : type(t), name(n), data_type(dt), param_count(pc) {}
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    ~SemanticAnalyzer();
    bool analyze(Program* program);
    void set_module_loader(ModuleLoader* loader);
    void set_current_file_path(const char* path);

private:
    static const int MAX_SCOPE_DEPTH = 64;

    struct ScopeEntry {
        const char* name;
        Symbol* previous;
        bool had_previous;
    };

    HashMap<const char*, Symbol*> _symbols;
    DynamicArray<ScopeEntry> _scope_stack[MAX_SCOPE_DEPTH];
    int _scope_depth;
    bool _has_errors;
    int _loop_depth;
    ModuleLoader* _module_loader;
    const char* _current_file_path;
    Program* _program;
    FunctionDecl* _current_function;

    void error(int line, const char* message);
    void push_scope();
    void pop_scope();
    void declare_in_scope(const char* name, Symbol* sym);

    void analyze_import_decl(ImportDecl* node, Program* program);
    void analyze_event_decl(EventDecl* node);
    void analyze_signal_decl(SignalDecl* node);
    void analyze_process_decl(ProcessDecl* node);
    void analyze_function_decl(FunctionDecl* node);
    void analyze_class_decl(ClassDecl* node);
    void analyze_interface_decl(InterfaceDecl* node);
    void analyze_extern_func_decl(ExternFuncDecl* node);

    void analyze_statement(StmtNode* node);
    void analyze_var_decl(VarDecl* node);
    void analyze_if_stmt(IfStmt* node);
    void analyze_while_stmt(WhileStmt* node);
    void analyze_for_stmt(ForStmt* node);
    void analyze_return_stmt(ReturnStmt* node);
    void analyze_expr_stmt(ExprStmt* node);
    void analyze_block(BlockStmt* node);
    void analyze_defer_stmt(DeferStmt* node);
    void analyze_switch_stmt(SwitchStmt* node);
    void analyze_try_catch_stmt(TryCatchStmt* node);
    void analyze_throw_stmt(ThrowStmt* node);

    void analyze_expression(ExprNode* node);
    void analyze_binary_expr(BinaryExpr* node);
    void analyze_unary_expr(UnaryExpr* node);
    void analyze_assign_expr(AssignExpr* node);
    void analyze_call_expr(CallExpr* node);
    void analyze_member_expr(MemberExpr* node);
    void analyze_identifier(IdentifierExpr* node);
    void analyze_index_expr(IndexExpr* node);
    void analyze_compound_assign_expr(CompoundAssignExpr* node);
    void analyze_postfix_expr(PostfixExpr* node);

    String infer_type(ExprNode* node);
    bool is_numeric_type(const String& t);
    bool is_integer_type(const String& t);
    bool is_float_type(const String& t);
    bool is_array_type(const String& t);
    String array_base_type(const String& t);
    bool types_compatible(const String& expected, const String& actual);
    bool is_builtin_function(const String& name);
};

}

#endif
