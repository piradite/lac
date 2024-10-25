#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "bytecode.h"
#include "util/utility.h"

Variable variables[256];
int var_count = 0;

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
            while ( *ptr == ' ' || *ptr == '\t') ptr++;
            int first_item = 1;

            while ( *ptr && *ptr != '\n') {
                has_content = 1;
                if ( *ptr == '"') {
                    if (!first_item && !last_was_newline) fwrite(&(char){0x06}, sizeof(char), 1, output);
                    first_item = last_was_newline = 0;

                    fwrite(&(char){0x01}, sizeof(char), 1, output);
                    const char * start = ++ptr;

                    while ( *ptr != '"' && *ptr) ptr++;
                    size_t len = ptr - start;
                    fwrite(&len, sizeof(size_t), 1, output);
                    fwrite(start, sizeof(char), len, output);
                    ptr++;
                } else if (isalnum( *ptr)) {
                    if (!first_item && !last_was_newline) fwrite(&(char){0x06}, sizeof(char), 1, output);
                    first_item = last_was_newline = 0;

                    char var_name[256];
                    int len = 0;

                    while (isalnum( *ptr)) var_name[len++] = *ptr++;
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

                    while ( *ptr == ' ' || *ptr == ':') ptr++;
                    if ( *ptr == '"') {
                        ptr++;
                        const char * start = ptr;
                        while ( *ptr != '"' && *ptr) ptr++;
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

                if ( *ptr == ',') ptr++;
                while ( *ptr == ' ' || *ptr == '\t') ptr++;
            }

            if ( *ptr == '\n' && ptr[1]) {
                fwrite(&(char){0x02}, sizeof(char), 1, output);
                last_was_newline = 1;
            }
        } else if (isalpha( *ptr)) {
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

            if (expected_type == INT) {
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

                if (var_index == -1) {
                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = INT;
                    variables[var_count].storage_type = storage_type;
                    variables[var_count++].value.int_val = int_value;
                } else if (variables[var_index].type == INT) {
                    variables[var_index].value.int_val = int_value;
                } else {
                    handle_error("Type mismatch in redeclaration");
                }

                fwrite(&(char){0x04}, sizeof(char), 1, output);
                fwrite(&len, sizeof(int), 1, output);
                fwrite(var_name, sizeof(char), len, output);
                fwrite(&int_value, sizeof(int), 1, output);

            } else if (expected_type == STRING && *ptr == '"') {
                ptr++;
                const char * start = ptr;
                while ( *ptr != '"' && *ptr != '\0')
                    ptr++;
                size_t len_str = ptr - start;
                ptr++;

                if (var_index == -1) {
                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = STRING;
                    variables[var_count].storage_type = storage_type;
                    strncpy(variables[var_count++].value.string_val, start, len_str);
                    variables[var_count - 1].value.string_val[len_str] = '\0';
                } else {
                    if (variables[var_index].type != STRING) {
                        handle_error("Type mismatch in redeclaration");
                    }

                    memset(variables[var_index].value.string_val, 0, sizeof(variables[var_index].value.string_val));
                    strncpy(variables[var_index].value.string_val, start, len_str);
                    variables[var_index].value.string_val[len_str] = '\0';
                }

                fwrite(&(char){0x05}, sizeof(char), 1, output);
                fwrite( & len, sizeof(int), 1, output);
                fwrite(var_name, sizeof(char), len, output);
                fwrite( & len_str, sizeof(int), 1, output);
                fwrite(start, sizeof(char), len_str, output);
            } else if (expected_type == FLOAT) {
                float float_value = 0.0;

                if (isdigit( *ptr) || *ptr == '-') {
                    float_value = strtof(ptr, (char **)&ptr);

                    if (var_index == -1) {

                        strcpy(variables[var_count].name, var_name);
                        variables[var_count].type = FLOAT;
                        variables[var_count].storage_type = storage_type;
                        variables[var_count++].value.float_val = float_value;
                    } else {

                        if (variables[var_index].type != FLOAT) {
                            handle_error("Type mismatch in redeclaration");
                        }
                        variables[var_index].value.float_val = float_value;
                    }

                    fwrite(&(char){0x08}, sizeof(char), 1, output);
                    fwrite( & len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                    fwrite( & float_value, sizeof(float), 1, output);
                } else {

                    char referenced_var_name[256];
                    int ref_len = 0;

                    while (isalnum( *ptr)) {
                        referenced_var_name[ref_len++] = *ptr++;
                    }
                    referenced_var_name[ref_len] = '\0';

                    int referenced_var_index = find_variable_index(referenced_var_name);
                    if (referenced_var_index == -1 || variables[referenced_var_index].type != FLOAT) {
                        handle_error("Invalid float value, expected a float literal or another float variable.");
                    }

                    float_value = variables[referenced_var_index].value.float_val;

                    if (var_index == -1) {

                        strcpy(variables[var_count].name, var_name);
                        variables[var_count].type = FLOAT;
                        variables[var_count].storage_type = storage_type;
                        variables[var_count++].value.float_val = float_value;
                    } else {

                        if (variables[var_index].type != FLOAT) {
                            handle_error("Type mismatch in redeclaration");
                        }
                        variables[var_index].value.float_val = float_value;
                    }

                    fwrite(&(char){0x08}, sizeof(char), 1, output);
                    fwrite( & len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                    fwrite( & float_value, sizeof(float), 1, output);
                }
            } else if (expected_type == BOOL) {
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

                    while (isalnum( *ptr)) {
                        referenced_var_name[ref_len++] = *ptr++;
                    }
                    referenced_var_name[ref_len] = '\0';

                    int referenced_var_index = find_variable_index(referenced_var_name);
                    if (referenced_var_index == -1 || variables[referenced_var_index].type != BOOL) {
                        handle_error("Invalid boolean value, expected 'true', 'false', or another boolean variable.");
                    }

                    bool_value = variables[referenced_var_index].value.bool_val;
                }

                if (var_index == -1) {
                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = BOOL;
                    variables[var_count].storage_type = storage_type;
                    variables[var_count++].value.bool_val = bool_value;
                } else {
                    if (variables[var_index].type != BOOL) {
                        handle_error("Type mismatch in redeclaration");
                    }
                    variables[var_index].value.bool_val = bool_value;
                }

                fwrite(&(char){0x09}, sizeof(char), 1, output);
                fwrite( & len, sizeof(int), 1, output);
                fwrite(var_name, sizeof(char), len, output);
                fwrite( & bool_value, sizeof(int), 1, output);

                if (var_index == -1) {
                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = BOOL;
                    variables[var_count].storage_type = storage_type;
                    variables[var_count++].value.bool_val = bool_value;
                } else {
                    if (variables[var_index].type != BOOL) {
                        handle_error("Type mismatch in redeclaration");
                    }
                    variables[var_index].value.bool_val = bool_value;
                }

                fwrite(&(char){0x09}, sizeof(char), 1, output);
                fwrite( & len, sizeof(int), 1, output);
                fwrite(var_name, sizeof(char), len, output);
                fwrite( & bool_value, sizeof(int), 1, output);
            } else if (expected_type == CHAR) {
                char char_value = '\0';

                if ( *ptr == '\'') {
                    ptr++;
                    char_value = *ptr++;
                    if ( *ptr == '\'') {
                        ptr++;
                    } else {
                        handle_error("Invalid char value, expected a single character in quotes.");
                    }
                } else {

                    char referenced_var_name[256];
                    int ref_len = 0;

                    while (isalnum( *ptr)) {
                        referenced_var_name[ref_len++] = *ptr++;
                    }
                    referenced_var_name[ref_len] = '\0';

                    int referenced_var_index = find_variable_index(referenced_var_name);
                    if (referenced_var_index == -1 || variables[referenced_var_index].type != CHAR) {
                        handle_error("Invalid character value, expected a char literal or another char variable.");
                    }

                    char_value = variables[referenced_var_index].value.char_val;
                }

                if (var_index == -1) {
                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = CHAR;
                    variables[var_count].storage_type = storage_type;
                    variables[var_count++].value.char_val = char_value;
                } else {
                    if (variables[var_index].type != CHAR) {
                        handle_error("Type mismatch in redeclaration");
                    }
                    variables[var_index].value.char_val = char_value;
                }

                fwrite(&(char){0x0A}, sizeof(char), 1, output);
                fwrite( & len, sizeof(int), 1, output);
                fwrite(var_name, sizeof(char), len, output);
                fwrite( & char_value, sizeof(char), 1, output);
            }
        }
        last_char = *ptr;
        ptr++;
    }

    if (last_char != '\n' ? (has_content = 0, 1) : has_content) {
        fwrite(&(char){0x02}, sizeof(char), 1, output);
    }

    fclose(output);
}