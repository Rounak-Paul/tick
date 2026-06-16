#ifndef TICK_TOKEN_H
#define TICK_TOKEN_H

#include "../core/string.h"

namespace Tick {

enum class TokenType {
    IDENTIFIER,
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,

    // bindings & decls
    LET,
    VAR,
    FUNC,
    STRUCT,
    INTERFACE,
    IMPL,
    ENUM,
    PUB,
    IMPORT,
    FROM,
    EXTERN,
    LINK,

    // qualifiers
    REF,
    SHARED,
    WEAK,
    DYN,
    SELF,

    // types
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    F32, F64,
    BOOL,
    STR,
    VOID,
    PTR,

    // control flow
    IF,
    ELSE,
    WHILE,
    FOR,
    IN,
    MATCH,
    RETURN,
    BREAK,
    CONTINUE,
    DEFER,
    UNSAFE,

    // concurrency
    SIGNAL,
    EVENT,
    PROCESS,
    ON,

    // literals / keywords
    TRUE,
    FALSE,
    CAST,
    SIZEOF,

    // punctuation
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    LBRACKET, RBRACKET,
    LANGLE, RANGLE,
    COLON,
    COMMA,
    SEMICOLON,
    DOT,
    DOTDOT,
    ARROW,        // =>

    // operators
    ASSIGN,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    AMPERSAND, PIPE, CARET, TILDE,
    LSHIFT, RSHIFT,
    PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN, PERCENT_ASSIGN,
    AMPERSAND_ASSIGN, PIPE_ASSIGN, CARET_ASSIGN, LSHIFT_ASSIGN, RSHIFT_ASSIGN,
    EQ, NEQ, LT, GT, LTE, GTE,
    AND, OR, NOT,
    BANG,         // postfix unwrap

    END_OF_FILE,
    INVALID
};

struct Token {
    TokenType type;
    String lexeme;
    int line;
    int column;

    Token() : type(TokenType::INVALID), line(0), column(0) {}
    Token(TokenType t, const char* lex, int l, int c)
        : type(t), lexeme(lex), line(l), column(c) {}
};

}

#endif
