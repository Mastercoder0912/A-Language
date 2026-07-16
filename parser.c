#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


Parser* parser_create(Lexer* lexer) {
    Parser* parser = malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->current_token = next_token(lexer);
    parser->peek_token = next_token(lexer);
    return parser;
}
ASTNode* parse_statement(Parser* parser);
ASTNode* parse_assignment(Parser* parser);
ASTNode* parse_return_statement(Parser* parser);

ASTNode* parse_expression(Parser* parser) {
    return parse_assignment(parser);
}
void parser_free(Parser* parser) {
    free(parser);
}

void parser_advance(Parser* parser) {
    parser->current_token = parser->peek_token;
    parser->peek_token = next_token(parser->lexer);
}

void parser_error(Parser* parser, const char* msg) {
    fprintf(stderr, "Parser error at line %d, column %d: %s\n", parser->current_token.line, parser->current_token.column, msg);
    exit(1);
}

static int is_builtin_type_token(int token_type) {
    return token_type == 31 ||
           token_type == 32 ||
           token_type == 34 ||
           token_type == 35 ||
           token_type == 36 ||
           token_type == 38 ||
           token_type == 39 ||
           token_type == 40;
}

static char* duplicate_text(const char* text) {
    if (!text) {
        return NULL;
    }

    char* copy = malloc(strlen(text) + 1);
    strcpy(copy, text);
    return copy;
}

static AST_BLOCK* parse_braced_block(Parser* parser, const char* context) {
    if (parser->current_token.type != 3) {
        parser_error(parser, context);
        return NULL;
    }
    parser_advance(parser);

    ASTNode** statements = NULL;
    int statement_count = 0;

    while (parser->current_token.type != 4 && parser->current_token.type != 50) {
        int start_position = parser->lexer->position;
        int start_token = parser->current_token.type;
        ASTNode* stmt = parse_statement(parser);
        statements = realloc(statements, sizeof(ASTNode*) * (statement_count + 1));
        statements[statement_count++] = stmt;
        if (parser->lexer->position == start_position && parser->current_token.type == start_token) {
            parser_error(parser, "Parser made no progress in block.");
            return NULL;
        }
    }

    if (parser->current_token.type == 50) {
        parser_error(parser, "Expected '}' before end of file.");
        return NULL;
    }

    parser_advance(parser);

    AST_BLOCK* block = malloc(sizeof(AST_BLOCK));
    block->statements = statements;
    block->statement_count = statement_count;
    return block;
}

ASTNode* parse_primary(Parser* parser) {
    Token current = parser->current_token;
    switch (current.type) {
        case 15: {
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_LITERAL_TYPE;
            node->data.literal.type = 15;
            node->data.literal.value = malloc(strlen(current.value) + 1);
            strcpy(node->data.literal.value, current.value);
            parser_advance(parser);
            return node;
        }
        case 16: {
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_LITERAL_TYPE;
            node->data.literal.type = 16;
            node->data.literal.value = malloc(strlen(current.value) + 1);
            strcpy(node->data.literal.value, current.value);
            parser_advance(parser);
            return node;
        }
        case TOKEN_F_STRING: {
            char* f_string_content = current.value;
            parser_advance(parser);
            return parse_f_string(parser, f_string_content);
        }
        case 29: {
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_LITERAL_TYPE;
            node->data.literal.type = 29;
            node->data.literal.value = malloc(strlen(current.value) + 1);
            strcpy(node->data.literal.value, current.value);
            parser_advance(parser);
            return node;
        }
        case 30: {
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_LITERAL_TYPE;
            node->data.literal.type = 30;
            node->data.literal.value = malloc(strlen(current.value) + 1);
            strcpy(node->data.literal.value, current.value);
            parser_advance(parser);
            return node;
        }
        case 24: {
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_LITERAL_TYPE;
            node->data.literal.type = 24;
            node->data.literal.value = malloc(strlen(current.value) + 1);
            strcpy(node->data.literal.value, current.value);
            parser_advance(parser);
            return node;
        }
        case 25: {
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_LITERAL_TYPE;
            node->data.literal.type = 25;
            node->data.literal.value = malloc(strlen(current.value) + 1);
            strcpy(node->data.literal.value, current.value);
            parser_advance(parser);
            return node;
        }
        case 14: {
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_IDENTIFIER_TYPE;
            node->data.identifier.value = malloc(strlen(current.value) + 1);
            strcpy(node->data.identifier.value, current.value);
            parser_advance(parser);
            return node;
        }
        case 31: {
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_IDENTIFIER_TYPE;
            node->data.identifier.value = malloc(strlen(current.value) + 1);
            strcpy(node->data.identifier.value, current.value);
            parser_advance(parser);
            return node;
        }
        case 1: {
            parser_advance(parser);
            ASTNode* expr = parse_expression(parser);
            if (parser->current_token.type != 2) {
                parser_error(parser, "Expected ')' after expression.");
                return NULL;
            }
            parser_advance(parser);
            return expr;
        }
        case 5: {
            parser_advance(parser);

            ASTNode** elements = NULL;
            int element_count = 0;

            while (parser->current_token.type != 6 && parser->current_token.type != 50) {
                ASTNode* element = parse_expression(parser);
                elements = realloc(elements, sizeof(ASTNode*) * (element_count + 1));
                elements[element_count++] = element;

                if (parser->current_token.type == 8) {
                    parser_advance(parser);
                } else if (parser->current_token.type != 6) {
                    parser_error(parser, "Expected ',' or ']' in list literal.");
                    return NULL;
                }
            }

            if (parser->current_token.type == 50) {
                parser_error(parser, "Expected ']' before end of file.");
                return NULL;
            }

            parser_advance(parser);

            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_LIST_LITERAL_TYPE;
            node->data.list_literal.elements = elements;
            node->data.list_literal.element_count = element_count;
            return node;
        }
        case 3: {
            parser_advance(parser);

            ASTNode** keys = NULL;
            ASTNode** values = NULL;
            int entry_count = 0;

            while (parser->current_token.type != 4 && parser->current_token.type != 50) {
                ASTNode* key = parse_expression(parser);

                if (parser->current_token.type != 45) {
                    parser_error(parser, "Expected ':' after dictionary key.");
                    return NULL;
                }
                parser_advance(parser);

                ASTNode* value = parse_expression(parser);

                keys = realloc(keys, sizeof(ASTNode*) * (entry_count + 1));
                values = realloc(values, sizeof(ASTNode*) * (entry_count + 1));
                keys[entry_count] = key;
                values[entry_count] = value;
                entry_count++;

                if (parser->current_token.type == 8) {
                    parser_advance(parser);
                } else if (parser->current_token.type != 4) {
                    parser_error(parser, "Expected ',' or '}' in dictionary literal.");
                    return NULL;
                }
            }

            if (parser->current_token.type == 50) {
                parser_error(parser, "Expected '}' before end of file.");
                return NULL;
            }

            parser_advance(parser);

            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_DICT_LITERAL_TYPE;
            node->data.dict_literal.keys = keys;
            node->data.dict_literal.values = values;
            node->data.dict_literal.entry_count = entry_count;
            return node;
        }
        default:
            parser_error(parser, "Expected primary expression.");
            return NULL;
    }
}

