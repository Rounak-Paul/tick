#include "checker.h"
#include <cstdio>
#include <cstdarg>

namespace Tick {

Checker::Checker(Program* program)
    : _program(program), _errors(false), _depth(0),
      _current_func(nullptr) {}

Checker::~Checker() {
    for (size_t i = 0; i < _stack.size(); i++) delete _stack[i].sym;
}

void Checker::error(int line, const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    fprintf(stderr, "Type error at line %d: %s\n", line, buf);
    _errors = true;
}

void Checker::push_scope() { _depth++; }

void Checker::pop_scope() {
    while (_stack.size() > 0 && _stack[_stack.size() - 1].depth == _depth) {
        delete _stack[_stack.size() - 1].sym;
        _stack.pop_back();
    }
    _depth--;
}

void Checker::declare(const String& name, Symbol* sym) {
    ScopeVar v;
    v.name = name;
    v.sym = sym;
    v.depth = _depth;
    _stack.push(v);
}

Symbol* Checker::lookup(const String& name) {
    for (size_t i = _stack.size(); i > 0; i--) {
        if (_stack[i - 1].name == name) return _stack[i - 1].sym;
    }
    return nullptr;
}

void Checker::mark_move(Node* node) { _moves.insert((const void*)node, true); }
void Checker::mark_reclaim(Node* decl) { _reclaims.insert((const void*)decl, true); }

bool Checker::is_move(Node* node) const {
    const bool* v = const_cast<Checker*>(this)->_moves.find((const void*)node);
    return v && *v;
}

bool Checker::needs_reclaim(Node* decl) const {
    const bool* v = const_cast<Checker*>(this)->_reclaims.find((const void*)decl);
    return v && *v;
}

// ---- lookups over the program ----

StructDecl* Checker::find_struct(const String& name) {
    for (size_t i = 0; i < _program->structs.size(); i++)
        if (_program->structs[i]->name == name) return _program->structs[i];
    return nullptr;
}

EnumDecl* Checker::find_enum(const String& name) {
    for (size_t i = 0; i < _program->enums.size(); i++)
        if (_program->enums[i]->name == name) return _program->enums[i];
    return nullptr;
}

InterfaceDecl* Checker::find_interface(const String& name) {
    for (size_t i = 0; i < _program->interfaces.size(); i++)
        if (_program->interfaces[i]->name == name) return _program->interfaces[i];
    return nullptr;
}

FuncDecl* Checker::find_method(const String& type_name, const String& method) {
    for (size_t i = 0; i < _program->impls.size(); i++) {
        if (_program->impls[i]->type_name != type_name) continue;
        for (size_t j = 0; j < _program->impls[i]->methods.size(); j++)
            if (_program->impls[i]->methods[j]->name == method)
                return _program->impls[i]->methods[j];
    }
    return nullptr;
}

FuncDecl* Checker::find_function(const String& name) {
    for (size_t i = 0; i < _program->functions.size(); i++)
        if (_program->functions[i]->name == name) return _program->functions[i];
    return nullptr;
}

ExternDecl* Checker::find_extern(const String& name) {
    for (size_t i = 0; i < _program->externs.size(); i++)
        if (_program->externs[i]->name == name) return _program->externs[i];
    return nullptr;
}

SignalDecl* Checker::find_signal(const String& name) {
    for (size_t i = 0; i < _program->signals.size(); i++)
        if (_program->signals[i]->name == name) return _program->signals[i];
    return nullptr;
}

EnumVariant* Checker::find_variant(EnumDecl* en, const String& variant) {
    for (size_t i = 0; i < en->variants.size(); i++)
        if (en->variants[i]->name == variant) return en->variants[i];
    return nullptr;
}

// ---- type relations ----

bool Checker::is_numeric(const TypeRef* t) const {
    return t && (t->kind == TypeKind::INT || t->kind == TypeKind::FLOAT);
}

bool Checker::types_equal(const TypeRef* a, const TypeRef* b) const {
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
        case TypeKind::INT: return a->int_bits == b->int_bits && a->int_unsigned == b->int_unsigned;
        case TypeKind::FLOAT: return a->float_bits == b->float_bits;
        case TypeKind::NAMED: case TypeKind::DYN: return a->name == b->name;
        case TypeKind::ARRAY: case TypeKind::PTR:
            return types_equal(a->inner, b->inner);
        case TypeKind::FIXED_ARRAY:
            return a->fixed_size == b->fixed_size && types_equal(a->inner, b->inner);
        default: return true;
    }
}

