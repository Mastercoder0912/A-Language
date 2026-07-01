#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"


typedef struct {
    Lexer* lexer;
    Token current_token;
    Token peek_token;
} Parser;

Parser* parser_create(Lexer* lexer);
void parser_free(Parser* parser);
Program* parse_program(Parser* parser);
Program* parser_parse_source(const char* source);
ASTNode* parse_f_string(Parser* parser, char* f_string_value);
void print_ast(ASTNode* node, int indent);

#endif
