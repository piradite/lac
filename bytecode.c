#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "bytecode.h"
#include "util/utility.h"
#include "util/variables.h"

int find_variable_index(const char * var_name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(var_name, variables[i].name) == 0)
            return i;
    return -1;
}

void check_variable_redefinition(int var_index, VarType new_type, VarStorageType storage_type) {
    if (var_index == -1) return;

    VarStorageType existing_storage = variables[var_index].storage_type;
    VarType existing_type = variables[var_index].type;

    if (existing_storage == CONST)
        handle_error("Cannot change 'const' variable. It is immutable.");
    else if (existing_storage == VAR && new_type != NONE && existing_type != new_type)
        handle_error("Type mismatch during reassignment.");
    else if (existing_storage == LET) {
        if (storage_type == LET)
            handle_error("Cannot redeclare a 'let' variable within the same scope.");
        if (new_type != NONE && existing_type != new_type)
            handle_error("Type mismatch during reassignment.");
    }
}

void write_to_output(FILE *output, char byte_code, int len, const char *name, void *data, size_t data_size) {
    char buffer[512];
    int offset = 0;

    buffer[offset++] = byte_code;
    memcpy(buffer + offset, &len, sizeof(int));
    offset += sizeof(int);
    memcpy(buffer + offset, name, len);
    offset += len;

    if (data != NULL && data_size > 0) {
        memcpy(buffer + offset, data, data_size);
        offset += data_size;
    }

    fwrite(buffer, sizeof(char), offset, output);
}

