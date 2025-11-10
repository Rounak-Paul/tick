#ifndef TICK_AST_H
#define TICK_AST_H

#include "../core/string.h"
#include "../core/dynamic_array.h"

namespace Tick {

enum class AstNodeType {
    PROGRAM,
    EVENT_DECL,
    SIGNAL_DECL,
    PROCESS_DECL,
    FUNCTION_DECL,
    CLASS_DECL,
    VAR_DECL,
    
    BLOCK_STMT,
    EXPR_STMT,
    IF_STMT,
    WHILE_STMT,
    FOR_STMT,
    RETURN_STMT,
    BREAK_STMT,
    
    BINARY_EXPR,
    UNARY_EXPR,
    CALL_EXPR,
    MEMBER_EXPR,
    INDEX_EXPR,
    ARRAY_EXPR,
    NEW_EXPR,
    THIS_EXPR,
    IDENTIFIER_EXPR,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    DOUBLE_LITERAL,
    BOOL_LITERAL,
    STRING_LITERAL
};

struct AstNode {
    AstNodeType type;
    
    AstNode(AstNodeType t) : type(t) {}
    virtual ~AstNode() {}
};

struct ExprNode : public AstNode {
    ExprNode(AstNodeType t) : AstNode(t) {}
};

struct StmtNode : public AstNode {
    StmtNode(AstNodeType t) : AstNode(t) {}
};

struct IntegerLiteral : public ExprNode {
    int value;
    
    IntegerLiteral(int val) 
        : ExprNode(AstNodeType::INTEGER_LITERAL), value(val) {}
};

struct FloatLiteral : public ExprNode {
    float value;
    
    FloatLiteral(float val) 
        : ExprNode(AstNodeType::FLOAT_LITERAL), value(val) {}
};

struct DoubleLiteral : public ExprNode {
    double value;
    
    DoubleLiteral(double val) 
        : ExprNode(AstNodeType::DOUBLE_LITERAL), value(val) {}
};

struct BoolLiteral : public ExprNode {
    bool value;
    
    BoolLiteral(bool val) 
        : ExprNode(AstNodeType::BOOL_LITERAL), value(val) {}
};

struct StringLiteral : public ExprNode {
    String value;
    
    StringLiteral(const String& val) 
        : ExprNode(AstNodeType::STRING_LITERAL), value(val) {}
};

struct IdentifierExpr : public ExprNode {
    String name;
    
    IdentifierExpr(const String& n) 
        : ExprNode(AstNodeType::IDENTIFIER_EXPR), name(n) {}
};

struct BinaryExpr : public ExprNode {
    ExprNode* left;
    String op;
    ExprNode* right;
    
    BinaryExpr(ExprNode* l, const String& o, ExprNode* r)
        : ExprNode(AstNodeType::BINARY_EXPR), left(l), op(o), right(r) {}
    
    ~BinaryExpr() {
        delete left;
        delete right;
    }
};

struct UnaryExpr : public ExprNode {
    String op;
    ExprNode* operand;
    
    UnaryExpr(const String& o, ExprNode* opnd)
        : ExprNode(AstNodeType::UNARY_EXPR), op(o), operand(opnd) {}
    
    ~UnaryExpr() {
        delete operand;
    }
};

struct CallExpr : public ExprNode {
    ExprNode* callee;
    DynamicArray<ExprNode*> arguments;
    
    CallExpr(ExprNode* c) 
        : ExprNode(AstNodeType::CALL_EXPR), callee(c) {}
    
    ~CallExpr() {
        delete callee;
        for (size_t i = 0; i < arguments.size(); i++) {
            delete arguments[i];
        }
    }
};

struct MemberExpr : public ExprNode {
    ExprNode* object;
    String member;
    
    MemberExpr(ExprNode* obj, const String& mem)
        : ExprNode(AstNodeType::MEMBER_EXPR), object(obj), member(mem) {}
    
    ~MemberExpr() {
        delete object;
    }
};

struct IndexExpr : public ExprNode {
    ExprNode* array;
    ExprNode* index;
    
    IndexExpr(ExprNode* arr, ExprNode* idx)
        : ExprNode(AstNodeType::INDEX_EXPR), array(arr), index(idx) {}
    
    ~IndexExpr() {
        delete array;
        delete index;
    }
};

struct ArrayExpr : public ExprNode {
    DynamicArray<ExprNode*> elements;
    
    ArrayExpr() : ExprNode(AstNodeType::ARRAY_EXPR) {}
    
    ~ArrayExpr() {
        for (size_t i = 0; i < elements.size(); i++) {
            delete elements[i];
        }
    }
};

struct NewExpr : public ExprNode {
    String class_name;
    DynamicArray<ExprNode*> arguments;
    
    NewExpr(const String& name) 
        : ExprNode(AstNodeType::NEW_EXPR), class_name(name) {}
    
