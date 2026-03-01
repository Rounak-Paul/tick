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
    void add_define(const char* name);
    bool has_define(const char* name) const;

private:
    const DynamicArray<Token>& _tokens;
    size_t _current;
    DynamicArray<String> _defines;
    
    Token current_token();
    Token peek_token(int offset = 1);
    bool match(TokenType type);
    bool check(TokenType type);
    Token consume(TokenType type, const char* message);
    void advance();
    
    ImportDecl* parse_import_decl();
    EventDecl* parse_event_decl();
    SignalDecl* parse_signal_decl();
    ProcessDecl* parse_process_decl();
    FunctionDecl* parse_function_decl();
    void parse_class_decl(Program* program, bool is_dataclass = false);
    EnumDecl* parse_enum_decl();
    UnionDecl* parse_union_decl();
    void parse_interface_decl(Program* program);
    ExternFuncDecl* parse_extern_func_decl();
    
    StmtNode* parse_statement();
    StmtNode* parse_try_catch_stmt();
    StmtNode* parse_throw_stmt();
    StmtNode* parse_var_decl();
    StmtNode* parse_if_stmt();
    StmtNode* parse_while_stmt();
    StmtNode* parse_for_stmt();
    StmtNode* parse_return_stmt();
    StmtNode* parse_break_stmt();
    StmtNode* parse_continue_stmt();
    StmtNode* parse_defer_stmt();
    StmtNode* parse_switch_stmt();
    StmtNode* parse_expr_stmt();
    BlockStmt* parse_block();
    
    ExprNode* parse_expression();
    ExprNode* parse_assignment();
    ExprNode* parse_logical_or();
    ExprNode* parse_logical_and();
    ExprNode* parse_bitwise_or();
    ExprNode* parse_bitwise_xor();
    ExprNode* parse_bitwise_and();
    ExprNode* parse_equality();
    ExprNode* parse_comparison();
    ExprNode* parse_shift();
    ExprNode* parse_term();
    ExprNode* parse_factor();
    ExprNode* parse_unary();
    ExprNode* parse_postfix();
    ExprNode* parse_primary();
    
    bool is_type_keyword();
    Token parse_type();
    bool parse_top_level_decl(Program* program);
    void skip_to_matching_brace();
    void parse_conditional_compile(Program* program);
};

}

#endif
