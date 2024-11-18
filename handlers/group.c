#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../headers/bytecode.h"

void handle_group(const char *args, FILE *output) {
    fwrite(&(unsigned char) { GROUP_OP }, 1, 1, output);

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

    char *group_start = strchr(args, '(');
    char *group_end = strrchr(args, ')');
    if (!group_start || !group_end || group_end <= group_start) {
	fprintf(stderr, "Error: Invalid group syntax\n");
	exit(ERR_MALFORMED_LIST);
    }

    size_t group_length = 0;
    long group_length_pos = ftell(output);
    fwrite(&group_length, sizeof(size_t), 1, output);

    Group *group = malloc(sizeof(Group));
    group->count = 0;
    group->elements = malloc(0);

    char *element = strtok(group_start + 1, ", ");
    while (element) {
	if (element[strlen(element) - 1] == ')') {
	element[strlen(element) - 1] = '\0';
	}

	Variable *var = malloc(sizeof(Variable));
	if (isdigit(*element) || (*element == '-' && isdigit(*(element + 1)))) {
    var->type = TYPE_INT;
    var->int_value = atoi(element);
    fwrite(&(unsigned char) { TYPE_INT }, 1, 1, output);
    fwrite(&var->int_value, sizeof(int), 1, output);
	} else if (*element == '"') {
    size_t str_len = strlen(element) - 2;
    var->type = TYPE_STRING;
    var->string_value = strndup(element + 1, str_len);
    fwrite(&(unsigned char) { TYPE_STRING }, 1, 1, output);
    fwrite(&str_len, sizeof(size_t), 1, output);
    fwrite(var->string_value, sizeof(char), str_len, output);
	} else {
    fprintf(stderr, "Error: Unsupported group element\n");
    free(var);
    exit(ERR_MALFORMED_LIST);
	}

	group->elements = realloc(group->elements, (group->count + 1) * sizeof(Variable *));
	group->elements[group->count++] = var;

	group_length++;
	element = strtok(NULL, ", ");
    }

    fseek(output, group_length_pos, SEEK_SET);
    fwrite(&group_length, sizeof(size_t), 1, output);
    fseek(output, 0, SEEK_END);

    if (set_variable(name, TYPE_GROUP, group) != 0) {
	fprintf(stderr, "Error: Could not register variable '%s'\n", name);
	exit(ERR_UNINITIALIZED_VARIABLE);
    }
}

void handle_group_opcode(FILE *input) {
    size_t name_len;
    if (fread(&name_len, sizeof(size_t), 1, input) != 1) {
	fprintf(stderr, "Error reading variable name length\n");
	exit(ERR_FILE_ERROR);
    }

    char name[32];
    if (fread(name, sizeof(char), name_len, input) != name_len) {
	fprintf(stderr, "Error reading variable name\n");
	exit(ERR_FILE_ERROR);
    }
    name[name_len] = '\0';

    size_t group_length;
    if (fread(&group_length, sizeof(size_t), 1, input) != 1) {
	fprintf(stderr, "Error reading group length\n");
	exit(ERR_FILE_ERROR);
    }

    Group *group = malloc(sizeof(Group));
    group->count = group_length;
    group->elements = malloc(group_length * sizeof(Variable *));

    for (size_t i = 0; i < group_length; i++) {
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
	}

	group->elements[i] = element;
    }

    set_variable(name, TYPE_GROUP, group);
}
