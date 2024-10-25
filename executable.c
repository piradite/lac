#include "executable.h"
#include "util/utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
        "#include <stdint.h>\n"
        "typedef enum { NONE, INT, STRING, FLOAT, BOOL, CHAR } VarType;\n"
        "typedef enum { CONST, LET, VAR } VarStorageType;\n"
        "typedef struct {\n"
        "    char name[256];\n"
        "    VarType type;\n"
        "    VarStorageType storage_type;\n"
        "    union {\n"
        "        int32_t int_val;\n"
        "        char string_val[256];\n"
        "        float float_val;\n"
        "        int bool_val;\n"
        "        char char_val;\n"
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
        "                else if (variables[index].type == CHAR) printf(\"%%c\", variables[index].value.char_val);\n"
        "                "
        "            }\n"
        "        } else if (instruction == 0x04) {\n"
        "            int len = *(int*)(bytecode + pc);\n"
        "            pc += sizeof(int);\n"
        "            char var_name[256];\n"
        "            memcpy(var_name, bytecode + pc, len);\n"
        "            var_name[len] = '\\0';\n"
        "            pc += len;\n"
        "            int32_t value = *(int32_t*)(bytecode + pc);\n"
        "            pc += sizeof(int32_t);\n"
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
        "    } else if (instruction == 0x0A) {\n"
        "        int len = *(int*)(bytecode + pc);\n"
        "        pc += sizeof(int);\n"
        "        char var_name[256];\n"
        "        memcpy(var_name, bytecode + pc, len);\n"
        "        var_name[len] = '\\0';\n"
        "        pc += len;\n"
        "        char value = *(char*)(bytecode + pc);\n"
        "        pc += sizeof(char);\n"
        "        int index = find_variable_index(var_name);\n"
        "        if (index == -1) {\n"
        "            strcpy(variables[var_count].name, var_name);\n"
        "            variables[var_count].type = CHAR;\n"
        "            variables[var_count++].value.char_val = value;\n"
        "        } else {\n"
        "            variables[index].value.char_val = value;\n"
        "        }\n"
        "      }\n"
        "    }\n"
        " }\n"

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