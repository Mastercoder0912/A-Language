#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct
{
    int active;
    char *name;
    char *expected;
    char *current;
    int seen_expected;
    int seen_current;
} TestHarnessState;

static TestHarnessState test_harness = {0, NULL, NULL, NULL, 0, 0};
static int test_total = 0;
static int test_correct = 0;

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

static void reset_test_harness(void)
{
    free(test_harness.name);
    free(test_harness.expected);
    free(test_harness.current);
    test_harness.name = NULL;
    test_harness.expected = NULL;
    test_harness.current = NULL;
    test_harness.active = 0;
    test_harness.seen_expected = 0;
    test_harness.seen_current = 0;
}

static void finalize_test_case(void)
{
    if (!test_harness.active)
    {
        return;
    }

    if (test_harness.seen_expected && test_harness.seen_current)
    {
        test_total++;
        if (test_harness.expected && test_harness.current && strcmp(test_harness.expected, test_harness.current) == 0)
        {
            test_correct++;
        }
        else
        {
            printf("FAIL: %s\n", test_harness.name ? test_harness.name : "unknown");
            if (test_harness.expected)
            {
                printf("expected: %s\n", test_harness.expected);
            }
            if (test_harness.current)
            {
                printf("current: %s\n", test_harness.current);
            }
        }
    }

    reset_test_harness();
}

static void begin_test_case(const char *name)
{
    finalize_test_case();
    test_harness.active = 1;
    test_harness.name = duplicate_string(name);
}

static void record_expected(const char *value)
{
    if (!test_harness.active)
    {
        return;
    }

    free(test_harness.expected);
    test_harness.expected = duplicate_string(value);
    test_harness.seen_expected = 1;
}

static void record_current(const char *value)
{
    if (!test_harness.active)
    {
        return;
    }

    free(test_harness.current);
    test_harness.current = duplicate_string(value);
    test_harness.seen_current = 1;

    test_total++;
    if (test_harness.expected && test_harness.current && strcmp(test_harness.expected, test_harness.current) == 0)
    {
        test_correct++;
    }
    else
    {
        printf("FAIL: %s\n", test_harness.name ? test_harness.name : "unknown");
        printf("expected: %s\n", test_harness.expected ? test_harness.expected : "");
        printf("current: %s\n", test_harness.current ? test_harness.current : "");
    }

    if (test_harness.name && strcmp(test_harness.name, "complete") == 0)
    {
        printf("Score: %d/%d correct\n", test_correct, test_total);
    }

    reset_test_harness();
}

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#define CHDIR _chdir
#define getcwd getcwd_win
#define getcwd_win _getcwd
#else
#include <unistd.h>
#include <dirent.h>
#define CHDIR chdir
#endif