ASTNode* parse_postfix(Parser* parser) {
    ASTNode* node = parse_primary(parser);
    
    while (1) {
        if (parser->current_token.type == 1) {  
            parser_advance(parser);

            ASTNode** arguments = NULL;
            int argument_count = 0;
            
            while (parser->current_token.type != 2 && parser->current_token.type != 50) { 
                ASTNode* arg = parse_expression(parser);
                
                arguments = realloc(arguments, sizeof(ASTNode*) * (argument_count + 1));
                arguments[argument_count++] = arg;

                if (parser->current_token.type == 8) { 
                    parser_advance(parser);
                } else if (parser->current_token.type != 2) { 
                    parser_error(parser, "Expected ',' or ')' in function call.");
                    return NULL;
                }
            }

            if (parser->current_token.type == 50) {
                parser_error(parser, "Expected ')' before end of file.");
                return NULL;
            }
            
            if (parser->current_token.type != 2) {
                parser_error(parser, "Expected ')' after arguments.");
                return NULL;
            }
            parser_advance(parser);

            ASTNode* call_node = malloc(sizeof(ASTNode));
            call_node->type = AST_FUNCTION_CALL_DEFINITION_TYPE;
            call_node->data.function_call_definition.name = node;
            call_node->data.function_call_definition.arguments = arguments;
            call_node->data.function_call_definition.argument_count = argument_count;
            
            node = call_node;
        } 
        else if (parser->current_token.type == 44) { 
            parser_advance(parser);
            
            if (parser->current_token.type != 14) {  
                parser_error(parser, "Expected identifier after '.'");
                return NULL;
            }
            
            Token member_name = parser->current_token;
            parser_advance(parser);
            
            ASTNode* member_node = malloc(sizeof(ASTNode));
            member_node->type = AST_MEMBER_ACCESS_TYPE;
            member_node->data.member_access.object = node;
            member_node->data.member_access.member = malloc(strlen(member_name.value) + 1);
            strcpy(member_node->data.member_access.member, member_name.value);
            
            node = member_node;
        }
        else if (parser->current_token.type == 5) {  
            parser_advance(parser);
            
            ASTNode* index_expr = parse_expression(parser);  
            
            if (parser->current_token.type != 6) {  
                parser_error(parser, "Expected ']' after index.");
                return NULL;
            }
            parser_advance(parser);
            
            ASTNode* index_node = malloc(sizeof(ASTNode));
            index_node->type = AST_INDEX_ACCESS_TYPE;
            index_node->data.index_access.array = node;
            index_node->data.index_access.index = index_expr;
            
            node = index_node;  
        }
        else if (parser->current_token.type == 14) {
            Token potential_type = parser->current_token;
            int parsed_cast = 0;
            
            if (parser->peek_token.type == 1) {
                int is_type = 0;
                
                if (strcmp(potential_type.value, "int") == 0 ||
                    strcmp(potential_type.value, "string") == 0 ||
                    strcmp(potential_type.value, "float") == 0 ||
                    strcmp(potential_type.value, "bool") == 0 ||
                    strcmp(potential_type.value, "char") == 0) {
                    is_type = 1;
                }
                
                if (is_type) {
                    int type_token = 0;
                    if (strcmp(potential_type.value, "int") == 0) type_token = 39;
                    else if (strcmp(potential_type.value, "string") == 0) type_token = 36;
                    else if (strcmp(potential_type.value, "float") == 0) type_token = 38;
                    else if (strcmp(potential_type.value, "bool") == 0) type_token = 40;
                    else if (strcmp(potential_type.value, "char") == 0) type_token = 35;
                    
                    parser_advance(parser);
                    parser_advance(parser);
                    
                    ASTNode* expr = parse_expression(parser);
                    
                    if (parser->current_token.type != 2) {
                        parser_error(parser, "Expected ')' after cast expression.");
                        return NULL;
                    }
                    parser_advance(parser);
                    
                    ASTNode* cast_node = malloc(sizeof(ASTNode));
                    cast_node->type = AST_CAST_TYPE;
                    cast_node->data.cast.target_type = type_token;
                    cast_node->data.cast.expression = expr;
                    
                    node = cast_node;
                    parsed_cast = 1;
                }
            }

            if (!parsed_cast) {
                break;
            }
        }
        else {
            break;
        }
    }
    
    return node;
}

