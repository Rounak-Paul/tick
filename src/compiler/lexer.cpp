#include "lexer.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace Tick {

Lexer::Lexer(const char* source)
    : _source(source), _pos(0), _line(1), _column(1) {}

char Lexer::current() const {
    return _source[_pos];
}

char Lexer::peek(int offset) const {
    for (int i = 0; i < offset; i++) {
        if (_source[_pos + i] == '\0') return '\0';
    }
    return _source[_pos + offset];
}

void Lexer::advance() {
    if (current() == '\n') {
        _line++;
        _column = 1;
    } else {
        _column++;
    }
    _pos++;
}

void Lexer::skip_trivia() {
    for (;;) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek() == '/') {
            while (current() != '\n' && current() != '\0') advance();
        } else if (c == '/' && peek() == '*') {
            advance();
            advance();
            while (current() != '\0' && !(current() == '*' && peek() == '/')) advance();
            if (current() != '\0') { advance(); advance(); }
        } else {
            return;
        }
    }
}

Token Lexer::make(TokenType type, const char* start, size_t length) {
    char* lexeme = (char*)malloc(length + 1);
    memcpy(lexeme, start, length);
    lexeme[length] = '\0';
    Token token(type, lexeme, _line, _column - (int)length);
    free(lexeme);
    return token;
}

TokenType Lexer::keyword_type(const char* str, size_t length) {
    switch (length) {
        case 2:
            if (memcmp(str, "if", 2) == 0) return TokenType::IF;
            if (memcmp(str, "in", 2) == 0) return TokenType::IN;
            if (memcmp(str, "on", 2) == 0) return TokenType::ON;
            if (memcmp(str, "i8", 2) == 0) return TokenType::I8;
            if (memcmp(str, "u8", 2) == 0) return TokenType::U8;
            break;
        case 3:
            if (memcmp(str, "var", 3) == 0) return TokenType::VAR;
            if (memcmp(str, "ref", 3) == 0) return TokenType::REF;
            if (memcmp(str, "dyn", 3) == 0) return TokenType::DYN;
            if (memcmp(str, "for", 3) == 0) return TokenType::FOR;
            if (memcmp(str, "pub", 3) == 0) return TokenType::PUB;
            if (memcmp(str, "str", 3) == 0) return TokenType::STR;
            if (memcmp(str, "ptr", 3) == 0) return TokenType::PTR;
            if (memcmp(str, "i16", 3) == 0) return TokenType::I16;
            if (memcmp(str, "i32", 3) == 0) return TokenType::I32;
            if (memcmp(str, "i64", 3) == 0) return TokenType::I64;
            if (memcmp(str, "u16", 3) == 0) return TokenType::U16;
            if (memcmp(str, "u32", 3) == 0) return TokenType::U32;
            if (memcmp(str, "u64", 3) == 0) return TokenType::U64;
            if (memcmp(str, "f32", 3) == 0) return TokenType::F32;
            if (memcmp(str, "f64", 3) == 0) return TokenType::F64;
            break;
        case 4:
            if (memcmp(str, "func", 4) == 0) return TokenType::FUNC;
            if (memcmp(str, "enum", 4) == 0) return TokenType::ENUM;
            if (memcmp(str, "impl", 4) == 0) return TokenType::IMPL;
            if (memcmp(str, "weak", 4) == 0) return TokenType::WEAK;
            if (memcmp(str, "self", 4) == 0) return TokenType::SELF;
            if (memcmp(str, "from", 4) == 0) return TokenType::FROM;
            if (memcmp(str, "else", 4) == 0) return TokenType::ELSE;
            if (memcmp(str, "true", 4) == 0) return TokenType::TRUE;
            if (memcmp(str, "void", 4) == 0) return TokenType::VOID;
            if (memcmp(str, "bool", 4) == 0) return TokenType::BOOL;
            if (memcmp(str, "cast", 4) == 0) return TokenType::CAST;
            if (memcmp(str, "link", 4) == 0) return TokenType::LINK;
            break;
        case 5:
            if (memcmp(str, "const", 5) == 0) return TokenType::CONST;
            if (memcmp(str, "while", 5) == 0) return TokenType::WHILE;
            if (memcmp(str, "match", 5) == 0) return TokenType::MATCH;
            if (memcmp(str, "break", 5) == 0) return TokenType::BREAK;
            if (memcmp(str, "defer", 5) == 0) return TokenType::DEFER;
            if (memcmp(str, "false", 5) == 0) return TokenType::FALSE;
            if (memcmp(str, "event", 5) == 0) return TokenType::EVENT;
            break;
        case 6:
            if (memcmp(str, "struct", 6) == 0) return TokenType::STRUCT;
            if (memcmp(str, "shared", 6) == 0) return TokenType::SHARED;
            if (memcmp(str, "return", 6) == 0) return TokenType::RETURN;
            if (memcmp(str, "import", 6) == 0) return TokenType::IMPORT;
            if (memcmp(str, "extern", 6) == 0) return TokenType::EXTERN;
            if (memcmp(str, "unsafe", 6) == 0) return TokenType::UNSAFE;
            if (memcmp(str, "signal", 6) == 0) return TokenType::SIGNAL;
            if (memcmp(str, "sizeof", 6) == 0) return TokenType::SIZEOF;
            break;
        case 7:
            if (memcmp(str, "process", 7) == 0) return TokenType::PROCESS;
            break;
        case 8:
            if (memcmp(str, "continue", 8) == 0) return TokenType::CONTINUE;
            break;
        case 9:
            if (memcmp(str, "interface", 9) == 0) return TokenType::INTERFACE;
            break;
    }
    return TokenType::IDENTIFIER;
}

