#ifndef TICK_PARSER_H
#define TICK_PARSER_H

#include "token.h"
#include "ast.h"
#include "../core/dynamic_array.h"

namespace Tick {

class Parser {
public:
    Parser(const DynamicArray<Token>& tokens);
    Program* parse();

private:
    const DynamicArray<Token>& _tokens;
    size_t _current;
    
    Token current_token();
    Token peek_token(int offset = 1);
    bool match(TokenType type);
    bool check(TokenType type);
    Token consume(TokenType type, const char* message);
    void advance();
    
    EventDecl* parse_event_decl();
    SignalDecl* parse_signal_decl();
    ProcessDecl* parse_process_decl();
    FunctionDecl* parse_function_decl();
    
    StmtNode* parse_statement();
    StmtNode* parse_var_decl();
    StmtNode* parse_if_stmt();
    StmtNode* parse_while_stmt();
    StmtNode* parse_return_stmt();
    StmtNode* parse_break_stmt();
    StmtNode* parse_expr_stmt();
    BlockStmt* parse_block();
    
    ExprNode* parse_expression();
    ExprNode* parse_logical_or();
    ExprNode* parse_logical_and();
    ExprNode* parse_equality();
    ExprNode* parse_comparison();
    ExprNode* parse_term();
    ExprNode* parse_factor();
    ExprNode* parse_unary();
    ExprNode* parse_call();
    ExprNode* parse_member();
    ExprNode* parse_postfix();
    ExprNode* parse_primary();
    
    bool is_type_keyword();
    Token parse_type();
};

}

#endif
