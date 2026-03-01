#include "semantic_analyzer.h"
#include <cstdio>
#include <cstring>

namespace Tick {

SemanticAnalyzer::SemanticAnalyzer()
    : _scope_depth(0), _has_errors(false), _loop_depth(0),
      _module_loader(nullptr), _current_file_path(nullptr),
      _program(nullptr), _current_function(nullptr) {}

SemanticAnalyzer::~SemanticAnalyzer() {
    _symbols.for_each([](const char*, Symbol*& sym) {
        delete sym;
    });
}

void SemanticAnalyzer::set_module_loader(ModuleLoader* loader) {
    _module_loader = loader;
}

void SemanticAnalyzer::set_current_file_path(const char* path) {
    _current_file_path = path;
}

void SemanticAnalyzer::error(int line, const char* message) {
    if (line > 0) {
        fprintf(stderr, "Semantic error at line %d: %s\n", line, message);
    } else {
        fprintf(stderr, "Semantic error: %s\n", message);
    }
    _has_errors = true;
}

void SemanticAnalyzer::push_scope() {
    if (_scope_depth >= MAX_SCOPE_DEPTH - 1) {
        error(0, "Maximum scope depth exceeded");
        return;
    }
    _scope_depth++;
}

void SemanticAnalyzer::pop_scope() {
    if (_scope_depth <= 0) return;
    DynamicArray<ScopeEntry>& entries = _scope_stack[_scope_depth];
    for (size_t i = entries.size(); i > 0; i--) {
        ScopeEntry& entry = entries[i - 1];
        Symbol** current = _symbols.find(entry.name);
        if (current && !entry.had_previous) {
            delete *current;
            _symbols.remove(entry.name);
        } else if (current && entry.had_previous) {
            delete *current;
            _symbols.insert(entry.name, entry.previous);
        }
    }
    entries.clear();
    _scope_depth--;
}

void SemanticAnalyzer::declare_in_scope(const char* name, Symbol* sym) {
    ScopeEntry entry;
    entry.name = name;
    entry.had_previous = false;
    entry.previous = nullptr;

    Symbol** existing = _symbols.find(name);
    if (existing) {
        entry.had_previous = true;
        entry.previous = *existing;
    }

    _symbols.insert(name, sym);
    _scope_stack[_scope_depth].push(entry);
}

bool SemanticAnalyzer::is_numeric_type(const String& t) {
    return is_integer_type(t) || is_float_type(t);
}

bool SemanticAnalyzer::is_integer_type(const String& t) {
    return t == "i8" || t == "i16" || t == "i32" || t == "i64" ||
           t == "u8" || t == "u16" || t == "u32" || t == "u64";
}

bool SemanticAnalyzer::is_float_type(const String& t) {
    return t == "f32" || t == "f64";
}

bool SemanticAnalyzer::is_array_type(const String& t) {
    return t.length() > 2 && t[t.length() - 2] == '[' && t[t.length() - 1] == ']';
}

String SemanticAnalyzer::array_base_type(const String& t) {
    if (is_array_type(t)) {
        return String(t.c_str(), t.length() - 2);
    }
    return t;
}

bool SemanticAnalyzer::is_typed_ptr_type(const String& t) {
    return t.length() > 4 && t[0] == 'p' && t[1] == 't' && t[2] == 'r' && t[3] == '<' && t[t.length() - 1] == '>';
}

String SemanticAnalyzer::typed_ptr_base_type(const String& t) {
    if (is_typed_ptr_type(t)) {
        return String(t.c_str() + 4, t.length() - 5);
    }
    return t;
}

bool SemanticAnalyzer::is_any_ptr_type(const String& t) {
    return t == "ptr" || is_typed_ptr_type(t);
}

bool SemanticAnalyzer::is_func_ptr_type(const String& t) {
    return t.length() > 6 && t[0] == 'f' && t[1] == 'u' && t[2] == 'n' && t[3] == 'c' && t[4] == '(';
}

bool SemanticAnalyzer::types_compatible(const String& expected, const String& actual) {
    if (expected == actual) return true;
    if (expected.empty() || actual.empty()) return true;
    if (is_numeric_type(expected) && is_numeric_type(actual)) return true;
    if (is_numeric_type(expected) && actual == "b8") return true;
    if (expected == "b8" && is_numeric_type(actual)) return true;
    if (is_array_type(expected) && is_array_type(actual)) {
        String eb = array_base_type(expected);
        String ab = array_base_type(actual);
        return types_compatible(eb, ab);
    }
    if (expected == "b8" && actual == "b8") return true;
    if (expected == "str" && actual == "str") return true;
    if (is_any_ptr_type(expected) && is_any_ptr_type(actual)) return true;
    if (is_any_ptr_type(expected) && actual == "null") return true;
    if (actual == "null" && is_any_ptr_type(expected)) return true;
    if (is_any_ptr_type(expected) && is_func_ptr_type(actual)) return true;
    if (is_func_ptr_type(expected) && is_any_ptr_type(actual)) return true;
    if (is_func_ptr_type(expected) && actual == "null") return true;
    if (is_func_ptr_type(expected) && is_func_ptr_type(actual)) return true;
    if (actual == "null" && expected == "str") return true;
    if (actual == "null") {
        if (_program) {
            for (size_t i = 0; i < _program->classes.size(); i++) {
                if (_program->classes[i]->name == expected) return true;
            }
        }
        return false;
    }
    if (is_any_ptr_type(expected) || is_any_ptr_type(actual)) return true;
    if (_program) {
        String base = actual;
        for (int depth = 0; depth < 32; depth++) {
            for (size_t i = 0; i < _program->classes.size(); i++) {
                if (_program->classes[i]->name == base && !_program->classes[i]->base_class.empty()) {
                    base = _program->classes[i]->base_class;
                    if (base == expected) return true;
                    goto next_depth;
                }
            }
            break;
            next_depth:;
        }
    }
    return false;
}

bool SemanticAnalyzer::is_builtin_function(const String& name) {
    return name == "print" || name == "println" || name == "input" ||
           name == "sqrt" || name == "abs" || name == "pow" ||
           name == "sin" || name == "cos" || name == "tan" ||
           name == "floor" || name == "ceil" || name == "round" ||
           name == "log" || name == "log2" || name == "log10" ||
           name == "fmin" || name == "fmax" ||
           name == "strlen" || name == "str_concat" ||
           name == "str_substring" || name == "str_index_of" ||
           name == "str_compare" || name == "to_str" ||
           name == "parse" || name == "str_char_at" ||
           name == "file_open" || name == "file_read" ||
           name == "file_write" || name == "file_close" ||
           name == "file_exists" ||
           name == "free" || name == "array_length" ||
           name == "addr" || name == "sizeof" || name == "deref" ||
           name == "malloc" || name == "memset" || name == "memcpy" ||
           name == "memcmp" || name == "memmove";
}

String SemanticAnalyzer::infer_type(ExprNode* node) {
    if (!node) return String("");
    switch (node->type) {
        case AstNodeType::INTEGER_LITERAL: return String("i32");
        case AstNodeType::FLOAT_LITERAL: return String("f32");
        case AstNodeType::DOUBLE_LITERAL: return String("f64");
        case AstNodeType::BOOL_LITERAL: return String("b8");
        case AstNodeType::STRING_LITERAL: return String("str");
        case AstNodeType::NULL_LITERAL: return String("null");
        case AstNodeType::CAST_EXPR: {
            CastExpr* ce = static_cast<CastExpr*>(node);
            return ce->target_type;
        }
        case AstNodeType::SIZEOF_EXPR: return String("u64");
        case AstNodeType::IDENTIFIER_EXPR: {
            IdentifierExpr* id = static_cast<IdentifierExpr*>(node);
            Symbol** sym = _symbols.find(id->name.c_str());
            if (sym) {
                if ((*sym)->type == SymbolType::FUNCTION && _program) {
                    for (size_t i = 0; i < _program->functions.size(); i++) {
                        if (_program->functions[i]->name == id->name) {
                            FunctionDecl* fn = _program->functions[i];
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
                    for (size_t i = 0; i < _program->extern_functions.size(); i++) {
                        if (_program->extern_functions[i]->name == id->name) {
                            ExternFuncDecl* fn = _program->extern_functions[i];
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
                }
                return (*sym)->data_type;
            }
            return String("");
        }
        case AstNodeType::MEMBER_EXPR: {
            MemberExpr* me = static_cast<MemberExpr*>(node);
            if (me->member == "length") {
                String obj_type = infer_type(me->object);
                if (is_array_type(obj_type) || obj_type == "str") {
                    return String("i32");
                }
            }
            String obj_type = infer_type(me->object);
            if (_program) {
                for (size_t i = 0; i < _program->classes.size(); i++) {
                    if (_program->classes[i]->name == obj_type) {
                        for (size_t j = 0; j < _program->classes[i]->fields.size(); j++) {
                            if (_program->classes[i]->fields[j]->name == me->member) {
                                return _program->classes[i]->fields[j]->type_name;
                            }
                        }
                    }
                }
            }
            return String("");
        }
        case AstNodeType::CALL_EXPR: {
            CallExpr* call = static_cast<CallExpr*>(node);
            if (call->callee->type == AstNodeType::IDENTIFIER_EXPR) {
                IdentifierExpr* id = static_cast<IdentifierExpr*>(call->callee);
                if (id->name == "strlen" || id->name == "str_compare" ||
                    id->name == "str_index_of") {
                    return String("i32");
                }
                if (id->name == "str_concat" || id->name == "str_substring" ||
                    id->name == "to_str" ||
                    id->name == "file_read" || id->name == "input") {
                    return String("str");
                }
                if (id->name == "parse") return String("");
                if (id->name == "str_char_at") return String("u8");
                if (id->name == "file_exists") return String("b8");
                if (id->name == "addr") {
                    if (call->arguments.size() > 0) {
                        String arg_type = infer_type(call->arguments[0]);
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
                        String arg_type = infer_type(call->arguments[0]);
                        if (is_typed_ptr_type(arg_type)) {
                            return typed_ptr_base_type(arg_type);
                        }
                    }
                    return String("");
                }
                if (id->name == "sizeof") return String("u64");
                if (id->name == "sqrt" || id->name == "pow" ||
                    id->name == "sin" || id->name == "cos" ||
                    id->name == "tan" || id->name == "floor" ||
                    id->name == "ceil" || id->name == "round" ||
                    id->name == "log" || id->name == "log2" ||
                    id->name == "log10" || id->name == "fmin" ||
                    id->name == "fmax") {
                    return String("f64");
                }
                if (id->name == "abs") return String("i32");
                if (_program) {
                    for (size_t i = 0; i < _program->functions.size(); i++) {
                        if (_program->functions[i]->name == id->name) {
                            return _program->functions[i]->return_type;
                        }
                    }
                    for (size_t i = 0; i < _program->classes.size(); i++) {
                        if (_program->classes[i]->name == id->name) {
                            return id->name;
                        }
                    }
                }
            }
            if (call->callee->type == AstNodeType::MEMBER_EXPR) {
                MemberExpr* me = static_cast<MemberExpr*>(call->callee);
                if (me->member == "length") return String("i32");
                if (me->member == "push") return String("void");
                if (me->member == "pop") {
                    String arr_type = infer_type(me->object);
                    if (is_array_type(arr_type)) return array_base_type(arr_type);
                }
                if (me->member == "recv") {
                    if (_program && me->object->type == AstNodeType::IDENTIFIER_EXPR) {
                        IdentifierExpr* sig_id = static_cast<IdentifierExpr*>(me->object);
                        for (size_t i = 0; i < _program->signals.size(); i++) {
                            if (_program->signals[i]->name == sig_id->name) {
                                return _program->signals[i]->type_param;
                            }
                        }
                    }
                    if (_program && me->object->type == AstNodeType::INDEX_EXPR) {
                        IndexExpr* idx = static_cast<IndexExpr*>(me->object);
                        if (idx->array->type == AstNodeType::IDENTIFIER_EXPR) {
                            IdentifierExpr* sig_id = static_cast<IdentifierExpr*>(idx->array);
                            for (size_t i = 0; i < _program->signals.size(); i++) {
                                if (_program->signals[i]->name == sig_id->name) {
                                    return _program->signals[i]->type_param;
                                }
                            }
                        }
                    }
                    return String("i32");
                }
                String obj_type = infer_type(me->object);
                if (_program) {
                    for (size_t i = 0; i < _program->methods.size(); i++) {
                        if (_program->methods[i]->class_name == obj_type &&
                            _program->methods[i]->name == me->member) {
                            return _program->methods[i]->return_type;
                        }
                    }
                }
            }
            return String("");
        }
        case AstNodeType::BINARY_EXPR: {
            BinaryExpr* bin = static_cast<BinaryExpr*>(node);
            if (bin->op == "==" || bin->op == "!=" || bin->op == "<" ||
                bin->op == ">" || bin->op == "<=" || bin->op == ">=" ||
                bin->op == "&&" || bin->op == "||") {
                return String("b8");
            }
            String left = infer_type(bin->left);
            String right = infer_type(bin->right);
            if (bin->op == "+" && (left == "str" || right == "str")) {
                return String("str");
            }
            if ((bin->op == "+" || bin->op == "-") && is_any_ptr_type(left)) {
                return left;
            }
            if (bin->op == "+" && is_any_ptr_type(right)) {
                return right;
            }
            if (is_float_type(left) || is_float_type(right)) {
                if (left == "f64" || right == "f64") return String("f64");
                return String("f32");
            }
            return left.empty() ? right : left;
        }
        case AstNodeType::UNARY_EXPR: {
            UnaryExpr* un = static_cast<UnaryExpr*>(node);
            if (un->op == "!") return String("b8");
            return infer_type(un->operand);
        }
        case AstNodeType::POSTFIX_EXPR: {
            PostfixExpr* post = static_cast<PostfixExpr*>(node);
            return infer_type(post->operand);
        }
        case AstNodeType::INDEX_EXPR: {
            IndexExpr* idx = static_cast<IndexExpr*>(node);
            String arr_type = infer_type(idx->array);
            if (is_array_type(arr_type)) return array_base_type(arr_type);
            if (is_typed_ptr_type(arr_type)) return typed_ptr_base_type(arr_type);
            if (arr_type == "str") return String("u8");
            return String("");
        }
        case AstNodeType::ARRAY_EXPR: {
            ArrayExpr* arr = static_cast<ArrayExpr*>(node);
            if (arr->elements.size() > 0) {
                String elem_type = infer_type(arr->elements[0]);
                char buf[128];
                snprintf(buf, sizeof(buf), "%s[]", elem_type.c_str());
                return String(buf);
            }
            return String("");
        }
        case AstNodeType::THIS_EXPR:
            return String("");
        case AstNodeType::NEW_EXPR: {
            NewExpr* ne = static_cast<NewExpr*>(node);
            return ne->class_name;
        }
        default:
            return String("");
    }
}

bool SemanticAnalyzer::analyze(Program* program) {
    _program = program;

    for (size_t i = 0; i < program->imports.size(); i++) {
        analyze_import_decl(program->imports[i], program);
    }

    for (size_t i = 0; i < program->enums.size(); i++) {
        EnumDecl* ed = program->enums[i];
        int next_val = 0;
        for (size_t j = 0; j < ed->values.size(); j++) {
            int v = ed->values[j]->has_value ? ed->values[j]->value : next_val;
            char buf[256];
            snprintf(buf, sizeof(buf), "%s_%s", ed->name.c_str(), ed->values[j]->name.c_str());
            _symbols.insert(strdup(buf), new Symbol(SymbolType::VARIABLE, String(buf), String("i32")));
            next_val = v + 1;
        }
    }

    for (size_t i = 0; i < program->unions.size(); i++) {
        _symbols.insert(program->unions[i]->name.c_str(),
            new Symbol(SymbolType::CLASS, program->unions[i]->name, String("union")));
    }

    for (size_t i = 0; i < program->interfaces.size(); i++) {
        analyze_interface_decl(program->interfaces[i]);
    }

    for (size_t i = 0; i < program->classes.size(); i++) {
        Symbol* sym = new Symbol(SymbolType::CLASS, program->classes[i]->name, String("class"));
        _symbols.insert(program->classes[i]->name.c_str(), sym);
    }

    for (size_t i = 0; i < program->globals.size(); i++) {
        VarDecl* var = program->globals[i];
        if (_symbols.contains(var->name.c_str())) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Duplicate global variable '%s'", var->name.c_str());
            error(var->line, msg);
        } else {
            _symbols.insert(var->name.c_str(), new Symbol(SymbolType::VARIABLE, var->name, var->type_name));
        }
        if (var->initializer) {
            analyze_expression(var->initializer);
            String init_type = infer_type(var->initializer);
            if (!init_type.empty() && !var->type_name.empty() &&
                !types_compatible(var->type_name, init_type)) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Type mismatch: cannot assign '%s' to variable '%s' of type '%s'",
                    init_type.c_str(), var->name.c_str(), var->type_name.c_str());
                error(var->line, msg);
            }
        }
    }

    for (size_t i = 0; i < program->events.size(); i++) {
        analyze_event_decl(program->events[i]);
    }

    for (size_t i = 0; i < program->signals.size(); i++) {
        analyze_signal_decl(program->signals[i]);
    }

    for (size_t i = 0; i < program->functions.size(); i++) {
        int pc = (int)program->functions[i]->parameters.size();
        _symbols.insert(program->functions[i]->name.c_str(),
            new Symbol(SymbolType::FUNCTION, program->functions[i]->name,
                       program->functions[i]->return_type, pc));
    }

    for (size_t i = 0; i < program->extern_functions.size(); i++) {
        analyze_extern_func_decl(program->extern_functions[i]);
    }

    for (size_t i = 0; i < program->classes.size(); i++) {
        analyze_class_decl(program->classes[i]);
    }

    for (size_t i = 0; i < program->functions.size(); i++) {
        analyze_function_decl(program->functions[i]);
    }

    for (size_t i = 0; i < program->processes.size(); i++) {
        analyze_process_decl(program->processes[i]);
    }

    return !_has_errors;
}

void SemanticAnalyzer::analyze_import_decl(ImportDecl* node, Program* program) {
    if (!_module_loader) {
        error(node->line, "Module loader not set");
        return;
    }

    if (!_current_file_path) {
        error(node->line, "Current file path not set");
        return;
    }

    Program* imported_module = _module_loader->load_module(node->module_path.c_str(), _current_file_path);
    if (!imported_module) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Failed to load module '%s'", node->module_path.c_str());
        error(node->line, err_msg);
        return;
    }

    const char* saved_path = _current_file_path;
    String module_path = _module_loader->resolve_module_path(node->module_path.c_str(), _current_file_path);
    if (module_path.length() > 0) {
        _current_file_path = module_path.c_str();
    }
    for (size_t i = 0; i < imported_module->imports.size(); i++) {
        if (imported_module->imports[i]) {
            analyze_import_decl(imported_module->imports[i], imported_module);
        }
    }
    _current_file_path = saved_path;

    if (node->import_all) {
        for (size_t i = 0; i < imported_module->globals.size(); i++) {
            if (imported_module->globals[i]) {
                program->globals.push(imported_module->globals[i]);
                imported_module->globals[i] = nullptr;
            }
        }
        for (size_t i = 0; i < imported_module->extern_functions.size(); i++) {
            if (imported_module->extern_functions[i]) {
                program->extern_functions.push(imported_module->extern_functions[i]);
                imported_module->extern_functions[i] = nullptr;
            }
        }
        for (size_t i = 0; i < imported_module->link_flags.size(); i++) {
            program->link_flags.push(imported_module->link_flags[i]);
        }
        for (size_t i = 0; i < imported_module->functions.size(); i++) {
            if (imported_module->functions[i]) {
                program->functions.push(imported_module->functions[i]);
                imported_module->functions[i] = nullptr;
            }
        }
        for (size_t i = 0; i < imported_module->classes.size(); i++) {
            if (imported_module->classes[i]) {
                program->classes.push(imported_module->classes[i]);
                imported_module->classes[i] = nullptr;
            }
        }
        for (size_t i = 0; i < imported_module->enums.size(); i++) {
            if (imported_module->enums[i]) {
                program->enums.push(imported_module->enums[i]);
                imported_module->enums[i] = nullptr;
            }
        }
        for (size_t i = 0; i < imported_module->unions.size(); i++) {
            if (imported_module->unions[i]) {
                program->unions.push(imported_module->unions[i]);
                imported_module->unions[i] = nullptr;
            }
        }
        for (size_t i = 0; i < imported_module->interfaces.size(); i++) {
            if (imported_module->interfaces[i]) {
                program->interfaces.push(imported_module->interfaces[i]);
                imported_module->interfaces[i] = nullptr;
            }
        }
        for (size_t i = 0; i < imported_module->events.size(); i++) {
            if (imported_module->events[i]) {
                program->events.push(imported_module->events[i]);
                imported_module->events[i] = nullptr;
            }
        }
        for (size_t i = 0; i < imported_module->signals.size(); i++) {
            if (imported_module->signals[i]) {
                program->signals.push(imported_module->signals[i]);
                imported_module->signals[i] = nullptr;
            }
        }
        for (size_t i = 0; i < imported_module->methods.size(); i++) {
            if (imported_module->methods[i]) {
                program->methods.push(imported_module->methods[i]);
                imported_module->methods[i] = nullptr;
            }
        }
        for (size_t i = 0; i < imported_module->processes.size(); i++) {
            if (imported_module->processes[i]) {
                program->processes.push(imported_module->processes[i]);
                imported_module->processes[i] = nullptr;
            }
        }
    } else if (node->imported_names.size() > 0) {
        for (size_t i = 0; i < node->imported_names.size(); i++) {
            const char* name = node->imported_names[i].c_str();
            bool found = false;

            for (size_t j = 0; !found && j < imported_module->globals.size(); j++) {
                if (imported_module->globals[j] && strcmp(imported_module->globals[j]->name.c_str(), name) == 0) {
                    program->globals.push(imported_module->globals[j]);
                    imported_module->globals[j] = nullptr;
                    found = true;
                }
            }
            for (size_t j = 0; !found && j < imported_module->functions.size(); j++) {
                if (imported_module->functions[j] && strcmp(imported_module->functions[j]->name.c_str(), name) == 0) {
                    program->functions.push(imported_module->functions[j]);
                    imported_module->functions[j] = nullptr;
                    found = true;
                }
            }
            for (size_t j = 0; !found && j < imported_module->extern_functions.size(); j++) {
                if (imported_module->extern_functions[j] && strcmp(imported_module->extern_functions[j]->name.c_str(), name) == 0) {
                    program->extern_functions.push(imported_module->extern_functions[j]);
                    imported_module->extern_functions[j] = nullptr;
                    found = true;
                }
            }
            for (size_t j = 0; !found && j < imported_module->classes.size(); j++) {
                if (imported_module->classes[j] && strcmp(imported_module->classes[j]->name.c_str(), name) == 0) {
                    program->classes.push(imported_module->classes[j]);
                    imported_module->classes[j] = nullptr;
                    found = true;
                }
            }
            for (size_t j = 0; !found && j < imported_module->enums.size(); j++) {
                if (imported_module->enums[j] && strcmp(imported_module->enums[j]->name.c_str(), name) == 0) {
                    program->enums.push(imported_module->enums[j]);
                    imported_module->enums[j] = nullptr;
                    found = true;
                }
            }
            for (size_t j = 0; !found && j < imported_module->unions.size(); j++) {
                if (imported_module->unions[j] && strcmp(imported_module->unions[j]->name.c_str(), name) == 0) {
                    program->unions.push(imported_module->unions[j]);
                    imported_module->unions[j] = nullptr;
                    found = true;
                }
            }
            for (size_t j = 0; !found && j < imported_module->interfaces.size(); j++) {
                if (imported_module->interfaces[j] && strcmp(imported_module->interfaces[j]->name.c_str(), name) == 0) {
                    program->interfaces.push(imported_module->interfaces[j]);
                    imported_module->interfaces[j] = nullptr;
                    found = true;
                }
            }
            if (!found) {
                char err_msg[256];
                snprintf(err_msg, sizeof(err_msg), "Name '%s' not found in module '%s'", name, node->module_path.c_str());
                error(node->line, err_msg);
            }
        }
    }
}

void SemanticAnalyzer::analyze_event_decl(EventDecl* node) {
    if (_symbols.contains(node->name.c_str())) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Event '%s' already declared", node->name.c_str());
        error(node->line, msg);
        return;
    }
    _symbols.insert(node->name.c_str(), new Symbol(SymbolType::EVENT, node->name, String("event")));
}

