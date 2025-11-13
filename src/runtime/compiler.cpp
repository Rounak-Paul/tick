#include "compiler.h"
#include "../compiler/lexer.h"
#include "../compiler/parser.h"
#include "../compiler/semantic_analyzer.h"
#include "../compiler/module_loader.h"
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

using namespace Tick;

const char* Compiler::tick_type_to_c_type(const String& tick_type, Program* program) {
    if (tick_type == "void") return "void";
    if (tick_type == "float") return "double";
    if (tick_type == "bool") return "bool";
    if (tick_type == "string") return "char*";
    
    if (tick_type.length() > 2 && tick_type[tick_type.length() - 2] == '[' && tick_type[tick_type.length() - 1] == ']') {
        String base_type(tick_type.c_str(), tick_type.length() - 2);
        static char result[256];
        if (base_type == "int") {
            snprintf(result, sizeof(result), "int*");
        } else if (base_type == "float") {
            snprintf(result, sizeof(result), "double*");
        } else if (base_type == "double") {
            snprintf(result, sizeof(result), "double*");
        } else if (base_type == "bool") {
            snprintf(result, sizeof(result), "bool*");
        } else if (base_type == "string") {
            snprintf(result, sizeof(result), "char**");
        } else {
            snprintf(result, sizeof(result), "%s*", base_type.c_str());
        }
        return result;
    }
    
    if (program) {
        for (size_t i = 0; i < program->classes.size(); i++) {
            if (program->classes[i]->name == tick_type) {
                static char result[256];
                snprintf(result, sizeof(result), "%s*", tick_type.c_str());
                return result;
            }
        }
    }
    
    return "int";
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
    
    bool success = invoke_gcc(temp_c, output_file);
    
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
    char* buffer = (char*)malloc(524288);
    int pos = 0;
    
    pos += sprintf(buffer + pos, "#include <stdio.h>\n");
    pos += sprintf(buffer + pos, "#include <stdlib.h>\n");
    pos += sprintf(buffer + pos, "#include <string.h>\n");
    pos += sprintf(buffer + pos, "#include <stdbool.h>\n");
    pos += sprintf(buffer + pos, "#include <stdint.h>\n");
    pos += sprintf(buffer + pos, "#include \"runtime/tick_runtime.h\"\n\n");
    
    for (size_t i = 0; i < program->globals.size(); i++) {
        VarDecl* var = program->globals[i];
        
        if (var->is_const) {
            pos += sprintf(buffer + pos, "const ");
        }
        
        bool is_array = var->type_name.length() > 2 && 
                       var->type_name[var->type_name.length() - 2] == '[' && 
                       var->type_name[var->type_name.length() - 1] == ']';
        
        if (is_array && var->initializer && var->initializer->type == AstNodeType::ARRAY_EXPR) {
            ArrayExpr* arr = static_cast<ArrayExpr*>(var->initializer);
            String base_type(var->type_name.c_str(), var->type_name.length() - 2);
            const char* c_base_type = tick_type_to_c_type(base_type, program);
            
            pos += sprintf(buffer + pos, "%s %s[] = ", c_base_type, var->name.c_str());
            pos = generate_expression(buffer, pos, var->initializer, program);
        } else {
            const char* c_type = tick_type_to_c_type(var->type_name, program);
            pos += sprintf(buffer + pos, "%s %s", c_type, var->name.c_str());
            if (var->initializer) {
                pos += sprintf(buffer + pos, " = ");
                pos = generate_expression(buffer, pos, var->initializer, program);
            }
        }
        pos += sprintf(buffer + pos, ";\n");
    }
    
    if (program->globals.size() > 0) {
        pos += sprintf(buffer + pos, "\n");
    }
    
    for (size_t i = 0; i < program->classes.size(); i++) {
        ClassDecl* cls = program->classes[i];
        pos += sprintf(buffer + pos, "typedef struct %s {\n", cls->name.c_str());
        
        for (size_t j = 0; j < cls->fields.size(); j++) {
            VarDecl* field = cls->fields[j];
            const char* field_type = tick_type_to_c_type(field->type_name, program);
            pos += sprintf(buffer + pos, "    %s %s;\n", field_type, field->name.c_str());
        }
        
        pos += sprintf(buffer + pos, "} %s;\n\n", cls->name.c_str());
    }
    
    for (size_t i = 0; i < program->signals.size(); i++) {
        SignalDecl* sig = program->signals[i];
        if (sig->array_size > 0) {
            pos += sprintf(buffer + pos, "TickSignal %s[%d];\n", sig->name.c_str(), sig->array_size);
        } else {
            pos += sprintf(buffer + pos, "TickSignal %s;\n", sig->name.c_str());
        }
    }
    
    for (size_t i = 0; i < program->events.size(); i++) {
        EventDecl* evt = program->events[i];
        pos += sprintf(buffer + pos, "TickEvent %s;\n", evt->name.c_str());
    }
    
    if (program->signals.size() > 0 || program->events.size() > 0) {
        pos += sprintf(buffer + pos, "\n");
    }
    
    for (size_t i = 0; i < program->processes.size(); i++) {
        ProcessDecl* proc = program->processes[i];
        pos += sprintf(buffer + pos, "void* %s(void* arg);\n", proc->name.c_str());
    }
    
    for (size_t i = 0; i < program->functions.size(); i++) {
        FunctionDecl* func = program->functions[i];
        
        const char* ret_type = tick_type_to_c_type(func->return_type, program);
        
        pos += sprintf(buffer + pos, "%s %s(", ret_type, func->name.c_str());
        
        for (size_t j = 0; j < func->parameters.size(); j++) {
            if (j > 0) pos += sprintf(buffer + pos, ", ");
            const char* param_type = tick_type_to_c_type(func->parameters[j]->type_name, program);
            pos += sprintf(buffer + pos, "%s %s", param_type, func->parameters[j]->name.c_str());
        }
        
        pos += sprintf(buffer + pos, ");\n");
    }
    
    for (size_t i = 0; i < program->classes.size(); i++) {
        ClassDecl* cls = program->classes[i];
        for (size_t j = 0; j < cls->methods.size(); j++) {
            FunctionDecl* method = cls->methods[j];
            const char* ret_type = tick_type_to_c_type(method->return_type, program);
            
            pos += sprintf(buffer + pos, "%s %s_%s(%s* this", 
                ret_type, cls->name.c_str(), method->name.c_str(), cls->name.c_str());
            
            for (size_t k = 0; k < method->parameters.size(); k++) {
                pos += sprintf(buffer + pos, ", ");
                const char* param_type = tick_type_to_c_type(method->parameters[k]->type_name, program);
                pos += sprintf(buffer + pos, "%s %s", param_type, method->parameters[k]->name.c_str());
            }
            
            pos += sprintf(buffer + pos, ");\n");
        }
    }
    
    pos += sprintf(buffer + pos, "\n");
    
    for (size_t i = 0; i < program->processes.size(); i++) {
        ProcessDecl* proc = program->processes[i];
        pos = generate_process(buffer, pos, proc, program);
    }
    
    for (size_t i = 0; i < program->functions.size(); i++) {
        FunctionDecl* func = program->functions[i];
        pos = generate_function(buffer, pos, func, program);
    }
    
    for (size_t i = 0; i < program->classes.size(); i++) {
        ClassDecl* cls = program->classes[i];
        for (size_t j = 0; j < cls->methods.size(); j++) {
            FunctionDecl* method = cls->methods[j];
            const char* ret_type = tick_type_to_c_type(method->return_type, program);
            
            pos += sprintf(buffer + pos, "%s %s_%s(%s* this", 
                ret_type, cls->name.c_str(), method->name.c_str(), cls->name.c_str());
            
            for (size_t k = 0; k < method->parameters.size(); k++) {
                pos += sprintf(buffer + pos, ", ");
                const char* param_type = tick_type_to_c_type(method->parameters[k]->type_name, program);
                pos += sprintf(buffer + pos, "%s %s", param_type, method->parameters[k]->name.c_str());
            }
            
            pos += sprintf(buffer + pos, ") {\n");
            
            if (method->body) {
                for (size_t k = 0; k < method->body->statements.size(); k++) {
                    pos = generate_statement(buffer, pos, method->body->statements[k], 1, program);
                }
            }
            
            pos += sprintf(buffer + pos, "}\n\n");
        }
    }
    
    String result(buffer);
    free(buffer);
    return result;
}

