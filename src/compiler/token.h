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
    
    VAR,
    FUNC,
    EVENT,
    SIGNAL,
    PROCESS,
    CLASS,
    CONST,
    THIS,
    IMPORT,
    FROM,
    U8,
    U16,
    U32,
    U64,
    I8,
    I16,
    I32,
    I64,
    F32,
    F64,
    B8,
    STR,
    VOID_TYPE,
    WHILE,
    FOR,
    IF,
    ELSE,
    RETURN,
    BREAK,
    CONTINUE,
    DEFER,
    SWITCH,
    CASE,
    DEFAULT,
    ENUM,
    UNION,
    TRY,
    CATCH,
    THROW,
    INTERFACE,
    IMPLEMENTS,
    EXTERN,
    DATACLASS,
    PTR,
    CAST,
    NULL_LIT,
    LINK,
    SIZEOF,
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
    COLON,
    COMMA,
    SEMICOLON,
    DOT,
    
    ASSIGN,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    AMPERSAND,
    PIPE,
    CARET,
    TILDE,
    LSHIFT,
    RSHIFT,
    INCREMENT,
    DECREMENT,
    
    PLUS_ASSIGN,
    MINUS_ASSIGN,
    STAR_ASSIGN,
    SLASH_ASSIGN,
    PERCENT_ASSIGN,
    AMPERSAND_ASSIGN,
    PIPE_ASSIGN,
    CARET_ASSIGN,
    LSHIFT_ASSIGN,
    RSHIFT_ASSIGN,
    
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
