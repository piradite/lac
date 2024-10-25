#include "bytecode.h"
#include "executable.h"
#include "util/utility.h"
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 3) handle_error("Usage: <executable> <source file>");

    char *source_code = read_file(argv[2]);
    compile_to_bytecode(source_code, "/tmp/output.bytecode");
    free(source_code);

    char command[256];
    generate_executable("/tmp/output.bytecode", "/tmp/temp.c");
    snprintf(command, sizeof(command), "gcc /tmp/temp.c -o %s", argv[1]);
    system(command);

    return 0;
}
