#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../headers/bytecode.h"

void handle_list(const char *args, FILE *output) {
    fwrite(&(unsigned char) { LIST_OP }, 1, 1, output);

    char name[32];
    size_t name_len = strcspn(args, ":");
    if (name_len >= sizeof(name)) {
	fprintf(stderr, "Error: Variable name too long\n");
	exit(ERR_MALFORMED_LIST);
    }
    strncpy(name, args, name_len);
    name[name_len] = '\0';

    fwrite(&name_len, sizeof(size_t), 1, output);
    fwrite(name, sizeof(char), name_len, output);

    char *list_start = strchr(args, '[');
    char *list_end = strrchr(args, ']');
    if (!list_start || !list_end || list_end <= list_start) {
	fprintf(stderr, "Error: Invalid list syntax\n");
	exit(ERR_MALFORMED_LIST);
    }

    size_t list_length = 0;
    long list_length_pos = ftell(output);
    fwrite(&list_length, sizeof(size_t), 1, output);

    List *list_var = malloc(sizeof(List));
    list_var->count = 0;
    size_t capacity = 10;
    list_var->elements = malloc(capacity * sizeof(Variable *));

    char *element = strtok(list_start + 1, ", ");
    while (element) {
	if (element[strlen(element) - 1] == ']') {
	    element[strlen(element) - 1] = '\0';
	}

	if (list_var->count >= capacity) {
	    capacity *= 2;
	    list_var->elements = realloc(list_var->elements, capacity * sizeof(Variable *));
	}

	Variable *new_element = malloc(sizeof(Variable));

	if (isdigit(*element) || (*element == '-' && isdigit(*(element + 1)))) {
		const char *str = element;
		if (*str == '-') str++;
		int dot_count = 0;
		
		while (*str && (isdigit(*str) || (*str == '.' && dot_count == 0))) { if (*str == '.') dot_count++; str++; }
		if (*str == '\0' && dot_count == 1) {
		float float_value = atof(element);
		fwrite(&(unsigned char) { TYPE_FLOAT }, 1, 1, output);
		fwrite(&float_value, sizeof(float), 1, output);
		new_element->type = TYPE_FLOAT;
		new_element->float_value = float_value;
		} else {
		int value = atoi(element);
		fwrite(&(unsigned char) { TYPE_INT }, 1, 1, output);
		fwrite(&value, sizeof(int), 1, output);
		new_element->type = TYPE_INT;
		new_element->int_value = value;
		}
	} else if (strcmp(element, "true") == 0 || strcmp(element, "false") == 0) {
	    unsigned char bool_value = (strcmp(element, "true") == 0) ? 1 : 0;
	    fwrite(&(unsigned char) { TYPE_BOOL }, 1, 1, output);
	    fwrite(&bool_value, sizeof(unsigned char), 1, output);
	    new_element->type = TYPE_BOOL;
	    new_element->bool_value = bool_value;
	} else if (*element == '"') {
	    size_t str_len = strlen(element) - 2;
	    fwrite(&(unsigned char) { TYPE_STRING }, 1, 1, output);
	    fwrite(&str_len, sizeof(size_t), 1, output);
	    fwrite(element + 1, sizeof(char), str_len, output);
	    new_element->type = TYPE_STRING;
	    new_element->string_value = strndup(element + 1, str_len);
	} else if (*element == '\'' && element[strlen(element) - 1] == '\'') {
	    if (strlen(element) != 3) {
		fprintf(stderr, "Error: Invalid char format\n");
		exit(ERR_MALFORMED_LIST);
	    }
	    char char_value = element[1];
	    fwrite(&(unsigned char) { TYPE_CHAR }, 1, 1, output);
	    fwrite(&char_value, sizeof(char), 1, output);
	    new_element->type = TYPE_CHAR;
	    new_element->char_value = char_value;
	} else {
	    fprintf(stderr, "Error: Unsupported list element\n");
	    exit(ERR_MALFORMED_LIST);
	}

	list_var->elements[list_var->count++] = new_element;
	element = strtok(NULL, ", ");
    }

    list_length = list_var->count;

    fseek(output, list_length_pos, SEEK_SET);
    fwrite(&list_length, sizeof(size_t), 1, output);
    fseek(output, 0, SEEK_END);

    set_variable(name, TYPE_LIST, list_var);
}

