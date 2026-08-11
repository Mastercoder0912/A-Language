#include "builtins.h"
#include "interpreter.h"
#include "runtime.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <ctype.h>
#include <sqlite3.h>

static char *duplicate_string(const char *text)
{
    if (!text)
    {
        return NULL;
    }
    size_t length = strlen(text);
    char *copy = malloc(length + 1);
    if (!copy)
    {
        return NULL;
    }
    strcpy(copy, text);
    return copy;
}

typedef struct
{
    int is_return;
    Value return_value;
} ReturnFlag;

static ReturnFlag return_flag = {0, {0}};
static Value copy_value(Value original);
Value eval_method_call(ASTNode *node, Environment *env);
Value eval_import_statement(ASTNode *node, Environment *env);

static char *json_escape_string(const char *text)
{
    if (!text)
    {
        return duplicate_string("");
    }

    size_t len = strlen(text);
    char *escaped = malloc(len * 2 + 1);
    size_t out = 0;
    for (size_t i = 0; i < len; i++)
    {
        char c = text[i];
        if (c == '"' || c == '\\')
        {
            escaped[out++] = '\\';
            escaped[out++] = c;
        }
        else
        {
            escaped[out++] = c;
        }
    }
    escaped[out] = '\0';
    return escaped;
}

static char *json_serialize(Value value, int depth)
{
    char *buffer = malloc(4096);
    buffer[0] = '\0';

    switch (value.type)
    {
    case VALUE_NULL:
        strcpy(buffer, "null");
        break;
    case VALUE_INT:
        sprintf(buffer, "%d", value.data.int_val);
        break;
    case VALUE_BOOL:
        strcpy(buffer, value.data.bool_val ? "true" : "false");
        break;
    case VALUE_STRING:
    {
        char *escaped = json_escape_string(value.data.string_val);
        sprintf(buffer, "\"%s\"", escaped);
        free(escaped);
        break;
    }
    case VALUE_LIST:
    {
        strcat(buffer, "[");
        for (int i = 0; i < value.data.list_val.count; i++)
        {
            if (i > 0)
            {
                strcat(buffer, ", ");
            }
            char *item = json_serialize(value.data.list_val.elements[i], depth + 1);
            strcat(buffer, item);
            free(item);
        }
        strcat(buffer, "]");
        break;
    }
    case VALUE_DICT:
    {
        strcat(buffer, "{");
        for (int i = 0; i < value.data.dict_val.count; i++)
        {
            if (i > 0)
            {
                strcat(buffer, ", ");
            }
            char *escaped_key = json_escape_string(value.data.dict_val.keys[i]);
            char *item = json_serialize(value.data.dict_val.values[i], depth + 1);
            sprintf(buffer + strlen(buffer), "\"%s\": %s", escaped_key, item);
            free(escaped_key);
            free(item);
        }
        strcat(buffer, "}");
        break;
    }
    default:
        strcpy(buffer, "null");
        break;
    }

    return buffer;
}

static Value json_parse_from_text(const char *text, int *index)
{
    while (text[*index] != '\0' && isspace((unsigned char)text[*index]))
    {
        (*index)++;
    }

    if (text[*index] == '\0')
    {
        return value_make_null();
    }

    if (text[*index] == '"')
    {
        (*index)++;
        char *buffer = malloc(strlen(text) + 1);
        int out = 0;
        while (text[*index] != '\0' && text[*index] != '"')
        {
            if (text[*index] == '\\')
            {
                (*index)++;
                if (text[*index] == '\0')
                {
                    break;
                }
                buffer[out++] = text[*index];
                (*index)++;
                continue;
            }
            buffer[out++] = text[*index];
            (*index)++;
        }
        if (text[*index] == '"')
        {
            (*index)++;
        }
        buffer[out] = '\0';
        return value_make_string(buffer);
    }

    if (text[*index] == '{')
    {
        (*index)++;
        Value dict = dict_create();
        while (text[*index] != '\0')
        {
            while (isspace((unsigned char)text[*index]))
            {
                (*index)++;
            }
            if (text[*index] == '}')
            {
                (*index)++;
                break;
            }

            if (text[*index] != '"')
            {
                break;
            }
            Value key_val = json_parse_from_text(text, index);
            char *key = value_to_string(key_val);
            while (isspace((unsigned char)text[*index]))
            {
                (*index)++;
            }
            if (text[*index] != ':')
            {
                break;
            }
            (*index)++;
            Value value = json_parse_from_text(text, index);
            dict_set(&dict, key, value);
            free(key);
            while (isspace((unsigned char)text[*index]))
            {
                (*index)++;
            }
            if (text[*index] == ',')
            {
                (*index)++;
                continue;
            }
            if (text[*index] == '}')
            {
                (*index)++;
                break;
            }
            break;
        }
        return dict;
    }

    if (text[*index] == '[')
    {
        (*index)++;
        Value list = list_create();
        while (text[*index] != '\0')
        {
            while (isspace((unsigned char)text[*index]))
            {
                (*index)++;
            }
            if (text[*index] == ']')
            {
                (*index)++;
                break;
            }
            Value item = json_parse_from_text(text, index);
            list_append(&list, item);
            while (isspace((unsigned char)text[*index]))
            {
                (*index)++;
            }
            if (text[*index] == ',')
            {
                (*index)++;
                continue;
            }
            if (text[*index] == ']')
            {
                (*index)++;
                break;
            }
            break;
        }
        return list;
    }

    if (strncmp(text + *index, "true", 4) == 0)
    {
        (*index) += 4;
        return value_make_bool(true);
    }
    if (strncmp(text + *index, "false", 5) == 0)
    {
        (*index) += 5;
        return value_make_bool(false);
    }
    if (strncmp(text + *index, "null", 4) == 0)
    {
        (*index) += 4;
        return value_make_null();
    }

    char *end = NULL;
    long number = strtol(text + *index, &end, 10);
    if (end != text + *index)
    {
        (*index) = (int)(end - text);
        return value_make_int((int)number);
    }

    return value_make_null();
}

static Value json_dumps_value(Value value)
{
    char *serialized = json_serialize(value, 0);
    Value result = value_make_string(serialized);
    free(serialized);
    return result;
}

static Value json_loads_value(Value value)
{
    if (value.type != VALUE_STRING)
    {
        return value_make_null();
    }

    int index = 0;
    Value parsed = json_parse_from_text(value.data.string_val, &index);
    return parsed;
}

static Value json_load_value(Value value)
{
    if (value.type != VALUE_STRING)
    {
        return value_make_null();
    }

    FILE *file = fopen(value.data.string_val, "r");
    if (!file)
    {
        fprintf(stderr, "Error: could not read JSON file '%s'\n", value.data.string_val);
        return value_make_null();
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);

    int index = 0;
    Value parsed = json_parse_from_text(buffer, &index);
    free(buffer);
    return parsed;
}

static Value json_dump_value(Value file_path, Value value)
{
    if (file_path.type != VALUE_STRING)
    {
        return value_make_null();
    }

    char *serialized = json_serialize(value, 0);
    FILE *file = fopen(file_path.data.string_val, "w");
    if (!file)
    {
        fprintf(stderr, "Error: could not write JSON file '%s'\n", file_path.data.string_val);
        free(serialized);
        return value_make_null();
    }

    fputs(serialized, file);
    fclose(file);
    free(serialized);
    return value_make_null();
}

static Value json_custom_encode_value(Value value)
{
    if (value.type == VALUE_STRUCT)
    {
        Value result = dict_create();
        StructValue *struct_val = value.data.struct_val;
        for (int i = 0; i < struct_val->field_count; i++)
        {
            dict_set(&result, struct_val->field_names[i], copy_value(struct_val->fields[i]));
        }
        return result;
    }

    if (value.type == VALUE_CLASS_INSTANCE)
    {
        Value result = dict_create();
        ClassInstanceValue *instance = value.data.class_instance_val;
        for (int i = 0; i < instance->field_count; i++)
        {
            dict_set(&result, instance->field_names[i], copy_value(instance->fields[i]));
        }
        return result;
    }

    return copy_value(value);
}

static Value json_custom_decode_value(Value dict_value, Value type_name_value, Environment *env)
{
    if (dict_value.type != VALUE_DICT || type_name_value.type != VALUE_STRING)
    {
        return value_make_null();
    }

    Value target = env_get(env, type_name_value.data.string_val);
    if (target.type == VALUE_STRUCT)
    {
        StructValue *struct_val = target.data.struct_val;
        StructValue *instance = malloc(sizeof(StructValue));
        instance->name = malloc(strlen(struct_val->name) + 1);
        strcpy(instance->name, struct_val->name);
        instance->field_count = struct_val->field_count;
        instance->fields = malloc(sizeof(Value) * struct_val->field_count);
        instance->field_names = malloc(sizeof(char *) * struct_val->field_count);
        instance->field_private = malloc(sizeof(int) * struct_val->field_count);
        for (int i = 0; i < struct_val->field_count; i++)
        {
            instance->field_names[i] = malloc(strlen(struct_val->field_names[i]) + 1);
            strcpy(instance->field_names[i], struct_val->field_names[i]);
            instance->field_private[i] = 0;
            Value field_value = dict_get(dict_value, struct_val->field_names[i]);
            instance->fields[i] = copy_value(field_value);
        }
        Value result;
        result.type = VALUE_STRUCT;
        result.data.struct_val = instance;
        return result;
    }

    if (target.type == VALUE_CLASS_INSTANCE)
    {
        AST_CLASS_DECLARATION *class_decl = (AST_CLASS_DECLARATION *)target.data.class_instance_val;
        ClassInstanceValue *instance = malloc(sizeof(ClassInstanceValue));
        instance->class_name = malloc(strlen(class_decl->name) + 1);
        strcpy(instance->class_name, class_decl->name);
        instance->field_count = class_decl->private_variable_count + class_decl->public_variable_count;
        instance->fields = malloc(sizeof(Value) * instance->field_count);
        instance->field_names = malloc(sizeof(char *) * instance->field_count);
        instance->field_private = malloc(sizeof(int) * instance->field_count);
        instance->class_decl = class_decl;
        int field_idx = 0;
        for (int i = 0; i < class_decl->private_variable_count; i++)
        {
            AST_VARIABLE_DECLARATION *field = &class_decl->private_variables[i]->data.variable_declaration;
            instance->field_names[field_idx] = malloc(strlen(field->name) + 1);
            strcpy(instance->field_names[field_idx], field->name);
            instance->field_private[field_idx] = 1;
            Value field_value = dict_get(dict_value, field->name);
            instance->fields[field_idx] = copy_value(field_value);
            field_idx++;
        }
        for (int i = 0; i < class_decl->public_variable_count; i++)
        {
            AST_VARIABLE_DECLARATION *field = &class_decl->public_variables[i]->data.variable_declaration;
            instance->field_names[field_idx] = malloc(strlen(field->name) + 1);
            strcpy(instance->field_names[field_idx], field->name);
            instance->field_private[field_idx] = 0;
            Value field_value = dict_get(dict_value, field->name);
            instance->fields[field_idx] = copy_value(field_value);
            field_idx++;
        }
        Value result;
        result.type = VALUE_CLASS_INSTANCE;
        result.data.class_instance_val = instance;
        return result;
    }

    return value_make_null();
}

static int sql_get_named_arg(AST_FUNCTION_CALL_DEFINITION *call, char *name, Environment *env, Value *out)
{
    for (int i = 0; i < call->argument_count; i++)
    {
        ASTNode *arg = call->arguments[i];
        if (arg->type == AST_ASSIGNMENT_TYPE)
        {
            AST_ASSIGNMENT *assignment = &arg->data.assignment;
            if (assignment->target->type == AST_IDENTIFIER_TYPE &&
                strcmp(assignment->target->data.identifier.value, name) == 0)
            {
                *out = eval_expression(assignment->value, env);
                return 1;
            }
        }
    }
    return 0;
}