Token Lexer::read_identifier() {
    const char* start = &_source[_pos];
    size_t length = 0;
    char c = current();
    while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_') {
        advance();
        length++;
        c = current();
    }
    return make(keyword_type(start, length), start, length);
}

Token Lexer::read_number() {
    const char* start = &_source[_pos];
    size_t length = 0;
    bool is_float = false;

    while (current() >= '0' && current() <= '9') { advance(); length++; }

    if (current() == '.' && peek() >= '0' && peek() <= '9') {
        is_float = true;
        advance(); length++;
        while (current() >= '0' && current() <= '9') { advance(); length++; }
    }

    return make(is_float ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL, start, length);
}

Token Lexer::read_string() {
    advance(); // opening quote
    const char* start = &_source[_pos];
    size_t length = 0;
    while (current() != '"' && current() != '\0') {
        if (current() == '\\') { advance(); length++; if (current() != '\0') { advance(); length++; } }
        else { advance(); length++; }
    }
    Token tok = make(TokenType::STRING_LITERAL, start, length);
    if (current() == '"') advance();
    return tok;
}

DynamicArray<Token> Lexer::tokenize() {
    DynamicArray<Token> tokens;

    for (;;) {
        skip_trivia();
        char c = current();
        if (c == '\0') break;

        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            tokens.push(read_identifier());
            continue;
        }
        if (c >= '0' && c <= '9') {
            tokens.push(read_number());
            continue;
        }
        if (c == '"') {
            tokens.push(read_string());
            continue;
        }

        switch (c) {
            case '(': advance(); tokens.push(make(TokenType::LPAREN, "(", 1)); break;
            case ')': advance(); tokens.push(make(TokenType::RPAREN, ")", 1)); break;
            case '{': advance(); tokens.push(make(TokenType::LBRACE, "{", 1)); break;
            case '}': advance(); tokens.push(make(TokenType::RBRACE, "}", 1)); break;
            case '[': advance(); tokens.push(make(TokenType::LBRACKET, "[", 1)); break;
            case ']': advance(); tokens.push(make(TokenType::RBRACKET, "]", 1)); break;
            case ',': advance(); tokens.push(make(TokenType::COMMA, ",", 1)); break;
            case ';': advance(); tokens.push(make(TokenType::SEMICOLON, ";", 1)); break;
            case ':': advance(); tokens.push(make(TokenType::COLON, ":", 1)); break;
            case '~': advance(); tokens.push(make(TokenType::TILDE, "~", 1)); break;
            case '.':
                advance();
                if (current() == '.') { advance(); tokens.push(make(TokenType::DOTDOT, "..", 2)); }
                else tokens.push(make(TokenType::DOT, ".", 1));
                break;
            case '<':
                advance();
                if (current() == '<') {
                    advance();
                    if (current() == '=') { advance(); tokens.push(make(TokenType::LSHIFT_ASSIGN, "<<=", 3)); }
                    else tokens.push(make(TokenType::LSHIFT, "<<", 2));
                } else if (current() == '=') { advance(); tokens.push(make(TokenType::LTE, "<=", 2)); }
                else tokens.push(make(TokenType::LANGLE, "<", 1));
                break;
            case '>':
                advance();
                if (current() == '>') {
                    advance();
                    if (current() == '=') { advance(); tokens.push(make(TokenType::RSHIFT_ASSIGN, ">>=", 3)); }
                    else tokens.push(make(TokenType::RSHIFT, ">>", 2));
                } else if (current() == '=') { advance(); tokens.push(make(TokenType::GTE, ">=", 2)); }
                else tokens.push(make(TokenType::RANGLE, ">", 1));
                break;
            case '=':
                advance();
                if (current() == '=') { advance(); tokens.push(make(TokenType::EQ, "==", 2)); }
                else if (current() == '>') { advance(); tokens.push(make(TokenType::ARROW, "=>", 2)); }
                else tokens.push(make(TokenType::ASSIGN, "=", 1));
                break;
            case '!':
                advance();
                if (current() == '=') { advance(); tokens.push(make(TokenType::NEQ, "!=", 2)); }
                else tokens.push(make(TokenType::BANG, "!", 1));
                break;
            case '+':
                advance();
                if (current() == '=') { advance(); tokens.push(make(TokenType::PLUS_ASSIGN, "+=", 2)); }
                else tokens.push(make(TokenType::PLUS, "+", 1));
                break;
            case '-':
                advance();
                if (current() == '=') { advance(); tokens.push(make(TokenType::MINUS_ASSIGN, "-=", 2)); }
                else tokens.push(make(TokenType::MINUS, "-", 1));
                break;
            case '*':
                advance();
                if (current() == '=') { advance(); tokens.push(make(TokenType::STAR_ASSIGN, "*=", 2)); }
                else tokens.push(make(TokenType::STAR, "*", 1));
                break;
            case '/':
                advance();
                if (current() == '=') { advance(); tokens.push(make(TokenType::SLASH_ASSIGN, "/=", 2)); }
                else tokens.push(make(TokenType::SLASH, "/", 1));
                break;
            case '%':
                advance();
                if (current() == '=') { advance(); tokens.push(make(TokenType::PERCENT_ASSIGN, "%=", 2)); }
                else tokens.push(make(TokenType::PERCENT, "%", 1));
                break;
            case '&':
                advance();
                if (current() == '&') { advance(); tokens.push(make(TokenType::AND, "&&", 2)); }
                else if (current() == '=') { advance(); tokens.push(make(TokenType::AMPERSAND_ASSIGN, "&=", 2)); }
                else tokens.push(make(TokenType::AMPERSAND, "&", 1));
                break;
            case '|':
                advance();
                if (current() == '|') { advance(); tokens.push(make(TokenType::OR, "||", 2)); }
                else if (current() == '=') { advance(); tokens.push(make(TokenType::PIPE_ASSIGN, "|=", 2)); }
                else tokens.push(make(TokenType::PIPE, "|", 1));
                break;
            case '^':
                advance();
                if (current() == '=') { advance(); tokens.push(make(TokenType::CARET_ASSIGN, "^=", 2)); }
                else tokens.push(make(TokenType::CARET, "^", 1));
                break;
            default:
                fprintf(stderr, "Lexer error: unexpected character '%c' at line %d, column %d\n",
                        c, _line, _column);
                advance();
                break;
        }
    }

    tokens.push(make(TokenType::END_OF_FILE, "", 0));
    return tokens;
}

}
