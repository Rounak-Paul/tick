#include "parser.h"
#include <cstdio>
#include <cstdlib>

namespace Tick {

Parser::Parser(const DynamicArray<Token>& tokens) 
    : _tokens(tokens), _current(0) {}

Token Parser::current_token() {
    return _tokens[_current];
}

Token Parser::peek_token(int offset) {
    if (_current + offset < _tokens.size()) {
        return _tokens[_current + offset];
    }
    return _tokens[_tokens.size() - 1];
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) {
    return current_token().type == type;
}

Token Parser::consume(TokenType type, const char* message) {
    if (check(type)) {
        Token token = current_token();
        advance();
        return token;
    }
    fprintf(stderr, "Parse error: %s\n", message);
    exit(1);
}

void Parser::advance() {
    if (current_token().type != TokenType::END_OF_FILE) {
        _current++;
    }
}

bool Parser::is_type_keyword() {
    TokenType type = current_token().type;
    return type == TokenType::INT || type == TokenType::BOOL || 
           type == TokenType::FLOAT || type == TokenType::DOUBLE ||
           type == TokenType::STRING_TYPE;
}

Token Parser::parse_type() {
    if (!is_type_keyword()) {
        fprintf(stderr, "Parse error: Expected type\n");
        exit(1);
    }
    Token type_token = current_token();
    advance();
    
    if (check(TokenType::LBRACKET)) {
        advance();
        consume(TokenType::RBRACKET, "Expected ']' after '['");
        size_t base_len = type_token.lexeme.length();
        char* array_type = (char*)malloc(base_len + 3);
        memcpy(array_type, type_token.lexeme.c_str(), base_len);
        array_type[base_len] = '[';
        array_type[base_len + 1] = ']';
        array_type[base_len + 2] = '\0';
        type_token.lexeme = String(array_type);
        free(array_type);
    }
    
    return type_token;
}

Program* Parser::parse() {
    Program* program = new Program();
    
    while (!check(TokenType::END_OF_FILE)) {
        if (check(TokenType::EVENT)) {
            program->events.push(parse_event_decl());
        }
        else if (check(TokenType::SIGNAL)) {
            program->signals.push(parse_signal_decl());
        }
        else if (check(TokenType::AT)) {
            program->processes.push(parse_process_decl());
        }
        else if (is_type_keyword()) {
            program->functions.push(parse_function_decl());
        }
        else {
            fprintf(stderr, "Unexpected token at top level\n");
            exit(1);
        }
    }
    
    return program;
}

EventDecl* Parser::parse_event_decl() {
    consume(TokenType::EVENT, "Expected 'event'");
    Token name = consume(TokenType::IDENTIFIER, "Expected event name");
    consume(TokenType::SEMICOLON, "Expected ';' after event declaration");
    
    return new EventDecl(name.lexeme);
}

SignalDecl* Parser::parse_signal_decl() {
    consume(TokenType::SIGNAL, "Expected 'signal'");
    consume(TokenType::LANGLE, "Expected '<'");
    Token type = parse_type();
    consume(TokenType::RANGLE, "Expected '>'");
    Token name = consume(TokenType::IDENTIFIER, "Expected signal name");
    consume(TokenType::SEMICOLON, "Expected ';' after signal declaration");
    
    return new SignalDecl(type.lexeme, name.lexeme);
}

ProcessDecl* Parser::parse_process_decl() {
    consume(TokenType::AT, "Expected '@'");
    Token event_name = consume(TokenType::IDENTIFIER, "Expected event name");
    consume(TokenType::PROCESS, "Expected 'process'");
    Token name = consume(TokenType::IDENTIFIER, "Expected process name");
    BlockStmt* body = parse_block();
    
    return new ProcessDecl(event_name.lexeme, name.lexeme, body);
}

FunctionDecl* Parser::parse_function_decl() {
    Token return_type = parse_type();
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    consume(TokenType::LPAREN, "Expected '(' after function name");
    
    FunctionDecl* func = new FunctionDecl(return_type.lexeme, name.lexeme, nullptr);
    
    if (!check(TokenType::RPAREN)) {
        do {
            Token param_type = parse_type();
            Token param_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            func->parameters.push(new Parameter(param_type.lexeme, param_name.lexeme));
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    func->body = parse_block();
    
    return func;
}

BlockStmt* Parser::parse_block() {
    consume(TokenType::LBRACE, "Expected '{'");
    
    BlockStmt* block = new BlockStmt();
    
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        block->statements.push(parse_statement());
    }
    
    consume(TokenType::RBRACE, "Expected '}'");
    return block;
}

StmtNode* Parser::parse_statement() {
    if (is_type_keyword()) {
        return parse_var_decl();
    }
    if (check(TokenType::IF)) {
        return parse_if_stmt();
    }
    if (check(TokenType::WHILE)) {
        return parse_while_stmt();
    }
    if (check(TokenType::RETURN)) {
        return parse_return_stmt();
    }
    if (check(TokenType::BREAK)) {
        return parse_break_stmt();
    }
    if (check(TokenType::LBRACE)) {
        return parse_block();
    }
    return parse_expr_stmt();
}

StmtNode* Parser::parse_var_decl() {
    Token type = parse_type();
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
    
    ExprNode* initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = parse_expression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return new VarDecl(type.lexeme, name.lexeme, initializer);
}

StmtNode* Parser::parse_if_stmt() {
    consume(TokenType::IF, "Expected 'if'");
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    ExprNode* condition = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    
    StmtNode* then_branch = parse_statement();
    StmtNode* else_branch = nullptr;
    
    if (match(TokenType::ELSE)) {
        else_branch = parse_statement();
    }
    
    return new IfStmt(condition, then_branch, else_branch);
}

StmtNode* Parser::parse_while_stmt() {
    consume(TokenType::WHILE, "Expected 'while'");
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    ExprNode* condition = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    StmtNode* body = parse_statement();
    
    return new WhileStmt(condition, body);
}

StmtNode* Parser::parse_return_stmt() {
    consume(TokenType::RETURN, "Expected 'return'");
    ExprNode* value = nullptr;
    
    if (!check(TokenType::SEMICOLON)) {
        value = parse_expression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after return statement");
    return new ReturnStmt(value);
}

StmtNode* Parser::parse_break_stmt() {
    consume(TokenType::BREAK, "Expected 'break'");
    consume(TokenType::SEMICOLON, "Expected ';' after break statement");
    return new BreakStmt();
}

StmtNode* Parser::parse_expr_stmt() {
    ExprNode* expr = parse_expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression");
    return new ExprStmt(expr);
}

ExprNode* Parser::parse_expression() {
    return parse_logical_or();
}

ExprNode* Parser::parse_logical_or() {
    ExprNode* expr = parse_logical_and();
    
    while (match(TokenType::OR)) {
        String op("||");
        ExprNode* right = parse_logical_and();
        expr = new BinaryExpr(expr, op, right);
    }
    
    return expr;
}

ExprNode* Parser::parse_logical_and() {
    ExprNode* expr = parse_equality();
    
    while (match(TokenType::AND)) {
        String op("&&");
        ExprNode* right = parse_equality();
        expr = new BinaryExpr(expr, op, right);
    }
    
    return expr;
}

ExprNode* Parser::parse_equality() {
    ExprNode* expr = parse_comparison();
    
    while (match(TokenType::EQ) || match(TokenType::NEQ)) {
        String op = _tokens[_current - 1].lexeme;
        ExprNode* right = parse_comparison();
        expr = new BinaryExpr(expr, op, right);
    }
    
    return expr;
}

ExprNode* Parser::parse_comparison() {
    ExprNode* expr = parse_term();
    
    while (match(TokenType::LT) || match(TokenType::GT) || 
           match(TokenType::LTE) || match(TokenType::GTE)) {
        String op = _tokens[_current - 1].lexeme;
        ExprNode* right = parse_term();
        expr = new BinaryExpr(expr, op, right);
    }
    
    return expr;
}

ExprNode* Parser::parse_term() {
    ExprNode* expr = parse_factor();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        String op = _tokens[_current - 1].lexeme;
        ExprNode* right = parse_factor();
        expr = new BinaryExpr(expr, op, right);
    }
    
    return expr;
}

ExprNode* Parser::parse_factor() {
    ExprNode* expr = parse_unary();
    
    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        String op = _tokens[_current - 1].lexeme;
        ExprNode* right = parse_unary();
        expr = new BinaryExpr(expr, op, right);
    }
    
    return expr;
}

ExprNode* Parser::parse_unary() {
    if (match(TokenType::NOT) || match(TokenType::MINUS)) {
        String op = _tokens[_current - 1].lexeme;
        ExprNode* operand = parse_unary();
        return new UnaryExpr(op, operand);
    }
    
    return parse_call();
}

ExprNode* Parser::parse_call() {
    ExprNode* expr = parse_member();
    
    while (match(TokenType::LPAREN)) {
        CallExpr* call = new CallExpr(expr);
        
        if (!check(TokenType::RPAREN)) {
            do {
                call->arguments.push(parse_expression());
            } while (match(TokenType::COMMA));
        }
        
        consume(TokenType::RPAREN, "Expected ')' after arguments");
        expr = call;
    }
    
    return expr;
}

ExprNode* Parser::parse_member() {
    ExprNode* expr = parse_postfix();
    
    while (match(TokenType::DOT)) {
        Token member = consume(TokenType::IDENTIFIER, "Expected member name");
        expr = new MemberExpr(expr, member.lexeme);
    }
    
    return expr;
}

ExprNode* Parser::parse_postfix() {
    ExprNode* expr = parse_primary();
    
    while (check(TokenType::LBRACKET)) {
        advance();
        ExprNode* index = parse_expression();
        consume(TokenType::RBRACKET, "Expected ']' after index");
        expr = new IndexExpr(expr, index);
    }
    
    return expr;
}

ExprNode* Parser::parse_primary() {
    if (match(TokenType::TRUE)) {
        return new BoolLiteral(true);
    }
    if (match(TokenType::FALSE)) {
        return new BoolLiteral(false);
    }
    if (match(TokenType::INTEGER)) {
        Token token = _tokens[_current - 1];
        int value = 0;
        for (size_t i = 0; i < token.lexeme.length(); i++) {
            value = value * 10 + (token.lexeme[i] - '0');
        }
        return new IntegerLiteral(value);
    }
    if (match(TokenType::FLOAT_LITERAL)) {
        Token token = _tokens[_current - 1];
        float value = 0.0f;
        float decimal = 0.0f;
        int decimal_places = 0;
        bool is_decimal = false;
        
        for (size_t i = 0; i < token.lexeme.length(); i++) {
            if (token.lexeme[i] == '.') {
                is_decimal = true;
                continue;
            }
            if (token.lexeme[i] == 'f') break;
            
            if (is_decimal) {
                decimal = decimal * 10.0f + (token.lexeme[i] - '0');
                decimal_places++;
            } else {
                value = value * 10.0f + (token.lexeme[i] - '0');
            }
        }
        
        for (int i = 0; i < decimal_places; i++) {
            decimal /= 10.0f;
        }
        value += decimal;
        
        return new FloatLiteral(value);
    }
    if (match(TokenType::DOUBLE_LITERAL)) {
        Token token = _tokens[_current - 1];
        double value = 0.0;
        double decimal = 0.0;
        int decimal_places = 0;
        bool is_decimal = false;
        
        for (size_t i = 0; i < token.lexeme.length(); i++) {
            if (token.lexeme[i] == '.') {
                is_decimal = true;
                continue;
            }
            
            if (is_decimal) {
                decimal = decimal * 10.0 + (token.lexeme[i] - '0');
                decimal_places++;
            } else {
                value = value * 10.0 + (token.lexeme[i] - '0');
            }
        }
        
        for (int i = 0; i < decimal_places; i++) {
            decimal /= 10.0;
        }
        value += decimal;
        
        return new DoubleLiteral(value);
    }
    if (match(TokenType::STRING)) {
        Token token = _tokens[_current - 1];
        char* str_val = (char*)malloc(token.lexeme.length() + 1);
        size_t j = 0;
        for (size_t i = 0; i < token.lexeme.length(); i++) {
            if (token.lexeme[i] == '\\' && i + 1 < token.lexeme.length()) {
                i++;
                if (token.lexeme[i] == 'n') str_val[j++] = '\n';
                else if (token.lexeme[i] == 't') str_val[j++] = '\t';
                else if (token.lexeme[i] == '\\') str_val[j++] = '\\';
                else if (token.lexeme[i] == '"') str_val[j++] = '"';
                else str_val[j++] = token.lexeme[i];
            } else {
                str_val[j++] = token.lexeme[i];
            }
        }
        str_val[j] = '\0';
        String result(str_val);
        free(str_val);
        return new StringLiteral(result);
    }
    if (match(TokenType::IDENTIFIER)) {
        return new IdentifierExpr(_tokens[_current - 1].lexeme);
    }
    if (match(TokenType::LPAREN)) {
        ExprNode* expr = parse_expression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    if (match(TokenType::LBRACKET)) {
        ArrayExpr* array = new ArrayExpr();
        
        if (!check(TokenType::RBRACKET)) {
            do {
                array->elements.push(parse_expression());
            } while (match(TokenType::COMMA));
        }
        
        consume(TokenType::RBRACKET, "Expected ']' after array elements");
        return array;
    }
    
    fprintf(stderr, "Unexpected token in expression\n");
    exit(1);
}

}