void SemanticAnalyzer::analyze_signal_decl(SignalDecl* node) {
    if (_symbols.contains(node->name.c_str())) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Signal '%s' already declared", node->name.c_str());
        error(node->line, msg);
        return;
    }
    _symbols.insert(node->name.c_str(), new Symbol(SymbolType::SIGNAL, node->name, node->type_param));
}

void SemanticAnalyzer::analyze_process_decl(ProcessDecl* node) {
    if (node->event_name.length() > 0 && !_symbols.contains(node->event_name.c_str())) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Undeclared event '%s' referenced by process '%s'",
            node->event_name.c_str(), node->name.c_str());
        error(node->line, msg);
    }

    if (_symbols.contains(node->name.c_str())) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Process '%s' already declared", node->name.c_str());
        error(node->line, msg);
        return;
    }

    _symbols.insert(node->name.c_str(), new Symbol(SymbolType::PROCESS, node->name, String("process")));

    push_scope();
    if (node->body) {
        for (size_t i = 0; i < node->body->statements.size(); i++) {
            analyze_statement(node->body->statements[i]);
        }
    }
    pop_scope();
}

void SemanticAnalyzer::analyze_function_decl(FunctionDecl* node) {
    FunctionDecl* prev_func = _current_function;
    _current_function = node;

    push_scope();

    for (size_t i = 0; i < node->parameters.size(); i++) {
        Parameter* param = node->parameters[i];
        declare_in_scope(param->name.c_str(), new Symbol(SymbolType::VARIABLE, param->name, param->type_name));
    }

    if (node->body) {
        for (size_t i = 0; i < node->body->statements.size(); i++) {
            analyze_statement(node->body->statements[i]);
        }
    }

    pop_scope();
    _current_function = prev_func;
}

