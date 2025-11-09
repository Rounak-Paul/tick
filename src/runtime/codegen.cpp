#include "codegen.h"
#include <cstring>
#include <cstdio>

namespace Tick {

CodeGenerator::CodeGenerator() : _current_code(nullptr), _next_local_index(0) {}

void CodeGenerator::generate(Program* program) {
    for (size_t i = 0; i < program->functions.size(); i++) {
        generate_function(program->functions[i]);
    }
    
    for (size_t i = 0; i < program->processes.size(); i++) {
        generate_process(program->processes[i]);
    }
}

DynamicArray<Instruction>* CodeGenerator::get_function_code(const char* name) {
    DynamicArray<Instruction>** code = _function_code.find(name);
    return code ? *code : nullptr;
}

DynamicArray<Instruction>* CodeGenerator::get_process_code(const char* name) {
    DynamicArray<Instruction>** code = _process_code.find(name);
    return code ? *code : nullptr;
}

DynamicArray<Value>* CodeGenerator::get_constants() {
    return &_constants;
}

StringPool* CodeGenerator::get_string_pool() {
    return &_string_pool;
}

void CodeGenerator::generate_function(FunctionDecl* func) {
    _current_code = new DynamicArray<Instruction>();
    _local_vars.insert("", 0);
    _next_local_index = 0;
    
    for (size_t i = 0; i < func->parameters.size(); i++) {
        _local_vars.insert(func->parameters[i]->name.c_str(), _next_local_index++);
    }
    
    generate_block(func->body);
    
    emit(OpCode::LOAD_CONST, add_constant(Value(0)));
    emit(OpCode::RETURN);
    
    _function_code.insert(func->name.c_str(), _current_code);
}

void CodeGenerator::generate_process(ProcessDecl* proc) {
    _current_code = new DynamicArray<Instruction>();
    _local_vars.insert("", 0);
    _next_local_index = 0;
    
    generate_block(proc->body);
    emit(OpCode::HALT);
    
    _process_code.insert(proc->name.c_str(), _current_code);
}

void CodeGenerator::generate_statement(StmtNode* stmt) {
    switch (stmt->type) {
        case AstNodeType::VAR_DECL:
            generate_var_decl(static_cast<VarDecl*>(stmt));
            break;
        case AstNodeType::IF_STMT:
            generate_if_stmt(static_cast<IfStmt*>(stmt));
            break;
        case AstNodeType::WHILE_STMT:
            generate_while_stmt(static_cast<WhileStmt*>(stmt));
            break;
        case AstNodeType::RETURN_STMT:
            generate_return_stmt(static_cast<ReturnStmt*>(stmt));
            break;
        case AstNodeType::EXPR_STMT:
            generate_expr_stmt(static_cast<ExprStmt*>(stmt));
            break;
        case AstNodeType::BLOCK_STMT:
            generate_block(static_cast<BlockStmt*>(stmt));
            break;
        default:
            break;
    }
}

void CodeGenerator::generate_var_decl(VarDecl* node) {
    int index = _next_local_index++;
    _local_vars.insert(node->name.c_str(), index);
    
    if (node->initializer) {
        generate_expression(node->initializer);
        emit(OpCode::STORE_VAR, index);
    } else {
        emit(OpCode::LOAD_CONST, add_constant(Value(0)));
        emit(OpCode::STORE_VAR, index);
    }
}

void CodeGenerator::generate_if_stmt(IfStmt* node) {
    generate_expression(node->condition);
    
    int jump_to_else = _current_code->size();
    emit(OpCode::JUMP_IF_FALSE, 0);
    
    generate_statement(node->then_branch);
    
    int jump_to_end = _current_code->size();
    emit(OpCode::JUMP, 0);
    
    (*_current_code)[jump_to_else].operand = _current_code->size();
    
    if (node->else_branch) {
        generate_statement(node->else_branch);
    }
    
    (*_current_code)[jump_to_end].operand = _current_code->size();
}

void CodeGenerator::generate_while_stmt(WhileStmt* node) {
    int loop_start = _current_code->size();
    
    generate_expression(node->condition);
    
    int jump_to_end = _current_code->size();
    emit(OpCode::JUMP_IF_FALSE, 0);
    
    generate_statement(node->body);
    
    emit(OpCode::JUMP, loop_start);
    
    (*_current_code)[jump_to_end].operand = _current_code->size();
}

void CodeGenerator::generate_return_stmt(ReturnStmt* node) {
    if (node->value) {
        generate_expression(node->value);
    } else {
        emit(OpCode::LOAD_CONST, add_constant(Value(0)));
    }
    emit(OpCode::RETURN);
}

void CodeGenerator::generate_expr_stmt(ExprStmt* node) {
    generate_expression(node->expression);
    emit(OpCode::POP);
}

void CodeGenerator::generate_block(BlockStmt* node) {
    for (size_t i = 0; i < node->statements.size(); i++) {
        generate_statement(node->statements[i]);
    }
}

void CodeGenerator::generate_expression(ExprNode* expr) {
    switch (expr->type) {
        case AstNodeType::INTEGER_LITERAL:
            generate_integer_literal(static_cast<IntegerLiteral*>(expr));
            break;
        case AstNodeType::BOOL_LITERAL:
            generate_bool_literal(static_cast<BoolLiteral*>(expr));
            break;
        case AstNodeType::STRING_LITERAL:
            generate_string_literal(static_cast<StringLiteral*>(expr));
            break;
        case AstNodeType::IDENTIFIER_EXPR:
            generate_identifier(static_cast<IdentifierExpr*>(expr));
            break;
        case AstNodeType::BINARY_EXPR:
            generate_binary_expr(static_cast<BinaryExpr*>(expr));
            break;
        case AstNodeType::UNARY_EXPR:
            generate_unary_expr(static_cast<UnaryExpr*>(expr));
            break;
        case AstNodeType::CALL_EXPR:
            generate_call_expr(static_cast<CallExpr*>(expr));
            break;
        case AstNodeType::MEMBER_EXPR:
            generate_member_expr(static_cast<MemberExpr*>(expr));
            break;
        default:
            break;
    }
}

void CodeGenerator::generate_binary_expr(BinaryExpr* node) {
    generate_expression(node->left);
    generate_expression(node->right);
    
    if (node->op == "+") emit(OpCode::ADD);
    else if (node->op == "-") emit(OpCode::SUB);
    else if (node->op == "*") emit(OpCode::MUL);
    else if (node->op == "/") emit(OpCode::DIV);
    else if (node->op == "%") emit(OpCode::MOD);
    else if (node->op == "==") emit(OpCode::EQ);
    else if (node->op == "!=") emit(OpCode::NEQ);
    else if (node->op == "<") emit(OpCode::LT);
    else if (node->op == ">") emit(OpCode::GT);
    else if (node->op == "<=") emit(OpCode::LTE);
    else if (node->op == ">=") emit(OpCode::GTE);
    else if (node->op == "&&") emit(OpCode::AND);
    else if (node->op == "||") emit(OpCode::OR);
}

void CodeGenerator::generate_unary_expr(UnaryExpr* node) {
    generate_expression(node->operand);
    
    if (node->op == "-") emit(OpCode::NEG);
    else if (node->op == "!") emit(OpCode::NOT);
}

void CodeGenerator::generate_call_expr(CallExpr* node) {
    if (node->callee->type == AstNodeType::MEMBER_EXPR) {
        MemberExpr* member = static_cast<MemberExpr*>(node->callee);
        if (member->object->type == AstNodeType::IDENTIFIER_EXPR) {
            IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
            
            if (member->member == "emit") {
                for (size_t i = 0; i < node->arguments.size(); i++) {
                    generate_expression(node->arguments[i]);
                }
                int name_index = _string_pool.add(obj->name);
                emit(OpCode::SIGNAL_EMIT, name_index);
                return;
            } else if (member->member == "recv") {
                int name_index = _string_pool.add(obj->name);
                emit(OpCode::SIGNAL_RECV, name_index);
                return;
            } else if (member->member == "execute") {
                int name_index = _string_pool.add(obj->name);
                emit(OpCode::EVENT_EXECUTE, name_index);
                return;
            }
        }
    }
    
    for (size_t i = 0; i < node->arguments.size(); i++) {
        generate_expression(node->arguments[i]);
    }
    
    emit(OpCode::LOAD_CONST, add_constant(Value((int)node->arguments.size())));
    
    if (node->callee->type == AstNodeType::IDENTIFIER_EXPR) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->callee);
        int name_index = _string_pool.add(ident->name);
        emit(OpCode::CALL, name_index);
    }
}

