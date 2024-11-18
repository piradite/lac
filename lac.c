#include <stdio.h>
#include <stdlib.h>
#include "headers/bytecode.h"

Opcode opcode_set[] = {
    { PRINT_OP, handle_print_opcode },
    { INT_OP, handle_int_opcode },
    { FLOAT_OP, handle_float_opcode },
    { STRING_OP, handle_string_opcode },
    { CHAR_OP, handle_char_opcode },
    { BOOL_OP, handle_bool_opcode },
    { LIST_OP, handle_list_opcode },
    { GROUP_OP, handle_group_opcode },
    { ENUM_OP, handle_enum_opcode }
};

size_t OPCODE_COUNT = sizeof(opcode_set) / sizeof(Opcode);

void interpret(const char *bytecode_file) {
    FILE *input = fopen(bytecode_file, "rb");
    if (!input) {
	perror("Failed to open bytecode file");
	exit(1);
    }

    unsigned char opcode;
    while (fread(&opcode, sizeof(unsigned char), 1, input)) {
	int found = 0;
	for (size_t i = 0; i < OPCODE_COUNT; i++) {
	    if (opcode == opcode_set[i].opcode) {
		opcode_set[i].handler(input);
		found = 1;
		break;
	    }
	}

	if (!found) {
	    fprintf(stderr, "Error: Unknown opcode '%u'\n", opcode);
	    exit(1);
	}
    }

    fclose(input);
}
