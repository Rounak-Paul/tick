#ifndef TICK_CODEGEN_H
#define TICK_CODEGEN_H

#include "../compiler/ast.h"
#include "bytecode.h"
#include "runtime.h"
#include "string_pool.h"
#include "../core/dynamic_array.h"
#include "../core/hash_map.h"

namespace Tick {

class CodeGenerator {
public:
    CodeGenerator();
    
    void generate(Program* program);
    DynamicArray<Instruction>* get_function_code(const char* name);
    DynamicArray<Instruction>* get_process_code(const char* name);
    DynamicArray<Value>* get_constants();
    StringPool* get_string_pool();

private:
    HashMap<const char*, DynamicArray<Instruction>*> _function_code;
    HashMap<const char*, DynamicArray<Instruction>*> _process_code;
    DynamicArray<Instruction>* _current_code;
    DynamicArray<Value> _constants;
    StringPool _string_pool;
    HashMap<const char*, int> _local_vars;
    int _next_local_index;
    
    void generate_function(FunctionDecl* func);
    void generate_process(ProcessDecl* proc);
    
    void generate_statement(StmtNode* stmt);
    void generate_var_decl(VarDecl* node);
    void generate_if_stmt(IfStmt* node);
    void generate_while_stmt(WhileStmt* node);
    void generate_return_stmt(ReturnStmt* node);
    void generate_expr_stmt(ExprStmt* node);
    void generate_block(BlockStmt* node);
    
    void generate_expression(ExprNode* expr);
    void generate_binary_expr(BinaryExpr* node);
    void generate_unary_expr(UnaryExpr* node);
    void generate_call_expr(CallExpr* node);
    void generate_member_expr(MemberExpr* node);
    void generate_identifier(IdentifierExpr* node);
    void generate_integer_literal(IntegerLiteral* node);
    void generate_bool_literal(BoolLiteral* node);
    void generate_string_literal(StringLiteral* node);
    
    void emit(OpCode opcode, int operand = 0);
    int add_constant(Value value);
};

}

#endif