static int sql_positional_count(AST_FUNCTION_CALL_DEFINITION *call)
{
    int count = 0;
    for (int i = 0; i < call->argument_count; i++)
    {
        if (call->arguments[i]->type != AST_ASSIGNMENT_TYPE)
        {
            count++;
        }
    }
    return count;
}

static Value sql_get_positional(AST_FUNCTION_CALL_DEFINITION *call, int wanted, Environment *env)
{
    int index = 0;
    for (int i = 0; i < call->argument_count; i++)
    {
        if (call->arguments[i]->type != AST_ASSIGNMENT_TYPE)
        {
            if (index == wanted)
            {
                return eval_expression(call->arguments[i], env);
            }
            index++;
        }
    }
    return value_make_null();
}

static int sql_has_named_arg(AST_FUNCTION_CALL_DEFINITION *call)
{
    for (int i = 0; i < call->argument_count; i++)
    {
        if (call->arguments[i]->type == AST_ASSIGNMENT_TYPE)
        {
            return 1;
        }
    }
    return 0;
}

static void sql_bind_value(sqlite3_stmt *stmt, int index, Value value)
{
    switch (value.type)
    {
    case VALUE_NULL:
        sqlite3_bind_null(stmt, index);
        break;
    case VALUE_INT:
        sqlite3_bind_int(stmt, index, value.data.int_val);
        break;
    case VALUE_BYTE:
        sqlite3_bind_int(stmt, index, value.data.byte_val);
        break;
    case VALUE_BOOL:
        sqlite3_bind_int(stmt, index, value.data.bool_val);
        break;
    case VALUE_STRING:
        sqlite3_bind_text(stmt, index, value.data.string_val, -1, SQLITE_TRANSIENT);
        break;
    default:
    {
        char *text = value_to_string(value);
        sqlite3_bind_text(stmt, index, text, -1, SQLITE_TRANSIENT);
        free(text);
        break;
    }
    }
}

static Value sql_column_value(sqlite3_stmt *stmt, int column)
{
    int type = sqlite3_column_type(stmt, column);
    if (type == SQLITE_INTEGER)
    {
        return value_make_int(sqlite3_column_int(stmt, column));
    }
    if (type == SQLITE_TEXT)
    {
        const unsigned char *text = sqlite3_column_text(stmt, column);
        return value_make_string((char *)(text ? text : (const unsigned char *)""));
    }
    if (type == SQLITE_NULL)
    {
        return value_make_null();
    }
    const unsigned char *text = sqlite3_column_text(stmt, column);
    return value_make_string((char *)(text ? text : (const unsigned char *)""));
}

static Value sql_fetch_query(SqlDatabase *db, char *query)
{
    sqlite3_stmt *stmt = NULL;
    Value result = list_create();
    if (sqlite3_prepare_v2(db->handle, query, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "Error: SQL fetch failed: %s\n", sqlite3_errmsg(db->handle));
        return result;
    }
    int column_count = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Value row = dict_create();
        for (int i = 0; i < column_count; i++)
        {
            const char *name = sqlite3_column_name(stmt, i);
            dict_set(&row, (char *)name, sql_column_value(stmt, i));
        }
        list_append(&result, row);
    }
    sqlite3_finalize(stmt);
    return result;
}

static char *sql_table_name_from_call(AST_FUNCTION_CALL_DEFINITION *call, Environment *env)
{
    Value table_arg;
    if (sql_get_named_arg(call, "table", env, &table_arg) && table_arg.type == VALUE_STRING)
    {
        return table_arg.data.string_val;
    }
    if (sql_positional_count(call) > 0)
    {
        Value first = sql_get_positional(call, 0, env);
        if (first.type == VALUE_STRING)
        {
            return first.data.string_val;
        }
    }
    return "default";
}

static char **sql_table_columns(SqlDatabase *db, char *table_name, int *count)
{
    *count = 0;
    char query[256];
    snprintf(query, sizeof(query), "PRAGMA table_info(%s)", table_name);
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, query, -1, &stmt, NULL) != SQLITE_OK)
    {
        return NULL;
    }
    int capacity = 8;
    char **columns = malloc(sizeof(char *) * capacity);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        if (!name)
        {
            continue;
        }
        if (*count >= capacity)
        {
            capacity *= 2;
            columns = realloc(columns, sizeof(char *) * capacity);
        }
        columns[*count] = duplicate_string((const char *)name);
        (*count)++;
    }
    sqlite3_finalize(stmt);
    return columns;
}

static void sql_free_columns(char **columns, int count)
{
    for (int i = 0; i < count; i++)
    {
        free(columns[i]);
    }
    free(columns);
}

static Value sql_entry(SqlDatabase *db, AST_FUNCTION_CALL_DEFINITION *call, Environment *env)
{
    int positions = sql_positional_count(call);
    Value first = sql_get_positional(call, 0, env);
    Value second = sql_get_positional(call, 1, env);
    int dict_mode = positions >= 2 && first.type == VALUE_STRING && second.type == VALUE_DICT && !sql_get_named_arg(call, "table", env, &first);
    char *table_name = dict_mode ? first.data.string_val : sql_table_name_from_call(call, env);
    Value row = dict_mode ? second : value_make_null();
    int value_start = dict_mode ? 2 : 0;
    int column_count = 0;
    char **columns = NULL;

    if (!dict_mode)
    {
        columns = sql_table_columns(db, table_name, &column_count);
    }

    int insert_count = dict_mode ? row.data.dict_val.count : positions - value_start;
    if (insert_count <= 0)
    {
        sql_free_columns(columns, column_count);
        return value_make_null();
    }

    char query[2048];
    snprintf(query, sizeof(query), "INSERT INTO %s (", table_name);
    for (int i = 0; i < insert_count; i++)
    {
        if (i > 0)
        {
            strcat(query, ", ");
        }
        if (dict_mode)
        {
            strcat(query, row.data.dict_val.keys[i]);
        }
        else if (column_count > 0)
        {
            int column_index = i;
            if (column_count > insert_count && strcmp(columns[0], "id") == 0)
            {
                column_index = i + 1;
            }
            strcat(query, columns[column_index]);
        }
        else if (i == 0)
        {
            strcat(query, "name");
        }
        else if (i == 1)
        {
            strcat(query, "age");
        }
        else
        {
            char key[32];
            sprintf(key, "column%d", i + 1);
            strcat(query, key);
        }
    }
    strcat(query, ") VALUES (");
    for (int i = 0; i < insert_count; i++)
    {
        if (i > 0)
        {
            strcat(query, ", ");
        }
        strcat(query, "?");
    }
    strcat(query, ")");

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, query, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "Error: SQL entry failed: %s\n", sqlite3_errmsg(db->handle));
        sql_free_columns(columns, column_count);
        return value_make_null();
    }
    for (int i = 0; i < insert_count; i++)
    {
        Value value = dict_mode ? row.data.dict_val.values[i] : sql_get_positional(call, value_start + i, env);
        sql_bind_value(stmt, i + 1, value);
    }
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stderr, "Error: SQL entry failed: %s\n", sqlite3_errmsg(db->handle));
    }
    sqlite3_finalize(stmt);
    sql_free_columns(columns, column_count);
    return value_make_null();
}

static Value sql_fetch(SqlDatabase *db, AST_FUNCTION_CALL_DEFINITION *call, Environment *env)
{
    if (call->argument_count == 1 && !sql_has_named_arg(call))
    {
        Value query = sql_get_positional(call, 0, env);
        if (query.type == VALUE_STRING)
        {
            return sql_fetch_query(db, query.data.string_val);
        }
    }

    char *table_name = sql_table_name_from_call(call, env);
    char query[2048];
    strcpy(query, "SELECT ");
    Value parameters;
    if (sql_get_named_arg(call, "parameters", env, &parameters) && parameters.type == VALUE_LIST)
    {
        for (int i = 0; i < parameters.data.list_val.count; i++)
        {
            if (i > 0)
            {
                strcat(query, ", ");
            }
            Value column = parameters.data.list_val.elements[i];
            if (column.type == VALUE_STRING)
            {
                strcat(query, column.data.string_val);
            }
        }
    }
    else
    {
        strcat(query, "*");
    }
    strcat(query, " FROM ");
    strcat(query, table_name);
    Value where;
    if (sql_get_named_arg(call, "where", env, &where) && where.type == VALUE_STRING)
    {
        strcat(query, " WHERE ");
        strcat(query, where.data.string_val);
    }
    return sql_fetch_query(db, query);
}

static Value sql_module_method(char *method_name, AST_FUNCTION_CALL_DEFINITION *call, Environment *env)
{
    if (strcmp(method_name, "database") != 0 && strcmp(method_name, "create") != 0)
    {
        fprintf(stderr, "Error: sql has no method '%s'\n", method_name);
        return value_make_null();
    }
    if (call->argument_count != 1)
    {
        return value_make_null();
    }
    Value path = eval_expression(call->arguments[0], env);
    if (path.type != VALUE_STRING)
    {
        return value_make_null();
    }
    SqlDatabase *db = sql_db_create(path.data.string_val);
    if (!db)
    {
        fprintf(stderr, "Error: could not open database '%s'\n", path.data.string_val);
        return value_make_null();
    }
    return value_make_sql_db(db);
}

static Value sql_db_method(SqlDatabase *db, char *method_name, AST_FUNCTION_CALL_DEFINITION *call, Environment *env)
{
    if (!db || db->closed || !db->handle)
    {
        fprintf(stderr, "Error: database is closed\n");
        return value_make_null();
    }

    if (strcmp(method_name, "execute") == 0)
    {
        if (call->argument_count != 1)
        {
            return value_make_null();
        }
        Value statement = eval_expression(call->arguments[0], env);
        if (statement.type == VALUE_STRING)
        {
            char *error = NULL;
            if (sqlite3_exec(db->handle, statement.data.string_val, NULL, NULL, &error) != SQLITE_OK)
            {
                fprintf(stderr, "Error: SQL execute failed: %s\n", error ? error : sqlite3_errmsg(db->handle));
                sqlite3_free(error);
            }
        }
        return value_make_null();
    }

    if (strcmp(method_name, "entry") == 0)
    {
        return sql_entry(db, call, env);
    }

    if (strcmp(method_name, "fetch") == 0)
    {
        return sql_fetch(db, call, env);
    }

    if (strcmp(method_name, "update") == 0)
    {
        char *table_name = sql_table_name_from_call(call, env);
        Value set_values;
        if (!sql_get_named_arg(call, "set", env, &set_values) || set_values.type != VALUE_DICT)
        {
            return value_make_null();
        }
        char query[2048];
        snprintf(query, sizeof(query), "UPDATE %s SET ", table_name);
        for (int i = 0; i < set_values.data.dict_val.count; i++)
        {
            if (i > 0)
            {
                strcat(query, ", ");
            }
            strcat(query, set_values.data.dict_val.keys[i]);
            strcat(query, " = ?");
        }
        Value where;
        if (sql_get_named_arg(call, "where", env, &where) && where.type == VALUE_STRING)
        {
            strcat(query, " WHERE ");
            strcat(query, where.data.string_val);
        }
        sqlite3_stmt *stmt = NULL;
        if (sqlite3_prepare_v2(db->handle, query, -1, &stmt, NULL) != SQLITE_OK)
        {
            fprintf(stderr, "Error: SQL update failed: %s\n", sqlite3_errmsg(db->handle));
            return value_make_null();
        }
        for (int i = 0; i < set_values.data.dict_val.count; i++)
        {
            sql_bind_value(stmt, i + 1, set_values.data.dict_val.values[i]);
        }
        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            fprintf(stderr, "Error: SQL update failed: %s\n", sqlite3_errmsg(db->handle));
        }
        sqlite3_finalize(stmt);
        return value_make_null();
    }

    if (strcmp(method_name, "delete") == 0)
    {
        char *table_name = sql_table_name_from_call(call, env);
        char query[1024];
        snprintf(query, sizeof(query), "DELETE FROM %s", table_name);
        Value where;
        if (sql_get_named_arg(call, "where", env, &where) && where.type == VALUE_STRING)
        {
            strcat(query, " WHERE ");
            strcat(query, where.data.string_val);
        }
        char *error = NULL;
        if (sqlite3_exec(db->handle, query, NULL, NULL, &error) != SQLITE_OK)
        {
            fprintf(stderr, "Error: SQL delete failed: %s\n", error ? error : sqlite3_errmsg(db->handle));
            sqlite3_free(error);
        }
        return value_make_null();
    }

    if (strcmp(method_name, "close") == 0)
    {
        sqlite3_close(db->handle);
        db->handle = NULL;
        db->closed = 1;
        return value_make_null();
    }

    fprintf(stderr, "Error: database has no method '%s'\n", method_name);
    return value_make_null();
}

