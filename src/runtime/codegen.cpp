#include "codegen.h"
#include <cstdlib>
#include <cstring>

namespace Tick {

CBuffer::CBuffer() : len(0), cap(65536) { data = (char*)malloc(cap); }
CBuffer::~CBuffer() { free(data); }

void CBuffer::put(const char* fmt, ...) {
    va_list a;
    va_start(a, fmt);
    int need = vsnprintf(nullptr, 0, fmt, a);
    va_end(a);
    while (len + need + 1 >= cap) { cap *= 2; data = (char*)realloc(data, cap); }
    va_start(a, fmt);
    len += vsnprintf(data + len, cap - len, fmt, a);
    va_end(a);
}

void CBuffer::indent(int n) { for (int i = 0; i < n; i++) put("    "); }

CodeGen::CodeGen(Program* program, Checker* checker, BuildMode mode)
    : _program(program), _checker(checker), _mode(mode), _scope(-1), _return_type(nullptr) {}

// ---- type lowering ----

String CodeGen::c_type_str(const TypeRef* t) {
    CBuffer b;
    c_type(t, b);
    return String(b.data, b.len);
}

void CodeGen::c_type(const TypeRef* t, CBuffer& b) {
    if (!t) { b.put("void"); return; }

    // references and shared/weak are pointers to the underlying C type
    if (t->ownership == Ownership::REF || t->ownership == Ownership::SHARED ||
        t->ownership == Ownership::WEAK) {
        TypeRef* base = t->clone();
        base->ownership = Ownership::VALUE;
        String inner = c_type_str(base);
        delete base;
        b.put("%s*", inner.c_str());
        return;
    }

    switch (t->kind) {
        case TypeKind::VOID: b.put("void"); break;
        case TypeKind::BOOL: b.put("bool"); break;
        case TypeKind::STR: b.put("char*"); break;
        case TypeKind::PTR: b.put("void*"); break;
        case TypeKind::INT:
            b.put("%sint%d_t", t->int_unsigned ? "u" : "", t->int_bits ? t->int_bits : 32);
            break;
        case TypeKind::FLOAT:
            b.put(t->float_bits == 32 ? "float" : "double");
            break;
        case TypeKind::NAMED: b.put("%s", t->name.c_str()); break;
        case TypeKind::DYN: b.put("Tick_dyn_%s", t->name.c_str()); break;
        case TypeKind::ARRAY: b.put("TickArray"); break;
        case TypeKind::FIXED_ARRAY: {
            // represented by value; element type used at declaration site
            String inner = c_type_str(t->inner);
            b.put("%s", inner.c_str());
            break;
        }
        default: b.put("void"); break;
    }
}

String CodeGen::mangle_func(const String& name) {
    if (name == "main") return String("main");
    CBuffer b; b.put("tk_%s", name.c_str());
    return String(b.data, b.len);
}

String CodeGen::mangle_method(const String& type, const String& method) {
    CBuffer b; b.put("tk_%s_%s", type.c_str(), method.c_str());
    return String(b.data, b.len);
}

// ---- reclaim tracking ----

bool CodeGen::is_reclaimable(const TypeRef* t) {
    if (!t) return false;
    if (t->ownership == Ownership::REF || t->ownership == Ownership::WEAK) return false;
    if (t->ownership == Ownership::SHARED) return true;
    return t->kind == TypeKind::ARRAY || t->kind == TypeKind::STR;
}

CodeGen::Reclaim CodeGen::reclaim_kind(const TypeRef* t) {
    if (t->ownership == Ownership::SHARED) return Reclaim::SHARED;
    if (t->kind == TypeKind::ARRAY) return Reclaim::ARRAY;
    return Reclaim::STRING;
}

void CodeGen::push_scope() { _scope++; _scopes[_scope].clear(); _defers[_scope].clear(); }

void CodeGen::track_owned(const String& name, const TypeRef* t) {
    if (!is_reclaimable(t)) return;
    Owned o; o.name = name; o.kind = reclaim_kind(t);
    _scopes[_scope].push(o);
}

void CodeGen::pop_scope_reclaims(int indent) {
    // defers first (LIFO), then owned-value reclamation
    DynamicArray<Node*>& d = _defers[_scope];
    for (size_t i = d.size(); i > 0; i--)
        gen_stmt(d[i - 1], indent);
    DynamicArray<Owned>& s = _scopes[_scope];
    for (size_t i = s.size(); i > 0; i--) {
        const Owned& o = s[i - 1];
        _out.indent(indent);
        switch (o.kind) {
            case Reclaim::ARRAY:  _out.put("tick_array_free(&%s);\n", o.name.c_str()); break;
            case Reclaim::STRING: _out.put("free((void*)%s);\n", o.name.c_str()); break;
            case Reclaim::SHARED: _out.put("tick_shared_release(%s);\n", o.name.c_str()); break;
            case Reclaim::WEAK:   _out.put("tick_weak_drop(%s);\n", o.name.c_str()); break;
        }
    }
}

void CodeGen::pop_scope() { _scope--; }

void CodeGen::emit_all_reclaims(int indent) {
    for (int s = _scope; s >= 0; s--) {
        DynamicArray<Node*>& d = _defers[s];
        for (size_t i = d.size(); i > 0; i--)
            gen_stmt(d[i - 1], indent);
        DynamicArray<Owned>& sc = _scopes[s];
        for (size_t i = sc.size(); i > 0; i--) {
            const Owned& o = sc[i - 1];
            _out.indent(indent);
            switch (o.kind) {
                case Reclaim::ARRAY:  _out.put("tick_array_free(&%s);\n", o.name.c_str()); break;
                case Reclaim::STRING: _out.put("free((void*)%s);\n", o.name.c_str()); break;
                case Reclaim::SHARED: _out.put("tick_shared_release(%s);\n", o.name.c_str()); break;
                case Reclaim::WEAK:   _out.put("tick_weak_drop(%s);\n", o.name.c_str()); break;
            }
        }
    }
}

// ---- top level ----

String CodeGen::generate() {
    emit_prelude();

    for (size_t i = 0; i < _program->structs.size(); i++) emit_struct(_program->structs[i]);
    for (size_t i = 0; i < _program->enums.size(); i++) emit_enum(_program->enums[i]);
    for (size_t i = 0; i < _program->externs.size(); i++) emit_extern(_program->externs[i]);

    // globals
    for (size_t i = 0; i < _program->globals.size(); i++) {
        GlobalDecl* g = _program->globals[i];
        String ct = c_type_str(g->type);
        if (g->is_const) _out.put("const ");
        _out.put("%s %s;\n", ct.c_str(), g->name.c_str());
    }

    // signals & events
    for (size_t i = 0; i < _program->signals.size(); i++)
        _out.put("TickSignal %s;\n", _program->signals[i]->name.c_str());
    for (size_t i = 0; i < _program->events.size(); i++)
        _out.put("TickEvent %s;\n", _program->events[i]->name.c_str());
    _out.put("\n");

    // prototypes
    for (size_t i = 0; i < _program->functions.size(); i++) emit_func_proto(_program->functions[i]);
    for (size_t i = 0; i < _program->impls.size(); i++)
        for (size_t j = 0; j < _program->impls[i]->methods.size(); j++)
            emit_func_proto(_program->impls[i]->methods[j]);
    for (size_t i = 0; i < _program->processes.size(); i++)
        _out.put("void* tk_proc_%s(void*);\n", _program->processes[i]->name.c_str());
    _out.put("\n");

    // process bodies
    for (size_t i = 0; i < _program->processes.size(); i++) emit_process(_program->processes[i]);

    // function & method bodies
    for (size_t i = 0; i < _program->functions.size(); i++) emit_func(_program->functions[i]);
    for (size_t i = 0; i < _program->impls.size(); i++)
        for (size_t j = 0; j < _program->impls[i]->methods.size(); j++)
            emit_func(_program->impls[i]->methods[j]);

    return String(_out.data, _out.len);
}

void CodeGen::emit_prelude() {
    _out.put("#include <stdio.h>\n");
    _out.put("#include <stdlib.h>\n");
    _out.put("#include <string.h>\n");
    _out.put("#include <stdbool.h>\n");
    _out.put("#include <stdint.h>\n");
    _out.put("#include <math.h>\n");
    _out.put("#include \"tick_runtime.h\"\n\n");
}

void CodeGen::emit_struct(StructDecl* st) {
    _out.put("typedef struct %s {\n", st->name.c_str());
    for (size_t i = 0; i < st->fields.size(); i++) {
        FieldDecl* f = st->fields[i];
        _out.indent(1);
        if (f->type->kind == TypeKind::FIXED_ARRAY) {
            String et = c_type_str(f->type->inner);
            _out.put("%s %s[%d];\n", et.c_str(), f->name.c_str(), f->type->fixed_size);
        } else {
            String ct = c_type_str(f->type);
            _out.put("%s %s;\n", ct.c_str(), f->name.c_str());
        }
    }
    _out.put("} %s;\n\n", st->name.c_str());
}

void CodeGen::emit_enum(EnumDecl* en) {
    // enums are named integer constants, represented as int32
    long long next = 0;
    for (size_t i = 0; i < en->variants.size(); i++) {
        EnumVariant* v = en->variants[i];
        long long val = v->has_int_value ? v->int_value : next;
        _out.put("#define %s_%s %lld\n", en->name.c_str(), v->name.c_str(), val);
        next = val + 1;
    }
    _out.put("typedef int32_t %s;\n\n", en->name.c_str());
}

void CodeGen::emit_extern(ExternDecl* ex) {
    String rt = c_type_str(ex->return_type);
    _out.put("extern %s %s(", rt.c_str(), ex->name.c_str());
    for (size_t i = 0; i < ex->params.size(); i++) {
        if (i) _out.put(", ");
        String pt = c_type_str(ex->params[i]->type);
        _out.put("%s %s", pt.c_str(), ex->params[i]->name.c_str());
    }
    if (ex->params.size() == 0) _out.put("void");
    _out.put(");\n");
}

void CodeGen::emit_func_proto(FuncDecl* fn) {
    if (!fn->body) return;
    String rt = c_type_str(fn->return_type);
    String name = fn->impl_type.empty() ? mangle_func(fn->name)
                                        : mangle_method(fn->impl_type, fn->name);
    _out.put("%s %s(", rt.c_str(), name.c_str());
    bool first = true;
    if (fn->self_kind != SelfKind::NONE && !fn->impl_type.empty()) {
        if (fn->self_kind == SelfKind::VALUE) _out.put("%s self", fn->impl_type.c_str());
        else _out.put("%s* self", fn->impl_type.c_str());
        first = false;
    }
    for (size_t i = 0; i < fn->params.size(); i++) {
        if (!first) _out.put(", ");
        first = false;
        String pt = c_type_str(fn->params[i]->type);
        _out.put("%s %s", pt.c_str(), fn->params[i]->name.c_str());
    }
    if (first) _out.put("void");
    _out.put(");\n");
}

void CodeGen::emit_func(FuncDecl* fn) {
    if (!fn->body) return;
    String rt = c_type_str(fn->return_type);
    String name = fn->impl_type.empty() ? mangle_func(fn->name)
                                        : mangle_method(fn->impl_type, fn->name);
    _out.put("%s %s(", rt.c_str(), name.c_str());
    bool first = true;
    if (fn->self_kind != SelfKind::NONE && !fn->impl_type.empty()) {
        if (fn->self_kind == SelfKind::VALUE) _out.put("%s self", fn->impl_type.c_str());
        else _out.put("%s* self", fn->impl_type.c_str());
        first = false;
    }
    for (size_t i = 0; i < fn->params.size(); i++) {
        if (!first) _out.put(", ");
        first = false;
        String pt = c_type_str(fn->params[i]->type);
        _out.put("%s %s", pt.c_str(), fn->params[i]->name.c_str());
    }
    if (first) _out.put("void");
    _out.put(") {\n");

    _return_type = fn->return_type;
    push_scope();
    if (fn->name == "main" && fn->impl_type.empty())
        emit_main_init(fn);

    for (size_t i = 0; i < fn->body->statements.size(); i++)
        gen_stmt(fn->body->statements[i], 1);

    pop_scope_reclaims(1);
    pop_scope();
    _out.put("}\n\n");
}

void CodeGen::emit_main_init(FuncDecl*) {
    for (size_t i = 0; i < _program->globals.size(); i++) {
        GlobalDecl* g = _program->globals[i];
        _out.indent(1);
        _out.put("%s = ", g->name.c_str());
        gen_expr(g->init);
        _out.put(";\n");
    }
    for (size_t i = 0; i < _program->signals.size(); i++) {
        _out.indent(1);
        _out.put("tick_signal_init(&%s);\n", _program->signals[i]->name.c_str());
    }
    for (size_t i = 0; i < _program->events.size(); i++) {
        EventDecl* ev = _program->events[i];
        int n = 0;
        for (size_t j = 0; j < _program->processes.size(); j++)
            if (_program->processes[j]->event_name == ev->name) n++;
        _out.indent(1);
        _out.put("tick_event_init(&%s, %d);\n", ev->name.c_str(), n);
        for (size_t j = 0; j < _program->processes.size(); j++)
            if (_program->processes[j]->event_name == ev->name) {
                _out.indent(1);
                _out.put("tick_event_bind(&%s, tk_proc_%s);\n",
                         ev->name.c_str(), _program->processes[j]->name.c_str());
            }
    }
}

void CodeGen::emit_process(ProcessDecl* pr) {
    _out.put("void* tk_proc_%s(void* _arg) {\n", pr->name.c_str());
    _out.put("    (void)_arg;\n");
    _return_type = nullptr;
    push_scope();
    for (size_t i = 0; i < pr->body->statements.size(); i++)
        gen_stmt(pr->body->statements[i], 1);
    pop_scope_reclaims(1);
    pop_scope();
    _out.put("    return NULL;\n");
    _out.put("}\n\n");
}

// ---- statements ----

void CodeGen::gen_block(Block* block, int indent, bool new_scope) {
    if (new_scope) push_scope();
    for (size_t i = 0; i < block->statements.size(); i++)
        gen_stmt(block->statements[i], indent);
    if (new_scope) { pop_scope_reclaims(indent); pop_scope(); }
}

void CodeGen::gen_stmt(Node* stmt, int indent) {
    switch (stmt->kind) {
        case NodeKind::VAR_DECL: gen_var(static_cast<VarDecl*>(stmt), indent); break;
        case NodeKind::EXPR_STMT: {
            Node* e = static_cast<ExprStmt*>(stmt)->expr;
            if (e->kind == NodeKind::MATCH) { gen_match(static_cast<MatchExpr*>(e), indent); break; }
            _out.indent(indent);
            gen_expr(e);
            _out.put(";\n");
            break;
        }
        case NodeKind::BLOCK:
            _out.indent(indent); _out.put("{\n");
            gen_block(static_cast<Block*>(stmt), indent + 1, true);
            _out.indent(indent); _out.put("}\n");
            break;
        case NodeKind::IF: {
            IfStmt* s = static_cast<IfStmt*>(stmt);
            _out.indent(indent); _out.put("if (");
            gen_expr(s->cond);
            _out.put(") {\n");
            gen_block(s->then_branch, indent + 1, true);
            _out.indent(indent); _out.put("}");
            if (s->else_branch) {
                _out.put(" else ");
                if (s->else_branch->kind == NodeKind::IF) {
                    _out.put("\n");
                    gen_stmt(s->else_branch, indent);
                } else {
                    _out.put("{\n");
                    gen_block(static_cast<Block*>(s->else_branch), indent + 1, true);
                    _out.indent(indent); _out.put("}\n");
                }
            } else {
                _out.put("\n");
            }
            break;
        }
        case NodeKind::WHILE: {
            WhileStmt* s = static_cast<WhileStmt*>(stmt);
            _out.indent(indent); _out.put("while (");
            gen_expr(s->cond);
            _out.put(") {\n");
            gen_block(s->body, indent + 1, true);
            _out.indent(indent); _out.put("}\n");
            break;
        }
        case NodeKind::FOR: {
            ForStmt* s = static_cast<ForStmt*>(stmt);
            if (s->iterable->kind == NodeKind::RANGE) {
                RangeExpr* r = static_cast<RangeExpr*>(s->iterable);
                _out.indent(indent);
                _out.put("for (int32_t %s = ", s->var_name.c_str());
                gen_expr(r->start);
                _out.put("; %s < ", s->var_name.c_str());
                gen_expr(r->end);
                _out.put("; %s++) {\n", s->var_name.c_str());
                gen_block(s->body, indent + 1, true);
                _out.indent(indent); _out.put("}\n");
            } else {
                // iterate an array by value or by ref
                TypeRef* arr_t = s->iterable->resolved_type;
                String et = (arr_t && arr_t->inner) ? c_type_str(arr_t->inner) : String("int32_t");
                bool iter_is_ptr = arr_t && arr_t->ownership == Ownership::REF;
                _out.indent(indent); _out.put("{\n");
                _out.indent(indent + 1);
                _out.put("TickArray* __it = %s(", iter_is_ptr ? "" : "&");
                gen_expr(s->iterable);
                _out.put(");\n");
                _out.indent(indent + 1);
                _out.put("for (int32_t __i = 0; __i < __it->len; __i++) {\n");
                _out.indent(indent + 2);
                if (s->bind_ref)
                    _out.put("%s* %s = (%s*)tick_array_at_unchecked(__it, __i);\n",
                             et.c_str(), s->var_name.c_str(), et.c_str());
                else
                    _out.put("%s %s = *(%s*)tick_array_at_unchecked(__it, __i);\n",
                             et.c_str(), s->var_name.c_str(), et.c_str());
                gen_block(s->body, indent + 3, true);
                _out.indent(indent + 1); _out.put("}\n");
                _out.indent(indent); _out.put("}\n");
            }
            break;
        }
        case NodeKind::RETURN: {
            ReturnStmt* s = static_cast<ReturnStmt*>(stmt);
            if (s->value) {
                String rt = c_type_str(_return_type);
                _out.indent(indent); _out.put("{ %s __ret = ", rt.c_str());
                gen_expr(s->value);
                _out.put(";\n");
                emit_all_reclaims(indent + 1);
                _out.indent(indent + 1); _out.put("return __ret;\n");
                _out.indent(indent); _out.put("}\n");
            } else {
                emit_all_reclaims(indent);
                _out.indent(indent); _out.put("return;\n");
            }
            break;
        }
        case NodeKind::BREAK:
            _out.indent(indent); _out.put("break;\n");
            break;
        case NodeKind::CONTINUE:
            _out.indent(indent); _out.put("continue;\n");
            break;
        case NodeKind::DEFER:
            // Register for LIFO execution at the end of the current scope
            // (pop_scope_reclaims / emit_all_reclaims run these).
            _defers[_scope].push(static_cast<DeferStmt*>(stmt)->stmt);
            break;
        case NodeKind::UNSAFE_BLOCK:
            gen_block(static_cast<UnsafeBlock*>(stmt)->body, indent, true);
            break;
        case NodeKind::MATCH:
            gen_match(static_cast<MatchExpr*>(stmt), indent);
            break;
        default:
            _out.indent(indent);
            gen_expr(stmt);
            _out.put(";\n");
            break;
    }
}

void CodeGen::gen_var(VarDecl* d, int indent) {
    TypeRef* t = d->declared_type ? d->declared_type
               : (d->init ? d->init->resolved_type : nullptr);
    String ct = c_type_str(t);
    _out.indent(indent);
    if (t && t->kind == TypeKind::FIXED_ARRAY) {
        String et = c_type_str(t->inner);
        _out.put("%s %s[%d]", et.c_str(), d->name.c_str(), t->fixed_size);
        if (!d->init) _out.put(" = {0}");
    } else {
        if (d->is_const) _out.put("const ");
        _out.put("%s %s", ct.c_str(), d->name.c_str());
        if (d->init) {
            _out.put(" = ");
            // Preserve value semantics for owned heap data so each binding is an
            // independent owner (no aliasing, no double free):
            //   shared : copying an existing box bumps the refcount
            //   array  : deep value copy of the buffer
            //   str    : duplicate the characters
            // A freshly produced value (boxed shared, array literal, concat, etc.)
            // is already uniquely owned, so it is bound directly.
            bool init_is_lvalue = d->init->kind == NodeKind::IDENT ||
                                  d->init->kind == NodeKind::FIELD ||
                                  d->init->kind == NodeKind::INDEX ||
                                  d->init->kind == NodeKind::SELF;
            if (t && t->ownership == Ownership::SHARED && d->init->kind != NodeKind::SHARED_EXPR) {
                _out.put("tick_shared_retain("); gen_expr(d->init); _out.put(")");
            } else if (t && t->ownership == Ownership::VALUE && t->kind == TypeKind::ARRAY && init_is_lvalue) {
                _out.put("tick_array_copy("); gen_expr(d->init); _out.put(")");
            } else if (t && t->ownership == Ownership::VALUE && t->kind == TypeKind::STR && init_is_lvalue) {
                _out.put("tick_str_dup("); gen_expr(d->init); _out.put(")");
            } else {
                gen_expr(d->init);
            }
        } else {
            _out.put(" = 0");
        }
    }
    _out.put(";\n");

    // Reclaim owned heap bindings at scope exit, unless moved out. A `str`
    // binding only owns heap memory when its initializer produced some (literals
    // are static; a borrowed ref owns nothing).
    if (_checker->needs_reclaim(d) && !_checker->is_move(d)) {
        if (t && t->kind == TypeKind::STR) {
            if (d->init && d->init->kind != NodeKind::STRING_LIT)
                track_owned(d->name, t);
        } else {
            track_owned(d->name, t);
        }
    }
}

// ---- expressions ----

void CodeGen::gen_expr(Node* expr) {
    if (!expr) { _out.put("0"); return; }
    switch (expr->kind) {
        case NodeKind::INT_LIT: _out.put("%lld", static_cast<IntLit*>(expr)->value); break;
        case NodeKind::FLOAT_LIT: _out.put("%g", static_cast<FloatLit*>(expr)->value); break;
        case NodeKind::BOOL_LIT: _out.put(static_cast<BoolLit*>(expr)->value ? "true" : "false"); break;
        case NodeKind::STRING_LIT: {
            StringLit* s = static_cast<StringLit*>(expr);
            _out.put("\"");
            for (size_t i = 0; i < s->value.length(); i++) {
                char c = s->value[i];
                if (c == '"') _out.put("\\\"");
                else if (c == '\\') _out.put("\\\\");
                else if (c == '\n') _out.put("\\n");
                else if (c == '\t') _out.put("\\t");
                else _out.put("%c", c);
            }
            _out.put("\"");
            break;
        }
        case NodeKind::SELF: _out.put("self"); break;
        case NodeKind::IDENT: {
            Ident* id = static_cast<Ident*>(expr);
            // a bare enum variant lowers to its Enum_Variant constant
            TypeRef* rt = id->resolved_type;
            if (rt && rt->kind == TypeKind::NAMED) {
                for (size_t i = 0; i < _program->enums.size(); i++) {
                    EnumDecl* en = _program->enums[i];
                    if (en->name != rt->name) continue;
                    for (size_t j = 0; j < en->variants.size(); j++) {
                        if (en->variants[j]->name == id->name) {
                            _out.put("%s_%s", en->name.c_str(), id->name.c_str());
                            goto ident_done;
                        }
                    }
                    break;
                }
            }
            _out.put("%s", id->name.c_str());
            ident_done:;
            break;
        }
        case NodeKind::BINARY: {
            Binary* b = static_cast<Binary*>(expr);
            TypeRef* lt = b->left->resolved_type;
            TypeRef* rt = b->right->resolved_type;
            bool str_op = (lt && lt->kind == TypeKind::STR) || (rt && rt->kind == TypeKind::STR);
            if (str_op && b->op == "+") {
                _out.put("tick_str_concat("); gen_expr(b->left); _out.put(", "); gen_expr(b->right); _out.put(")");
            } else if (str_op && b->op == "==") {
                _out.put("tick_str_eq("); gen_expr(b->left); _out.put(", "); gen_expr(b->right); _out.put(")");
            } else if (str_op && b->op == "!=") {
                _out.put("(!tick_str_eq("); gen_expr(b->left); _out.put(", "); gen_expr(b->right); _out.put("))");
            } else {
                _out.put("("); gen_expr(b->left); _out.put(" %s ", b->op.c_str()); gen_expr(b->right); _out.put(")");
            }
            break;
        }
        case NodeKind::UNARY: {
            Unary* u = static_cast<Unary*>(expr);
            _out.put("%s", u->op.c_str());
            gen_expr(u->operand);
            break;
        }
        case NodeKind::ASSIGN: {
            Assign* a = static_cast<Assign*>(expr);
            gen_expr(a->target); _out.put(" = "); gen_expr(a->value);
            break;
        }
        case NodeKind::COMPOUND_ASSIGN: {
            CompoundAssign* a = static_cast<CompoundAssign*>(expr);
            gen_expr(a->target); _out.put(" %s= ", a->op.c_str()); gen_expr(a->value);
            break;
        }
        case NodeKind::CALL: gen_call(static_cast<Call*>(expr)); break;
        case NodeKind::METHOD_CALL: gen_method_call(static_cast<MethodCall*>(expr)); break;
        case NodeKind::FIELD: {
            Field* f = static_cast<Field*>(expr);
            // qualified enum value
            if (f->object->kind == NodeKind::IDENT) {
                const String& base = static_cast<Ident*>(f->object)->name;
                for (size_t i = 0; i < _program->enums.size(); i++)
                    if (_program->enums[i]->name == base) {
                        _out.put("%s_%s", base.c_str(), f->name.c_str());
                        return;
                    }
            }
            TypeRef* ot = f->object->resolved_type;
            bool ptr = ot && (ot->ownership == Ownership::REF || ot->ownership == Ownership::SHARED ||
                              ot->ownership == Ownership::WEAK);
            gen_expr(f->object);
            _out.put(ptr ? "->%s" : ".%s", f->name.c_str());
            break;
        }
        case NodeKind::INDEX: {
            Index* ix = static_cast<Index*>(expr);
            TypeRef* at = ix->array->resolved_type;
            TypeRef* et = at ? at->inner : nullptr;
            String ets = c_type_str(et);
            if (at && at->kind == TypeKind::ARRAY) {
                bool recv_ptr = at->ownership == Ownership::REF;
                _out.put("(*(%s*)%s(%s", ets.c_str(),
                         checks_on() ? "tick_array_at" : "tick_array_at_unchecked",
                         recv_ptr ? "" : "&");
                gen_expr(ix->array);
                _out.put(", ");
                gen_expr(ix->index);
                _out.put("))");
            } else {
                gen_expr(ix->array); _out.put("["); gen_expr(ix->index); _out.put("]");
            }
            break;
        }
        case NodeKind::STRUCT_LIT: {
            StructLit* lit = static_cast<StructLit*>(expr);
            StructDecl* st = nullptr;
            for (size_t i = 0; i < _program->structs.size(); i++)
                if (_program->structs[i]->name == lit->type_name) st = _program->structs[i];
            _out.put("(%s){", lit->type_name.c_str());
            // emit in declared field order
            for (size_t i = 0; i < st->fields.size(); i++) {
                if (i) _out.put(", ");
                _out.put(".%s = ", st->fields[i]->name.c_str());
                Node* val = nullptr;
                for (size_t j = 0; j < lit->fields.size(); j++)
                    if (lit->fields[j].name == st->fields[i]->name) val = lit->fields[j].value;
                if (val) gen_expr(val); else _out.put("0");
            }
            _out.put("}");
            break;
        }
        case NodeKind::ARRAY_LIT: {
            ArrayLit* lit = static_cast<ArrayLit*>(expr);
            TypeRef* t = lit->resolved_type;
            String et = (t && t->inner) ? c_type_str(t->inner) : String("int32_t");
            _out.put("({ TickArray __a = tick_array_new(sizeof(%s)); ", et.c_str());
            for (size_t i = 0; i < lit->elements.size(); i++) {
                _out.put("{ %s __e = ", et.c_str());
                gen_expr(lit->elements[i]);
                _out.put("; tick_array_push(&__a, &__e); } ");
            }
            _out.put("__a; })");
            break;
        }
        case NodeKind::CAST: {
            CastExpr* c = static_cast<CastExpr*>(expr);
            String ct = c_type_str(c->target);
            _out.put("(%s)(", ct.c_str());
            gen_expr(c->operand);
            _out.put(")");
            break;
        }
        case NodeKind::SIZEOF: {
            SizeofExpr* s = static_cast<SizeofExpr*>(expr);
            String ct = c_type_str(s->target);
            _out.put("sizeof(%s)", ct.c_str());
            break;
        }
        case NodeKind::REF_EXPR: {
            RefExpr* r = static_cast<RefExpr*>(expr);
            _out.put("(&("); gen_expr(r->operand); _out.put("))");
            break;
        }
        case NodeKind::SHARED_EXPR: {
            // wrap a value into a shared box
            SharedExpr* s = static_cast<SharedExpr*>(expr);
            TypeRef* ot = s->operand->resolved_type;
            String it = c_type_str(ot);
            _out.put("({ %s* __p = (%s*)tick_shared_new(sizeof(%s)); *__p = ",
                     it.c_str(), it.c_str(), it.c_str());
            gen_expr(s->operand);
            _out.put("; __p; })");
            break;
        }
        case NodeKind::WEAK_EXPR: {
            WeakExpr* w = static_cast<WeakExpr*>(expr);
            _out.put("tick_weak_from("); gen_expr(w->operand); _out.put(")");
            break;
        }
        case NodeKind::RANGE: {
            // ranges only appear in for; defensive fallback
            RangeExpr* r = static_cast<RangeExpr*>(expr);
            gen_expr(r->start);
            break;
        }
        case NodeKind::MATCH:
            // match as an expression is lowered to a statement-expression
            // handled by gen_match for statement position; expression position
            // is not supported in this version.
            _out.put("0");
            break;
        default:
            _out.put("0");
            break;
    }
}

void CodeGen::gen_call(Call* call) {
    const String& n = call->callee;

    // built-ins
    if (n == "println" || n == "print") {
        // print a single argument by inferred type
        Node* a = call->args.size() ? call->args[0].value : nullptr;
        TypeRef* t = a ? a->resolved_type : nullptr;
        const char* nl = (n == "println") ? "\\n" : "";
        if (!a) { _out.put("printf(\"%s\")", nl); return; }
        if (t && t->kind == TypeKind::STR) {
            _out.put("printf(\"%%s%s\", ", nl); gen_expr(a); _out.put(")");
        } else if (t && t->kind == TypeKind::FLOAT) {
            _out.put("printf(\"%%g%s\", (double)(", nl); gen_expr(a); _out.put("))");
        } else if (t && t->kind == TypeKind::BOOL) {
            _out.put("printf(\"%%s%s\", (", nl); gen_expr(a); _out.put(") ? \"true\" : \"false\")");
        } else {
            _out.put("printf(\"%%lld%s\", (long long)(", nl); gen_expr(a); _out.put("))");
        }
        return;
    }
    if (n == "to_str") {
        Node* a = call->args[0].value;
        TypeRef* t = a->resolved_type;
        if (t && t->kind == TypeKind::FLOAT) { _out.put("tick_str_from_f64((double)("); gen_expr(a); _out.put("))"); }
        else if (t && t->kind == TypeKind::BOOL) { _out.put("tick_str_from_bool("); gen_expr(a); _out.put(")"); }
        else if (t && t->kind == TypeKind::INT && t->int_unsigned) { _out.put("tick_str_from_u64((uint64_t)("); gen_expr(a); _out.put("))"); }
        else { _out.put("tick_str_from_i64((int64_t)("); gen_expr(a); _out.put("))"); }
        return;
    }
    if (n == "len") {
        Node* a = call->args[0].value;
        TypeRef* t = a->resolved_type;
        if (t && t->kind == TypeKind::STR) { _out.put("tick_str_len("); gen_expr(a); _out.put(")"); }
        else { _out.put("("); gen_expr(a); _out.put(").len"); }
        return;
    }
    if (n == "str_order") {
        _out.put("tick_str_order("); gen_expr(call->args[0].value);
        _out.put(", "); gen_expr(call->args[1].value); _out.put(")");
        return;
    }
    struct { const char* tk; const char* c; } math1[] = {
        {"sqrt","sqrt"},{"sin","sin"},{"cos","cos"},{"tan","tan"},
        {"floor","floor"},{"ceil","ceil"},{"round","round"},{nullptr,nullptr}
    };
    for (int i = 0; math1[i].tk; i++) if (n == math1[i].tk) {
        _out.put("%s((double)(", math1[i].c); gen_expr(call->args[0].value); _out.put("))"); return;
    }
    if (n == "pow") { _out.put("pow((double)("); gen_expr(call->args[0].value); _out.put("), (double)("); gen_expr(call->args[1].value); _out.put("))"); return; }
    if (n == "min") { _out.put("fmin((double)("); gen_expr(call->args[0].value); _out.put("), (double)("); gen_expr(call->args[1].value); _out.put("))"); return; }
    if (n == "max") { _out.put("fmax((double)("); gen_expr(call->args[0].value); _out.put("), (double)("); gen_expr(call->args[1].value); _out.put("))"); return; }
    if (n == "abs") { _out.put("llabs((long long)("); gen_expr(call->args[0].value); _out.put("))"); return; }

    // struct constructor (positional)
    for (size_t i = 0; i < _program->structs.size(); i++) {
        StructDecl* st = _program->structs[i];
        if (st->name != n) continue;
        _out.put("(%s){", n.c_str());
        for (size_t j = 0; j < st->fields.size(); j++) {
            if (j) _out.put(", ");
            _out.put(".%s = ", st->fields[j]->name.c_str());
            if (j < call->args.size()) gen_expr(call->args[j].value); else _out.put("0");
        }
        _out.put("}");
        return;
    }

    // user function or extern
    FuncDecl* fn_decl = nullptr;
    bool is_extern = false;
    for (size_t i = 0; i < _program->externs.size(); i++)
        if (_program->externs[i]->name == n) { is_extern = true; break; }
    if (!is_extern)
        for (size_t i = 0; i < _program->functions.size(); i++)
            if (_program->functions[i]->name == n) { fn_decl = _program->functions[i]; break; }
    String cname = is_extern ? n : mangle_func(n);
    _out.put("%s(", cname.c_str());
    for (size_t i = 0; i < call->args.size(); i++) {
        if (i) _out.put(", ");
        bool param_by_ref = fn_decl && i < fn_decl->params.size() &&
                            fn_decl->params[i]->type->ownership == Ownership::REF;
        if (param_by_ref) {
            TypeRef* at = call->args[i].value->resolved_type;
            bool already_ptr = at && (at->ownership == Ownership::REF ||
                                      at->ownership == Ownership::SHARED ||
                                      at->ownership == Ownership::WEAK);
            if (already_ptr) gen_expr(call->args[i].value);
            else { _out.put("&("); gen_expr(call->args[i].value); _out.put(")"); }
        } else {
            gen_expr(call->args[i].value);
        }
    }
    _out.put(")");
}

void CodeGen::gen_method_call(MethodCall* mc) {
    TypeRef* rt = mc->receiver->resolved_type;

    // array methods. A ref-typed receiver is already a TickArray*.
    if (rt && rt->kind == TypeKind::ARRAY) {
        bool recv_ptr = rt->ownership == Ownership::REF;
        const char* deref = recv_ptr ? "" : "&";
        const char* dot = recv_ptr ? "->" : ".";
        if (mc->method == "len") {
            _out.put("("); gen_expr(mc->receiver); _out.put(")%slen", dot); return;
        }
        if (mc->method == "push") {
            String et = c_type_str(rt->inner);
            _out.put("({ %s __e = ", et.c_str());
            gen_expr(mc->args[0].value);
            _out.put("; tick_array_push(%s(", deref); gen_expr(mc->receiver); _out.put("), &__e); })");
            return;
        }
        if (mc->method == "pop") {
            String et = c_type_str(rt->inner);
            _out.put("({ TickArray* __a = %s(", deref); gen_expr(mc->receiver);
            _out.put("); __a->len--; *(%s*)tick_array_at_unchecked(__a, __a->len); })", et.c_str());
            return;
        }
    }
    if (rt && rt->kind == TypeKind::STR && mc->method == "len") {
        _out.put("tick_str_len("); gen_expr(mc->receiver); _out.put(")"); return;
    }

    // signal methods
    if (mc->receiver->kind == NodeKind::IDENT) {
        const String& rn = static_cast<Ident*>(mc->receiver)->name;
        for (size_t i = 0; i < _program->signals.size(); i++) {
            if (_program->signals[i]->name != rn) continue;
            TypeRef* pt = _program->signals[i]->payload_type;
            String pts = c_type_str(pt);
            if (mc->method == "emit") {
                _out.put("({ %s* __v = (%s*)malloc(sizeof(%s)); *__v = ", pts.c_str(), pts.c_str(), pts.c_str());
                gen_expr(mc->args[0].value);
                _out.put("; tick_signal_emit(&%s, __v); })", rn.c_str());
                return;
            }
            if (mc->method == "recv") {
                _out.put("({ %s* __v = (%s*)tick_signal_recv(&%s); %s __r = *__v; free(__v); __r; })",
                         pts.c_str(), pts.c_str(), rn.c_str(), pts.c_str());
                return;
            }
        }
        for (size_t i = 0; i < _program->events.size(); i++) {
            if (_program->events[i]->name == rn && mc->method == "fire") {
                _out.put("tick_event_fire(&%s)", rn.c_str());
                return;
            }
        }
    }

    // user method
    if (rt && (rt->kind == TypeKind::NAMED || rt->kind == TypeKind::DYN)) {
        FuncDecl* m = nullptr;
        for (size_t i = 0; i < _program->impls.size() && !m; i++)
            if (_program->impls[i]->type_name == rt->name)
                for (size_t j = 0; j < _program->impls[i]->methods.size(); j++)
                    if (_program->impls[i]->methods[j]->name == mc->method)
                        m = _program->impls[i]->methods[j];
        if (m) {
            String cname = mangle_method(rt->name, mc->method);
            _out.put("%s(", cname.c_str());
            bool by_ref = (m->self_kind == SelfKind::REF);
            bool recv_is_ptr = rt->ownership == Ownership::REF || rt->ownership == Ownership::SHARED;
            if (by_ref && !recv_is_ptr) { _out.put("&("); gen_expr(mc->receiver); _out.put(")"); }
            else { gen_expr(mc->receiver); }
            for (size_t i = 0; i < mc->args.size(); i++) {
                _out.put(", ");
                gen_expr(mc->args[i].value);
            }
            _out.put(")");
            return;
        }
    }

    _out.put("0");
}

void CodeGen::gen_match(MatchExpr* m, int indent) {
    TypeRef* st = m->subject->resolved_type;
    String enum_name = (st && st->kind == TypeKind::NAMED) ? st->name : String("");

    _out.indent(indent);
    _out.put("{ int32_t __m = (int32_t)(");
    gen_expr(m->subject);
    _out.put(");\n");
    for (size_t i = 0; i < m->arms.size(); i++) {
        MatchArm* arm = m->arms[i];
        _out.indent(indent + 1);
        if (arm->is_wildcard) {
            _out.put("{\n");
        } else {
            _out.put("if (__m == %s_%s) {\n", enum_name.c_str(), arm->variant.c_str());
        }
        gen_block(static_cast<Block*>(arm->body), indent + 2, true);
        _out.indent(indent + 1);
        _out.put("}%s\n", (i + 1 < m->arms.size() && !arm->is_wildcard) ? " else" : "");
    }
    _out.indent(indent);
    _out.put("}\n");
}

}
