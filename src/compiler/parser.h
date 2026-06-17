#ifndef TICK_PARSER_H
#define TICK_PARSER_H

#include "token.h"
#include "ast.h"
#include "../core/dynamic_array.h"

namespace Tick {

/// Recursive-descent parser producing the AST. Reports a fatal error and
/// aborts on the first syntax error (single-error model, like v1).
class Parser {
public:
    explicit Parser(const DynamicArray<Token>& tokens);
    Program* parse();

private:
    const DynamicArray<Token>& _tokens;
    size_t _current;
    // suppresses `Name { ... }` struct-literal parsing in positions immediately
    // followed by a block (if/while/for conditions), where `{` opens the body.
    bool _no_struct_lit;

    const Token& cur() const;
    const Token& peek(int offset = 1) const;
    bool check(TokenType t) const;
    bool match(TokenType t);
    Token consume(TokenType t, const char* message);
    void advance();
    [[noreturn]] void fail(const char* message) const;

    // declarations
    void parse_top_level(Program* program);
    FuncDecl* parse_func(bool is_pub, bool allow_self);
    StructDecl* parse_struct(bool is_pub);
    EnumDecl* parse_enum(bool is_pub);
    InterfaceDecl* parse_interface(bool is_pub);
    ImplDecl* parse_impl();
    ExternDecl* parse_extern();
    SignalDecl* parse_signal();
    EventDecl* parse_event();
    ProcessDecl* parse_process();
    GlobalDecl* parse_global(bool is_const);

    // types & params
    TypeRef* parse_type();
    TypeRef* parse_base_type();
    Param* parse_param();

    // statements
    Node* parse_statement();
    Block* parse_block();
    VarDecl* parse_var_decl(bool is_const);
    Node* parse_if();
    Node* parse_while();
    Node* parse_for();
    Node* parse_return();
    Node* parse_defer();
    Node* parse_unsafe();

    // expressions (precedence climbing)
    Node* parse_expression();
    Node* parse_assignment();
    Node* parse_range();
    Node* parse_or();
    Node* parse_and();
    Node* parse_bit_or();
    Node* parse_bit_xor();
    Node* parse_bit_and();
    Node* parse_equality();
    Node* parse_comparison();
    Node* parse_shift();
    Node* parse_term();
    Node* parse_factor();
    Node* parse_unary();
    Node* parse_postfix();
    Node* parse_primary();
    Node* parse_match();
    DynamicArray<Arg> parse_args();
};

}

#endif