void handle_list_opcode(FILE *input) {
    size_t name_len;
    if (fread(&name_len, sizeof(size_t), 1, input) != 1) {
	fprintf(stderr, "Error reading variable name length\n");
	exit(ERR_FILE_ERROR);
    }

    if (name_len >= 32) {
	fprintf(stderr, "Variable name length exceeds limit\n");
	exit(ERR_MALFORMED_LIST);
    }

    char name[32];
    if (fread(name, sizeof(char), name_len, input) != name_len) {
	fprintf(stderr, "Error reading variable name\n");
	exit(ERR_FILE_ERROR);
    }
    name[name_len] = '\0';

    size_t list_length;
    if (fread(&list_length, sizeof(size_t), 1, input) != 1) {
	fprintf(stderr, "Error reading list length\n");
	exit(ERR_FILE_ERROR);
    }

    const size_t MAX_LIST_LENGTH = 10000;
    if (list_length > MAX_LIST_LENGTH) {
	fprintf(stderr, "Error: List length %zu exceeds the maximum allowed (%zu)\n", list_length, MAX_LIST_LENGTH);
	exit(ERR_MALFORMED_LIST);
    }

    List *list = malloc(sizeof(List));
    list->count = list_length;
    list->elements = malloc(list_length * sizeof(Variable *));

    for (size_t i = 0; i < list_length; i++) {
	unsigned char type;
	if (fread(&type, 1, 1, input) != 1) {
	    fprintf(stderr, "Error reading element type\n");
	    exit(ERR_FILE_ERROR);
	}

	Variable *element = malloc(sizeof(Variable));
	element->type = type;

	if (type == TYPE_INT) {
	    if (fread(&element->int_value, sizeof(int), 1, input) != 1) {
		fprintf(stderr, "Error reading integer value\n");
		exit(ERR_FILE_ERROR);
	    }
	} else if (type == TYPE_STRING) {
	    size_t str_len;
	    if (fread(&str_len, sizeof(size_t), 1, input) != 1) {
		fprintf(stderr, "Error reading string length\n");
		exit(ERR_FILE_ERROR);
	    }

	    element->string_value = malloc(str_len + 1);
	    if (fread(element->string_value, sizeof(char), str_len, input) != str_len) {
		fprintf(stderr, "Error reading string value\n");
		exit(ERR_FILE_ERROR);
	    }
	    element->string_value[str_len] = '\0';
	} else if (type == TYPE_BOOL) {
	    unsigned char bool_value;
	    if (fread(&bool_value, sizeof(unsigned char), 1, input) != 1) {
		fprintf(stderr, "Error reading boolean value\n");
		exit(ERR_FILE_ERROR);
	    }
	    element->bool_value = bool_value;
	} else if (type == TYPE_CHAR) {
	    char char_value;
	    if (fread(&char_value, sizeof(char), 1, input) != 1) {
		fprintf(stderr, "Error reading char value\n");
		exit(ERR_FILE_ERROR);
	    }
	    element->char_value = char_value;
	} else if (type == TYPE_FLOAT) {
		if (fread(&element->float_value, sizeof(float), 1, input) != 1) {
		fprintf(stderr, "Error reading float value\n");
		exit(ERR_FILE_ERROR);
	    }
	} else {
	    fprintf(stderr, "Unknown element type: %d\n", type);
	    exit(ERR_MALFORMED_LIST);
	}
	list->elements[i] = element;
    }

    set_variable(name, TYPE_LIST, list);
}
