#include "lexer.h"
#include <cstring>

namespace Tick {

Lexer::Lexer(const char* source) 
    : _source(source), _position(0), _line(1), _column(1) {}

char Lexer::current_char() {
    return _source[_position];
}

char Lexer::peek_char(int offset) {
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
    if (length == 5 && memcmp(str, "event", 5) == 0) return TokenType::EVENT;
    if (length == 6 && memcmp(str, "signal", 6) == 0) return TokenType::SIGNAL;
    if (length == 7 && memcmp(str, "process", 7) == 0) return TokenType::PROCESS;
    if (length == 3 && memcmp(str, "int", 3) == 0) return TokenType::INT;
    if (length == 5 && memcmp(str, "while", 5) == 0) return TokenType::WHILE;
    if (length == 3 && memcmp(str, "for", 3) == 0) return TokenType::FOR;
    if (length == 2 && memcmp(str, "if", 2) == 0) return TokenType::IF;
    if (length == 4 && memcmp(str, "else", 4) == 0) return TokenType::ELSE;
    if (length == 6 && memcmp(str, "return", 6) == 0) return TokenType::RETURN;
    if (length == 5 && memcmp(str, "break", 5) == 0) return TokenType::BREAK;
    if (length == 4 && memcmp(str, "true", 4) == 0) return TokenType::TRUE;
    if (length == 5 && memcmp(str, "false", 5) == 0) return TokenType::FALSE;
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
    
    while (current_char() >= '0' && current_char() <= '9') {
        advance();
        length++;
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

DynamicArray<Token> Lexer::tokenize() {
    DynamicArray<Token> tokens;
    
    while (current_char() != '\0') {
        skip_whitespace();
        skip_comment();
        
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
        else if (c == '@') {
            advance();
            tokens.push(make_token(TokenType::AT, "@", 1));
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
        else if (c == '<') {
            advance();
            if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::LTE, "<=", 2));
            } else {
                tokens.push(make_token(TokenType::LANGLE, "<", 1));
            }
        }
        else if (c == '>') {
            advance();
            if (current_char() == '=') {
                advance();
                tokens.push(make_token(TokenType::GTE, ">=", 2));
            } else {
                tokens.push(make_token(TokenType::RANGLE, ">", 1));
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
            tokens.push(make_token(TokenType::PLUS, "+", 1));
        }
        else if (c == '-') {
            advance();
            tokens.push(make_token(TokenType::MINUS, "-", 1));
        }
        else if (c == '*') {
            advance();
            tokens.push(make_token(TokenType::STAR, "*", 1));
        }
        else if (c == '/') {
            advance();
            tokens.push(make_token(TokenType::SLASH, "/", 1));
        }
        else if (c == '%') {
            advance();
            tokens.push(make_token(TokenType::PERCENT, "%", 1));
        }
        else if (c == '&' && peek_char() == '&') {
            advance();
            advance();
            tokens.push(make_token(TokenType::AND, "&&", 2));
        }
        else if (c == '|' && peek_char() == '|') {
            advance();
            advance();
            tokens.push(make_token(TokenType::OR, "||", 2));
        }
        else {
            advance();
        }
    }
    
    tokens.push(make_token(TokenType::END_OF_FILE, "", 0));
    return tokens;
}

}