static int class_field_index(ClassInstanceValue *instance, char *name)
{
    for (int i = 0; i < instance->field_count; i++)
    {
        if (strcmp(instance->field_names[i], name) == 0)
        {
            return i;
        }
    }
    return -1;
}

static Value get_this_field(Environment *env, char *name, int *found)
{
    if (found)
    {
        *found = 0;
    }
    Value this_val = env_get(env, "this");
    if (this_val.type != VALUE_CLASS_INSTANCE)
    {
        return value_make_null();
    }
    ClassInstanceValue *instance = this_val.data.class_instance_val;
    int index = class_field_index(instance, name);
    if (index < 0)
    {
        return value_make_null();
    }
    if (found)
    {
        *found = 1;
    }
    return instance->fields[index];
}

static int set_this_field(Environment *env, char *name, Value value)
{
    Value this_val = env_get(env, "this");
    if (this_val.type != VALUE_CLASS_INSTANCE)
    {
        return 0;
    }
    ClassInstanceValue *instance = this_val.data.class_instance_val;
    int index = class_field_index(instance, name);
    if (index < 0)
    {
        return 0;
    }
    value_free(instance->fields[index]);
    instance->fields[index] = value;
    return 1;
}

static Value *resolve_target_ref(ASTNode *target, Environment *env)
{
    if (!target)
    {
        return NULL;
    }

    if (target->type == AST_IDENTIFIER_TYPE)
    {
        return env_get_ref(env, target->data.identifier.value);
    }

    if (target->type == AST_INDEX_ACCESS_TYPE)
    {
        AST_INDEX_ACCESS *access = &target->data.index_access;
        Value *collection = resolve_target_ref(access->array, env);
        if (!collection)
        {
            return NULL;
        }

        Value index = eval_expression(access->index, env);
        if (collection->type == VALUE_LIST && index.type == VALUE_INT)
        {
            if (index.data.int_val < 0 || index.data.int_val >= collection->data.list_val.count)
            {
                return NULL;
            }
            return &collection->data.list_val.elements[index.data.int_val];
        }
        if (collection->type == VALUE_DICT)
        {
            char *key = value_to_string(index);
            Value *value_ref = dict_get_ref(collection, key, true);
            free(key);
            return value_ref;
        }
        return NULL;
    }

    return NULL;
}

static Value copy_value(Value original)
{
    switch (original.type)
    {
    case VALUE_NULL:
        return value_make_null();
    case VALUE_INT:
        return value_make_int(original.data.int_val);
    case VALUE_BYTE:
        return value_make_byte(original.data.byte_val);
    case VALUE_STRING:
        return value_make_string(original.data.string_val);
    case VALUE_BOOL:
        return value_make_bool(original.data.bool_val);
    case VALUE_LIST:
    {
        Value result = list_create();
        for (int i = 0; i < original.data.list_val.count; i++)
        {
            list_append(&result, copy_value(original.data.list_val.elements[i]));
        }
        return result;
    }
    case VALUE_DICT:
    {
        Value result = dict_create();
        for (int i = 0; i < original.data.dict_val.count; i++)
        {
            Value copied = copy_value(original.data.dict_val.values[i]);
            dict_set(&result, original.data.dict_val.keys[i], copied);
        }
        return result;
    }
    case VALUE_STRUCT:
    {
        StructValue *source = original.data.struct_val;
        StructValue *copy = malloc(sizeof(StructValue));
        copy->name = malloc(strlen(source->name) + 1);
        strcpy(copy->name, source->name);
        copy->field_count = source->field_count;
        copy->fields = malloc(sizeof(Value) * copy->field_count);
        copy->field_names = malloc(sizeof(char *) * copy->field_count);
        copy->field_private = malloc(sizeof(int) * copy->field_count);
        for (int i = 0; i < copy->field_count; i++)
        {
            copy->field_private[i] = source->field_private[i];
            copy->field_names[i] = malloc(strlen(source->field_names[i]) + 1);
            strcpy(copy->field_names[i], source->field_names[i]);
            copy->fields[i] = copy_value(source->fields[i]);
        }
        Value result;
        result.type = VALUE_STRUCT;
        result.data.struct_val = copy;
        return result;
    }
    case VALUE_CLASS_INSTANCE:
    {
        ClassInstanceValue *source = original.data.class_instance_val;
        ClassInstanceValue *copy = malloc(sizeof(ClassInstanceValue));
        copy->class_name = malloc(strlen(source->class_name) + 1);
        strcpy(copy->class_name, source->class_name);
        copy->field_count = source->field_count;
        copy->fields = malloc(sizeof(Value) * copy->field_count);
        copy->field_names = malloc(sizeof(char *) * copy->field_count);
        copy->field_private = malloc(sizeof(int) * copy->field_count);
        copy->class_decl = source->class_decl;
        for (int i = 0; i < copy->field_count; i++)
        {
            copy->field_private[i] = source->field_private[i];
            copy->field_names[i] = malloc(strlen(source->field_names[i]) + 1);
            strcpy(copy->field_names[i], source->field_names[i]);
            copy->fields[i] = copy_value(source->fields[i]);
        }
        Value result;
        result.type = VALUE_CLASS_INSTANCE;
        result.data.class_instance_val = copy;
        return result;
    }
    case VALUE_FUNCTION:
    {
        Value result;
        result.type = VALUE_FUNCTION;
        result.data.function_val.body = original.data.function_val.body;
        result.data.function_val.closure = original.data.function_val.closure;
        return result;
    }
    case VALUE_BUILTIN:
    {
        Value result;
        result.type = VALUE_BUILTIN;
        result.data.builtin_fn = original.data.builtin_fn;
        return result;
    }
    case VALUE_JSON_MODULE:
        return value_make_json_module();
    case VALUE_SQL_MODULE:
        return value_make_sql_module();
    case VALUE_SQL_DB:
        return value_make_sql_db(original.data.sql_db_val);
    default:
        return value_make_null();
    }
}

static int values_equal(Value left, Value right)
{
    if (left.type != right.type)
    {
        return 0;
    }

    switch (left.type)
    {
    case VALUE_NULL:
        return 1;
    case VALUE_INT:
        return left.data.int_val == right.data.int_val;
    case VALUE_BYTE:
        return left.data.byte_val == right.data.byte_val;
    case VALUE_BOOL:
        return left.data.bool_val == right.data.bool_val;
    case VALUE_STRING:
        return strcmp(left.data.string_val, right.data.string_val) == 0;
    default:
        return 0;
    }
}

static char *replace_all_text(const char *source, const char *needle, const char *replacement)
{
    if (!source || !needle || !replacement || needle[0] == '\0')
    {
        char *copy = malloc(strlen(source ? source : "") + 1);
        strcpy(copy, source ? source : "");
        return copy;
    }

    int source_len = strlen(source);
    int needle_len = strlen(needle);
    int replacement_len = strlen(replacement);
    int count = 0;

    for (const char *p = source; (p = strstr(p, needle)) != NULL; p += needle_len)
    {
        count++;
    }

    int result_len = source_len + count * (replacement_len - needle_len);
    char *result = malloc(result_len + 1);
    char *out = result;
    const char *p = source;
    const char *next = NULL;

    while ((next = strstr(p, needle)) != NULL)
    {
        int span = next - p;
        memcpy(out, p, span);
        out += span;
        memcpy(out, replacement, replacement_len);
        out += replacement_len;
        p = next + needle_len;
    }

    strcpy(out, p);
    return result;
}

static int count_substrings(const char *source, const char *needle)
{
    if (!source || !needle || needle[0] == '\0')
    {
        return 0;
    }

    int count = 0;
    int needle_len = strlen(needle);
    const char *p = source;

    while ((p = strstr(p, needle)) != NULL)
    {
        count++;
        p += needle_len;
    }

    return count;
}

static int compare_values(Value left, Value right)
{
    if ((left.type == VALUE_INT || left.type == VALUE_BYTE || left.type == VALUE_BOOL) &&
        (right.type == VALUE_INT || right.type == VALUE_BYTE || right.type == VALUE_BOOL))
    {
        int left_num = left.type == VALUE_INT ? left.data.int_val : left.type == VALUE_BYTE ? left.data.byte_val
                                                                                            : left.data.bool_val;
        int right_num = right.type == VALUE_INT ? right.data.int_val : right.type == VALUE_BYTE ? right.data.byte_val
                                                                                                : right.data.bool_val;
        return left_num - right_num;
    }

    char *left_text = value_to_string(left);
    char *right_text = value_to_string(right);
    int result = strcmp(left_text, right_text);
    free(left_text);
    free(right_text);
    return result;
}

static Value split_string(const char *text, const char *delimiter)
{
    Value result = list_create();

    if (!delimiter || delimiter[0] == '\0')
    {
        char tmp[2] = {'\0', '\0'};
        for (int i = 0; text[i] != '\0'; i++)
        {
            tmp[0] = text[i];
            list_append(&result, value_make_string(tmp));
        }
        return result;
    }

    int delimiter_len = strlen(delimiter);
    const char *start = text;
    const char *next = NULL;

    while ((next = strstr(start, delimiter)) != NULL)
    {
        int part_len = next - start;
        char *part = malloc(part_len + 1);
        memcpy(part, start, part_len);
        part[part_len] = '\0';
        list_append(&result, value_make_string(part));
        free(part);
        start = next + delimiter_len;
    }

    list_append(&result, value_make_string((char *)start));
    return result;
}

static char *duplicate_path(const char *module_name)
{
    char *path = malloc(strlen(module_name) + 4 + 1);
    int w = 0;
    for (int i = 0; module_name[i] != '\0'; i++)
    {
        if (module_name[i] == '.')
        {
            path[w++] = '/';
        }
        else
        {
            path[w++] = module_name[i];
        }
    }
    path[w++] = '.';
    path[w++] = 'a';
    path[w] = '\0';
    return path;
}

static char *module_alias_name(const char *module_name, const char *alias)
{
    if (alias)
    {
        char *alias_name = malloc(strlen(alias) + 1);
        strcpy(alias_name, alias);
        return alias_name;
    }

    const char *last_dot = strrchr(module_name, '.');
    const char *base = last_dot ? last_dot + 1 : module_name;
    char *alias_name = malloc(strlen(base) + 1);
    strcpy(alias_name, base);
    return alias_name;
}

