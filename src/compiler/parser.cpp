#include "parser.h"
#include <cstdio>
#include <cstdlib>

namespace Tick {

Parser::Parser(const DynamicArray<Token>& tokens)
    : _tokens(tokens), _current(0), _no_struct_lit(false) {}

const Token& Parser::cur() const { return _tokens[_current]; }

const Token& Parser::peek(int offset) const {
    size_t i = _current + offset;
    if (i < _tokens.size()) return _tokens[i];
    return _tokens[_tokens.size() - 1];
}

bool Parser::check(TokenType t) const { return cur().type == t; }

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

void Parser::advance() {
    if (cur().type != TokenType::END_OF_FILE) _current++;
}

Token Parser::consume(TokenType t, const char* message) {
    if (check(t)) {
        Token tok = cur();
        advance();
        return tok;
    }
    fail(message);
}

void Parser::fail(const char* message) const {
    fprintf(stderr, "Parse error at line %d: %s (found '%s')\n",
            cur().line, message, cur().lexeme.c_str());
    exit(1);
}

// ---- types ----

TypeRef* Parser::parse_base_type() {
    TokenType t = cur().type;
    switch (t) {
        case TokenType::VOID: advance(); return new TypeRef(TypeKind::VOID);
        case TokenType::BOOL: advance(); return new TypeRef(TypeKind::BOOL);
        case TokenType::STR:  advance(); return new TypeRef(TypeKind::STR);
        case TokenType::I8: case TokenType::I16: case TokenType::I32: case TokenType::I64:
        case TokenType::U8: case TokenType::U16: case TokenType::U32: case TokenType::U64: {
            TypeRef* tr = new TypeRef(TypeKind::INT);
            const char* lx = cur().lexeme.c_str();
            tr->int_unsigned = (lx[0] == 'u');
            tr->int_bits = atoi(lx + 1);
            advance();
            return tr;
        }
        case TokenType::F32: case TokenType::F64: {
            TypeRef* tr = new TypeRef(TypeKind::FLOAT);
            tr->float_bits = atoi(cur().lexeme.c_str() + 1);
            advance();
            return tr;
        }
        case TokenType::PTR: {
            advance();
            TypeRef* tr = new TypeRef(TypeKind::PTR);
            if (match(TokenType::LANGLE)) {
                tr->inner = parse_type();
                consume(TokenType::RANGLE, "Expected '>' after ptr<T>");
            }
            return tr;
        }
        case TokenType::DYN: {
            advance();
            TypeRef* tr = new TypeRef(TypeKind::DYN);
            tr->name = consume(TokenType::IDENTIFIER, "Expected interface name after 'dyn'").lexeme;
            return tr;
        }
        case TokenType::IDENTIFIER: {
            TypeRef* tr = new TypeRef(TypeKind::NAMED);
            tr->name = cur().lexeme;
            advance();
            return tr;
        }
        default:
            fail("Expected a type");
    }
}

Param* Parser::parse_param() {
    // optional `ref` / `ref var` / `shared` / `weak` qualifier precedes the name
    Ownership own = Ownership::VALUE;
    bool ref_mut = false;
    if (match(TokenType::REF)) {
        own = Ownership::REF;
        if (match(TokenType::VAR)) ref_mut = true;
    } else if (match(TokenType::SHARED)) {
        own = Ownership::SHARED;
    } else if (match(TokenType::WEAK)) {
        own = Ownership::WEAK;
    }
    Param* p = new Param();
    p->name = consume(TokenType::IDENTIFIER, "Expected parameter name").lexeme;
    consume(TokenType::COLON, "Expected ':' after parameter name");
    p->type = parse_type();
    if (own != Ownership::VALUE) {
        p->type->ownership = own;
        p->type->ref_mutable = ref_mut;
    }
    return p;
}

TypeRef* Parser::parse_type() {
    // ownership prefix
    Ownership own = Ownership::VALUE;
    bool ref_mut = false;
    if (match(TokenType::REF)) {
        own = Ownership::REF;
        if (match(TokenType::VAR)) ref_mut = true;
    } else if (match(TokenType::SHARED)) {
        own = Ownership::SHARED;
    } else if (match(TokenType::WEAK)) {
        own = Ownership::WEAK;
    }

    // const qualifier: `const T` — immutable binding, zero runtime cost
    bool is_const = match(TokenType::CONST);

    TypeRef* base = parse_base_type();

    // suffixes: T[], T[N], T?
    for (;;) {
        if (check(TokenType::LBRACKET)) {
            advance();
            if (check(TokenType::INT_LITERAL)) {
                TypeRef* arr = new TypeRef(TypeKind::FIXED_ARRAY);
                arr->fixed_size = atoi(cur().lexeme.c_str());
                advance();
                consume(TokenType::RBRACKET, "Expected ']' after fixed array size");
                arr->inner = base;
                base = arr;
            } else {
                consume(TokenType::RBRACKET, "Expected ']' for dynamic array");
                TypeRef* arr = new TypeRef(TypeKind::ARRAY);
                arr->inner = base;
                base = arr;
            }
        } else {
            break;
        }
    }

    base->ownership = own;
    base->ref_mutable = ref_mut;
    base->is_const = is_const;
    return base;
}

// ---- top level ----

Program* Parser::parse() {
    Program* program = new Program();
    while (!check(TokenType::END_OF_FILE)) {
        parse_top_level(program);
    }
    return program;
}

void Parser::parse_top_level(Program* program) {
    bool is_pub = match(TokenType::PUB);

    switch (cur().type) {
        case TokenType::FUNC:
            program->functions.push(parse_func(is_pub, false));
            break;
        case TokenType::STRUCT:
            program->structs.push(parse_struct(is_pub));
            break;
        case TokenType::ENUM:
            program->enums.push(parse_enum(is_pub));
            break;
        case TokenType::INTERFACE:
            program->interfaces.push(parse_interface(is_pub));
            break;
        case TokenType::IMPL:
            if (is_pub) fail("'impl' cannot be 'pub'");
            program->impls.push(parse_impl());
            break;
        case TokenType::EXTERN:
            if (is_pub) fail("'extern' cannot be 'pub'");
            program->externs.push(parse_extern());
            break;
        case TokenType::SIGNAL:
            if (is_pub) fail("'signal' cannot be 'pub'");
            program->signals.push(parse_signal());
            break;
        case TokenType::EVENT:
            if (is_pub) fail("'event' cannot be 'pub'");
            program->events.push(parse_event());
            break;
        case TokenType::PROCESS:
            if (is_pub) fail("'process' cannot be 'pub'");
            program->processes.push(parse_process());
            break;
        case TokenType::VAR:
            advance();
            program->globals.push(parse_global(false));
            break;
        case TokenType::LINK: {
            if (is_pub) fail("'link' cannot be 'pub'");
            advance();
            Token flag = consume(TokenType::STRING_LITERAL, "Expected string after 'link'");
            program->link_flags.push(flag.lexeme);
            break;
        }
        default:
            fail("Expected a top-level declaration");
    }
}

FuncDecl* Parser::parse_func(bool is_pub, bool allow_self) {
    int ln = cur().line;
    consume(TokenType::FUNC, "Expected 'func'");
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    FuncDecl* fn = new FuncDecl(name.lexeme);
    fn->is_pub = is_pub;
    fn->line = ln;

    consume(TokenType::LPAREN, "Expected '(' after function name");

    // optional self receiver as first parameter
    if (allow_self && (check(TokenType::SELF) || check(TokenType::REF))) {
        if (check(TokenType::REF)) {
            advance();
            bool mut = match(TokenType::VAR);
            consume(TokenType::SELF, "Expected 'self' after 'ref'");
            fn->self_kind = mut ? SelfKind::REF_MUT : SelfKind::REF;
        } else {
            advance(); // self
            fn->self_kind = SelfKind::VALUE;
        }
        if (!check(TokenType::RPAREN)) consume(TokenType::COMMA, "Expected ',' after self");
    }

    if (!check(TokenType::RPAREN)) {
        do {
            fn->params.push(parse_param());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after parameters");

    if (match(TokenType::COLON)) {
        fn->return_type = parse_type();
    } else {
        fn->return_type = new TypeRef(TypeKind::VOID);
    }

    if (check(TokenType::LBRACE)) {
        fn->body = parse_block();
    } else {
        consume(TokenType::SEMICOLON, "Expected '{' or ';' after function signature");
    }
    return fn;
}

StructDecl* Parser::parse_struct(bool is_pub) {
    int ln = cur().line;
    consume(TokenType::STRUCT, "Expected 'struct'");
    StructDecl* st = new StructDecl(consume(TokenType::IDENTIFIER, "Expected struct name").lexeme);
    st->is_pub = is_pub;
    st->line = ln;
    consume(TokenType::LBRACE, "Expected '{' after struct name");
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        FieldDecl* f = new FieldDecl();
        f->name = consume(TokenType::IDENTIFIER, "Expected field name").lexeme;
        consume(TokenType::COLON, "Expected ':' after field name");
        f->type = parse_type();
        match(TokenType::COMMA);
        st->fields.push(f);
    }
    consume(TokenType::RBRACE, "Expected '}' after struct body");
    return st;
}

EnumDecl* Parser::parse_enum(bool is_pub) {
    int ln = cur().line;
    consume(TokenType::ENUM, "Expected 'enum'");
    EnumDecl* en = new EnumDecl(consume(TokenType::IDENTIFIER, "Expected enum name").lexeme);
    en->is_pub = is_pub;
    en->line = ln;
    consume(TokenType::LBRACE, "Expected '{' after enum name");
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        EnumVariant* v = new EnumVariant();
        v->name = consume(TokenType::IDENTIFIER, "Expected variant name").lexeme;
        if (match(TokenType::ASSIGN)) {
            v->has_int_value = true;
            v->int_value = atoll(consume(TokenType::INT_LITERAL, "Expected integer value").lexeme.c_str());
        }
        match(TokenType::COMMA);
        en->variants.push(v);
    }
    consume(TokenType::RBRACE, "Expected '}' after enum body");
    return en;
}

InterfaceDecl* Parser::parse_interface(bool is_pub) {
    int ln = cur().line;
    consume(TokenType::INTERFACE, "Expected 'interface'");
    InterfaceDecl* it = new InterfaceDecl(consume(TokenType::IDENTIFIER, "Expected interface name").lexeme);
    it->is_pub = is_pub;
    it->line = ln;
    consume(TokenType::LBRACE, "Expected '{' after interface name");
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        it->methods.push(parse_func(false, true));
    }
    consume(TokenType::RBRACE, "Expected '}' after interface body");
    return it;
}