bool Checker::assignable(const TypeRef* target, const TypeRef* value) const {
    if (!target || !value) return true; // error already reported elsewhere
    if (types_equal(target, value)) return true;
    // numeric widening/coercion is allowed (validation layer guards narrowing)
    if (is_numeric(target) && is_numeric(value)) return true;
    // ptr interop only inside unsafe; checked at use site, not here
    return false;
}

// ---- entry ----

bool Checker::check() {
    collect_globals();

    // signals and events are addressable names
    for (size_t i = 0; i < _program->signals.size(); i++) {
        Symbol* s = new Symbol();
        s->kind = SymKind::SIGNAL;
        s->name = _program->signals[i]->name;
        s->type = new TypeRef(TypeKind::VOID);
        declare(s->name, s);
    }
    for (size_t i = 0; i < _program->events.size(); i++) {
        Symbol* s = new Symbol();
        s->kind = SymKind::EVENT;
        s->name = _program->events[i]->name;
        s->type = new TypeRef(TypeKind::VOID);
        declare(s->name, s);
    }

    check_interfaces();

    for (size_t i = 0; i < _program->globals.size(); i++) {
        GlobalDecl* g = _program->globals[i];
        if (g->init) {
            TypeRef* it = check_expr(g->init);
            if (!g->type && it) g->type = it->clone();
            if (g->type && it && !assignable(g->type, it))
                error(g->line, "global '%s' initializer type mismatch", g->name.c_str());
        }
    }

    for (size_t i = 0; i < _program->functions.size(); i++)
        check_func(_program->functions[i]);
    for (size_t i = 0; i < _program->impls.size(); i++)
        check_impl(_program->impls[i]);
    for (size_t i = 0; i < _program->processes.size(); i++) {
        push_scope();
        check_block(_program->processes[i]->body);
        pop_scope();
    }

    return !_errors;
}

void Checker::collect_globals() {
    for (size_t i = 0; i < _program->globals.size(); i++) {
        GlobalDecl* g = _program->globals[i];
        Symbol* s = new Symbol();
        s->kind = SymKind::GLOBAL;
        s->name = g->name;
        s->is_mutable = !g->is_const;
        s->type = g->type ? g->type->clone() : nullptr;
        declare(g->name, s);
    }
}

void Checker::check_interfaces() {
    // verify each `impl I for T` provides every method I requires with a body
    for (size_t i = 0; i < _program->impls.size(); i++) {
        ImplDecl* impl = _program->impls[i];
        if (impl->interface_name.empty()) continue;
        InterfaceDecl* it = find_interface(impl->interface_name);
        if (!it) { error(impl->line, "unknown interface '%s'", impl->interface_name.c_str()); continue; }
        for (size_t m = 0; m < it->methods.size(); m++) {
            const String& want = it->methods[m]->name;
            if (!find_method(impl->type_name, want))
                error(impl->line, "type '%s' does not implement method '%s' of interface '%s'",
                      impl->type_name.c_str(), want.c_str(), impl->interface_name.c_str());
        }
    }
}