Value eval_import_statement(ASTNode *node, Environment *env)
{
    if (node->type != AST_IMPORT_STATEMENT_TYPE)
    {
        return value_make_null();
    }

    AST_IMPORT_STATEMENT *import_stmt = &node->data.import_statement;
    char *path = duplicate_path(import_stmt->module_name);

    FILE *file = fopen(path, "r");
    if (!file)
    {
        fprintf(stderr, "Error: could not open module file '%s'\n", path);
        free(path);
        return value_make_null();
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        free(path);
        return value_make_null();
    }

    long size = ftell(file);
    if (size < 0)
    {
        fclose(file);
        free(path);
        return value_make_null();
    }
    rewind(file);

    char *source = malloc((size_t)size + 1);
    if (!source)
    {
        fclose(file);
        free(path);
        return value_make_null();
    }

    if (fread(source, 1, (size_t)size, file) != (size_t)size)
    {
        free(source);
        fclose(file);
        free(path);
        return value_make_null();
    }
    source[size] = '\0';
    fclose(file);
    free(path);

    Program *module_program = parser_parse_source(source);
    free(source);

    Environment *module_env = env_create();
    register_builtins(module_env);
    eval_program(module_program, module_env);

    Value module_value = dict_create();
    SymbolTable *scope = &module_env->scopes[0];
    for (int i = 0; i < scope->count; i++)
    {
        Value copied = copy_value(scope->symbols[i].value);
        dict_set(&module_value, scope->symbols[i].name, copied);
    }

    char *alias_name = module_alias_name(import_stmt->module_name, import_stmt->alias);
    env_define(env, alias_name, module_value);
    free(alias_name);

    return value_make_null();
}

Value eval_literal(ASTNode *node)
{
    if (node->type != AST_LITERAL_TYPE)
    {
        return value_make_null();
    }

    AST_LITERAL *lit = &node->data.literal;

    switch (lit->type)
    {
    case 15:
        return value_make_int(atoi(lit->value));
    case 16:
        return value_make_string(lit->value);
    case 29:
        return value_make_bool(true);
    case 30:
        return value_make_bool(false);
    case 24:
        return value_make_null();
    case 25:
        return value_make_string(lit->value);
    default:
        return value_make_null();
    }
}

Value eval_identifier(ASTNode *node, Environment *env)
{
    if (node->type != AST_IDENTIFIER_TYPE)
    {
        return value_make_null();
    }

    char *name = node->data.identifier.value;

    if (!env_exists(env, name))
    {
        int found = 0;
        Value field = get_this_field(env, name, &found);
        if (found)
        {
            return field;
        }
        fprintf(stderr, "Error: undefined variable '%s'\n", name);
        return value_make_null();
    }

    return env_get(env, name);
}

Value eval_binary_op(ASTNode *node, Environment *env)
{
    if (node->type != AST_BINARY_OPERATION_TYPE)
    {
        return value_make_null();
    }

    AST_BINARY_OPERATION *binop = &node->data.binary_operation;
    Value left = eval_expression(binop->left_operand, env);
    Value right = eval_expression(binop->right_operand, env);

    int op = binop->op;

    if (op == 9)
    {
        if (left.type == VALUE_STRING || right.type == VALUE_STRING)
        {
            char *left_str = value_to_string(left);
            char *right_str = value_to_string(right);
            char *result = malloc(strlen(left_str) + strlen(right_str) + 1);
            strcpy(result, left_str);
            strcat(result, right_str);
            Value res = value_make_string(result);
            free(left_str);
            free(right_str);
            free(result);
            return res;
        }
        else if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            return value_make_int(left.data.int_val + right.data.int_val);
        }
    }
    else if (op == 10)
    {
        if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            return value_make_int(left.data.int_val - right.data.int_val);
        }
    }
    else if (op == 11)
    {
        if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            return value_make_int(left.data.int_val * right.data.int_val);
        }
    }
    else if (op == 12)
    {
        if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            if (right.data.int_val == 0)
            {
                fprintf(stderr, "Error: division by zero\n");
                return value_make_null();
            }
            return value_make_int(left.data.int_val / right.data.int_val);
        }
    }
    else if (op == 46)
    {
        if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            if (right.data.int_val == 0)
            {
                fprintf(stderr, "Error: modulo by zero\n");
                return value_make_null();
            }
            return value_make_int(left.data.int_val % right.data.int_val);
        }
    }
    else if (op == 52)
    {
        if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            return value_make_bool(left.data.int_val < right.data.int_val);
        }
    }
    else if (op == 53)
    {
        if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            return value_make_bool(left.data.int_val > right.data.int_val);
        }
    }
    else if (op == 113)
    {
        if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            return value_make_bool(left.data.int_val == right.data.int_val);
        }
        else if (left.type == VALUE_BOOL && right.type == VALUE_BOOL)
        {
            return value_make_bool(left.data.bool_val == right.data.bool_val);
        }
        else if (left.type == VALUE_STRING && right.type == VALUE_STRING)
        {
            return value_make_bool(strcmp(left.data.string_val, right.data.string_val) == 0);
        }
        return value_make_bool(left.type == right.type);
    }
    else if (op == 111)
    {
        if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            return value_make_bool(left.data.int_val != right.data.int_val);
        }
        else if (left.type == VALUE_BOOL && right.type == VALUE_BOOL)
        {
            return value_make_bool(left.data.bool_val != right.data.bool_val);
        }
        return value_make_bool(left.type != right.type);
    }
    else if (op == 112)
    {
        if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            return value_make_bool(left.data.int_val <= right.data.int_val);
        }
    }
    else if (op == 114)
    {
        if (left.type == VALUE_INT && right.type == VALUE_INT)
        {
            return value_make_bool(left.data.int_val >= right.data.int_val);
        }
    }
    else if (op == 115 || op == 47)
    {
        return value_make_bool(value_is_truthy(left) && value_is_truthy(right));
    }
    else if (op == 116 || op == 49)
    {
        return value_make_bool(value_is_truthy(left) || value_is_truthy(right));
    }

    return value_make_null();
}

Value eval_unary_op(ASTNode *node, Environment *env)
{
    if (node->type != AST_UNARY_OPERATION_TYPE)
    {
        return value_make_null();
    }

    AST_UNARY_OPERATION *unop = &node->data.unary_operation;
    int op = unop->op;

    if (op == 10)
    {
        Value val = eval_expression(unop->operand, env);
        if (val.type == VALUE_INT)
        {
            return value_make_int(-val.data.int_val);
        }
    }
    else if (op == 54)
    {
        Value val = eval_expression(unop->operand, env);
        return value_make_bool(!value_is_truthy(val));
    }
    else if (op == 107)
    {
        if (unop->operand->type == AST_IDENTIFIER_TYPE)
        {
            char *name = unop->operand->data.identifier.value;
            Value val = env_get(env, name);
            if (val.type == VALUE_INT)
            {
                Value new_val = value_make_int(val.data.int_val + 1);
                env_set(env, name, new_val);
                return new_val;
            }
        }
    }
    else if (op == 108)
    {
        if (unop->operand->type == AST_IDENTIFIER_TYPE)
        {
            char *name = unop->operand->data.identifier.value;
            Value val = env_get(env, name);
            if (val.type == VALUE_INT)
            {
                Value old_val = value_make_int(val.data.int_val);
                Value new_val = value_make_int(val.data.int_val - 1);
                env_set(env, name, new_val);
                return old_val;
            }
        }
    }

    return value_make_null();
}

Value eval_cast(ASTNode *node, Environment *env)
{
    if (node->type != AST_CAST_TYPE)
    {
        return value_make_null();
    }

    AST_CAST *cast = &node->data.cast;
    Value value = eval_expression(cast->expression, env);

    switch (cast->target_type)
    {
    case 39:
        if (value.type == VALUE_INT)
        {
            return value;
        }
        if (value.type == VALUE_BYTE)
        {
            return value_make_int(value.data.byte_val);
        }
        if (value.type == VALUE_BOOL)
        {
            return value_make_int(value.data.bool_val ? 1 : 0);
        }
        if (value.type == VALUE_STRING)
        {
            return value_make_int(atoi(value.data.string_val));
        }
        break;
    case 36:
    {
        char *str = value_to_string(value);
        Value result = value_make_string(str);
        free(str);
        return result;
    }
    case 40:
        return value_make_bool(value_is_truthy(value));
    case TOKEN_BYTE:
        if (value.type == VALUE_BYTE)
        {
            return value;
        }
        if (value.type == VALUE_INT)
        {
            return value_make_byte(value.data.int_val);
        }
        if (value.type == VALUE_BOOL)
        {
            return value_make_byte(value.data.bool_val ? 1 : 0);
        }
        if (value.type == VALUE_STRING)
        {
            return value_make_byte(atoi(value.data.string_val));
        }
        break;
    case 35:
        if (value.type == VALUE_STRING && value.data.string_val && value.data.string_val[0] != '\0')
        {
            char tmp[2] = {value.data.string_val[0], '\0'};
            return value_make_string(tmp);
        }
        if (value.type == VALUE_INT)
        {
            char tmp[2] = {(char)value.data.int_val, '\0'};
            return value_make_string(tmp);
        }
        break;
    case 38:
        if (value.type == VALUE_INT)
        {
            return value;
        }
        if (value.type == VALUE_BYTE)
        {
            return value_make_int(value.data.byte_val);
        }
        if (value.type == VALUE_BOOL)
        {
            return value_make_int(value.data.bool_val ? 1 : 0);
        }
        if (value.type == VALUE_STRING)
        {
            return value_make_int(atoi(value.data.string_val));
        }
        break;
    default:
        break;
    }

    return value_make_null();
}

Value eval_list_literal(ASTNode *node, Environment *env)
{
    if (node->type != AST_LIST_LITERAL_TYPE)
    {
        return value_make_null();
    }

    Value list = list_create();
    AST_LIST_LITERAL *literal = &node->data.list_literal;

    for (int i = 0; i < literal->element_count; i++)
    {
        list_append(&list, eval_expression(literal->elements[i], env));
    }

    return list;
}

Value eval_dict_literal(ASTNode *node, Environment *env)
{
    if (node->type != AST_DICT_LITERAL_TYPE)
    {
        return value_make_null();
    }

    Value dict = dict_create();
    AST_DICT_LITERAL *literal = &node->data.dict_literal;

    for (int i = 0; i < literal->entry_count; i++)
    {
        Value key_val = eval_expression(literal->keys[i], env);
        Value value_val = eval_expression(literal->values[i], env);
        char *key_str = value_to_string(key_val);
        dict_set(&dict, key_str, value_val);
        free(key_str);
    }

    return dict;
}

