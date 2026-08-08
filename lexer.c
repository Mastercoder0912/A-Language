#include "lexer.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

static int is_identifier_char(char c)
{
    return isalnum(c) || c == '_';
}

static int is_identifier_boundary(char c)
{
    return !is_identifier_char(c);
}

static int read_identifier_length(const char *source, int start_pos)
{
    int pos = start_pos;
    while (is_identifier_char(source[pos]))
    {
        pos++;
    }
    return pos - start_pos;
}

static int keyword_match(const char *source, int pos, const char *keyword, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (source[pos + i] != keyword[i])
        {
            return 0;
        }
    }
    return is_identifier_boundary(source[pos + length]);
}

Lexer *lexer_init(const char *source)
{
    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    return lexer;
}

Token next_token(Lexer *lexer)
{
    while (lexer->source[lexer->position] != '\0')
    {
        char current_char = lexer->source[lexer->position];

        if (current_char == ' ' || current_char == '\t' || current_char == '\r')
        {
            lexer->position++;
            lexer->column++;
            continue;
        }
        else if (current_char == '\n')
        {
            lexer->position++;
            lexer->line++;
            lexer->column = 1;
            continue;
        }

        Token token;
        token.line = lexer->line;
        token.column = lexer->column;
        token.value = NULL;

        if (current_char == '#')
        {
            lexer->position++;
            lexer->column++;
            while (lexer->source[lexer->position] != '\n' && lexer->source[lexer->position] != '\0')
            {
                lexer->position++;
                lexer->column++;
            }
            continue;
        }

        if (current_char == '/' && lexer->source[lexer->position + 1] == '/')
        {
            lexer->position += 2;
            lexer->column += 2;
            while (lexer->source[lexer->position] != '\n' && lexer->source[lexer->position] != '\0')
            {
                lexer->position++;
                lexer->column++;
            }
            continue;
        }

        if (current_char == '/' && lexer->source[lexer->position + 1] == '*')
        {
            lexer->position += 2;
            lexer->column += 2;
            while (lexer->source[lexer->position] != '\0' && !(lexer->source[lexer->position] == '*' && lexer->source[lexer->position + 1] == '/'))
            {
                if (lexer->source[lexer->position] == '\n')
                {
                    lexer->line++;
                    lexer->column = 1;
                }
                else
                {
                    lexer->column++;
                }
                lexer->position++;
            }
            if (lexer->source[lexer->position] != '\0')
            {
                lexer->position += 2;
                lexer->column += 2;
            }
            continue;
        }

        if (current_char == '"')
        {
            token.type = 16;
            lexer->position++;
            lexer->column++;
            int start_position = lexer->position;

            while (lexer->source[lexer->position] != '"' && lexer->source[lexer->position] != '\0')
            {
                if (lexer->source[lexer->position] == '\n')
                {
                    lexer->line++;
                    lexer->column = 1;
                }
                else
                {
                    lexer->column++;
                }
                lexer->position++;
            }

            int length = lexer->position - start_position;
            token.value = (char *)malloc(length + 1);
            for (int i = 0; i < length; i++)
            {
                token.value[i] = lexer->source[start_position + i];
            }
            token.value[length] = '\0';

            if (lexer->source[lexer->position] == '"')
            {
                lexer->position++;
                lexer->column++;
            }
            return token;
        }

        // Handle character literals
        if (current_char == '\'')
        {
            token.type = 25;
            lexer->position++;
            lexer->column++;
            int start_position = lexer->position;

            while (lexer->source[lexer->position] != '\'' && lexer->source[lexer->position] != '\0')
            {
                lexer->position++;
                lexer->column++;
            }

            int length = lexer->position - start_position;
            token.value = (char *)malloc(length + 1);
            for (int i = 0; i < length; i++)
            {
                token.value[i] = lexer->source[start_position + i];
            }
            token.value[length] = '\0';

            if (lexer->source[lexer->position] == '\'')
            {
                lexer->position++;
                lexer->column++;
            }
            return token;
        }

        if (isalpha(current_char) || current_char == '_')
        {
            int id_length = read_identifier_length(lexer->source, lexer->position);

            token.value = (char *)malloc(id_length + 1);
            for (int i = 0; i < id_length; i++)
            {
                token.value[i] = lexer->source[lexer->position + i];
            }
            token.value[id_length] = '\0';

            if (keyword_match(lexer->source, lexer->position, "loop", 4))
                token.type = 17;
            else if (keyword_match(lexer->source, lexer->position, "import", 6))
                token.type = 22;
            else if (keyword_match(lexer->source, lexer->position, "as", 2))
                token.type = 23;
            else if (keyword_match(lexer->source, lexer->position, "null", 4))
                token.type = 24;
            else if (keyword_match(lexer->source, lexer->position, "if", 2))
                token.type = 18;
            else if (keyword_match(lexer->source, lexer->position, "else", 4))
                token.type = 19;
            else if (keyword_match(lexer->source, lexer->position, "return", 6))
                token.type = 20;
            else if (keyword_match(lexer->source, lexer->position, "void", 4))
                token.type = 21;
            else if (keyword_match(lexer->source, lexer->position, "function", 8))
                token.type = 26;
            else if (keyword_match(lexer->source, lexer->position, "struct", 6))
                token.type = 27;
            else if (keyword_match(lexer->source, lexer->position, "class", 5))
                token.type = 28;
            else if (keyword_match(lexer->source, lexer->position, "True", 4))
                token.type = 29;
            else if (keyword_match(lexer->source, lexer->position, "False", 5))
                token.type = 30;
            else if (keyword_match(lexer->source, lexer->position, "list", 4))
                token.type = 31;
            else if (keyword_match(lexer->source, lexer->position, "dict", 4))
                token.type = 32;
            else if (keyword_match(lexer->source, lexer->position, "pass", 4))
                token.type = 33;
            else if (keyword_match(lexer->source, lexer->position, "boolean", 7))
                token.type = 34;
            else if (keyword_match(lexer->source, lexer->position, "char", 4))
                token.type = 35;
            else if (keyword_match(lexer->source, lexer->position, "string", 6))
                token.type = 36;
            else if (keyword_match(lexer->source, lexer->position, "array", 5))
                token.type = 37;
            else if (keyword_match(lexer->source, lexer->position, "float", 5))
                token.type = 38;
            else if (keyword_match(lexer->source, lexer->position, "int", 3))
                token.type = 39;
            else if (keyword_match(lexer->source, lexer->position, "bool", 4))
                token.type = 40;
            else if (keyword_match(lexer->source, lexer->position, "const", 5))
                token.type = 41;
            else if (keyword_match(lexer->source, lexer->position, "private", 7))
                token.type = 42;
            else if (keyword_match(lexer->source, lexer->position, "public", 6))
                token.type = 43;
            else if (keyword_match(lexer->source, lexer->position, "and", 3))
                token.type = 47;
            else if (keyword_match(lexer->source, lexer->position, "or", 2))
                token.type = 49;
            else
                token.type = 14;

            lexer->position += id_length;
            lexer->column += id_length;
            return token;
        }

        if (isdigit(current_char))
        {
            token.type = 15;
            int start_position = lexer->position;
            int has_decimal_point = 0;

            while (isdigit(lexer->source[lexer->position]) || lexer->source[lexer->position] == '.')
            {
                if (lexer->source[lexer->position] == '.')
                {
                    if (has_decimal_point)
                    {
                        break;
                    }
                    has_decimal_point = 1;
                }
                lexer->position++;
                lexer->column++;
            }

            int length = lexer->position - start_position;
            token.value = (char *)malloc(length + 1);
            for (int i = 0; i < length; i++)
            {
                token.value[i] = lexer->source[start_position + i];
            }
            token.value[length] = '\0';
            return token;
        }

        if (current_char == '+' && lexer->source[lexer->position + 1] == '+')
        {
            token.type = 7 + 100;
            token.value = (char *)malloc(3);
            strcpy(token.value, "++");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }
        if (current_char == '-' && lexer->source[lexer->position + 1] == '-')
        {
            token.type = 108;
            token.value = (char *)malloc(3);
            strcpy(token.value, "--");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }
        if (current_char == '+' && lexer->source[lexer->position + 1] == '=')
        {
            token.type = 9 + 100;
            token.value = (char *)malloc(3);
            strcpy(token.value, "+=");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }
        if (current_char == '-' && lexer->source[lexer->position + 1] == '=')
        {
            token.type = 10 + 100;
            token.value = (char *)malloc(3);
            strcpy(token.value, "-=");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }
        if (current_char == '=' && lexer->source[lexer->position + 1] == '=')
        {
            token.type = 13 + 100;
            token.value = (char *)malloc(3);
            strcpy(token.value, "==");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }
        if (current_char == '!' && lexer->source[lexer->position + 1] == '=')
        {
            token.type = 11 + 100;
            token.value = (char *)malloc(3);
            strcpy(token.value, "!=");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }
        if (current_char == '<' && lexer->source[lexer->position + 1] == '=')
        {
            token.type = 12 + 100;
            token.value = (char *)malloc(3);
            strcpy(token.value, "<=");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }
        if (current_char == '>' && lexer->source[lexer->position + 1] == '=')
        {
            token.type = 14 + 100;
            token.value = (char *)malloc(3);
            strcpy(token.value, ">=");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }
        if (current_char == '&' && lexer->source[lexer->position + 1] == '&')
        {
            token.type = 15 + 100;
            token.value = (char *)malloc(3);
            strcpy(token.value, "&&");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }
        if (current_char == '|' && lexer->source[lexer->position + 1] == '|')
        {
            token.type = 116;
            token.value = (char *)malloc(3);
            strcpy(token.value, "||");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }
        if (current_char == '-' && lexer->source[lexer->position + 1] == '>')
        {
            token.type = 48;
            token.value = (char *)malloc(3);
            strcpy(token.value, "->");
            lexer->position += 2;
            lexer->column += 2;
            return token;
        }

        token.type = (current_char == '(') ? 1 : (current_char == ')') ? 2
                                             : (current_char == '{')   ? 3
                                             : (current_char == '}')   ? 4
                                             : (current_char == '[')   ? 5
                                             : (current_char == ']')   ? 6
                                             : (current_char == ';')   ? 7
                                             : (current_char == ',')   ? 8
                                             : (current_char == '+')   ? 9
                                             : (current_char == '-')   ? 10
                                             : (current_char == '*')   ? 11
                                             : (current_char == '/')   ? 12
                                             : (current_char == '=')   ? 13
                                             : (current_char == '<')   ? 52
                                             : (current_char == '>')   ? 53
                                             : (current_char == '!')   ? 54
                                             : (current_char == '.')   ? 44
                                             : (current_char == ':')   ? 45
                                             : (current_char == '%')   ? 46
                                                                       : 0;

        if (token.type > 0)
        {
            token.value = (char *)malloc(2);
            token.value[0] = current_char;
            token.value[1] = '\0';
            lexer->position++;
            lexer->column++;
            return token;
        }

        lexer->position++;
        lexer->column++;
    }

    Token eof_token;
    eof_token.type = 50;
    eof_token.value = (char *)malloc(1);
    eof_token.value[0] = '\0';
    eof_token.line = lexer->line;
    eof_token.column = lexer->column;
    return eof_token;
}