void SemanticAnalyzer::analyze_class_decl(ClassDecl* node) {
    if (!node->base_class.empty()) {
        bool found = false;
        for (size_t i = 0; i < _program->classes.size(); i++) {
            if (_program->classes[i]->name == node->base_class) {
                found = true;
                break;
            }
        }
        if (!found) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Base class '%s' not found for class '%s'",
                node->base_class.c_str(), node->name.c_str());
            error(node->line, msg);
        }
    }

    for (size_t i = 0; i < node->interfaces.size(); i++) {
        bool found = false;
        for (size_t j = 0; j < _program->interfaces.size(); j++) {
            if (_program->interfaces[j]->name == node->interfaces[i]) {
                found = true;
                InterfaceDecl* iface = _program->interfaces[j];
                for (size_t k = 0; k < iface->methods.size(); k++) {
                    bool has_method = false;
                    for (size_t m = 0; m < _program->methods.size(); m++) {
                        if (_program->methods[m]->class_name == node->name &&
                            _program->methods[m]->name == iface->methods[k]->name) {
                            has_method = true;
                            break;
                        }
                    }
                    if (!has_method) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Class '%s' does not implement method '%s' from interface '%s'",
                            node->name.c_str(), iface->methods[k]->name.c_str(), iface->name.c_str());
                        error(node->line, msg);
                    }
                }
                break;
            }
        }
        if (!found) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Interface '%s' not found for class '%s'",
                node->interfaces[i].c_str(), node->name.c_str());
            error(node->line, msg);
        }
    }

    push_scope();

    for (size_t i = 0; i < node->fields.size(); i++) {
        VarDecl* field = node->fields[i];
        declare_in_scope(field->name.c_str(), new Symbol(SymbolType::VARIABLE, field->name, field->type_name));
    }

    for (size_t i = 0; i < _program->methods.size(); i++) {
        FunctionDecl* method = _program->methods[i];
        if (!(method->class_name == node->name)) continue;
        FunctionDecl* prev_func = _current_function;
        _current_function = method;

        push_scope();

        declare_in_scope("self", new Symbol(SymbolType::VARIABLE, String("self"), node->name));

        for (size_t j = 0; j < method->parameters.size(); j++) {
            Parameter* param = method->parameters[j];
            declare_in_scope(param->name.c_str(), new Symbol(SymbolType::VARIABLE, param->name, param->type_name));
        }

        if (method->body) {
            for (size_t j = 0; j < method->body->statements.size(); j++) {
                analyze_statement(method->body->statements[j]);
            }
        }

        pop_scope();
        _current_function = prev_func;
    }

    pop_scope();
}

