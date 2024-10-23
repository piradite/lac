#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdio.h>

typedef enum {
    INT,
    STRING,
    FLOAT,
    BOOL,
    CHAR
} VarType;

typedef struct {
    char name[256];
    VarType type;
    union {
        int int_val;
        char string_val[256];
        float float_val;
        int bool_val;
        char char_val;
    } value;
} Variable;

void compile_to_bytecode(const char *source_code, const char *bytecode_file);
int find_variable_index(const char *var_name);

#endif