ImplDecl* Parser::parse_impl() {
    int ln = cur().line;
    consume(TokenType::IMPL, "Expected 'impl'");
    String first = consume(TokenType::IDENTIFIER, "Expected type or interface name").lexeme;
    ImplDecl* impl;
    // syntax: `impl T { ... }`  or  `impl I for T { ... }`
    if (check(TokenType::FOR)) {
        advance();
        String type_name = consume(TokenType::IDENTIFIER, "Expected type name after 'for'").lexeme;
        impl = new ImplDecl(type_name);
        impl->interface_name = first;
    } else {
        impl = new ImplDecl(first);
    }
    impl->line = ln;
    consume(TokenType::LBRACE, "Expected '{' after impl header");
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        FuncDecl* m = parse_func(false, true);
        m->impl_type = impl->type_name;
        impl->methods.push(m);
    }
    consume(TokenType::RBRACE, "Expected '}' after impl body");
    return impl;
}

ExternDecl* Parser::parse_extern() {
    int ln = cur().line;
    consume(TokenType::EXTERN, "Expected 'extern'");
    consume(TokenType::FUNC, "Expected 'func' after 'extern'");
    ExternDecl* ex = new ExternDecl(consume(TokenType::IDENTIFIER, "Expected function name").lexeme);
    ex->line = ln;
    consume(TokenType::LPAREN, "Expected '(' after function name");
    if (!check(TokenType::RPAREN)) {
        do {
            ex->params.push(parse_param());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    if (match(TokenType::COLON)) ex->return_type = parse_type();
    else ex->return_type = new TypeRef(TypeKind::VOID);
    consume(TokenType::SEMICOLON, "Expected ';' after extern declaration");
    return ex;
}

SignalDecl* Parser::parse_signal() {
    int ln = cur().line;
    consume(TokenType::SIGNAL, "Expected 'signal'");
    SignalDecl* sg = new SignalDecl(consume(TokenType::IDENTIFIER, "Expected signal name").lexeme);
    sg->line = ln;
    consume(TokenType::COLON, "Expected ':' after signal name");
    sg->payload_type = parse_type();
    consume(TokenType::SEMICOLON, "Expected ';' after signal declaration");
    return sg;
}

EventDecl* Parser::parse_event() {
    int ln = cur().line;
    consume(TokenType::EVENT, "Expected 'event'");
    EventDecl* ev = new EventDecl(consume(TokenType::IDENTIFIER, "Expected event name").lexeme);
    ev->line = ln;
    consume(TokenType::SEMICOLON, "Expected ';' after event declaration");
    return ev;
}

ProcessDecl* Parser::parse_process() {
    int ln = cur().line;
    consume(TokenType::PROCESS, "Expected 'process'");
    String name = consume(TokenType::IDENTIFIER, "Expected process name").lexeme;
    consume(TokenType::ON, "Expected 'on' after process name");
    String event = consume(TokenType::IDENTIFIER, "Expected event name after 'on'").lexeme;
    ProcessDecl* pr = new ProcessDecl(name, event);
    pr->line = ln;
    pr->body = parse_block();
    return pr;
}

GlobalDecl* Parser::parse_global(bool is_const) {
    int ln = cur().line;
    GlobalDecl* g = new GlobalDecl(is_const, consume(TokenType::IDENTIFIER, "Expected global name").lexeme);
    g->line = ln;
    if (match(TokenType::COLON)) {
        g->type = parse_type();
        if (g->type->is_const) g->is_const = true;
    }
    consume(TokenType::ASSIGN, "Global must be initialized");
    g->init = parse_expression();
    consume(TokenType::SEMICOLON, "Expected ';' after global declaration");
    return g;
}

// ---- statements ----

Block* Parser::parse_block() {
    int ln = cur().line;
    consume(TokenType::LBRACE, "Expected '{'");
    Block* block = new Block();
    block->line = ln;
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        block->statements.push(parse_statement());
    }
    consume(TokenType::RBRACE, "Expected '}'");
    return block;
}

Node* Parser::parse_statement() {
    switch (cur().type) {
        case TokenType::VAR: advance(); return parse_var_decl(false);
        case TokenType::IF: return parse_if();
        case TokenType::WHILE: return parse_while();
        case TokenType::FOR: return parse_for();
        case TokenType::RETURN: return parse_return();
        case TokenType::DEFER: return parse_defer();
        case TokenType::UNSAFE: return parse_unsafe();
        case TokenType::MATCH: {
            int ln = cur().line;
            Node* m = parse_match();
            ExprStmt* s = new ExprStmt(m);
            s->line = ln;
            return s;
        }
        case TokenType::BREAK: { advance(); consume(TokenType::SEMICOLON, "Expected ';' after break"); return new BreakStmt(); }
        case TokenType::CONTINUE: { advance(); consume(TokenType::SEMICOLON, "Expected ';' after continue"); return new ContinueStmt(); }
        case TokenType::LBRACE: return parse_block();
        default: {
            int ln = cur().line;
            Node* e = parse_expression();
            consume(TokenType::SEMICOLON, "Expected ';' after expression");
            ExprStmt* s = new ExprStmt(e);
            s->line = ln;
            return s;
        }
    }
}

// parse a var binding; the `var` keyword is already consumed.
// is_const is derived from `var x : const T` — set when the declared_type has is_const.
VarDecl* Parser::parse_var_decl(bool is_const) {
    int ln = cur().line;
    VarDecl* d = new VarDecl(is_const, consume(TokenType::IDENTIFIER, "Expected binding name").lexeme);
    d->line = ln;
    if (match(TokenType::COLON)) {
        d->declared_type = parse_type();
        if (d->declared_type->is_const) d->is_const = true;
    }
    if (match(TokenType::ASSIGN)) d->init = parse_expression();
    else if (d->is_const) fail("'const' binding must be initialized");
    consume(TokenType::SEMICOLON, "Expected ';' after binding");
    return d;
}

Node* Parser::parse_if() {
    int ln = cur().line;
    consume(TokenType::IF, "Expected 'if'");
    _no_struct_lit = true;
    Node* cond = parse_expression();
    _no_struct_lit = false;
    Block* then_branch = parse_block();
    Node* else_branch = nullptr;
    if (match(TokenType::ELSE)) {
        if (check(TokenType::IF)) else_branch = parse_if();
        else else_branch = parse_block();
    }
    IfStmt* s = new IfStmt(cond, then_branch, else_branch);
    s->line = ln;
    return s;
}

Node* Parser::parse_while() {
    int ln = cur().line;
    consume(TokenType::WHILE, "Expected 'while'");
    _no_struct_lit = true;
    Node* cond = parse_expression();
    _no_struct_lit = false;
    Block* body = parse_block();
    WhileStmt* s = new WhileStmt(cond, body);
    s->line = ln;
    return s;
}

Node* Parser::parse_for() {
    int ln = cur().line;
    consume(TokenType::FOR, "Expected 'for'");
    bool bind_ref = match(TokenType::REF);
    String var = consume(TokenType::IDENTIFIER, "Expected loop variable").lexeme;
    consume(TokenType::IN, "Expected 'in' after loop variable");
    ForStmt* s = new ForStmt(var);
    s->bind_ref = bind_ref;
    s->line = ln;
    _no_struct_lit = true;
    s->iterable = parse_expression();
    _no_struct_lit = false;
    s->body = parse_block();
    return s;
}

Node* Parser::parse_return() {
    int ln = cur().line;
    consume(TokenType::RETURN, "Expected 'return'");
    Node* value = nullptr;
    if (!check(TokenType::SEMICOLON)) value = parse_expression();
    consume(TokenType::SEMICOLON, "Expected ';' after return");
    ReturnStmt* s = new ReturnStmt(value);
    s->line = ln;
    return s;
}

Node* Parser::parse_defer() {
    int ln = cur().line;
    consume(TokenType::DEFER, "Expected 'defer'");
    Node* inner = parse_statement();
    DeferStmt* s = new DeferStmt(inner);
    s->line = ln;
    return s;
}

Node* Parser::parse_unsafe() {
    int ln = cur().line;
    consume(TokenType::UNSAFE, "Expected 'unsafe'");
    UnsafeBlock* s = new UnsafeBlock(parse_block());
    s->line = ln;
    return s;
}

// ---- expressions ----

Node* Parser::parse_expression() { return parse_assignment(); }

Node* Parser::parse_assignment() {
    Node* left = parse_range();
    int ln = cur().line;

    if (match(TokenType::ASSIGN)) {
        Node* value = parse_assignment();
        Assign* a = new Assign(left, value);
        a->line = ln;
        return a;
    }

    static const struct { TokenType tok; const char* op; } compounds[] = {
        {TokenType::PLUS_ASSIGN, "+"}, {TokenType::MINUS_ASSIGN, "-"},
        {TokenType::STAR_ASSIGN, "*"}, {TokenType::SLASH_ASSIGN, "/"},
        {TokenType::PERCENT_ASSIGN, "%"}, {TokenType::AMPERSAND_ASSIGN, "&"},
        {TokenType::PIPE_ASSIGN, "|"}, {TokenType::CARET_ASSIGN, "^"},
        {TokenType::LSHIFT_ASSIGN, "<<"}, {TokenType::RSHIFT_ASSIGN, ">>"},
    };
    for (const auto& c : compounds) {
        if (match(c.tok)) {
            Node* value = parse_assignment();
            CompoundAssign* a = new CompoundAssign(left, String(c.op), value);
            a->line = ln;
            return a;
        }
    }
    return left;
}

Node* Parser::parse_range() {
    Node* left = parse_or();
    if (match(TokenType::DOTDOT)) {
        int ln = cur().line;
        Node* right = parse_or();
        RangeExpr* r = new RangeExpr(left, right);
        r->line = ln;
        return r;
    }
    return left;
}

Node* Parser::parse_or() {
    Node* left = parse_and();
    while (check(TokenType::OR)) {
        int ln = cur().line; advance();
        Node* right = parse_and();
        left = new Binary(left, String("||"), right);
        left->line = ln;
    }
    return left;
}

Node* Parser::parse_and() {
    Node* left = parse_bit_or();
    while (check(TokenType::AND)) {
        int ln = cur().line; advance();
        Node* right = parse_bit_or();
        left = new Binary(left, String("&&"), right);
        left->line = ln;
    }
    return left;
}

Node* Parser::parse_bit_or() {
    Node* left = parse_bit_xor();
    while (check(TokenType::PIPE)) {
        int ln = cur().line; advance();
        Node* right = parse_bit_xor();
        left = new Binary(left, String("|"), right);
        left->line = ln;
    }
    return left;
}

Node* Parser::parse_bit_xor() {
    Node* left = parse_bit_and();
    while (check(TokenType::CARET)) {
        int ln = cur().line; advance();
        Node* right = parse_bit_and();
        left = new Binary(left, String("^"), right);
        left->line = ln;
    }
    return left;
}

Node* Parser::parse_bit_and() {
    Node* left = parse_equality();
    while (check(TokenType::AMPERSAND)) {
        int ln = cur().line; advance();
        Node* right = parse_equality();
        left = new Binary(left, String("&"), right);
        left->line = ln;
    }
    return left;
}

Node* Parser::parse_equality() {
    Node* left = parse_comparison();
    while (check(TokenType::EQ) || check(TokenType::NEQ)) {
        String op = cur().lexeme; int ln = cur().line; advance();
        Node* right = parse_comparison();
        left = new Binary(left, op, right);
        left->line = ln;
    }
    return left;
}

Node* Parser::parse_comparison() {
    Node* left = parse_shift();
    while (check(TokenType::LANGLE) || check(TokenType::RANGLE) ||
           check(TokenType::LTE) || check(TokenType::GTE)) {
        String op = cur().lexeme; int ln = cur().line; advance();
        Node* right = parse_shift();
        left = new Binary(left, op, right);
        left->line = ln;
    }
    return left;
}

Node* Parser::parse_shift() {
    Node* left = parse_term();
    while (check(TokenType::LSHIFT) || check(TokenType::RSHIFT)) {
        String op = cur().lexeme; int ln = cur().line; advance();
        Node* right = parse_term();
        left = new Binary(left, op, right);
        left->line = ln;
    }
    return left;
}

Node* Parser::parse_term() {
    Node* left = parse_factor();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        String op = cur().lexeme; int ln = cur().line; advance();
        Node* right = parse_factor();
        left = new Binary(left, op, right);
        left->line = ln;
    }
    return left;
}