Value builtin_print(Value *args, int arg_count)
{
    for (int i = 0; i < arg_count; i++)
    {
        char *str = value_to_string(args[i]);

        if (strncmp(str, "test:", 5) == 0)
        {
            begin_test_case(str + 5);
            free(str);
            continue;
        }

        if (test_harness.active)
        {
            if (strncmp(str, "expected:", 9) == 0)
            {
                record_expected(str + 9);
                free(str);
                continue;
            }
            if (strncmp(str, "current:", 8) == 0)
            {
                record_current(str + 8);
                free(str);
                continue;
            }
        }

        printf("%s", str);
        free(str);
    }

    if (!test_harness.active)
    {
        printf("\n");
    }
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

Value builtin_list(Value *args, int arg_count)
{
    if (arg_count == 0)
    {
        return list_create();
    }
    if (arg_count != 1)
    {
        return value_make_null();
    }

    Value arg = args[0];

    if (arg.type == VALUE_LIST)
    {
        Value result = list_create();
        for (int i = 0; i < arg.data.list_val.count; i++)
        {
            list_append(&result, arg.data.list_val.elements[i]);
        }
        return result;
    }
    if (arg.type == VALUE_STRING)
    {
        Value result = list_create();
        char tmp[2] = {'\0', '\0'};
        for (int i = 0; arg.data.string_val[i] != '\0'; i++)
        {
            tmp[0] = arg.data.string_val[i];
            list_append(&result, value_make_string(tmp));
        }
        return result;
    }
    if (arg.type == VALUE_DICT)
    {
        Value result = list_create();
        for (int i = 0; i < arg.data.dict_val.count; i++)
        {
            list_append(&result, value_make_string(arg.data.dict_val.keys[i]));
        }
        return result;
    }

    fprintf(stderr, "Error: cannot convert to list\n");
    return value_make_null();
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
    if (!fgets(buffer, 256, stdin))
    {
        fprintf(stderr, "Error: failed to read input\n");
        return value_make_null();
    }

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
    case VALUE_BYTE:
        return value_make_string("byte");
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

Value builtin_queue_create(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_INT)
    {
        fprintf(stderr, "Error: queue_create() expects one integer mode\n");
        return value_make_null();
    }

    QueueMode mode = args[0].data.int_val == 0 ? QUEUE_MODE_FIFO : QUEUE_MODE_PRIORITY;
    Queue *queue = queue_create(mode);
    return value_make_queue(queue);
}

Value builtin_queue_add(Value *args, int arg_count)
{
    if (arg_count != 3 || args[0].type != VALUE_QUEUE || args[2].type != VALUE_INT)
    {
        fprintf(stderr, "Error: queue_add() expects a queue, value, and priority\n");
        return value_make_null();
    }

    queue_add(args[0].data.queue_val, args[1], args[2].data.int_val);
    return value_make_null();
}

Value builtin_queue_remove(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_QUEUE)
    {
        fprintf(stderr, "Error: queue_remove() expects a queue\n");
        return value_make_null();
    }

    return queue_remove(args[0].data.queue_val);
}

Value builtin_queue_peek(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_QUEUE)
    {
        fprintf(stderr, "Error: queue_peek() expects a queue\n");
        return value_make_null();
    }

    return queue_peek(args[0].data.queue_val);
}

Value builtin_queue_is_empty(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_QUEUE)
    {
        fprintf(stderr, "Error: queue_is_empty() expects a queue\n");
        return value_make_null();
    }

    return value_make_bool(queue_is_empty(args[0].data.queue_val));
}

Value os_getcwd(Value *args, int arg_count)
{
    (void)args;
    (void)arg_count;
    char cwd[1024];
    char *result = getcwd(cwd, sizeof(cwd));
    if (result == NULL)
    {
        perror("getcwd() error");
        return value_make_null();
    }
    return value_make_string(cwd);
}

Value os_chdir(Value *args, int arg_count)
{
    (void)args;
    (void)arg_count;
    /* TODO: change the current working directory for cross platform compatibility */
    if (CHDIR(args[0].data.string_val) != 0)
    {
        perror("chdir() error");
        return value_make_null();
    }
    return value_make_null();
}

Value os_listdir(Value *args, int arg_count)
{
    (void)args;
    (void)arg_count;

    Value result = list_create();

    DIR *dir = opendir(".");
    if (dir == NULL)
    {
        perror("opendir");
        return result;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        Value filename = value_make_string(entry->d_name);

        list_append(&result, filename);
    }

    closedir(dir);

    return result;
}

Value os_exists(Value *args, int arg_count)
{
    (void)args;
    (void)arg_count;
    /* TODO: check if the given path exists */
    return value_make_bool(false);
}

Value os_join(Value *args, int arg_count)
{
    (void)args;
    (void)arg_count;
    /* TODO */
    return value_make_null();
}

Value os_exec(Value *args, int arg_count)
{
    (void)args;
    (void)arg_count;
    /* TODO */
    return value_make_null();
}

Value os_environ(Value *args, int arg_count)
{
    (void)args;
    (void)arg_count;
    /* TODO */
    return value_make_null();
}
