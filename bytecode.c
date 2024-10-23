#include "bytecode.h"
#include "util/utility.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

Variable variables[256];
int var_count = 0;

int find_variable_index(const char *var_name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_name, variables[i].name) == 0) return i;
    }
    return -1;
}

void compile_to_bytecode(const char *source_code, const char *bytecode_file) {
    FILE *output = fopen(bytecode_file, "wb");
    if (!output) {
        handle_error("Could not create bytecode file");
    }

    const char *ptr = source_code;
    int last_was_newline = 0;
    char last_char = '\0';
    int has_content = 0;

    while (*ptr) {

        if (*ptr == ';' && *(ptr + 1) == ';' && *(ptr + 2) != '*') {
            while (*ptr && *ptr != '\n') {
                ptr++;
            }
            continue;
        }


        if (*ptr == ';' && *(ptr + 1) == '*') {
            ptr += 2;
            while (*ptr && !(*ptr == '*' && *(ptr + 1) == ';')) {
                ptr++;
            }
            if (*ptr) ptr += 2;
            continue;
        }


        if (strncmp(ptr, "print", 5) == 0) {
            ptr += 5;
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            int first_item = 1;
            while (*ptr && *ptr != '\n') {
                has_content = 1;
                if (*ptr == '"') {
                    if (!first_item && !last_was_newline) fprintf(output, "%c", 0x06);
                    first_item = last_was_newline = 0;
                    fprintf(output, "%c", 0x01);
                    const char *start = ++ptr;
                    while (*ptr != '"' && *ptr != '\0') ptr++;
                    size_t len = ptr - start;
                    fwrite(&len, sizeof(size_t), 1, output);
                    fwrite(start, sizeof(char), len, output);
                    ptr++;
                } else if (isalnum(*ptr)) {
                    if (!first_item && !last_was_newline) fprintf(output, "%c", 0x06);
                    first_item = last_was_newline = 0;
                    char var_name[256];
                    int len = 0;
                    while (isalnum(*ptr)) var_name[len++] = *ptr++;
                    var_name[len] = '\0';
                    int var_index = find_variable_index(var_name);
                    if (var_index == -1) {
                        handle_error("Variable not declared");
                    }
                    fprintf(output, "%c", 0x03);
                    fwrite(&len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                }
                if (*ptr == ',') ptr++;
                while (*ptr == ' ' || *ptr == '\t') ptr++;
            }
            if (*ptr == '\n' && ptr[1]) fprintf(output, "%c", 0x02), last_was_newline = 1;
        } else if (isalpha(*ptr)) {
            char var_name[256], src_var[256];
            int len = 0;
            while (isalnum(*ptr)) var_name[len++] = *ptr++;
            var_name[len] = '\0';
            if (strncmp(ptr, ": int ->", 8) == 0) {
                ptr += 8;
                while (*ptr == ' ' || *ptr == '-') ptr++;

                if (isdigit(*ptr)) {
                    int value = atoi(ptr);
                    while (isdigit(*ptr)) ptr++;
                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = INT;
                    variables[var_count++].value.int_val = value;

                    fprintf(output, "%c", 0x04);
                    fwrite(&len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                    fwrite(&value, sizeof(int), 1, output);

                } else {
                    char src_var[256];
                    int src_len = 0;
                    while (isalnum(*ptr)) src_var[src_len++] = *ptr++;
                    src_var[src_len] = '\0';

                    int src_index = find_variable_index(src_var);
                    if (src_index == -1) {
                        handle_error("Source variable not declared");
                    }

                    if (variables[src_index].type != INT) {
                        handle_error("Source variable is not of type int");
                    }

                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = INT;
                    variables[var_count++].value.int_val = variables[src_index].value.int_val;

                    fprintf(output, "%c", 0x07);
                    fwrite(&len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                    fwrite(&src_len, sizeof(int), 1, output);
                    fwrite(src_var, sizeof(char), src_len, output);
                }
            } else if (strncmp(ptr, ": string ->", 11) == 0) {
                ptr += 11;
                while (*ptr == ' ' || *ptr == '-') ptr++;

                if (*ptr == '"') {
                    ptr++;
                    const char *start = ptr;
                    while (*ptr != '"' && *ptr != '\0') ptr++;
                    size_t len_str = ptr - start;
                    ptr++;

                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = STRING;
                    strncpy(variables[var_count++].value.string_val, start, len_str);
                    variables[var_count - 1].value.string_val[len_str] = '\0';

                    fprintf(output, "%c", 0x05);
                    fwrite(&len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                    fwrite(&len_str, sizeof(int), 1, output);
                    fwrite(start, sizeof(char), len_str, output);
                } else {

                    char src_var[256];
                    int src_len = 0;
                    while (isalnum(*ptr)) src_var[src_len++] = *ptr++;
                    src_var[src_len] = '\0';

                    int src_index = find_variable_index(src_var);
                    if (src_index == -1) {
                        handle_error("Source variable not declared");
                    }

                    if (variables[src_index].type != STRING) {
                        handle_error("Source variable is not of type string");
                    }

                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = STRING;
                    strcpy(variables[var_count++].value.string_val, variables[src_index].value.string_val);

                    fprintf(output, "%c", 0x07);
                    fwrite(&len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                    fwrite(&src_len, sizeof(int), 1, output);
                    fwrite(src_var, sizeof(char), src_len, output);
                }
                } else if (strncmp(ptr, ": float ->", 10) == 0) {
                    ptr += 10;
                    while (*ptr == ' ' || *ptr == '-') ptr++;

                    if (isdigit(*ptr) || *ptr == '-') {
                        float value = strtof(ptr, (char **)&ptr);
                        strcpy(variables[var_count].name, var_name);
                        variables[var_count].type = FLOAT;
                        variables[var_count++].value.float_val = value;

                        fprintf(output, "%c", 0x08);
                        fwrite(&len, sizeof(int), 1, output);
                        fwrite(var_name, sizeof(char), len, output);
                        fwrite(&value, sizeof(float), 1, output);
                    } else {
                        handle_error("Invalid float value.");
                    }
                } else if (strncmp(ptr, ": bool ->", 9) == 0) {
                    ptr += 9;
                    while (*ptr == ' ' || *ptr == '-') ptr++;

                    int bool_value;
                    if (strncmp(ptr, "true", 4) == 0) {
                        bool_value = 1;
                        ptr += 4;
                    } else if (strncmp(ptr, "false", 5) == 0) {
                        bool_value = 0;
                        ptr += 5;
                    } else if (*ptr == '1' || *ptr == '0') {
                        bool_value = (*ptr == '1') ? 1 : 0;
                        ptr++;
                    } else {
                        handle_error("Invalid bool value.");
                    }

                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = BOOL;
                    variables[var_count++].value.bool_val = bool_value;

                    fprintf(output, "%c", 0x09);
                    fwrite(&len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                    fwrite(&bool_value, sizeof(int), 1, output);

                    } else if (strncmp(ptr, ": char ->", 9) == 0) {
                        ptr += 9;
                        while (*ptr == ' ' || *ptr == '-') ptr++;

                        if (*ptr == '"') {
                            ptr++;
                            const char *start = ptr;
                            char last_char = '\0';

                            while (*ptr != '"' && *ptr != '\0') {
                                last_char = *ptr++;
                            }
                            ptr++;

                            strcpy(variables[var_count].name, var_name);
                            variables[var_count].type = CHAR;
                            variables[var_count++].value.char_val = last_char;

                            fprintf(output, "%c", 0x0A);
                            fwrite(&len, sizeof(int), 1, output);
                            fwrite(var_name, sizeof(char), len, output);
                            fwrite(&last_char, sizeof(char), 1, output);

                        } else {
                            handle_error("Invalid char value, expected a quoted string.");
                        }
                        } else if (strncmp(ptr, "->", 2) == 0) {
                            ptr += 2;
                            while (*ptr == ' ' || *ptr == '-') ptr++;
                            int src_len = 0;
                            while (isalnum(*ptr)) src_var[src_len++] = *ptr++;
                            src_var[src_len] = '\0';
                            if (src_len == 0 || find_variable_index(src_var) == -1) {
                                handle_error("Source variable not declared");
                            }
                            fprintf(output, "%c", 0x07);
                            fwrite(&len, sizeof(int), 1, output);
                            fwrite(var_name, sizeof(char), len, output);
                            fwrite(&src_len, sizeof(int), 1, output);
                            fwrite(src_var, sizeof(char), src_len, output);
            } 

        }
        last_char = *ptr;
        ptr++;
    }
    if (last_char !='\n') {
        has_content = 0;
        fprintf(output, "%c", 0x02);
    }
    
    if (last_char == '\n' && has_content) {
        fprintf(output, "%c", 0x02);
    }

    fclose(output);
}