Node* Parser::parse_factor() {
    Node* left = parse_unary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        String op = cur().lexeme; int ln = cur().line; advance();
        Node* right = parse_unary();
        left = new Binary(left, op, right);
        left->line = ln;
    }
    return left;
}

Node* Parser::parse_unary() {
    if (check(TokenType::BANG) || check(TokenType::NOT) ||
        check(TokenType::MINUS) || check(TokenType::TILDE)) {
        String op = cur().lexeme; int ln = cur().line; advance();
        Node* operand = parse_unary();
        // BANG as prefix means logical not
        Unary* u = new Unary(op == "!" ? String("!") : op, operand);
        u->line = ln;
        return u;
    }
    if (check(TokenType::REF)) {
        int ln = cur().line; advance();
        bool mut = match(TokenType::VAR);
        Node* operand = parse_unary();
        RefExpr* r = new RefExpr(operand, mut);
        r->line = ln;
        return r;
    }
    if (check(TokenType::SHARED)) {
        int ln = cur().line; advance();
        Node* operand = parse_unary();
        SharedExpr* s = new SharedExpr(operand);
        s->line = ln;
        return s;
    }
    if (check(TokenType::WEAK)) {
        int ln = cur().line; advance();
        Node* operand = parse_unary();
        WeakExpr* w = new WeakExpr(operand);
        w->line = ln;
        return w;
    }
    return parse_postfix();
}

