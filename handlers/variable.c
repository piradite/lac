#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../headers/bytecode.h"

int set_variable(const char *name, VariableType type, void *value) {
    for (size_t i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
        if (variables[i].type == TYPE_STRING) {
        free(variables[i].string_value);
        } else if (variables[i].type == TYPE_LIST) {
        for (size_t j = 0; j < variables[i].list_value->count; j++) {
        free(variables[i].list_value->elements[j]);
        }
        free(variables[i].list_value->elements);
        free(variables[i].list_value);
        } else if (variables[i].type == TYPE_GROUP) {
        for (size_t j = 0; j < variables[i].group_value->count; j++) {
        free(variables[i].group_value->elements[j]);
        }
        free(variables[i].group_value->elements);
        free(variables[i].group_value);
        }

        variables[i].type = type;
        if (type == TYPE_INT) variables[i].int_value = *(int *)value;
        else if (type == TYPE_FLOAT) variables[i].float_value = *(float *)value;
        else if (type == TYPE_STRING) variables[i].string_value = strdup((char *)value);
        else if (type == TYPE_CHAR) variables[i].char_value = *(char *)value;
        else if (type == TYPE_BOOL) variables[i].bool_value = *(bool *)value;
        else if (type == TYPE_LIST) variables[i].list_value = (List *)value;
        else if (type == TYPE_GROUP) variables[i].group_value = (Group *)value;
        else if (type == TYPE_ENUM) { variables[i].enum_value = malloc(sizeof(Enum));
        memcpy(variables[i].enum_value, (Enum *)value, sizeof(Enum));
        }
        return 0;
        }
    }

    strncpy(variables[variable_count].name, name, sizeof(variables[variable_count].name) - 1);
    variables[variable_count].type = type;

    if (type == TYPE_INT) variables[variable_count].int_value = *(int *)value;
    else if (type == TYPE_FLOAT) variables[variable_count].float_value = *(float *)value;
    else if (type == TYPE_STRING) variables[variable_count].string_value = strdup((char *)value);
    else if (type == TYPE_CHAR) variables[variable_count].char_value = *(char *)value;
    else if (type == TYPE_BOOL) variables[variable_count].bool_value = *(bool *)value;
    else if (type == TYPE_LIST) variables[variable_count].list_value = (List *)value;
    else if (type == TYPE_GROUP) variables[variable_count].group_value = (Group *)value;
    else if (type == TYPE_ENUM) { variables[variable_count].enum_value = malloc(sizeof(Enum));
    memcpy(variables[variable_count].enum_value, (Enum *)value, sizeof(Enum));
    }

    variable_count++;
    return 0;
}

int get_variable(const char *name, Variable *var) {
    for (size_t i = 0; i < variable_count; i++) {
	if (strcmp(variables[i].name, name) == 0) {
	    *var = variables[i];
	    return 0;
	}
    }
    return -1;
}
