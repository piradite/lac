#include "bytecode.h"
#include "executable.h"
#include "util/utility.h"
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        handle_error("Usage: <executable> <source file>");
    }

    const char *executable_name = argv[1];
    char *source_code = read_file(argv[2]);
    compile_to_bytecode(source_code, "/tmp/output.bytecode");
    free(source_code);

    generate_executable("/tmp/output.bytecode", "/tmp/temp.c");

    char command[256];
    snprintf(command, sizeof(command), "gcc /tmp/temp.c -o %s", executable_name);
    system(command);

    return 0;
}
