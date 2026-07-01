#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_UNKNOWN = 0,
    TOKEN_LEFT_PAREN = 1,
    TOKEN_RIGHT_PAREN = 2,
    TOKEN_LEFT_BRACE = 3,
    TOKEN_RIGHT_BRACE = 4,
    TOKEN_LEFT_BRACKET = 5,
    TOKEN_RIGHT_BRACKET = 6,
    TOKEN_SEMICOLON = 7,
    TOKEN_COMMA = 8,
    TOKEN_PLUS = 9,
    TOKEN_MINUS = 10,
    TOKEN_STAR = 11,
    TOKEN_SLASH = 12,
    TOKEN_ASSIGN = 13,
    TOKEN_IDENTIFIER = 14,
    TOKEN_NUMBER = 15,
    TOKEN_STRING = 16,
    TOKEN_LOOP = 17,
    TOKEN_IF = 18,
    TOKEN_ELSE = 19,
    TOKEN_RETURN = 20,
    TOKEN_VOID = 21,
    TOKEN_IMPORT = 22,
    TOKEN_AS = 23,
    TOKEN_NULL = 24,
    TOKEN_CHAR_LITERAL = 25,
    TOKEN_FUNCTION = 26,
    TOKEN_STRUCT = 27,
    TOKEN_CLASS = 28,
    TOKEN_TRUE = 29,
    TOKEN_FALSE = 30,
    TOKEN_LIST = 31,
    TOKEN_DICT = 32,
    TOKEN_PASS = 33,
    TOKEN_BOOLEAN = 34,
    TOKEN_CHAR = 35,
    TOKEN_STRING_TYPE = 36,
    TOKEN_ARRAY = 37,
    TOKEN_FLOAT = 38,
    TOKEN_INT = 39,
    TOKEN_BOOL = 40,
    TOKEN_CONST = 41,
    TOKEN_PRIVATE = 42,
    TOKEN_PUBLIC = 43,
    TOKEN_DOT = 44,
    TOKEN_COLON = 45,
    TOKEN_PERCENT = 46,
    TOKEN_AND = 47,
    TOKEN_ARROW = 48,
    TOKEN_OR = 49,
    TOKEN_EOF = 50,
    TOKEN_F_STRING = 51,
    TOKEN_LESS = 52,
    TOKEN_GREATER = 53,
    TOKEN_BANG = 54,
    TOKEN_INCREMENT = 107,
    TOKEN_DECREMENT = 108,
    TOKEN_PLUS_EQUAL = 109,
    TOKEN_MINUS_EQUAL = 110,
    TOKEN_NOT_EQUAL = 111,
    TOKEN_LESS_EQUAL = 112,
    TOKEN_EQUAL_EQUAL = 113,
    TOKEN_GREATER_EQUAL = 114,
    TOKEN_AND_AND = 115,
    TOKEN_OR_OR = 116
} TokenType;

typedef struct {
    int type;
    char* value;
    int line;
    int column;
} Token;

typedef struct {
    const char* source;
    int position;
    int line;
    int column;
} Lexer;

Lexer* lexer_init(const char* source);
Token next_token(Lexer* lexer);
void free_lexer(Lexer* lexer);
void token_print(Token token);

#endif