Value eval_function_call(ASTNode *node, Environment *env)
{
    if (node->type != AST_FUNCTION_CALL_DEFINITION_TYPE)
    {
        return value_make_null();
    }

    AST_FUNCTION_CALL_DEFINITION *call = &node->data.function_call_definition;

    if (call->name->type == AST_MEMBER_ACCESS_TYPE)
    {
        return eval_method_call(node, env);
    }

    char *func_name = NULL;
    if (call->name->type == AST_IDENTIFIER_TYPE)
    {
        func_name = call->name->data.identifier.value;
    }

    if (!func_name)
        return value_make_null();

    Value *args = malloc(sizeof(Value) * call->argument_count);
    for (int i = 0; i < call->argument_count; i++)
    {
        args[i] = eval_expression(call->arguments[i], env);
    }

    if (strcmp(func_name, "print") == 0)
    {
        return builtin_print(args, call->argument_count);
    }
    else if (strcmp(func_name, "int") == 0)
    {
        return builtin_int(args, call->argument_count);
    }
    else if (strcmp(func_name, "string") == 0)
    {
        return builtin_string(args, call->argument_count);
    }
    else if (strcmp(func_name, "list") == 0)
    {
        return builtin_list(args, call->argument_count);
    }
    else if (strcmp(func_name, "len") == 0)
    {
        return builtin_len(args, call->argument_count);
    }
    else if (strcmp(func_name, "input") == 0)
    {
        return builtin_input(args, call->argument_count);
    }
    else if (strcmp(func_name, "type") == 0)
    {
        return builtin_type(args, call->argument_count);
    }
    else if (strcmp(func_name, "randint") == 0)
    {
        return builtin_randint(args, call->argument_count);
    }
    else if (strcmp(func_name, "pow") == 0)
    {
        return builtin_pow(args, call->argument_count);
    }
    else if (strcmp(func_name, "read_file") == 0)
    {
        return builtin_read_file(args, call->argument_count);
    }
    else if (strcmp(func_name, "write_file") == 0)
    {
        return builtin_write_file(args, call->argument_count);
    }
    else if (strcmp(func_name, "queue_create") == 0)
    {
        return builtin_queue_create(args, call->argument_count);
    }
    else if (strcmp(func_name, "queue_add") == 0)
    {
        return builtin_queue_add(args, call->argument_count);
    }
    else if (strcmp(func_name, "queue_remove") == 0)
    {
        return builtin_queue_remove(args, call->argument_count);
    }
    else if (strcmp(func_name, "queue_peek") == 0)
    {
        return builtin_queue_peek(args, call->argument_count);
    }
    else if (strcmp(func_name, "queue_is_empty") == 0)
    {
        return builtin_queue_is_empty(args, call->argument_count);
    }
    else
    {
        Value func_val = env_get(env, func_name);

        if (func_val.type == VALUE_CLASS_INSTANCE)
        {
            return eval_class_instantiation(func_name, args, call->argument_count, env);
        }
        else if (func_val.type == VALUE_STRUCT)
        {
            return eval_struct_instantiation(func_name, args, call->argument_count, env);
        }
        else if (func_val.type == VALUE_FUNCTION)
        {
            FunctionValue *func = &func_val.data.function_val;
            if (!func->body || func->body->type != AST_FUNCTION_DECLARATION_TYPE)
            {
                free(args);
                return value_make_null();
            }

            AST_FUNCTION_DECLARATION *func_decl = &func->body->data.function_declaration;

            env_push_scope(func->closure);

            for (int i = 0; i < func_decl->parameter_count && i < call->argument_count; i++)
            {
                ASTNode *param = func_decl->parameters[i];
                if (param->type == AST_VARIABLE_DECLARATION_TYPE)
                {
                    char *param_name = param->data.variable_declaration.name;
                    env_define(func->closure, param_name, args[i]);
                }
            }

            for (int i = call->argument_count; i < func_decl->parameter_count; i++)
            {
                ASTNode *param = func_decl->parameters[i];
                if (param->type == AST_VARIABLE_DECLARATION_TYPE)
                {
                    char *param_name = param->data.variable_declaration.name;
                    if (param->data.variable_declaration.init_value)
                    {
                        Value default_val = eval_expression(param->data.variable_declaration.init_value, func->closure);
                        env_define(func->closure, param_name, default_val);
                    }
                }
            }

            return_flag.is_return = 0;
            ASTNode body_node;
            body_node.type = AST_BLOCK_TYPE;
            body_node.data.block = *func_decl->body;
            eval_statement(&body_node, func->closure);

            Value result = return_flag.return_value;
            return_flag.is_return = 0;

            env_pop_scope(func->closure);

            free(args);
            return result;
        }
    }

    free(args);
    return value_make_null();
}

Value eval_member_access(ASTNode *node, Environment *env)
{
    if (node->type != AST_MEMBER_ACCESS_TYPE)
    {
        return value_make_null();
    }

    AST_MEMBER_ACCESS *access = &node->data.member_access;
    Value obj = eval_expression(access->object, env);
    char *member_name = access->member;

    if (obj.type == VALUE_STRUCT)
    {
        StructValue *struct_val = obj.data.struct_val;

        for (int i = 0; i < struct_val->field_count; i++)
        {
            if (strcmp(struct_val->field_names[i], member_name) == 0)
            {
                return struct_val->fields[i];
            }
        }

        fprintf(stderr, "Error: struct '%s' has no field '%s'\n",
                struct_val->name, member_name);
        return value_make_null();
    }

    if (obj.type == VALUE_CLASS_INSTANCE)
    {
        Value this_val = env_get(env, "this");
        int is_inside_class = (this_val.type == VALUE_CLASS_INSTANCE);

        ClassInstanceValue *class_val = obj.data.class_instance_val;

        for (int i = 0; i < class_val->field_count; i++)
        {
            if (strcmp(class_val->field_names[i], member_name) == 0)
            {
                if (class_val->field_private[i] && !is_inside_class)
                {
                    fprintf(stderr, "Error: cannot access private member '%s'\n", member_name);
                    return value_make_null();
                }
                return class_val->fields[i];
            }
        }

        fprintf(stderr, "Error: class '%s' has no field '%s'\n",
                class_val->class_name, member_name);
        return value_make_null();
    }

    if (obj.type == VALUE_DICT)
    {
        DictValue *dict = &obj.data.dict_val;
        for (int i = 0; i < dict->count; i++)
        {
            if (strcmp(dict->keys[i], member_name) == 0)
            {
                return dict->values[i];
            }
        }
        fprintf(stderr, "Error: module has no member '%s'\n", member_name);
        return value_make_null();
    }

    fprintf(stderr, "Error: cannot access member '%s' on non-class or non-dict value\n", member_name);
    return value_make_null();
}

Value eval_index_access(ASTNode *node, Environment *env)
{
    if (node->type != AST_INDEX_ACCESS_TYPE)
    {
        return value_make_null();
    }

    AST_INDEX_ACCESS *access = &node->data.index_access;
    Value arr = eval_expression(access->array, env);
    Value idx = eval_expression(access->index, env);

    if (arr.type == VALUE_LIST && idx.type == VALUE_INT)
    {
        return list_get(arr, idx.data.int_val);
    }
    else if (arr.type == VALUE_DICT && idx.type == VALUE_STRING)
    {
        return dict_get(arr, idx.data.string_val);
    }

    return value_make_null();
}

void eval_variable_declaration(ASTNode *node, Environment *env)
{
    if (node->type != AST_VARIABLE_DECLARATION_TYPE)
        return;

    AST_VARIABLE_DECLARATION *vardecl = &node->data.variable_declaration;

    Value val = value_make_null();
    if (vardecl->init_value)
    {
        val = eval_expression(vardecl->init_value, env);
    }

    env_define(env, vardecl->name, val);
}

void eval_block(ASTNode *node, Environment *env)
{
    if (node->type != AST_BLOCK_TYPE)
        return;

    AST_BLOCK *block = &node->data.block;
    env_push_scope(env);

    for (int i = 0; i < block->statement_count; i++)
    {
        eval_statement(block->statements[i], env);
        if (return_flag.is_return)
            break;
    }

    env_pop_scope(env);
}

void eval_if_statement(ASTNode *node, Environment *env)
{
    if (node->type != AST_IF_STATEMENT_TYPE)
        return;

    AST_IF_STATEMENT *ifstmt = &node->data.if_statement;

    Value cond = eval_expression(ifstmt->condition, env);

    if (value_is_truthy(cond))
    {
        env_push_scope(env);
        for (int i = 0; i < ifstmt->then_block->statement_count; i++)
        {
            eval_statement(ifstmt->then_block->statements[i], env);
            if (return_flag.is_return)
                break;
        }
        env_pop_scope(env);
    }
    else
    {
        int found_true = 0;
        for (int i = 0; i < ifstmt->else_if_count; i++)
        {
            Value elif_cond = eval_expression(ifstmt->else_if_conditions[i], env);
            if (value_is_truthy(elif_cond))
            {
                env_push_scope(env);
                for (int j = 0; j < ifstmt->else_if_blocks[i]->statement_count; j++)
                {
                    eval_statement(ifstmt->else_if_blocks[i]->statements[j], env);
                    if (return_flag.is_return)
                        break;
                }
                env_pop_scope(env);
                found_true = 1;
                break;
            }
        }

        if (!found_true && ifstmt->else_block)
        {
            env_push_scope(env);
            for (int i = 0; i < ifstmt->else_block->statement_count; i++)
            {
                eval_statement(ifstmt->else_block->statements[i], env);
                if (return_flag.is_return)
                    break;
            }
            env_pop_scope(env);
        }
    }
}

void eval_loop_statement(ASTNode *node, Environment *env)
{
    if (node->type != AST_LOOP_STATEMENT_TYPE)
        return;

    AST_LOOP_STATEMENT *loop = &node->data.loop_statement;

    if (strcmp(loop->loop_type, "counted") == 0)
    {
        Value count_val = eval_expression(loop->parameters[0], env);

        if (count_val.type == VALUE_INT)
        {
            int count = count_val.data.int_val;
            ASTNode *var_node = loop->parameters[1];
            char *var_name = var_node->data.identifier.value;

            for (int i = 0; i < count; i++)
            {
                env_push_scope(env);
                env_define(env, var_name, value_make_int(i));

                for (int j = 0; j < loop->body->statement_count; j++)
                {
                    eval_statement(loop->body->statements[j], env);
                    if (return_flag.is_return)
                        break;
                }

                env_pop_scope(env);
                if (return_flag.is_return)
                    break;
            }
        }
    }
    else if (strcmp(loop->loop_type, "conditional") == 0)
    {
        while (value_is_truthy(eval_expression(loop->parameters[0], env)))
        {
            env_push_scope(env);
            for (int i = 0; i < loop->body->statement_count; i++)
            {
                eval_statement(loop->body->statements[i], env);
                if (return_flag.is_return)
                    break;
            }
            env_pop_scope(env);
            if (return_flag.is_return)
                break;
        }
    }
    else
    {
        printf("Error: loop type not detected");
    }
}

void eval_pass_statement(ASTNode *node)
{
    (void)node;
}

void eval_return_statement(ASTNode *node, Environment *env)
{
    if (node->type != AST_RETURN_STATEMENT_TYPE)
        return;

    return_flag.is_return = 1;
    if (node->data.return_statement.value)
    {
        return_flag.return_value = eval_expression(node->data.return_statement.value, env);
    }
}

Value eval_struct_instantiation(char *struct_name, Value *args, int arg_count, Environment *env)
{
    Value struct_def = env_get(env, struct_name);

    if (struct_def.type != VALUE_STRUCT)
    {
        fprintf(stderr, "Error: '%s' is not a struct\n", struct_name);
        return value_make_null();
    }

    StructValue *struct_val = struct_def.data.struct_val;

    if (arg_count != struct_val->field_count)
    {
        fprintf(stderr, "Error: struct '%s' expects %d arguments, got %d\n",
                struct_name, struct_val->field_count, arg_count);
        return value_make_null();
    }

    StructValue *instance = malloc(sizeof(StructValue));
    instance->name = malloc(strlen(struct_name) + 1);
    strcpy(instance->name, struct_name);
    instance->fields = malloc(sizeof(Value) * struct_val->field_count);
    instance->field_names = malloc(sizeof(char *) * struct_val->field_count);
    instance->field_count = struct_val->field_count;

    for (int i = 0; i < struct_val->field_count; i++)
    {
        instance->fields[i] = args[i];
        instance->field_names[i] = malloc(strlen(struct_val->field_names[i]) + 1);
        strcpy(instance->field_names[i], struct_val->field_names[i]);
    }

    Value result;
    result.type = VALUE_STRUCT;
    result.data.struct_val = instance;
    return result;
}