ASTNode* parse_unary(Parser* parser) {
    if ((parser->current_token.type == 39 ||
         parser->current_token.type == 36 ||
         parser->current_token.type == 38 ||
         parser->current_token.type == 40 ||
         parser->current_token.type == 35) &&
        parser->peek_token.type == 1) {
        int target_type = parser->current_token.type;
        parser_advance(parser);
        parser_advance(parser);

        ASTNode* expr = parse_expression(parser);

        if (parser->current_token.type != 2) {
            parser_error(parser, "Expected ')' after cast expression.");
            return NULL;
        }
        parser_advance(parser);

        ASTNode* cast_node = malloc(sizeof(ASTNode));
        cast_node->type = AST_CAST_TYPE;
        cast_node->data.cast.target_type = target_type;
        cast_node->data.cast.expression = expr;
        return cast_node;
    }

    if (parser->current_token.type == 107 || 
        parser->current_token.type == 108 ||  
        parser->current_token.type == 11 ||  
        parser->current_token.type == 10 ||  
        parser->current_token.type == 9 ||    
        parser->current_token.type == 54) {   
        
        int op = parser->current_token.type;
        parser_advance(parser);
        ASTNode* operand = parse_unary(parser);  
        
        ASTNode* unary_node = malloc(sizeof(ASTNode));
        unary_node->type = AST_UNARY_OPERATION_TYPE;
        unary_node->data.unary_operation.operand = operand;
        unary_node->data.unary_operation.op = op;
        
        return unary_node;
    }

    ASTNode* node = parse_postfix(parser);

    if (parser->current_token.type == 107 || parser->current_token.type == 108) {
        int op = parser->current_token.type;
        parser_advance(parser);
        
        ASTNode* unary_node = malloc(sizeof(ASTNode));
        unary_node->type = AST_UNARY_OPERATION_TYPE;
        unary_node->data.unary_operation.operand = node;
        unary_node->data.unary_operation.op = op;
        
        node = unary_node;
    }
    
    return node;
}

ASTNode* parse_multiplicative(Parser* parser) {
    ASTNode* node = parse_unary(parser);
    
    while (parser->current_token.type == 11 ||
           parser->current_token.type == 12 ||
           parser->current_token.type == 46) {
        
        int op = parser->current_token.type;
        parser_advance(parser);
        
        ASTNode* right = parse_unary(parser);
        
        ASTNode* binary_node = malloc(sizeof(ASTNode));
        binary_node->type = AST_BINARY_OPERATION_TYPE;
        binary_node->data.binary_operation.left_operand = node;
        binary_node->data.binary_operation.op = op;
        binary_node->data.binary_operation.right_operand = right;
        
        node = binary_node;
    }
    
    return node;
}

ASTNode* parse_additive(Parser* parser) {
    ASTNode* node = parse_multiplicative(parser);
    
    while (parser->current_token.type == 9 ||  
           parser->current_token.type == 10) {
        
        int op = parser->current_token.type;
        parser_advance(parser);
        
        ASTNode* right = parse_multiplicative(parser);
        
        ASTNode* binary_node = malloc(sizeof(ASTNode));
        binary_node->type = AST_BINARY_OPERATION_TYPE;
        binary_node->data.binary_operation.left_operand = node;
        binary_node->data.binary_operation.op = op;
        binary_node->data.binary_operation.right_operand = right;
        
        node = binary_node;
    }
    
    return node;
}

ASTNode* parse_comparison(Parser* parser) {
    ASTNode* node = parse_additive(parser);
    
    while (parser->current_token.type == 52 ||  
           parser->current_token.type == 53 ||   
           parser->current_token.type == 113 || 
           parser->current_token.type == 111 ||  
           parser->current_token.type == 112 ||  
           parser->current_token.type == 114) {  
        
        int op = parser->current_token.type;
        parser_advance(parser);
        
        ASTNode* right = parse_additive(parser);
        
        ASTNode* binary_node = malloc(sizeof(ASTNode));
        binary_node->type = AST_BINARY_OPERATION_TYPE;
        binary_node->data.binary_operation.left_operand = node;
        binary_node->data.binary_operation.op = op;
        binary_node->data.binary_operation.right_operand = right;
        
        node = binary_node;
    }
    
    return node;
}

ASTNode* parse_logical_and(Parser* parser) {
    ASTNode* node = parse_comparison(parser);
    
    while (parser->current_token.type == 115 || parser->current_token.type == 47) { 
        
        int op = parser->current_token.type;
        parser_advance(parser);
        
        ASTNode* right = parse_comparison(parser);
        
        ASTNode* binary_node = malloc(sizeof(ASTNode));
        binary_node->type = AST_BINARY_OPERATION_TYPE;
        binary_node->data.binary_operation.left_operand = node;
        binary_node->data.binary_operation.op = op;
        binary_node->data.binary_operation.right_operand = right;
        
        node = binary_node;
    }
    
    return node;
}

ASTNode* parse_logical_or(Parser* parser) {
    ASTNode* node = parse_logical_and(parser);
    
    while (parser->current_token.type == 116 || parser->current_token.type == 49) {
        
        int op = parser->current_token.type;
        parser_advance(parser);
        
        ASTNode* right = parse_logical_and(parser);
        
        ASTNode* binary_node = malloc(sizeof(ASTNode));
        binary_node->type = AST_BINARY_OPERATION_TYPE;
        binary_node->data.binary_operation.left_operand = node;
        binary_node->data.binary_operation.op = op;
        binary_node->data.binary_operation.right_operand = right;
        
        node = binary_node;
    }
    
    return node;
}

ASTNode* parse_assignment(Parser* parser) {
    ASTNode* node = parse_logical_or(parser);

    if (parser->current_token.type == 13 ||
        parser->current_token.type == 109 ||
        parser->current_token.type == 110) {
        int op = parser->current_token.type;
        parser_advance(parser);

        ASTNode* value = parse_assignment(parser);

        ASTNode* assignment_node = malloc(sizeof(ASTNode));
        assignment_node->type = AST_ASSIGNMENT_TYPE;
        assignment_node->data.assignment.target = node;
        assignment_node->data.assignment.op = op;
        assignment_node->data.assignment.value = value;
        return assignment_node;
    }

    return node;
}

