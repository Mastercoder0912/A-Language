#ifndef BUILTINS_H
#define BUILTINS_H

#include "runtime.h"

Value builtin_print(Value *args, int arg_count);
Value builtin_int(Value *args, int arg_count);
Value builtin_string(Value *args, int arg_count);
Value builtin_list(Value *args, int arg_count);
Value builtin_len(Value *args, int arg_count);
Value builtin_input(Value *args, int arg_count);
Value builtin_type(Value *args, int arg_count);
Value builtin_randint(Value *args, int arg_count);
Value builtin_pow(Value *args, int arg_count);
Value builtin_read_file(Value *args, int arg_count);
Value builtin_write_file(Value *args, int arg_count);
Value builtin_queue_create(Value *args, int arg_count);
Value builtin_queue_add(Value *args, int arg_count);
Value builtin_queue_remove(Value *args, int arg_count);
Value builtin_queue_peek(Value *args, int arg_count);
Value builtin_queue_is_empty(Value *args, int arg_count);

Value os_getcwd(Value *args, int arg_count);
Value os_chdir(Value *args, int arg_count);
Value os_listdir(Value *args, int arg_count);
Value os_exists(Value *args, int arg_count);
Value os_join(Value *args, int arg_count);
Value os_exec(Value *args, int arg_count);
Value os_environ(Value *args, int arg_count);

#endif