void Checker::check_func(FuncDecl* fn) {
    if (!fn->body) return;
    FuncDecl* prev = _current_func;
    _current_func = fn;
    push_scope();
    if (fn->self_kind != SelfKind::NONE && !fn->impl_type.empty()) {
        Symbol* s = new Symbol();
        s->kind = SymKind::PARAM;
        s->name = "self";
        TypeRef* st = new TypeRef(TypeKind::NAMED);
        st->name = fn->impl_type;
        if (fn->self_kind == SelfKind::REF || fn->self_kind == SelfKind::REF_MUT)
            st->ownership = Ownership::REF;
        s->type = st;
        s->is_mutable = (fn->self_kind == SelfKind::REF_MUT || fn->self_kind == SelfKind::VALUE);
        declare("self", s);
    }
    for (size_t i = 0; i < fn->params.size(); i++) {
        Symbol* s = new Symbol();
        s->kind = SymKind::PARAM;
        s->name = fn->params[i]->name;
        s->type = fn->params[i]->type->clone();
        s->is_mutable = (fn->params[i]->type->ownership == Ownership::REF && fn->params[i]->type->ref_mutable)
                        || fn->params[i]->type->ownership == Ownership::VALUE;
        declare(s->name, s);
    }
    check_block(fn->body);
    pop_scope();
    _current_func = prev;
}

void Checker::check_impl(ImplDecl* impl) {
    String prev_self = _current_self_type;
    _current_self_type = impl->type_name;
    for (size_t i = 0; i < impl->methods.size(); i++)
        check_func(impl->methods[i]);
    _current_self_type = prev_self;
}

// ---- statements ----

void Checker::check_block(Block* block) {
    push_scope();
    for (size_t i = 0; i < block->statements.size(); i++)
        check_stmt(block->statements[i]);
    pop_scope();
}

void Checker::check_stmt(Node* stmt) {
    switch (stmt->kind) {
        case NodeKind::VAR_DECL: check_var(static_cast<VarDecl*>(stmt)); break;
        case NodeKind::EXPR_STMT: check_expr(static_cast<ExprStmt*>(stmt)->expr); break;
        case NodeKind::BLOCK: check_block(static_cast<Block*>(stmt)); break;
        case NodeKind::IF: {
            IfStmt* s = static_cast<IfStmt*>(stmt);
            TypeRef* c = check_expr(s->cond);
            if (c && c->kind != TypeKind::BOOL) error(s->line, "if condition must be bool");
            check_block(s->then_branch);
            if (s->else_branch) check_stmt(s->else_branch);
            break;
        }
        case NodeKind::WHILE: {
            WhileStmt* s = static_cast<WhileStmt*>(stmt);
            TypeRef* c = check_expr(s->cond);
            if (c && c->kind != TypeKind::BOOL) error(s->line, "while condition must be bool");
            check_block(s->body);
            break;
        }
        case NodeKind::FOR: {
            ForStmt* s = static_cast<ForStmt*>(stmt);
            TypeRef* it = check_expr(s->iterable);
            push_scope();
            Symbol* sym = new Symbol();
            sym->kind = SymKind::LOCAL;
            sym->name = s->var_name;
            sym->is_mutable = false;
            if (s->iterable->kind == NodeKind::RANGE) {
                sym->type = new TypeRef(TypeKind::INT); sym->type->int_bits = 32;
            } else if (it && it->kind == TypeKind::ARRAY) {
                sym->type = it->inner ? it->inner->clone() : new TypeRef(TypeKind::INT);
                if (s->bind_ref) sym->type->ownership = Ownership::REF;
            } else {
                sym->type = new TypeRef(TypeKind::INT); sym->type->int_bits = 32;
            }
            declare(s->var_name, sym);
            check_block(s->body);
            pop_scope();
            break;
        }
        case NodeKind::RETURN: {
            ReturnStmt* s = static_cast<ReturnStmt*>(stmt);
            if (s->value) {
                TypeRef* vt = check_expr(s->value);
                // returning a bare owned local is a move out
                if (s->value->kind == NodeKind::IDENT) {
                    Symbol* sym = lookup(static_cast<Ident*>(s->value)->name);
                    if (sym && sym->type && sym->type->is_heap_owned() &&
                        sym->type->ownership == Ownership::VALUE)
                        mark_move(s->value);
                }
                if (_current_func && _current_func->return_type && vt &&
                    _current_func->return_type->kind != TypeKind::VOID &&
                    !assignable(_current_func->return_type, vt))
                    error(s->line, "return type mismatch in '%s'", _current_func->name.c_str());
            }
            break;
        }
        case NodeKind::DEFER: check_stmt(static_cast<DeferStmt*>(stmt)->stmt); break;
        case NodeKind::UNSAFE_BLOCK: check_block(static_cast<UnsafeBlock*>(stmt)->body); break;
        case NodeKind::BREAK: case NodeKind::CONTINUE: break;
        default: check_expr(stmt); break;
    }
}

