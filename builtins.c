#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

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

#if !defined(_WIN32) && !defined(_WIN64)
extern FILE *popen(const char *command, const char *type);
extern int pclose(FILE *stream);
#endif

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
            printf("%s failed\n", test_harness.name ? test_harness.name : "unknown");
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

static const char *test_value_text(const char *text)
{
    while (*text == ' ')
    {
        text++;
    }
    return text;
}

static void begin_test_case(const char *name)
{
    finalize_test_case();
    test_harness.active = 1;
    test_harness.name = duplicate_string(test_value_text(name));
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
        printf("%s failed\n", test_harness.name ? test_harness.name : "unknown");
        printf("expected: %s\n", test_harness.expected ? test_harness.expected : "");
        printf("current: %s\n", test_harness.current ? test_harness.current : "");
    }

    if (test_harness.name && strcmp(test_harness.name, "complete") == 0)
    {
        printf("Score: %d/%d correct\n", test_correct, test_total);
    }

    reset_test_harness();
}

static void append_text(char **buffer, size_t *length, size_t *capacity, const char *text)
{
    if (!buffer || !length || !capacity || !text)
    {
        return;
    }

    size_t text_len = strlen(text);
    size_t needed = *length + text_len + 1;
    if (needed > *capacity)
    {
        size_t next_capacity = *capacity;
        while (needed > next_capacity)
        {
            next_capacity *= 2;
        }
        char *next = realloc(*buffer, next_capacity);
        if (!next)
        {
            return;
        }
        *buffer = next;
        *capacity = next_capacity;
    }

    memcpy(*buffer + *length, text, text_len);
    *length += text_len;
    (*buffer)[*length] = '\0';
}

static char *shell_quote(const char *text)
{
    if (!text)
    {
        return duplicate_string("''");
    }

    size_t len = strlen(text);
    size_t max_len = len * 4 + 3;
    char *quoted = malloc(max_len);
    if (!quoted)
    {
        return NULL;
    }

    size_t w = 0;
    quoted[w++] = '\'';
    for (size_t i = 0; i < len; i++)
    {
        if (text[i] == '\'')
        {
            quoted[w++] = '\'';
            quoted[w++] = '\\';
            quoted[w++] = '\'';
            quoted[w++] = '\'';
        }
        else
        {
            quoted[w++] = text[i];
        }
    }
    quoted[w++] = '\'';
    quoted[w] = '\0';
    return quoted;
}

static Value make_command_result(int status, const char *output, const char *error)
{
    Value result = dict_create();
    dict_set(&result, "ok", value_make_bool(status == 0));
    dict_set(&result, "status", value_make_int(status));
    dict_set(&result, "output", value_make_string((char *)(output ? output : "")));
    if (error)
    {
        dict_set(&result, "error", value_make_string((char *)error));
    }
    else
    {
        dict_set(&result, "error", value_make_null());
    }
    return result;
}

static int process_exit_code(int raw_status)
{
#if defined(_WIN32) || defined(_WIN64)
    return raw_status;
#else
    if (raw_status < 0)
    {
        return -1;
    }
    return (raw_status >> 8) & 0xFF;
#endif
}

static const char *python_candidates[] = {"python3", "python", NULL};

static int command_works(const char *command)
{
    if (!command)
    {
        return 0;
    }

    char command_line[256];
#if defined(_WIN32) || defined(_WIN64)
    snprintf(command_line, sizeof(command_line), "%s --version > NUL 2>&1", command);
#else
    snprintf(command_line, sizeof(command_line), "%s --version > /dev/null 2>&1", command);
#endif
    int result = system(command_line);
    return process_exit_code(result) == 0;
}

static char *find_python_command(void)
{
    const char *env_python = getenv("A_LANG_PYTHON");
    if (env_python && env_python[0] != '\0')
    {
        if (command_works(env_python))
        {
            return duplicate_string(env_python);
        }
        return NULL;
    }

    for (int i = 0; python_candidates[i] != NULL; i++)
    {
        if (command_works(python_candidates[i]))
        {
            return duplicate_string(python_candidates[i]);
        }
    }
    return NULL;
}

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#include <io.h>
#include <windows.h>
#define CHDIR _chdir
#define GETCWD _getcwd
#define PATH_SEPARATOR '\\'
#define ACCESS _access
#define MKDIR(path) _mkdir(path)
#define RMDIR(path) _rmdir(path)
#define POPEN _popen
#define PCLOSE _pclose
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#define CHDIR chdir
#define GETCWD getcwd
#define PATH_SEPARATOR '/'
#define ACCESS access
#define MKDIR(path) mkdir(path, 0777)
#define RMDIR(path) rmdir(path)
#define POPEN popen
#define PCLOSE pclose
extern char **environ;
#endif