Value eval_expression(ASTNode *node, Environment *env)
{
    if (!node)
        return value_make_null();

    switch (node->type)
    {
    case AST_LITERAL_TYPE:
        return eval_literal(node);
    case AST_IDENTIFIER_TYPE:
        return eval_identifier(node, env);
    case AST_BINARY_OPERATION_TYPE:
        return eval_binary_op(node, env);
    case AST_UNARY_OPERATION_TYPE:
        return eval_unary_op(node, env);
    case AST_CAST_TYPE:
        return eval_cast(node, env);
    case AST_FUNCTION_CALL_DEFINITION_TYPE:
        return eval_function_call(node, env);
    case AST_MEMBER_ACCESS_TYPE:
        return eval_member_access(node, env);
    case AST_INDEX_ACCESS_TYPE:
        return eval_index_access(node, env);
    case AST_LIST_LITERAL_TYPE:
        return eval_list_literal(node, env);
    case AST_DICT_LITERAL_TYPE:
        return eval_dict_literal(node, env);
    case AST_F_STRING_TYPE:
        return eval_f_string(node, env);
    default:
        return value_make_null();
    }
}

void eval_statement(ASTNode *node, Environment *env)
{
    if (!node)
        return;

    switch (node->type)
    {
    case AST_VARIABLE_DECLARATION_TYPE:
        eval_variable_declaration(node, env);
        break;
    case AST_BLOCK_TYPE:
        eval_block(node, env);
        break;
    case AST_IF_STATEMENT_TYPE:
        eval_if_statement(node, env);
        break;
    case AST_LOOP_STATEMENT_TYPE:
        eval_loop_statement(node, env);
        break;
    case AST_PASS_STATEMENT_TYPE:
        eval_pass_statement(node);
        break;
    case AST_RETURN_STATEMENT_TYPE:
        eval_return_statement(node, env);
        break;
    case AST_ASSIGNMENT_TYPE:
        eval_assignment(node, env);
        break;
    case AST_IMPORT_STATEMENT_TYPE:
        eval_import_statement(node, env);
        break;
    default:
        eval_expression(node, env);
        break;
    }
}

void eval_program(ASTNode *node, Environment *env)
{
    if (node->type != AST_PROGRAM_TYPE)
        return;

    AST_PROGRAM *prog = &node->data.program;

    for (int i = 0; i < prog->declaration_count; i++)
    {
        ASTNode *decl = prog->declarations[i];

        if (decl->type == AST_FUNCTION_DECLARATION_TYPE)
        {
            AST_FUNCTION_DECLARATION *func_decl = &decl->data.function_declaration;

            Value func_val;
            func_val.type = VALUE_FUNCTION;
            func_val.data.function_val.body = decl;
            func_val.data.function_val.closure = env;

            env_define(env, func_decl->name, func_val);
        }
        else if (decl->type == AST_STRUCT_DECLARATION_TYPE)
        {
            AST_STRUCT_DECLARATION *struct_decl = &decl->data.struct_declaration;

            StructValue *struct_val = malloc(sizeof(StructValue));
            struct_val->name = malloc(strlen(struct_decl->name) + 1);
            strcpy(struct_val->name, struct_decl->name);
            struct_val->field_count = struct_decl->field_count;
            struct_val->fields = malloc(sizeof(Value) * struct_decl->field_count);

            struct_val->field_names = malloc(sizeof(char *) * struct_decl->field_count);
            for (int j = 0; j < struct_decl->field_count; j++)
            {
                struct_val->fields[j] = value_make_null();
                struct_val->field_names[j] = malloc(strlen(struct_decl->field_names[j]) + 1);
                strcpy(struct_val->field_names[j], struct_decl->field_names[j]);
            }

            Value struct_value;
            struct_value.type = VALUE_STRUCT;
            struct_value.data.struct_val = struct_val;

            env_define(env, struct_decl->name, struct_value);
        }
        else if (decl->type == AST_CLASS_DECLARATION_TYPE)
        {
            AST_CLASS_DECLARATION *class_decl = &decl->data.class_declaration;

            Value class_value;
            class_value.type = VALUE_CLASS_INSTANCE;
            class_value.data.class_instance_val = (ClassInstanceValue *)class_decl;

            env_define(env, class_decl->name, class_value);
        }
    }

    for (int i = 0; i < prog->statement_count; i++)
    {
        eval_statement(prog->statements[i], env);
    }
}

Value eval_class_instantiation(char *class_name, Value *args, int arg_count, Environment *env)
{
    Value class_def = env_get(env, class_name);

    if (class_def.type != VALUE_CLASS_INSTANCE)
    {
        fprintf(stderr, "Error: '%s' is not a class\n", class_name);
        return value_make_null();
    }

    AST_CLASS_DECLARATION *class_decl = (AST_CLASS_DECLARATION *)class_def.data.class_instance_val;

    ClassInstanceValue *instance = malloc(sizeof(ClassInstanceValue));
    instance->class_name = malloc(strlen(class_name) + 1);
    strcpy(instance->class_name, class_name);
    instance->field_count = class_decl->private_variable_count + class_decl->public_variable_count;
    instance->fields = malloc(sizeof(Value) * instance->field_count);
    instance->field_private = malloc(sizeof(int) * instance->field_count);
    instance->field_names = malloc(sizeof(char *) * instance->field_count);
    instance->class_decl = class_decl;

    int field_idx = 0;
    for (int i = 0; i < class_decl->private_variable_count; i++)
    {
        AST_VARIABLE_DECLARATION *field = &class_decl->private_variables[i]->data.variable_declaration;
        instance->field_names[field_idx] = malloc(strlen(field->name) + 1);
        strcpy(instance->field_names[field_idx], field->name);
        instance->fields[field_idx] = value_make_null();
        instance->field_private[field_idx] = 1;
        field_idx++;
    }

    for (int i = 0; i < class_decl->public_variable_count; i++)
    {
        AST_VARIABLE_DECLARATION *field = &class_decl->public_variables[i]->data.variable_declaration;
        instance->field_names[field_idx] = malloc(strlen(field->name) + 1);
        strcpy(instance->field_names[field_idx], field->name);
        instance->fields[field_idx] = value_make_null();
        instance->field_private[field_idx] = 0;
        field_idx++;
    }

    Value instance_value;
    instance_value.type = VALUE_CLASS_INSTANCE;
    instance_value.data.class_instance_val = instance;

    ASTNode *constructor = NULL;
    for (int i = 0; i < class_decl->private_method_count; i++)
    {
        ASTNode *method = class_decl->private_methods[i];
        if (method->type == AST_FUNCTION_DECLARATION_TYPE)
        {
            AST_FUNCTION_DECLARATION *func_decl = &method->data.function_declaration;
            if (strcmp(func_decl->name, class_name) == 0)
            {
                constructor = method;
                break;
            }
        }
    }

    if (!constructor)
    {
        for (int i = 0; i < class_decl->public_method_count; i++)
        {
            ASTNode *method = class_decl->public_methods[i];
            if (method->type == AST_FUNCTION_DECLARATION_TYPE)
            {
                AST_FUNCTION_DECLARATION *func_decl = &method->data.function_declaration;
                if (strcmp(func_decl->name, class_name) == 0)
                {
                    constructor = method;
                    break;
                }
            }
        }
    }

    if (constructor)
    {
        AST_FUNCTION_DECLARATION *constructor_decl = &constructor->data.function_declaration;

        Environment *instance_env = env_create();
        env_define(instance_env, "this", instance_value);

        for (int i = 0; i < constructor_decl->parameter_count && i < arg_count; i++)
        {
            ASTNode *param = constructor_decl->parameters[i];
            if (param->type == AST_VARIABLE_DECLARATION_TYPE)
            {
                char *param_name = param->data.variable_declaration.name;
                Value arg_val = args[i];
                env_define(instance_env, param_name, arg_val);
            }
        }

        return_flag.is_return = 0;
        ASTNode body_node;
        body_node.type = AST_BLOCK_TYPE;
        body_node.data.block = *constructor_decl->body;
        eval_statement(&body_node, instance_env);
        return_flag.is_return = 0;

        env_free(instance_env);
    }

    return instance_value;
}

static int value_to_int_like(Value value, int *ok)
{
    switch (value.type)
    {
    case VALUE_INT:
        *ok = 1;
        return value.data.int_val;
    case VALUE_BYTE:
        *ok = 1;
        return value.data.byte_val;
    case VALUE_BOOL:
        *ok = 1;
        return value.data.bool_val ? 1 : 0;
    default:
        *ok = 0;
        return 0;
    }
}

static Value apply_assignment_operator(Value current, Value right, int op)
{
    if (op == 13)
    {
        return right;
    }

    if (op == 109)
    {
        if (current.type == VALUE_STRING || right.type == VALUE_STRING)
        {
            char *left_str = value_to_string(current);
            char *right_str = value_to_string(right);
            char *result = malloc(strlen(left_str) + strlen(right_str) + 1);
            strcpy(result, left_str);
            strcat(result, right_str);
            Value combined = value_make_string(result);
            free(left_str);
            free(right_str);
            free(result);
            return combined;
        }

        int left_ok = 0;
        int right_ok = 0;
        int left = value_to_int_like(current, &left_ok);
        int rhs = value_to_int_like(right, &right_ok);
        if (left_ok && right_ok)
        {
            return value_make_int(left + rhs);
        }
        return right;
    }

    if (op == 110)
    {
        int left_ok = 0;
        int right_ok = 0;
        int left = value_to_int_like(current, &left_ok);
        int rhs = value_to_int_like(right, &right_ok);
        if (left_ok && right_ok)
        {
            return value_make_int(left - rhs);
        }
        return right;
    }

    return right;
}

void eval_assignment(ASTNode *node, Environment *env)
{
    if (node->type != AST_ASSIGNMENT_TYPE)
        return;

    AST_ASSIGNMENT *assignment = &node->data.assignment;
    Value right_val = eval_expression(assignment->value, env);

    if (assignment->target->type == AST_IDENTIFIER_TYPE)
    {
        char *var_name = assignment->target->data.identifier.value;
        int has_local = env_exists(env, var_name);
        int has_field = 0;
        Value current_val = value_make_null();

        if (has_local)
        {
            current_val = env_get(env, var_name);
        }
        else
        {
            current_val = get_this_field(env, var_name, &has_field);
        }

        Value assigned = apply_assignment_operator(current_val, right_val, assignment->op);
        if (!has_local && has_field && set_this_field(env, var_name, assigned))
        {
            return;
        }
        env_set(env, var_name, assigned);
    }
    else if (assignment->target->type == AST_MEMBER_ACCESS_TYPE)
    {
        AST_MEMBER_ACCESS *access = &assignment->target->data.member_access;
        Value obj = eval_expression(access->object, env);
        if (obj.type == VALUE_CLASS_INSTANCE)
        {
            ClassInstanceValue *instance = obj.data.class_instance_val;
            int index = class_field_index(instance, access->member);
            if (index >= 0)
            {
                Value current = instance->fields[index];
                Value assigned = apply_assignment_operator(current, right_val, assignment->op);
                value_free(instance->fields[index]);
                instance->fields[index] = assigned;
            }
        }
    }
    else if (assignment->target->type == AST_INDEX_ACCESS_TYPE)
    {
        AST_INDEX_ACCESS *access = &assignment->target->data.index_access;
        Value *collection = resolve_target_ref(access->array, env);
        Value index = eval_expression(access->index, env);
        if (collection && collection->type == VALUE_LIST && index.type == VALUE_INT)
        {
            int i = index.data.int_val;
            if (i >= 0 && i < collection->data.list_val.count)
            {
                Value current = collection->data.list_val.elements[i];
                Value assigned = apply_assignment_operator(current, right_val, assignment->op);
                list_set(collection, i, assigned);
            }
        }
        else if (collection && collection->type == VALUE_DICT)
        {
            char *key = value_to_string(index);
            Value current = dict_get(*collection, key);
            Value assigned = apply_assignment_operator(current, right_val, assignment->op);
            dict_set(collection, key, assigned);
            free(key);
        }
    }
}

