#include "runtime.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Value value_make_null()
{
    Value v;
    v.type = VALUE_NULL;
    return v;
}

Value value_make_int(int i)
{
    Value v;
    v.type = VALUE_INT;
    v.data.int_val = i;
    return v;
}

Value value_make_byte(int b)
{
    Value v;
    v.type = VALUE_BYTE;
    v.data.byte_val = (unsigned char)b;
    return v;
}

Value value_make_string(char *s)
{
    Value v;
    v.type = VALUE_STRING;
    v.data.string_val = malloc(strlen(s) + 1);
    strcpy(v.data.string_val, s);
    return v;
}

Value value_make_bool(bool b)
{
    Value v;
    v.type = VALUE_BOOL;
    v.data.bool_val = b ? 1 : 0;
    return v;
}

Value value_make_list()
{
    Value v;
    v.type = VALUE_LIST;
    v.data.list_val.elements = NULL;
    v.data.list_val.count = 0;
    v.data.list_val.capacity = 0;
    return v;
}

Value value_make_dict()
{
    Value v;
    v.type = VALUE_DICT;
    v.data.dict_val.keys = NULL;
    v.data.dict_val.values = NULL;
    v.data.dict_val.count = 0;
    v.data.dict_val.capacity = 0;
    return v;
}

Value value_make_queue(Queue *queue)
{
    Value v;
    v.type = VALUE_QUEUE;
    v.data.queue_val = queue;
    return v;
}

Value value_make_json_module()
{
    Value v;
    v.type = VALUE_JSON_MODULE;
    v.data.json_module_val = NULL;
    return v;
}

bool value_is_truthy(Value v)
{
    switch (v.type)
    {
    case VALUE_NULL:
        return false;
    case VALUE_BOOL:
        return v.data.bool_val != 0;
    case VALUE_INT:
        return v.data.int_val != 0;
    case VALUE_BYTE:
        return v.data.byte_val != 0;
    case VALUE_STRING:
        return strlen(v.data.string_val) > 0;
    default:
        return true;
    }
}

char *value_to_string(Value v)
{
    char *result = malloc(256);

    switch (v.type)
    {
    case VALUE_NULL:
        strcpy(result, "null");
        break;
    case VALUE_INT:
        sprintf(result, "%d", v.data.int_val);
        break;
    case VALUE_BYTE:
        sprintf(result, "%u", v.data.byte_val);
        break;
    case VALUE_STRING:
        strcpy(result, v.data.string_val);
        break;
    case VALUE_BOOL:
        strcpy(result, v.data.bool_val ? "True" : "False");
        break;
    case VALUE_LIST:
    {
        char *buf = malloc(1024);
        strcpy(buf, "[");

        for (int i = 0; i < v.data.list_val.count; i++)
        {
            if (i > 0)
                strcat(buf, ", ");
            char *elem_str = value_to_string(v.data.list_val.elements[i]);
            strcat(buf, elem_str);
            free(elem_str);
        }

        strcat(buf, "]");
        return buf;
    }
    case VALUE_DICT:
    {
        char *result = malloc(1024);
        strcpy(result, "{");

        for (int i = 0; i < v.data.dict_val.count; i++)
        {
            if (i > 0)
                strcat(result, ", ");
            strcat(result, "\"");
            strcat(result, v.data.dict_val.keys[i]);
            strcat(result, "\": ");
            char *val_str = value_to_string(v.data.dict_val.values[i]);
            strcat(result, val_str);
            free(val_str);
        }

        strcat(result, "}");
        return result;
    }
    case VALUE_QUEUE:
        strcpy(result, "[queue]");
        break;
    case VALUE_JSON_MODULE:
        strcpy(result, "[json]");
        break;

    default:
        strcpy(result, "[object]");
        break;
    }

    return result;
}

