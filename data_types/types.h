#ifndef INT_H
#define INT_H
#include <stdio.h>

void handle_int(FILE *output, const char **ptr, const char *var_name, int var_index, VarStorageType storage_type);
void handle_string(FILE *output, const char **ptr, const char *var_name, int var_index, VarStorageType storage_type);
void handle_float(FILE *output, const char **ptr, const char *var_name, int var_index, VarStorageType storage_type);
void handle_char(FILE *output, const char **ptr, const char *var_name, int var_index, VarStorageType storage_type);
void handle_bool(FILE *output, const char **ptr, const char *var_name, int var_index, VarStorageType storage_type);

#endif