ASTNode* parse_if_statement(Parser* parser) {
    parser_advance(parser);
    ASTNode* condition = parse_expression(parser);
    
    AST_BLOCK* then_block = parse_braced_block(parser, "Expected '{' after if condition.");
    
    ASTNode** else_if_conditions = NULL;
    AST_BLOCK** else_if_blocks = NULL;
    int else_if_count = 0;
    
    while (parser->current_token.type == 19 && 
           parser->peek_token.type == 18) { 
        parser_advance(parser);  
        parser_advance(parser);  
        
        ASTNode* elif_condition = parse_expression(parser);
        
        AST_BLOCK* elif_block = parse_braced_block(parser, "Expected '{' after else if condition.");
        
        else_if_conditions = realloc(else_if_conditions, sizeof(ASTNode*) * (else_if_count + 1));
        else_if_blocks = realloc(else_if_blocks, sizeof(AST_BLOCK*) * (else_if_count + 1));
        
        else_if_conditions[else_if_count] = elif_condition;
        else_if_blocks[else_if_count] = elif_block;
        
        else_if_count++;
    }

    AST_BLOCK* else_block = NULL;
    
    if (parser->current_token.type == 19) {
        parser_advance(parser);
        
        else_block = parse_braced_block(parser, "Expected '{' after else.");
    }

    ASTNode* if_node = malloc(sizeof(ASTNode));
    if_node->type = AST_IF_STATEMENT_TYPE;
    if_node->data.if_statement.condition = condition;
    if_node->data.if_statement.then_block = then_block;
    if_node->data.if_statement.else_if_conditions = else_if_conditions;
    if_node->data.if_statement.else_if_blocks = else_if_blocks;
    if_node->data.if_statement.else_if_count = else_if_count;
    if_node->data.if_statement.else_block = else_block;
    
    return if_node;
}

ASTNode* parse_loop_statement(Parser* parser) {
    parser_advance(parser);
    
    if (parser->current_token.type != 1) {
        parser_error(parser, "Expected '(' after loop.");
        return NULL;
    }
    parser_advance(parser);
    
    ASTNode* loop_node = malloc(sizeof(ASTNode));
    loop_node->type = AST_LOOP_STATEMENT_TYPE;
    
    ASTNode* first_parameter = parse_expression(parser);

    if (parser->current_token.type == 23) {
        parser_advance(parser);

        if (parser->current_token.type != 14) {
            parser_error(parser, "Expected loop variable after 'as'.");
            return NULL;
        }

        ASTNode* var_name = parse_primary(parser);
            
        if (parser->current_token.type != 2) {
            parser_error(parser, "Expected ')' after loop.");
            return NULL;
        }
        parser_advance(parser);
            
        AST_BLOCK* body = parse_braced_block(parser, "Expected '{' for loop body.");
            
        loop_node->data.loop_statement.loop_type = "counted";
        loop_node->data.loop_statement.parameters = malloc(sizeof(ASTNode*) * 2);
        loop_node->data.loop_statement.parameters[0] = first_parameter;
        loop_node->data.loop_statement.parameters[1] = var_name;
        loop_node->data.loop_statement.parameter_count = 2;
        loop_node->data.loop_statement.body = body;
            
        return loop_node;
    }
    
    ASTNode* condition = first_parameter;
    
    if (parser->current_token.type != 2) {
        parser_error(parser, "Expected ')' after loop condition.");
        return NULL;
    }
    parser_advance(parser);
    
    AST_BLOCK* body = NULL;
    if (parser->current_token.type == 3) {
        body = parse_braced_block(parser, "Expected '{' for loop body.");
    } else if (parser->current_token.type == 45) {
        parser_advance(parser);
        ASTNode* stmt = parse_statement(parser);
        body = malloc(sizeof(AST_BLOCK));
        body->statements = malloc(sizeof(ASTNode*));
        body->statements[0] = stmt;
        body->statement_count = 1;
    } else {
        parser_error(parser, "Expected '{' or ':' for loop body.");
        return NULL;
    }
    
    loop_node->data.loop_statement.loop_type = "conditional";
    loop_node->data.loop_statement.parameters = malloc(sizeof(ASTNode*));
    loop_node->data.loop_statement.parameters[0] = condition;
    loop_node->data.loop_statement.parameter_count = 1;
    loop_node->data.loop_statement.body = body;
    
    return loop_node;
}

ASTNode* parse_const_declaration(Parser* parser) {
    int type_token = parser->current_token.type;
    Token type_name = parser->current_token;
    parser_advance(parser);

    if (parser->current_token.type != 14) {
        parser_error(parser, "Expected identifier after const.");
        return NULL;
    }

    Token var_name = parser->current_token;
    parser_advance(parser);

    ASTNode* init_value = NULL;

    if (parser->current_token.type == 13) {
        parser_advance(parser);
        init_value = parse_expression(parser);
    }

    if (parser->current_token.type == 7) {
        parser_advance(parser);
    }

    ASTNode* var_node = malloc(sizeof(ASTNode));
    var_node->type = AST_VARIABLE_DECLARATION_TYPE;
    var_node->data.variable_declaration.name = malloc(strlen(var_name.value) + 1);
    strcpy(var_node->data.variable_declaration.name, var_name.value);
    var_node->data.variable_declaration.type = type_token;
    var_node->data.variable_declaration.type_name = malloc(strlen(type_name.value) + 1);
    strcpy(var_node->data.variable_declaration.type_name, type_name.value);
    var_node->data.variable_declaration.init_value = init_value;

    return var_node;
}

ASTNode* parse_variable_declaration(Parser* parser) {
    int type_token = parser->current_token.type;
    Token type_name = parser->current_token;
    parser_advance(parser);
    
    if (parser->current_token.type != 14) {
        parser_error(parser, "Expected identifier after type.");
        return NULL;
    }
    
    Token var_name = parser->current_token;
    parser_advance(parser);
    
    ASTNode* init_value = NULL;
    
    if (parser->current_token.type == 13) {
        parser_advance(parser);
        init_value = parse_expression(parser);
    }
    
    if (parser->current_token.type == 7) {
        parser_advance(parser);
    }
    
    ASTNode* var_node = malloc(sizeof(ASTNode));
    var_node->type = AST_VARIABLE_DECLARATION_TYPE;
    var_node->data.variable_declaration.name = malloc(strlen(var_name.value) + 1);
    strcpy(var_node->data.variable_declaration.name, var_name.value);
    var_node->data.variable_declaration.type = type_token;
    var_node->data.variable_declaration.type_name = malloc(strlen(type_name.value) + 1);
    strcpy(var_node->data.variable_declaration.type_name, type_name.value);
    var_node->data.variable_declaration.init_value = init_value;
    
    return var_node;
}

