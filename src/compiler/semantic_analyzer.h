#ifndef TICK_SEMANTIC_ANALYZER_H
#define TICK_SEMANTIC_ANALYZER_H

#include "ast.h"
#include "../core/hash_map.h"
#include "../core/string.h"

namespace Tick {

enum class SymbolType {
    VARIABLE,
    FUNCTION,
    EVENT,
    SIGNAL,
    PROCESS
};

struct Symbol {
    SymbolType type;
    String name;
    String data_type;
    
    Symbol() : type(SymbolType::VARIABLE) {}
    Symbol(SymbolType t, const String& n, const String& dt)
        : type(t), name(n), data_type(dt) {}
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    bool analyze(Program* program);

private:
    HashMap<const char*, Symbol*> _symbols;
    bool _has_errors;
    
    void error(const char* message);
    
    void analyze_event_decl(EventDecl* node);
    void analyze_signal_decl(SignalDecl* node);
    void analyze_process_decl(ProcessDecl* node);
    void analyze_function_decl(FunctionDecl* node);
    
    void analyze_statement(StmtNode* node);
    void analyze_var_decl(VarDecl* node);
    void analyze_if_stmt(IfStmt* node);
    void analyze_while_stmt(WhileStmt* node);
    void analyze_return_stmt(ReturnStmt* node);
    void analyze_expr_stmt(ExprStmt* node);
    void analyze_block(BlockStmt* node);
    
    void analyze_expression(ExprNode* node);
    void analyze_binary_expr(BinaryExpr* node);
    void analyze_unary_expr(UnaryExpr* node);
    void analyze_call_expr(CallExpr* node);
    void analyze_member_expr(MemberExpr* node);
    void analyze_identifier(IdentifierExpr* node);
};

}

#endif