void Checker::check_var(VarDecl* d) {
    TypeRef* it = nullptr;
    if (d->init) {
        it = check_expr(d->init);
        if (d->declared_type && it && !assignable(d->declared_type, it))
            error(d->line, "binding '%s' type mismatch", d->name.c_str());
    }
    TypeRef* bind_type = d->declared_type ? d->declared_type->clone()
                       : (it ? it->clone() : new TypeRef(TypeKind::VOID));

    Symbol* s = new Symbol();
    s->kind = SymKind::LOCAL;
    s->name = d->name;
    s->is_mutable = !d->is_const;
    s->type = bind_type;
    declare(d->name, s);

    // ownership: an owned heap value bound here is reclaimed at scope exit,
    // unless it was moved in (init is a moved local / constructor result).
    if (bind_type->is_heap_owned() && bind_type->ownership != Ownership::REF &&
        bind_type->ownership != Ownership::WEAK) {
        mark_reclaim(d);
    }
}

// ---- expressions ----

TypeRef* Checker::check_expr(Node* expr) {
    if (!expr) return nullptr;
    TypeRef* t = nullptr;
    switch (expr->kind) {
        case NodeKind::INT_LIT: t = new TypeRef(TypeKind::INT); t->int_bits = 32; break;
        case NodeKind::FLOAT_LIT: t = new TypeRef(TypeKind::FLOAT); t->float_bits = 64; break;
        case NodeKind::BOOL_LIT: t = new TypeRef(TypeKind::BOOL); break;
        case NodeKind::STRING_LIT: t = new TypeRef(TypeKind::STR); break;
        case NodeKind::SELF: {
            Symbol* sym = lookup("self");
            t = sym && sym->type ? sym->type->clone() : new TypeRef(TypeKind::VOID);
            break;
        }
        case NodeKind::IDENT: {
            Ident* id = static_cast<Ident*>(expr);
            Symbol* sym = lookup(id->name);
            if (sym && sym->type) { t = sym->type->clone(); }
            else if (find_enum(id->name)) { t = new TypeRef(TypeKind::NAMED); t->name = id->name; }
            else {
                // bare enum variant, e.g. `East` for `enum Dir { East }`
                EnumDecl* owner = nullptr;
                for (size_t i = 0; i < _program->enums.size(); i++)
                    if (find_variant(_program->enums[i], id->name)) { owner = _program->enums[i]; break; }
                if (owner) { t = new TypeRef(TypeKind::NAMED); t->name = owner->name; }
                else { error(expr->line, "use of undeclared name '%s'", id->name.c_str()); t = new TypeRef(TypeKind::VOID); }
            }
            break;
        }
        case NodeKind::BINARY: t = check_binary(static_cast<Binary*>(expr)); break;
        case NodeKind::UNARY: {
            Unary* u = static_cast<Unary*>(expr);
            TypeRef* ot = check_expr(u->operand);
            if (u->op == "!") t = new TypeRef(TypeKind::BOOL);
            else t = ot ? ot->clone() : new TypeRef(TypeKind::INT);
            break;
        }
        case NodeKind::ASSIGN: {
            Assign* a = static_cast<Assign*>(expr);
            TypeRef* tt = check_expr(a->target);
            TypeRef* vt = check_expr(a->value);
            if (a->target->kind == NodeKind::IDENT) {
                Symbol* sym = lookup(static_cast<Ident*>(a->target)->name);
                if (sym && !sym->is_mutable)
                    error(a->line, "cannot assign to immutable binding '%s'",
                          static_cast<Ident*>(a->target)->name.c_str());
            }
            if (tt && vt && !assignable(tt, vt))
                error(a->line, "assignment type mismatch");
            t = new TypeRef(TypeKind::VOID);
            break;
        }
        case NodeKind::COMPOUND_ASSIGN: {
            CompoundAssign* a = static_cast<CompoundAssign*>(expr);
            check_expr(a->target);
            check_expr(a->value);
            if (a->target->kind == NodeKind::IDENT) {
                Symbol* sym = lookup(static_cast<Ident*>(a->target)->name);
                if (sym && !sym->is_mutable)
                    error(a->line, "cannot modify immutable binding '%s'",
                          static_cast<Ident*>(a->target)->name.c_str());
            }
            t = new TypeRef(TypeKind::VOID);
            break;
        }
        case NodeKind::CALL: t = check_call(static_cast<Call*>(expr)); break;
        case NodeKind::METHOD_CALL: t = check_method_call(static_cast<MethodCall*>(expr)); break;
        case NodeKind::FIELD: t = check_field(static_cast<Field*>(expr)); break;
        case NodeKind::INDEX: t = check_index(static_cast<Index*>(expr)); break;
        case NodeKind::STRUCT_LIT: t = check_struct_lit(static_cast<StructLit*>(expr)); break;
        case NodeKind::ARRAY_LIT: t = check_array_lit(static_cast<ArrayLit*>(expr)); break;
        case NodeKind::MATCH: t = check_match(static_cast<MatchExpr*>(expr)); break;
        case NodeKind::RANGE: {
            RangeExpr* r = static_cast<RangeExpr*>(expr);
            check_expr(r->start); check_expr(r->end);
            t = new TypeRef(TypeKind::INT); t->int_bits = 32;
            break;
        }
        case NodeKind::REF_EXPR: {
            RefExpr* r = static_cast<RefExpr*>(expr);
            TypeRef* ot = check_expr(r->operand);
            if (r->operand->kind == NodeKind::IDENT) {
                Symbol* sym = lookup(static_cast<Ident*>(r->operand)->name);
                if (r->mutable_ref && sym && !sym->is_mutable)
                    error(r->line, "cannot take 'ref var' of immutable binding '%s'",
                          static_cast<Ident*>(r->operand)->name.c_str());
            }
            t = ot ? ot->clone() : new TypeRef(TypeKind::VOID);
            t->ownership = Ownership::REF;
            t->ref_mutable = r->mutable_ref;
            break;
        }
        case NodeKind::SHARED_EXPR: {
            SharedExpr* s = static_cast<SharedExpr*>(expr);
            TypeRef* ot = check_expr(s->operand);
            t = ot ? ot->clone() : new TypeRef(TypeKind::VOID);
            t->ownership = Ownership::SHARED;
            break;
        }
        case NodeKind::WEAK_EXPR: {
            WeakExpr* w = static_cast<WeakExpr*>(expr);
            TypeRef* ot = check_expr(w->operand);
            if (ot && ot->ownership != Ownership::SHARED)
                error(w->line, "'weak' requires a 'shared' value");
            t = ot ? ot->clone() : new TypeRef(TypeKind::VOID);
            t->ownership = Ownership::WEAK;
            break;
        }
        case NodeKind::CAST: {
            CastExpr* c = static_cast<CastExpr*>(expr);
            check_expr(c->operand);
            t = c->target->clone();
            break;
        }
        case NodeKind::SIZEOF: {
            t = new TypeRef(TypeKind::INT); t->int_bits = 64; t->int_unsigned = true;
            break;
        }
        default:
            t = new TypeRef(TypeKind::VOID);
            break;
    }
    expr->resolved_type = t;
    return t;
}