void SemanticAnalyzer::analyze_statement(StmtNode* node) {
    if (!node) return;
    switch (node->type) {
        case AstNodeType::VAR_DECL:
            analyze_var_decl(static_cast<VarDecl*>(node));
            break;
        case AstNodeType::IF_STMT:
            analyze_if_stmt(static_cast<IfStmt*>(node));
            break;
        case AstNodeType::WHILE_STMT:
            analyze_while_stmt(static_cast<WhileStmt*>(node));
            break;
        case AstNodeType::FOR_STMT:
            analyze_for_stmt(static_cast<ForStmt*>(node));
            break;
        case AstNodeType::RETURN_STMT:
            analyze_return_stmt(static_cast<ReturnStmt*>(node));
            break;
        case AstNodeType::EXPR_STMT:
            analyze_expr_stmt(static_cast<ExprStmt*>(node));
            break;
        case AstNodeType::BLOCK_STMT:
            analyze_block(static_cast<BlockStmt*>(node));
            break;
        case AstNodeType::DEFER_STMT:
            analyze_defer_stmt(static_cast<DeferStmt*>(node));
            break;
        case AstNodeType::SWITCH_STMT:
            analyze_switch_stmt(static_cast<SwitchStmt*>(node));
            break;
        case AstNodeType::TRY_CATCH_STMT:
            analyze_try_catch_stmt(static_cast<TryCatchStmt*>(node));
            break;
        case AstNodeType::THROW_STMT:
            analyze_throw_stmt(static_cast<ThrowStmt*>(node));
            break;
        case AstNodeType::BREAK_STMT:
            if (_loop_depth == 0) {
                error(node->line, "'break' used outside of loop");
            }
            break;
        case AstNodeType::CONTINUE_STMT:
            if (_loop_depth == 0) {
                error(node->line, "'continue' used outside of loop");
            }
            break;
        default:
            break;
    }
}

