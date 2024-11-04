#ifndef UTILITY_H
#define UTILITY_H
#include <stdio.h>
void handle_error(const char *msg);
char *read_file(const char *filename);
void process_print_statement(FILE *output, const char **ptr);

#endif
