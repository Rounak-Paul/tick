#include "compiler.h"
#include "../compiler/lexer.h"
#include "../compiler/parser.h"
#include "../compiler/semantic_analyzer.h"
#include "../compiler/module_loader.h"
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

using namespace Tick;

FunctionDecl* Compiler::_current_func = nullptr;
ClassDecl* Compiler::_current_class = nullptr;
ProcessDecl* Compiler::_current_process = nullptr;
char Compiler::_defines[64][128] = {};
int Compiler::_define_count = 0;
StmtNode* Compiler::_defer_scopes[MAX_DEFER_SCOPES][MAX_DEFERS_PER_SCOPE] = {};
int Compiler::_defer_counts[MAX_DEFER_SCOPES] = {};
int Compiler::_defer_depth = -1;
String Compiler::_expected_type;
RaiiEntry Compiler::_raii_scopes[MAX_DEFER_SCOPES][MAX_RAII_PER_SCOPE] = {};
int Compiler::_raii_counts[MAX_DEFER_SCOPES] = {};
String Compiler::_array_scopes[MAX_DEFER_SCOPES][MAX_ARRAYS_PER_SCOPE] = {};
int Compiler::_array_counts[MAX_DEFER_SCOPES] = {};

bool Compiler::is_string_type(ExprNode* expr, Program* program) {
    return infer_expr_type(expr, program) == "str";
}

void Compiler::add_define(const char* name) {
    if (_define_count < 64) {
        snprintf(_defines[_define_count], 128, "%s", name);
        _define_count++;
    }
}

bool Compiler::is_fixed_array_type(const String& t) {
    int len = (int)t.length();
    if (len < 4) return false;
    if (t[len - 1] != ']') return false;
    int i = len - 2;
    while (i > 0 && t[i] >= '0' && t[i] <= '9') i--;
    return t[i] == '[' && i > 0 && i < len - 2;
}

String Compiler::fixed_array_base_type(const String& t) {
    int i = (int)t.length() - 1;
    while (i > 0 && t[i] != '[') i--;
    return String(t.c_str(), i);
}

String Compiler::fixed_array_size_str(const String& t) {
    int i = (int)t.length() - 1;
    while (i > 0 && t[i] != '[') i--;
    return String(t.c_str() + i + 1, t.length() - i - 2);
}

bool Compiler::is_array_type_str(const String& t) {
    return t.length() > 2 && t[t.length() - 2] == '[' && t[t.length() - 1] == ']';
}

bool Compiler::is_array_param(const String& name) {
    if (_current_func) {
        for (size_t i = 0; i < _current_func->parameters.size(); i++) {
            if (_current_func->parameters[i]->name == name &&
                is_array_type_str(_current_func->parameters[i]->type_name)) {
                return true;
            }
        }
    }
    return false;
}

String Compiler::mangle(const String& name) {
    if (name == "main") return name;
    char buf[256];
    snprintf(buf, sizeof(buf), "_t_%s", name.c_str());
    return String(buf);
}

bool Compiler::is_typed_ptr_type(const String& t) {
    return t.length() > 4 && t[0] == 'p' && t[1] == 't' && t[2] == 'r' && t[3] == '<' && t[t.length() - 1] == '>';
}

String Compiler::typed_ptr_base_type(const String& t) {
    if (is_typed_ptr_type(t)) {
        return String(t.c_str() + 4, t.length() - 5);
    }
    return t;
}

bool Compiler::is_func_ptr_type(const String& t) {
    return t.length() > 6 && t[0] == 'f' && t[1] == 'u' && t[2] == 'n' && t[3] == 'c' && t[4] == '(';
}

void Compiler::push_defer_scope() {
    _defer_depth++;
    if (_defer_depth < MAX_DEFER_SCOPES) {
        _defer_counts[_defer_depth] = 0;
        _raii_counts[_defer_depth] = 0;
        _array_counts[_defer_depth] = 0;
    }
}

void Compiler::pop_defer_scope() {
    if (_defer_depth >= 0) {
        _defer_depth--;
    }
}

void Compiler::generate_deferred(CodeBuffer& buf, int indent, Program* program) {
    if (_defer_depth < 0) return;
    generate_raii_cleanup(buf, indent, program);
    int count = _defer_counts[_defer_depth];
    for (int i = count - 1; i >= 0; i--) {
        generate_statement(buf, _defer_scopes[_defer_depth][i], indent, program);
    }
}

void Compiler::generate_all_deferred(CodeBuffer& buf, int indent, Program* program) {
    for (int s = _defer_depth; s >= 0; s--) {
        int rc = _raii_counts[s];
        for (int i = rc - 1; i >= 0; i--) {
            RaiiEntry& entry = _raii_scopes[s][i];
            for (int t = 0; t < indent; t++) buf.append("    ");
            buf.append("if (%s) { %s_dtor(%s); free(%s); %s = NULL; }\n", entry.var_name.c_str(), entry.class_name.c_str(), entry.var_name.c_str(), entry.var_name.c_str(), entry.var_name.c_str());
        }
        int ac = _array_counts[s];
        for (int i = ac - 1; i >= 0; i--) {
            const char* n = _array_scopes[s][i].c_str();
            for (int t = 0; t < indent; t++) buf.append("    ");
            buf.append("if (%s.ptr) { free(%s.ptr); %s.ptr = NULL; }\n", n, n, n);
        }
        int count = _defer_counts[s];
        for (int i = count - 1; i >= 0; i--) {
            generate_statement(buf, _defer_scopes[s][i], indent, program);
        }
    }
}

void Compiler::generate_raii_cleanup(CodeBuffer& buf, int indent, Program* program) {
    if (_defer_depth < 0) return;
    int rc = _raii_counts[_defer_depth];
    for (int i = rc - 1; i >= 0; i--) {
        RaiiEntry& entry = _raii_scopes[_defer_depth][i];
        for (int t = 0; t < indent; t++) buf.append("    ");
        buf.append("if (%s) { %s_dtor(%s); free(%s); %s = NULL; }\n", entry.var_name.c_str(), entry.class_name.c_str(), entry.var_name.c_str(), entry.var_name.c_str(), entry.var_name.c_str());
    }
    int ac = _array_counts[_defer_depth];
    for (int i = ac - 1; i >= 0; i--) {
        const char* n = _array_scopes[_defer_depth][i].c_str();
        for (int t = 0; t < indent; t++) buf.append("    ");
        buf.append("if (%s.ptr) { free(%s.ptr); %s.ptr = NULL; }\n", n, n, n);
    }
}

void Compiler::generate_all_raii_cleanup(CodeBuffer& buf, int indent, Program* program) {
    for (int s = _defer_depth; s >= 0; s--) {
        int rc = _raii_counts[s];
        for (int i = rc - 1; i >= 0; i--) {
            RaiiEntry& entry = _raii_scopes[s][i];
            for (int t = 0; t < indent; t++) buf.append("    ");
            buf.append("if (%s) { %s_dtor(%s); free(%s); %s = NULL; }\n", entry.var_name.c_str(), entry.class_name.c_str(), entry.var_name.c_str(), entry.var_name.c_str(), entry.var_name.c_str());
        }
        int ac = _array_counts[s];
        for (int i = ac - 1; i >= 0; i--) {
            const char* n = _array_scopes[s][i].c_str();
            for (int t = 0; t < indent; t++) buf.append("    ");
            buf.append("if (%s.ptr) { free(%s.ptr); %s.ptr = NULL; }\n", n, n, n);
        }
    }
}

bool Compiler::class_has_destructor(const String& class_name, Program* program) {
    for (size_t i = 0; i < program->methods.size(); i++) {
        if (program->methods[i]->class_name == class_name && program->methods[i]->is_destructor) {
            return true;
        }
    }
    return false;
}

ClassDecl* Compiler::find_class(const String& name, Program* program) {
    for (size_t i = 0; i < program->classes.size(); i++) {
        if (program->classes[i]->name == name) return program->classes[i];
    }
    return nullptr;
}

FunctionDecl* Compiler::find_method(const String& class_name, const String& method_name, Program* program) {
    for (size_t i = 0; i < program->methods.size(); i++) {
        if (program->methods[i]->class_name == class_name && program->methods[i]->name == method_name) {
            return program->methods[i];
        }
    }
    ClassDecl* cls = find_class(class_name, program);
    if (cls && !cls->base_class.empty()) {
        return find_method(cls->base_class, method_name, program);
    }
    return nullptr;
}

CodeBuffer::CodeBuffer() : pos(0), capacity(1048576) {
    data = (char*)malloc(capacity);
}

CodeBuffer::~CodeBuffer() {
    free(data);
}

void CodeBuffer::ensure(int needed) {
    while (pos + needed >= capacity) {
        capacity *= 2;
        data = (char*)realloc(data, capacity);
    }
}

int CodeBuffer::append(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    ensure(needed + 1);

    va_start(args, fmt);
    int written = vsnprintf(data + pos, capacity - pos, fmt, args);
    va_end(args);
    pos += written;
    return written;
}

void Compiler::lookup_var_type_in_block(const String& name, BlockStmt* block, String& result) {
    if (!block || !result.empty()) return;
    for (size_t i = 0; i < block->statements.size(); i++) {
        StmtNode* s = block->statements[i];
        if (s->type == AstNodeType::VAR_DECL) {
            VarDecl* v = static_cast<VarDecl*>(s);
            if (v->name == name) {
                result = v->type_name;
                return;
            }
        } else if (s->type == AstNodeType::BLOCK_STMT) {
            lookup_var_type_in_block(name, static_cast<BlockStmt*>(s), result);
        } else if (s->type == AstNodeType::IF_STMT) {
            IfStmt* ifs = static_cast<IfStmt*>(s);
            if (ifs->then_branch && ifs->then_branch->type == AstNodeType::BLOCK_STMT)
                lookup_var_type_in_block(name, static_cast<BlockStmt*>(ifs->then_branch), result);
            if (ifs->else_branch && ifs->else_branch->type == AstNodeType::BLOCK_STMT)
                lookup_var_type_in_block(name, static_cast<BlockStmt*>(ifs->else_branch), result);
        } else if (s->type == AstNodeType::WHILE_STMT) {
            WhileStmt* ws = static_cast<WhileStmt*>(s);
            if (ws->body && ws->body->type == AstNodeType::BLOCK_STMT)
                lookup_var_type_in_block(name, static_cast<BlockStmt*>(ws->body), result);
        } else if (s->type == AstNodeType::FOR_STMT) {
            ForStmt* fs = static_cast<ForStmt*>(s);
            if (fs->initializer && fs->initializer->type == AstNodeType::VAR_DECL) {
                VarDecl* v = static_cast<VarDecl*>(fs->initializer);
                if (v->name == name) {
                    result = v->type_name;
                    return;
                }
            }
            if (fs->body && fs->body->type == AstNodeType::BLOCK_STMT)
                lookup_var_type_in_block(name, static_cast<BlockStmt*>(fs->body), result);
        }
        if (!result.empty()) return;
    }
}