TypeRef* Checker::check_binary(Binary* b) {
    TypeRef* lt = check_expr(b->left);
    TypeRef* rt = check_expr(b->right);
    const String& op = b->op;
    if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=" ||
        op == "&&" || op == "||")
        return new TypeRef(TypeKind::BOOL);
    if (op == "+" && lt && (lt->kind == TypeKind::STR || (rt && rt->kind == TypeKind::STR)))
        return new TypeRef(TypeKind::STR);
    if (lt && lt->kind == TypeKind::FLOAT) return lt->clone();
    if (rt && rt->kind == TypeKind::FLOAT) return rt->clone();
    return lt ? lt->clone() : new TypeRef(TypeKind::INT);
}

bool Checker::is_builtin(const String& name) const {
    static const char* names[] = {
        "print", "println", "to_str", "len", "str_order", "sqrt", "pow",
        "sin", "cos", "tan", "floor", "ceil", "round", "abs", "min", "max",
        "input", "malloc", "free", "write", "read",
        nullptr
    };
    for (int i = 0; names[i]; i++) if (name == names[i]) return true;
    return false;
}

TypeRef* Checker::builtin_call_type(const String& name, Call* call) {
    if (name == "to_str") return new TypeRef(TypeKind::STR);
    if (name == "input") return new TypeRef(TypeKind::STR);
    if (name == "len") { TypeRef* t = new TypeRef(TypeKind::INT); t->int_bits = 32; return t; }
    if (name == "str_order") { TypeRef* t = new TypeRef(TypeKind::INT); t->int_bits = 32; return t; }
    if (name == "abs") { TypeRef* t = new TypeRef(TypeKind::INT); t->int_bits = 32; return t; }
    if (name == "sqrt" || name == "pow" || name == "sin" || name == "cos" ||
        name == "tan" || name == "floor" || name == "ceil" || name == "round" ||
        name == "min" || name == "max") {
        TypeRef* t = new TypeRef(TypeKind::FLOAT); t->float_bits = 64; return t;
    }
    if (name == "print" || name == "println" || name == "free" || name == "write")
        return new TypeRef(TypeKind::VOID);
    if (name == "malloc") return new TypeRef(TypeKind::PTR);
    if (name == "read") { TypeRef* t = new TypeRef(TypeKind::INT); t->int_bits = 8; t->int_unsigned = true; return t; }
    (void)call;
    return new TypeRef(TypeKind::VOID);
}