Value builtin_print(Value *args, int arg_count)
{
    int printed_normal = 0;
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
                record_expected(test_value_text(str + 9));
                free(str);
                continue;
            }
            if (strncmp(str, "current:", 8) == 0)
            {
                record_current(test_value_text(str + 8));
                free(str);
                continue;
            }
        }

        printf("%s", str);
        printed_normal = 1;
        free(str);
    }

    if (printed_normal)
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
    case VALUE_QUEUE:
        return value_make_string("queue");
    case VALUE_JSON_MODULE:
        return value_make_string("json");
    case VALUE_SQL_MODULE:
        return value_make_string("sql");
    case VALUE_SQL_DB:
        return value_make_string("database");
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
#if defined(_WIN32) || defined(_WIN64)
    char cwd[MAX_PATH];
#else
    char cwd[1024];
#endif
    (void)args;
    (void)arg_count;
    char *result = GETCWD(cwd, sizeof(cwd));
    if (result == NULL)
    {
        perror("getcwd() error");
        return value_make_null();
    }
    return value_make_string(cwd);
}

Value os_chdir(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: os.chdir() expects one path string\n");
        return value_make_null();
    }

    if (CHDIR(args[0].data.string_val) != 0)
    {
        perror("chdir() error");
        return value_make_null();
    }
    return value_make_null();
}

Value os_listdir(Value *args, int arg_count)
{
    Value result = list_create();
    const char *path = ".";
    if (arg_count == 1 && args[0].type == VALUE_STRING)
    {
        path = args[0].data.string_val;
    }
    else if (arg_count != 0)
    {
        fprintf(stderr, "Error: os.listdir() expects zero args or one path string\n");
        return result;
    }

#if defined(_WIN32) || defined(_WIN64)
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    WIN32_FIND_DATAA file_data;
    HANDLE handle = FindFirstFileA(search_path, &file_data);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return result;
    }

    do
    {
        if (strcmp(file_data.cFileName, ".") == 0 || strcmp(file_data.cFileName, "..") == 0)
        {
            continue;
        }
        list_append(&result, value_make_string(file_data.cFileName));
    } while (FindNextFileA(handle, &file_data) != 0);

    FindClose(handle);
#else
    DIR *dir = opendir(path);
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
#endif

    return result;
}

Value os_exists(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: os.exists() expects one path string\n");
        return value_make_bool(false);
    }

    return value_make_bool(ACCESS(args[0].data.string_val, 0) == 0);
}

Value os_join(Value *args, int arg_count)
{
    if (arg_count < 2)
    {
        fprintf(stderr, "Error: os.join() expects at least two path strings\n");
        return value_make_null();
    }

    size_t total = 1;
    for (int i = 0; i < arg_count; i++)
    {
        if (args[i].type != VALUE_STRING)
        {
            fprintf(stderr, "Error: os.join() expects string arguments\n");
            return value_make_null();
        }
        total += strlen(args[i].data.string_val) + 1;
    }

    char *result = malloc(total);
    if (!result)
    {
        return value_make_null();
    }
    result[0] = '\0';

    for (int i = 0; i < arg_count; i++)
    {
        const char *part = args[i].data.string_val;
        if (i == 0)
        {
            strcpy(result, part);
            continue;
        }

        size_t len = strlen(result);
        int result_ends_sep = (len > 0 && (result[len - 1] == '/' || result[len - 1] == '\\'));
        while (*part == '/' || *part == '\\')
        {
            part++;
        }

        if (!result_ends_sep)
        {
            size_t offset = strlen(result);
            result[offset] = PATH_SEPARATOR;
            result[offset + 1] = '\0';
        }
        strcat(result, part);
    }

    Value out = value_make_string(result);
    free(result);
    return out;
}

Value os_exec(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: os.exec() expects one command string\n");
        return value_make_null();
    }

    int rc = system(args[0].data.string_val);
    return value_make_int(rc);
}