String Compiler::lookup_var_type(const String& name, Program* program) {
    if (_current_func) {
        for (size_t i = 0; i < _current_func->parameters.size(); i++) {
            if (_current_func->parameters[i]->name == name) {
                return _current_func->parameters[i]->type_name;
            }
        }
        if (_current_func->body) {
            String result;
            lookup_var_type_in_block(name, _current_func->body, result);
            if (!result.empty()) return result;
        }
    }
    if (_current_process && _current_process->body) {
        String result;
        lookup_var_type_in_block(name, _current_process->body, result);
        if (!result.empty()) return result;
    }
    if (_current_class) {
        ClassDecl* cls = _current_class;
        for (int depth = 0; depth < 32 && cls; depth++) {
            for (size_t i = 0; i < cls->fields.size(); i++) {
                if (cls->fields[i]->name == name) {
                    return cls->fields[i]->type_name;
                }
            }
            if (!cls->base_class.empty()) {
                cls = find_class(cls->base_class, program);
            } else {
                cls = nullptr;
            }
        }
    }
    for (size_t i = 0; i < program->globals.size(); i++) {
        if (program->globals[i]->name == name) {
            return program->globals[i]->type_name;
        }
    }
    return String("");
}

String Compiler::infer_expr_type(ExprNode* expr, Program* program) {
    if (!expr) return String("i32");
    switch (expr->type) {
        case AstNodeType::INTEGER_LITERAL: return String("i32");
        case AstNodeType::FLOAT_LITERAL: return String("f32");
        case AstNodeType::DOUBLE_LITERAL: return String("f64");
        case AstNodeType::BOOL_LITERAL: return String("b8");
        case AstNodeType::STRING_LITERAL: return String("str");
        case AstNodeType::NULL_LITERAL: return String("ptr");
        case AstNodeType::CAST_EXPR: {
            CastExpr* ce = static_cast<CastExpr*>(expr);
            return ce->target_type;
        }
        case AstNodeType::SIZEOF_EXPR: return String("u64");
        case AstNodeType::IDENTIFIER_EXPR: {
            IdentifierExpr* id = static_cast<IdentifierExpr*>(expr);
            String vt = lookup_var_type(id->name, program);
            if (!vt.empty()) return vt;
            for (size_t i = 0; i < program->functions.size(); i++) {
                if (program->functions[i]->name == id->name) {
                    FunctionDecl* fn = program->functions[i];
                    char buf[512];
                    int pos = snprintf(buf, sizeof(buf), "func(");
                    for (size_t j = 0; j < fn->parameters.size(); j++) {
                        if (j > 0) pos += snprintf(buf + pos, sizeof(buf) - pos, ",");
                        pos += snprintf(buf + pos, sizeof(buf) - pos, "%s", fn->parameters[j]->type_name.c_str());
                    }
                    snprintf(buf + pos, sizeof(buf) - pos, "):%s", fn->return_type.c_str());
                    return String(buf);
                }
            }
            for (size_t i = 0; i < program->extern_functions.size(); i++) {
                if (program->extern_functions[i]->name == id->name) {
                    ExternFuncDecl* fn = program->extern_functions[i];
                    char buf[512];
                    int pos = snprintf(buf, sizeof(buf), "func(");
                    for (size_t j = 0; j < fn->parameters.size(); j++) {
                        if (j > 0) pos += snprintf(buf + pos, sizeof(buf) - pos, ",");
                        pos += snprintf(buf + pos, sizeof(buf) - pos, "%s", fn->parameters[j]->type_name.c_str());
                    }
                    snprintf(buf + pos, sizeof(buf) - pos, "):%s", fn->return_type.c_str());
                    return String(buf);
                }
            }
            for (size_t i = 0; i < program->enums.size(); i++) {
                for (size_t j = 0; j < program->enums[i]->values.size(); j++) {
                    String qualified;
                    qualified = program->enums[i]->name;
                    qualified = qualified + "_";
                    qualified = qualified + program->enums[i]->values[j]->name;
                    if (qualified == id->name) return String("i32");
                }
            }
            return String("i32");
        }
        case AstNodeType::POSTFIX_EXPR: {
            PostfixExpr* post = static_cast<PostfixExpr*>(expr);
            return infer_expr_type(post->operand, program);
        }
        case AstNodeType::MEMBER_EXPR: {
            MemberExpr* me = static_cast<MemberExpr*>(expr);
            if (me->member == "length") {
                return String("i32");
            }
            String obj_type = infer_expr_type(me->object, program);
            String search_type = obj_type;
            for (int depth = 0; depth < 32 && !search_type.empty(); depth++) {
                for (size_t i = 0; i < program->classes.size(); i++) {
                    if (program->classes[i]->name == search_type) {
                        for (size_t j = 0; j < program->classes[i]->fields.size(); j++) {
                            if (program->classes[i]->fields[j]->name == me->member) {
                                return program->classes[i]->fields[j]->type_name;
                            }
                        }
                        search_type = program->classes[i]->base_class;
                        goto next_member_depth;
                    }
                }
                break;
                next_member_depth:;
            }
            for (size_t i = 0; i < program->unions.size(); i++) {
                if (program->unions[i]->name == obj_type) {
                    for (size_t j = 0; j < program->unions[i]->fields.size(); j++) {
                        if (program->unions[i]->fields[j]->name == me->member) {
                            return program->unions[i]->fields[j]->type_name;
                        }
                    }
                }
            }
            return String("i32");
        }
        case AstNodeType::CALL_EXPR: {
            CallExpr* call = static_cast<CallExpr*>(expr);
            if (call->callee->type == AstNodeType::IDENTIFIER_EXPR) {
                IdentifierExpr* id = static_cast<IdentifierExpr*>(call->callee);
                if (id->name == "str_concat" || id->name == "str_substring" ||
                    id->name == "to_str" ||
                    id->name == "file_read" || id->name == "input") {
                    return String("str");
                }
                if (id->name == "str_index_of" ||
                    id->name == "str_compare" || id->name == "str_length") {
                    return String("i32");
                }
                if (id->name == "parse") return _expected_type.length() > 0 ? _expected_type : String("i64");
                if (id->name == "file_exists") return String("b8");
                if (id->name == "sqrt" || id->name == "sin" || id->name == "cos" ||
                    id->name == "tan" || id->name == "pow" || id->name == "floor" ||
                    id->name == "ceil" || id->name == "round" || id->name == "log" ||
                    id->name == "log2" || id->name == "log10" || id->name == "fmin" ||
                    id->name == "fmax") {
                    return String("f64");
                }
                if (id->name == "abs") return String("i32");
                if (id->name == "addr") {
                    if (call->arguments.size() > 0) {
                        String arg_type = infer_expr_type(call->arguments[0], program);
                        if (!arg_type.empty()) {
                            char buf[256];
                            snprintf(buf, sizeof(buf), "ptr<%s>", arg_type.c_str());
                            return String(buf);
                        }
                    }
                    return String("ptr");
                }
                if (id->name == "deref") {
                    if (call->arguments.size() > 0) {
                        String arg_type = infer_expr_type(call->arguments[0], program);
                        if (is_typed_ptr_type(arg_type)) {
                            return typed_ptr_base_type(arg_type);
                        }
                    }
                    return String("i32");
                }
                for (size_t i = 0; i < program->functions.size(); i++) {
                    if (program->functions[i]->name == id->name) {
                        return program->functions[i]->return_type;
                    }
                }
                for (size_t i = 0; i < program->extern_functions.size(); i++) {
                    if (program->extern_functions[i]->name == id->name) {
                        return program->extern_functions[i]->return_type;
                    }
                }
            }
            if (call->callee->type == AstNodeType::MEMBER_EXPR) {
                MemberExpr* me = static_cast<MemberExpr*>(call->callee);
                if (me->member == "length") return String("i32");
                if (me->member == "push") return String("void");
                if (me->member == "pop") {
                    String arr_type = infer_expr_type(me->object, program);
                    if (is_array_type_str(arr_type)) {
                        return String(arr_type.c_str(), arr_type.length() - 2);
                    }
                }
                if (me->member == "substring" || me->member == "index_of" ||
                    me->member == "char_at") {
                    return String("str");
                }
                if (me->member == "recv") return String("i32");
            }
            return String("i32");
        }
        case AstNodeType::BINARY_EXPR: {
            BinaryExpr* bin = static_cast<BinaryExpr*>(expr);
            if (bin->op == "==" || bin->op == "!=" || bin->op == "<" ||
                bin->op == ">" || bin->op == "<=" || bin->op == ">=" ||
                bin->op == "&&" || bin->op == "||") {
                return String("b8");
            }
            String left_t = infer_expr_type(bin->left, program);
            String right_t = infer_expr_type(bin->right, program);
            if (bin->op == "+" && (left_t == "str" || right_t == "str")) {
                return String("str");
            }
            if ((bin->op == "+" || bin->op == "-") && (is_typed_ptr_type(left_t) || left_t == "ptr")) {
                return left_t;
            }
            if (bin->op == "+" && (is_typed_ptr_type(right_t) || right_t == "ptr")) {
                return right_t;
            }
            return left_t;
        }
        case AstNodeType::INDEX_EXPR: {
            IndexExpr* idx = static_cast<IndexExpr*>(expr);
            String arr_type = infer_expr_type(idx->array, program);
            if (arr_type.length() > 2 &&
                arr_type[arr_type.length() - 2] == '[' && arr_type[arr_type.length() - 1] == ']') {
                return String(arr_type.c_str(), arr_type.length() - 2);
            }
            if (is_fixed_array_type(arr_type)) {
                return fixed_array_base_type(arr_type);
            }
            if (is_typed_ptr_type(arr_type)) {
                return typed_ptr_base_type(arr_type);
            }
            return String("i32");
        }
        default:
            return String("i32");
    }
}

void Compiler::tick_type_to_c_type(const String& tick_type, Program* program, char* out, size_t out_size) {
    if (tick_type == "void") { snprintf(out, out_size, "void"); return; }
    if (tick_type == "u8") { snprintf(out, out_size, "uint8_t"); return; }
    if (tick_type == "u16") { snprintf(out, out_size, "uint16_t"); return; }
    if (tick_type == "u32") { snprintf(out, out_size, "uint32_t"); return; }
    if (tick_type == "u64") { snprintf(out, out_size, "uint64_t"); return; }
    if (tick_type == "i8") { snprintf(out, out_size, "int8_t"); return; }
    if (tick_type == "i16") { snprintf(out, out_size, "int16_t"); return; }
    if (tick_type == "i32") { snprintf(out, out_size, "int32_t"); return; }
    if (tick_type == "i64") { snprintf(out, out_size, "int64_t"); return; }
    if (tick_type == "f32") { snprintf(out, out_size, "float"); return; }
    if (tick_type == "f64") { snprintf(out, out_size, "double"); return; }
    if (tick_type == "b8") { snprintf(out, out_size, "bool"); return; }
    if (tick_type == "str") { snprintf(out, out_size, "char*"); return; }
    if (tick_type == "ptr") { snprintf(out, out_size, "void*"); return; }

    if (is_typed_ptr_type(tick_type)) {
        String inner = typed_ptr_base_type(tick_type);
        char inner_c[128];
        tick_type_to_c_type(inner, program, inner_c, sizeof(inner_c));
        snprintf(out, out_size, "%s*", inner_c);
        return;
    }

    if (is_func_ptr_type(tick_type)) {
        const char* s = tick_type.c_str();
        int paren_start = 4;
        int depth = 1;
        int paren_end = paren_start + 1;
        while (s[paren_end] && depth > 0) {
            if (s[paren_end] == '(') depth++;
            else if (s[paren_end] == ')') depth--;
            if (depth > 0) paren_end++;
        }
        String ret_str(s + paren_end + 2);
        char ret_c[128];
        tick_type_to_c_type(ret_str, program, ret_c, sizeof(ret_c));
        char params_buf[512];
        int params_pos = 0;
        if (paren_end > paren_start + 1) {
            String params_str(s + paren_start + 1, paren_end - paren_start - 1);
            const char* p = params_str.c_str();
            int start = 0;
            int len = (int)params_str.length();
            int pd = 0;
            bool first = true;
            for (int i = 0; i <= len; i++) {
                if (i < len && p[i] == '(') pd++;
                else if (i < len && p[i] == ')') pd--;
                if ((i == len || (p[i] == ',' && pd == 0))) {
                    String param_type(p + start, i - start);
                    char param_c[128];
                    tick_type_to_c_type(param_type, program, param_c, sizeof(param_c));
                    if (!first) params_pos += snprintf(params_buf + params_pos, sizeof(params_buf) - params_pos, ", ");
                    params_pos += snprintf(params_buf + params_pos, sizeof(params_buf) - params_pos, "%s", param_c);
                    first = false;
                    start = i + 1;
                }
            }
        } else {
            params_pos += snprintf(params_buf + params_pos, sizeof(params_buf) - params_pos, "void");
        }
        params_buf[params_pos] = '\0';
        snprintf(out, out_size, "%s (*)(%s)", ret_c, params_buf);
        return;
    }

    if (is_fixed_array_type(tick_type)) {
        String base = fixed_array_base_type(tick_type);
        tick_type_to_c_type(base, program, out, out_size);
        return;
    }

    if (tick_type.length() > 2 && tick_type[tick_type.length() - 2] == '[' && tick_type[tick_type.length() - 1] == ']') {
        snprintf(out, out_size, "TickArray");
        return;
    }

    if (program) {
        for (size_t i = 0; i < program->classes.size(); i++) {
            if (program->classes[i]->name == tick_type) {
                if (program->classes[i]->is_dataclass) {
                    snprintf(out, out_size, "%s", tick_type.c_str());
                } else {
                    snprintf(out, out_size, "%s*", tick_type.c_str());
                }
                return;
            }
        }
        for (size_t i = 0; i < program->enums.size(); i++) {
            if (program->enums[i]->name == tick_type) {
                snprintf(out, out_size, "int32_t");
                return;
            }
        }
        for (size_t i = 0; i < program->unions.size(); i++) {
            if (program->unions[i]->name == tick_type) {
                snprintf(out, out_size, "%s", tick_type.c_str());
                return;
            }
        }
    }

    snprintf(out, out_size, "int32_t");
}

