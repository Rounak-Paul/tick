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
    fprintf(stderr, "Parse error at line %d: %s\n", current_token().line, message);
    exit(1);
}

void Parser::advance() {
    if (current_token().type != TokenType::END_OF_FILE) {
        _current++;
    }
}

bool Parser::is_type_keyword() {
    TokenType type = current_token().type;
    if (type == TokenType::U8 || type == TokenType::U16 ||
        type == TokenType::U32 || type == TokenType::U64 ||
        type == TokenType::I8 || type == TokenType::I16 ||
        type == TokenType::I32 || type == TokenType::I64 ||
        type == TokenType::F32 || type == TokenType::F64 ||
        type == TokenType::B8 || type == TokenType::STR ||
        type == TokenType::VOID_TYPE || type == TokenType::PTR ||
        type == TokenType::FUNC) {
        return true;
    }
    if (type == TokenType::IDENTIFIER) {
        Token next = peek_token(1);
        return next.type == TokenType::IDENTIFIER || next.type == TokenType::LBRACKET;
    }
    return false;
}

Token Parser::parse_type() {
    TokenType type = current_token().type;
    if (type == TokenType::FUNC) {
        Token type_token = current_token();
        advance();
        consume(TokenType::LPAREN, "Expected '(' after 'func' in function pointer type");
        char buf[512];
        int pos = 0;
        pos += snprintf(buf + pos, sizeof(buf) - pos, "func(");
        if (!check(TokenType::RPAREN)) {
            Token pt = parse_type();
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%s", pt.lexeme.c_str());
            while (match(TokenType::COMMA)) {
                pt = parse_type();
                pos += snprintf(buf + pos, sizeof(buf) - pos, ",%s", pt.lexeme.c_str());
            }
        }
        consume(TokenType::RPAREN, "Expected ')' after function pointer params");
        consume(TokenType::COLON, "Expected ':' after function pointer params");
        Token ret = parse_type();
        pos += snprintf(buf + pos, sizeof(buf) - pos, "):%s", ret.lexeme.c_str());
        type_token.lexeme = String(buf);
        return type_token;
    }
    if (type == TokenType::U8 || type == TokenType::U16 ||
        type == TokenType::U32 || type == TokenType::U64 ||
        type == TokenType::I8 || type == TokenType::I16 ||
        type == TokenType::I32 || type == TokenType::I64 ||
        type == TokenType::F32 || type == TokenType::F64 ||
        type == TokenType::B8 || type == TokenType::STR ||
        type == TokenType::VOID_TYPE || type == TokenType::PTR ||
        type == TokenType::IDENTIFIER) {
        Token type_token = current_token();
        advance();
        
        if (type == TokenType::PTR && check(TokenType::LT)) {
            advance();
            Token inner = parse_type();
            consume(TokenType::GT, "Expected '>' after ptr<type");
            char buf[256];
            snprintf(buf, sizeof(buf), "ptr<%s>", inner.lexeme.c_str());
            type_token.lexeme = String(buf);
            return type_token;
        }
        
        if (check(TokenType::LBRACKET)) {
            advance();
            if (check(TokenType::INTEGER)) {
                Token size_tok = current_token();
                advance();
                consume(TokenType::RBRACKET, "Expected ']' after array size");
                size_t base_len = type_token.lexeme.length();
                size_t size_len = size_tok.lexeme.length();
                char* array_type = (char*)malloc(base_len + size_len + 3);
                memcpy(array_type, type_token.lexeme.c_str(), base_len);
                array_type[base_len] = '[';
                memcpy(array_type + base_len + 1, size_tok.lexeme.c_str(), size_len);
                array_type[base_len + 1 + size_len] = ']';
                array_type[base_len + 2 + size_len] = '\0';
                type_token.lexeme = String(array_type);
                free(array_type);
            } else {
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
        }
        
        return type_token;
    }
    
    fprintf(stderr, "Parse error at line %d: Expected type\n", current_token().line);
    exit(1);
}

Program* Parser::parse() {
    Program* program = new Program();
    
    while (!check(TokenType::END_OF_FILE)) {
        if (!parse_top_level_decl(program)) {
            fprintf(stderr, "Parse error at line %d: Unexpected token at top level\n", current_token().line);
            exit(1);
        }
    }
    
    return program;
}

ImportDecl* Parser::parse_import_decl() {
    int ln = current_token().line;
    if (check(TokenType::FROM)) {
        advance();
        Token module = consume(TokenType::IDENTIFIER, "Expected module name");
        consume(TokenType::IMPORT, "Expected 'import'");
        
        ImportDecl* import_decl = new ImportDecl(module.lexeme);
        import_decl->line = ln;
        import_decl->import_all = false;
        
        if (check(TokenType::STAR)) {
            advance();
            import_decl->import_all = true;
        } else {
            Token name = consume(TokenType::IDENTIFIER, "Expected identifier");
            import_decl->imported_names.push(name.lexeme);
            
            while (check(TokenType::COMMA)) {
                advance();
                Token next_name = consume(TokenType::IDENTIFIER, "Expected identifier");
                import_decl->imported_names.push(next_name.lexeme);
            }
        }
        
        consume(TokenType::SEMICOLON, "Expected ';' after import");
        return import_decl;
    } else {
        advance();
        Token module = consume(TokenType::IDENTIFIER, "Expected module name");
        consume(TokenType::SEMICOLON, "Expected ';' after import");
        
        ImportDecl* decl = new ImportDecl(module.lexeme);
        decl->line = ln;
        return decl;
    }
}

EventDecl* Parser::parse_event_decl() {
    int ln = current_token().line;
    consume(TokenType::EVENT, "Expected 'event'");
    Token name = consume(TokenType::IDENTIFIER, "Expected event name");
    consume(TokenType::SEMICOLON, "Expected ';' after event declaration");
    
    EventDecl* decl = new EventDecl(name.lexeme);
    decl->line = ln;
    return decl;
}

SignalDecl* Parser::parse_signal_decl() {
    int ln = current_token().line;
    consume(TokenType::SIGNAL, "Expected 'signal'");
    Token name = consume(TokenType::IDENTIFIER, "Expected signal name");
    
    int array_size = 0;
    if (check(TokenType::LBRACKET)) {
        advance();
        if (!check(TokenType::RBRACKET)) {
            Token size_token = consume(TokenType::INTEGER, "Expected array size");
            array_size = atoi(size_token.lexeme.c_str());
        }
        consume(TokenType::RBRACKET, "Expected ']'");
    }
    
    consume(TokenType::COLON, "Expected ':'");
    Token type = parse_type();
    consume(TokenType::SEMICOLON, "Expected ';' after signal declaration");
    
    SignalDecl* decl = new SignalDecl(type.lexeme, name.lexeme, array_size);
    decl->line = ln;
    return decl;
}

ProcessDecl* Parser::parse_process_decl() {
    int ln = current_token().line;
    consume(TokenType::AT, "Expected '@'");
    Token event_name = consume(TokenType::IDENTIFIER, "Expected event name");
    consume(TokenType::PROCESS, "Expected 'process'");
    Token name = consume(TokenType::IDENTIFIER, "Expected process name");
    BlockStmt* body = parse_block();
    
    ProcessDecl* decl = new ProcessDecl(event_name.lexeme, name.lexeme, body);
    decl->line = ln;
    return decl;
}

void Parser::parse_class_decl(Program* program, bool is_dataclass) {
    int ln = current_token().line;
    consume(TokenType::CLASS, "Expected 'class'");
    Token name = consume(TokenType::IDENTIFIER, "Expected class name");
    
    String base_class;
    DynamicArray<String> iface_list;
    
    if (!is_dataclass && match(TokenType::COLON)) {
        Token base = consume(TokenType::IDENTIFIER, "Expected base class name after ':'");
        base_class = base.lexeme;
    }
    
    if (!is_dataclass && match(TokenType::IMPLEMENTS)) {
        do {
            Token iface = consume(TokenType::IDENTIFIER, "Expected interface name");
            iface_list.push(iface.lexeme);
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::LBRACE, "Expected '{' after class declaration");
    
    ClassDecl* cls = new ClassDecl(name.lexeme);
    cls->line = ln;
    cls->base_class = base_class;
    cls->is_dataclass = is_dataclass;
    for (size_t i = 0; i < iface_list.size(); i++) {
        cls->interfaces.push(iface_list[i]);
    }
    
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        if (check(TokenType::VAR)) {
            int fln = current_token().line;
            advance();
            Token field_name = consume(TokenType::IDENTIFIER, "Expected field name after 'var'");
            consume(TokenType::COLON, "Expected ':' after field name");
            Token type = parse_type();
            
            ExprNode* initializer = nullptr;
            if (match(TokenType::ASSIGN)) {
                initializer = parse_expression();
            }
            consume(TokenType::SEMICOLON, "Expected ';' after field declaration");
            VarDecl* field = new VarDecl(type.lexeme, field_name.lexeme, initializer);
            field->line = fln;
            cls->fields.push(field);
        } else if (!is_dataclass && check(TokenType::FUNC)) {
            int mln = current_token().line;
            advance();
            
            bool is_dtor = false;
            if (match(TokenType::TILDE)) {
                is_dtor = true;
            }
            
            Token method_name = consume(TokenType::IDENTIFIER, "Expected method name after 'func'");
            consume(TokenType::LPAREN, "Expected '(' after method name");
            
            FunctionDecl* method = new FunctionDecl("", method_name.lexeme, nullptr);
            method->line = mln;
            method->class_name = name.lexeme;
            method->is_destructor = is_dtor;
            
            if (!check(TokenType::RPAREN)) {
                do {
                    Token param_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
                    consume(TokenType::COLON, "Expected ':' after parameter name");
                    Token param_type = parse_type();
                    method->parameters.push(new Parameter(param_type.lexeme, param_name.lexeme));
                } while (match(TokenType::COMMA));
            }
            
            consume(TokenType::RPAREN, "Expected ')' after parameters");
            consume(TokenType::COLON, "Expected ':' after parameters");
            Token return_type = parse_type();
            method->return_type = return_type.lexeme;
            method->body = parse_block();
            program->methods.push(method);
        } else {
            fprintf(stderr, "Parse error at line %d: Expected 'var' or 'func' in class body\n", current_token().line);
            exit(1);
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}' after class body");
    program->classes.push(cls);
}

FunctionDecl* Parser::parse_function_decl() {
    int ln = current_token().line;
    advance();
    
    Token name = consume(TokenType::IDENTIFIER, "Expected function name after 'func'");
    consume(TokenType::LPAREN, "Expected '(' after function name");
    
    FunctionDecl* func = new FunctionDecl("", name.lexeme, nullptr);
    func->line = ln;
    
    if (!check(TokenType::RPAREN)) {
        do {
            Token param_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            consume(TokenType::COLON, "Expected ':' after parameter name");
            Token param_type = parse_type();
            
            func->parameters.push(new Parameter(param_type.lexeme, param_name.lexeme));
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    consume(TokenType::COLON, "Expected ':' after parameters");
    Token return_type = parse_type();
    
    func->return_type = return_type.lexeme;
    func->body = parse_block();
    
    return func;
}

BlockStmt* Parser::parse_block() {
    consume(TokenType::LBRACE, "Expected '{'");
    
    BlockStmt* block = new BlockStmt();
    block->line = current_token().line;
    
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        block->statements.push(parse_statement());
    }
    
    consume(TokenType::RBRACE, "Expected '}'");
    return block;
}

StmtNode* Parser::parse_statement() {
    if (check(TokenType::VAR) || check(TokenType::CONST)) {
        return parse_var_decl();
    }
    if (check(TokenType::IF)) {
        return parse_if_stmt();
    }
    if (check(TokenType::WHILE)) {
        return parse_while_stmt();
    }
    if (check(TokenType::FOR)) {
        return parse_for_stmt();
    }
    if (check(TokenType::RETURN)) {
        return parse_return_stmt();
    }
    if (check(TokenType::BREAK)) {
        return parse_break_stmt();
    }
    if (check(TokenType::CONTINUE)) {
        return parse_continue_stmt();
    }
    if (check(TokenType::DEFER)) {
        return parse_defer_stmt();
    }
    if (check(TokenType::SWITCH)) {
        return parse_switch_stmt();
    }
    if (check(TokenType::TRY)) {
        return parse_try_catch_stmt();
    }
    if (check(TokenType::THROW)) {
        return parse_throw_stmt();
    }
    if (check(TokenType::LBRACE)) {
        return parse_block();
    }
    return parse_expr_stmt();
}

StmtNode* Parser::parse_var_decl() {
    int ln = current_token().line;
    bool is_const = false;
    if (check(TokenType::CONST)) {
        is_const = true;
        advance();
    } else {
        advance();
    }
    
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'var' or 'const'");
    consume(TokenType::COLON, "Expected ':' after variable name");
    Token type = parse_type();
    
    ExprNode* initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = parse_expression();
    } else if (is_const) {
        fprintf(stderr, "Parse error at line %d: const variables must be initialized\n", ln);
        exit(1);
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    VarDecl* decl = new VarDecl(type.lexeme, name.lexeme, initializer, is_const);
    decl->line = ln;
    return decl;
}

StmtNode* Parser::parse_if_stmt() {
    int ln = current_token().line;
    consume(TokenType::IF, "Expected 'if'");
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    ExprNode* condition = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    
    StmtNode* then_branch = parse_statement();
    StmtNode* else_branch = nullptr;
    
    if (match(TokenType::ELSE)) {
        else_branch = parse_statement();
    }
    
    IfStmt* stmt = new IfStmt(condition, then_branch, else_branch);
    stmt->line = ln;
    return stmt;
}

StmtNode* Parser::parse_while_stmt() {
    int ln = current_token().line;
    consume(TokenType::WHILE, "Expected 'while'");
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    ExprNode* condition = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    StmtNode* body = parse_statement();
    
    WhileStmt* stmt = new WhileStmt(condition, body);
    stmt->line = ln;
    return stmt;
}

StmtNode* Parser::parse_for_stmt() {
    int ln = current_token().line;
    consume(TokenType::FOR, "Expected 'for'");
    consume(TokenType::LPAREN, "Expected '(' after 'for'");
    
    StmtNode* initializer = nullptr;
    if (check(TokenType::VAR)) {
        initializer = parse_var_decl();
    } else if (!check(TokenType::SEMICOLON)) {
        initializer = parse_expr_stmt();
    } else {
        advance();
    }
    
    ExprNode* condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = parse_expression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after for condition");
    
    ExprNode* increment = nullptr;
    if (!check(TokenType::RPAREN)) {
        increment = parse_expression();
    }
    consume(TokenType::RPAREN, "Expected ')' after for clauses");
    
    StmtNode* body = parse_statement();
    
    ForStmt* stmt = new ForStmt(initializer, condition, increment, body);
    stmt->line = ln;
    return stmt;
}

StmtNode* Parser::parse_return_stmt() {
    int ln = current_token().line;
    consume(TokenType::RETURN, "Expected 'return'");
    ExprNode* value = nullptr;
    
    if (!check(TokenType::SEMICOLON)) {
        value = parse_expression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after return statement");
    ReturnStmt* stmt = new ReturnStmt(value);
    stmt->line = ln;
    return stmt;
}

StmtNode* Parser::parse_break_stmt() {
    int ln = current_token().line;
    consume(TokenType::BREAK, "Expected 'break'");
    consume(TokenType::SEMICOLON, "Expected ';' after break statement");
    BreakStmt* stmt = new BreakStmt();
    stmt->line = ln;
    return stmt;
}

StmtNode* Parser::parse_continue_stmt() {
    int ln = current_token().line;
    consume(TokenType::CONTINUE, "Expected 'continue'");
    consume(TokenType::SEMICOLON, "Expected ';' after continue statement");
    ContinueStmt* stmt = new ContinueStmt();
    stmt->line = ln;
    return stmt;
}

StmtNode* Parser::parse_defer_stmt() {
    int ln = current_token().line;
    consume(TokenType::DEFER, "Expected 'defer'");
    StmtNode* stmt = parse_statement();
    DeferStmt* defer = new DeferStmt(stmt);
    defer->line = ln;
    return defer;
}

StmtNode* Parser::parse_switch_stmt() {
    int ln = current_token().line;
    consume(TokenType::SWITCH, "Expected 'switch'");
    consume(TokenType::LPAREN, "Expected '(' after 'switch'");
    ExprNode* subject = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after switch expression");
    consume(TokenType::LBRACE, "Expected '{' after switch");

    SwitchStmt* sw = new SwitchStmt(subject);
    sw->line = ln;

    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        SwitchCase* sc = new SwitchCase();
        if (check(TokenType::CASE)) {
            advance();
            sc->values.push(parse_expression());
            consume(TokenType::COLON, "Expected ':' after case value");

            while (check(TokenType::CASE)) {
                advance();
                sc->values.push(parse_expression());
                consume(TokenType::COLON, "Expected ':' after case value");
            }

            BlockStmt* body = new BlockStmt();
            body->line = current_token().line;
            while (!check(TokenType::CASE) && !check(TokenType::DEFAULT) &&
                   !check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
                body->statements.push(parse_statement());
            }
            sc->body = body;
        } else if (check(TokenType::DEFAULT)) {
            advance();
            consume(TokenType::COLON, "Expected ':' after 'default'");
            sc->is_default = true;

            BlockStmt* body = new BlockStmt();
            body->line = current_token().line;
            while (!check(TokenType::CASE) && !check(TokenType::DEFAULT) &&
                   !check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
                body->statements.push(parse_statement());
            }
            sc->body = body;
        } else {
            delete sc;
            fprintf(stderr, "Parse error at line %d: Expected 'case' or 'default' in switch\n", current_token().line);
            exit(1);
        }
        sw->cases.push(sc);
    }

    consume(TokenType::RBRACE, "Expected '}' after switch body");
    return sw;
}

EnumDecl* Parser::parse_enum_decl() {
    int ln = current_token().line;
    consume(TokenType::ENUM, "Expected 'enum'");
    Token name = consume(TokenType::IDENTIFIER, "Expected enum name");
    consume(TokenType::LBRACE, "Expected '{' after enum name");

    EnumDecl* decl = new EnumDecl(name.lexeme);
    decl->line = ln;

    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        Token val_name = consume(TokenType::IDENTIFIER, "Expected enum value name");
        if (match(TokenType::ASSIGN)) {
            Token val = consume(TokenType::INTEGER, "Expected integer value");
            int v = 0;
            for (size_t i = 0; i < val.lexeme.length(); i++) {
                v = v * 10 + (val.lexeme[i] - '0');
            }
            decl->values.push(new EnumValue(val_name.lexeme, v));
        } else {
            decl->values.push(new EnumValue(val_name.lexeme));
        }
        if (!check(TokenType::RBRACE)) {
            consume(TokenType::COMMA, "Expected ',' between enum values");
        }
    }

    consume(TokenType::RBRACE, "Expected '}' after enum body");
    return decl;
}

UnionDecl* Parser::parse_union_decl() {
    int ln = current_token().line;
    consume(TokenType::UNION, "Expected 'union'");
    Token name = consume(TokenType::IDENTIFIER, "Expected union name");
    consume(TokenType::LBRACE, "Expected '{' after union name");

    UnionDecl* decl = new UnionDecl(name.lexeme);
    decl->line = ln;

    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        Token field_name = consume(TokenType::IDENTIFIER, "Expected field name");
        consume(TokenType::COLON, "Expected ':' after field name");
        Token type = parse_type();
        consume(TokenType::SEMICOLON, "Expected ';' after union field");
        decl->fields.push(new UnionField(type.lexeme, field_name.lexeme));
    }

    consume(TokenType::RBRACE, "Expected '}' after union body");
    return decl;
}

StmtNode* Parser::parse_expr_stmt() {
    int ln = current_token().line;
    ExprNode* expr = parse_expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression");
    ExprStmt* stmt = new ExprStmt(expr);
    stmt->line = ln;
    return stmt;
}

ExprNode* Parser::parse_expression() {
    return parse_assignment();
}

ExprNode* Parser::parse_assignment() {
    ExprNode* expr = parse_logical_or();
    int ln = current_token().line;
    
    if (match(TokenType::ASSIGN)) {
        ExprNode* value = parse_assignment();
        AssignExpr* node = new AssignExpr(expr, value);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::PLUS_ASSIGN)) {
        ExprNode* value = parse_assignment();
        CompoundAssignExpr* node = new CompoundAssignExpr(expr, String("+"), value);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::MINUS_ASSIGN)) {
        ExprNode* value = parse_assignment();
        CompoundAssignExpr* node = new CompoundAssignExpr(expr, String("-"), value);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::STAR_ASSIGN)) {
        ExprNode* value = parse_assignment();
        CompoundAssignExpr* node = new CompoundAssignExpr(expr, String("*"), value);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::SLASH_ASSIGN)) {
        ExprNode* value = parse_assignment();
        CompoundAssignExpr* node = new CompoundAssignExpr(expr, String("/"), value);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::PERCENT_ASSIGN)) {
        ExprNode* value = parse_assignment();
        CompoundAssignExpr* node = new CompoundAssignExpr(expr, String("%"), value);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::AMPERSAND_ASSIGN)) {
        ExprNode* value = parse_assignment();
        CompoundAssignExpr* node = new CompoundAssignExpr(expr, String("&"), value);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::PIPE_ASSIGN)) {
        ExprNode* value = parse_assignment();
        CompoundAssignExpr* node = new CompoundAssignExpr(expr, String("|"), value);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::CARET_ASSIGN)) {
        ExprNode* value = parse_assignment();
        CompoundAssignExpr* node = new CompoundAssignExpr(expr, String("^"), value);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::LSHIFT_ASSIGN)) {
        ExprNode* value = parse_assignment();
        CompoundAssignExpr* node = new CompoundAssignExpr(expr, String("<<"), value);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::RSHIFT_ASSIGN)) {
        ExprNode* value = parse_assignment();
        CompoundAssignExpr* node = new CompoundAssignExpr(expr, String(">>"), value);
        node->line = ln;
        return node;
    }
    
    return expr;
}

