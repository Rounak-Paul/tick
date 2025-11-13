#include "semantic_analyzer.h"
#include <cstdio>
#include <cstring>

namespace Tick {

SemanticAnalyzer::SemanticAnalyzer() : _has_errors(false), _module_loader(nullptr), _current_file_path(nullptr) {}

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

bool SemanticAnalyzer::analyze(Program* program) {
    for (size_t i = 0; i < program->imports.size(); i++) {
        analyze_import_decl(program->imports[i], program);
    }
    
    for (size_t i = 0; i < program->globals.size(); i++) {
        VarDecl* var = program->globals[i];
        if (_symbols.contains(var->name.c_str())) {
            error("Duplicate global variable");
        } else {
            _symbols.insert(var->name.c_str(), new Symbol(SymbolType::VARIABLE, var->name, var->type_name));
        }
    }
    
    for (size_t i = 0; i < program->events.size(); i++) {
        analyze_event_decl(program->events[i]);
    }
    
    for (size_t i = 0; i < program->signals.size(); i++) {
        analyze_signal_decl(program->signals[i]);
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
            program->functions.push(imported_module->functions[i]);
            imported_module->functions[i] = nullptr;
        }
        
        for (size_t i = 0; i < imported_module->classes.size(); i++) {
            program->classes.push(imported_module->classes[i]);
            imported_module->classes[i] = nullptr;
        }
        
        for (size_t i = 0; i < imported_module->events.size(); i++) {
            program->events.push(imported_module->events[i]);
            imported_module->events[i] = nullptr;
        }
        
        for (size_t i = 0; i < imported_module->signals.size(); i++) {
            program->signals.push(imported_module->signals[i]);
            imported_module->signals[i] = nullptr;
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
        error("Event already declared");
        return;
    }
    
    Symbol* symbol = new Symbol(SymbolType::EVENT, node->name, String("event"));
    _symbols.insert(node->name.c_str(), symbol);
}

void SemanticAnalyzer::analyze_signal_decl(SignalDecl* node) {
    if (_symbols.contains(node->name.c_str())) {
        error("Signal already declared");
        return;
    }
    
    Symbol* symbol = new Symbol(SymbolType::SIGNAL, node->name, node->type_param);
    _symbols.insert(node->name.c_str(), symbol);
}

void SemanticAnalyzer::analyze_process_decl(ProcessDecl* node) {
    if (node->event_name.length() > 0 && !_symbols.contains(node->event_name.c_str())) {
    }
    
    if (_symbols.contains(node->name.c_str())) {
        error("Process already declared");
        return;
    }
    
    Symbol* symbol = new Symbol(SymbolType::PROCESS, node->name, String("process"));
    _symbols.insert(node->name.c_str(), symbol);
    
    analyze_block(node->body);
}

void SemanticAnalyzer::analyze_function_decl(FunctionDecl* node) {
    if (_symbols.contains(node->name.c_str())) {
        error("Function already declared");
        return;
    }
    
    Symbol* symbol = new Symbol(SymbolType::FUNCTION, node->name, node->return_type);
    _symbols.insert(node->name.c_str(), symbol);
    
    for (size_t i = 0; i < node->parameters.size(); i++) {
        Parameter* param = node->parameters[i];
        Symbol* param_sym = new Symbol(SymbolType::VARIABLE, param->name, param->type_name);
        _symbols.insert(param->name.c_str(), param_sym);
    }
    
    analyze_block(node->body);
    
    for (size_t i = 0; i < node->parameters.size(); i++) {
        _symbols.remove(node->parameters[i]->name.c_str());
    }
}

void SemanticAnalyzer::analyze_class_decl(ClassDecl* node) {
    if (_symbols.contains(node->name.c_str())) {
        error("Class already declared");
        return;
    }
    
    Symbol* symbol = new Symbol(SymbolType::CLASS, node->name, String("class"));
    _symbols.insert(node->name.c_str(), symbol);
    
    for (size_t i = 0; i < node->fields.size(); i++) {
        analyze_var_decl(node->fields[i]);
    }
    
    for (size_t i = 0; i < node->methods.size(); i++) {
        FunctionDecl* method = node->methods[i];
        char qualified_name_buf[256];
        snprintf(qualified_name_buf, sizeof(qualified_name_buf), "%s_%s", node->name.c_str(), method->name.c_str());
        
        char* qualified_name = strdup(qualified_name_buf);
        
        if (_symbols.contains(qualified_name)) {
            error("Method already declared");
            free(qualified_name);
            return;
        }
        
        Symbol* method_sym = new Symbol(SymbolType::FUNCTION, String(qualified_name), method->return_type);
        _symbols.insert(qualified_name, method_sym);
        
        for (size_t j = 0; j < method->parameters.size(); j++) {
            Parameter* param = method->parameters[j];
            Symbol* param_sym = new Symbol(SymbolType::VARIABLE, param->name, param->type_name);
            _symbols.insert(param->name.c_str(), param_sym);
        }
        
        if (method->body) {
            analyze_block(method->body);
        }
        
        for (size_t j = 0; j < method->parameters.size(); j++) {
            _symbols.remove(method->parameters[j]->name.c_str());
        }
    }
}

void SemanticAnalyzer::analyze_statement(StmtNode* node) {
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
        default:
            break;
    }
}

