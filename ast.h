#ifndef AST_H
#define AST_H

typedef struct ASTNode ASTNode;

typedef struct {
    int type;
    char* value;
} AST_LITERAL;

typedef struct {
    char* value;
} AST_IDENTIFIER;

typedef struct {
    ASTNode* left_operand;
    int op;
    ASTNode* right_operand;
} AST_BINARY_OPERATION;

typedef struct {
    ASTNode* operand;
    int op;
} AST_UNARY_OPERATION;

typedef struct {
    ASTNode* name;
    ASTNode** arguments; 
    int argument_count;
} AST_FUNCTION_CALL_DEFINITION;

typedef struct {
    ASTNode* object;
    char* member;
} AST_MEMBER_ACCESS;

typedef struct {
    ASTNode* array;
    ASTNode* index;
} AST_INDEX_ACCESS;

typedef struct {
    int target_type;
    ASTNode* expression;
} AST_CAST;

typedef struct {
    ASTNode* target;
    int op;
    ASTNode* value;
} AST_ASSIGNMENT;

typedef struct {
    char* module_name;
    char* alias;
} AST_IMPORT_STATEMENT;

typedef struct {
    char* name;
    int type;
    char* type_name;
    ASTNode* init_value;
} AST_VARIABLE_DECLARATION;

typedef struct {
    ASTNode** statements;
    int statement_count;
} AST_BLOCK;

typedef struct {
    ASTNode* condition;
    AST_BLOCK* then_block;
    ASTNode** else_if_conditions;
    AST_BLOCK** else_if_blocks;
    int else_if_count;
    AST_BLOCK* else_block;
} AST_IF_STATEMENT;

typedef struct {
    char* loop_type;
    ASTNode** parameters;
    int parameter_count;
    AST_BLOCK* body;
} AST_LOOP_STATEMENT;

typedef struct {
    AST_FUNCTION_CALL_DEFINITION* function_call;
} AST_FUNCTION_CALL_STATEMENT;

typedef struct {
    int is_expression;
    union {
        char* string_part;
        ASTNode* expression;
    } content;
} F_STRING_PART;

typedef struct {
    F_STRING_PART** parts;
    int part_count;
} AST_F_STRING;

typedef struct {
    char* value;
} AST_PASS_STATEMENT;

typedef struct {
    ASTNode* value;
} AST_RETURN_STATEMENT;

typedef struct {
    char* name;
    ASTNode** parameters;
    int parameter_count;
    AST_BLOCK* body;
} AST_FUNCTION_DECLARATION;

typedef struct {
    ASTNode** elements;
    int element_count;
} AST_LIST_LITERAL;

typedef struct {
    ASTNode** keys;
    ASTNode** values;
    int entry_count;
} AST_DICT_LITERAL;

typedef struct {
    char* name;
    char** field_names;
    int* field_types;
    char** field_type_names;
    int field_count;
} AST_STRUCT_DECLARATION;

typedef struct {
    char* name;
    ASTNode** private_variables;
    int private_variable_count;
    ASTNode** public_variables;
    int public_variable_count;
    ASTNode** private_methods;
    int private_method_count;
    ASTNode** public_methods;
    int public_method_count;
} AST_CLASS_DECLARATION;

typedef struct {
    ASTNode** declarations;
    int declaration_count;
    ASTNode** statements;
    int statement_count;
} AST_PROGRAM;

typedef enum {
    AST_LITERAL_TYPE,
    AST_IDENTIFIER_TYPE,
    AST_BINARY_OPERATION_TYPE,
    AST_UNARY_OPERATION_TYPE,
    AST_FUNCTION_CALL_DEFINITION_TYPE,
    AST_MEMBER_ACCESS_TYPE,
    AST_INDEX_ACCESS_TYPE,
    AST_CAST_TYPE,
    AST_ASSIGNMENT_TYPE,
    AST_IMPORT_STATEMENT_TYPE,
    AST_VARIABLE_DECLARATION_TYPE,
    AST_BLOCK_TYPE,
    AST_IF_STATEMENT_TYPE,
    AST_LOOP_STATEMENT_TYPE,
    AST_FUNCTION_CALL_STATEMENT_TYPE,
    AST_PASS_STATEMENT_TYPE,
    AST_RETURN_STATEMENT_TYPE,
    AST_FUNCTION_DECLARATION_TYPE,
    AST_STRUCT_DECLARATION_TYPE,
    AST_CLASS_DECLARATION_TYPE,
    AST_PROGRAM_TYPE,
    AST_F_STRING_TYPE,
    AST_LIST_LITERAL_TYPE,
    AST_DICT_LITERAL_TYPE
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        AST_LITERAL literal;
        AST_IDENTIFIER identifier;
        AST_BINARY_OPERATION binary_operation;
        AST_UNARY_OPERATION unary_operation;
        AST_FUNCTION_CALL_DEFINITION function_call_definition;
        AST_MEMBER_ACCESS member_access;
        AST_INDEX_ACCESS index_access;
        AST_CAST cast;
        AST_ASSIGNMENT assignment;
        AST_IMPORT_STATEMENT import_statement;
        AST_VARIABLE_DECLARATION variable_declaration;
        AST_BLOCK block;
        AST_IF_STATEMENT if_statement;
        AST_LOOP_STATEMENT loop_statement;
        AST_FUNCTION_CALL_STATEMENT function_call_statement;
        AST_PASS_STATEMENT pass_statement;
        AST_RETURN_STATEMENT return_statement;
        AST_FUNCTION_DECLARATION function_declaration;
        AST_STRUCT_DECLARATION struct_declaration;
        AST_CLASS_DECLARATION class_declaration;
        AST_PROGRAM program;;
        AST_F_STRING f_string;
        AST_LIST_LITERAL list_literal;
        AST_DICT_LITERAL dict_literal;
    } data;
} ASTNode;

typedef ASTNode Program;

void ast_print(ASTNode* node, int depth);

#endif
