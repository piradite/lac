#ifndef VARIABLES_H
#define VARIABLES_H

#include "../bytecode.h"

extern Variable variables[256];
extern int var_count;
void assign_variable_value(int var_index, const char *var_name, VarType expected_type, VarStorageType storage_type, void *value, size_t value_size);
#endif
