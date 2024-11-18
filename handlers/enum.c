#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "../headers/bytecode.h"

void handle_enum(const char *args, FILE *output) {
    fwrite(&(unsigned char) { ENUM_OP }, 1, 1, output);

    char name[32];
    size_t name_len;
    char *colon_pos = strchr(args, ':');
    char *paren_pos = strchr(args, '(');

    if (!paren_pos || (colon_pos && colon_pos < paren_pos)) {
        name_len = colon_pos - args;
    } else {
        name_len = paren_pos - args;
    }

    if (name_len >= sizeof(name)) {
        fprintf(stderr, "Error: Enum name too long\n");
        exit(ERR_MALFORMED_LIST);
    }

    strncpy(name, args, name_len);
    name[name_len] = '\0';

    fwrite(&name_len, sizeof(size_t), 1, output);
    fwrite(name, sizeof(char), name_len, output);

    Enum *enum_var = malloc(sizeof(Enum));
    if (!enum_var) {
        fprintf(stderr, "Error: Memory allocation for enum_var failed\n");
        exit(ERR_MALFORMED_LIST);
    }
    strncpy(enum_var->name, name, sizeof(enum_var->name));
    enum_var->count = 0;

    char *entries_start = strchr(args, '(');
    char *entries_end = strrchr(args, ')');
    if (!entries_start || !entries_end || entries_end <= entries_start) {
        fprintf(stderr, "Error: Invalid enum syntax\n");
        free(enum_var);
        exit(ERR_MALFORMED_LIST);
    }

    size_t entries_length = entries_end - entries_start - 1;
    char entries[entries_length + 1];
    strncpy(entries, entries_start + 1, entries_length);
    entries[entries_length] = '\0';

    char *entry = strtok(entries, ",");
    while (entry) {
        while (isspace((unsigned char)*entry))
            entry++;
        char *end = entry + strlen(entry) - 1;
        while (end > entry && isspace((unsigned char)*end))
            *end-- = '\0';

        char *colon_pos = strchr(entry, ':');
        if (!colon_pos) {
            fprintf(stderr, "Error: Invalid enum entry format\n");
            free(enum_var);
            exit(ERR_MALFORMED_LIST);
        }

        *colon_pos = '\0';
        const char *key = entry;
        char *value_str = colon_pos + 1;

        while (isspace((unsigned char)*value_str))
            value_str++;
        char *value_end = value_str + strlen(value_str) - 1;
        while (value_end > value_str && isspace((unsigned char)*value_end))
            *value_end-- = '\0';

        for (const char *ch = key; *ch; ch++) {
            if (!isupper((unsigned char)*ch)) {
                fprintf(stderr, "Error: Enum keys must be uppercase, but found '%s'\n", key);
                free(enum_var);
                exit(ERR_MALFORMED_LIST);
            }
        }

        for (const char *ch = value_str; *ch; ch++) {
            if (!isdigit((unsigned char)*ch)) {
                fprintf(stderr, "Error: Enum values must be numeric, but found '%s'\n", value_str);
                free(enum_var);
                exit(ERR_MALFORMED_LIST);
            }
        }

        int value = atoi(value_str);

        if (enum_var->count >= 100) {
            fprintf(stderr, "Error: Too many enum entries\n");
            free(enum_var);
            exit(ERR_MALFORMED_LIST);
        }

        strncpy(enum_var->entries[enum_var->count].key, key, sizeof(enum_var->entries[enum_var->count].key));
        enum_var->entries[enum_var->count].value = value;
        enum_var->count++;
        entry = strtok(NULL, ",");
    }

    fwrite(&enum_var->count, sizeof(size_t), 1, output);

    for (size_t i = 0; i < enum_var->count; i++) {
        size_t key_len = strlen(enum_var->entries[i].key);
        fwrite(&key_len, sizeof(size_t), 1, output);
        fwrite(enum_var->entries[i].key, sizeof(char), key_len, output);
        fwrite(&enum_var->entries[i].value, sizeof(int), 1, output);
    }

    if (set_variable(name, TYPE_ENUM, enum_var) != 0) {
        fprintf(stderr, "Error: Could not register enum '%s'\n", name);
        free(enum_var);
        exit(ERR_UNINITIALIZED_VARIABLE);
    }
}

void handle_enum_opcode(FILE *input) {
    size_t name_len;
    if (fread(&name_len, sizeof(size_t), 1, input) != 1) {
        fprintf(stderr, "Error reading enum name length\n");
        exit(ERR_FILE_ERROR);
    }

    char name[32];
    if (name_len >= sizeof(name)) {
        fprintf(stderr, "Error: Enum name too long\n");
        exit(ERR_FILE_ERROR);
    }
    if (fread(name, sizeof(char), name_len, input) != name_len) {
        fprintf(stderr, "Error reading enum name\n");
        exit(ERR_FILE_ERROR);
    }
    name[name_len] = '\0';

    Enum *enum_var = malloc(sizeof(Enum));
    if (!enum_var) {
        fprintf(stderr, "Error: Memory allocation for enum_var failed\n");
        exit(ERR_FILE_ERROR);
    }
    strncpy(enum_var->name, name, sizeof(enum_var->name));
    enum_var->count = 0;

    size_t entries_count;
    if (fread(&entries_count, sizeof(size_t), 1, input) != 1) {
        fprintf(stderr, "Error reading enum entries count\n");
        free(enum_var);
        exit(ERR_FILE_ERROR);
    }

    if (entries_count > 100) {
        fprintf(stderr, "Error: Too many enum entries\n");
        free(enum_var);
        exit(ERR_FILE_ERROR);
    }

    for (size_t i = 0; i < entries_count; i++) {
        size_t key_len;
        if (fread(&key_len, sizeof(size_t), 1, input) != 1) {
            fprintf(stderr, "Error reading enum key length\n");
            free(enum_var);
            exit(ERR_FILE_ERROR);
        }

        char key[32];
        if (key_len >= sizeof(key)) {
            fprintf(stderr, "Error: Enum key too long\n");
            free(enum_var);
            exit(ERR_FILE_ERROR);
        }

        if (fread(key, sizeof(char), key_len, input) != key_len) {
            fprintf(stderr, "Error reading enum key\n");
            free(enum_var);
            exit(ERR_FILE_ERROR);
        }
        key[key_len] = '\0';

        for (const char *ch = key; *ch; ch++) {
            if (!isupper((unsigned char)*ch)) {
                fprintf(stderr, "Error: Enum keys must be uppercase\n");
                free(enum_var);
                exit(ERR_FILE_ERROR);
            }
        }

        int value;
        if (fread(&value, sizeof(int), 1, input) != 1) {
            fprintf(stderr, "Error reading enum value\n");
            free(enum_var);
            exit(ERR_FILE_ERROR);
        }

        strncpy(enum_var->entries[enum_var->count].key, key, sizeof(enum_var->entries[enum_var->count].key));
        enum_var->entries[enum_var->count].value = value;
        enum_var->count++;
    }

    if (set_variable(name, TYPE_ENUM, enum_var) != 0) {
        fprintf(stderr, "Error: Could not register enum '%s'\n", name);
        free(enum_var);
        exit(ERR_UNINITIALIZED_VARIABLE);
    }
}