void SemanticAnalyzer::analyze_var_decl(VarDecl* node) {
    if (node->initializer) {
        analyze_expression(node->initializer);

        String init_type = infer_type(node->initializer);
        if (!init_type.empty() && !node->type_name.empty() &&
            !types_compatible(node->type_name, init_type)) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Type mismatch: cannot assign '%s' to variable '%s' of type '%s'",
                init_type.c_str(), node->name.c_str(), node->type_name.c_str());
            error(node->line, msg);
        }
    }

    declare_in_scope(node->name.c_str(), new Symbol(SymbolType::VARIABLE, node->name, node->type_name));
}

void SemanticAnalyzer::analyze_if_stmt(IfStmt* node) {
    analyze_expression(node->condition);
    push_scope();
    if (node->then_branch->type == AstNodeType::BLOCK_STMT) {
        BlockStmt* block = static_cast<BlockStmt*>(node->then_branch);
        for (size_t i = 0; i < block->statements.size(); i++) {
            analyze_statement(block->statements[i]);
        }
    } else {
        analyze_statement(node->then_branch);
    }
    pop_scope();

    if (node->else_branch) {
        push_scope();
        if (node->else_branch->type == AstNodeType::BLOCK_STMT) {
            BlockStmt* block = static_cast<BlockStmt*>(node->else_branch);
            for (size_t i = 0; i < block->statements.size(); i++) {
                analyze_statement(block->statements[i]);
            }
        } else {
            analyze_statement(node->else_branch);
        }
        pop_scope();
    }
}

