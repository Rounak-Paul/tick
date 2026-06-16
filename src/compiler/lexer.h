#ifndef TICK_LEXER_H
#define TICK_LEXER_H

#include "token.h"
#include "../core/dynamic_array.h"

namespace Tick {

/// Converts Tick source text into a flat token stream.
class Lexer {
public:
    /// @param source null-terminated source buffer (not owned)
    explicit Lexer(const char* source);

    /// Tokenize the whole source. Always terminates with END_OF_FILE.
    DynamicArray<Token> tokenize();

private:
    const char* _source;
    size_t _pos;
    int _line;
    int _column;

    char current() const;
    char peek(int offset = 1) const;
    void advance();
    void skip_trivia();

    Token make(TokenType type, const char* start, size_t length);
    Token read_identifier();
    Token read_number();
    Token read_string();
    static TokenType keyword_type(const char* str, size_t length);
};

}

#endif
