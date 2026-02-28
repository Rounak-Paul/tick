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
    if (_current_class) {
        for (size_t i = 0; i < _current_class->fields.size(); i++) {
            if (_current_class->fields[i]->name == name) {
                return _current_class->fields[i]->type_name;
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
        case AstNodeType::IDENTIFIER_EXPR: {
            IdentifierExpr* id = static_cast<IdentifierExpr*>(expr);
            return lookup_var_type(id->name, program);
        }
        case AstNodeType::MEMBER_EXPR: {
            MemberExpr* me = static_cast<MemberExpr*>(expr);
            String obj_type = infer_expr_type(me->object, program);
            for (size_t i = 0; i < program->classes.size(); i++) {
                if (program->classes[i]->name == obj_type) {
                    for (size_t j = 0; j < program->classes[i]->fields.size(); j++) {
                        if (program->classes[i]->fields[j]->name == me->member) {
                            return program->classes[i]->fields[j]->type_name;
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
                for (size_t i = 0; i < program->functions.size(); i++) {
                    if (program->functions[i]->name == id->name) {
                        return program->functions[i]->return_type;
                    }
                }
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
            return infer_expr_type(bin->left, program);
        }
        case AstNodeType::INDEX_EXPR: {
            IndexExpr* idx = static_cast<IndexExpr*>(expr);
            String arr_type = infer_expr_type(idx->array, program);
            if (arr_type.length() > 2 &&
                arr_type[arr_type.length() - 2] == '[' && arr_type[arr_type.length() - 1] == ']') {
                return String(arr_type.c_str(), arr_type.length() - 2);
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

    if (tick_type.length() > 2 && tick_type[tick_type.length() - 2] == '[' && tick_type[tick_type.length() - 1] == ']') {
        String base_type(tick_type.c_str(), tick_type.length() - 2);
        char base_c[128];
        tick_type_to_c_type(base_type, program, base_c, sizeof(base_c));
        snprintf(out, out_size, "%s*", base_c);
        return;
    }

    if (program) {
        for (size_t i = 0; i < program->classes.size(); i++) {
            if (program->classes[i]->name == tick_type) {
                snprintf(out, out_size, "%s*", tick_type.c_str());
                return;
            }
        }
    }

    snprintf(out, out_size, "int32_t");
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
    CodeBuffer buf;

    buf.append("#include <stdio.h>\n");
    buf.append("#include <stdlib.h>\n");
    buf.append("#include <string.h>\n");
    buf.append("#include <stdbool.h>\n");
    buf.append("#include <stdint.h>\n");
    buf.append("#include \"runtime/tick_runtime.h\"\n\n");

    for (size_t i = 0; i < program->globals.size(); i++) {
        VarDecl* var = program->globals[i];
        if (var->is_const) buf.append("const ");

        bool is_array = var->type_name.length() > 2 &&
                       var->type_name[var->type_name.length() - 2] == '[' &&
                       var->type_name[var->type_name.length() - 1] == ']';

        if (is_array && var->initializer && var->initializer->type == AstNodeType::ARRAY_EXPR) {
            String base_type(var->type_name.c_str(), var->type_name.length() - 2);
            char c_base[128];
            tick_type_to_c_type(base_type, program, c_base, sizeof(c_base));
            buf.append("%s %s[] = ", c_base, var->name.c_str());
            generate_expression(buf, var->initializer, program);
        } else {
            char c_type[128];
            tick_type_to_c_type(var->type_name, program, c_type, sizeof(c_type));
            buf.append("%s %s", c_type, var->name.c_str());
            if (var->initializer) {
                buf.append(" = ");
                generate_expression(buf, var->initializer, program);
            }
        }
        buf.append(";\n");
    }

    if (program->globals.size() > 0) buf.append("\n");

    for (size_t i = 0; i < program->classes.size(); i++) {
        ClassDecl* cls = program->classes[i];
        buf.append("typedef struct %s {\n", cls->name.c_str());
        for (size_t j = 0; j < cls->fields.size(); j++) {
            VarDecl* field = cls->fields[j];
            char field_type[128];
            tick_type_to_c_type(field->type_name, program, field_type, sizeof(field_type));
            buf.append("    %s %s;\n", field_type, field->name.c_str());
        }
        buf.append("} %s;\n\n", cls->name.c_str());
    }

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
        buf.append("void* %s(void* arg);\n", program->processes[i]->name.c_str());
    }

    for (size_t i = 0; i < program->functions.size(); i++) {
        FunctionDecl* func = program->functions[i];
        char ret_type[128];
        tick_type_to_c_type(func->return_type, program, ret_type, sizeof(ret_type));
        buf.append("%s %s(", ret_type, func->name.c_str());
        for (size_t j = 0; j < func->parameters.size(); j++) {
            if (j > 0) buf.append(", ");
            char param_type[128];
            tick_type_to_c_type(func->parameters[j]->type_name, program, param_type, sizeof(param_type));
            buf.append("%s %s", param_type, func->parameters[j]->name.c_str());
        }
        buf.append(");\n");
    }

    for (size_t i = 0; i < program->classes.size(); i++) {
        ClassDecl* cls = program->classes[i];
        for (size_t j = 0; j < cls->methods.size(); j++) {
            FunctionDecl* method = cls->methods[j];
            char ret_type[128];
            tick_type_to_c_type(method->return_type, program, ret_type, sizeof(ret_type));
            buf.append("%s %s_%s(%s* self", ret_type, cls->name.c_str(), method->name.c_str(), cls->name.c_str());
            for (size_t k = 0; k < method->parameters.size(); k++) {
                char param_type[128];
                tick_type_to_c_type(method->parameters[k]->type_name, program, param_type, sizeof(param_type));
                buf.append(", %s %s", param_type, method->parameters[k]->name.c_str());
            }
            buf.append(");\n");
        }
    }

    buf.append("\n");

    for (size_t i = 0; i < program->processes.size(); i++) {
        generate_process(buf, program->processes[i], program);
    }

    for (size_t i = 0; i < program->functions.size(); i++) {
        generate_function(buf, program->functions[i], program);
    }

    for (size_t i = 0; i < program->classes.size(); i++) {
        ClassDecl* cls = program->classes[i];
        for (size_t j = 0; j < cls->methods.size(); j++) {
            FunctionDecl* method = cls->methods[j];
            char ret_type[128];
            tick_type_to_c_type(method->return_type, program, ret_type, sizeof(ret_type));
            buf.append("%s %s_%s(%s* self", ret_type, cls->name.c_str(), method->name.c_str(), cls->name.c_str());
            for (size_t k = 0; k < method->parameters.size(); k++) {
                char param_type[128];
                tick_type_to_c_type(method->parameters[k]->type_name, program, param_type, sizeof(param_type));
                buf.append(", %s %s", param_type, method->parameters[k]->name.c_str());
            }
            buf.append(") {\n");

            _current_func = method;
            _current_class = cls;

            if (method->body) {
                for (size_t k = 0; k < method->body->statements.size(); k++) {
                    generate_statement(buf, method->body->statements[k], 1, program);
                }
            }

            buf.append("}\n\n");
        }
    }

    String result(buf.data, buf.pos);
    return result;
}

void Compiler::generate_process(CodeBuffer& buf, ProcessDecl* proc, Program* program) {
    buf.append("void* %s(void* arg) {\n", proc->name.c_str());

    for (size_t i = 0; i < proc->body->statements.size(); i++) {
        generate_statement(buf, proc->body->statements[i], 1, program);
    }

    buf.append("    return NULL;\n");
    buf.append("}\n\n");
}

void Compiler::generate_function(CodeBuffer& buf, FunctionDecl* func, Program* program) {
    char ret_type[128];
    tick_type_to_c_type(func->return_type, program, ret_type, sizeof(ret_type));

    _current_func = func;
    _current_class = nullptr;

    buf.append("%s %s(", ret_type, func->name.c_str());
    for (size_t j = 0; j < func->parameters.size(); j++) {
        if (j > 0) buf.append(", ");
        char param_type[128];
        tick_type_to_c_type(func->parameters[j]->type_name, program, param_type, sizeof(param_type));
        buf.append("%s %s", param_type, func->parameters[j]->name.c_str());
    }
    buf.append(") {\n");

    if (func->name == "main") {
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
                    buf.append("    tick_event_add_process(&%s, %s);\n", evt->name.c_str(), proc->name.c_str());
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
                String base_type(decl->type_name.c_str(), decl->type_name.length() - 2);
                char c_base[128];
                tick_type_to_c_type(base_type, program, c_base, sizeof(c_base));
                buf.append("%s %s[] = ", c_base, decl->name.c_str());
                generate_expression(buf, decl->initializer, program);
            } else {
                char c_type[128];
                tick_type_to_c_type(decl->type_name, program, c_type, sizeof(c_type));
                buf.append("%s %s", c_type, decl->name.c_str());
                if (decl->initializer) {
                    buf.append(" = ");
                    generate_expression(buf, decl->initializer, program);
                } else {
                    buf.append(" = 0");
                }
            }
            buf.append(";\n");
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

            if (if_stmt->then_branch->type == AstNodeType::BLOCK_STMT) {
                BlockStmt* block = static_cast<BlockStmt*>(if_stmt->then_branch);
                for (size_t i = 0; i < block->statements.size(); i++) {
                    generate_statement(buf, block->statements[i], indent + 1, program);
                }
            } else {
                generate_statement(buf, if_stmt->then_branch, indent + 1, program);
            }

            for (int i = 0; i < indent; i++) buf.append("    ");
            buf.append("}");

            if (if_stmt->else_branch) {
                buf.append(" else {\n");
                if (if_stmt->else_branch->type == AstNodeType::BLOCK_STMT) {
                    BlockStmt* block = static_cast<BlockStmt*>(if_stmt->else_branch);
                    for (size_t i = 0; i < block->statements.size(); i++) {
                        generate_statement(buf, block->statements[i], indent + 1, program);
                    }
                } else {
                    generate_statement(buf, if_stmt->else_branch, indent + 1, program);
                }
                for (int i = 0; i < indent; i++) buf.append("    ");
                buf.append("}");
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

            if (while_stmt->body->type == AstNodeType::BLOCK_STMT) {
                BlockStmt* block = static_cast<BlockStmt*>(while_stmt->body);
                for (size_t i = 0; i < block->statements.size(); i++) {
                    generate_statement(buf, block->statements[i], indent + 1, program);
                }
            } else {
                generate_statement(buf, while_stmt->body, indent + 1, program);
            }

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
                    char c_type[128];
                    tick_type_to_c_type(decl->type_name, program, c_type, sizeof(c_type));
                    buf.append("%s %s", c_type, decl->name.c_str());
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

            if (for_stmt->body->type == AstNodeType::BLOCK_STMT) {
                BlockStmt* block = static_cast<BlockStmt*>(for_stmt->body);
                for (size_t i = 0; i < block->statements.size(); i++) {
                    generate_statement(buf, block->statements[i], indent + 1, program);
                }
            } else {
                generate_statement(buf, for_stmt->body, indent + 1, program);
            }

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
            for (size_t i = 0; i < block->statements.size(); i++) {
                generate_statement(buf, block->statements[i], indent + 1, program);
            }
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
        buf.append("\"%s\"", lit->value.c_str());
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

        case AstNodeType::STRING_LITERAL: {
            StringLiteral* lit = static_cast<StringLiteral*>(expr);
            buf.append("\"%s\"", lit->value.c_str());
            break;
        }

        case AstNodeType::IDENTIFIER_EXPR: {
            IdentifierExpr* ident = static_cast<IdentifierExpr*>(expr);
            buf.append("%s", ident->name.c_str());
            break;
        }

        case AstNodeType::THIS_EXPR: {
            buf.append("self");
            break;
        }

        case AstNodeType::BINARY_EXPR: {
            BinaryExpr* bin = static_cast<BinaryExpr*>(expr);
            buf.append("(");
            generate_expression(buf, bin->left, program);
            buf.append(" %s ", bin->op.c_str());
            generate_expression(buf, bin->right, program);
            buf.append(")");
            break;
        }

        case AstNodeType::UNARY_EXPR: {
            UnaryExpr* un = static_cast<UnaryExpr*>(expr);
            buf.append("%s", un->op.c_str());
            generate_expression(buf, un->operand, program);
            break;
        }

        case AstNodeType::ASSIGN_EXPR: {
            AssignExpr* assign = static_cast<AssignExpr*>(expr);
            generate_expression(buf, assign->target, program);
            buf.append(" = ");
            generate_expression(buf, assign->value, program);
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
                    if (call->arguments.size() > 0) {
                        generate_expression(buf, call->arguments[0], program);
                    }
                    buf.append("))");
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
                } else if (member->member == "execute") {
                    IdentifierExpr* obj = static_cast<IdentifierExpr*>(member->object);
                    buf.append("tick_event_execute(&%s)", obj->name.c_str());
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
                            buf.append("%s_%s(", obj_class->name.c_str(), member->member.c_str());
                            generate_expression(buf, member->object, program);
                            for (size_t i = 0; i < call->arguments.size(); i++) {
                                buf.append(", ");
                                generate_expression(buf, call->arguments[i], program);
                            }
                            buf.append(")");
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

                if (matching_class) {
                    buf.append("({%s* __obj = malloc(sizeof(%s)); ",
                        matching_class->name.c_str(), matching_class->name.c_str());

                    for (size_t i = 0; i < matching_class->fields.size(); i++) {
                        buf.append("__obj->%s = 0; ", matching_class->fields[i]->name.c_str());
                    }

                    FunctionDecl* constructor = nullptr;
                    for (size_t i = 0; i < matching_class->methods.size(); i++) {
                        if (matching_class->methods[i]->name == matching_class->name) {
                            constructor = matching_class->methods[i];
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
            break;
        }

        case AstNodeType::INDEX_EXPR: {
            IndexExpr* idx = static_cast<IndexExpr*>(expr);
            generate_expression(buf, idx->array, program);
            buf.append("[");
            generate_expression(buf, idx->index, program);
            buf.append("]");
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
            generate_expression(buf, member->object, program);
            buf.append("->%s", member->member.c_str());
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