void SemanticAnalyzer::analyze_var_decl(VarDecl* node) {
    Symbol* symbol = new Symbol(SymbolType::VARIABLE, node->name, node->type_name);
    _symbols.insert(node->name.c_str(), symbol);
    
    if (node->initializer) {
        analyze_expression(node->initializer);
    }
}

void SemanticAnalyzer::analyze_if_stmt(IfStmt* node) {
    analyze_expression(node->condition);
    analyze_statement(node->then_branch);
    if (node->else_branch) {
        analyze_statement(node->else_branch);
    }
}

void SemanticAnalyzer::analyze_while_stmt(WhileStmt* node) {
    analyze_expression(node->condition);
    analyze_statement(node->body);
}

void SemanticAnalyzer::analyze_for_stmt(ForStmt* node) {
    if (node->initializer) {
        analyze_statement(node->initializer);
    }
    if (node->condition) {
        analyze_expression(node->condition);
    }
    if (node->increment) {
        analyze_expression(node->increment);
    }
    analyze_statement(node->body);
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
    for (size_t i = 0; i < node->statements.size(); i++) {
        analyze_statement(node->statements[i]);
    }
}

void SemanticAnalyzer::analyze_expression(ExprNode* node) {
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
        case AstNodeType::CALL_EXPR:
            analyze_call_expr(static_cast<CallExpr*>(node));
            break;
        case AstNodeType::MEMBER_EXPR:
            analyze_member_expr(static_cast<MemberExpr*>(node));
            break;
        case AstNodeType::IDENTIFIER_EXPR:
            analyze_identifier(static_cast<IdentifierExpr*>(node));
            break;
        case AstNodeType::NEW_EXPR:
            {
                NewExpr* new_expr = static_cast<NewExpr*>(node);
                for (size_t i = 0; i < new_expr->arguments.size(); i++) {
                    analyze_expression(new_expr->arguments[i]);
                }
            }
            break;
        case AstNodeType::THIS_EXPR:
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

void SemanticAnalyzer::analyze_call_expr(CallExpr* node) {
    analyze_expression(node->callee);
    for (size_t i = 0; i < node->arguments.size(); i++) {
        analyze_expression(node->arguments[i]);
    }
}

void SemanticAnalyzer::analyze_member_expr(MemberExpr* node) {
    analyze_expression(node->object);
}

void SemanticAnalyzer::analyze_identifier(IdentifierExpr* node) {
    
}

}