int Compiler::generate_process(char* buffer, int pos, ProcessDecl* proc, Program* program) {
    pos += sprintf(buffer + pos, "void* %s(void* arg) {\n", proc->name.c_str());
    
    BlockStmt* body = proc->body;
    for (size_t i = 0; i < body->statements.size(); i++) {
        pos = generate_statement(buffer, pos, body->statements[i], 1, program);
    }
    
    pos += sprintf(buffer + pos, "    return NULL;\n");
    pos += sprintf(buffer + pos, "}\n\n");
    
    return pos;
}

int Compiler::generate_function(char* buffer, int pos, FunctionDecl* func, Program* program) {
    const char* ret_type = tick_type_to_c_type(func->return_type, program);
    
    pos += sprintf(buffer + pos, "%s %s(", ret_type, func->name.c_str());
    
    for (size_t j = 0; j < func->parameters.size(); j++) {
        if (j > 0) pos += sprintf(buffer + pos, ", ");
        const char* param_type = tick_type_to_c_type(func->parameters[j]->type_name, program);
        pos += sprintf(buffer + pos, "%s %s", param_type, func->parameters[j]->name.c_str());
    }
    
    pos += sprintf(buffer + pos, ") {\n");
    
    if (func->name == "main") {
        for (size_t i = 0; i < program->signals.size(); i++) {
            SignalDecl* sig = program->signals[i];
            if (sig->array_size > 0) {
                pos += sprintf(buffer + pos, "    for (int _i = 0; _i < %d; _i++) {\n", sig->array_size);
                pos += sprintf(buffer + pos, "        tick_signal_init(&%s[_i]);\n", sig->name.c_str());
                pos += sprintf(buffer + pos, "    }\n");
            } else {
                pos += sprintf(buffer + pos, "    tick_signal_init(&%s);\n", sig->name.c_str());
            }
        }
        
        for (size_t i = 0; i < program->events.size(); i++) {
            EventDecl* evt = program->events[i];
            
            int proc_count = 0;
            for (size_t j = 0; j < program->processes.size(); j++) {
                if (program->processes[j]->event_name == evt->name) {
                    proc_count++;
                }
            }
            
            pos += sprintf(buffer + pos, "    tick_event_init(&%s, %d);\n", evt->name.c_str(), proc_count);
            
            for (size_t j = 0; j < program->processes.size(); j++) {
                ProcessDecl* proc = program->processes[j];
                if (proc->event_name == evt->name) {
                    pos += sprintf(buffer + pos, "    tick_event_add_process(&%s, %s);\n", 
                        evt->name.c_str(), proc->name.c_str());
                }
            }
        }
        
        if (program->signals.size() > 0 || program->events.size() > 0) {
            pos += sprintf(buffer + pos, "\n");
        }
    }
    
    if (func->body) {
        for (size_t i = 0; i < func->body->statements.size(); i++) {
            pos = generate_statement(buffer, pos, func->body->statements[i], 1, program);
        }
    }
    
    pos += sprintf(buffer + pos, "}\n\n");
    return pos;
}