TypeRef* Checker::check_call(Call* call) {
    // struct constructor by positional fields
    StructDecl* st = find_struct(call->callee);
    if (st) {
        if (call->args.size() != st->fields.size())
            error(call->line, "struct '%s' expects %d field(s), got %d",
                  call->callee.c_str(), (int)st->fields.size(), (int)call->args.size());
        for (size_t i = 0; i < call->args.size(); i++) check_expr(call->args[i].value);
        TypeRef* t = new TypeRef(TypeKind::NAMED);
        t->name = call->callee;
        return t;
    }
    if (is_builtin(call->callee)) {
        for (size_t i = 0; i < call->args.size(); i++)
            check_expr(call->args[i].value);
        return builtin_call_type(call->callee, call);
    }

    FuncDecl* fn = find_function(call->callee);
    ExternDecl* ex = fn ? nullptr : find_extern(call->callee);
    if (!fn && !ex) {
        error(call->line, "call to undeclared function '%s'", call->callee.c_str());
        for (size_t i = 0; i < call->args.size(); i++) check_expr(call->args[i].value);
        return new TypeRef(TypeKind::VOID);
    }

    DynamicArray<Param*>& params = fn ? fn->params : ex->params;
    if (call->args.size() != params.size())
        error(call->line, "'%s' expects %d argument(s), got %d",
              call->callee.c_str(), (int)params.size(), (int)call->args.size());

    for (size_t i = 0; i < call->args.size(); i++) {
        Arg& a = call->args[i];
        TypeRef* at = check_expr(a.value);
        bool param_is_ref = i < params.size() && params[i]->type->ownership == Ownership::REF;
        if (a.is_ref != param_is_ref) {
            if (param_is_ref)
                error(call->line, "argument %d of '%s' must be passed with 'ref'", (int)i + 1, call->callee.c_str());
            else
                error(call->line, "argument %d of '%s' is by value; remove 'ref'", (int)i + 1, call->callee.c_str());
        }
        // by-value pass of an owned local at last use -> move
        if (!a.is_ref && a.value->kind == NodeKind::IDENT) {
            Symbol* sym = lookup(static_cast<Ident*>(a.value)->name);
            if (sym && sym->type && sym->type->is_heap_owned() &&
                sym->type->ownership == Ownership::VALUE)
                mark_move(a.value);
        }
        (void)at;
    }
    TypeRef* ret = fn ? fn->return_type : ex->return_type;
    return ret ? ret->clone() : new TypeRef(TypeKind::VOID);
}