void SemanticAnalyzer::analyze_while_stmt(WhileStmt* node) {
    analyze_expression(node->condition);
    _loop_depth++;
    push_scope();
    if (node->body->type == AstNodeType::BLOCK_STMT) {
        BlockStmt* block = static_cast<BlockStmt*>(node->body);
        for (size_t i = 0; i < block->statements.size(); i++) {
            analyze_statement(block->statements[i]);
        }
    } else {
        analyze_statement(node->body);
    }
    pop_scope();
    _loop_depth--;
}

void SemanticAnalyzer::analyze_for_stmt(ForStmt* node) {
    push_scope();
    if (node->initializer) {
        analyze_statement(node->initializer);
    }
    if (node->condition) {
        analyze_expression(node->condition);
    }
    if (node->increment) {
        analyze_expression(node->increment);
    }
    _loop_depth++;
    if (node->body->type == AstNodeType::BLOCK_STMT) {
        BlockStmt* block = static_cast<BlockStmt*>(node->body);
        for (size_t i = 0; i < block->statements.size(); i++) {
            analyze_statement(block->statements[i]);
        }
    } else {
        analyze_statement(node->body);
    }
    _loop_depth--;
    pop_scope();
}

void SemanticAnalyzer::analyze_return_stmt(ReturnStmt* node) {
    if (node->value) {
        analyze_expression(node->value);
        if (_current_function && _current_function->return_type != "void") {
            String ret_type = infer_type(node->value);
            if (!ret_type.empty() && !types_compatible(_current_function->return_type, ret_type)) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Return type mismatch: function '%s' expects '%s' but got '%s'",
                    _current_function->name.c_str(),
                    _current_function->return_type.c_str(),
                    ret_type.c_str());
                error(node->line, msg);
            }
        }
    }
}