void value_free(Value v)
{
    switch (v.type)
    {
    case VALUE_STRING:
        free(v.data.string_val);
        break;
    case VALUE_LIST:
        for (int i = 0; i < v.data.list_val.count; i++)
        {
            value_free(v.data.list_val.elements[i]);
        }
        free(v.data.list_val.elements);
        break;
    case VALUE_DICT:
        for (int i = 0; i < v.data.dict_val.count; i++)
        {
            free(v.data.dict_val.keys[i]);
            value_free(v.data.dict_val.values[i]);
        }
        free(v.data.dict_val.keys);
        free(v.data.dict_val.values);
        break;
    case VALUE_QUEUE:
        queue_free(v.data.queue_val);
        break;
    default:
        break;
    }
}

void value_print(Value v)
{
    char *str = value_to_string(v);
    printf("%s", str);
    free(str);
}

Environment *env_create()
{
    Environment *env = malloc(sizeof(Environment));
    env->scopes = malloc(sizeof(SymbolTable) * 10);
    env->scope_count = 1;
    env->scope_capacity = 10;

    env->scopes->symbols = malloc(sizeof(Symbol) * 10);
    env->scopes->count = 0;
    env->scopes->capacity = 10;

    return env;
}

void env_push_scope(Environment *env)
{
    if (env->scope_count >= env->scope_capacity)
    {
        env->scope_capacity *= 2;
        env->scopes = realloc(env->scopes, sizeof(SymbolTable) * env->scope_capacity);
    }

    env->scopes[env->scope_count].symbols = malloc(sizeof(Symbol) * 10);
    env->scopes[env->scope_count].count = 0;
    env->scopes[env->scope_count].capacity = 10;
    env->scope_count++;
}

void env_pop_scope(Environment *env)
{
    if (env->scope_count <= 1)
        return;

    SymbolTable *scope = &env->scopes[env->scope_count - 1];
    for (int i = 0; i < scope->count; i++)
    {
        free(scope->symbols[i].name);
        value_free(scope->symbols[i].value);
    }
    free(scope->symbols);
    env->scope_count--;
}

void env_define(Environment *env, char *name, Value val)
{
    SymbolTable *scope = &env->scopes[env->scope_count - 1];

    for (int i = 0; i < scope->count; i++)
    {
        if (strcmp(scope->symbols[i].name, name) == 0)
        {
            value_free(scope->symbols[i].value);
            scope->symbols[i].value = val;
            return;
        }
    }

    if (scope->count >= scope->capacity)
    {
        scope->capacity *= 2;
        scope->symbols = realloc(scope->symbols, sizeof(Symbol) * scope->capacity);
    }

    scope->symbols[scope->count].name = malloc(strlen(name) + 1);
    strcpy(scope->symbols[scope->count].name, name);
    scope->symbols[scope->count].value = val;
    scope->count++;
}

Value env_get(Environment *env, char *name)
{
    for (int i = env->scope_count - 1; i >= 0; i--)
    {
        SymbolTable *scope = &env->scopes[i];
        for (int j = 0; j < scope->count; j++)
        {
            if (strcmp(scope->symbols[j].name, name) == 0)
            {
                return scope->symbols[j].value;
            }
        }
    }

    return value_make_null();
}

Value *env_get_ref(Environment *env, char *name)
{
    for (int i = env->scope_count - 1; i >= 0; i--)
    {
        SymbolTable *scope = &env->scopes[i];
        for (int j = 0; j < scope->count; j++)
        {
            if (strcmp(scope->symbols[j].name, name) == 0)
            {
                return &scope->symbols[j].value;
            }
        }
    }

    return NULL;
}

void env_set(Environment *env, char *name, Value val)
{
    for (int i = env->scope_count - 1; i >= 0; i--)
    {
        SymbolTable *scope = &env->scopes[i];
        for (int j = 0; j < scope->count; j++)
        {
            if (strcmp(scope->symbols[j].name, name) == 0)
            {
                value_free(scope->symbols[j].value);
                scope->symbols[j].value = val;
                return;
            }
        }
    }

    env_define(env, name, val);
}

