#include "semantic_analyzer.h"
#include <cstdio>

namespace Tick {

SemanticAnalyzer::SemanticAnalyzer() : _has_errors(false) {}

void SemanticAnalyzer::error(const char* message) {
    fprintf(stderr, "Semantic error: %s\n", message);
    _has_errors = true;
}

bool SemanticAnalyzer::analyze(Program* program) {
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
        analyze_function_decl(node->methods[i]);
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
