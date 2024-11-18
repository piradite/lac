#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../headers/bytecode.h"

void log_error(const char *message) {
    fprintf(stderr, "Error: %s\n", message);
}

void log_message(const char *message) {
    printf("%s\n", message);
}