void compile_to_bytecode(const char * source_code,
    const char * bytecode_file) {
    FILE * output = fopen(bytecode_file, "wb");
    if (!output) {
        handle_error("Could not create bytecode file");
    }

    const char *ptr = source_code;
    int last_was_newline = 0;
    char last_char = '\0';
    int has_content = 0;

    while ( *ptr) {

        if (( *ptr == ';' && ( * (ptr + 1) == ';' || * (ptr + 1) == '*'))) {
            ptr += ( * (ptr + 1) == ';') ? strcspn(ptr, "\n") : 2 + strcspn(ptr + 2, "*;");
            continue;
        }

        if (strncmp(ptr, "print", 5) == 0) {
            ptr += 5;
            while (*ptr == ' ' || *ptr == '\t') ptr++;
    
            int first_item = 1;
            while (*ptr && *ptr != '\n') {
                has_content = 1;
                if (*ptr == '"') {
                    if (!first_item && !last_was_newline) write_to_output(output, 0x06, 0, NULL, NULL, 0);
                    first_item = last_was_newline = 0;

                    fwrite(&(char){0x01}, sizeof(char), 1, output);
                    const char *start = ++ptr;
                    while (*ptr != '"' && *ptr) ptr++;
                    size_t len = ptr - start;
                    fwrite(&len, sizeof(size_t), 1, output);
                    fwrite(start, sizeof(char), len, output);
                    ptr++;
        } else if (isalnum(*ptr)) {
            if (!first_item && !last_was_newline) write_to_output(output, 0x06, 0, NULL, NULL, 0);
            first_item = last_was_newline = 0;

            char var_name[256];
            int len = 0;
            while (isalnum(*ptr)) var_name[len++] = *ptr++;
            var_name[len] = '\0';

            int var_index = find_variable_index(var_name);
            if (var_index == -1) handle_error("Variable not declared");

            if (variables[var_index].type == STRING) {
                fwrite(&(char){0x01}, sizeof(char), 1, output);
                size_t str_len = strlen(variables[var_index].value.string_val);
                fwrite(&str_len, sizeof(size_t), 1, output);
                fwrite(variables[var_index].value.string_val, sizeof(char), str_len, output);
            } else {
                fwrite(&(char){0x03}, sizeof(char), 1, output);
                fwrite(&len, sizeof(int), 1, output);
                fwrite(var_name, sizeof(char), len, output);
            }

            while (*ptr == ' ' || *ptr == ':') ptr++;
            if (*ptr == '"') {
                ptr++;
                const char *start = ptr;
                while (*ptr != '"' && *ptr) ptr++;
                size_t len_str = ptr - start;
                ptr++;

                if (variables[var_index].type != STRING) handle_error("Type mismatch during reassignment.");
                if (len_str >= sizeof(variables[var_index].value.string_val)) handle_error("String value exceeds allocated space.");

                strncpy(variables[var_index].value.string_val, start, len_str);
                variables[var_index].value.string_val[len_str] = '\0';

                fwrite(&(char){0x05}, sizeof(char), 1, output);
                fwrite(&len, sizeof(int), 1, output);
                fwrite(var_name, sizeof(char), len, output);
                fwrite(&len_str, sizeof(int), 1, output);
                fwrite(start, sizeof(char), len_str, output);
            }
        }

        if (*ptr == ',') ptr++;
        while (*ptr == ' ' || *ptr == '\t') ptr++;
    }

    if (*ptr == '\n' && ptr[1]) {
        fwrite(&(char){0x02}, sizeof(char), 1, output);
        last_was_newline = 1;
    }
}
 else if (isalpha( *ptr)) {
            char var_name[256];
            int len = 0;
            VarStorageType storage_type = VAR;
            VarType expected_type = NONE;

            while (isalnum( *ptr)) var_name[len++] = *ptr++;
            var_name[len] = '\0';

            int var_index = find_variable_index(var_name);

            if ( *ptr == ':')
                ptr++;

            while ( *ptr == ' ' || *ptr == '\t')
                ptr++;
            
            
            if (var_index != -1 && ( *ptr == '"' || isdigit( *ptr) || ( *ptr == '\''))) {
                expected_type = variables[var_index].type;
            } else {
                if (var_index != -1) {
                    storage_type = variables[var_index].storage_type;
                    expected_type = variables[var_index].type;
                } else if (strncmp(ptr, "const", 5) == 0) {
                    storage_type = CONST;
                    ptr += 5;
                } else if (strncmp(ptr, "let", 3) == 0) {
                    storage_type = LET;
                    ptr += 3;
                } else if (strncmp(ptr, "var", 3) == 0) {
                    storage_type = VAR;
                    ptr += 3;
                } else if (var_index == -1) {
                    expected_type = variables[var_index].type;
                    check_variable_redefinition(var_index, expected_type, storage_type);
                } else {
                    handle_error("Variable not declared before use.");
                }
            }

            while ( *ptr == ' ' || *ptr == '\t')
                ptr++;
            
            if (strncmp(ptr, "int ->", 6) == 0) {
                ptr += 6;
                expected_type = INT;
            } else if (strncmp(ptr, "string ->", 9) == 0) {
                ptr += 9;
                expected_type = STRING;

            } else if (strncmp(ptr, "float ->", 8) == 0) {
                ptr += 8;
                expected_type = FLOAT;
            } else if (strncmp(ptr, "bool ->", 7) == 0) {
                ptr += 7;
                expected_type = BOOL;
            } else if (strncmp(ptr, "char ->", 7) == 0) {
                ptr += 7;
                expected_type = CHAR;
            }
            
            check_variable_redefinition(var_index, expected_type, storage_type);

            if (expected_type == NONE && var_index != -1) {
                expected_type = variables[var_index].type;
            }

            while (*ptr == ' ') ptr++;

        switch (expected_type) {
            case INT: {
                int int_value = 0;
                char referenced_var_name[256];
                int ref_len = 0;
                bool is_literal = isdigit(*ptr) || *ptr == '-';

                if (is_literal) {
                    int_value = strtol(ptr, (char **)&ptr, 10);
                    while (isdigit(*ptr) || *ptr == '-') ptr++;
                } else {
                    while (isalnum(*ptr)) referenced_var_name[ref_len++] = *ptr++;
                    referenced_var_name[ref_len] = '\0';

                    int referenced_var_index = find_variable_index(referenced_var_name);
                    if (referenced_var_index == -1 || variables[referenced_var_index].type != INT) {
                        handle_error("Invalid int value, expected an int literal or another int variable.");
                    }
                    int_value = variables[referenced_var_index].value.int_val;
                }

                assign_variable_value(var_index, var_name, INT, storage_type, &int_value, sizeof(int));
                write_to_output(output, 0x04, len, var_name, &int_value, sizeof(int));
                break;
            }

            case STRING: {
                if (*ptr == '"') {
                    ptr++;
                    const char *start = ptr;
                    while (*ptr != '"' && *ptr != '\0') ptr++;
                    size_t len_str = ptr - start;
                    ptr++;

                    assign_variable_value(var_index, var_name, STRING, storage_type, (void *)start, len_str);
                    fwrite(&(char){0x05}, sizeof(char), 1, output);
                    fwrite(&len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                    fwrite(&len_str, sizeof(int), 1, output);
                    fwrite(start, sizeof(char), len_str, output);
                }
                break;
            }

            case FLOAT: {
                float float_value = 0.0;

                if (isdigit(*ptr) || *ptr == '-') {
                    float_value = strtof(ptr, (char **)&ptr);
                } else {
                    char referenced_var_name[256];
                    int ref_len = 0;
                    while (isalnum(*ptr)) {
                        referenced_var_name[ref_len++] = *ptr++;
                    }
                    referenced_var_name[ref_len] = '\0';

                    int referenced_var_index = find_variable_index(referenced_var_name);
                    if (referenced_var_index == -1 || variables[referenced_var_index].type != FLOAT) {
                        handle_error("Invalid float value, expected a float literal or another float variable.");
                    }
                    float_value = variables[referenced_var_index].value.float_val;
                }

                assign_variable_value(var_index, var_name, FLOAT, storage_type, &float_value, sizeof(char));
                write_to_output(output, 0x08, len, var_name, &float_value, sizeof(float));
                break;
            }

            case BOOL: {
                int bool_value = 0;

                if (strncmp(ptr, "true", 4) == 0) {
                    ptr += 4;
                    bool_value = 1;
                } else if (strncmp(ptr, "false", 5) == 0) {
                    ptr += 5;
                    bool_value = 0;
                } else {
                    char referenced_var_name[256];
                    int ref_len = 0;

                    while (isalnum(*ptr)) {
                        referenced_var_name[ref_len++] = *ptr++;
                    }
                    referenced_var_name[ref_len] = '\0';

                    int referenced_var_index = find_variable_index(referenced_var_name);
                    if (referenced_var_index == -1 || variables[referenced_var_index].type != BOOL) {
                        handle_error("Invalid boolean value, expected 'true', 'false', or another boolean variable.");
                    }

                    bool_value = variables[referenced_var_index].value.bool_val;
                }

                assign_variable_value(var_index, var_name, BOOL, storage_type, &bool_value, sizeof(int));
                write_to_output(output, 0x09, len, var_name, &bool_value, sizeof(int));
                break;
            }

            case CHAR: {
                char char_value = '\0';

                if (*ptr == '\'') {
                    ptr++;
                    char_value = *ptr++;
                    if (*ptr != '\'') {
                        handle_error("Invalid char value, expected a single character in quotes.");
                    }
                    ptr++;
                } else {
                    char referenced_var_name[256];
                    int ref_len = 0;

                    while (isalnum(*ptr)) {
                        referenced_var_name[ref_len++] = *ptr++;
                    }
                    referenced_var_name[ref_len] = '\0';

                    int referenced_var_index = find_variable_index(referenced_var_name);
                    if (referenced_var_index == -1 || variables[referenced_var_index].type != CHAR) {
                        handle_error("Invalid character value, expected a char literal or another char variable.");
                    }

                    char_value = variables[referenced_var_index].value.char_val;
                }

                assign_variable_value(var_index, var_name, CHAR, storage_type, &char_value, sizeof(char));
                write_to_output(output, 0x0A, len, var_name, &char_value, sizeof(char));
                break;
            }

            default:
                handle_error("Unsupported type");
                break;
        }
    }

        last_char = *ptr;
        ptr++;
    }

    if (last_char != '\n' ? (has_content = 0, 1) : has_content) {
        write_to_output(output, 0x02, 0, NULL, NULL, sizeof(char));
    }

    fclose(output);
}