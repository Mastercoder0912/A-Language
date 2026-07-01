#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

Value builtin_print(Value *args, int arg_count)
{
    for (int i = 0; i < arg_count; i++)
    {
        char *str = value_to_string(args[i]);
        printf("%s", str);
        free(str);
    }
    printf("\n");
    return value_make_null();
}

Value builtin_int(Value *args, int arg_count)
{
    if (arg_count != 1)
        return value_make_null();

    Value arg = args[0];

    if (arg.type == VALUE_INT)
    {
        return arg;
    }
    else if (arg.type == VALUE_STRING)
    {
        return value_make_int(atoi(arg.data.string_val));
    }
    else if (arg.type == VALUE_BOOL)
    {
        return value_make_int(arg.data.bool_val ? 1 : 0);
    }

    fprintf(stderr, "Error: cannot convert to int\n");
    return value_make_null();
}

Value builtin_string(Value *args, int arg_count)
{
    if (arg_count != 1)
        return value_make_null();

    char *str = value_to_string(args[0]);
    Value result = value_make_string(str);
    free(str);
    return result;
}

Value builtin_len(Value *args, int arg_count)
{
    if (arg_count != 1)
        return value_make_null();

    Value arg = args[0];

    if (arg.type == VALUE_STRING)
    {
        return value_make_int(strlen(arg.data.string_val));
    }
    else if (arg.type == VALUE_LIST)
    {
        return value_make_int(arg.data.list_val.count);
    }
    else if (arg.type == VALUE_DICT)
    {
        return value_make_int(arg.data.dict_val.count);
    }

    fprintf(stderr, "Error: cannot get length of this type\n");
    return value_make_null();
}

Value builtin_input(Value *args, int arg_count)
{
    if (arg_count > 1)
        return value_make_null();

    if (arg_count == 1)
    {
        value_print(args[0]);
    }

    char *buffer = malloc(256);
    fgets(buffer, 256, stdin);

    char *newline = strchr(buffer, '\n');
    if (newline)
    {
        *newline = '\0';
    }

    Value result = value_make_string(buffer);
    free(buffer);
    return result;
}

Value builtin_type(Value *args, int arg_count)
{
    if (arg_count != 1)
        return value_make_null();

    Value arg = args[0];

    switch (arg.type)
    {
    case VALUE_NULL:
        return value_make_string("null");
    case VALUE_INT:
        return value_make_string("int");
    case VALUE_STRING:
        return value_make_string("string");
    case VALUE_BOOL:
        return value_make_string("bool");
    case VALUE_LIST:
        return value_make_string("list");
    case VALUE_DICT:
        return value_make_string("dict");
    case VALUE_STRUCT:
        return value_make_string("struct");
    case VALUE_CLASS_INSTANCE:
        return value_make_string("class");
    case VALUE_FUNCTION:
        return value_make_string("function");
    case VALUE_BUILTIN:
        return value_make_string("builtin");
    default:
        return value_make_string("unknown");
    }
}

Value builtin_randint(Value *args, int arg_count)
{
    if (arg_count != 2)
        return value_make_null();

    if (args[0].type != VALUE_INT || args[1].type != VALUE_INT)
    {
        return value_make_null();
    }

    int min = args[0].data.int_val;
    int max = args[1].data.int_val;

    if (min > max)
    {
        int temp = min;
        min = max;
        max = temp;
    }

    int range = max - min + 1;
    return value_make_int(min + (rand() % range));
}

Value builtin_pow(Value *args, int arg_count)
{
    if (arg_count != 2)
        return value_make_null();
    if (args[0].type != VALUE_INT || args[1].type != VALUE_INT)
    {
        fprintf(stderr, "Error: pow() expects two integers\n");
        return value_make_null();
    }

    int base = args[0].data.int_val;
    int exponent = args[1].data.int_val;
    if (exponent < 0)
    {
        fprintf(stderr, "Error: pow() does not support negative exponents\n");
        return value_make_null();
    }

    long long result = 1;
    for (int i = 0; i < exponent; i++)
    {
        result *= base;
    }

    return value_make_int((int)result);
}

Value builtin_read_file(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: read_file() expects a single string path\n");
        return value_make_null();
    }

    const char *path = args[0].data.string_val;
    FILE *file = fopen(path, "r");
    if (!file)
    {
        fprintf(stderr, "Error: could not open file '%s'\n", path);
        return value_make_null();
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return value_make_null();
    }

    long size = ftell(file);
    if (size < 0)
    {
        fclose(file);
        return value_make_null();
    }
    rewind(file);

    char *buffer = malloc((size_t)size + 1);
    if (!buffer)
    {
        fclose(file);
        return value_make_null();
    }

    if (fread(buffer, 1, (size_t)size, file) != (size_t)size)
    {
        free(buffer);
        fclose(file);
        return value_make_null();
    }
    buffer[size] = '\0';
    fclose(file);

    Value result = value_make_string(buffer);
    free(buffer);
    return result;
}

Value builtin_write_file(Value *args, int arg_count)
{
    if (arg_count != 2 || args[0].type != VALUE_STRING || args[1].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: write_file() expects a path and a string\n");
        return value_make_null();
    }

    const char *path = args[0].data.string_val;
    const char *contents = args[1].data.string_val;
    FILE *file = fopen(path, "w");
    if (!file)
    {
        fprintf(stderr, "Error: could not open file '%s' for writing\n", path);
        return value_make_null();
    }

    fputs(contents, file);
    fclose(file);
    return value_make_null();
}