ExprNode* Parser::parse_logical_or() {
    ExprNode* expr = parse_logical_and();
    
    while (match(TokenType::OR)) {
        int ln = _tokens[_current - 1].line;
        String op("||");
        ExprNode* right = parse_logical_and();
        BinaryExpr* node = new BinaryExpr(expr, op, right);
        node->line = ln;
        expr = node;
    }
    
    return expr;
}

ExprNode* Parser::parse_logical_and() {
    ExprNode* expr = parse_bitwise_or();
    
    while (match(TokenType::AND)) {
        int ln = _tokens[_current - 1].line;
        String op("&&");
        ExprNode* right = parse_bitwise_or();
        BinaryExpr* node = new BinaryExpr(expr, op, right);
        node->line = ln;
        expr = node;
    }
    
    return expr;
}

ExprNode* Parser::parse_equality() {
    ExprNode* expr = parse_comparison();
    
    while (match(TokenType::EQ) || match(TokenType::NEQ)) {
        int ln = _tokens[_current - 1].line;
        String op = _tokens[_current - 1].lexeme;
        ExprNode* right = parse_comparison();
        BinaryExpr* node = new BinaryExpr(expr, op, right);
        node->line = ln;
        expr = node;
    }
    
    return expr;
}

ExprNode* Parser::parse_comparison() {
    ExprNode* expr = parse_shift();
    
    while (match(TokenType::LT) || match(TokenType::GT) || 
           match(TokenType::LTE) || match(TokenType::GTE)) {
        int ln = _tokens[_current - 1].line;
        String op = _tokens[_current - 1].lexeme;
        ExprNode* right = parse_shift();
        BinaryExpr* node = new BinaryExpr(expr, op, right);
        node->line = ln;
        expr = node;
    }
    
    return expr;
}