void CodeGenerator::generate_member_expr(MemberExpr* node) {
    if (node->object->type == AstNodeType::IDENTIFIER_EXPR) {
        IdentifierExpr* obj = static_cast<IdentifierExpr*>(node->object);
        
        if (node->member == "emit") {
            int name_index = _string_pool.add(obj->name);
            emit(OpCode::SIGNAL_EMIT, name_index);
        } else if (node->member == "recv") {
            int name_index = _string_pool.add(obj->name);
            emit(OpCode::SIGNAL_RECV, name_index);
        } else if (node->member == "execute") {
            int name_index = _string_pool.add(obj->name);
            emit(OpCode::EVENT_EXECUTE, name_index);
        }
    }
}

void CodeGenerator::generate_identifier(IdentifierExpr* node) {
    int* index = _local_vars.find(node->name.c_str());
    if (index) {
        emit(OpCode::LOAD_VAR, *index);
    } else {
        int name_index = _string_pool.add(node->name);
        emit(OpCode::LOAD_GLOBAL, name_index);
    }
}

void CodeGenerator::generate_integer_literal(IntegerLiteral* node) {
    int const_index = add_constant(Value(node->value));
    emit(OpCode::LOAD_CONST, const_index);
}

void CodeGenerator::generate_bool_literal(BoolLiteral* node) {
    int const_index = add_constant(Value(node->value));
    emit(OpCode::LOAD_CONST, const_index);
}

void CodeGenerator::generate_string_literal(StringLiteral* node) {
    int str_index = _string_pool.add(node->value);
    int const_index = add_constant(Value(str_index, true));
    emit(OpCode::LOAD_CONST, const_index);
}

void CodeGenerator::emit(OpCode opcode, int operand) {
    _current_code->push(Instruction(opcode, operand));
}

int CodeGenerator::add_constant(Value value) {
    _constants.push(value);
    return _constants.size() - 1;
}

}