    ~NewExpr() {
        for (size_t i = 0; i < arguments.size(); i++) {
            delete arguments[i];
        }
    }
};

struct ThisExpr : public ExprNode {
    ThisExpr() : ExprNode(AstNodeType::THIS_EXPR) {}
};

struct BlockStmt : public StmtNode {
    DynamicArray<StmtNode*> statements;
    
    BlockStmt() : StmtNode(AstNodeType::BLOCK_STMT) {}
    
    ~BlockStmt() {
        for (size_t i = 0; i < statements.size(); i++) {
            delete statements[i];
        }
    }
};

struct ExprStmt : public StmtNode {
    ExprNode* expression;
    
    ExprStmt(ExprNode* expr) 
        : StmtNode(AstNodeType::EXPR_STMT), expression(expr) {}
    
    ~ExprStmt() {
        delete expression;
    }
};

struct VarDecl : public StmtNode {
    String type_name;
    String name;
    ExprNode* initializer;
    
    VarDecl(const String& type, const String& n, ExprNode* init)
        : StmtNode(AstNodeType::VAR_DECL), type_name(type), name(n), initializer(init) {}
    
    ~VarDecl() {
        if (initializer) delete initializer;
    }
};

struct IfStmt : public StmtNode {
    ExprNode* condition;
    StmtNode* then_branch;
    StmtNode* else_branch;
    
    IfStmt(ExprNode* cond, StmtNode* then_br, StmtNode* else_br)
        : StmtNode(AstNodeType::IF_STMT), condition(cond), then_branch(then_br), else_branch(else_br) {}
    
    ~IfStmt() {
        delete condition;
        delete then_branch;
        if (else_branch) delete else_branch;
    }
};

struct WhileStmt : public StmtNode {
    ExprNode* condition;
    StmtNode* body;
    
    WhileStmt(ExprNode* cond, StmtNode* b)
        : StmtNode(AstNodeType::WHILE_STMT), condition(cond), body(b) {}
    
    ~WhileStmt() {
        delete condition;
        delete body;
    }
};

struct ReturnStmt : public StmtNode {
    ExprNode* value;
    
    ReturnStmt(ExprNode* val)
        : StmtNode(AstNodeType::RETURN_STMT), value(val) {}
    
    ~ReturnStmt() {
        if (value) delete value;
    }
};

struct BreakStmt : public StmtNode {
    BreakStmt() : StmtNode(AstNodeType::BREAK_STMT) {}
};

struct EventDecl : public AstNode {
    String name;
    
    EventDecl(const String& n)
        : AstNode(AstNodeType::EVENT_DECL), name(n) {}
};

struct SignalDecl : public AstNode {
    String type_param;
    String name;
    
    SignalDecl(const String& type, const String& n)
        : AstNode(AstNodeType::SIGNAL_DECL), type_param(type), name(n) {}
};

struct ProcessDecl : public AstNode {
    String event_name;
    String name;
    BlockStmt* body;
    
    ProcessDecl(const String& evt, const String& n, BlockStmt* b)
        : AstNode(AstNodeType::PROCESS_DECL), event_name(evt), name(n), body(b) {}
    
    ~ProcessDecl() {
        delete body;
    }
};

struct Parameter {
    String type_name;
    String name;
    
    Parameter(const String& type, const String& n)
        : type_name(type), name(n) {}
};

struct FunctionDecl : public AstNode {
    String return_type;
    String name;
    DynamicArray<Parameter*> parameters;
    BlockStmt* body;
    
    FunctionDecl(const String& ret_type, const String& n, BlockStmt* b)
        : AstNode(AstNodeType::FUNCTION_DECL), return_type(ret_type), name(n), body(b) {}
    
    ~FunctionDecl() {
        for (size_t i = 0; i < parameters.size(); i++) {
            delete parameters[i];
        }
        delete body;
    }
};

struct ClassDecl : public AstNode {
    String name;
    DynamicArray<VarDecl*> fields;
    DynamicArray<FunctionDecl*> methods;
    
    ClassDecl(const String& n)
        : AstNode(AstNodeType::CLASS_DECL), name(n) {}
    
    ~ClassDecl() {
        for (size_t i = 0; i < fields.size(); i++) {
            delete fields[i];
        }
        for (size_t i = 0; i < methods.size(); i++) {
            delete methods[i];
        }
    }
};

struct Program : public AstNode {
    DynamicArray<EventDecl*> events;
    DynamicArray<SignalDecl*> signals;
    DynamicArray<ProcessDecl*> processes;
    DynamicArray<FunctionDecl*> functions;
    DynamicArray<ClassDecl*> classes;
    
    Program() : AstNode(AstNodeType::PROGRAM) {}
    
    ~Program() {
        for (size_t i = 0; i < events.size(); i++) delete events[i];
        for (size_t i = 0; i < signals.size(); i++) delete signals[i];
        for (size_t i = 0; i < processes.size(); i++) delete processes[i];
        for (size_t i = 0; i < functions.size(); i++) delete functions[i];
        for (size_t i = 0; i < classes.size(); i++) delete classes[i];
    }
};

}

#endif
