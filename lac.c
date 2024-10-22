#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util/utility.h"

typedef enum {
    INT,
    STRING,
    FLOAT,
    BOOL
} VarType;

typedef struct {
    char name[256];
    VarType type;
    union {
        int int_val;
        char string_val[256];
        float float_val;
        int bool_val;
    } value;
} Variable;

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

void generate_executable(const char * bytecode_file,
    const char * output_c_file) {
    FILE * bytecode_fp = fopen(bytecode_file, "rb");
    if (!bytecode_fp) {
        handle_error("Could not open bytecode file for reading");
    }

    fseek(bytecode_fp, 0, SEEK_END);
    size_t bytecode_length = ftell(bytecode_fp);
    fseek(bytecode_fp, 0, SEEK_SET);

    unsigned char * bytecode = malloc(bytecode_length);
    fread(bytecode, 1, bytecode_length, bytecode_fp);
    fclose(bytecode_fp);

    FILE * output_fp = fopen(output_c_file, "w");
    if (!output_fp) {
        handle_error("Could not create C file for executable");
    }

    fprintf(output_fp,
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "typedef enum { INT, STRING, FLOAT, BOOL } VarType;\n"
        "typedef struct {\n"
        "    char name[256];\n"
        "    VarType type;\n"
        "    union {\n"
        "        int int_val;\n"
        "        char string_val[256];\n"
        "        float float_val;"
        "        int bool_val;"
        "    } value;\n"
        "} Variable;\n"
        "Variable variables[256];\n"
        "int var_count = 0;\n"
    );

    fprintf(output_fp, "unsigned char bytecode[] = {");
    for (size_t i = 0; i < bytecode_length; i++) {
        fprintf(output_fp, "0x%02x%s", bytecode[i], (i < bytecode_length - 1) ? ", " : "");
    }
    fprintf(output_fp, "};\nsize_t bytecode_length = %zu;\n", bytecode_length);

    fprintf(output_fp,
        "int find_variable_index(const char* var_name) {\n"
        "    for (int i = 0; i < var_count; i++) {\n"
        "        if (strcmp(var_name, variables[i].name) == 0) return i;\n"
        "    }\n"
        "    return -1;\n"
        "}\n"
    );

    fprintf(output_fp,
        "void run_bytecode(const unsigned char* bytecode, size_t length) {\n"
        "    size_t pc = 0;\n"
        "    while (pc < length) {\n"
        "        unsigned char instruction = bytecode[pc++];\n"
        "        if (instruction == 0x01) {\n"
        "            size_t len = *(size_t*)(bytecode + pc);\n"
        "            pc += sizeof(size_t);\n"
        "            char* string = (char*)malloc(len + 1);\n"
        "            memcpy(string, bytecode + pc, len);\n"
        "            string[len] = '\\0';\n"
        "            printf(\"%%s\", string);\n"
        "            pc += len;\n"
        "            free(string);\n"
        "        } else if (instruction == 0x02) {\n"
        "            printf(\"\\n\");\n"
        "        } else if (instruction == 0x03) {\n"
        "            int len = *(int*)(bytecode + pc);\n"
        "            pc += sizeof(int);\n"
        "            char var_name[256];\n"
        "            memcpy(var_name, bytecode + pc, len);\n"
        "            var_name[len] = '\\0';\n"
        "            pc += len;\n"
        "            int index = find_variable_index(var_name);\n"
        "            if (index != -1) {\n"
        "                if (variables[index].type == INT) printf(\"%%d\", variables[index].value.int_val);\n"
        "                else if (variables[index].type == STRING) printf(\"%%s\", variables[index].value.string_val);\n"
        "                else if (variables[index].type == FLOAT) printf(\"%%.2f\", variables[index].value.float_val);\n"
        "                else if (variables[index].type == BOOL) printf(\"%%s\", variables[index].value.bool_val ? \"true\" : \"false\");\n"
        "            }\n"
        "        } else if (instruction == 0x04) {\n"
        "            int len = *(int*)(bytecode + pc);\n"
        "            pc += sizeof(int);\n"
        "            char var_name[256];\n"
        "            memcpy(var_name, bytecode + pc, len);\n"
        "            var_name[len] = '\\0';\n"
        "            pc += len;\n"
        "            int value = *(int*)(bytecode + pc);\n"
        "            pc += sizeof(int);\n"
        "            int index = find_variable_index(var_name);\n"
        "            if (index == -1) {\n"
        "                strcpy(variables[var_count].name, var_name);\n"
        "                variables[var_count].type = INT;\n"
        "                variables[var_count++].value.int_val = value;\n"
        "            } else {\n"
        "                variables[index].value.int_val = value;\n"
        "            }\n"
        "        } else if (instruction == 0x05) {\n"
        "            int len = *(int*)(bytecode + pc);\n"
        "            pc += sizeof(int);\n"
        "            char var_name[256];\n"
        "            memcpy(var_name, bytecode + pc, len);\n"
        "            var_name[len] = '\\0';\n"
        "            pc += len;\n"
        "            int len_str = *(int*)(bytecode + pc);\n"
        "            pc += sizeof(int);\n"
        "            char string_val[256];\n"
        "            memcpy(string_val, bytecode + pc, len_str);\n"
        "            string_val[len_str] = '\\0';\n"
        "            pc += len_str;\n"
        "            strcpy(variables[var_count].name, var_name);\n"
        "            variables[var_count].type = STRING;\n"
        "            strcpy(variables[var_count++].value.string_val, string_val);\n"
        "        } else if (instruction == 0x06) {\n"
        "            printf(\" \");\n"
        "        } else if (instruction == 0x07) {\n"
        "            int len = *(int*)(bytecode + pc);\n"
        "            pc += sizeof(int);\n"
        "            char var_name[256];\n"
        "            memcpy(var_name, bytecode + pc, len);\n"
        "            var_name[len] = '\\0';\n"
        "            pc += len;\n"
        "\n"
        "            int src_len = *(int*)(bytecode + pc);\n"
        "            pc += sizeof(int);\n"
        "            char src_var[256];\n"
        "            memcpy(src_var, bytecode + pc, src_len);\n"
        "            src_var[src_len] = '\\0';\n"
        "            pc += src_len;\n"
        "\n"
        "            int src_index = find_variable_index(src_var);\n"
        "            if (src_index == -1) {\n"
        "            }\n"
        "\n"
        "            int target_index = find_variable_index(var_name);\n"
        "            if (target_index == -1) {\n"
        "                strcpy(variables[var_count].name, var_name);\n"
        "                variables[var_count].type = variables[src_index].type;\n"
        "                if (variables[src_index].type == INT) {\n"
        "                    variables[var_count++].value.int_val = variables[src_index].value.int_val;\n"
        "                } else if (variables[src_index].type == STRING) {\n"
        "                    strcpy(variables[var_count++].value.string_val, variables[src_index].value.string_val);\n"
        "                }\n"
        "            } else {\n"
        "                variables[target_index].type = variables[src_index].type;\n"
        "                if (variables[src_index].type == INT) {\n"
        "                    variables[target_index].value.int_val = variables[src_index].value.int_val;\n"
        "                } else if (variables[src_index].type == STRING) {\n"
        "                    strcpy(variables[target_index].value.string_val, variables[src_index].value.string_val);\n"
        "                }\n"
        "    }} else if (instruction == 0x08) {\n"
        "        int len = *(int*)(bytecode +pc);\n"
        "        pc += sizeof(int);\n"
        "        char var_name[256];\n"
        "        memcpy(var_name, bytecode + pc, len);\n"
        "        var_name[len] = '\\0';\n"
        "        pc += len;\n"
        "        float value = *(float*)(bytecode +pc);\n"
        "        pc += sizeof(float);\n"
        "        int index = find_variable_index(var_name);\n"
        "        if (index == -1) {\n"
        "            strcpy(variables[var_count].name, var_name);\n"
        "            variables[var_count].type = FLOAT;\n"
        "            variables[var_count++].value.float_val = value;\n"
        "        } else {\n"
        "            variables[index].value.float_val = value;\n"
        "        }\n"
        "    } else if (instruction == 0x09) {\n"
        "        int len = *(int*)(bytecode + pc);\n"
        "        pc += sizeof(int);\n"
        "        char var_name[256];\n"
        "        memcpy(var_name, bytecode + pc, len);\n"
        "        var_name[len] = '\\0';\n"
        "        pc += len;\n"
        "        int bool_value = *(int*)(bytecode + pc);\n"
        "        pc += sizeof(int);\n"
        "        int index = find_variable_index(var_name);\n"
        "        if (index == -1) {\n"
        "            strcpy(variables[var_count].name, var_name);\n"
        "            variables[var_count].type = BOOL;\n"
        "            variables[var_count++].value.bool_val = bool_value;\n"
        "        } else {\n"
        "            variables[index].value.bool_val = bool_value;\n"
        "        }\n"
        "}}}\n"

    );

    fprintf(output_fp,
        "int main() {\n"
        "    run_bytecode(bytecode, bytecode_length);\n"
        "    return 0;\n"
        "}\n"
    );

    fclose(output_fp);
    free(bytecode);
}

int main(int argc, char ** argv) {
    if (argc != 3) {
        handle_error("Usage: <executable_name> <input_file>");
    }

    const char * executable_name = argv[1];
    char * source_code = read_file(argv[2]);
    compile_to_bytecode(source_code, "/tmp/output.bytecode");
    free(source_code);

    generate_executable("/tmp/output.bytecode", "/tmp/temp.c");

    char command[256];
    snprintf(command, sizeof(command), "gcc /tmp/temp.c -o %s", executable_name);
    system(command);

    return 0;
}