ASTNode* parse_user_type_declaration(Parser* parser) {
    int type_token = parser->current_token.type;
    Token type_name = parser->current_token;
    parser_advance(parser);

    int is_pointer = 0;
    if (parser->current_token.type == 11) {
        is_pointer = 1;
        parser_advance(parser);
    }

    if (parser->current_token.type != 14) {
        parser_error(parser, "Expected identifier after type.");
        return NULL;
    }

    Token var_name = parser->current_token;
    parser_advance(parser);

    ASTNode* init_value = NULL;

    if (parser->current_token.type == 13) {
        parser_advance(parser);
        init_value = parse_expression(parser);
    }

    if (parser->current_token.type == 7) {
        parser_advance(parser);
    }

    ASTNode* var_node = malloc(sizeof(ASTNode));
    var_node->type = AST_VARIABLE_DECLARATION_TYPE;
    var_node->data.variable_declaration.name = malloc(strlen(var_name.value) + 1);
    strcpy(var_node->data.variable_declaration.name, var_name.value);
    var_node->data.variable_declaration.type = type_token;
    if (is_pointer) {
        int len = strlen(type_name.value);
        var_node->data.variable_declaration.type_name = malloc(len + 2);
        strcpy(var_node->data.variable_declaration.type_name, type_name.value);
        strcat(var_node->data.variable_declaration.type_name, "*");
    } else {
        var_node->data.variable_declaration.type_name = malloc(strlen(type_name.value) + 1);
        strcpy(var_node->data.variable_declaration.type_name, type_name.value);
    }
    var_node->data.variable_declaration.init_value = init_value;

    return var_node;
}

ASTNode* parse_import_statement(Parser* parser) {
    parser_advance(parser);

    if (parser->current_token.type != 14) {
        parser_error(parser, "Expected module name after import.");
        return NULL;
    }

    char* module_name = duplicate_text(parser->current_token.value);
    parser_advance(parser);

    while (parser->current_token.type == 44) {
        parser_advance(parser);

        if (parser->current_token.type != 14) {
            parser_error(parser, "Expected module name after '.'.");
            return NULL;
        }

        int old_len = strlen(module_name);
        int part_len = strlen(parser->current_token.value);
        module_name = realloc(module_name, old_len + part_len + 2);
        module_name[old_len] = '.';
        strcpy(module_name + old_len + 1, parser->current_token.value);
        parser_advance(parser);
    }

    char* alias = NULL;
    if (parser->current_token.type == 23) {
        parser_advance(parser);
        if (parser->current_token.type != 14) {
            parser_error(parser, "Expected alias after as.");
            return NULL;
        }
        alias = malloc(strlen(parser->current_token.value) + 1);
        strcpy(alias, parser->current_token.value);
        parser_advance(parser);
    }

    ASTNode* import_node = malloc(sizeof(ASTNode));
    import_node->type = AST_IMPORT_STATEMENT_TYPE;
    import_node->data.import_statement.module_name = module_name;
    import_node->data.import_statement.alias = alias;

    return import_node;
}

ASTNode* parse_return_statement(Parser* parser) {
    parser_advance(parser);

    ASTNode* value = NULL;
    if (parser->current_token.type != 7 &&
        parser->current_token.type != 4 &&
        parser->current_token.type != 50) {
        value = parse_expression(parser);
    }

    if (parser->current_token.type == 7) {
        parser_advance(parser);
    }

    ASTNode* return_node = malloc(sizeof(ASTNode));
    return_node->type = AST_RETURN_STATEMENT_TYPE;
    return_node->data.return_statement.value = value;

    return return_node;
}

ASTNode* parse_block(Parser* parser) {
    if (parser->current_token.type != 3) {
        parser_error(parser, "Expected '{' at start of block.");
        return NULL;
    }
    parser_advance(parser);
    
    ASTNode** statements = NULL;
    int statement_count = 0;
    
    while (parser->current_token.type != 4 && parser->current_token.type != 50) {
        int start_position = parser->lexer->position;
        int start_token = parser->current_token.type;
        ASTNode* stmt = parse_statement(parser);
        statements = realloc(statements, sizeof(ASTNode*) * (statement_count + 1));
        statements[statement_count++] = stmt;
        if (parser->lexer->position == start_position && parser->current_token.type == start_token) {
            parser_error(parser, "Parser made no progress in block.");
            return NULL;
        }
    }
    
    if (parser->current_token.type != 4) {
        parser_error(parser, "Expected '}' at end of block.");
        return NULL;
    }
    parser_advance(parser);
    
    ASTNode* block_node = malloc(sizeof(ASTNode));
    block_node->type = AST_BLOCK_TYPE;
    block_node->data.block.statements = statements;
    block_node->data.block.statement_count = statement_count;
    
    return block_node;
}

ASTNode* parse_statement(Parser* parser) {
    switch (parser->current_token.type) {
        case 18: {
            return parse_if_statement(parser);
        }
        case 17: {
            return parse_loop_statement(parser);
        }
        case 39:
        case 31:
        case 32:
        case 37:
        case 36:
        case 34:
        case 40:
        case 35:
        case 38: {
            if (parser->peek_token.type == 1) {
                ASTNode* expr = parse_expression(parser);
                if (parser->current_token.type == 7) {
                    parser_advance(parser);
                }
                return expr;
            }
            return parse_variable_declaration(parser);
        }
        case 41: {
            return parse_const_declaration(parser);
        }
        case 3: {
            return parse_block(parser);
        }
        case 33: {
            ASTNode* pass_node = malloc(sizeof(ASTNode));
            pass_node->type = AST_PASS_STATEMENT_TYPE;
            parser_advance(parser);
            if (parser->current_token.type == 7) {
                parser_advance(parser);
            }
            return pass_node;
        }
        case 20: {
            return parse_return_statement(parser);
        }
        case 22: {
            return parse_import_statement(parser);
        }
        default: {
            if (parser->current_token.type == 14 &&
                (parser->peek_token.type == 14 || parser->peek_token.type == 11)) {
                return parse_user_type_declaration(parser);
            }
            ASTNode* expr = parse_expression(parser);
            if (parser->current_token.type == 7) {
                parser_advance(parser);
            }
            return expr;
        }
    }
}

