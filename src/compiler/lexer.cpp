#include "lexer.h"
#include <cstdio>
#include <cstring>
#include <cctype>

namespace Tick {

Lexer::Lexer(const char* source) 
    : _source(source), _position(0), _line(1), _column(1) {}

char Lexer::current_char() {
    return _source[_position];
}

char Lexer::peek_char(int offset) {
    for (int i = 0; i < offset; i++) {
        if (_source[_position + i] == '\0') return '\0';
    }
    return _source[_position + offset];
}

void Lexer::advance() {
    if (current_char() == '\n') {
        _line++;
        _column = 1;
    } else {
        _column++;
    }
    _position++;
}

void Lexer::skip_whitespace() {
    while (current_char() == ' ' || current_char() == '\t' || 
           current_char() == '\n' || current_char() == '\r') {
        advance();
    }
}

void Lexer::skip_comment() {
    if (current_char() == '/' && peek_char() == '/') {
        while (current_char() != '\n' && current_char() != '\0') {
            advance();
        }
    } else if (current_char() == '/' && peek_char() == '*') {
        advance();
        advance();
        while (current_char() != '\0') {
            if (current_char() == '*' && peek_char() == '/') {
                advance();
                advance();
                return;
            }
            advance();
        }
    }
}

Token Lexer::make_token(TokenType type, const char* start, size_t length) {
    char* lexeme = (char*)malloc(length + 1);
    memcpy(lexeme, start, length);
    lexeme[length] = '\0';
    Token token(type, lexeme, _line, _column - length);
    free(lexeme);
    return token;
}

TokenType Lexer::check_keyword(const char* str, size_t length) {
    if (length == 2) {
        if (memcmp(str, "if", 2) == 0) return TokenType::IF;
        if (memcmp(str, "u8", 2) == 0) return TokenType::U8;
        if (memcmp(str, "i8", 2) == 0) return TokenType::I8;
        if (memcmp(str, "b8", 2) == 0) return TokenType::B8;
    }
    if (length == 3) {
        if (memcmp(str, "var", 3) == 0) return TokenType::VAR;
        if (memcmp(str, "for", 3) == 0) return TokenType::FOR;
        if (memcmp(str, "str", 3) == 0) return TokenType::STR;
        if (memcmp(str, "try", 3) == 0) return TokenType::TRY;
        if (memcmp(str, "ptr", 3) == 0) return TokenType::PTR;
        if (memcmp(str, "u16", 3) == 0) return TokenType::U16;
        if (memcmp(str, "u32", 3) == 0) return TokenType::U32;
        if (memcmp(str, "u64", 3) == 0) return TokenType::U64;
        if (memcmp(str, "i16", 3) == 0) return TokenType::I16;
        if (memcmp(str, "i32", 3) == 0) return TokenType::I32;
        if (memcmp(str, "i64", 3) == 0) return TokenType::I64;
        if (memcmp(str, "f32", 3) == 0) return TokenType::F32;
        if (memcmp(str, "f64", 3) == 0) return TokenType::F64;
    }
    if (length == 4) {
        if (memcmp(str, "func", 4) == 0) return TokenType::FUNC;
        if (memcmp(str, "this", 4) == 0) return TokenType::THIS;
        if (memcmp(str, "from", 4) == 0) return TokenType::FROM;
        if (memcmp(str, "void", 4) == 0) return TokenType::VOID_TYPE;
        if (memcmp(str, "else", 4) == 0) return TokenType::ELSE;
        if (memcmp(str, "true", 4) == 0) return TokenType::TRUE;
        if (memcmp(str, "case", 4) == 0) return TokenType::CASE;
        if (memcmp(str, "enum", 4) == 0) return TokenType::ENUM;
        if (memcmp(str, "cast", 4) == 0) return TokenType::CAST;
        if (memcmp(str, "null", 4) == 0) return TokenType::NULL_LIT;
        if (memcmp(str, "link", 4) == 0) return TokenType::LINK;
    }
    if (length == 5) {
        if (memcmp(str, "event", 5) == 0) return TokenType::EVENT;
        if (memcmp(str, "class", 5) == 0) return TokenType::CLASS;
        if (memcmp(str, "const", 5) == 0) return TokenType::CONST;
        if (memcmp(str, "while", 5) == 0) return TokenType::WHILE;
        if (memcmp(str, "break", 5) == 0) return TokenType::BREAK;
        if (memcmp(str, "false", 5) == 0) return TokenType::FALSE;
        if (memcmp(str, "defer", 5) == 0) return TokenType::DEFER;
        if (memcmp(str, "union", 5) == 0) return TokenType::UNION;
        if (memcmp(str, "catch", 5) == 0) return TokenType::CATCH;
        if (memcmp(str, "throw", 5) == 0) return TokenType::THROW;
    }
    if (length == 6) {
        if (memcmp(str, "signal", 6) == 0) return TokenType::SIGNAL;
        if (memcmp(str, "import", 6) == 0) return TokenType::IMPORT;
        if (memcmp(str, "return", 6) == 0) return TokenType::RETURN;
        if (memcmp(str, "switch", 6) == 0) return TokenType::SWITCH;
        if (memcmp(str, "extern", 6) == 0) return TokenType::EXTERN;
        if (memcmp(str, "sizeof", 6) == 0) return TokenType::SIZEOF;
    }
    if (length == 7) {
        if (memcmp(str, "process", 7) == 0) return TokenType::PROCESS;
        if (memcmp(str, "default", 7) == 0) return TokenType::DEFAULT;
    }
    if (length == 8 && memcmp(str, "continue", 8) == 0) return TokenType::CONTINUE;
    if (length == 9 && memcmp(str, "interface", 9) == 0) return TokenType::INTERFACE;
    if (length == 10 && memcmp(str, "implements", 10) == 0) return TokenType::IMPLEMENTS;
    return TokenType::IDENTIFIER;
}

Token Lexer::read_identifier() {
    const char* start = &_source[_position];
    size_t length = 0;
    
    while ((current_char() >= 'a' && current_char() <= 'z') ||
           (current_char() >= 'A' && current_char() <= 'Z') ||
           (current_char() >= '0' && current_char() <= '9') ||
           current_char() == '_') {
        advance();
        length++;
    }
    
    TokenType type = check_keyword(start, length);
    return make_token(type, start, length);
}

Token Lexer::read_number() {
    const char* start = &_source[_position];
    size_t length = 0;
    bool is_float = false;
    
    while (current_char() >= '0' && current_char() <= '9') {
        advance();
        length++;
    }
    
    if (current_char() == '.' && peek_char() >= '0' && peek_char() <= '9') {
        is_float = true;
        advance();
        length++;
        
        while (current_char() >= '0' && current_char() <= '9') {
            advance();
            length++;
        }
    }
    
    if (is_float) {
        if (current_char() == 'f' || current_char() == 'F') {
            advance();
            length++;
            return make_token(TokenType::FLOAT_LITERAL, start, length);
        }
        return make_token(TokenType::DOUBLE_LITERAL, start, length);
    }
    
    return make_token(TokenType::INTEGER, start, length);
}

Token Lexer::read_string() {
    advance();
    const char* start = &_source[_position];
    size_t length = 0;
    
    while (current_char() != '"' && current_char() != '\0') {
        if (current_char() == '\\') {
            advance();
            if (current_char() != '\0') {
                advance();
                length += 2;
            }
        } else {
            advance();
            length++;
        }
    }
    
    if (current_char() == '"') {
        advance();
    }
    
    return make_token(TokenType::STRING, start, length);
}

Token Lexer::read_multiline_string() {
    advance();
    const char* start = &_source[_position];
    size_t length = 0;
    
    while (current_char() != '`' && current_char() != '\0') {
        advance();
        length++;
    }
    
    if (current_char() == '`') {
        advance();
    }
    
    return make_token(TokenType::STRING, start, length);
}

DynamicArray<Token> Lexer::tokenize() {
    DynamicArray<Token> tokens;
    
    while (current_char() != '\0') {
        skip_whitespace();
        
        while (current_char() == '/' && (peek_char() == '/' || peek_char() == '*')) {
            skip_comment();
            skip_whitespace();
        }
        
        if (current_char() == '\0') break;
        
        char c = current_char();
        
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            tokens.push(read_identifier());
        }
        else if (c >= '0' && c <= '9') {
            tokens.push(read_number());
        }
        else if (c == '"') {
            tokens.push(read_string());
        }
        else if (c == '`') {
            tokens.push(read_multiline_string());
        }
        else if (c == '@') {
            advance();
            if (memcmp(_source + _position, "dataclass", 9) == 0) {
                char after = _source[_position + 9];
                if (!isalnum(after) && after != '_') {
                    _position += 9;
                    _column += 9;
                    tokens.push(make_token(TokenType::DATACLASS, "@dataclass", 10));
                } else {
                    tokens.push(make_token(TokenType::AT, "@", 1));
                }
            } else {
                tokens.push(make_token(TokenType::AT, "@", 1));
            }
        }
        else if (c == '(') {
            advance();
            tokens.push(make_token(TokenType::LPAREN, "(", 1));
        }
        else if (c == ')') {
            advance();
            tokens.push(make_token(TokenType::RPAREN, ")", 1));
        }
        else if (c == '{') {
            advance();
            tokens.push(make_token(TokenType::LBRACE, "{", 1));
        }
        else if (c == '}') {
            advance();
            tokens.push(make_token(TokenType::RBRACE, "}", 1));
        }
        else if (c == '[') {
            advance();
            tokens.push(make_token(TokenType::LBRACKET, "[", 1));
        }
        else if (c == ']') {
            advance();
            tokens.push(make_token(TokenType::RBRACKET, "]", 1));
        }
        else if (c == '<') {
            advance();
            if (current_char() == '<') {
                advance();
                if (current_char() == '=') {
                    advance();
                    tokens.push(make_token(TokenType::LSHIFT_ASSIGN, "<<=", 3));
                } else {
                    tokens.push(make_token(TokenType::LSHIFT, "<<", 2));
                }
            } else if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::LTE, "<=", 2));
            } else {
                tokens.push(make_token(TokenType::LT, "<", 1));
            }
        }
        else if (c == '>') {
            advance();
            if (current_char() == '>') {
                advance();
                if (current_char() == '=') {
                    advance();
                    tokens.push(make_token(TokenType::RSHIFT_ASSIGN, ">>=", 3));
                } else {
                    tokens.push(make_token(TokenType::RSHIFT, ">>", 2));
                }
            } else if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::GTE, ">=", 2));
            } else {
                tokens.push(make_token(TokenType::GT, ">", 1));
            }
        }
        else if (c == ',') {
            advance();
            tokens.push(make_token(TokenType::COMMA, ",", 1));
        }
        else if (c == ';') {
            advance();
            tokens.push(make_token(TokenType::SEMICOLON, ";", 1));
        }
        else if (c == ':') {
            advance();
            tokens.push(make_token(TokenType::COLON, ":", 1));
        }
        else if (c == '.') {
            advance();
            tokens.push(make_token(TokenType::DOT, ".", 1));
        }
        else if (c == '=') {
            advance();
            if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::EQ, "==", 2));
            } else {
                tokens.push(make_token(TokenType::ASSIGN, "=", 1));
            }
        }
        else if (c == '!') {
            advance();
            if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::NEQ, "!=", 2));
            } else {
                tokens.push(make_token(TokenType::NOT, "!", 1));
            }
        }
        else if (c == '+') {
            advance();
            if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::PLUS_ASSIGN, "+=", 2));
            } else if (current_char() == '+') {
                advance();
                tokens.push(make_token(TokenType::INCREMENT, "++", 2));
            } else {
                tokens.push(make_token(TokenType::PLUS, "+", 1));
            }
        }
        else if (c == '-') {
            advance();
            if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::MINUS_ASSIGN, "-=", 2));
            } else if (current_char() == '-') {
                advance();
                tokens.push(make_token(TokenType::DECREMENT, "--", 2));
            } else {
                tokens.push(make_token(TokenType::MINUS, "-", 1));
            }
        }
        else if (c == '*') {
            advance();
            if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::STAR_ASSIGN, "*=", 2));
            } else {
                tokens.push(make_token(TokenType::STAR, "*", 1));
            }
        }
        else if (c == '/') {
            advance();
            if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::SLASH_ASSIGN, "/=", 2));
            } else {
                tokens.push(make_token(TokenType::SLASH, "/", 1));
            }
        }
        else if (c == '%') {
            advance();
            if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::PERCENT_ASSIGN, "%=", 2));
            } else {
                tokens.push(make_token(TokenType::PERCENT, "%", 1));
            }
        }
        else if (c == '&') {
            advance();
            if (current_char() == '&') {
                advance();
                tokens.push(make_token(TokenType::AND, "&&", 2));
            } else if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::AMPERSAND_ASSIGN, "&=", 2));
            } else {
                tokens.push(make_token(TokenType::AMPERSAND, "&", 1));
            }
        }
        else if (c == '|') {
            advance();
            if (current_char() == '|') {
                advance();
                tokens.push(make_token(TokenType::OR, "||", 2));
            } else if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::PIPE_ASSIGN, "|=", 2));
            } else {
                tokens.push(make_token(TokenType::PIPE, "|", 1));
            }
        }
        else if (c == '^') {
            advance();
            if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::CARET_ASSIGN, "^=", 2));
            } else {
                tokens.push(make_token(TokenType::CARET, "^", 1));
            }
        }
        else if (c == '~') {
            advance();
            tokens.push(make_token(TokenType::TILDE, "~", 1));
        }
        else {
            fprintf(stderr, "Lexer error: unexpected character '%c' at line %zu, column %zu\n", 
                c, _line, _column);
            advance();
        }
    }
    
    tokens.push(make_token(TokenType::END_OF_FILE, "", 0));
    return tokens;
}

}