void Compiler::generate_typed_decl(CodeBuffer& buf, const String& tick_type, const char* name, Program* program) {
    if (is_fixed_array_type(tick_type)) {
        String base = fixed_array_base_type(tick_type);
        String size = fixed_array_size_str(tick_type);
        char c_base[128];
        tick_type_to_c_type(base, program, c_base, sizeof(c_base));
        buf.append("%s %s[%s]", c_base, name, size.c_str());
        return;
    }
    if (is_func_ptr_type(tick_type)) {
        const char* s = tick_type.c_str();
        int paren_start = 4;
        int depth = 1;
        int paren_end = paren_start + 1;
        while (s[paren_end] && depth > 0) {
            if (s[paren_end] == '(') depth++;
            else if (s[paren_end] == ')') depth--;
            if (depth > 0) paren_end++;
        }
        String ret_str(s + paren_end + 2);
        char ret_c[128];
        tick_type_to_c_type(ret_str, program, ret_c, sizeof(ret_c));
        buf.append("%s (*%s)(", ret_c, name);
        if (paren_end > paren_start + 1) {
            String params_str(s + paren_start + 1, paren_end - paren_start - 1);
            const char* p = params_str.c_str();
            int start = 0;
            int len = (int)params_str.length();
            int pd = 0;
            bool first = true;
            for (int i = 0; i <= len; i++) {
                if (i < len && p[i] == '(') pd++;
                else if (i < len && p[i] == ')') pd--;
                if ((i == len || (p[i] == ',' && pd == 0))) {
                    String param_type(p + start, i - start);
                    char param_c[128];
                    tick_type_to_c_type(param_type, program, param_c, sizeof(param_c));
                    if (!first) buf.append(", ");
                    buf.append("%s", param_c);
                    first = false;
                    start = i + 1;
                }
            }
        } else {
            buf.append("void");
        }
        buf.append(")");
    } else {
        char c_type[128];
        tick_type_to_c_type(tick_type, program, c_type, sizeof(c_type));
        buf.append("%s %s", c_type, name);
    }
}