ASTNode* parse_function_declaration(Parser* parser) {
    parser_advance(parser);
    
    if (parser->current_token.type != 14) {
        parser_error(parser, "Expected function name.");
        return NULL;
    }
    
    Token func_name = parser->current_token;
    parser_advance(parser);
    
    if (parser->current_token.type != 1) {
        parser_error(parser, "Expected '(' after function name.");
        return NULL;
    }
    parser_advance(parser);
    
    ASTNode** parameters = NULL;
    int param_count = 0;
    
    while (parser->current_token.type != 2) {
        int param_type = 0;
        char* param_type_name = NULL;
        Token param_name;

        if (is_builtin_type_token(parser->current_token.type) && parser->peek_token.type == 14) {
            param_type = parser->current_token.type;
            param_type_name = duplicate_text(parser->current_token.value);
            parser_advance(parser);
            param_name = parser->current_token;
        } else if (parser->current_token.type == 14) {
            param_name = parser->current_token;
        } else {
            parser_error(parser, "Expected parameter name.");
            return NULL;
        }

        parser_advance(parser);
        
        ASTNode* default_value = NULL;
        
        if (parser->current_token.type == 13) {
            parser_advance(parser);
            default_value = parse_expression(parser);
        }
        
        ASTNode* param_node = malloc(sizeof(ASTNode));
        param_node->type = AST_VARIABLE_DECLARATION_TYPE;
        param_node->data.variable_declaration.name = malloc(strlen(param_name.value) + 1);
        strcpy(param_node->data.variable_declaration.name, param_name.value);
        param_node->data.variable_declaration.type = param_type;
        param_node->data.variable_declaration.type_name = param_type_name;
        param_node->data.variable_declaration.init_value = default_value;
        
        parameters = realloc(parameters, sizeof(ASTNode*) * (param_count + 1));
        parameters[param_count++] = param_node;
        
        if (parser->current_token.type == 8) {
            parser_advance(parser);
        } else if (parser->current_token.type != 2) {
            parser_error(parser, "Expected ',' or ')' in parameter list.");
            return NULL;
        }
    }
    
    parser_advance(parser);
    
    AST_BLOCK* body = parse_braced_block(parser, "Expected '{' before function body.");
    
    ASTNode* func_node = malloc(sizeof(ASTNode));
    func_node->type = AST_FUNCTION_DECLARATION_TYPE;
    func_node->data.function_declaration.name = malloc(strlen(func_name.value) + 1);
    strcpy(func_node->data.function_declaration.name, func_name.value);
    func_node->data.function_declaration.parameters = parameters;
    func_node->data.function_declaration.parameter_count = param_count;
    func_node->data.function_declaration.body = body;
    
    return func_node;
}

