#ifndef RUNTIME_H
#define RUNTIME_H

#include "ast.h"
#include <stdbool.h>

typedef enum {
    VALUE_NULL,
    VALUE_INT,
    VALUE_STRING,
    VALUE_BOOL,
    VALUE_LIST,
    VALUE_DICT,
    VALUE_STRUCT,
    VALUE_CLASS_INSTANCE,
    VALUE_FUNCTION,
    VALUE_BUILTIN
} ValueType;

typedef struct Value Value;

typedef struct {
    Value* elements;
    int count;
    int capacity;
} ListValue;

typedef struct {
    char** keys;
    Value* values;
    int count;
    int capacity;
} DictValue;

typedef struct {
    char* name;
    char** field_names;     
    Value* fields;
    int* field_private;
    int field_count;
} StructValue;

typedef struct {
    char* class_name;
    char** field_names;
    Value* fields;
    int* field_private;
    int field_count;
    AST_CLASS_DECLARATION* class_decl;
} ClassInstanceValue;

typedef struct {
    ASTNode* body;
    struct Environment* closure;
} FunctionValue;

typedef Value (*BuiltinFunction)(Value* args, int arg_count);

typedef struct Value {
    ValueType type;
    union {
        int int_val;
        char* string_val;
        int bool_val;
        ListValue list_val;
        DictValue dict_val;
        StructValue* struct_val;
        ClassInstanceValue* class_instance_val;
        FunctionValue function_val;
        BuiltinFunction builtin_fn;
    } data;
} Value;

typedef struct Symbol {
    char* name;
    Value value;
} Symbol;

typedef struct SymbolTable {
    Symbol* symbols;
    int count;
    int capacity;
}  SymbolTable;

typedef struct Environment {
    SymbolTable* scopes;
    int scope_count;
    int scope_capacity;
} Environment;

Value value_make_null();
Value value_make_int(int i);
Value value_make_string(char* s);
Value value_make_bool(bool b);
Value value_make_list();
Value value_make_dict();

bool value_is_truthy(Value v);
char* value_to_string(Value v);
void value_free(Value v);
void value_print(Value v);

Environment* env_create();
void env_push_scope(Environment* env);
void env_pop_scope(Environment* env);
void env_define(Environment* env, char* name, Value val);
Value env_get(Environment* env, char* name);
Value* env_get_ref(Environment* env, char* name);
void env_set(Environment* env, char* name, Value val);
bool env_exists(Environment* env, char* name);
void env_free(Environment* env);

Value list_create();
void list_append(Value* list, Value value);
void list_fill(Value* list, Value value);
Value list_get(Value list, int index);
void list_set(Value* list, int index, Value value);

Value dict_create();
void dict_set(Value* dict, char* key, Value value);
Value dict_get(Value dict, char* key);
Value* dict_get_ref(Value* dict, char* key, bool create_if_missing);

#endif