bool Compiler::compile_to_native(const char* source_file, const char* output_file, bool keep_c) {
    FILE* f = fopen(source_file, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s\n", source_file);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* source = (char*)malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();

    Parser parser(tokens);
    for (int i = 0; i < _define_count; i++) {
        parser.add_define(_defines[i]);
    }
    Program* program = parser.parse();

    if (!program) {
        fprintf(stderr, "Parse error\n");
        free(source);
        return false;
    }

    ModuleLoader module_loader;
    SemanticAnalyzer analyzer;
    analyzer.set_module_loader(&module_loader);
    analyzer.set_current_file_path(source_file);

    if (!analyzer.analyze(program)) {
        fprintf(stderr, "Semantic analysis failed\n");
        delete program;
        free(source);
        return false;
    }

    String c_code = generate_c_code(program);

    char temp_c[256];
    if (keep_c) {
        snprintf(temp_c, sizeof(temp_c), "%s.c", output_file);
    } else {
        snprintf(temp_c, sizeof(temp_c), "/tmp/tick_%d.c", getpid());
    }

    write_to_file(temp_c, c_code.c_str());

    char extra_flags[2048] = {0};
    size_t flags_offset = 0;
    for (size_t i = 0; i < program->link_flags.size(); i++) {
        int written = snprintf(extra_flags + flags_offset, sizeof(extra_flags) - flags_offset, "%s ", program->link_flags[i].c_str());
        if (written > 0) flags_offset += written;
    }

    bool success = invoke_gcc(temp_c, output_file, extra_flags);

    if (success && !keep_c) {
        remove(temp_c);
    } else if (!success) {
        fprintf(stderr, "Generated C file saved at: %s\n", temp_c);
    } else if (keep_c) {
        printf("Generated C file saved at: %s\n", temp_c);
    }

    delete program;
    free(source);

    return success;
}

String Compiler::generate_c_code(Program* program) {
    CodeBuffer buf;

    buf.append("#include <stdio.h>\n");
    buf.append("#include <stdlib.h>\n");
    buf.append("#include <string.h>\n");
    buf.append("#include <stdbool.h>\n");
    buf.append("#include <stdint.h>\n");
    buf.append("#include <math.h>\n");
    buf.append("#include <setjmp.h>\n");
    buf.append("#include \"runtime/tick_runtime.h\"\n\n");

    buf.append("static jmp_buf _tick_try_stack[64];\n");
    buf.append("static const char* _tick_try_msg[64];\n");
    buf.append("static int _tick_try_depth = 0;\n\n");

    for (size_t i = 0; i < program->globals.size(); i++) {
        VarDecl* var = program->globals[i];
        if (var->is_const) buf.append("const ");

        bool is_array = var->type_name.length() > 2 &&
                       var->type_name[var->type_name.length() - 2] == '[' &&
                       var->type_name[var->type_name.length() - 1] == ']';

        if (is_array) {
            buf.append("TickArray %s;\n", var->name.c_str());
        } else {
            generate_typed_decl(buf, var->type_name, var->name.c_str(), program);
            if (var->initializer) {
                buf.append(" = ");
                generate_expression(buf, var->initializer, program);
            }
            buf.append(";\n");
        }
    }

    if (program->globals.size() > 0) buf.append("\n");

    for (size_t i = 0; i < program->enums.size(); i++) {
        EnumDecl* ed = program->enums[i];
        int next_val = 0;
        for (size_t j = 0; j < ed->values.size(); j++) {
            int v = ed->values[j]->has_value ? ed->values[j]->value : next_val;
            buf.append("#define %s_%s %d\n", ed->name.c_str(), ed->values[j]->name.c_str(), v);
            next_val = v + 1;
        }
        buf.append("\n");
    }

    for (size_t i = 0; i < program->classes.size(); i++) {
        ClassDecl* cls = program->classes[i];
        if (cls->is_dataclass) continue;
        buf.append("typedef struct %s {\n", cls->name.c_str());
        if (!cls->base_class.empty()) {
            ClassDecl* base = find_class(cls->base_class, program);
            while (base) {
                ClassDecl* root = base;
                DynamicArray<ClassDecl*> chain;
                chain.push(root);
                while (!root->base_class.empty()) {
                    root = find_class(root->base_class, program);
                    if (!root) break;
                    chain.push(root);
                }
                for (size_t ci = chain.size(); ci > 0; ci--) {
                    ClassDecl* ancestor = chain[ci - 1];
                    for (size_t j = 0; j < ancestor->fields.size(); j++) {
                        VarDecl* field = ancestor->fields[j];
                        buf.append("    ");
                        generate_typed_decl(buf, field->type_name, field->name.c_str(), program);
                        buf.append(";\n");
                    }
                }
                break;
            }
        }
        for (size_t j = 0; j < cls->fields.size(); j++) {
            VarDecl* field = cls->fields[j];
            buf.append("    ");
            generate_typed_decl(buf, field->type_name, field->name.c_str(), program);
            buf.append(";\n");
        }
        buf.append("} %s;\n\n", cls->name.c_str());
    }

    for (size_t i = 0; i < program->classes.size(); i++) {
        ClassDecl* dc = program->classes[i];
        if (!dc->is_dataclass) continue;
        buf.append("typedef struct %s {\n", dc->name.c_str());
        for (size_t j = 0; j < dc->fields.size(); j++) {
            VarDecl* field = dc->fields[j];
            buf.append("    ");
            generate_typed_decl(buf, field->type_name, field->name.c_str(), program);
            buf.append(";\n");
        }
        buf.append("} %s;\n\n", dc->name.c_str());
    }

    for (size_t i = 0; i < program->unions.size(); i++) {
        UnionDecl* ud = program->unions[i];
        buf.append("typedef union %s {\n", ud->name.c_str());
        for (size_t j = 0; j < ud->fields.size(); j++) {
            bool is_class_field = false;
            for (size_t k = 0; k < program->classes.size(); k++) {
                if (program->classes[k]->name == ud->fields[j]->type_name) {
                    is_class_field = true;
                    break;
                }
            }
            if (is_class_field) {
                buf.append("    %s %s;\n", ud->fields[j]->type_name.c_str(), ud->fields[j]->name.c_str());
            } else {
                buf.append("    ");
                generate_typed_decl(buf, ud->fields[j]->type_name, ud->fields[j]->name.c_str(), program);
                buf.append(";\n");
            }
        }
        buf.append("} %s;\n\n", ud->name.c_str());
    }

    for (size_t i = 0; i < program->extern_functions.size(); i++) {
        ExternFuncDecl* ef = program->extern_functions[i];
        const char* n = ef->name.c_str();
        if (strcmp(n, "malloc") == 0 || strcmp(n, "free") == 0 ||
            strcmp(n, "memset") == 0 || strcmp(n, "memcpy") == 0 ||
            strcmp(n, "memcmp") == 0 || strcmp(n, "memmove") == 0) {
            continue;
        }
        char ret_type[128];
        tick_type_to_c_type(ef->return_type, program, ret_type, sizeof(ret_type));
        buf.append("%s %s(", ret_type, ef->name.c_str());
        for (size_t j = 0; j < ef->parameters.size(); j++) {
            if (j > 0) buf.append(", ");
            generate_typed_decl(buf, ef->parameters[j]->type_name, ef->parameters[j]->name.c_str(), program);
        }
        buf.append(");\n");
    }
    if (program->extern_functions.size() > 0) buf.append("\n");

    for (size_t i = 0; i < program->signals.size(); i++) {
        SignalDecl* sig = program->signals[i];
        if (sig->array_size > 0) {
            buf.append("TickSignal %s[%d];\n", sig->name.c_str(), sig->array_size);
        } else {
            buf.append("TickSignal %s;\n", sig->name.c_str());
        }
    }

    for (size_t i = 0; i < program->events.size(); i++) {
        buf.append("TickEvent %s;\n", program->events[i]->name.c_str());
    }

    if (program->signals.size() > 0 || program->events.size() > 0) buf.append("\n");

    for (size_t i = 0; i < program->processes.size(); i++) {
        buf.append("void* %s(void* arg);\n", mangle(program->processes[i]->name).c_str());
    }

    for (size_t i = 0; i < program->functions.size(); i++) {
        FunctionDecl* func = program->functions[i];
        char ret_type[128];
        tick_type_to_c_type(func->return_type, program, ret_type, sizeof(ret_type));
        buf.append("%s %s(", ret_type, mangle(func->name).c_str());
        for (size_t j = 0; j < func->parameters.size(); j++) {
            if (j > 0) buf.append(", ");
            if (is_array_type_str(func->parameters[j]->type_name)) {
                buf.append("TickArray* %s", func->parameters[j]->name.c_str());
            } else {
                generate_typed_decl(buf, func->parameters[j]->type_name, func->parameters[j]->name.c_str(), program);
            }
        }
        buf.append(");\n");
    }

    for (size_t i = 0; i < program->methods.size(); i++) {
        FunctionDecl* method = program->methods[i];
        char ret_type[128];
        tick_type_to_c_type(method->return_type, program, ret_type, sizeof(ret_type));
        const char* method_c_name = method->is_destructor ? "dtor" : method->name.c_str();
        buf.append("%s %s_%s(%s* self", ret_type, method->class_name.c_str(), method_c_name, method->class_name.c_str());
        for (size_t k = 0; k < method->parameters.size(); k++) {
            buf.append(", ");
            if (is_array_type_str(method->parameters[k]->type_name)) {
                buf.append("TickArray* %s", method->parameters[k]->name.c_str());
            } else {
                generate_typed_decl(buf, method->parameters[k]->type_name, method->parameters[k]->name.c_str(), program);
            }
        }
        buf.append(");\n");
    }

    buf.append("\n");

    for (size_t i = 0; i < program->processes.size(); i++) {
        generate_process(buf, program->processes[i], program);
    }

    for (size_t i = 0; i < program->functions.size(); i++) {
        generate_function(buf, program->functions[i], program);
    }

    for (size_t i = 0; i < program->methods.size(); i++) {
        FunctionDecl* method = program->methods[i];
        ClassDecl* cls = nullptr;
        for (size_t j = 0; j < program->classes.size(); j++) {
            if (program->classes[j]->name == method->class_name) {
                cls = program->classes[j];
                break;
            }
        }
        char ret_type[128];
        tick_type_to_c_type(method->return_type, program, ret_type, sizeof(ret_type));
        const char* method_c_name = method->is_destructor ? "dtor" : method->name.c_str();
        buf.append("%s %s_%s(%s* self", ret_type, method->class_name.c_str(), method_c_name, method->class_name.c_str());
        for (size_t k = 0; k < method->parameters.size(); k++) {
            buf.append(", ");
            if (is_array_type_str(method->parameters[k]->type_name)) {
                buf.append("TickArray* %s", method->parameters[k]->name.c_str());
            } else {
                generate_typed_decl(buf, method->parameters[k]->type_name, method->parameters[k]->name.c_str(), program);
            }
        }
        buf.append(") {\n");

        _current_func = method;
        _current_class = cls;

        if (method->body) {
            for (size_t k = 0; k < method->body->statements.size(); k++) {
                generate_statement(buf, method->body->statements[k], 1, program);
            }
        }

        if (method->is_destructor && cls && !cls->base_class.empty()) {
            if (class_has_destructor(cls->base_class, program)) {
                buf.append("    %s_dtor((%s*)self);\n", cls->base_class.c_str(), cls->base_class.c_str());
            }
        }

        buf.append("}\n\n");
    }

    String result(buf.data, buf.pos);
    return result;
}

void Compiler::generate_process(CodeBuffer& buf, ProcessDecl* proc, Program* program) {
    buf.append("void* %s(void* arg) {\n", mangle(proc->name).c_str());

    _current_process = proc;
    _current_func = nullptr;
    push_defer_scope();
    for (size_t i = 0; i < proc->body->statements.size(); i++) {
        generate_statement(buf, proc->body->statements[i], 1, program);
    }
    generate_deferred(buf, 1, program);
    pop_defer_scope();

    _current_process = nullptr;
    buf.append("    return NULL;\n");
    buf.append("}\n\n");
}

void Compiler::generate_function(CodeBuffer& buf, FunctionDecl* func, Program* program) {
    char ret_type[128];
    tick_type_to_c_type(func->return_type, program, ret_type, sizeof(ret_type));

    _current_func = func;
    _current_class = nullptr;

    buf.append("%s %s(", ret_type, mangle(func->name).c_str());
    for (size_t j = 0; j < func->parameters.size(); j++) {
        if (j > 0) buf.append(", ");
        if (is_array_type_str(func->parameters[j]->type_name)) {
            buf.append("TickArray* %s", func->parameters[j]->name.c_str());
        } else {
            generate_typed_decl(buf, func->parameters[j]->type_name, func->parameters[j]->name.c_str(), program);
        }
    }
    buf.append(") {\n");

    push_defer_scope();

    if (func->name == "main") {
        for (size_t i = 0; i < program->globals.size(); i++) {
            VarDecl* var = program->globals[i];
            bool is_arr = var->type_name.length() > 2 &&
                         var->type_name[var->type_name.length() - 2] == '[' &&
                         var->type_name[var->type_name.length() - 1] == ']';
            if (is_arr && var->initializer && var->initializer->type == AstNodeType::ARRAY_EXPR) {
                ArrayExpr* arr = static_cast<ArrayExpr*>(var->initializer);
                int count = (int)arr->elements.size();
                String base_type(var->type_name.c_str(), var->type_name.length() - 2);
                char c_base[128];
                tick_type_to_c_type(base_type, program, c_base, sizeof(c_base));
                int init_cap = count < 4 ? 4 : count;
                buf.append("    %s.ptr = malloc(%d * sizeof(%s));\n",
                    var->name.c_str(), init_cap, c_base);
                for (int ei = 0; ei < count; ei++) {
                    buf.append("    ((%s*)%s.ptr)[%d] = ", c_base, var->name.c_str(), ei);
                    generate_expression(buf, arr->elements[ei], program);
                    buf.append(";\n");
                }
                buf.append("    %s.len = %d;\n", var->name.c_str(), count);
                buf.append("    %s.cap = %d;\n", var->name.c_str(), init_cap);
            } else if (is_arr) {
                buf.append("    %s.ptr = NULL;\n", var->name.c_str());
                buf.append("    %s.len = 0;\n", var->name.c_str());
                buf.append("    %s.cap = 0;\n", var->name.c_str());
            }
        }

        for (size_t i = 0; i < program->signals.size(); i++) {
            SignalDecl* sig = program->signals[i];
            if (sig->array_size > 0) {
                buf.append("    for (int _i = 0; _i < %d; _i++) {\n", sig->array_size);
                buf.append("        tick_signal_init(&%s[_i]);\n", sig->name.c_str());
                buf.append("    }\n");
            } else {
                buf.append("    tick_signal_init(&%s);\n", sig->name.c_str());
            }
        }

        for (size_t i = 0; i < program->events.size(); i++) {
            EventDecl* evt = program->events[i];
            int proc_count = 0;
            for (size_t j = 0; j < program->processes.size(); j++) {
                if (program->processes[j]->event_name == evt->name) proc_count++;
            }
            buf.append("    tick_event_init(&%s, %d);\n", evt->name.c_str(), proc_count);
            for (size_t j = 0; j < program->processes.size(); j++) {
                ProcessDecl* proc = program->processes[j];
                if (proc->event_name == evt->name) {
                    buf.append("    tick_event_add_process(&%s, %s);\n", evt->name.c_str(), mangle(proc->name).c_str());
                }
            }
        }

        if (program->signals.size() > 0 || program->events.size() > 0) buf.append("\n");
    }

    if (func->body) {
        for (size_t i = 0; i < func->body->statements.size(); i++) {
            generate_statement(buf, func->body->statements[i], 1, program);
        }
    }

    generate_deferred(buf, 1, program);
    pop_defer_scope();

    buf.append("}\n\n");
}

void Compiler::generate_statement(CodeBuffer& buf, StmtNode* stmt, int indent, Program* program) {
    if (!stmt) return;

    switch (stmt->type) {
        case AstNodeType::VAR_DECL: {
            VarDecl* decl = static_cast<VarDecl*>(stmt);
            for (int i = 0; i < indent; i++) buf.append("    ");
            if (decl->is_const) buf.append("const ");

            bool is_array = decl->type_name.length() > 2 &&
                           decl->type_name[decl->type_name.length() - 2] == '[' &&
                           decl->type_name[decl->type_name.length() - 1] == ']';

            if (is_array && decl->initializer && decl->initializer->type == AstNodeType::ARRAY_EXPR) {
                ArrayExpr* arr = static_cast<ArrayExpr*>(decl->initializer);
                int count = (int)arr->elements.size();
                String base_type(decl->type_name.c_str(), decl->type_name.length() - 2);
                char c_base[128];
                tick_type_to_c_type(base_type, program, c_base, sizeof(c_base));
                int init_cap = count < 4 ? 4 : count;
                buf.append("TickArray %s;\n", decl->name.c_str());
                for (int i = 0; i < indent; i++) buf.append("    ");
                buf.append("%s.ptr = malloc(%d * sizeof(%s));\n", decl->name.c_str(), init_cap, c_base);
                for (int ei = 0; ei < count; ei++) {
                    for (int i = 0; i < indent; i++) buf.append("    ");
                    buf.append("((%s*)%s.ptr)[%d] = ", c_base, decl->name.c_str(), ei);
                    generate_expression(buf, arr->elements[ei], program);
                    buf.append(";\n");
                }
                for (int i = 0; i < indent; i++) buf.append("    ");
                buf.append("%s.len = %d;\n", decl->name.c_str(), count);
                for (int i = 0; i < indent; i++) buf.append("    ");
                buf.append("%s.cap = %d;\n", decl->name.c_str(), init_cap);
            } else if (is_array && decl->initializer) {
                buf.append("TickArray %s = ", decl->name.c_str());
                generate_expression(buf, decl->initializer, program);
                buf.append(";\n");
            } else if (is_array) {
                buf.append("TickArray %s = {0};\n", decl->name.c_str());
            } else {
                generate_typed_decl(buf, decl->type_name, decl->name.c_str(), program);
                if (decl->initializer) {
                    buf.append(" = ");
                    _expected_type = decl->type_name;
                    generate_expression(buf, decl->initializer, program);
                    _expected_type = String("");
                } else {
                    bool is_union_type = false;
                    for (size_t ui = 0; ui < program->unions.size(); ui++) {
                        if (program->unions[ui]->name == decl->type_name) { is_union_type = true; break; }
                    }
                    bool is_class_type = false;
                    for (size_t ci = 0; ci < program->classes.size(); ci++) {
                        if (program->classes[ci]->name == decl->type_name) { is_class_type = true; break; }
                    }
                    if (decl->type_name == "str") {
                        buf.append(" = NULL");
                    } else if (decl->type_name == "ptr" || is_typed_ptr_type(decl->type_name) || is_func_ptr_type(decl->type_name)) {
                        buf.append(" = NULL");
                    } else if (is_fixed_array_type(decl->type_name) || is_union_type || is_class_type) {
                        buf.append(" = {0}");
                    } else {
                        buf.append(" = 0");
                    }
                }
                buf.append(";\n");
                if (class_has_destructor(decl->type_name, program) && _defer_depth >= 0 &&
                    _raii_counts[_defer_depth] < MAX_RAII_PER_SCOPE) {
                    _raii_scopes[_defer_depth][_raii_counts[_defer_depth]].var_name = decl->name;
                    _raii_scopes[_defer_depth][_raii_counts[_defer_depth]].class_name = decl->type_name;
                    _raii_counts[_defer_depth]++;
                }
            }
            if (is_array && !decl->is_const && _defer_depth >= 0 &&
                _array_counts[_defer_depth] < MAX_ARRAYS_PER_SCOPE) {
                _array_scopes[_defer_depth][_array_counts[_defer_depth]] = decl->name;
                _array_counts[_defer_depth]++;
            }
            break;
        }

        case AstNodeType::EXPR_STMT: {
            ExprStmt* expr_stmt = static_cast<ExprStmt*>(stmt);
            for (int i = 0; i < indent; i++) buf.append("    ");
            generate_expression(buf, expr_stmt->expression, program);
            buf.append(";\n");
            break;
        }

        case AstNodeType::RETURN_STMT: {
            ReturnStmt* ret = static_cast<ReturnStmt*>(stmt);
            generate_all_deferred(buf, indent, program);
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("return");
            if (ret->value) {
                buf.append(" ");
                generate_expression(buf, ret->value, program);
            }
            buf.append(";\n");
            break;
        }

        case AstNodeType::IF_STMT: {
            IfStmt* if_stmt = static_cast<IfStmt*>(stmt);
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("if (");
            generate_expression(buf, if_stmt->condition, program);
            buf.append(") {\n");

            push_defer_scope();
            if (if_stmt->then_branch->type == AstNodeType::BLOCK_STMT) {
                BlockStmt* block = static_cast<BlockStmt*>(if_stmt->then_branch);
                for (size_t i = 0; i < block->statements.size(); i++) {
                    generate_statement(buf, block->statements[i], indent + 1, program);
                }
            } else {
                generate_statement(buf, if_stmt->then_branch, indent + 1, program);
            }
            generate_deferred(buf, indent + 1, program);
            pop_defer_scope();

            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("}");

            if (if_stmt->else_branch) {
                if (if_stmt->else_branch->type == AstNodeType::IF_STMT) {
                    buf.append(" else ");
                    generate_statement(buf, if_stmt->else_branch, indent, program);
                } else {
                    buf.append(" else {\n");
                    push_defer_scope();
                    if (if_stmt->else_branch->type == AstNodeType::BLOCK_STMT) {
                        BlockStmt* block = static_cast<BlockStmt*>(if_stmt->else_branch);
                        for (size_t i = 0; i < block->statements.size(); i++) {
                            generate_statement(buf, block->statements[i], indent + 1, program);
                        }
                    } else {
                        generate_statement(buf, if_stmt->else_branch, indent + 1, program);
                    }
                    generate_deferred(buf, indent + 1, program);
                    pop_defer_scope();
                    for (int i = 0; i < indent; i++) buf.append("    ");
                    buf.append("}");
                }
            }
            buf.append("\n");
            break;
        }

        case AstNodeType::WHILE_STMT: {
            WhileStmt* while_stmt = static_cast<WhileStmt*>(stmt);
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("while (");
            generate_expression(buf, while_stmt->condition, program);
            buf.append(") {\n");

            push_defer_scope();
            if (while_stmt->body->type == AstNodeType::BLOCK_STMT) {
                BlockStmt* block = static_cast<BlockStmt*>(while_stmt->body);
                for (size_t i = 0; i < block->statements.size(); i++) {
                    generate_statement(buf, block->statements[i], indent + 1, program);
                }
            } else {
                generate_statement(buf, while_stmt->body, indent + 1, program);
            }
            generate_deferred(buf, indent + 1, program);
            pop_defer_scope();

            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("}\n");
            break;
        }

        case AstNodeType::FOR_STMT: {
            ForStmt* for_stmt = static_cast<ForStmt*>(stmt);
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("for (");

            if (for_stmt->initializer) {
                if (for_stmt->initializer->type == AstNodeType::VAR_DECL) {
                    VarDecl* decl = static_cast<VarDecl*>(for_stmt->initializer);
                    generate_typed_decl(buf, decl->type_name, decl->name.c_str(), program);
                    if (decl->initializer) {
                        buf.append(" = ");
                        generate_expression(buf, decl->initializer, program);
                    } else {
                        buf.append(" = 0");
                    }
                } else {
                    generate_expression(buf, static_cast<ExprStmt*>(for_stmt->initializer)->expression, program);
                }
            }
            buf.append("; ");

            if (for_stmt->condition) {
                generate_expression(buf, for_stmt->condition, program);
            }
            buf.append("; ");

            if (for_stmt->increment) {
                generate_expression(buf, for_stmt->increment, program);
            }
            buf.append(") {\n");

            push_defer_scope();
            if (for_stmt->body->type == AstNodeType::BLOCK_STMT) {
                BlockStmt* block = static_cast<BlockStmt*>(for_stmt->body);
                for (size_t i = 0; i < block->statements.size(); i++) {
                    generate_statement(buf, block->statements[i], indent + 1, program);
                }
            } else {
                generate_statement(buf, for_stmt->body, indent + 1, program);
            }
            generate_deferred(buf, indent + 1, program);
            pop_defer_scope();

            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("}\n");
            break;
        }

        case AstNodeType::BREAK_STMT: {
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("break;\n");
            break;
        }

        case AstNodeType::CONTINUE_STMT: {
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("continue;\n");
            break;
        }

        case AstNodeType::BLOCK_STMT: {
            BlockStmt* block = static_cast<BlockStmt*>(stmt);
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("{\n");
            push_defer_scope();
            for (size_t i = 0; i < block->statements.size(); i++) {
                generate_statement(buf, block->statements[i], indent + 1, program);
            }
            generate_deferred(buf, indent + 1, program);
            pop_defer_scope();
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("}\n");
            break;
        }

        case AstNodeType::DEFER_STMT: {
            DeferStmt* defer = static_cast<DeferStmt*>(stmt);
            if (_defer_depth >= 0 && _defer_counts[_defer_depth] < MAX_DEFERS_PER_SCOPE) {
                _defer_scopes[_defer_depth][_defer_counts[_defer_depth]++] = defer->statement;
            }
            break;
        }

        case AstNodeType::SWITCH_STMT: {
            SwitchStmt* sw = static_cast<SwitchStmt*>(stmt);
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("switch (");
            generate_expression(buf, sw->subject, program);
            buf.append(") {\n");
            for (size_t c = 0; c < sw->cases.size(); c++) {
                SwitchCase* sc = sw->cases[c];
                if (sc->is_default) {
                    for (int i = 0; i < indent + 1; i++) buf.append("    ");
                    buf.append("default: {\n");
                } else {
                    for (size_t v = 0; v < sc->values.size(); v++) {
                        for (int i = 0; i < indent + 1; i++) buf.append("    ");
                        buf.append("case ");
                        generate_expression(buf, sc->values[v], program);
                        buf.append(":\n");
                    }
                    for (int i = 0; i < indent + 1; i++) buf.append("    ");
                    buf.append("{\n");
                }
                if (sc->body) {
                    for (size_t s = 0; s < sc->body->statements.size(); s++) {
                        generate_statement(buf, sc->body->statements[s], indent + 2, program);
                    }
                }
                for (int i = 0; i < indent + 2; i++) buf.append("    ");
                buf.append("break;\n");
                for (int i = 0; i < indent + 1; i++) buf.append("    ");
                buf.append("}\n");
            }
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("}\n");
            break;
        }

        case AstNodeType::TRY_CATCH_STMT: {
            TryCatchStmt* tc = static_cast<TryCatchStmt*>(stmt);
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("_tick_try_depth++;\n");
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("if (setjmp(_tick_try_stack[_tick_try_depth - 1]) == 0) {\n");
            push_defer_scope();
            if (tc->try_body) {
                for (size_t i = 0; i < tc->try_body->statements.size(); i++) {
                    generate_statement(buf, tc->try_body->statements[i], indent + 1, program);
                }
            }
            generate_deferred(buf, indent + 1, program);
            pop_defer_scope();
            for (int i = 0; i < indent + 1; i++) buf.append("    ");
            buf.append("_tick_try_depth--;\n");
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("} else {\n");
            for (int i = 0; i < indent + 1; i++) buf.append("    ");
            buf.append("_tick_try_depth--;\n");
            for (int i = 0; i < indent + 1; i++) buf.append("    ");
            {
                char catch_c_type[128];
                tick_type_to_c_type(String(tc->catch_type), program, catch_c_type, sizeof(catch_c_type));
                generate_typed_decl(buf, String(tc->catch_type), tc->catch_var.c_str(), program);
                buf.append(" = (%s)_tick_try_msg[_tick_try_depth];\n", catch_c_type);
            }
            push_defer_scope();
            if (tc->catch_body) {
                for (size_t i = 0; i < tc->catch_body->statements.size(); i++) {
                    generate_statement(buf, tc->catch_body->statements[i], indent + 1, program);
                }
            }
            generate_deferred(buf, indent + 1, program);
            pop_defer_scope();
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("}\n");
            break;
        }

        case AstNodeType::THROW_STMT: {
            ThrowStmt* ts = static_cast<ThrowStmt*>(stmt);
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("if (_tick_try_depth > 0) {\n");
            for (int i = 0; i < indent + 1; i++) buf.append("    ");
            buf.append("_tick_try_msg[_tick_try_depth - 1] = ");
            generate_expression(buf, ts->value, program);
            buf.append(";\n");
            for (int i = 0; i < indent + 1; i++) buf.append("    ");
            buf.append("longjmp(_tick_try_stack[_tick_try_depth - 1], 1);\n");
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("} else {\n");
            for (int i = 0; i < indent + 1; i++) buf.append("    ");
            buf.append("fprintf(stderr, \"Unhandled exception: %%s\\n\", ");
            generate_expression(buf, ts->value, program);
            buf.append(");\n");
            for (int i = 0; i < indent + 1; i++) buf.append("    ");
            buf.append("exit(1);\n");
            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("}\n");
            break;
        }

        default:
            break;
    }
}

void Compiler::generate_print_arg(CodeBuffer& buf, ExprNode* arg, Program* program) {
    if (arg->type == AstNodeType::STRING_LITERAL) {
        StringLiteral* lit = static_cast<StringLiteral*>(arg);
        buf.append("\"");
        for (size_t ci = 0; ci < lit->value.length(); ci++) {
            char ch = lit->value[ci];
            if (ch == '"') buf.append("\\\"");
            else if (ch == '\\') buf.append("\\\\");
            else if (ch == '\n') buf.append("\\n");
            else if (ch == '\r') buf.append("\\r");
            else if (ch == '\t') buf.append("\\t");
            else if (ch == '%') buf.append("%%%%");
            else { char tmp[2] = {ch, 0}; buf.append("%s", tmp); }
        }
        buf.append("\"");
        return;
    }
    String arg_type = infer_expr_type(arg, program);
    if (arg_type == "f32" || arg_type == "f64") {
        buf.append("\"%%f\", ");
    } else if (arg_type == "b8") {
        buf.append("\"%%s\", (");
        generate_expression(buf, arg, program);
        buf.append(") ? \"true\" : \"false\"");
        return;
    } else if (arg_type == "str") {
        buf.append("\"%%s\", ");
    } else if (arg_type == "i64") {
        buf.append("\"%%lld\", (long long)");
    } else if (arg_type == "u64") {
        buf.append("\"%%llu\", (unsigned long long)");
    } else if (arg_type == "u8" || arg_type == "u16" || arg_type == "u32") {
        buf.append("\"%%u\", ");
    } else {
        buf.append("\"%%d\", ");
    }
    generate_expression(buf, arg, program);
}

void Compiler::generate_expression(CodeBuffer& buf, ExprNode* expr, Program* program) {
    if (!expr) return;

    switch (expr->type) {
        case AstNodeType::INTEGER_LITERAL: {
            IntegerLiteral* lit = static_cast<IntegerLiteral*>(expr);
            buf.append("%d", lit->value);
            break;
        }

        case AstNodeType::FLOAT_LITERAL: {
            FloatLiteral* lit = static_cast<FloatLiteral*>(expr);
            buf.append("%f", lit->value);
            break;
        }

        case AstNodeType::DOUBLE_LITERAL: {
            DoubleLiteral* lit = static_cast<DoubleLiteral*>(expr);
            buf.append("%f", lit->value);
            break;
        }

        case AstNodeType::BOOL_LITERAL: {
            BoolLiteral* lit = static_cast<BoolLiteral*>(expr);
            buf.append("%s", lit->value ? "true" : "false");
            break;
        }

        case AstNodeType::NULL_LITERAL: {
            buf.append("NULL");
            break;
        }

        case AstNodeType::CAST_EXPR: {
            CastExpr* ce = static_cast<CastExpr*>(expr);
            char c_type[128];
            tick_type_to_c_type(ce->target_type, program, c_type, sizeof(c_type));
            buf.append("(%s)(", c_type);
            generate_expression(buf, ce->expression, program);
            buf.append(")");
            break;
        }

        case AstNodeType::SIZEOF_EXPR: {
            SizeofExpr* se = static_cast<SizeofExpr*>(expr);
            char c_type[128];
            tick_type_to_c_type(se->target_type, program, c_type, sizeof(c_type));
            buf.append("sizeof(%s)", c_type);
            break;
        }

        case AstNodeType::STRING_LITERAL: {
            StringLiteral* lit = static_cast<StringLiteral*>(expr);
            buf.append("\"");
            for (size_t ci = 0; ci < lit->value.length(); ci++) {
                char ch = lit->value[ci];
                if (ch == '"') buf.append("\\\"");
                else if (ch == '\\') buf.append("\\\\");
                else if (ch == '\n') buf.append("\\n");
                else if (ch == '\r') buf.append("\\r");
                else if (ch == '\t') buf.append("\\t");
                else { char tmp[2] = {ch, 0}; buf.append("%s", tmp); }
            }
            buf.append("\"");
            break;
        }

        case AstNodeType::IDENTIFIER_EXPR: {
            IdentifierExpr* ident = static_cast<IdentifierExpr*>(expr);
            for (size_t i = 0; i < program->functions.size(); i++) {
                if (program->functions[i]->name == ident->name) {
                    buf.append("%s", mangle(ident->name).c_str());
                    goto ident_done;
                }
            }
            for (size_t i = 0; i < program->processes.size(); i++) {
                if (program->processes[i]->name == ident->name) {
                    buf.append("%s", mangle(ident->name).c_str());
                    goto ident_done;
                }
            }
            buf.append("%s", ident->name.c_str());
            ident_done:
            break;
        }

        case AstNodeType::THIS_EXPR: {
            buf.append("self");
            break;
        }

        case AstNodeType::BINARY_EXPR: {
            BinaryExpr* bin = static_cast<BinaryExpr*>(expr);
            String left_type = infer_expr_type(bin->left, program);
            String right_type = infer_expr_type(bin->right, program);

            if (bin->op == "+" && (left_type == "str" || right_type == "str")) {
                buf.append("tick_str_concat(");
                generate_expression(buf, bin->left, program);
                buf.append(", ");
                generate_expression(buf, bin->right, program);
                buf.append(")");
            } else if (bin->op == "==" && left_type == "str" && right_type == "str") {
                buf.append("(tick_str_compare(");
                generate_expression(buf, bin->left, program);
                buf.append(", ");
                generate_expression(buf, bin->right, program);
                buf.append(") == 0)");
            } else if (bin->op == "!=" && left_type == "str" && right_type == "str") {
                buf.append("(tick_str_compare(");
                generate_expression(buf, bin->left, program);
                buf.append(", ");
                generate_expression(buf, bin->right, program);
                buf.append(") != 0)");
            } else {
                buf.append("(");
                generate_expression(buf, bin->left, program);
                buf.append(" %s ", bin->op.c_str());
                generate_expression(buf, bin->right, program);
                buf.append(")");
            }
            break;
        }

        case AstNodeType::UNARY_EXPR: {
            UnaryExpr* un = static_cast<UnaryExpr*>(expr);
            buf.append("%s", un->op.c_str());
            generate_expression(buf, un->operand, program);
            break;
        }

        case AstNodeType::POSTFIX_EXPR: {
            PostfixExpr* post = static_cast<PostfixExpr*>(expr);
            generate_expression(buf, post->operand, program);
            buf.append("%s", post->op.c_str());
            break;
        }

        case AstNodeType::ASSIGN_EXPR: {
            AssignExpr* assign = static_cast<AssignExpr*>(expr);
            generate_expression(buf, assign->target, program);
            buf.append(" = ");
            _expected_type = infer_expr_type(assign->target, program);
            bool deref_rhs = false;
            if (assign->target->type == AstNodeType::MEMBER_EXPR) {
                MemberExpr* tgt = static_cast<MemberExpr*>(assign->target);
                String tgt_obj_type = infer_expr_type(tgt->object, program);
                for (size_t i = 0; i < program->unions.size(); i++) {
                    if (program->unions[i]->name == tgt_obj_type) {
                        for (size_t j = 0; j < program->unions[i]->fields.size(); j++) {
                            if (program->unions[i]->fields[j]->name == tgt->member) {
                                String ft = program->unions[i]->fields[j]->type_name;
                                for (size_t k = 0; k < program->classes.size(); k++) {
                                    if (program->classes[k]->name == ft) { deref_rhs = true; break; }
                                }
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            if (deref_rhs) buf.append("*");
            generate_expression(buf, assign->value, program);
            _expected_type = String("");
            break;
        }

        case AstNodeType::COMPOUND_ASSIGN_EXPR: {
            CompoundAssignExpr* compound = static_cast<CompoundAssignExpr*>(expr);
            generate_expression(buf, compound->target, program);
            buf.append(" %s= ", compound->op.c_str());
            generate_expression(buf, compound->value, program);
            break;
        }

        case AstNodeType::CALL_EXPR: {
            CallExpr* call = static_cast<CallExpr*>(expr);

            if (call->callee->type == AstNodeType::MEMBER_EXPR) {
                MemberExpr* member = static_cast<MemberExpr*>(call->callee);

                if (member->member == "emit") {
                    String signal_name;
                    String signal_type;

                    if (member->object->type == AstNodeType::INDEX_EXPR) {
                        IndexExpr* idx_expr = static_cast<IndexExpr*>(member->object);
                        IdentifierExpr* obj = static_cast<IdentifierExpr*>(idx_expr->array);
                        signal_name = obj->name;
                    } else {
                        IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                        signal_name = obj->name;
                    }

                    for (size_t i = 0; i < program->signals.size(); i++) {
                        if (program->signals[i]->name == signal_name) {
                            signal_type = program->signals[i]->type_param;
                            break;
                        }
                    }

                    bool is_pointer_type = false;
                    bool is_array_signal = false;
                    if (signal_type.length() > 2 && signal_type[signal_type.length() - 2] == '[' && signal_type[signal_type.length() - 1] == ']') {
                        is_array_signal = true;
                    } else {
                        for (size_t i = 0; i < program->classes.size(); i++) {
                            if (program->classes[i]->name == signal_type) {
                                is_pointer_type = true;
                                break;
                            }
                        }
                    }

                    if (is_array_signal) {
                        String base_type(signal_type.c_str(), signal_type.length() - 2);
                        char c_base[128];
                        tick_type_to_c_type(base_type, program, c_base, sizeof(c_base));
                        if (member->object->type == AstNodeType::INDEX_EXPR) {
                            IndexExpr* idx_expr = static_cast<IndexExpr*>(member->object);
                            IdentifierExpr* obj = static_cast<IdentifierExpr*>(idx_expr->array);
                            buf.append("({ TickArray __se = ");
                            if (call->arguments.size() > 0) generate_expression(buf, call->arguments[0], program);
                            buf.append("; TickArray* __sa = (TickArray*)malloc(sizeof(TickArray)); __sa->len = __se.len; __sa->cap = __se.cap; __sa->ptr = malloc((size_t)__se.cap * sizeof(%s)); memcpy(__sa->ptr, __se.ptr, (size_t)__se.len * sizeof(%s)); tick_signal_emit(&%s[", c_base, c_base, obj->name.c_str());
                            generate_expression(buf, idx_expr->index, program);
                            buf.append("], __sa); })");
                        } else {
                            IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                            buf.append("({ TickArray __se = ");
                            if (call->arguments.size() > 0) generate_expression(buf, call->arguments[0], program);
                            buf.append("; TickArray* __sa = (TickArray*)malloc(sizeof(TickArray)); __sa->len = __se.len; __sa->cap = __se.cap; __sa->ptr = malloc((size_t)__se.cap * sizeof(%s)); memcpy(__sa->ptr, __se.ptr, (size_t)__se.len * sizeof(%s)); tick_signal_emit(&%s, __sa); })", c_base, c_base, obj->name.c_str());
                        }
                    } else if (member->object->type == AstNodeType::INDEX_EXPR) {
                        IndexExpr* idx_expr = static_cast<IndexExpr*>(member->object);
                        IdentifierExpr* obj = static_cast<IdentifierExpr*>(idx_expr->array);
                        buf.append("tick_signal_emit(&%s[", obj->name.c_str());
                        generate_expression(buf, idx_expr->index, program);
                        if (is_pointer_type) {
                            buf.append("], (void*)(");
                        } else {
                            buf.append("], (void*)(intptr_t)(");
                        }
                    } else {
                        IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                        if (is_pointer_type) {
                            buf.append("tick_signal_emit(&%s, (void*)(", obj->name.c_str());
                        } else {
                            buf.append("tick_signal_emit(&%s, (void*)(intptr_t)(", obj->name.c_str());
                        }
                    }
                    if (!is_array_signal) {
                        if (call->arguments.size() > 0) {
                            generate_expression(buf, call->arguments[0], program);
                        }
                        buf.append("))");
                    }
                } else if (member->member == "recv") {
                    String signal_name;
                    String signal_type;

                    if (member->object->type == AstNodeType::INDEX_EXPR) {
                        IndexExpr* idx_expr = static_cast<IndexExpr*>(member->object);
                        IdentifierExpr* obj = static_cast<IdentifierExpr*>(idx_expr->array);
                        signal_name = obj->name;
                    } else {
                        IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                        signal_name = obj->name;
                    }

                    for (size_t i = 0; i < program->signals.size(); i++) {
                        if (program->signals[i]->name == signal_name) {
                            signal_type = program->signals[i]->type_param;
                            break;
                        }
                    }

                    bool is_pointer_type = false;
                    bool is_array_signal = false;
                    if (signal_type.length() > 2 && signal_type[signal_type.length() - 2] == '[' && signal_type[signal_type.length() - 1] == ']') {
                        is_array_signal = true;
                    } else {
                        for (size_t i = 0; i < program->classes.size(); i++) {
                            if (program->classes[i]->name == signal_type) {
                                is_pointer_type = true;
                                break;
                            }
                        }
                    }

                    if (is_array_signal) {
                        if (member->object->type == AstNodeType::INDEX_EXPR) {
                            IndexExpr* idx_expr = static_cast<IndexExpr*>(member->object);
                            IdentifierExpr* obj = static_cast<IdentifierExpr*>(idx_expr->array);
                            buf.append("({ TickArray* __sr = (TickArray*)tick_signal_recv(&%s[", obj->name.c_str());
                            generate_expression(buf, idx_expr->index, program);
                            buf.append("]); TickArray __rv = *__sr; free(__sr); __rv; })");
                        } else {
                            IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                            buf.append("({ TickArray* __sr = (TickArray*)tick_signal_recv(&%s); TickArray __rv = *__sr; free(__sr); __rv; })", obj->name.c_str());
                        }
                    } else {
                        if (!is_pointer_type) {
                            buf.append("(intptr_t)");
                        }

                        if (member->object->type == AstNodeType::INDEX_EXPR) {
                            IndexExpr* idx_expr = static_cast<IndexExpr*>(member->object);
                            IdentifierExpr* obj = static_cast<IdentifierExpr*>(idx_expr->array);
                            buf.append("tick_signal_recv(&%s[", obj->name.c_str());
                            generate_expression(buf, idx_expr->index, program);
                            buf.append("])");
                        } else {
                            IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                            buf.append("tick_signal_recv(&%s)", obj->name.c_str());
                        }
                    }
                } else if (member->member == "execute") {
                    IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                    buf.append("tick_event_execute(&%s)", obj->name.c_str());
                } else if (member->member == "length" && call->arguments.size() == 0) {
                    String obj_type = infer_expr_type(member->object, program);
                    if (obj_type == "str") {
                        buf.append("tick_str_length(");
                        generate_expression(buf, member->object, program);
                        buf.append(")");
                    } else if (is_array_type_str(obj_type) && member->object->type == AstNodeType::IDENTIFIER_EXPR) {
                        IdentifierExpr* obj_id = static_cast<IdentifierExpr*>(member->object);
                        bool is_param = is_array_param(obj_id->name);
                        const char* acc = is_param ? "->" : ".";
                        buf.append("%s%slen", obj_id->name.c_str(), acc);
                    } else {
                        generate_expression(buf, call->callee, program);
                        buf.append("(");
                        buf.append(")");
                    }
                } else if (member->member == "push" && call->arguments.size() == 1) {
                    String obj_type = infer_expr_type(member->object, program);
                    if (is_array_type_str(obj_type) && member->object->type == AstNodeType::IDENTIFIER_EXPR) {
                        IdentifierExpr* obj_id = static_cast<IdentifierExpr*>(member->object);
                        String base_type(obj_type.c_str(), obj_type.length() - 2);
                        char c_base[128];
                        tick_type_to_c_type(base_type, program, c_base, sizeof(c_base));
                        bool is_param = is_array_param(obj_id->name);
                        const char* addr = is_param ? "" : "&";
                        const char* acc = is_param ? "->" : ".";
                        buf.append("(tick_array_push(%s%s, sizeof(%s)), ((%s*)%s%sptr)[%s%slen - 1] = ",
                            addr, obj_id->name.c_str(), c_base,
                            c_base, obj_id->name.c_str(), acc,
                            obj_id->name.c_str(), acc);
                        generate_expression(buf, call->arguments[0], program);
                        buf.append(")");
                    } else {
                        generate_expression(buf, call->callee, program);
                        buf.append("(");
                        generate_expression(buf, call->arguments[0], program);
                        buf.append(")");
                    }
                } else if (member->member == "pop" && call->arguments.size() == 0) {
                    String obj_type = infer_expr_type(member->object, program);
                    if (is_array_type_str(obj_type) && member->object->type == AstNodeType::IDENTIFIER_EXPR) {
                        IdentifierExpr* obj_id = static_cast<IdentifierExpr*>(member->object);
                        String pop_base(obj_type.c_str(), obj_type.length() - 2);
                        char pop_c_base[128];
                        tick_type_to_c_type(pop_base, program, pop_c_base, sizeof(pop_c_base));
                        bool is_param = is_array_param(obj_id->name);
                        const char* addr = is_param ? "" : "&";
                        const char* acc = is_param ? "->" : ".";
                        buf.append("(tick_array_pop(%s%s), ((%s*)%s%sptr)[%s%slen])",
                            addr, obj_id->name.c_str(), pop_c_base, obj_id->name.c_str(), acc, obj_id->name.c_str(), acc);
                    } else {
                        generate_expression(buf, call->callee, program);
                        buf.append("()");
                    }
                } else if (member->member == "substring" && call->arguments.size() == 2) {
                    buf.append("tick_str_substring(");
                    generate_expression(buf, member->object, program);
                    buf.append(", ");
                    generate_expression(buf, call->arguments[0], program);
                    buf.append(", ");
                    generate_expression(buf, call->arguments[1], program);
                    buf.append(")");
                } else if (member->member == "index_of" && call->arguments.size() == 1) {
                    buf.append("tick_str_index_of(");
                    generate_expression(buf, member->object, program);
                    buf.append(", ");
                    generate_expression(buf, call->arguments[0], program);
                    buf.append(")");
                } else if (member->member == "char_at" && call->arguments.size() == 1) {
                    buf.append("tick_str_char_at(");
                    generate_expression(buf, member->object, program);
                    buf.append(", ");
                    generate_expression(buf, call->arguments[0], program);
                    buf.append(")");
                } else {
                    if (member->object->type == AstNodeType::IDENTIFIER_EXPR) {
                        IdentifierExpr* obj_ident = static_cast<IdentifierExpr*>(member->object);

                        ClassDecl* obj_class = nullptr;
                        String var_type = lookup_var_type(obj_ident->name, program);
                        for (size_t i = 0; i < program->classes.size(); i++) {
                            if (program->classes[i]->name == var_type) {
                                obj_class = program->classes[i];
                                break;
                            }
                        }

                        if (obj_class) {
                            FunctionDecl* resolved = find_method(obj_class->name, member->member, program);
                            if (resolved) {
                                buf.append("%s_%s(", resolved->class_name.c_str(), resolved->name.c_str());
                                if (resolved->class_name == obj_class->name) {
                                    generate_expression(buf, member->object, program);
                                } else {
                                    buf.append("(%s*)", resolved->class_name.c_str());
                                    generate_expression(buf, member->object, program);
                                }
                                for (size_t i = 0; i < call->arguments.size(); i++) {
                                    buf.append(", ");
                                    if (i < resolved->parameters.size() &&
                                        is_array_type_str(resolved->parameters[i]->type_name) &&
                                        call->arguments[i]->type == AstNodeType::IDENTIFIER_EXPR) {
                                        IdentifierExpr* arg_id = static_cast<IdentifierExpr*>(call->arguments[i]);
                                        buf.append("%s%s", is_array_param(arg_id->name) ? "" : "&", arg_id->name.c_str());
                                    } else {
                                        generate_expression(buf, call->arguments[i], program);
                                    }
                                }
                                buf.append(")");
                            } else {
                                buf.append("%s_%s(", obj_class->name.c_str(), member->member.c_str());
                                generate_expression(buf, member->object, program);
                                for (size_t i = 0; i < call->arguments.size(); i++) {
                                    buf.append(", ");
                                    generate_expression(buf, call->arguments[i], program);
                                }
                                buf.append(")");
                            }
                        } else {
                            generate_expression(buf, call->callee, program);
                            buf.append("(");
                            for (size_t i = 0; i < call->arguments.size(); i++) {
                                if (i > 0) buf.append(", ");
                                generate_expression(buf, call->arguments[i], program);
                            }
                            buf.append(")");
                        }
                    } else {
                        generate_expression(buf, call->callee, program);
                        buf.append("(");
                        for (size_t i = 0; i < call->arguments.size(); i++) {
                            if (i > 0) buf.append(", ");
                            generate_expression(buf, call->arguments[i], program);
                        }
                        buf.append(")");
                    }
                }
            } else if (call->callee->type == AstNodeType::IDENTIFIER_EXPR) {
                IdentifierExpr* ident = static_cast<IdentifierExpr*>(call->callee);

                ClassDecl* matching_class = nullptr;
                for (size_t i = 0; i < program->classes.size(); i++) {
                    if (program->classes[i]->name == ident->name) {
                        matching_class = program->classes[i];
                        break;
                    }
                }

                if (matching_class && !matching_class->is_dataclass) {
                    buf.append("({%s* __obj = (%s*)malloc(sizeof(%s)); memset(__obj, 0, sizeof(%s)); ",
                        matching_class->name.c_str(), matching_class->name.c_str(),
                        matching_class->name.c_str(), matching_class->name.c_str());

                    FunctionDecl* constructor = nullptr;
                    for (size_t i = 0; i < program->methods.size(); i++) {
                        if (program->methods[i]->class_name == matching_class->name &&
                            program->methods[i]->name == matching_class->name &&
                            !program->methods[i]->is_destructor) {
                            constructor = program->methods[i];
                            break;
                        }
                    }

                    if (constructor) {
                        buf.append("%s_%s(__obj", matching_class->name.c_str(), constructor->name.c_str());
                        for (size_t i = 0; i < call->arguments.size(); i++) {
                            buf.append(", ");
                            generate_expression(buf, call->arguments[i], program);
                        }
                        buf.append("); ");
                    }

                    buf.append("__obj; })");
                } else if (ident->name == "print" || ident->name == "println") {
                    buf.append("printf(");
                    if (call->arguments.size() > 0) {
                        generate_print_arg(buf, call->arguments[0], program);
                    }
                    if (ident->name == "println") {
                        if (call->arguments.size() == 0) {
                            buf.append("\"\\n\"");
                        } else {
                            buf.append("), printf(\"\\n\"");
                        }
                    }
                    buf.append(")");
                } else if (ident->name == "free") {
                    if (call->arguments.size() > 0) {
                        String arg_type = infer_expr_type(call->arguments[0], program);
                        bool is_arr = is_array_type_str(arg_type);
                        if (is_arr && call->arguments[0]->type == AstNodeType::IDENTIFIER_EXPR) {
                            IdentifierExpr* arg_id = static_cast<IdentifierExpr*>(call->arguments[0]);
                            bool is_param = is_array_param(arg_id->name);
                            const char* acc = is_param ? "->" : ".";
                            buf.append("{ free(%s%sptr); %s%sptr = NULL; %s%slen = 0; %s%scap = 0; }",
                                arg_id->name.c_str(), acc,
                                arg_id->name.c_str(), acc,
                                arg_id->name.c_str(), acc,
                                arg_id->name.c_str(), acc);
                        } else {
                            buf.append("free(");
                            generate_expression(buf, call->arguments[0], program);
                            buf.append(")");
                        }
                    }
                } else if (ident->name == "addr") {
                    if (call->arguments.size() > 0) {
                        String arg_type = infer_expr_type(call->arguments[0], program);
                        if (!arg_type.empty() && arg_type != "i32") {
                            char c_type[128];
                            tick_type_to_c_type(arg_type, program, c_type, sizeof(c_type));
                            buf.append("(%s*)&", c_type);
                        } else {
                            buf.append("(void*)&");
                        }
                        generate_expression(buf, call->arguments[0], program);
                    }
                } else if (ident->name == "deref") {
                    buf.append("(*");
                    if (call->arguments.size() > 0) {
                        generate_expression(buf, call->arguments[0], program);
                    }
                    buf.append(")");
                } else if (ident->name == "str_concat") {
                    buf.append("tick_str_concat(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        generate_expression(buf, call->arguments[i], program);
                    }
                    buf.append(")");
                } else if (ident->name == "str_substring") {
                    buf.append("tick_str_substring(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        generate_expression(buf, call->arguments[i], program);
                    }
                    buf.append(")");
                } else if (ident->name == "str_index_of") {
                    buf.append("tick_str_index_of(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        generate_expression(buf, call->arguments[i], program);
                    }
                    buf.append(")");
                } else if (ident->name == "str_compare") {
                    buf.append("tick_str_compare(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        generate_expression(buf, call->arguments[i], program);
                    }
                    buf.append(")");
                } else if (ident->name == "str_char_at") {
                    buf.append("tick_str_char_at(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        generate_expression(buf, call->arguments[i], program);
                    }
                    buf.append(")");
                } else if (ident->name == "to_str") {
                    String arg_type;
                    if (call->arguments.size() > 0) {
                        arg_type = infer_expr_type(call->arguments[0], program);
                    }
                    if (arg_type == "b8") {
                        buf.append("tick_str_from_b8(");
                    } else if (arg_type == "f32" || arg_type == "f64") {
                        buf.append("tick_str_from_f64((double)");
                    } else if (arg_type == "u64") {
                        buf.append("tick_str_from_u64(");
                    } else {
                        buf.append("tick_str_from_i64((int64_t)");
                    }
                    if (call->arguments.size() > 0) {
                        generate_expression(buf, call->arguments[0], program);
                    }
                    buf.append(")");
                } else if (ident->name == "parse") {
                    if (_expected_type == "b8") {
                        buf.append("tick_str_to_b8(");
                    } else if (_expected_type == "f32" || _expected_type == "f64") {
                        buf.append("(%s)tick_str_to_f64(", _expected_type == "f32" ? "float" : "double");
                    } else {
                        char c_type[128];
                        tick_type_to_c_type(_expected_type.length() > 0 ? _expected_type : String("i64"), program, c_type, sizeof(c_type));
                        buf.append("(%s)tick_str_to_i64(", c_type);
                    }
                    if (call->arguments.size() > 0) {
                        generate_expression(buf, call->arguments[0], program);
                    }
                    buf.append(")");
                } else if (ident->name == "file_open") {
                    buf.append("tick_file_open(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        generate_expression(buf, call->arguments[i], program);
                    }
                    buf.append(")");
                } else if (ident->name == "file_read") {
                    buf.append("tick_file_read(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        generate_expression(buf, call->arguments[i], program);
                    }
                    buf.append(")");
                } else if (ident->name == "file_write") {
                    buf.append("tick_file_write(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        generate_expression(buf, call->arguments[i], program);
                    }
                    buf.append(")");
                } else if (ident->name == "file_close") {
                    buf.append("tick_file_close(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        generate_expression(buf, call->arguments[i], program);
                    }
                    buf.append(")");
                } else if (ident->name == "file_exists") {
                    buf.append("tick_file_exists(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        generate_expression(buf, call->arguments[i], program);
                    }
                    buf.append(")");
                } else if (ident->name == "input") {
                    buf.append("tick_input_readline(");
                    if (call->arguments.size() > 0) {
                        generate_expression(buf, call->arguments[0], program);
                    } else {
                        buf.append("\"\"");
                    }
                    buf.append(")");
                } else {
                    FunctionDecl* target_func = nullptr;
                    for (size_t fi = 0; fi < program->functions.size(); fi++) {
                        if (program->functions[fi]->name == ident->name) {
                            target_func = program->functions[fi];
                            break;
                        }
                    }
                    generate_expression(buf, call->callee, program);
                    buf.append("(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) buf.append(", ");
                        if (target_func && i < target_func->parameters.size() &&
                            is_array_type_str(target_func->parameters[i]->type_name)) {
                            if (call->arguments[i]->type == AstNodeType::IDENTIFIER_EXPR) {
                                IdentifierExpr* arg_id = static_cast<IdentifierExpr*>(call->arguments[i]);
                                if (is_array_param(arg_id->name)) {
                                    buf.append("%s", arg_id->name.c_str());
                                } else {
                                    buf.append("&%s", arg_id->name.c_str());
                                }
                            } else {
                                generate_expression(buf, call->arguments[i], program);
                            }
                        } else {
                            generate_expression(buf, call->arguments[i], program);
                        }
                    }
                    buf.append(")");
                }
            } else {
                generate_expression(buf, call->callee, program);
                buf.append("(");
                for (size_t i = 0; i < call->arguments.size(); i++) {
                    if (i > 0) buf.append(", ");
                    generate_expression(buf, call->arguments[i], program);
                }
                buf.append(")");
            }
            break;
        }

        case AstNodeType::INDEX_EXPR: {
            IndexExpr* idx = static_cast<IndexExpr*>(expr);
            String arr_type = infer_expr_type(idx->array, program);
            if (is_array_type_str(arr_type) && idx->array->type == AstNodeType::IDENTIFIER_EXPR) {
                IdentifierExpr* arr_id = static_cast<IdentifierExpr*>(idx->array);
                String base_type(arr_type.c_str(), arr_type.length() - 2);
                char c_base[128];
                tick_type_to_c_type(base_type, program, c_base, sizeof(c_base));
                const char* acc = is_array_param(arr_id->name) ? "->" : ".";
                buf.append("((%s*)%s%sptr)[", c_base, arr_id->name.c_str(), acc);
                generate_expression(buf, idx->index, program);
                buf.append("]");
            } else {
                generate_expression(buf, idx->array, program);
                buf.append("[");
                generate_expression(buf, idx->index, program);
                buf.append("]");
            }
            break;
        }

        case AstNodeType::ARRAY_EXPR: {
            ArrayExpr* arr = static_cast<ArrayExpr*>(expr);
            buf.append("{");
            for (size_t i = 0; i < arr->elements.size(); i++) {
                if (i > 0) buf.append(", ");
                generate_expression(buf, arr->elements[i], program);
            }
            buf.append("}");
            break;
        }

        case AstNodeType::MEMBER_EXPR: {
            MemberExpr* member = static_cast<MemberExpr*>(expr);
            if (member->object->type == AstNodeType::IDENTIFIER_EXPR) {
                IdentifierExpr* obj_id = static_cast<IdentifierExpr*>(member->object);
                for (size_t i = 0; i < program->enums.size(); i++) {
                    if (program->enums[i]->name == obj_id->name) {
                        buf.append("%s_%s", obj_id->name.c_str(), member->member.c_str());
                        goto member_done;
                    }
                }
            }
            if (member->member == "length") {
                String obj_type = infer_expr_type(member->object, program);
                if (obj_type == "str") {
                    buf.append("tick_str_length(");
                    generate_expression(buf, member->object, program);
                    buf.append(")");
                    break;
                }
                if (is_array_type_str(obj_type) && member->object->type == AstNodeType::IDENTIFIER_EXPR) {
                    IdentifierExpr* obj_id = static_cast<IdentifierExpr*>(member->object);
                    const char* acc = is_array_param(obj_id->name) ? "->" : ".";
                    buf.append("%s%slen", obj_id->name.c_str(), acc);
                    break;
                }
            }
            generate_expression(buf, member->object, program);
            {
                String obj_type = infer_expr_type(member->object, program);
                bool use_dot = false;
                for (size_t i = 0; i < program->unions.size(); i++) {
                    if (program->unions[i]->name == obj_type) { use_dot = true; break; }
                }
                if (!use_dot) {
                    for (size_t i = 0; i < program->classes.size(); i++) {
                        if (program->classes[i]->name == obj_type && program->classes[i]->is_dataclass) {
                            use_dot = true; break;
                        }
                    }
                }
                if (!use_dot && member->object->type == AstNodeType::MEMBER_EXPR) {
                    MemberExpr* inner = static_cast<MemberExpr*>(member->object);
                    String inner_obj_type = infer_expr_type(inner->object, program);
                    for (size_t i = 0; i < program->unions.size(); i++) {
                        if (program->unions[i]->name == inner_obj_type) {
                            for (size_t j = 0; j < program->unions[i]->fields.size(); j++) {
                                if (program->unions[i]->fields[j]->name == inner->member) {
                                    for (size_t k = 0; k < program->classes.size(); k++) {
                                        if (program->classes[k]->name == program->unions[i]->fields[j]->type_name) {
                                            use_dot = true;
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                if (use_dot) {
                    buf.append(".%s", member->member.c_str());
                } else {
                    buf.append("->%s", member->member.c_str());
                }
            }
            member_done:
            break;
        }

        default:
            buf.append("0");
            break;
    }
}

void Compiler::write_to_file(const char* filename, const char* content) {
    FILE* f = fopen(filename, "w");
    if (f) {
        fprintf(f, "%s", content);
        fclose(f);
    }
}

bool Compiler::invoke_gcc(const char* c_file, const char* output_file, const char* extra_flags) {
    char cmd[2048];
    char exe_path[1024];
    char runtime_path[1024];

#ifdef __APPLE__
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) == 0) {
        char real[1024];
        if (realpath(exe_path, real)) strncpy(exe_path, real, sizeof(exe_path) - 1);
        char* last_slash = strrchr(exe_path, '/');
        if (last_slash) *last_slash = '\0';
    }
#else
    char link_path[64];
    snprintf(link_path, sizeof(link_path), "/proc/%d/exe", getpid());
    ssize_t len = readlink(link_path, exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        char* last_slash = strrchr(exe_path, '/');
        if (last_slash) *last_slash = '\0';
    }
#endif

    char candidate[1024];
    snprintf(candidate, sizeof(candidate), "%s/../src/runtime/tick_runtime.c", exe_path);
    if (access(candidate, F_OK) == 0) {
        strncpy(runtime_path, candidate, sizeof(runtime_path) - 1);
    } else {
        snprintf(candidate, sizeof(candidate), "%s/../share/tick/runtime/tick_runtime.c", exe_path);
        if (access(candidate, F_OK) == 0) {
            strncpy(runtime_path, candidate, sizeof(runtime_path) - 1);
        } else {
            strcpy(runtime_path, "src/runtime/tick_runtime.c");
        }
    }

    char include_path[1024];
    char inc_candidate[1024];
    snprintf(inc_candidate, sizeof(inc_candidate), "%s/../src", exe_path);
    if (access(inc_candidate, F_OK) == 0) {
        snprintf(include_path, sizeof(include_path), "-I%s", inc_candidate);
    } else {
        snprintf(include_path, sizeof(include_path), "-I%s/../share/tick", exe_path);
    }

    snprintf(cmd, sizeof(cmd), "gcc -O2 -o %s %s %s -pthread -lm %s %s",
        output_file, c_file, runtime_path, include_path, extra_flags ? extra_flags : "");

    int result = system(cmd);
    return result == 0;
}