int Compiler::generate_statement(char* buffer, int pos, StmtNode* stmt, int indent, Program* program) {
    if (!stmt) return pos;
    
    switch (stmt->type) {
        case AstNodeType::VAR_DECL: {
            VarDecl* decl = static_cast<VarDecl*>(stmt);
            for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
            
            if (decl->is_const) {
                pos += sprintf(buffer + pos, "const ");
            }
            
            const char* type = tick_type_to_c_type(decl->type_name, program);
            bool is_array = decl->type_name.length() > 2 && 
                           decl->type_name[decl->type_name.length() - 2] == '[' && 
                           decl->type_name[decl->type_name.length() - 1] == ']';
            
            if (is_array && decl->initializer && decl->initializer->type == AstNodeType::ARRAY_EXPR) {
                ArrayExpr* arr = static_cast<ArrayExpr*>(decl->initializer);
                String base_type(decl->type_name.c_str(), decl->type_name.length() - 2);
                const char* c_base_type = tick_type_to_c_type(base_type, program);
                
                pos += sprintf(buffer + pos, "%s %s[] = ", c_base_type, decl->name.c_str());
                pos = generate_expression(buffer, pos, decl->initializer, program);
            } else {
                pos += sprintf(buffer + pos, "%s %s", type, decl->name.c_str());
                
                if (decl->initializer) {
                    pos += sprintf(buffer + pos, " = ");
                    pos = generate_expression(buffer, pos, decl->initializer, program);
                } else {
                    pos += sprintf(buffer + pos, " = 0");
                }
            }
            
            pos += sprintf(buffer + pos, ";\n");
            break;
        }
        
        case AstNodeType::EXPR_STMT: {
            ExprStmt* expr_stmt = static_cast<ExprStmt*>(stmt);
            for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
            pos = generate_expression(buffer, pos, expr_stmt->expression, program);
            pos += sprintf(buffer + pos, ";\n");
            break;
        }
        
        case AstNodeType::RETURN_STMT: {
            ReturnStmt* ret = static_cast<ReturnStmt*>(stmt);
            for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
            pos += sprintf(buffer + pos, "return");
            if (ret->value) {
                pos += sprintf(buffer + pos, " ");
                pos = generate_expression(buffer, pos, ret->value, program);
            }
            pos += sprintf(buffer + pos, ";\n");
            break;
        }
        
        case AstNodeType::IF_STMT: {
            IfStmt* if_stmt = static_cast<IfStmt*>(stmt);
            for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
            pos += sprintf(buffer + pos, "if (");
            pos = generate_expression(buffer, pos, if_stmt->condition, program);
            pos += sprintf(buffer + pos, ") {\n");
            
            if (if_stmt->then_branch->type == AstNodeType::BLOCK_STMT) {
                BlockStmt* block = static_cast<BlockStmt*>(if_stmt->then_branch);
                for (size_t i = 0; i < block->statements.size(); i++) {
                    pos = generate_statement(buffer, pos, block->statements[i], indent + 1, program);
                }
            } else {
                pos = generate_statement(buffer, pos, if_stmt->then_branch, indent + 1, program);
            }
            
            for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
            pos += sprintf(buffer + pos, "}");
            
            if (if_stmt->else_branch) {
                pos += sprintf(buffer + pos, " else {\n");
                if (if_stmt->else_branch->type == AstNodeType::BLOCK_STMT) {
                    BlockStmt* block = static_cast<BlockStmt*>(if_stmt->else_branch);
                    for (size_t i = 0; i < block->statements.size(); i++) {
                        pos = generate_statement(buffer, pos, block->statements[i], indent + 1, program);
                    }
                } else {
                    pos = generate_statement(buffer, pos, if_stmt->else_branch, indent + 1, program);
                }
                for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
                pos += sprintf(buffer + pos, "}");
            }
            
            pos += sprintf(buffer + pos, "\n");
            break;
        }
        
        case AstNodeType::WHILE_STMT: {
            WhileStmt* while_stmt = static_cast<WhileStmt*>(stmt);
            for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
            pos += sprintf(buffer + pos, "while (");
            pos = generate_expression(buffer, pos, while_stmt->condition, program);
            pos += sprintf(buffer + pos, ") {\n");
            
            if (while_stmt->body->type == AstNodeType::BLOCK_STMT) {
                BlockStmt* block = static_cast<BlockStmt*>(while_stmt->body);
                for (size_t i = 0; i < block->statements.size(); i++) {
                    pos = generate_statement(buffer, pos, block->statements[i], indent + 1, program);
                }
            } else {
                pos = generate_statement(buffer, pos, while_stmt->body, indent + 1, program);
            }
            
            for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
            pos += sprintf(buffer + pos, "}\n");
            break;
        }
        
        case AstNodeType::FOR_STMT: {
            ForStmt* for_stmt = static_cast<ForStmt*>(stmt);
            for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
            pos += sprintf(buffer + pos, "for (");
            
            if (for_stmt->initializer) {
                if (for_stmt->initializer->type == AstNodeType::VAR_DECL) {
                    VarDecl* decl = static_cast<VarDecl*>(for_stmt->initializer);
                    const char* type = tick_type_to_c_type(decl->type_name, program);
                    
                    pos += sprintf(buffer + pos, "%s %s", type, decl->name.c_str());
                    if (decl->initializer) {
                        pos += sprintf(buffer + pos, " = ");
                        pos = generate_expression(buffer, pos, decl->initializer, program);
                    } else {
                        pos += sprintf(buffer + pos, " = 0");
                    }
                } else {
                    pos = generate_expression(buffer, pos, static_cast<ExprStmt*>(for_stmt->initializer)->expression, program);
                }
            }
            pos += sprintf(buffer + pos, "; ");
            
            if (for_stmt->condition) {
                pos = generate_expression(buffer, pos, for_stmt->condition, program);
            }
            pos += sprintf(buffer + pos, "; ");
            
            if (for_stmt->increment) {
                pos = generate_expression(buffer, pos, for_stmt->increment, program);
            }
            pos += sprintf(buffer + pos, ") {\n");
            
            if (for_stmt->body->type == AstNodeType::BLOCK_STMT) {
                BlockStmt* block = static_cast<BlockStmt*>(for_stmt->body);
                for (size_t i = 0; i < block->statements.size(); i++) {
                    pos = generate_statement(buffer, pos, block->statements[i], indent + 1, program);
                }
            } else {
                pos = generate_statement(buffer, pos, for_stmt->body, indent + 1, program);
            }
            
            for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
            pos += sprintf(buffer + pos, "}\n");
            break;
        }
        
        case AstNodeType::BREAK_STMT: {
            for (int i = 0; i < indent; i++) pos += sprintf(buffer + pos, "    ");
            pos += sprintf(buffer + pos, "break;\n");
            break;
        }
        
        default:
            break;
    }
    
    return pos;
}