bool env_exists(Environment *env, char *name)
{
    for (int i = env->scope_count - 1; i >= 0; i--)
    {
        SymbolTable *scope = &env->scopes[i];
        for (int j = 0; j < scope->count; j++)
        {
            if (strcmp(scope->symbols[j].name, name) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

void env_free(Environment *env)
{
    for (int i = 0; i < env->scope_count; i++)
    {
        SymbolTable *scope = &env->scopes[i];
        for (int j = 0; j < scope->count; j++)
        {
            free(scope->symbols[j].name);
            value_free(scope->symbols[j].value);
        }
        free(scope->symbols);
    }
    free(env->scopes);
    free(env);
}

Value list_create()
{
    Value v;
    v.type = VALUE_LIST;
    v.data.list_val.elements = malloc(sizeof(Value) * 10);
    v.data.list_val.count = 0;
    v.data.list_val.capacity = 10;
    return v;
}

void list_append(Value *list, Value value)
{
    if (list->type != VALUE_LIST)
        return;

    if (list->data.list_val.count >= list->data.list_val.capacity)
    {
        list->data.list_val.capacity *= 2;
        list->data.list_val.elements = realloc(list->data.list_val.elements,
                                               sizeof(Value) * list->data.list_val.capacity);
    }

    list->data.list_val.elements[list->data.list_val.count] = value;
    list->data.list_val.count++;
}

void list_fill(Value *list, Value value)
{
    if (list->type != VALUE_LIST)
        return;

    for (int i = 0; i < list->data.list_val.count; i++)
    {
        list->data.list_val.elements[i] = value;
    }
}

Value list_get(Value list, int index)
{
    if (list.type != VALUE_LIST)
        return value_make_null();

    if (index < 0 || index >= list.data.list_val.count)
    {
        fprintf(stderr, "Error: list index %d out of bounds\n", index);
        return value_make_null();
    }

    return list.data.list_val.elements[index];
}

void list_set(Value *list, int index, Value value)
{
    if (list->type != VALUE_LIST)
        return;

    if (index < 0 || index >= list->data.list_val.count)
    {
        fprintf(stderr, "Error: list index %d out of bounds\n", index);
        return;
    }

    list->data.list_val.elements[index] = value;
}

Value dict_create()
{
    Value v;
    v.type = VALUE_DICT;
    v.data.dict_val.keys = malloc(sizeof(char *) * 10);
    v.data.dict_val.values = malloc(sizeof(Value) * 10);
    v.data.dict_val.count = 0;
    v.data.dict_val.capacity = 10;
    return v;
}

void dict_set(Value *dict, char *key, Value value)
{
    if (dict->type != VALUE_DICT)
        return;

    for (int i = 0; i < dict->data.dict_val.count; i++)
    {
        if (strcmp(dict->data.dict_val.keys[i], key) == 0)
        {
            value_free(dict->data.dict_val.values[i]);
            dict->data.dict_val.values[i] = value;
            return;
        }
    }

    if (dict->data.dict_val.count >= dict->data.dict_val.capacity)
    {
        dict->data.dict_val.capacity *= 2;
        dict->data.dict_val.keys = realloc(dict->data.dict_val.keys,
                                           sizeof(char *) * dict->data.dict_val.capacity);
        dict->data.dict_val.values = realloc(dict->data.dict_val.values,
                                             sizeof(Value) * dict->data.dict_val.capacity);
    }

    int idx = dict->data.dict_val.count;
    dict->data.dict_val.keys[idx] = malloc(strlen(key) + 1);
    strcpy(dict->data.dict_val.keys[idx], key);
    dict->data.dict_val.values[idx] = value;
    dict->data.dict_val.count++;
}

Value dict_get(Value dict, char *key)
{
    if (dict.type != VALUE_DICT)
        return value_make_null();

    for (int i = 0; i < dict.data.dict_val.count; i++)
    {
        if (strcmp(dict.data.dict_val.keys[i], key) == 0)
        {
            return dict.data.dict_val.values[i];
        }
    }

    return value_make_null();
}

Value *dict_get_ref(Value *dict, char *key, bool create_if_missing)
{
    if (!dict || dict->type != VALUE_DICT)
        return NULL;

    for (int i = 0; i < dict->data.dict_val.count; i++)
    {
        if (strcmp(dict->data.dict_val.keys[i], key) == 0)
        {
            return &dict->data.dict_val.values[i];
        }
    }

    if (!create_if_missing)
    {
        return NULL;
    }

    if (dict->data.dict_val.count >= dict->data.dict_val.capacity)
    {
        dict->data.dict_val.capacity *= 2;
        dict->data.dict_val.keys = realloc(dict->data.dict_val.keys,
                                           sizeof(char *) * dict->data.dict_val.capacity);
        dict->data.dict_val.values = realloc(dict->data.dict_val.values,
                                             sizeof(Value) * dict->data.dict_val.capacity);
    }

    int idx = dict->data.dict_val.count++;
    dict->data.dict_val.keys[idx] = malloc(strlen(key) + 1);
    strcpy(dict->data.dict_val.keys[idx], key);
    dict->data.dict_val.values[idx] = value_make_null();
    return &dict->data.dict_val.values[idx];
}

Queue *queue_create(QueueMode mode)
{
    Queue *queue = malloc(sizeof(Queue));
    if (!queue)
    {
        return NULL;
    }

    queue->entries = NULL;
    queue->count = 0;
    queue->capacity = 0;
    queue->mode = mode;
    return queue;
}

void queue_free(Queue *queue)
{
    if (!queue)
    {
        return;
    }

    for (int i = 0; i < queue->count; i++)
    {
        value_free(queue->entries[i].value);
    }
    free(queue->entries);
    free(queue);
}

void queue_add(Queue *queue, Value value, int priority)
{
    if (!queue)
    {
        return;
    }

    if (queue->count >= queue->capacity)
    {
        queue->capacity = queue->capacity == 0 ? 4 : queue->capacity * 2;
        queue->entries = realloc(queue->entries, sizeof(QueueEntry) * queue->capacity);
    }

    if (queue->mode == QUEUE_MODE_FIFO)
    {
        queue->entries[queue->count].value = value;
        queue->entries[queue->count].priority = priority;
        queue->count++;
        return;
    }

    int insert_index = queue->count;
    for (int i = 0; i < queue->count; i++)
    {
        if (priority > queue->entries[i].priority)
        {
            insert_index = i;
            break;
        }
    }

    for (int i = queue->count; i > insert_index; i--)
    {
        queue->entries[i] = queue->entries[i - 1];
    }

    queue->entries[insert_index].value = value;
    queue->entries[insert_index].priority = priority;
    queue->count++;
}

Value queue_remove(Queue *queue)
{
    if (!queue || queue->count == 0)
    {
        return value_make_null();
    }

    int remove_index = 0;
    if (queue->mode == QUEUE_MODE_PRIORITY)
    {
        int highest_priority = queue->entries[0].priority;
        for (int i = 1; i < queue->count; i++)
        {
            if (queue->entries[i].priority > highest_priority)
            {
                highest_priority = queue->entries[i].priority;
                remove_index = i;
            }
        }
    }

    Value removed_value = queue->entries[remove_index].value;
    for (int i = remove_index; i < queue->count - 1; i++)
    {
        queue->entries[i] = queue->entries[i + 1];
    }
    queue->count--;
    return removed_value;
}

Value queue_peek(Queue *queue)
{
    if (!queue || queue->count == 0)
    {
        return value_make_null();
    }

    if (queue->mode == QUEUE_MODE_PRIORITY)
    {
        int highest_index = 0;
        int highest_priority = queue->entries[0].priority;
        for (int i = 1; i < queue->count; i++)
        {
            if (queue->entries[i].priority > highest_priority)
            {
                highest_priority = queue->entries[i].priority;
                highest_index = i;
            }
        }
        return queue->entries[highest_index].value;
    }

    return queue->entries[0].value;
}

int queue_is_empty(Queue *queue)
{
    if (!queue)
    {
        return 1;
    }

    return queue->count == 0;
}
