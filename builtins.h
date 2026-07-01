#ifndef BUILTINS_H
#define BUILTINS_H

#include "runtime.h"

Value builtin_print(Value *args, int arg_count);
Value builtin_int(Value *args, int arg_count);
Value builtin_string(Value *args, int arg_count);
Value builtin_len(Value *args, int arg_count);
Value builtin_input(Value *args, int arg_count);
Value builtin_type(Value *args, int arg_count);
Value builtin_randint(Value *args, int arg_count);
Value builtin_pow(Value *args, int arg_count);
Value builtin_read_file(Value *args, int arg_count);
Value builtin_write_file(Value *args, int arg_count);

#endif