Value os_environ(Value *args, int arg_count)
{
    (void)args;
    if (arg_count != 0)
    {
        fprintf(stderr, "Error: os.environ() expects no arguments\n");
        return value_make_null();
    }

    Value env_map = dict_create();
#if defined(_WIN32) || defined(_WIN64)
    LPCH env_strings = GetEnvironmentStringsA();
    if (!env_strings)
    {
        return env_map;
    }

    for (LPCH current = env_strings; *current != '\0'; current += strlen(current) + 1)
    {
        char *equal = strchr(current, '=');
        if (!equal || equal == current)
        {
            continue;
        }
        size_t key_len = (size_t)(equal - current);
        char *key = malloc(key_len + 1);
        strncpy(key, current, key_len);
        key[key_len] = '\0';
        dict_set(&env_map, key, value_make_string(equal + 1));
        free(key);
    }
    FreeEnvironmentStringsA(env_strings);
#else
    for (char **entry = environ; entry && *entry; entry++)
    {
        char *equal = strchr(*entry, '=');
        if (!equal || equal == *entry)
        {
            continue;
        }

        size_t key_len = (size_t)(equal - *entry);
        char *key = malloc(key_len + 1);
        strncpy(key, *entry, key_len);
        key[key_len] = '\0';
        dict_set(&env_map, key, value_make_string(equal + 1));
        free(key);
    }
#endif
    return env_map;
}

Value os_pwd(Value *args, int arg_count)
{
    if (arg_count != 0)
    {
        fprintf(stderr, "Error: os.pwd() expects no arguments\n");
        return value_make_null();
    }
    return os_getcwd(args, arg_count);
}

Value os_ls(Value *args, int arg_count)
{
    if (!(arg_count == 0 || (arg_count == 1 && args[0].type == VALUE_STRING)))
    {
        fprintf(stderr, "Error: os.ls() expects zero args or one path string\n");
        return list_create();
    }
    return os_listdir(args, arg_count);
}

Value os_cd(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: os.cd() expects one path string\n");
        return value_make_null();
    }
    return os_chdir(args, arg_count);
}

Value os_mkdir(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: os.mkdir() expects one path string\n");
        return value_make_null();
    }

    if (MKDIR(args[0].data.string_val) != 0)
    {
        perror("os.mkdir() error");
        return value_make_null();
    }

    return value_make_bool(true);
}

Value os_touch(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: os.touch() expects one path string\n");
        return value_make_null();
    }

    FILE *file = fopen(args[0].data.string_val, "ab");
    if (!file)
    {
        perror("os.touch() error");
        return value_make_null();
    }
    fclose(file);
    return value_make_bool(true);
}

static int copy_file_binary(const char *source_path, const char *dest_path)
{
    FILE *src = fopen(source_path, "rb");
    if (!src)
    {
        return 0;
    }

    FILE *dest = fopen(dest_path, "wb");
    if (!dest)
    {
        fclose(src);
        return 0;
    }

    char buffer[4096];
    size_t read_size = 0;
    while ((read_size = fread(buffer, 1, sizeof(buffer), src)) > 0)
    {
        if (fwrite(buffer, 1, read_size, dest) != read_size)
        {
            fclose(src);
            fclose(dest);
            return 0;
        }
    }

    fclose(src);
    fclose(dest);
    return 1;
}

Value os_cp(Value *args, int arg_count)
{
    if (arg_count != 2 || args[0].type != VALUE_STRING || args[1].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: os.cp() expects source and destination path strings\n");
        return value_make_null();
    }

    if (!copy_file_binary(args[0].data.string_val, args[1].data.string_val))
    {
        perror("os.cp() error");
        return value_make_null();
    }

    return value_make_bool(true);
}

Value os_mv(Value *args, int arg_count)
{
    if (arg_count != 2 || args[0].type != VALUE_STRING || args[1].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: os.mv() expects source and destination path strings\n");
        return value_make_null();
    }

    if (rename(args[0].data.string_val, args[1].data.string_val) != 0)
    {
        perror("os.mv() error");
        return value_make_null();
    }

    return value_make_bool(true);
}

Value os_rm(Value *args, int arg_count)
{
    if (arg_count != 1 || args[0].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: os.rm() expects one path string\n");
        return value_make_null();
    }

    const char *path = args[0].data.string_val;
    if (remove(path) == 0)
    {
        return value_make_bool(true);
    }

    if (RMDIR(path) == 0)
    {
        return value_make_bool(true);
    }

    perror("os.rm() error");
    return value_make_null();
}