Value eval_method_call(ASTNode *node, Environment *env)
{
    if (node->type != AST_FUNCTION_CALL_DEFINITION_TYPE)
    {
        return value_make_null();
    }

    AST_FUNCTION_CALL_DEFINITION *call = &node->data.function_call_definition;

    if (call->name->type != AST_MEMBER_ACCESS_TYPE)
    {
        return value_make_null();
    }

    AST_MEMBER_ACCESS *member = &call->name->data.member_access;
    Value obj = eval_expression(member->object, env);
    char *method_name = member->member;

    if (obj.type == VALUE_JSON_MODULE)
    {
        if (strcmp(method_name, "dumps") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value payload = eval_expression(call->arguments[0], env);
            return json_dumps_value(payload);
        }

        if (strcmp(method_name, "loads") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value payload = eval_expression(call->arguments[0], env);
            return json_loads_value(payload);
        }

        if (strcmp(method_name, "load") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value payload = eval_expression(call->arguments[0], env);
            return json_load_value(payload);
        }

        if (strcmp(method_name, "dump") == 0)
        {
            if (call->argument_count != 2)
            {
                return value_make_null();
            }
            Value file_path = eval_expression(call->arguments[0], env);
            Value payload = eval_expression(call->arguments[1], env);
            return json_dump_value(file_path, payload);
        }

        if (strcmp(method_name, "customEncode") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value payload = eval_expression(call->arguments[0], env);
            return json_custom_encode_value(payload);
        }

        if (strcmp(method_name, "customDecode") == 0)
        {
            if (call->argument_count != 2)
            {
                return value_make_null();
            }
            Value payload = eval_expression(call->arguments[0], env);
            Value type_name = eval_expression(call->arguments[1], env);
            return json_custom_decode_value(payload, type_name, env);
        }

        fprintf(stderr, "Error: json has no method '%s'\n", method_name);
        return value_make_null();
    }

    if (obj.type == VALUE_SQL_MODULE)
    {
        return sql_module_method(method_name, call, env);
    }

    if (obj.type == VALUE_SQL_DB)
    {
        return sql_db_method(obj.data.sql_db_val, method_name, call, env);
    }

    if (obj.type == VALUE_LIST)
    {
        Value *list = NULL;
        if (member->object->type == AST_IDENTIFIER_TYPE)
        {
            list = env_get_ref(env, member->object->data.identifier.value);
        }

        if (strcmp(method_name, "fill") == 0)
        {
            if (call->argument_count == 1 && list)
            {
                Value value = eval_expression(call->arguments[0], env);
                list_fill(list, value);
            }
            return value_make_null();
        }

        if (strcmp(method_name, "append") == 0)
        {
            if (call->argument_count == 1 && list)
            {
                Value value = eval_expression(call->arguments[0], env);
                list_append(list, value);
            }
            return value_make_null();
        }

        if (strcmp(method_name, "remove") == 0)
        {
            if (call->argument_count == 1 && list)
            {
                Value value = eval_expression(call->arguments[0], env);
                for (int i = 0; i < list->data.list_val.count; i++)
                {
                    if (values_equal(list->data.list_val.elements[i], value))
                    {
                        value_free(list->data.list_val.elements[i]);
                        for (int j = i; j < list->data.list_val.count - 1; j++)
                        {
                            list->data.list_val.elements[j] = list->data.list_val.elements[j + 1];
                        }
                        list->data.list_val.count--;
                        break;
                    }
                }
            }
            return value_make_null();
        }

        if (strcmp(method_name, "reverse") == 0)
        {
            if (call->argument_count == 0 && list)
            {
                int count = list->data.list_val.count;
                for (int i = 0; i < count / 2; i++)
                {
                    Value temp = list->data.list_val.elements[i];
                    list->data.list_val.elements[i] = list->data.list_val.elements[count - i - 1];
                    list->data.list_val.elements[count - i - 1] = temp;
                }
            }
            return value_make_null();
        }

        if (strcmp(method_name, "sort") == 0)
        {
            if (call->argument_count == 0 && list)
            {
                for (int i = 1; i < list->data.list_val.count; i++)
                {
                    Value current = list->data.list_val.elements[i];
                    int j = i - 1;
                    while (j >= 0 && compare_values(list->data.list_val.elements[j], current) > 0)
                    {
                        list->data.list_val.elements[j + 1] = list->data.list_val.elements[j];
                        j--;
                    }
                    list->data.list_val.elements[j + 1] = current;
                }
            }
            return value_make_null();
        }

        if (strcmp(method_name, "index") == 0)
        {
            if (call->argument_count == 1)
            {
                Value value = eval_expression(call->arguments[0], env);
                for (int i = 0; i < obj.data.list_val.count; i++)
                {
                    if (values_equal(obj.data.list_val.elements[i], value))
                    {
                        return value_make_int(i);
                    }
                }
            }
            return value_make_int(-1);
        }

        fprintf(stderr, "Error: list has no method '%s'\n", method_name);
        return value_make_null();
    }

    if (obj.type == VALUE_STRING)
    {
        char *text = obj.data.string_val;

        if (strcmp(method_name, "upper") == 0)
        {
            if (call->argument_count != 0)
            {
                return value_make_null();
            }
            int len = strlen(text);
            char *result = malloc(len + 1);
            for (int i = 0; i < len; i++)
            {
                result[i] = toupper((unsigned char)text[i]);
            }
            result[len] = '\0';
            Value value = value_make_string(result);
            free(result);
            return value;
        }

        if (strcmp(method_name, "lower") == 0)
        {
            if (call->argument_count != 0)
            {
                return value_make_null();
            }
            int len = strlen(text);
            char *result = malloc(len + 1);
            for (int i = 0; i < len; i++)
            {
                result[i] = tolower((unsigned char)text[i]);
            }
            result[len] = '\0';
            Value value = value_make_string(result);
            free(result);
            return value;
        }

        if (strcmp(method_name, "find") == 0 || strcmp(method_name, "index") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_int(-1);
            }
            Value needle = eval_expression(call->arguments[0], env);
            if (needle.type != VALUE_STRING)
            {
                return value_make_int(-1);
            }
            char *found = strstr(text, needle.data.string_val);
            return value_make_int(found ? (int)(found - text) : -1);
        }

        if (strcmp(method_name, "substring") == 0)
        {
            if (call->argument_count != 2)
            {
                return value_make_null();
            }
            Value start_val = eval_expression(call->arguments[0], env);
            Value end_val = eval_expression(call->arguments[1], env);
            if (start_val.type != VALUE_INT || end_val.type != VALUE_INT)
            {
                return value_make_null();
            }
            int len = strlen(text);
            int start = start_val.data.int_val;
            int end = end_val.data.int_val;
            if (start < 0)
            {
                start = 0;
            }
            if (end > len)
            {
                end = len;
            }
            if (end < start)
            {
                end = start;
            }
            char *result = malloc(end - start + 1);
            memcpy(result, text + start, end - start);
            result[end - start] = '\0';
            Value value = value_make_string(result);
            free(result);
            return value;
        }

        if (strcmp(method_name, "replace") == 0)
        {
            if (call->argument_count != 2)
            {
                return value_make_null();
            }
            Value needle = eval_expression(call->arguments[0], env);
            Value replacement = eval_expression(call->arguments[1], env);
            if (needle.type != VALUE_STRING || replacement.type != VALUE_STRING)
            {
                return value_make_null();
            }
            char *result = replace_all_text(text, needle.data.string_val, replacement.data.string_val);
            Value value = value_make_string(result);
            free(result);
            return value;
        }

        if (strcmp(method_name, "substringCount") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_int(0);
            }
            Value needle = eval_expression(call->arguments[0], env);
            if (needle.type != VALUE_STRING)
            {
                return value_make_int(0);
            }
            return value_make_int(count_substrings(text, needle.data.string_val));
        }

        if (strcmp(method_name, "split") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value delimiter = eval_expression(call->arguments[0], env);
            if (delimiter.type != VALUE_STRING)
            {
                return value_make_null();
            }
            return split_string(text, delimiter.data.string_val);
        }

        if (strcmp(method_name, "join") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value parts = eval_expression(call->arguments[0], env);
            if (parts.type != VALUE_LIST)
            {
                return value_make_null();
            }

            char *result = malloc(2048);
            result[0] = '\0';
            for (int i = 0; i < parts.data.list_val.count; i++)
            {
                if (i > 0)
                {
                    strcat(result, text);
                }
                char *part = value_to_string(parts.data.list_val.elements[i]);
                strcat(result, part);
                free(part);
            }
            Value value = value_make_string(result);
            free(result);
            return value;
        }

        fprintf(stderr, "Error: string has no method '%s'\n", method_name);
        return value_make_null();
    }

    if (obj.type == VALUE_DICT)
    {
        Value *dict = NULL;
        if (member->object->type == AST_IDENTIFIER_TYPE)
        {
            dict = env_get_ref(env, member->object->data.identifier.value);
        }

        if (strcmp(method_name, "get") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value key = eval_expression(call->arguments[0], env);
            char *key_text = value_to_string(key);
            Value value = dict_get(obj, key_text);
            free(key_text);
            return value;
        }

        if (strcmp(method_name, "keys") == 0)
        {
            if (call->argument_count != 0)
            {
                return value_make_null();
            }
            Value result = list_create();
            for (int i = 0; i < obj.data.dict_val.count; i++)
            {
                list_append(&result, value_make_string(obj.data.dict_val.keys[i]));
            }
            return result;
        }

        if (strcmp(method_name, "values") == 0)
        {
            if (call->argument_count != 0)
            {
                return value_make_null();
            }
            Value result = list_create();
            for (int i = 0; i < obj.data.dict_val.count; i++)
            {
                list_append(&result, copy_value(obj.data.dict_val.values[i]));
            }
            return result;
        }

        if (strcmp(method_name, "items") == 0)
        {
            if (call->argument_count != 0)
            {
                return value_make_null();
            }
            Value result = list_create();
            for (int i = 0; i < obj.data.dict_val.count; i++)
            {
                Value pair = list_create();
                list_append(&pair, value_make_string(obj.data.dict_val.keys[i]));
                list_append(&pair, copy_value(obj.data.dict_val.values[i]));
                list_append(&result, pair);
            }
            return result;
        }

        if (strcmp(method_name, "update") == 0)
        {
            if (call->argument_count == 1 && dict)
            {
                Value other = eval_expression(call->arguments[0], env);
                if (other.type == VALUE_DICT)
                {
                    for (int i = 0; i < other.data.dict_val.count; i++)
                    {
                        dict_set(dict, other.data.dict_val.keys[i], copy_value(other.data.dict_val.values[i]));
                    }
                }
            }
            return value_make_null();
        }

        if (strcmp(method_name, "dumps") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value payload = eval_expression(call->arguments[0], env);
            return json_dumps_value(payload);
        }

        if (strcmp(method_name, "loads") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value payload = eval_expression(call->arguments[0], env);
            return json_loads_value(payload);
        }

        if (strcmp(method_name, "load") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value payload = eval_expression(call->arguments[0], env);
            return json_load_value(payload);
        }

        if (strcmp(method_name, "dump") == 0)
        {
            if (call->argument_count != 2)
            {
                return value_make_null();
            }
            Value file_path = eval_expression(call->arguments[0], env);
            Value payload = eval_expression(call->arguments[1], env);
            return json_dump_value(file_path, payload);
        }

        if (strcmp(method_name, "customEncode") == 0)
        {
            if (call->argument_count != 1)
            {
                return value_make_null();
            }
            Value payload = eval_expression(call->arguments[0], env);
            return json_custom_encode_value(payload);
        }

        if (strcmp(method_name, "customDecode") == 0)
        {
            if (call->argument_count != 2)
            {
                return value_make_null();
            }
            Value payload = eval_expression(call->arguments[0], env);
            Value type_name = eval_expression(call->arguments[1], env);
            return json_custom_decode_value(payload, type_name, env);
        }

        Value member_value = dict_get(obj, method_name);
        if (member_value.type == VALUE_BUILTIN)
        {
            Value *args = malloc(sizeof(Value) * call->argument_count);
            for (int i = 0; i < call->argument_count; i++)
            {
                args[i] = eval_expression(call->arguments[i], env);
            }
            Value result = member_value.data.builtin_fn(args, call->argument_count);
            free(args);
            return result;
        }
        if (member_value.type == VALUE_FUNCTION)
        {
            FunctionValue *func = &member_value.data.function_val;
            if (!func->body || func->body->type != AST_FUNCTION_DECLARATION_TYPE)
            {
                return value_make_null();
            }

            AST_FUNCTION_DECLARATION *func_decl = &func->body->data.function_declaration;
            Value *args = malloc(sizeof(Value) * call->argument_count);
            for (int i = 0; i < call->argument_count; i++)
            {
                args[i] = eval_expression(call->arguments[i], env);
            }

            env_push_scope(func->closure);
            for (int i = 0; i < func_decl->parameter_count && i < call->argument_count; i++)
            {
                ASTNode *param = func_decl->parameters[i];
                if (param->type == AST_VARIABLE_DECLARATION_TYPE)
                {
                    env_define(func->closure, param->data.variable_declaration.name, args[i]);
                }
            }
            for (int i = call->argument_count; i < func_decl->parameter_count; i++)
            {
                ASTNode *param = func_decl->parameters[i];
                if (param->type == AST_VARIABLE_DECLARATION_TYPE && param->data.variable_declaration.init_value)
                {
                    Value default_val = eval_expression(param->data.variable_declaration.init_value, func->closure);
                    env_define(func->closure, param->data.variable_declaration.name, default_val);
                }
            }

            return_flag.is_return = 0;
            ASTNode body_node;
            body_node.type = AST_BLOCK_TYPE;
            body_node.data.block = *func_decl->body;
            eval_statement(&body_node, func->closure);
            Value result = return_flag.return_value;
            return_flag.is_return = 0;
            env_pop_scope(func->closure);
            free(args);
            return result;
        }
        if (member_value.type != VALUE_NULL)
        {
            fprintf(stderr, "Error: module member '%s' is not callable\n", method_name);
            return value_make_null();
        }

        fprintf(stderr, "Error: dict has no method '%s'\n", method_name);
        return value_make_null();
    }

    if (obj.type != VALUE_CLASS_INSTANCE)
    {
        fprintf(stderr, "Error: cannot call method on non-class value\n");
        return value_make_null();
    }

    ClassInstanceValue *instance = obj.data.class_instance_val;

    ASTNode *method_node = NULL;
    AST_CLASS_DECLARATION *class_decl = instance->class_decl;

    for (int i = 0; i < class_decl->private_method_count; i++)
    {
        ASTNode *method = class_decl->private_methods[i];
        if (method->type == AST_FUNCTION_DECLARATION_TYPE)
        {
            AST_FUNCTION_DECLARATION *func_decl = &method->data.function_declaration;
            if (strcmp(func_decl->name, method_name) == 0)
            {
                method_node = method;
                break;
            }
        }
    }

    if (!method_node)
    {
        for (int i = 0; i < class_decl->public_method_count; i++)
        {
            ASTNode *method = class_decl->public_methods[i];
            if (method->type == AST_FUNCTION_DECLARATION_TYPE)
            {
                AST_FUNCTION_DECLARATION *func_decl = &method->data.function_declaration;
                if (strcmp(func_decl->name, method_name) == 0)
                {
                    method_node = method;
                    break;
                }
            }
        }
    }

    if (!method_node)
    {
        fprintf(stderr, "Error: class '%s' has no method '%s'\n",
                instance->class_name, method_name);
        return value_make_null();
    }

    AST_FUNCTION_DECLARATION *method_decl = &method_node->data.function_declaration;

    Value *args = malloc(sizeof(Value) * call->argument_count);
    for (int i = 0; i < call->argument_count; i++)
    {
        args[i] = eval_expression(call->arguments[i], env);
    }

    Environment *method_env = env_create();
    env_define(method_env, "this", obj);

    for (int i = 0; i < method_decl->parameter_count && i < call->argument_count; i++)
    {
        ASTNode *param = method_decl->parameters[i];
        if (param->type == AST_VARIABLE_DECLARATION_TYPE)
        {
            char *param_name = param->data.variable_declaration.name;
            env_define(method_env, param_name, args[i]);
        }
    }

    for (int i = call->argument_count; i < method_decl->parameter_count; i++)
    {
        ASTNode *param = method_decl->parameters[i];
        if (param->type == AST_VARIABLE_DECLARATION_TYPE)
        {
            char *param_name = param->data.variable_declaration.name;
            if (param->data.variable_declaration.init_value)
            {
                Value default_val = eval_expression(param->data.variable_declaration.init_value, method_env);
                env_define(method_env, param_name, default_val);
            }
        }
    }

    return_flag.is_return = 0;
    ASTNode body_node;
    body_node.type = AST_BLOCK_TYPE;
    body_node.data.block = *method_decl->body;
    eval_statement(&body_node, method_env);

    Value result = return_flag.return_value;
    return_flag.is_return = 0;

    env_free(method_env);
    free(args);

    return result;
}

