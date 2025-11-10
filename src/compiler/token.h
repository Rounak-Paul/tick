#ifndef TICK_TOKEN_H
#define TICK_TOKEN_H

#include "../core/string.h"

namespace Tick {

enum class TokenType {
    IDENTIFIER,
    INTEGER,
    FLOAT_LITERAL,
    DOUBLE_LITERAL,
    STRING,
    
    EVENT,
    SIGNAL,
    PROCESS,
    CLASS,
    NEW,
    THIS,
    INT,
    BOOL,
    FLOAT,
    DOUBLE,
    STRING_TYPE,
    WHILE,
    FOR,
    IF,
    ELSE,
    RETURN,
    BREAK,
    TRUE,
    FALSE,
    
    AT,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    LANGLE,
    RANGLE,
    COMMA,
    SEMICOLON,
    DOT,
    
    ASSIGN,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    
    EQ,
    NEQ,
    LT,
    GT,
    LTE,
    GTE,
    
    AND,
    OR,
    NOT,
    
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