void SemanticAnalyzer::analyze_expr_stmt(ExprStmt* node) {
    analyze_expression(node->expression);
}

void SemanticAnalyzer::analyze_block(BlockStmt* node) {
    push_scope();
    for (size_t i = 0; i < node->statements.size(); i++) {
        analyze_statement(node->statements[i]);
    }
    pop_scope();
}

void SemanticAnalyzer::analyze_defer_stmt(DeferStmt* node) {
    analyze_statement(node->statement);
}

void SemanticAnalyzer::analyze_switch_stmt(SwitchStmt* node) {
    analyze_expression(node->subject);
    for (size_t i = 0; i < node->cases.size(); i++) {
        SwitchCase* sc = node->cases[i];
        for (size_t j = 0; j < sc->values.size(); j++) {
            analyze_expression(sc->values[j]);
        }
        if (sc->body) {
            push_scope();
            for (size_t j = 0; j < sc->body->statements.size(); j++) {
                analyze_statement(sc->body->statements[j]);
            }
            pop_scope();
        }
    }
}

void SemanticAnalyzer::analyze_expression(ExprNode* node) {
    if (!node) return;
    switch (node->type) {
        case AstNodeType::BINARY_EXPR:
            analyze_binary_expr(static_cast<BinaryExpr*>(node));
            break;
        case AstNodeType::UNARY_EXPR:
            analyze_unary_expr(static_cast<UnaryExpr*>(node));
            break;
        case AstNodeType::ASSIGN_EXPR:
            analyze_assign_expr(static_cast<AssignExpr*>(node));
            break;
        case AstNodeType::COMPOUND_ASSIGN_EXPR:
            analyze_compound_assign_expr(static_cast<CompoundAssignExpr*>(node));
            break;
        case AstNodeType::POSTFIX_EXPR:
            analyze_postfix_expr(static_cast<PostfixExpr*>(node));
            break;
        case AstNodeType::CALL_EXPR:
            analyze_call_expr(static_cast<CallExpr*>(node));
            break;
        case AstNodeType::MEMBER_EXPR:
            analyze_member_expr(static_cast<MemberExpr*>(node));
            break;
        case AstNodeType::IDENTIFIER_EXPR:
            analyze_identifier(static_cast<IdentifierExpr*>(node));
            break;
        case AstNodeType::INDEX_EXPR:
            analyze_index_expr(static_cast<IndexExpr*>(node));
            break;
        case AstNodeType::ARRAY_EXPR: {
            ArrayExpr* arr = static_cast<ArrayExpr*>(node);
            for (size_t i = 0; i < arr->elements.size(); i++) {
                analyze_expression(arr->elements[i]);
            }
            break;
        }
        case AstNodeType::NEW_EXPR: {
            NewExpr* new_expr = static_cast<NewExpr*>(node);
            for (size_t i = 0; i < new_expr->arguments.size(); i++) {
                analyze_expression(new_expr->arguments[i]);
            }
            break;
        }
        case AstNodeType::THIS_EXPR:
        case AstNodeType::INTEGER_LITERAL:
        case AstNodeType::FLOAT_LITERAL:
        case AstNodeType::DOUBLE_LITERAL:
        case AstNodeType::BOOL_LITERAL:
        case AstNodeType::STRING_LITERAL:
        case AstNodeType::NULL_LITERAL:
        case AstNodeType::SIZEOF_EXPR:
            break;
        case AstNodeType::CAST_EXPR: {
            CastExpr* ce = static_cast<CastExpr*>(node);
            analyze_expression(ce->expression);
            break;
        }
        default:
            break;
    }
}