ASTNode* parse_class_declaration(Parser* parser) {
    parser_advance(parser);
    
    if (parser->current_token.type != 14) {
        parser_error(parser, "Expected class name.");
        return NULL;
    }
    
    Token class_name = parser->current_token;
    parser_advance(parser);
    
    if (parser->current_token.type != 3) {
        parser_error(parser, "Expected '{' after class name.");
        return NULL;
    }
    parser_advance(parser);
    
    ASTNode** private_variables = NULL;
    int private_var_count = 0;
    ASTNode** public_variables = NULL;
    int public_var_count = 0;
    ASTNode** private_methods = NULL;
    int private_method_count = 0;
    ASTNode** public_methods = NULL;
    int public_method_count = 0;
    
    while (parser->current_token.type != 4 && parser->current_token.type != 50) {
        int start_position = parser->lexer->position;
        int start_token = parser->current_token.type;
        int is_private = 0;
        
        if (parser->current_token.type == 42) {
            is_private = 1;
            parser_advance(parser);
        } else if (parser->current_token.type == 43) {
            is_private = 0;
            parser_advance(parser);
        }
        
        if (parser->current_token.type == 26) {
            parser_advance(parser);

            if (parser->current_token.type == 44) {
                parser_advance(parser);
            }

            if (parser->current_token.type != 14) {
                parser_error(parser, "Expected method name.");
                return NULL;
            }
            
            Token method_name = parser->current_token;
            parser_advance(parser);
            
            if (parser->current_token.type != 1) {
                parser_error(parser, "Expected '(' after method name.");
                return NULL;
            }
            parser_advance(parser);
            
            ASTNode** parameters = NULL;
            int param_count = 0;
            
            while (parser->current_token.type != 2) {
                int param_type = 0;
                char* param_type_name = NULL;
                Token param_name;

                if (is_builtin_type_token(parser->current_token.type) && parser->peek_token.type == 14) {
                    param_type = parser->current_token.type;
                    param_type_name = duplicate_text(parser->current_token.value);
                    parser_advance(parser);
                    param_name = parser->current_token;
                } else if (parser->current_token.type == 14) {
                    param_name = parser->current_token;
                } else {
                    parser_error(parser, "Expected parameter name.");
                    return NULL;
                }

                parser_advance(parser);
                
                ASTNode* default_value = NULL;
                
                if (parser->current_token.type == 13) {
                    parser_advance(parser);
                    default_value = parse_expression(parser);
                }
                
                ASTNode* param_node = malloc(sizeof(ASTNode));
                param_node->type = AST_VARIABLE_DECLARATION_TYPE;
                param_node->data.variable_declaration.name = malloc(strlen(param_name.value) + 1);
                strcpy(param_node->data.variable_declaration.name, param_name.value);
                param_node->data.variable_declaration.type = param_type;
                param_node->data.variable_declaration.type_name = param_type_name;
                param_node->data.variable_declaration.init_value = default_value;
                
                parameters = realloc(parameters, sizeof(ASTNode*) * (param_count + 1));
                parameters[param_count++] = param_node;
                
                if (parser->current_token.type == 8) {
                    parser_advance(parser);
                } else if (parser->current_token.type != 2) {
                    parser_error(parser, "Expected ',' or ')' in parameter list.");
                    return NULL;
                }
            }
            
            parser_advance(parser);
            
            AST_BLOCK* body = parse_braced_block(parser, "Expected '{' before method body.");
            
            ASTNode* method_node = malloc(sizeof(ASTNode));
            method_node->type = AST_FUNCTION_DECLARATION_TYPE;
            method_node->data.function_declaration.name = malloc(strlen(method_name.value) + 1);
            strcpy(method_node->data.function_declaration.name, method_name.value);
            method_node->data.function_declaration.parameters = parameters;
            method_node->data.function_declaration.parameter_count = param_count;
            method_node->data.function_declaration.body = body;
            
            if (is_private) {
                private_methods = realloc(private_methods, sizeof(ASTNode*) * (private_method_count + 1));
                private_methods[private_method_count++] = method_node;
            } else {
                public_methods = realloc(public_methods, sizeof(ASTNode*) * (public_method_count + 1));
                public_methods[public_method_count++] = method_node;
            }
        } else if (parser->current_token.type == 39 ||
                   parser->current_token.type == 36 ||
                   parser->current_token.type == 34 ||
                   parser->current_token.type == 40 ||
                   parser->current_token.type == 35 ||
                   parser->current_token.type == 37 ||
                   parser->current_token.type == 38) {
            
            int var_type = parser->current_token.type;
            Token type_name = parser->current_token;
            parser_advance(parser);
            
            if (parser->current_token.type != 14) {
                parser_error(parser, "Expected variable name.");
                return NULL;
            }
            
            Token var_name = parser->current_token;
            parser_advance(parser);
            
            ASTNode* var_node = malloc(sizeof(ASTNode));
            var_node->type = AST_VARIABLE_DECLARATION_TYPE;
            var_node->data.variable_declaration.name = malloc(strlen(var_name.value) + 1);
            strcpy(var_node->data.variable_declaration.name, var_name.value);
            var_node->data.variable_declaration.type = var_type;
            var_node->data.variable_declaration.type_name = malloc(strlen(type_name.value) + 1);
            strcpy(var_node->data.variable_declaration.type_name, type_name.value);
            var_node->data.variable_declaration.init_value = NULL;
            
            if (is_private) {
                private_variables = realloc(private_variables, sizeof(ASTNode*) * (private_var_count + 1));
                private_variables[private_var_count++] = var_node;
            } else {
                public_variables = realloc(public_variables, sizeof(ASTNode*) * (public_var_count + 1));
                public_variables[public_var_count++] = var_node;
            }
        } else {
            parser_error(parser, "Expected class member declaration.");
            return NULL;
        }

        if (parser->lexer->position == start_position && parser->current_token.type == start_token) {
            parser_error(parser, "Parser made no progress in class body.");
            return NULL;
        }
    }

    if (parser->current_token.type == 50) {
        parser_error(parser, "Expected '}' before end of file.");
        return NULL;
    }
    
    parser_advance(parser);
    
    ASTNode* class_node = malloc(sizeof(ASTNode));
    class_node->type = AST_CLASS_DECLARATION_TYPE;
    class_node->data.class_declaration.name = malloc(strlen(class_name.value) + 1);
    strcpy(class_node->data.class_declaration.name, class_name.value);
    class_node->data.class_declaration.private_variables = private_variables;
    class_node->data.class_declaration.private_variable_count = private_var_count;
    class_node->data.class_declaration.public_variables = public_variables;
    class_node->data.class_declaration.public_variable_count = public_var_count;
    class_node->data.class_declaration.private_methods = private_methods;
    class_node->data.class_declaration.private_method_count = private_method_count;
    class_node->data.class_declaration.public_methods = public_methods;
    class_node->data.class_declaration.public_method_count = public_method_count;
    
    return class_node;
}

ASTNode* parse_struct_declaration(Parser* parser) {
    parser_advance(parser);
    
    if (parser->current_token.type != 14) {
        parser_error(parser, "Expected struct name.");
        return NULL;
    }
    
    Token struct_name = parser->current_token;
    parser_advance(parser);
    
    if (parser->current_token.type != 3) {
        parser_error(parser, "Expected '{' after struct name.");
        return NULL;
    }
    parser_advance(parser);
    
    char** field_names = NULL;
    int* field_types = NULL;
    char** field_type_names = NULL;
    int field_count = 0;
    
    while (parser->current_token.type != 4 && parser->current_token.type != 50) {
        if (parser->current_token.type == 39 ||
            parser->current_token.type == 36 ||
            parser->current_token.type == 34 ||
            parser->current_token.type == 40 ||
            parser->current_token.type == 35 ||
            parser->current_token.type == 37 ||
            parser->current_token.type == 38) {
            
            int field_type = parser->current_token.type;
            Token field_type_name = parser->current_token;
            parser_advance(parser);
            
            if (parser->current_token.type != 14) {
                parser_error(parser, "Expected field name.");
                return NULL;
            }
            
            Token field_name = parser->current_token;
            parser_advance(parser);
            
            if (parser->current_token.type == 7) {
                parser_advance(parser);
            }
            
            field_names = realloc(field_names, sizeof(char*) * (field_count + 1));
            field_names[field_count] = malloc(strlen(field_name.value) + 1);
            strcpy(field_names[field_count], field_name.value);
            
            field_types = realloc(field_types, sizeof(int) * (field_count + 1));
            field_types[field_count] = field_type;

            field_type_names = realloc(field_type_names, sizeof(char*) * (field_count + 1));
            field_type_names[field_count] = duplicate_text(field_type_name.value);
            
            field_count++;
        } else {
            parser_error(parser, "Expected type in struct field.");
            return NULL;
        }
    }

    if (parser->current_token.type == 50) {
        parser_error(parser, "Expected '}' before end of file.");
        return NULL;
    }
    
    parser_advance(parser);
    
    ASTNode* struct_node = malloc(sizeof(ASTNode));
    struct_node->type = AST_STRUCT_DECLARATION_TYPE;
    struct_node->data.struct_declaration.name = malloc(strlen(struct_name.value) + 1);
    strcpy(struct_node->data.struct_declaration.name, struct_name.value);
    struct_node->data.struct_declaration.field_names = field_names;
    struct_node->data.struct_declaration.field_types = field_types;
    struct_node->data.struct_declaration.field_type_names = field_type_names;
    struct_node->data.struct_declaration.field_count = field_count;
    
    return struct_node;
}