ExprNode* Parser::parse_term() {
    ExprNode* expr = parse_factor();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        int ln = _tokens[_current - 1].line;
        String op = _tokens[_current - 1].lexeme;
        ExprNode* right = parse_factor();
        BinaryExpr* node = new BinaryExpr(expr, op, right);
        node->line = ln;
        expr = node;
    }
    
    return expr;
}

ExprNode* Parser::parse_factor() {
    ExprNode* expr = parse_unary();
    
    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        int ln = _tokens[_current - 1].line;
        String op = _tokens[_current - 1].lexeme;
        ExprNode* right = parse_unary();
        BinaryExpr* node = new BinaryExpr(expr, op, right);
        node->line = ln;
        expr = node;
    }
    
    return expr;
}

ExprNode* Parser::parse_unary() {
    if (match(TokenType::NOT) || match(TokenType::MINUS) || match(TokenType::TILDE)) {
        int ln = _tokens[_current - 1].line;
        String op = _tokens[_current - 1].lexeme;
        ExprNode* operand = parse_unary();
        UnaryExpr* node = new UnaryExpr(op, operand);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::INCREMENT)) {
        int ln = _tokens[_current - 1].line;
        ExprNode* operand = parse_unary();
        UnaryExpr* node = new UnaryExpr(String("++"), operand);
        node->line = ln;
        return node;
    }
    
    if (match(TokenType::DECREMENT)) {
        int ln = _tokens[_current - 1].line;
        ExprNode* operand = parse_unary();
        UnaryExpr* node = new UnaryExpr(String("--"), operand);
        node->line = ln;
        return node;
    }
    
    return parse_postfix();
}