void SemanticAnalyzer::analyze_binary_expr(BinaryExpr* node) {
    analyze_expression(node->left);
    analyze_expression(node->right);

    String left_type = infer_type(node->left);
    String right_type = infer_type(node->right);

    if (node->op == "+" && (left_type == "str" || right_type == "str")) {
        return;
    }

    if (!left_type.empty() && !right_type.empty()) {
        if (node->op == "&&" || node->op == "||") {
            return;
        }
        if ((node->op == "==" || node->op == "!=") &&
            (left_type == "str" && right_type == "str")) {
            return;
        }
        if ((node->op == "==" || node->op == "!=") &&
            (left_type == "b8" && right_type == "b8")) {
            return;
        }
        if (is_numeric_type(left_type) && is_numeric_type(right_type)) {
            return;
        }
        if (left_type == "b8" && right_type == "b8" &&
            (node->op == "==" || node->op == "!=")) {
            return;
        }
    }
}

void SemanticAnalyzer::analyze_unary_expr(UnaryExpr* node) {
    analyze_expression(node->operand);
}

void SemanticAnalyzer::analyze_assign_expr(AssignExpr* node) {
    analyze_expression(node->target);
    analyze_expression(node->value);

    if (node->target->type == AstNodeType::IDENTIFIER_EXPR) {
        IdentifierExpr* id = static_cast<IdentifierExpr*>(node->target);
        Symbol** sym = _symbols.find(id->name.c_str());
        if (sym) {
            String val_type = infer_type(node->value);
            if (!val_type.empty() && !(*sym)->data_type.empty() &&
                !types_compatible((*sym)->data_type, val_type)) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Type mismatch: cannot assign '%s' to '%s' of type '%s'",
                    val_type.c_str(), id->name.c_str(), (*sym)->data_type.c_str());
                error(node->line, msg);
            }
        }
    }
}

void SemanticAnalyzer::analyze_compound_assign_expr(CompoundAssignExpr* node) {
    analyze_expression(node->target);
    analyze_expression(node->value);
}

void SemanticAnalyzer::analyze_postfix_expr(PostfixExpr* node) {
    analyze_expression(node->operand);
}

void SemanticAnalyzer::analyze_call_expr(CallExpr* node) {
    if (node->callee->type == AstNodeType::IDENTIFIER_EXPR) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->callee);
        if (!is_builtin_function(ident->name)) {
            bool found = _symbols.contains(ident->name.c_str());
            if (!found && _program) {
                for (size_t i = 0; i < _program->classes.size(); i++) {
                    if (_program->classes[i]->name == ident->name) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Call to undeclared function or class '%s'", ident->name.c_str());
                error(node->line, msg);
            }
        }

        Symbol** sym = _symbols.find(ident->name.c_str());
        if (sym && (*sym)->type == SymbolType::FUNCTION && (*sym)->param_count >= 0) {
            int expected = (*sym)->param_count;
            int actual = (int)node->arguments.size();
            if (expected != actual) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Function '%s' expects %d argument(s) but got %d",
                    ident->name.c_str(), expected, actual);
                error(node->line, msg);
            }
        }
    } else {
        analyze_expression(node->callee);
    }

    for (size_t i = 0; i < node->arguments.size(); i++) {
        analyze_expression(node->arguments[i]);
    }
}

void SemanticAnalyzer::analyze_member_expr(MemberExpr* node) {
    analyze_expression(node->object);
}

void SemanticAnalyzer::analyze_index_expr(IndexExpr* node) {
    analyze_expression(node->array);
    analyze_expression(node->index);

    String idx_type = infer_type(node->index);
    if (!idx_type.empty() && !is_integer_type(idx_type)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Array index must be an integer type, got '%s'", idx_type.c_str());
        error(node->line, msg);
    }
}

void SemanticAnalyzer::analyze_identifier(IdentifierExpr* node) {
    if (!_symbols.contains(node->name.c_str())) {
        if (_program) {
            for (size_t i = 0; i < _program->enums.size(); i++) {
                if (_program->enums[i]->name == node->name) return;
            }
        }
        char msg[256];
        snprintf(msg, sizeof(msg), "Use of undeclared identifier '%s'", node->name.c_str());
        error(node->line, msg);
    }
}

void SemanticAnalyzer::analyze_interface_decl(InterfaceDecl* node) {
    _symbols.insert(node->name.c_str(),
        new Symbol(SymbolType::CLASS, node->name, String("interface")));
}

void SemanticAnalyzer::analyze_extern_func_decl(ExternFuncDecl* node) {
    int pc = (int)node->parameters.size();
    _symbols.insert(node->name.c_str(),
        new Symbol(SymbolType::FUNCTION, node->name, node->return_type, pc));
}

void SemanticAnalyzer::analyze_try_catch_stmt(TryCatchStmt* node) {
    push_scope();
    if (node->try_body) {
        for (size_t i = 0; i < node->try_body->statements.size(); i++) {
            analyze_statement(node->try_body->statements[i]);
        }
    }
    pop_scope();

    push_scope();
    declare_in_scope(node->catch_var.c_str(),
        new Symbol(SymbolType::VARIABLE, node->catch_var, node->catch_type));
    if (node->catch_body) {
        for (size_t i = 0; i < node->catch_body->statements.size(); i++) {
            analyze_statement(node->catch_body->statements[i]);
        }
    }
    pop_scope();
}

void SemanticAnalyzer::analyze_throw_stmt(ThrowStmt* node) {
    if (node->value) {
        analyze_expression(node->value);
    }
}

}