DynamicArray<Arg> Parser::parse_args() {
    DynamicArray<Arg> args;
    bool saved = _no_struct_lit;
    _no_struct_lit = false;
    if (!check(TokenType::RPAREN)) {
        do {
            Arg a;
            if (match(TokenType::REF)) {
                a.is_ref = true;
                a.ref_mutable = match(TokenType::VAR);
            }
            a.value = parse_expression();
            args.push(a);
        } while (match(TokenType::COMMA));
    }
    _no_struct_lit = saved;
    return args;
}

Node* Parser::parse_postfix() {
    Node* expr = parse_primary();
    for (;;) {
        if (match(TokenType::DOT)) {
            int ln = cur().line;
            String member = consume(TokenType::IDENTIFIER, "Expected member name after '.'").lexeme;
            if (match(TokenType::LPAREN)) {
                MethodCall* mc = new MethodCall(expr, member);
                mc->line = ln;
                mc->args = parse_args();
                consume(TokenType::RPAREN, "Expected ')' after method arguments");
                expr = mc;
            } else {
                Field* f = new Field(expr, member);
                f->line = ln;
                expr = f;
            }
        } else if (check(TokenType::LBRACKET)) {
            int ln = cur().line; advance();
            Node* idx = parse_expression();
            consume(TokenType::RBRACKET, "Expected ']' after index");
            Index* ix = new Index(expr, idx);
            ix->line = ln;
            expr = ix;
        } else {
            break;
        }
    }
    return expr;
}

