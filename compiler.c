#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/bytecode.h"

Instruction instruction_set[] = {
    { "print", PRINT_OP, handle_print },
    { "int", INT_OP, handle_int },
    { "float", FLOAT_OP, handle_float },
    { "string", FLOAT_OP, handle_string },
    { "char", CHAR_OP, handle_char },
    { "bool", BOOL_OP, handle_bool },
    { "list", LIST_OP, handle_list },
	{ "group", GROUP_OP, handle_group },
	{ "enum", ENUM_OP, handle_enum }
};

size_t INSTRUCTION_COUNT = sizeof(instruction_set) / sizeof(Instruction);

Variable variables[100];
size_t variable_count;

int handle_type_instruction(const char *line, FILE *output) {
    char *start = strstr(line, ": ");
    char *end = strstr(line, " ->");

    if (start && end && end > start) {
	size_t type_len = end - start - 2;
	char type[type_len + 1];
	strncpy(type, start + 2, type_len);
	type[type_len] = '\0';

	for (size_t i = 0; i < INSTRUCTION_COUNT; i++) {
	    if (strcmp(type, instruction_set[i].name) == 0) {
		instruction_set[i].handler(line, output);
		return 1;
	    }
	}
    }

    return 0;
}

void compile(const char *input_file, const char *output_file) {
    FILE *input = fopen(input_file, "r");
    FILE *output = fopen(output_file, "wb");
    if (!input || !output) {
	log_error("File error");
	exit(ERR_FILE_ERROR);
    }

    char line[256];
    char full_line[1024] = "";
    int in_block_comment = 0;

    while (fgets(line, sizeof(line), input)) {
	char *trimmed_line = line + strspn(line, " \t");
	trimmed_line[strcspn(trimmed_line, "\n")] = '\0';

	if (strlen(trimmed_line) == 0) {
	    continue;
	}
	if (in_block_comment) {
	    if (strstr(trimmed_line, "*;")) {
		in_block_comment = 0;
	    }
	    continue;
	}
	char *start_block_comment = strstr(trimmed_line, ";*");
	char *end_block_comment = strstr(trimmed_line, "*;");
    
	if (start_block_comment && !end_block_comment) {
	    in_block_comment = 1;
	    *start_block_comment = '\0';
	} else if (start_block_comment && end_block_comment) {
	    memmove(start_block_comment, end_block_comment + 2, strlen(end_block_comment + 2) + 1);
	}

	char *inline_comment = strstr(trimmed_line, ";;");

	if (inline_comment) {
	    *inline_comment = '\0';
	}

	trimmed_line = line + strspn(line, " \t");
	trimmed_line[strcspn(trimmed_line, "\n")] = '\0';

	if (strlen(trimmed_line) == 0) {
	    continue;
	}

	size_t len = strlen(trimmed_line);
	if (trimmed_line[len - 1] == '\\') {
		trimmed_line[len - 1] = '\0';
		strcat(full_line, trimmed_line);
		continue;
	} else {
		strcat(full_line, trimmed_line);
	}

	if (handle_type_instruction(full_line, output)) {
		full_line[0] = '\0';
		continue;
	}

	char *command = strtok(full_line, " ");
	char *args = strtok(NULL, "\n");
	int found = 0;

	for (size_t i = 0; i < INSTRUCTION_COUNT; i++) {
	    if (command && strcmp(command, instruction_set[i].name) == 0) {
		instruction_set[i].handler(args, output);
		found = 1;
		break;
	    }
	}
	
	if (!found) {
	    char error_msg[256];
	    snprintf(error_msg, sizeof(error_msg), "Unknown instruction '%s'", command ? command : "NULL");
	    log_error(error_msg);
	    exit(ERR_UNKNOWN_INSTRUCTION);
	}

	full_line[0] = '\0';
    }

    fclose(input);
    fclose(output);
}