TypeRef* Checker::check_method_call(MethodCall* mc) {
    TypeRef* recv = check_expr(mc->receiver);

    // built-in methods on arrays / strings / signals
    if (recv) {
        if (recv->kind == TypeKind::ARRAY) {
            if (mc->method == "len") { TypeRef* t = new TypeRef(TypeKind::INT); t->int_bits = 32; return t; }
            if (mc->method == "push") { for (size_t i = 0; i < mc->args.size(); i++) check_expr(mc->args[i].value); return new TypeRef(TypeKind::VOID); }
            if (mc->method == "pop") return recv->inner ? recv->inner->clone() : new TypeRef(TypeKind::VOID);
        }
        if (recv->kind == TypeKind::STR) {
            if (mc->method == "len") { TypeRef* t = new TypeRef(TypeKind::INT); t->int_bits = 32; return t; }
        }
    }

    // signal & event methods
    if (mc->receiver->kind == NodeKind::IDENT) {
        const String& rn = static_cast<Ident*>(mc->receiver)->name;
        SignalDecl* sig = find_signal(rn);
        if (sig) {
            if (mc->method == "emit") { for (size_t i = 0; i < mc->args.size(); i++) check_expr(mc->args[i].value); return new TypeRef(TypeKind::VOID); }
            if (mc->method == "recv") return sig->payload_type ? sig->payload_type->clone() : new TypeRef(TypeKind::VOID);
        }
        for (size_t i = 0; i < _program->events.size(); i++) {
            if (_program->events[i]->name == rn && mc->method == "fire")
                return new TypeRef(TypeKind::VOID);
        }
    }

    // user method on a named type
    if (recv && (recv->kind == TypeKind::NAMED || recv->kind == TypeKind::DYN)) {
        FuncDecl* m = find_method(recv->name, mc->method);
        if (!m && recv->kind == TypeKind::DYN) {
            InterfaceDecl* it = find_interface(recv->name);
            if (it) for (size_t i = 0; i < it->methods.size(); i++)
                if (it->methods[i]->name == mc->method) m = it->methods[i];
        }
        if (m) {
            for (size_t i = 0; i < mc->args.size(); i++) check_expr(mc->args[i].value);
            return m->return_type ? m->return_type->clone() : new TypeRef(TypeKind::VOID);
        }
    }

    error(mc->line, "no method '%s' on this type", mc->method.c_str());
    for (size_t i = 0; i < mc->args.size(); i++) check_expr(mc->args[i].value);
    return new TypeRef(TypeKind::VOID);
}

TypeRef* Checker::check_field(Field* f) {
    // qualified enum value: EnumName.Variant
    if (f->object->kind == NodeKind::IDENT) {
        const String& base = static_cast<Ident*>(f->object)->name;
        EnumDecl* en = find_enum(base);
        if (en && !lookup(base)) {
            if (!find_variant(en, f->name))
                error(f->line, "enum '%s' has no variant '%s'", base.c_str(), f->name.c_str());
            TypeRef* t = new TypeRef(TypeKind::NAMED);
            t->name = base;
            return t;
        }
    }
    TypeRef* ot = check_expr(f->object);
    if (ot && ot->kind == TypeKind::NAMED) {
        StructDecl* st = find_struct(ot->name);
        if (st) {
            for (size_t i = 0; i < st->fields.size(); i++)
                if (st->fields[i]->name == f->name)
                    return st->fields[i]->type->clone();
            error(f->line, "type '%s' has no field '%s'", ot->name.c_str(), f->name.c_str());
        }
    }
    return new TypeRef(TypeKind::VOID);
}

