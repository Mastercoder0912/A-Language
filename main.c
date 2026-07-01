#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "runtime.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: arun <file.a>\n");
        return 1;
    }
    else if (argc == 2){
        const char* filename = argv[1];
        FILE* file = fopen(filename, "r");
        if (!file) {
            fprintf(stderr, "Error: could not open file '%s'\n", filename);
            return 1;
        }

        if (fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            return 1;
        }

        long size = ftell(file);
        if (size < 0) {
            fclose(file);
            return 1;
        }

        rewind(file);

        char* code = malloc((size_t)size + 1);
        if (!code) {
            fclose(file);
            return 1;
        }

        if (fread(code, 1, (size_t)size, file) != (size_t)size) {
            free(code);
            fclose(file);
            return 1;
        }
        code[size] = '\0';
        fclose(file);

        Lexer* lexer = lexer_init(code);
        Parser* parser = parser_create(lexer);
        Program* ast = parse_program(parser);

        if (!ast) {
            fprintf(stderr, "Parse failed\n");
            parser_free(parser);
            free_lexer(lexer);
            free(code);
            return 1;
        }
    
        Environment* env = env_create();
        register_builtins(env);
        eval_program(ast, env);

        env_free(env);
        parser_free(parser);
        free_lexer(lexer);
        free(code);
    }

    return 0;
}
