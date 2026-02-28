#include "semantic_analyzer.h"
#include <cstdio>
#include <cstring>

namespace Tick {

SemanticAnalyzer::SemanticAnalyzer()
    : _scope_depth(0), _has_errors(false), _loop_depth(0),
      _module_loader(nullptr), _current_file_path(nullptr), _program(nullptr) {}

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

void SemanticAnalyzer::error(const char* message) {
    fprintf(stderr, "Semantic error: %s\n", message);
    _has_errors = true;
}

void SemanticAnalyzer::push_scope() {
    if (_scope_depth >= MAX_SCOPE_DEPTH - 1) {
        error("Maximum scope depth exceeded");
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

bool SemanticAnalyzer::analyze(Program* program) {
    _program = program;

    for (size_t i = 0; i < program->imports.size(); i++) {
        analyze_import_decl(program->imports[i], program);
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
            error(msg);
        } else {
            _symbols.insert(var->name.c_str(), new Symbol(SymbolType::VARIABLE, var->name, var->type_name));
        }
        if (var->initializer) {
            analyze_expression(var->initializer);
        }
    }

    for (size_t i = 0; i < program->events.size(); i++) {
        analyze_event_decl(program->events[i]);
    }

    for (size_t i = 0; i < program->signals.size(); i++) {
        analyze_signal_decl(program->signals[i]);
    }

    for (size_t i = 0; i < program->functions.size(); i++) {
        _symbols.insert(program->functions[i]->name.c_str(),
            new Symbol(SymbolType::FUNCTION, program->functions[i]->name, program->functions[i]->return_type));
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
        error("Module loader not set");
        return;
    }

    if (!_current_file_path) {
        error("Current file path not set");
        return;
    }

    Program* imported_module = _module_loader->load_module(node->module_path.c_str(), _current_file_path);
    if (!imported_module) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Failed to load module '%s'", node->module_path.c_str());
        error(err_msg);
        return;
    }

    if (node->import_all) {
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
    } else if (node->imported_names.size() > 0) {
        for (size_t i = 0; i < node->imported_names.size(); i++) {
            const char* name = node->imported_names[i].c_str();
            bool found = false;

            for (size_t j = 0; j < imported_module->functions.size(); j++) {
                if (imported_module->functions[j] && strcmp(imported_module->functions[j]->name.c_str(), name) == 0) {
                    program->functions.push(imported_module->functions[j]);
                    imported_module->functions[j] = nullptr;
                    found = true;
                    break;
                }
            }

            if (!found) {
                for (size_t j = 0; j < imported_module->classes.size(); j++) {
                    if (imported_module->classes[j] && strcmp(imported_module->classes[j]->name.c_str(), name) == 0) {
                        program->classes.push(imported_module->classes[j]);
                        imported_module->classes[j] = nullptr;
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                char err_msg[256];
                snprintf(err_msg, sizeof(err_msg), "Name '%s' not found in module '%s'", name, node->module_path.c_str());
                error(err_msg);
            }
        }
    }
}

void SemanticAnalyzer::analyze_event_decl(EventDecl* node) {
    if (_symbols.contains(node->name.c_str())) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Event '%s' already declared", node->name.c_str());
        error(msg);
        return;
    }

    _symbols.insert(node->name.c_str(), new Symbol(SymbolType::EVENT, node->name, String("event")));
}

void SemanticAnalyzer::analyze_signal_decl(SignalDecl* node) {
    if (_symbols.contains(node->name.c_str())) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Signal '%s' already declared", node->name.c_str());
        error(msg);
        return;
    }

    _symbols.insert(node->name.c_str(), new Symbol(SymbolType::SIGNAL, node->name, node->type_param));
}

void SemanticAnalyzer::analyze_process_decl(ProcessDecl* node) {
    if (node->event_name.length() > 0 && !_symbols.contains(node->event_name.c_str())) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Undeclared event '%s' referenced by process '%s'",
            node->event_name.c_str(), node->name.c_str());
        error(msg);
    }

    if (_symbols.contains(node->name.c_str())) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Process '%s' already declared", node->name.c_str());
        error(msg);
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
}

void SemanticAnalyzer::analyze_class_decl(ClassDecl* node) {
    push_scope();

    for (size_t i = 0; i < node->fields.size(); i++) {
        VarDecl* field = node->fields[i];
        declare_in_scope(field->name.c_str(), new Symbol(SymbolType::VARIABLE, field->name, field->type_name));
    }

    for (size_t i = 0; i < node->methods.size(); i++) {
        FunctionDecl* method = node->methods[i];

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
        case AstNodeType::BREAK_STMT:
            if (_loop_depth == 0) {
                error("'break' used outside of loop");
            }
            break;
        case AstNodeType::CONTINUE_STMT:
            if (_loop_depth == 0) {
                error("'continue' used outside of loop");
            }
            break;
        default:
            break;
    }
}

void SemanticAnalyzer::analyze_var_decl(VarDecl* node) {
    if (node->initializer) {
        analyze_expression(node->initializer);
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
            break;
        default:
            break;
    }
}

void SemanticAnalyzer::analyze_binary_expr(BinaryExpr* node) {
    analyze_expression(node->left);
    analyze_expression(node->right);
}

void SemanticAnalyzer::analyze_unary_expr(UnaryExpr* node) {
    analyze_expression(node->operand);
}

void SemanticAnalyzer::analyze_assign_expr(AssignExpr* node) {
    analyze_expression(node->target);
    analyze_expression(node->value);
}

void SemanticAnalyzer::analyze_compound_assign_expr(CompoundAssignExpr* node) {
    analyze_expression(node->target);
    analyze_expression(node->value);
}

void SemanticAnalyzer::analyze_call_expr(CallExpr* node) {
    if (node->callee->type == AstNodeType::IDENTIFIER_EXPR) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->callee);
        if (!(ident->name == "print" || ident->name == "println")) {
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
                error(msg);
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
}

void SemanticAnalyzer::analyze_identifier(IdentifierExpr* node) {
    if (!_symbols.contains(node->name.c_str())) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Use of undeclared identifier '%s'", node->name.c_str());
        error(msg);
    }
}

}