Program* parse_program(Parser* parser) {
    ASTNode** declarations = NULL;
    int declaration_count = 0;
    ASTNode** statements = NULL;
    int statement_count = 0;
    
    while (parser->current_token.type != 50) {
        int start_position = parser->lexer->position;
        int start_token = parser->current_token.type;

        if (parser->current_token.type == 26) {
            ASTNode* func_decl = parse_function_declaration(parser);
            declarations = realloc(declarations, sizeof(ASTNode*) * (declaration_count + 1));
            declarations[declaration_count++] = func_decl;
        }
        else if (parser->current_token.type == 27) {
            ASTNode* struct_decl = parse_struct_declaration(parser);
            declarations = realloc(declarations, sizeof(ASTNode*) * (declaration_count + 1));
            declarations[declaration_count++] = struct_decl;
        }
        else if (parser->current_token.type == 28) {
            ASTNode* class_decl = parse_class_declaration(parser);
            declarations = realloc(declarations, sizeof(ASTNode*) * (declaration_count + 1));
            declarations[declaration_count++] = class_decl;
        }
        else {
            ASTNode* stmt = parse_statement(parser);
            statements = realloc(statements, sizeof(ASTNode*) * (statement_count + 1));
            statements[statement_count++] = stmt;
        }

        if (parser->lexer->position == start_position && parser->current_token.type == start_token) {
            parser_error(parser, "Parser made no progress.");
            return NULL;
        }
    }
    
    ASTNode* program_node = malloc(sizeof(ASTNode));
    program_node->type = AST_PROGRAM_TYPE;
    program_node->data.program.declarations = declarations;
    program_node->data.program.declaration_count = declaration_count;
    program_node->data.program.statements = statements;
    program_node->data.program.statement_count = statement_count;
    
    return program_node;
}

Program* parser_parse_source(const char* source) {
    Lexer* lexer = lexer_init(source);
    Parser* parser = parser_create(lexer);
    Program* program = parse_program(parser);

    parser_free(parser);
    free_lexer(lexer);

    return program;
}

ASTNode* parse_f_string(Parser* parser, char* f_string_value) {
    (void)parser;
    F_STRING_PART** parts = NULL;
    int part_count = 0;
    
    int i = 0;
    int len = strlen(f_string_value);
    
    while (i < len) {
        if (f_string_value[i] == '{') {
            i++;
            int expr_start = i;
            int brace_depth = 1;
            
            while (i < len && brace_depth > 0) {
                if (f_string_value[i] == '{') brace_depth++;
                if (f_string_value[i] == '}') brace_depth--;
                i++;
            }
            
            int expr_len = i - expr_start - 1;
            char* expr_str = malloc(expr_len + 1);
            strncpy(expr_str, f_string_value + expr_start, expr_len);
            expr_str[expr_len] = '\0';
            
            Lexer expr_lexer = {0};
            expr_lexer.source = expr_str;
            expr_lexer.position = 0;
            expr_lexer.line = 1;
            expr_lexer.column = 1;
            
            Parser expr_parser = {0};
            expr_parser.lexer = &expr_lexer;
            expr_parser.current_token = next_token(&expr_lexer);
            expr_parser.peek_token = next_token(&expr_lexer);
            
            ASTNode* expr = parse_expression(&expr_parser);
            
            F_STRING_PART* expr_part = malloc(sizeof(F_STRING_PART));
            expr_part->is_expression = 1;
            expr_part->content.expression = expr;
            
            parts = realloc(parts, sizeof(F_STRING_PART*) * (part_count + 1));
            parts[part_count++] = expr_part;
            
            free(expr_str);
        } else {
            int str_start = i;
            
            while (i < len && f_string_value[i] != '{') {
                i++;
            }
            
            int str_len = i - str_start;
            char* str_part = malloc(str_len + 1);
            strncpy(str_part, f_string_value + str_start, str_len);
            str_part[str_len] = '\0';
            
            F_STRING_PART* string_part = malloc(sizeof(F_STRING_PART));
            string_part->is_expression = 0;
            string_part->content.string_part = str_part;
            
            parts = realloc(parts, sizeof(F_STRING_PART*) * (part_count + 1));
            parts[part_count++] = string_part;
        }
    }
    
    ASTNode* fstring_node = malloc(sizeof(ASTNode));
    fstring_node->type = AST_F_STRING_TYPE;
    fstring_node->data.f_string.parts = parts;
    fstring_node->data.f_string.part_count = part_count;
    
    return fstring_node;
}

void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    if (node->type == AST_FUNCTION_CALL_DEFINITION_TYPE) {
        printf("FUNCTION_CALL: %s\n", node->data.function_call_definition.name->data.identifier.value);
    } else if (node->type == AST_BINARY_OPERATION_TYPE) {
        printf("BINARY_OP: %d\n", node->data.binary_operation.op);
        print_ast(node->data.binary_operation.left_operand, indent + 1);
        print_ast(node->data.binary_operation.right_operand, indent + 1);
    } else if (node->type == AST_IDENTIFIER_TYPE) {
        printf("IDENTIFIER: %s\n", node->data.identifier.value);
    } else if (node->type == AST_LITERAL_TYPE) {
        printf("LITERAL: type=%d\n", node->data.literal.type);
    }
}