void free_lexer(Lexer *lexer)
{
    free(lexer);
}

static const char *token_type_name(int type)
{
    switch (type)
    {
    case 1:
        return "LEFT_PAREN";
    case 2:
        return "RIGHT_PAREN";
    case 3:
        return "LEFT_BRACE";
    case 4:
        return "RIGHT_BRACE";
    case 5:
        return "LEFT_BRACKET";
    case 6:
        return "RIGHT_BRACKET";
    case 7:
        return "SEMICOLON";
    case 8:
        return "COMMA";
    case 9:
        return "PLUS";
    case 10:
        return "MINUS";
    case 11:
        return "STAR";
    case 12:
        return "SLASH";
    case 13:
        return "ASSIGN";
    case 14:
        return "IDENTIFIER";
    case 15:
        return "NUMBER";
    case 16:
        return "STRING";
    case 17:
        return "LOOP";
    case 18:
        return "IF";
    case 19:
        return "ELSE";
    case 20:
        return "RETURN";
    case 21:
        return "VOID";
    case 22:
        return "IMPORT";
    case 23:
        return "AS";
    case 24:
        return "NULL";
    case 25:
        return "CHAR";
    case 26:
        return "FUNCTION";
    case 27:
        return "STRUCT";
    case 28:
        return "CLASS";
    case 29:
        return "TRUE";
    case 30:
        return "FALSE";
    case 31:
        return "LIST";
    case 32:
        return "DICT";
    case 33:
        return "PASS";
    case 34:
        return "BOOLEAN";
    case 35:
        return "CHAR_TYPE";
    case 36:
        return "STRING_TYPE";
    case 37:
        return "ARRAY";
    case 38:
        return "FLOAT";
    case 39:
        return "INT";
    case 40:
        return "BOOL";
    case 41:
        return "CONST";
    case 42:
        return "PRIVATE";
    case 43:
        return "PUBLIC";
    case 44:
        return "DOT";
    case 45:
        return "COLON";
    case 46:
        return "PERCENT";
    case 48:
        return "ARROW";
    case 50:
        return "EOF";
    case 52:
        return "LESS";
    case 53:
        return "GREATER";
    case 54:
        return "BANG";
    case 107:
        return "INCREMENT";
    case 108:
        return "DECREMENT";
    case 109:
        return "PLUS_EQUAL";
    case 110:
        return "MINUS_EQUAL";
    case 111:
        return "NOT_EQUAL";
    case 112:
        return "LESS_EQUAL";
    case 113:
        return "EQUAL_EQUAL";
    case 114:
        return "GREATER_EQUAL";
    case 115:
        return "AND_AND";
    case 116:
        return "OR_OR";
    default:
        return "UNKNOWN";
    }
}

void token_print(Token token)
{
    printf("%-24s type=%3d line=%d column=%d value=\"%s\"\n",
           token_type_name(token.type),
           token.type,
           token.line,
           token.column,
           token.value ? token.value : "");
}