ExprNode* Parser::parse_postfix() {
    ExprNode* expr = parse_primary();
    
    for (;;) {
        if (match(TokenType::DOT)) {
            int ln = _tokens[_current - 1].line;
            Token member = consume(TokenType::IDENTIFIER, "Expected member name");
            MemberExpr* node = new MemberExpr(expr, member.lexeme);
            node->line = ln;
            expr = node;
        } else if (check(TokenType::LBRACKET)) {
            int ln = current_token().line;
            advance();
            ExprNode* index = parse_expression();
            consume(TokenType::RBRACKET, "Expected ']' after index");
            IndexExpr* node = new IndexExpr(expr, index);
            node->line = ln;
            expr = node;
        } else if (match(TokenType::LPAREN)) {
            int ln = _tokens[_current - 1].line;
            CallExpr* call = new CallExpr(expr);
            call->line = ln;
            if (!check(TokenType::RPAREN)) {
                do {
                    call->arguments.push(parse_expression());
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RPAREN, "Expected ')' after arguments");
            expr = call;
        } else if (check(TokenType::INCREMENT)) {
            int ln = current_token().line;
            advance();
            PostfixExpr* node = new PostfixExpr(expr, String("++"));
            node->line = ln;
            expr = node;
        } else if (check(TokenType::DECREMENT)) {
            int ln = current_token().line;
            advance();
            PostfixExpr* node = new PostfixExpr(expr, String("--"));
            node->line = ln;
            expr = node;
        } else {
            break;
        }
    }
    
    return expr;
}

ExprNode* Parser::parse_primary() {
    int ln = current_token().line;
    
    if (match(TokenType::TRUE)) {
        BoolLiteral* node = new BoolLiteral(true);
        node->line = ln;
        return node;
    }
    if (match(TokenType::FALSE)) {
        BoolLiteral* node = new BoolLiteral(false);
        node->line = ln;
        return node;
    }
    if (match(TokenType::NULL_LIT)) {
        NullLiteral* node = new NullLiteral();
        node->line = ln;
        return node;
    }
    if (match(TokenType::CAST)) {
        consume(TokenType::LPAREN, "Expected '(' after 'cast'");
        ExprNode* expr = parse_expression();
        consume(TokenType::COMMA, "Expected ',' after cast expression");
        Token target = parse_type();
        consume(TokenType::RPAREN, "Expected ')' after cast type");
        CastExpr* node = new CastExpr(expr, target.lexeme);
        node->line = ln;
        return node;
    }
    if (match(TokenType::SIZEOF)) {
        consume(TokenType::LPAREN, "Expected '(' after 'sizeof'");
        Token target = parse_type();
        consume(TokenType::RPAREN, "Expected ')' after sizeof type");
        SizeofExpr* node = new SizeofExpr(target.lexeme);
        node->line = ln;
        return node;
    }
    if (match(TokenType::INTEGER)) {
        Token token = _tokens[_current - 1];
        int value = 0;
        for (size_t i = 0; i < token.lexeme.length(); i++) {
            value = value * 10 + (token.lexeme[i] - '0');
        }
        IntegerLiteral* node = new IntegerLiteral(value);
        node->line = ln;
        return node;
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
        
        FloatLiteral* node = new FloatLiteral(value);
        node->line = ln;
        return node;
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
        
        DoubleLiteral* node = new DoubleLiteral(value);
        node->line = ln;
        return node;
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
        StringLiteral* node = new StringLiteral(result);
        node->line = ln;
        return node;
    }
    if (match(TokenType::IDENTIFIER)) {
        IdentifierExpr* node = new IdentifierExpr(_tokens[_current - 1].lexeme);
        node->line = ln;
        return node;
    }
    if (match(TokenType::THIS)) {
        ThisExpr* node = new ThisExpr();
        node->line = ln;
        return node;
    }
    if (match(TokenType::LPAREN)) {
        ExprNode* expr = parse_expression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    if (match(TokenType::LBRACKET)) {
        ArrayExpr* array = new ArrayExpr();
        array->line = ln;
        
        if (!check(TokenType::RBRACKET)) {
            do {
                array->elements.push(parse_expression());
            } while (match(TokenType::COMMA));
        }
        
        consume(TokenType::RBRACKET, "Expected ']' after array elements");
        return array;
    }
    
    fprintf(stderr, "Parse error at line %d: Unexpected token in expression\n", ln);
    exit(1);
}

ExprNode* Parser::parse_bitwise_or() {
    ExprNode* expr = parse_bitwise_xor();
    
    while (match(TokenType::PIPE)) {
        int ln = _tokens[_current - 1].line;
        ExprNode* right = parse_bitwise_xor();
        BinaryExpr* node = new BinaryExpr(expr, String("|"), right);
        node->line = ln;
        expr = node;
    }
    
    return expr;
}

ExprNode* Parser::parse_bitwise_xor() {
    ExprNode* expr = parse_bitwise_and();
    
    while (match(TokenType::CARET)) {
        int ln = _tokens[_current - 1].line;
        ExprNode* right = parse_bitwise_and();
        BinaryExpr* node = new BinaryExpr(expr, String("^"), right);
        node->line = ln;
        expr = node;
    }
    
    return expr;
}

ExprNode* Parser::parse_bitwise_and() {
    ExprNode* expr = parse_equality();
    
    while (match(TokenType::AMPERSAND)) {
        int ln = _tokens[_current - 1].line;
        ExprNode* right = parse_equality();
        BinaryExpr* node = new BinaryExpr(expr, String("&"), right);
        node->line = ln;
        expr = node;
    }
    
    return expr;
}

ExprNode* Parser::parse_shift() {
    ExprNode* expr = parse_term();
    
    while (match(TokenType::LSHIFT) || match(TokenType::RSHIFT)) {
        int ln = _tokens[_current - 1].line;
        String op = _tokens[_current - 1].lexeme;
        ExprNode* right = parse_term();
        BinaryExpr* node = new BinaryExpr(expr, op, right);
        node->line = ln;
        expr = node;
    }
    
    return expr;
}

ExternFuncDecl* Parser::parse_extern_func_decl() {
    int ln = current_token().line;
    consume(TokenType::EXTERN, "Expected 'extern'");
    consume(TokenType::FUNC, "Expected 'func' after 'extern'");
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    consume(TokenType::LPAREN, "Expected '(' after function name");
    
    ExternFuncDecl* decl = new ExternFuncDecl(String(""), name.lexeme);
    decl->line = ln;
    
    if (!check(TokenType::RPAREN)) {
        do {
            Token param_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            consume(TokenType::COLON, "Expected ':' after parameter name");
            Token param_type = parse_type();
            decl->parameters.push(new Parameter(param_type.lexeme, param_name.lexeme));
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    consume(TokenType::COLON, "Expected ':' after parameters");
    Token return_type = parse_type();
    decl->return_type = return_type.lexeme;
    consume(TokenType::SEMICOLON, "Expected ';' after extern function declaration");
    
    return decl;
}

void Parser::parse_interface_decl(Program* program) {
    int ln = current_token().line;
    consume(TokenType::INTERFACE, "Expected 'interface'");
    Token name = consume(TokenType::IDENTIFIER, "Expected interface name");
    consume(TokenType::LBRACE, "Expected '{' after interface name");
    
    InterfaceDecl* iface = new InterfaceDecl(name.lexeme);
    iface->line = ln;
    
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        consume(TokenType::FUNC, "Expected 'func' in interface body");
        Token method_name = consume(TokenType::IDENTIFIER, "Expected method name");
        consume(TokenType::LPAREN, "Expected '(' after method name");
        
        InterfaceMethod* method = new InterfaceMethod(method_name.lexeme, String(""));
        
        if (!check(TokenType::RPAREN)) {
            do {
                Token param_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
                consume(TokenType::COLON, "Expected ':' after parameter name");
                Token param_type = parse_type();
                method->parameters.push(new Parameter(param_type.lexeme, param_name.lexeme));
            } while (match(TokenType::COMMA));
        }
        
        consume(TokenType::RPAREN, "Expected ')' after parameters");
        consume(TokenType::COLON, "Expected ':' after parameters");
        Token return_type = parse_type();
        method->return_type = return_type.lexeme;
        consume(TokenType::SEMICOLON, "Expected ';' after interface method");
        
        iface->methods.push(method);
    }
    
    consume(TokenType::RBRACE, "Expected '}' after interface body");
    program->interfaces.push(iface);
}

StmtNode* Parser::parse_try_catch_stmt() {
    int ln = current_token().line;
    consume(TokenType::TRY, "Expected 'try'");
    BlockStmt* try_body = parse_block();
    
    consume(TokenType::CATCH, "Expected 'catch' after try block");
    consume(TokenType::LPAREN, "Expected '(' after 'catch'");
    Token catch_var = consume(TokenType::IDENTIFIER, "Expected catch variable name");
    consume(TokenType::COLON, "Expected ':' after catch variable");
    Token catch_type = parse_type();
    consume(TokenType::RPAREN, "Expected ')' after catch type");
    BlockStmt* catch_body = parse_block();
    
    TryCatchStmt* stmt = new TryCatchStmt(try_body, catch_var.lexeme, catch_type.lexeme, catch_body);
    stmt->line = ln;
    return stmt;
}

StmtNode* Parser::parse_throw_stmt() {
    int ln = current_token().line;
    consume(TokenType::THROW, "Expected 'throw'");
    ExprNode* value = parse_expression();
    consume(TokenType::SEMICOLON, "Expected ';' after throw expression");
    ThrowStmt* stmt = new ThrowStmt(value);
    stmt->line = ln;
    return stmt;
}

void Parser::add_define(const char* name) {
    _defines.push(String(name));
}

bool Parser::has_define(const char* name) const {
    for (size_t i = 0; i < _defines.size(); i++) {
        if (_defines[i] == name) return true;
    }
    return false;
}

void Parser::skip_to_matching_brace() {
    int depth = 1;
    while (!check(TokenType::END_OF_FILE)) {
        if (check(TokenType::LBRACE)) depth++;
        else if (check(TokenType::RBRACE)) {
            depth--;
            if (depth == 0) { advance(); return; }
        }
        advance();
    }
}

bool Parser::parse_top_level_decl(Program* program) {
    if (check(TokenType::IMPORT) || check(TokenType::FROM)) {
        program->imports.push(parse_import_decl());
    } else if (check(TokenType::VAR) || check(TokenType::CONST)) {
        program->globals.push((VarDecl*)parse_var_decl());
    } else if (check(TokenType::EVENT)) {
        program->events.push(parse_event_decl());
    } else if (check(TokenType::SIGNAL)) {
        program->signals.push(parse_signal_decl());
    } else if (check(TokenType::DATACLASS)) {
        advance();
        parse_class_decl(program, true);
    } else if (check(TokenType::AT)) {
        if (peek_token(1).type == TokenType::IF) {
            parse_conditional_compile(program);
        } else {
            program->processes.push(parse_process_decl());
        }
    } else if (check(TokenType::CLASS)) {
        parse_class_decl(program);
    } else if (check(TokenType::FUNC)) {
        program->functions.push(parse_function_decl());
    } else if (check(TokenType::ENUM)) {
        program->enums.push(parse_enum_decl());
    } else if (check(TokenType::UNION)) {
        program->unions.push(parse_union_decl());
    } else if (check(TokenType::INTERFACE)) {
        parse_interface_decl(program);
    } else if (check(TokenType::EXTERN)) {
        program->extern_functions.push(parse_extern_func_decl());
    } else if (check(TokenType::LINK)) {
        advance();
        Token flag = consume(TokenType::STRING, "Expected string after 'link'");
        consume(TokenType::SEMICOLON, "Expected ';' after link directive");
        program->link_flags.push(flag.lexeme);
    } else {
        return false;
    }
    return true;
}

void Parser::parse_conditional_compile(Program* program) {
    consume(TokenType::AT, "Expected '@'");
    consume(TokenType::IF, "Expected 'if' after '@'");
    consume(TokenType::LPAREN, "Expected '(' after '@if'");
    Token define_name = consume(TokenType::IDENTIFIER, "Expected define name");
    consume(TokenType::RPAREN, "Expected ')' after define name");
    consume(TokenType::LBRACE, "Expected '{' after '@if(...)'");

    bool active = has_define(define_name.lexeme.c_str());

    if (active) {
        while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
            if (!parse_top_level_decl(program)) {
                fprintf(stderr, "Parse error at line %d: Unexpected token inside @if block\n",
                        current_token().line);
                exit(1);
            }
        }
        consume(TokenType::RBRACE, "Expected '}' to close @if block");
    } else {
        skip_to_matching_brace();
    }

    if (check(TokenType::AT) && peek_token(1).type == TokenType::ELSE) {
        consume(TokenType::AT, "Expected '@'");
        consume(TokenType::ELSE, "Expected 'else' after '@'");
        consume(TokenType::LBRACE, "Expected '{' after '@else'");

        if (!active) {
            while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
                if (!parse_top_level_decl(program)) {
                    fprintf(stderr, "Parse error at line %d: Unexpected token inside @else block\n",
                            current_token().line);
                    exit(1);
                }
            }
            consume(TokenType::RBRACE, "Expected '}' to close @else block");
        } else {
            skip_to_matching_brace();
        }
    }
}

}
