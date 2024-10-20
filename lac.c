#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    INT,
    CHAR
}
VarType;

typedef struct {
    char name[256];
    VarType type;
    union {
        int int_val;
        char char_val[256];
    }
    value;
}
Variable;

Variable variables[256];
int var_count = 0;

void handle_error(const char * message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(1);
}

char * read_file(const char * filename) {
    FILE * file = fopen(filename, "r");
    if (!file) {
        handle_error("Could not open file");
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char * content = malloc(length + 1);
    if (!content) {
        handle_error("Memory allocation failed");
    }
    fread(content, 1, length, file);
    fclose(file);
    content[length] = '\0';
    return content;
}

int find_variable_index(const char * var_name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_name, variables[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

void compile_to_bytecode(const char * source_code,
    const char * bytecode_file) {
    FILE * output = fopen(bytecode_file, "wb");
    if (!output) {
        handle_error("Could not create bytecode file");
    }
    const char * ptr = source_code;
    int last_was_newline = 0;

    while ( * ptr) {
        if (strncmp(ptr, "print", 5) == 0) {
            ptr += 5;
            while ( * ptr == ' ' || * ptr == '\t') ptr++;
            int first_item = 1;
            while ( * ptr && * ptr != '\n') {
                if ( * ptr == '"') {
                    if (!first_item && !last_was_newline) fprintf(output, "%c", 0x06);
                    first_item = last_was_newline = 0;
                    fprintf(output, "%c", 0x01);
                    const char * start = ++ptr;
                    while ( * ptr != '"' && * ptr != '\0') ptr++;
                    size_t len = ptr - start;
                    fwrite( & len, sizeof(size_t), 1, output);
                    fwrite(start, sizeof(char), len, output);
                    ptr++;
                } else if (isalnum( * ptr)) {
                    if (!first_item && !last_was_newline) fprintf(output, "%c", 0x06);
                    first_item = last_was_newline = 0;
                    char var_name[256];
                    int len = 0;
                    while (isalnum( * ptr)) var_name[len++] = * ptr++;
                    var_name[len] = '\0';
                    int var_index = find_variable_index(var_name);
                    if (var_index == -1) {
                        handle_error("Variable not declared");
                    }
                    fprintf(output, "%c", 0x03);
                    fwrite( & len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                }
                if ( * ptr == ',') ptr++;
                while ( * ptr == ' ' || * ptr == '\t') ptr++;
            }
            if ( * ptr == '\n' && ptr[1]) fprintf(output, "%c", 0x02), last_was_newline = 1;
        } else if (isalpha( * ptr)) {
            char var_name[256], src_var[256];
            int len = 0;
            while (isalnum( * ptr)) var_name[len++] = * ptr++;
            var_name[len] = '\0';
            if (strncmp(ptr, ": int ->", 8) == 0) {
                ptr += 8;
                while ( * ptr == ' ' || * ptr == '-') ptr++;
                if (isdigit( * ptr)) {
                    int value = atoi(ptr);
                    while (isdigit( * ptr)) ptr++;
                    strcpy(variables[var_count].name, var_name);
                    variables[var_count].type = INT;
                    variables[var_count++].value.int_val = value;
                    fprintf(output, "%c", 0x04);
                    fwrite( & len, sizeof(int), 1, output);
                    fwrite(var_name, sizeof(char), len, output);
                    fwrite( & value, sizeof(int), 1, output);
                }
            } else if (strncmp(ptr, ": char ->", 9) == 0) {
    ptr += 9;
    while (*ptr == ' ' || *ptr == '-') ptr++;

    if (*ptr == '"') {
        ptr++;
        const char *start = ptr;
        while (*ptr != '"' && *ptr != '\0') ptr++;
        size_t len_str = ptr - start;
        ptr++;

        strcpy(variables[var_count].name, var_name);
        variables[var_count].type = CHAR;
        strncpy(variables[var_count++].value.char_val, start, len_str);
        variables[var_count - 1].value.char_val[len_str] = '\0';

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

        if (variables[src_index].type != CHAR) {
            handle_error("Source variable is not of type char");
        }

        strcpy(variables[var_count].name, var_name);
        variables[var_count].type = CHAR;
        strcpy(variables[var_count++].value.char_val, variables[src_index].value.char_val);

        fprintf(output, "%c", 0x07); 
        fwrite(&len, sizeof(int), 1, output);
        fwrite(var_name, sizeof(char), len, output);
        fwrite(&src_len, sizeof(int), 1, output);
        fwrite(src_var, sizeof(char), src_len, output);
    }

            } else if (strncmp(ptr, "->", 2) == 0) {
                ptr += 2;
                while ( * ptr == ' ' || * ptr == '-') ptr++;
                int src_len = 0;
                while (isalnum( * ptr)) src_var[src_len++] = * ptr++;
                src_var[src_len] = '\0';
                if (src_len == 0 || find_variable_index(src_var) == -1) {
                    handle_error("Source variable not declared");
                }
                fprintf(output, "%c", 0x07);
                fwrite( & len, sizeof(int), 1, output);
                fwrite(var_name, sizeof(char), len, output);
                fwrite( & src_len, sizeof(int), 1, output);
                fwrite(src_var, sizeof(char), src_len, output);
            }

        }
        ptr++;
    }
    fclose(output);
}

void run_bytecode(const unsigned char * bytecode, size_t length) {
    size_t pc = 0;

    while (pc < length) {
        unsigned char instruction = bytecode[pc++];
        if (instruction == 0x01) {
            size_t len = * (size_t * )(bytecode + pc);
            pc += sizeof(size_t);
            char * string = (char * ) malloc(len + 1);
            memcpy(string, bytecode + pc, len);
            string[len] = '\0';
            printf("%s", string);
            pc += len;
            free(string);
        } else if (instruction == 0x02) {
            printf("\n");
        } else if (instruction == 0x03) {
            int len = * (int * )(bytecode + pc);
            pc += sizeof(int);
            char var_name[256];
            memcpy(var_name, bytecode + pc, len);
            var_name[len] = '\0';
            pc += len;
            for (int i = 0; i < var_count; i++) {
                if (!strcmp(var_name, variables[i].name)) {
                    if (variables[i].type == INT) {
                        printf("%d", variables[i].value.int_val);
                    } else if (variables[i].type == CHAR) {
                        printf("%s", variables[i].value.char_val);
                    }
                    break;
                }
            }
        } else if (instruction == 0x04) {
            int len = * (int * )(bytecode + pc);
            pc += sizeof(int);
            char var_name[256];
            memcpy(var_name, bytecode + pc, len);
            var_name[len] = '\0';
            pc += len;
            int value = * (int * )(bytecode + pc);
            pc += sizeof(int);
            for (int i = 0; i < var_count; i++) {
                if (!strcmp(var_name, variables[i].name)) {
                    variables[i].value.int_val = value;
                    goto done;
                }
            }
            strcpy(variables[var_count].name, var_name);
            variables[var_count].type = INT;
            variables[var_count++].value.int_val = value;
            done: ;
        } else if (instruction == 0x05) {
            int len = * (int * )(bytecode + pc);
            pc += sizeof(int);
            char var_name[256];
            memcpy(var_name, bytecode + pc, len);
            var_name[len] = '\0';
            pc += len;
            int len_str = * (int * )(bytecode + pc);
            pc += sizeof(int);
            char char_val[256];
            memcpy(char_val, bytecode + pc, len_str);
            char_val[len_str] = '\0';
            pc += len_str;
            for (int i = 0; i < var_count; i++) {
                if (!strcmp(var_name, variables[i].name)) {
                    strcpy(variables[i].value.char_val, char_val);
                    goto done2;
                }
            }
            strcpy(variables[var_count].name, var_name);
            variables[var_count].type = CHAR;
            strcpy(variables[var_count++].value.char_val, char_val);
            done2: ;
        } else if (instruction == 0x06) {
            printf(" ");
        } else if (instruction == 0x07) { 
    int len = *(int *)(bytecode + pc);
    pc += sizeof(int);
    char var_name[256];
    memcpy(var_name, bytecode + pc, len);
    var_name[len] = '\0';
    pc += len;
    int src_len = *(int *)(bytecode + pc);
    pc += sizeof(int);
    char src_var[256];
    memcpy(src_var, bytecode + pc, src_len);
    src_var[src_len] = '\0';
    pc += src_len;

    int src_index = find_variable_index(src_var);
    if (src_index != -1) {
        int target_index = find_variable_index(var_name);
        if (target_index == -1) {
            handle_error("Target variable not found during assignment");
        }

        if (variables[src_index].type == CHAR) {
            strcpy(variables[target_index].value.char_val, variables[src_index].value.char_val);
        } else {
            variables[target_index].value.int_val = variables[src_index].value.int_val;
        }
    } else {
        handle_error("Source variable not found during assignment");
    }
}

    }
}

int main(int argc, char ** argv) {
    if (argc != 2) {
        handle_error("Usage: <input_file>");
    }

    char * source_code = read_file(argv[1]);
    compile_to_bytecode(source_code, "/tmp/output.bytecode");
    free(source_code);

    FILE * bytecode_file = fopen("/tmp/output.bytecode", "rb");
    fseek(bytecode_file, 0, SEEK_END);
    size_t bytecode_length = ftell(bytecode_file);
    fseek(bytecode_file, 0, SEEK_SET);
    unsigned char * bytecode = malloc(bytecode_length);
    fread(bytecode, 1, bytecode_length, bytecode_file);
    fclose(bytecode_file);

    run_bytecode(bytecode, bytecode_length);
    printf("\n");
    free(bytecode);

    return 0;
}
