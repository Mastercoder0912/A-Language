#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "runtime.h"

typedef struct Interpreter { 
    Program* ast; 
    Environment* env; 
    char* output;
} Interpreter;

Value eval_literal(ASTNode* node);
Value eval_identifier(ASTNode* node, Environment* env);
Value eval_binary_op(ASTNode* node, Environment* env);
Value eval_unary_op(ASTNode* node, Environment* env);
Value eval_cast(ASTNode* node, Environment* env);
Value eval_list_literal(ASTNode* node, Environment* env);
Value eval_dict_literal(ASTNode* node, Environment* env);
Value eval_function_call(ASTNode* node, Environment* env);
Value eval_member_access(ASTNode* node, Environment* env);
Value eval_index_access(ASTNode* node, Environment* env);

void eval_variable_declaration(ASTNode* node, Environment* env);
void eval_block(ASTNode* node, Environment* env);
void eval_if_statement(ASTNode* node, Environment* env);
void eval_loop_statement(ASTNode* node, Environment* env);
void eval_pass_statement(ASTNode* node);
void eval_return_statement(ASTNode* node, Environment* env);
void eval_assignment(ASTNode* node, Environment* env);
void register_builtins(Environment* env);
Value eval_struct_instantiation(char* struct_name, Value* args, int arg_count, Environment* env);
Value eval_class_instantiation(char* class_name, Value* args, int arg_count, Environment* env);
Value eval_expression(ASTNode* node, Environment* env);
void eval_statement(ASTNode* node, Environment* env);
void eval_program(ASTNode* node, Environment* env);
Value eval_f_string(ASTNode* node, Environment* env);

#endif