Node* Parser::parse_primary() {
    int ln = cur().line;
    Token tok = cur();

    switch (tok.type) {
        case TokenType::INT_LITERAL: {
            advance();
            IntLit* n = new IntLit(atoll(tok.lexeme.c_str()));
            n->line = ln; return n;
        }
        case TokenType::FLOAT_LITERAL: {
            advance();
            FloatLit* n = new FloatLit(atof(tok.lexeme.c_str()));
            n->line = ln; return n;
        }
        case TokenType::TRUE: { advance(); BoolLit* n = new BoolLit(true); n->line = ln; return n; }
        case TokenType::FALSE: { advance(); BoolLit* n = new BoolLit(false); n->line = ln; return n; }
        case TokenType::SELF: { advance(); SelfExpr* n = new SelfExpr(); n->line = ln; return n; }
        case TokenType::STRING_LITERAL: {
            advance();
            // process escapes
            String raw = tok.lexeme;
            char* buf = (char*)malloc(raw.length() + 1);
            size_t j = 0;
            for (size_t i = 0; i < raw.length(); i++) {
                if (raw[i] == '\\' && i + 1 < raw.length()) {
                    i++;
                    char e = raw[i];
                    buf[j++] = (e == 'n') ? '\n' : (e == 't') ? '\t' :
                               (e == '\\') ? '\\' : (e == '"') ? '"' : e;
                } else buf[j++] = raw[i];
            }
            buf[j] = '\0';
            StringLit* n = new StringLit(String(buf, j));
            free(buf);
            n->line = ln; return n;
        }
        case TokenType::MATCH:
            return parse_match();
        case TokenType::CAST: {
            advance();
            consume(TokenType::LPAREN, "Expected '(' after 'cast'");
            Node* operand = parse_expression();
            consume(TokenType::COMMA, "Expected ',' in cast");
            TypeRef* t = parse_type();
            consume(TokenType::RPAREN, "Expected ')' after cast");
            CastExpr* c = new CastExpr(operand, t);
            c->line = ln; return c;
        }
        case TokenType::SIZEOF: {
            advance();
            consume(TokenType::LPAREN, "Expected '(' after 'sizeof'");
            TypeRef* t = parse_type();
            consume(TokenType::RPAREN, "Expected ')' after sizeof");
            SizeofExpr* s = new SizeofExpr(t);
            s->line = ln; return s;
        }
        case TokenType::LPAREN: {
            advance();
            bool saved = _no_struct_lit;
            _no_struct_lit = false;
            Node* inner = parse_expression();
            _no_struct_lit = saved;
            consume(TokenType::RPAREN, "Expected ')'");
            return inner;
        }
        case TokenType::LBRACKET: {
            advance();
            ArrayLit* arr = new ArrayLit();
            arr->line = ln;
            if (!check(TokenType::RBRACKET)) {
                do { arr->elements.push(parse_expression()); } while (match(TokenType::COMMA));
            }
            consume(TokenType::RBRACKET, "Expected ']' after array literal");
            return arr;
        }
        case TokenType::IDENTIFIER: {
            String name = tok.lexeme;
            advance();
            // struct literal: Name { field = expr, ... }
            if (!_no_struct_lit && check(TokenType::LBRACE) && peek().type == TokenType::IDENTIFIER &&
                peek(2).type == TokenType::ASSIGN) {
                advance(); // {
                StructLit* lit = new StructLit(name);
                lit->line = ln;
                if (!check(TokenType::RBRACE)) {
                    do {
                        FieldInit fi;
                        fi.name = consume(TokenType::IDENTIFIER, "Expected field name").lexeme;
                        consume(TokenType::ASSIGN, "Expected '=' in struct literal");
                        fi.value = parse_expression();
                        lit->fields.push(fi);
                    } while (match(TokenType::COMMA));
                }
                consume(TokenType::RBRACE, "Expected '}' after struct literal");
                return lit;
            }
            // call: name(args)
            if (match(TokenType::LPAREN)) {
                Call* call = new Call(name);
                call->line = ln;
                call->args = parse_args();
                consume(TokenType::RPAREN, "Expected ')' after arguments");
                return call;
            }
            Ident* id = new Ident(name);
            id->line = ln;
            return id;
        }
        default:
            fail("Unexpected token in expression");
    }
}

