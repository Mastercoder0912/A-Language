#include "ast.h"
#include <stdio.h>

static void print_indent(int depth)
{
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
}

static void print_block(AST_BLOCK* block, int depth)
{
    if (!block) {
        print_indent(depth);
        printf("Block(NULL)\n");
        return;
    }

    print_indent(depth);
    printf("Block(statement_count=%d)\n", block->statement_count);
    for (int i = 0; i < block->statement_count; i++) {
        ast_print(block->statements[i], depth + 1);
    }
}

void ast_print(ASTNode* node, int depth)
{
    if (!node) {
        print_indent(depth);
        printf("NULL\n");
        return;
    }

    print_indent(depth);

    switch (node->type) {
    case AST_PROGRAM_TYPE:
        printf("AST_PROGRAM(declaration_count=%d, statement_count=%d)\n",
               node->data.program.declaration_count,
               node->data.program.statement_count);
        print_indent(depth + 1);
        printf("declarations:\n");
        for (int i = 0; i < node->data.program.declaration_count; i++) {
            ast_print(node->data.program.declarations[i], depth + 2);
        }
        print_indent(depth + 1);
        printf("statements:\n");
        for (int i = 0; i < node->data.program.statement_count; i++) {
            ast_print(node->data.program.statements[i], depth + 2);
        }
        break;

    case AST_LITERAL_TYPE:
        printf("AST_LITERAL(type=%d, value=\"%s\")\n",
               node->data.literal.type,
               node->data.literal.value ? node->data.literal.value : "");
        break;

    case AST_IDENTIFIER_TYPE:
        printf("AST_IDENTIFIER(value=\"%s\")\n",
               node->data.identifier.value ? node->data.identifier.value : "");
        break;

    case AST_BINARY_OPERATION_TYPE:
        printf("AST_BINARY_OPERATION(op=%d)\n", node->data.binary_operation.op);
        print_indent(depth + 1);
        printf("left:\n");
        ast_print(node->data.binary_operation.left_operand, depth + 2);
        print_indent(depth + 1);
        printf("right:\n");
        ast_print(node->data.binary_operation.right_operand, depth + 2);
        break;

    case AST_UNARY_OPERATION_TYPE:
        printf("AST_UNARY_OPERATION(op=%d)\n", node->data.unary_operation.op);
        print_indent(depth + 1);
        printf("operand:\n");
        ast_print(node->data.unary_operation.operand, depth + 2);
        break;

    case AST_FUNCTION_CALL_DEFINITION_TYPE:
        printf("AST_FUNCTION_CALL(argument_count=%d)\n",
               node->data.function_call_definition.argument_count);
        print_indent(depth + 1);
        printf("name:\n");
        ast_print(node->data.function_call_definition.name, depth + 2);
        print_indent(depth + 1);
        printf("arguments:\n");
        for (int i = 0; i < node->data.function_call_definition.argument_count; i++) {
            ast_print(node->data.function_call_definition.arguments[i], depth + 2);
        }
        break;

    case AST_MEMBER_ACCESS_TYPE:
        printf("AST_MEMBER_ACCESS(member=\"%s\")\n",
               node->data.member_access.member ? node->data.member_access.member : "");
        print_indent(depth + 1);
        printf("object:\n");
        ast_print(node->data.member_access.object, depth + 2);
        break;

    case AST_INDEX_ACCESS_TYPE:
        printf("AST_INDEX_ACCESS\n");
        print_indent(depth + 1);
        printf("array:\n");
        ast_print(node->data.index_access.array, depth + 2);
        print_indent(depth + 1);
        printf("index:\n");
        ast_print(node->data.index_access.index, depth + 2);
        break;

    case AST_CAST_TYPE:
        printf("AST_CAST(target_type=%d)\n", node->data.cast.target_type);
        print_indent(depth + 1);
        printf("expression:\n");
        ast_print(node->data.cast.expression, depth + 2);
        break;

    case AST_ASSIGNMENT_TYPE:
        printf("AST_ASSIGNMENT(op=%d)\n", node->data.assignment.op);
        print_indent(depth + 1);
        printf("target:\n");
        ast_print(node->data.assignment.target, depth + 2);
        print_indent(depth + 1);
        printf("value:\n");
        ast_print(node->data.assignment.value, depth + 2);
        break;

    case AST_IMPORT_STATEMENT_TYPE:
        printf("AST_IMPORT(module=\"%s\", alias=\"%s\")\n",
               node->data.import_statement.module_name ? node->data.import_statement.module_name : "",
               node->data.import_statement.alias ? node->data.import_statement.alias : "");
        break;

    case AST_VARIABLE_DECLARATION_TYPE:
        printf("AST_VARIABLE_DECLARATION(name=\"%s\", type=%d, type_name=\"%s\")\n",
               node->data.variable_declaration.name ? node->data.variable_declaration.name : "",
               node->data.variable_declaration.type,
               node->data.variable_declaration.type_name ? node->data.variable_declaration.type_name : "");
        if (node->data.variable_declaration.init_value) {
            print_indent(depth + 1);
            printf("init_value:\n");
            ast_print(node->data.variable_declaration.init_value, depth + 2);
        }
        break;

    case AST_BLOCK_TYPE:
        print_block(&node->data.block, depth);
        break;

    case AST_IF_STATEMENT_TYPE:
        printf("AST_IF_STATEMENT(else_if_count=%d)\n", node->data.if_statement.else_if_count);
        print_indent(depth + 1);
        printf("condition:\n");
        ast_print(node->data.if_statement.condition, depth + 2);
        print_indent(depth + 1);
        printf("then_block:\n");
        print_block(node->data.if_statement.then_block, depth + 2);
        print_indent(depth + 1);
        printf("else_if_conditions:\n");
        for (int i = 0; i < node->data.if_statement.else_if_count; i++) {
            ast_print(node->data.if_statement.else_if_conditions[i], depth + 2);
            print_block(node->data.if_statement.else_if_blocks[i], depth + 2);
        }
        if (node->data.if_statement.else_block) {
            print_indent(depth + 1);
            printf("else_block:\n");
            print_block(node->data.if_statement.else_block, depth + 2);
        }
        break;

    case AST_LOOP_STATEMENT_TYPE:
        printf("AST_LOOP_STATEMENT(loop_type=\"%s\", parameter_count=%d)\n",
               node->data.loop_statement.loop_type ? node->data.loop_statement.loop_type : "",
               node->data.loop_statement.parameter_count);
        print_indent(depth + 1);
        printf("parameters:\n");
        for (int i = 0; i < node->data.loop_statement.parameter_count; i++) {
            ast_print(node->data.loop_statement.parameters[i], depth + 2);
        }
        print_indent(depth + 1);
        printf("body:\n");
        print_block(node->data.loop_statement.body, depth + 2);
        break;

    case AST_FUNCTION_CALL_STATEMENT_TYPE:
        printf("AST_FUNCTION_CALL_STATEMENT\n");
        break;

    case AST_PASS_STATEMENT_TYPE:
        printf("AST_PASS_STATEMENT\n");
        break;

    case AST_RETURN_STATEMENT_TYPE:
        printf("AST_RETURN_STATEMENT\n");
        if (node->data.return_statement.value) {
            print_indent(depth + 1);
            printf("expression:\n");
            ast_print(node->data.return_statement.value, depth + 2);
        }
        break;

    case AST_FUNCTION_DECLARATION_TYPE:
        printf("AST_FUNCTION_DECLARATION(name=\"%s\", parameter_count=%d)\n",
               node->data.function_declaration.name ? node->data.function_declaration.name : "",
               node->data.function_declaration.parameter_count);
        print_indent(depth + 1);
        printf("parameters:\n");
        for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
            ast_print(node->data.function_declaration.parameters[i], depth + 2);
        }
        print_indent(depth + 1);
        printf("body:\n");
        print_block(node->data.function_declaration.body, depth + 2);
        break;

    case AST_STRUCT_DECLARATION_TYPE:
        printf("AST_STRUCT_DECLARATION(name=\"%s\", field_count=%d)\n",
               node->data.struct_declaration.name ? node->data.struct_declaration.name : "",
               node->data.struct_declaration.field_count);
        for (int i = 0; i < node->data.struct_declaration.field_count; i++) {
            print_indent(depth + 1);
            printf("field(name=\"%s\", type=%d, type_name=\"%s\")\n",
                   node->data.struct_declaration.field_names[i],
                   node->data.struct_declaration.field_types[i],
                   node->data.struct_declaration.field_type_names ? node->data.struct_declaration.field_type_names[i] : "");
        }
        break;

    case AST_CLASS_DECLARATION_TYPE:
        printf("AST_CLASS_DECLARATION(name=\"%s\")\n",
               node->data.class_declaration.name ? node->data.class_declaration.name : "");
        print_indent(depth + 1);
        printf("private_variables(count=%d):\n", node->data.class_declaration.private_variable_count);
        for (int i = 0; i < node->data.class_declaration.private_variable_count; i++) {
            ast_print(node->data.class_declaration.private_variables[i], depth + 2);
        }
        print_indent(depth + 1);
        printf("public_variables(count=%d):\n", node->data.class_declaration.public_variable_count);
        for (int i = 0; i < node->data.class_declaration.public_variable_count; i++) {
            ast_print(node->data.class_declaration.public_variables[i], depth + 2);
        }
        print_indent(depth + 1);
        printf("private_methods(count=%d):\n", node->data.class_declaration.private_method_count);
        for (int i = 0; i < node->data.class_declaration.private_method_count; i++) {
            ast_print(node->data.class_declaration.private_methods[i], depth + 2);
        }
        print_indent(depth + 1);
        printf("public_methods(count=%d):\n", node->data.class_declaration.public_method_count);
        for (int i = 0; i < node->data.class_declaration.public_method_count; i++) {
            ast_print(node->data.class_declaration.public_methods[i], depth + 2);
        }
        break;

    case AST_F_STRING_TYPE:
        printf("AST_F_STRING(part_count=%d)\n", node->data.f_string.part_count);
        for (int i = 0; i < node->data.f_string.part_count; i++) {
            F_STRING_PART* part = node->data.f_string.parts[i];
            print_indent(depth + 1);
            if (part->is_expression) {
                printf("expression_part:\n");
                ast_print(part->content.expression, depth + 2);
            } else {
                printf("string_part(\"%s\")\n", part->content.string_part);
            }
        }
        break;

    case AST_LIST_LITERAL_TYPE:
        printf("AST_LIST_LITERAL(element_count=%d)\n", node->data.list_literal.element_count);
        for (int i = 0; i < node->data.list_literal.element_count; i++) {
            ast_print(node->data.list_literal.elements[i], depth + 1);
        }
        break;

    case AST_DICT_LITERAL_TYPE:
        printf("AST_DICT_LITERAL(entry_count=%d)\n", node->data.dict_literal.entry_count);
        for (int i = 0; i < node->data.dict_literal.entry_count; i++) {
            print_indent(depth + 1);
            printf("key:\n");
            ast_print(node->data.dict_literal.keys[i], depth + 2);
            print_indent(depth + 1);
            printf("value:\n");
            ast_print(node->data.dict_literal.values[i], depth + 2);
        }
        break;

    default:
        printf("UNKNOWN_AST_NODE(type=%d)\n", node->type);
        break;
    }
}
