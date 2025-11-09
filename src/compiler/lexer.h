#ifndef TICK_LEXER_H
#define TICK_LEXER_H

#include "token.h"
#include "../core/dynamic_array.h"
#include "../core/string.h"

namespace Tick {

class Lexer {
public:
    Lexer(const char* source);
    DynamicArray<Token> tokenize();

private:
    const char* _source;
    size_t _position;
    size_t _line;
    size_t _column;
    
    char current_char();
    char peek_char(int offset = 1);
    void advance();
    void skip_whitespace();
    void skip_comment();
    
    Token make_token(TokenType type, const char* start, size_t length);
    Token read_identifier();
    Token read_number();
    Token read_string();
    TokenType check_keyword(const char* str, size_t length);
};

}

#endif