Node* Parser::parse_match() {
    int ln = cur().line;
    consume(TokenType::MATCH, "Expected 'match'");
    Node* subject = parse_expression();
    consume(TokenType::LBRACE, "Expected '{' after match subject");
    MatchExpr* m = new MatchExpr(subject);
    m->line = ln;
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        MatchArm* arm = new MatchArm();
        if (cur().type == TokenType::IDENTIFIER && cur().lexeme == "_") {
            arm->is_wildcard = true;
            advance();
        } else {
            arm->variant = consume(TokenType::IDENTIFIER, "Expected variant name in match arm").lexeme;
        }
        consume(TokenType::ARROW, "Expected '=>' in match arm");
        // arm body: a block, or a single comma-terminated statement
        if (check(TokenType::LBRACE)) {
            arm->body = parse_block();
        } else {
            Block* b = new Block();
            b->line = cur().line;
            if (check(TokenType::RETURN)) {
                int rln = cur().line;
                advance();
                Node* val = (check(TokenType::COMMA) || check(TokenType::RBRACE))
                            ? nullptr : parse_expression();
                ReturnStmt* rs = new ReturnStmt(val);
                rs->line = rln;
                b->statements.push(rs);
            } else if (check(TokenType::BREAK)) {
                advance(); b->statements.push(new BreakStmt());
            } else if (check(TokenType::CONTINUE)) {
                advance(); b->statements.push(new ContinueStmt());
            } else {
                Node* e = parse_expression();
                ExprStmt* s = new ExprStmt(e);
                s->line = e->line;
                b->statements.push(s);
            }
            arm->body = b;
            match(TokenType::COMMA);
        }
        match(TokenType::COMMA);
        m->arms.push(arm);
    }
    consume(TokenType::RBRACE, "Expected '}' after match arms");
    return m;
}

}
