#include <stdio.h>
#include <stdlib.h>
#include "headers/bytecode.h"

void compile(const char *input_file, const char *output_file);
void interpret(const char *bytecode_file);

int main(int argc, char *argv[]) {
    if (argc != 2) {
	fprintf(stderr, "Usage: %s <input.pa>\n", argv[0]);
	return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = "output.bytecode";

    compile(input_file, output_file);

    interpret(output_file);

    return 0;
}