TypeRef* Checker::check_index(Index* ix) {
    TypeRef* at = check_expr(ix->array);
    TypeRef* it = check_expr(ix->index);
    if (it && it->kind != TypeKind::INT)
        error(ix->line, "array index must be an integer");
    if (at && (at->kind == TypeKind::ARRAY || at->kind == TypeKind::FIXED_ARRAY) && at->inner)
        return at->inner->clone();
    if (at && at->kind == TypeKind::STR) { TypeRef* t = new TypeRef(TypeKind::INT); t->int_bits = 8; t->int_unsigned = true; return t; }
    return new TypeRef(TypeKind::VOID);
}

TypeRef* Checker::check_struct_lit(StructLit* lit) {
    StructDecl* st = find_struct(lit->type_name);
    if (!st) { error(lit->line, "unknown struct '%s'", lit->type_name.c_str()); return new TypeRef(TypeKind::VOID); }
    for (size_t i = 0; i < lit->fields.size(); i++) {
        TypeRef* vt = check_expr(lit->fields[i].value);
        bool found = false;
        for (size_t j = 0; j < st->fields.size(); j++) {
            if (st->fields[j]->name == lit->fields[i].name) {
                found = true;
                if (vt && !assignable(st->fields[j]->type, vt))
                    error(lit->line, "field '%s' type mismatch", lit->fields[i].name.c_str());
                break;
            }
        }
        if (!found) error(lit->line, "struct '%s' has no field '%s'", lit->type_name.c_str(), lit->fields[i].name.c_str());
    }
    if (lit->fields.size() != st->fields.size())
        error(lit->line, "struct literal '%s' must initialize all %d fields", lit->type_name.c_str(), (int)st->fields.size());
    TypeRef* t = new TypeRef(TypeKind::NAMED);
    t->name = lit->type_name;
    return t;
}

TypeRef* Checker::check_array_lit(ArrayLit* lit) {
    TypeRef* elem = nullptr;
    for (size_t i = 0; i < lit->elements.size(); i++) {
        TypeRef* et = check_expr(lit->elements[i]);
        if (!elem && et) elem = et->clone();
    }
    TypeRef* t = new TypeRef(TypeKind::ARRAY);
    t->inner = elem ? elem : new TypeRef(TypeKind::INT);
    if (t->inner->kind == TypeKind::INT && t->inner->int_bits == 0) t->inner->int_bits = 32;
    return t;
}

TypeRef* Checker::check_match(MatchExpr* m) {
    TypeRef* st = check_expr(m->subject);
    EnumDecl* en = nullptr;
    if (st && st->kind == TypeKind::NAMED) en = find_enum(st->name);

    bool has_wildcard = false;
    DynamicArray<String> covered;
    for (size_t i = 0; i < m->arms.size(); i++) {
        MatchArm* arm = m->arms[i];
        push_scope();
        if (arm->is_wildcard) {
            has_wildcard = true;
        } else if (en) {
            EnumVariant* v = find_variant(en, arm->variant);
            if (!v) error(m->line, "enum '%s' has no variant '%s'", en->name.c_str(), arm->variant.c_str());
            else covered.push(arm->variant);
        }
        check_block(static_cast<Block*>(arm->body));
        pop_scope();
    }

    if (en && !has_wildcard) {
        for (size_t i = 0; i < en->variants.size(); i++) {
            bool found = false;
            for (size_t j = 0; j < covered.size(); j++)
                if (covered[j] == en->variants[i]->name) { found = true; break; }
            if (!found)
                error(m->line, "match not exhaustive: missing variant '%s'", en->variants[i]->name.c_str());
        }
    }
    return new TypeRef(TypeKind::VOID);
}

}
