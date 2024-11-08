#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
typedef enum {
    NONE,
    INT,
    STRING,
    FLOAT,
    BOOL,
    CHAR,
    ANY
} VarType;

typedef enum {
    VAR,
    LET,
    CONST
} VarStorageType;

typedef struct {
    char name[256];
    VarType type;
    VarStorageType storage_type;
    bool is_deleted;
    union {
	int32_t int_val;
	char string_val[256];
	float float_val;
	int bool_val;
	char char_val;
    } value;
} Variable;

void compile_to_bytecode(const char *source_code, const char *bytecode_file);
int find_variable_index(const char *var_name);
void write_to_output(FILE *output, char byte_code, int len, const char *name, void *data, size_t data_size);

#endif