int Compiler::generate_expression(char* buffer, int pos, ExprNode* expr, Program* program) {
    if (!expr) return pos;
    
    switch (expr->type) {
        case AstNodeType::INTEGER_LITERAL: {
            IntegerLiteral* lit = static_cast<IntegerLiteral*>(expr);
            pos += sprintf(buffer + pos, "%d", lit->value);
            break;
        }
        
        case AstNodeType::FLOAT_LITERAL: {
            FloatLiteral* lit = static_cast<FloatLiteral*>(expr);
            pos += sprintf(buffer + pos, "%f", lit->value);
            break;
        }
        
        case AstNodeType::DOUBLE_LITERAL: {
            DoubleLiteral* lit = static_cast<DoubleLiteral*>(expr);
            pos += sprintf(buffer + pos, "%f", lit->value);
            break;
        }
        
        case AstNodeType::BOOL_LITERAL: {
            BoolLiteral* lit = static_cast<BoolLiteral*>(expr);
            pos += sprintf(buffer + pos, "%s", lit->value ? "true" : "false");
            break;
        }
        
        case AstNodeType::STRING_LITERAL: {
            StringLiteral* lit = static_cast<StringLiteral*>(expr);
            pos += sprintf(buffer + pos, "\"%s\"", lit->value.c_str());
            break;
        }
        
        case AstNodeType::IDENTIFIER_EXPR: {
            IdentifierExpr* ident = static_cast<IdentifierExpr*>(expr);
            pos += sprintf(buffer + pos, "%s", ident->name.c_str());
            break;
        }
        
        case AstNodeType::THIS_EXPR: {
            pos += sprintf(buffer + pos, "this");
            break;
        }
        
        case AstNodeType::BINARY_EXPR: {
            BinaryExpr* bin = static_cast<BinaryExpr*>(expr);
            pos += sprintf(buffer + pos, "(");
            pos = generate_expression(buffer, pos, bin->left, program);
            pos += sprintf(buffer + pos, " %s ", bin->op.c_str());
            pos = generate_expression(buffer, pos, bin->right, program);
            pos += sprintf(buffer + pos, ")");
            break;
        }
        
        case AstNodeType::UNARY_EXPR: {
            UnaryExpr* un = static_cast<UnaryExpr*>(expr);
            pos += sprintf(buffer + pos, "%s", un->op.c_str());
            pos = generate_expression(buffer, pos, un->operand, program);
            break;
        }
        
        case AstNodeType::ASSIGN_EXPR: {
            AssignExpr* assign = static_cast<AssignExpr*>(expr);
            pos = generate_expression(buffer, pos, assign->target, program);
            pos += sprintf(buffer + pos, " = ");
            pos = generate_expression(buffer, pos, assign->value, program);
            break;
        }
        
        case AstNodeType::COMPOUND_ASSIGN_EXPR: {
            CompoundAssignExpr* compound = static_cast<CompoundAssignExpr*>(expr);
            pos = generate_expression(buffer, pos, compound->target, program);
            pos += sprintf(buffer + pos, " %s= ", compound->op.c_str());
            pos = generate_expression(buffer, pos, compound->value, program);
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
                    if (signal_type.length() > 2 && signal_type[signal_type.length() - 2] == '[' && signal_type[signal_type.length() - 1] == ']') {
                        is_pointer_type = true;
                    } else {
                        for (size_t i = 0; i < program->classes.size(); i++) {
                            if (program->classes[i]->name == signal_type) {
                                is_pointer_type = true;
                                break;
                            }
                        }
                    }
                    
                    if (member->object->type == AstNodeType::INDEX_EXPR) {
                        IndexExpr* idx_expr = static_cast<IndexExpr*>(member->object);
                        IdentifierExpr* obj = static_cast<IdentifierExpr*>(idx_expr->array);
                        pos += sprintf(buffer + pos, "tick_signal_emit(&%s[", obj->name.c_str());
                        pos = generate_expression(buffer, pos, idx_expr->index, program);
                        if (is_pointer_type) {
                            pos += sprintf(buffer + pos, "], (void*)(");
                        } else {
                            pos += sprintf(buffer + pos, "], (void*)(intptr_t)(");
                        }
                    } else {
                        IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                        if (is_pointer_type) {
                            pos += sprintf(buffer + pos, "tick_signal_emit(&%s, (void*)(", obj->name.c_str());
                        } else {
                            pos += sprintf(buffer + pos, "tick_signal_emit(&%s, (void*)(intptr_t)(", obj->name.c_str());
                        }
                    }
                    if (call->arguments.size() > 0) {
                        pos = generate_expression(buffer, pos, call->arguments[0], program);
                    }
                    pos += sprintf(buffer + pos, "))");
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
                    if (signal_type.length() > 2 && signal_type[signal_type.length() - 2] == '[' && signal_type[signal_type.length() - 1] == ']') {
                        is_pointer_type = true;
                    } else {
                        for (size_t i = 0; i < program->classes.size(); i++) {
                            if (program->classes[i]->name == signal_type) {
                                is_pointer_type = true;
                                break;
                            }
                        }
                    }
                    
                    if (!is_pointer_type) {
                        pos += sprintf(buffer + pos, "(intptr_t)");
                    }
                    
                    if (member->object->type == AstNodeType::INDEX_EXPR) {
                        IndexExpr* idx_expr = static_cast<IndexExpr*>(member->object);
                        IdentifierExpr* obj = static_cast<IdentifierExpr*>(idx_expr->array);
                        pos += sprintf(buffer + pos, "tick_signal_recv(&%s[", obj->name.c_str());
                        pos = generate_expression(buffer, pos, idx_expr->index, program);
                        pos += sprintf(buffer + pos, "])");
                    } else {
                        IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                        pos += sprintf(buffer + pos, "tick_signal_recv(&%s)", obj->name.c_str());
                    }
                } else if (member->member == "execute") {
                    IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                    pos += sprintf(buffer + pos, "tick_event_execute(&%s)", obj->name.c_str());
                } else {
                    if (member->object->type == AstNodeType::IDENTIFIER_EXPR) {
                        IdentifierExpr* obj_ident = static_cast<IdentifierExpr*>(member->object);
                        
                        ClassDecl* obj_class = nullptr;
                        if (program) {
                            for (size_t i = 0; i < program->classes.size(); i++) {
                                if (program->classes[i]->name == obj_ident->name) {
                                    obj_class = program->classes[i];
                                    break;
                                }
                            }
                        }
                        
                        if (obj_class) {
                            pos += sprintf(buffer + pos, "%s_%s(", obj_class->name.c_str(), member->member.c_str());
                            pos = generate_expression(buffer, pos, member->object, program);
                            for (size_t i = 0; i < call->arguments.size(); i++) {
                                pos += sprintf(buffer + pos, ", ");
                                pos = generate_expression(buffer, pos, call->arguments[i], program);
                            }
                            pos += sprintf(buffer + pos, ")");
                        } else {
                            pos = generate_expression(buffer, pos, call->callee, program);
                            pos += sprintf(buffer + pos, "(");
                            for (size_t i = 0; i < call->arguments.size(); i++) {
                                if (i > 0) pos += sprintf(buffer + pos, ", ");
                                pos = generate_expression(buffer, pos, call->arguments[i], program);
                            }
                            pos += sprintf(buffer + pos, ")");
                        }
                    } else {
                        pos = generate_expression(buffer, pos, call->callee, program);
                        pos += sprintf(buffer + pos, "(");
                        for (size_t i = 0; i < call->arguments.size(); i++) {
                            if (i > 0) pos += sprintf(buffer + pos, ", ");
                            pos = generate_expression(buffer, pos, call->arguments[i], program);
                        }
                        pos += sprintf(buffer + pos, ")");
                    }
                }
            } else if (call->callee->type == AstNodeType::IDENTIFIER_EXPR) {
                IdentifierExpr* ident = static_cast<IdentifierExpr*>(call->callee);
                
                ClassDecl* matching_class = nullptr;
                if (program) {
                    for (size_t i = 0; i < program->classes.size(); i++) {
                        if (program->classes[i]->name == ident->name) {
                            matching_class = program->classes[i];
                            break;
                        }
                    }
                }
                
                if (matching_class) {
                    pos += sprintf(buffer + pos, "({%s* __obj = malloc(sizeof(%s)); ", 
                        matching_class->name.c_str(), matching_class->name.c_str());
                    
                    for (size_t i = 0; i < matching_class->fields.size(); i++) {
                        VarDecl* field = matching_class->fields[i];
                        pos += sprintf(buffer + pos, "__obj->%s = 0; ", field->name.c_str());
                    }
                    
                    FunctionDecl* constructor = nullptr;
                    for (size_t i = 0; i < matching_class->methods.size(); i++) {
                        if (matching_class->methods[i]->name == matching_class->name) {
                            constructor = matching_class->methods[i];
                            break;
                        }
                    }
                    
                    if (constructor) {
                        pos += sprintf(buffer + pos, "%s_%s(__obj", 
                            matching_class->name.c_str(), constructor->name.c_str());
                        for (size_t i = 0; i < call->arguments.size(); i++) {
                            pos += sprintf(buffer + pos, ", ");
                            pos = generate_expression(buffer, pos, call->arguments[i], program);
                        }
                        pos += sprintf(buffer + pos, "); ");
                    }
                    
                    pos += sprintf(buffer + pos, "__obj; })");
                } else if (ident->name == "print" || ident->name == "println") {
                    pos += sprintf(buffer + pos, "printf(");
                    if (call->arguments.size() > 0) {
                        if (call->arguments[0]->type == AstNodeType::STRING_LITERAL) {
                            pos = generate_expression(buffer, pos, call->arguments[0], program);
                        } else {
                            pos += sprintf(buffer + pos, "\"%%d\", ");
                            pos = generate_expression(buffer, pos, call->arguments[0], program);
                        }
                    }
                    if (ident->name == "println") {
                        if (call->arguments.size() == 0) {
                            pos += sprintf(buffer + pos, "\"\\n\"");
                        } else {
                            int i = pos - 1;
                            while (i > 0 && buffer[i] != '"') i--;
                            if (buffer[i] == '"') {
                                memmove(buffer + i + 2, buffer + i, pos - i);
                                buffer[i] = '\\';
                                buffer[i + 1] = 'n';
                                pos += 2;
                            }
                        }
                    }
                    pos += sprintf(buffer + pos, ")");
                } else {
                    pos = generate_expression(buffer, pos, call->callee, program);
                    pos += sprintf(buffer + pos, "(");
                    for (size_t i = 0; i < call->arguments.size(); i++) {
                        if (i > 0) pos += sprintf(buffer + pos, ", ");
                        pos = generate_expression(buffer, pos, call->arguments[i], program);
                    }
                    pos += sprintf(buffer + pos, ")");
                }
            } else {
                pos = generate_expression(buffer, pos, call->callee, program);
                pos += sprintf(buffer + pos, "(");
                for (size_t i = 0; i < call->arguments.size(); i++) {
                    if (i > 0) pos += sprintf(buffer + pos, ", ");
                    pos = generate_expression(buffer, pos, call->arguments[i], program);
                }
                pos += sprintf(buffer + pos, ")");
            }
            break;
        }
        
        case AstNodeType::INDEX_EXPR: {
            IndexExpr* idx = static_cast<IndexExpr*>(expr);
            pos = generate_expression(buffer, pos, idx->array, program);
            pos += sprintf(buffer + pos, "[");
            pos = generate_expression(buffer, pos, idx->index, program);
            pos += sprintf(buffer + pos, "]");
            break;
        }
        
        case AstNodeType::ARRAY_EXPR: {
            ArrayExpr* arr = static_cast<ArrayExpr*>(expr);
            pos += sprintf(buffer + pos, "{");
            for (size_t i = 0; i < arr->elements.size(); i++) {
                if (i > 0) pos += sprintf(buffer + pos, ", ");
                pos = generate_expression(buffer, pos, arr->elements[i], program);
            }
            pos += sprintf(buffer + pos, "}");
            break;
        }
        
        case AstNodeType::MEMBER_EXPR: {
            MemberExpr* member = static_cast<MemberExpr*>(expr);
            pos = generate_expression(buffer, pos, member->object, program);
            pos += sprintf(buffer + pos, "->%s", member->member.c_str());
            break;
        }
        
        default:
            pos += sprintf(buffer + pos, "0");
            break;
    }
    
    return pos;
}