Value os_echo(Value *args, int arg_count)
{
    if (arg_count < 1)
    {
        fprintf(stderr, "Error: os.echo() expects at least one argument\n");
        return value_make_null();
    }

    size_t capacity = 64;
    size_t length = 0;
    char *joined = malloc(capacity);
    if (!joined)
    {
        return value_make_null();
    }
    joined[0] = '\0';

    for (int i = 0; i < arg_count; i++)
    {
        if (i > 0)
        {
            append_text(&joined, &length, &capacity, " ");
        }
        char *text = value_to_string(args[i]);
        append_text(&joined, &length, &capacity, text);
        free(text);
    }

    printf("%s\n", joined);
    Value result = value_make_string(joined);
    free(joined);
    return result;
}

Value python_run(Value *args, int arg_count)
{
    if (arg_count < 1 || arg_count > 2 || args[0].type != VALUE_STRING)
    {
        fprintf(stderr, "Error: python.run() expects a script path and optional argument list\n");
        return make_command_result(-1, "", "invalid arguments");
    }

    const char *script_path = args[0].data.string_val;
    FILE *script_file = fopen(script_path, "r");
    if (!script_file)
    {
        return make_command_result(-1, "", "script file not found");
    }
    fclose(script_file);

    if (arg_count == 2 && args[1].type != VALUE_LIST)
    {
        fprintf(stderr, "Error: python.run() optional second argument must be a list\n");
        return make_command_result(-1, "", "python arguments must be a list");
    }

    char *python_cmd = find_python_command();
    if (!python_cmd)
    {
        return make_command_result(-1, "", "python interpreter not found");
    }

    char *quoted_python = shell_quote(python_cmd);
    char *quoted_script = shell_quote(script_path);
    if (!quoted_python || !quoted_script)
    {
        free(python_cmd);
        free(quoted_python);
        free(quoted_script);
        return make_command_result(-1, "", "failed to prepare python command");
    }

    size_t capacity = 1024;
    size_t length = 0;
    char *command = malloc(capacity);
    if (!command)
    {
        free(python_cmd);
        free(quoted_python);
        free(quoted_script);
        return make_command_result(-1, "", "failed to allocate command buffer");
    }
    command[0] = '\0';

    append_text(&command, &length, &capacity, quoted_python);
    append_text(&command, &length, &capacity, " ");
    append_text(&command, &length, &capacity, quoted_script);

    if (arg_count == 2)
    {
        for (int i = 0; i < args[1].data.list_val.count; i++)
        {
            char *arg_text = value_to_string(args[1].data.list_val.elements[i]);
            char *quoted_arg = shell_quote(arg_text);
            free(arg_text);
            if (!quoted_arg)
            {
                free(command);
                free(python_cmd);
                free(quoted_python);
                free(quoted_script);
                return make_command_result(-1, "", "failed to prepare python arguments");
            }
            append_text(&command, &length, &capacity, " ");
            append_text(&command, &length, &capacity, quoted_arg);
            free(quoted_arg);
        }
    }

    append_text(&command, &length, &capacity, " 2>&1");

    FILE *pipe = POPEN(command, "r");
    if (!pipe)
    {
        free(command);
        free(python_cmd);
        free(quoted_python);
        free(quoted_script);
        return make_command_result(-1, "", "failed to execute python process");
    }

    size_t output_capacity = 1024;
    size_t output_length = 0;
    char *output = malloc(output_capacity);
    if (!output)
    {
        PCLOSE(pipe);
        free(command);
        free(python_cmd);
        free(quoted_python);
        free(quoted_script);
        return make_command_result(-1, "", "failed to allocate output buffer");
    }
    output[0] = '\0';

    char chunk[512];
    while (fgets(chunk, sizeof(chunk), pipe))
    {
        append_text(&output, &output_length, &output_capacity, chunk);
    }

    int exit_code = process_exit_code(PCLOSE(pipe));
    Value result = make_command_result(exit_code, output, exit_code == 0 ? NULL : "python process exited with non-zero status");

    free(output);
    free(command);
    free(python_cmd);
    free(quoted_python);
    free(quoted_script);
    return result;
}