void register_builtins(Environment *env)
{
    Value print_fn;
    print_fn.type = VALUE_BUILTIN;
    print_fn.data.builtin_fn = builtin_print;
    env_define(env, "print", print_fn);

    Value int_fn;
    int_fn.type = VALUE_BUILTIN;
    int_fn.data.builtin_fn = builtin_int;
    env_define(env, "int", int_fn);

    Value string_fn;
    string_fn.type = VALUE_BUILTIN;
    string_fn.data.builtin_fn = builtin_string;
    env_define(env, "string", string_fn);

    Value len_fn;
    len_fn.type = VALUE_BUILTIN;
    len_fn.data.builtin_fn = builtin_len;
    env_define(env, "len", len_fn);

    Value list_fn;
    list_fn.type = VALUE_BUILTIN;
    list_fn.data.builtin_fn = builtin_list;
    env_define(env, "list", list_fn);

    Value input_fn;
    input_fn.type = VALUE_BUILTIN;
    input_fn.data.builtin_fn = builtin_input;
    env_define(env, "input", input_fn);

    Value type_fn;
    type_fn.type = VALUE_BUILTIN;
    type_fn.data.builtin_fn = builtin_type;
    env_define(env, "type", type_fn);

    Value randint_fn;
    randint_fn.type = VALUE_BUILTIN;
    randint_fn.data.builtin_fn = builtin_randint;
    env_define(env, "randint", randint_fn);

    Value pow_fn;
    pow_fn.type = VALUE_BUILTIN;
    pow_fn.data.builtin_fn = builtin_pow;
    env_define(env, "pow", pow_fn);

    Value read_file_fn;
    read_file_fn.type = VALUE_BUILTIN;
    read_file_fn.data.builtin_fn = builtin_read_file;
    env_define(env, "read_file", read_file_fn);

    Value write_file_fn;
    write_file_fn.type = VALUE_BUILTIN;
    write_file_fn.data.builtin_fn = builtin_write_file;
    env_define(env, "write_file", write_file_fn);

    Value queue_create_fn;
    queue_create_fn.type = VALUE_BUILTIN;
    queue_create_fn.data.builtin_fn = builtin_queue_create;
    env_define(env, "queue_create", queue_create_fn);

    Value queue_add_fn;
    queue_add_fn.type = VALUE_BUILTIN;
    queue_add_fn.data.builtin_fn = builtin_queue_add;
    env_define(env, "queue_add", queue_add_fn);

    Value queue_remove_fn;
    queue_remove_fn.type = VALUE_BUILTIN;
    queue_remove_fn.data.builtin_fn = builtin_queue_remove;
    env_define(env, "queue_remove", queue_remove_fn);

    Value queue_peek_fn;
    queue_peek_fn.type = VALUE_BUILTIN;
    queue_peek_fn.data.builtin_fn = builtin_queue_peek;
    env_define(env, "queue_peek", queue_peek_fn);

    Value queue_is_empty_fn;
    queue_is_empty_fn.type = VALUE_BUILTIN;
    queue_is_empty_fn.data.builtin_fn = builtin_queue_is_empty;
    env_define(env, "queue_is_empty", queue_is_empty_fn);

    Value json_module = value_make_json_module();
    env_define(env, "json", json_module);

    Value sql_module = value_make_sql_module();
    env_define(env, "sql", sql_module);

    Value os_module = dict_create();

    Value os_getcwd_fn;
    os_getcwd_fn.type = VALUE_BUILTIN;
    os_getcwd_fn.data.builtin_fn = os_getcwd;
    dict_set(&os_module, "getcwd", os_getcwd_fn);

    Value os_chdir_fn;
    os_chdir_fn.type = VALUE_BUILTIN;
    os_chdir_fn.data.builtin_fn = os_chdir;
    dict_set(&os_module, "chdir", os_chdir_fn);

    Value os_listdir_fn;
    os_listdir_fn.type = VALUE_BUILTIN;
    os_listdir_fn.data.builtin_fn = os_listdir;
    dict_set(&os_module, "listdir", os_listdir_fn);

    Value os_exists_fn;
    os_exists_fn.type = VALUE_BUILTIN;
    os_exists_fn.data.builtin_fn = os_exists;
    dict_set(&os_module, "exists", os_exists_fn);

    Value os_join_fn;
    os_join_fn.type = VALUE_BUILTIN;
    os_join_fn.data.builtin_fn = os_join;
    dict_set(&os_module, "join", os_join_fn);

    Value os_exec_fn;
    os_exec_fn.type = VALUE_BUILTIN;
    os_exec_fn.data.builtin_fn = os_exec;
    dict_set(&os_module, "exec", os_exec_fn);

    Value os_environ_fn;
    os_environ_fn.type = VALUE_BUILTIN;
    os_environ_fn.data.builtin_fn = os_environ;
    dict_set(&os_module, "environ", os_environ_fn);

    env_define(env, "os", os_module);
}

Value eval_f_string(ASTNode *node, Environment *env)
{
    if (node->type != AST_F_STRING_TYPE)
    {
        return value_make_null();
    }

    char *result = malloc(2048);
    result[0] = '\0';

    for (int i = 0; i < node->data.f_string.part_count; i++)
    {
        F_STRING_PART *part = node->data.f_string.parts[i];

        if (part->is_expression)
        {
            Value expr_val = eval_expression(part->content.expression, env);
            char *expr_str = value_to_string(expr_val);
            strcat(result, expr_str);
            free(expr_str);
        }
        else
        {
            strcat(result, part->content.string_part);
        }
    }

    Value res = value_make_string(result);
    free(result);
    return res;
}