void Compiler::write_to_file(const char* filename, const char* content) {
    FILE* f = fopen(filename, "w");
    if (f) {
        fprintf(f, "%s", content);
        fclose(f);
    }
}

bool Compiler::invoke_gcc(const char* c_file, const char* output_file) {
    char cmd[2048];
    char exe_path[1024];
    char runtime_path[1024];
    
#ifdef __APPLE__
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) == 0) {
        char* last_slash = strrchr(exe_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            snprintf(runtime_path, sizeof(runtime_path), "%s/../src/runtime/tick_runtime.c", exe_path);
        } else {
            strcpy(runtime_path, "src/runtime/tick_runtime.c");
        }
    } else {
        strcpy(runtime_path, "src/runtime/tick_runtime.c");
    }
#else
    char link_path[64];
    snprintf(link_path, sizeof(link_path), "/proc/%d/exe", getpid());
    ssize_t len = readlink(link_path, exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        char* last_slash = strrchr(exe_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            snprintf(runtime_path, sizeof(runtime_path), "%s/../src/runtime/tick_runtime.c", exe_path);
        } else {
            strcpy(runtime_path, "src/runtime/tick_runtime.c");
        }
    } else {
        strcpy(runtime_path, "src/runtime/tick_runtime.c");
    }
#endif
    
    char include_path[1024];
    snprintf(include_path, sizeof(include_path), "-I%s/../src", exe_path);
    
    snprintf(cmd, sizeof(cmd), "gcc -O2 -o %s %s %s -pthread %s", 
        output_file, c_file, runtime_path, include_path);
    
    int result = system(cmd);
    return result == 0;
}
