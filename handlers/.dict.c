#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../headers/bytecode.h"

void handle_dict(const char *args, FILE *output) {
    fwrite(&(unsigned char){ GROUP_OP }, 1, 1, output);

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
        fprintf(stderr, "Error: Dict name too long\n");
        exit(ERR_MALFORMED_LIST);
    }

    strncpy(name, args, name_len);
    name[name_len] = '\0';

    fwrite(&name_len, sizeof(size_t), 1, output);
    fwrite(name, sizeof(char), name_len, output);

    Dict *dict_var = malloc(sizeof(Dict));
    if (!dict_var) {
        fprintf(stderr, "Error: Memory allocation for dict_var failed\n");
        exit(ERR_MALFORMED_LIST);
    }
    strncpy(dict_var->name, name, sizeof(dict_var->name));
    dict_var->count = 0;

    char *entries_start = strchr(args, '(');
    char *entries_end = strrchr(args, ')');
    if (!entries_start || !entries_end || entries_end <= entries_start) {
        fprintf(stderr, "Error: Invalid dict syntax\n");
        free(dict_var);
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
            fprintf(stderr, "Error: Invalid dict entry format\n");
            free(dict_var);
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

        DictEntry *entry_var = &dict_var->entries[dict_var->count];
        strncpy(entry_var->key, key, sizeof(entry_var->key));

        if (value_str[0] == '"' && value_str[strlen(value_str) - 1] == '"') {
            // String value
            entry_var->value.type = TYPE_STRING;
            value_str[strlen(value_str) - 1] = '\0';
            entry_var->value.string_value = strdup(value_str + 1);
        } else if (isdigit((unsigned char)value_str[0]) || value_str[0] == '-') {
            // Numeric value
            if (strchr(value_str, '.')) {
                entry_var->value.type = TYPE_FLOAT;
                entry_var->value.float_value = atof(value_str);
            } else {
                entry_var->value.type = TYPE_INT;
                entry_var->value.int_value = atoi(value_str);
            }
        } else if (value_str[0] == '\'' && value_str[strlen(value_str) - 1] == '\'') {
            // Char value
            entry_var->value.type = TYPE_CHAR;
            entry_var->value.char_value = value_str[1];
        } else {
            fprintf(stderr, "Error: Unknown dict value type\n");
            free(dict_var);
            exit(ERR_MALFORMED_LIST);
        }

        dict_var->count++;
        if (dict_var->count > 100) {
            fprintf(stderr, "Error: Too many dict entries\n");
            free(dict_var);
            exit(ERR_MALFORMED_LIST);
        }

        entry = strtok(NULL, ",");
    }

    fwrite(&dict_var->count, sizeof(size_t), 1, output);

    for (size_t i = 0; i < dict_var->count; i++) {
        size_t key_len = strlen(dict_var->entries[i].key);
        fwrite(&key_len, sizeof(size_t), 1, output);
        fwrite(dict_var->entries[i].key, sizeof(char), key_len, output);

        Variable *value = &dict_var->entries[i].value;
        fwrite(&value->type, sizeof(VariableType), 1, output);
        switch (value->type) {
            case TYPE_STRING:
                {
                    size_t len = strlen(value->string_value);
                    fwrite(&len, sizeof(size_t), 1, output);
                    fwrite(value->string_value, sizeof(char), len, output);
                }
                break;
            case TYPE_INT:
                fwrite(&value->int_value, sizeof(int), 1, output);
                break;
            case TYPE_FLOAT:
                fwrite(&value->float_value, sizeof(float), 1, output);
                break;
            case TYPE_CHAR:
                fwrite(&value->char_value, sizeof(char), 1, output);
                break;
            default:
                fprintf(stderr, "Error: Unsupported dict value type\n");
                free(dict_var);
                exit(ERR_MALFORMED_LIST);
        }
    }

    if (set_variable(name, TYPE_DICT, dict_var) != 0) {
        fprintf(stderr, "Error: Could not register dict '%s'\n", name);
        free(dict_var);
        exit(ERR_UNINITIALIZED_VARIABLE);
    }
}

void handle_dict_opcode(FILE *input) {
    size_t name_len;
    if (fread(&name_len, sizeof(size_t), 1, input) != 1) {
        fprintf(stderr, "Error reading dict name length\n");
        exit(ERR_FILE_ERROR);
    }

    char name[32];
    if (name_len >= sizeof(name)) {
        fprintf(stderr, "Error: Dict name too long\n");
        exit(ERR_FILE_ERROR);
    }
    if (fread(name, sizeof(char), name_len, input) != name_len) {
        fprintf(stderr, "Error reading dict name\n");
        exit(ERR_FILE_ERROR);
    }
    name[name_len] = '\0';

    Dict *dict_var = malloc(sizeof(Dict));
    if (!dict_var) {
        fprintf(stderr, "Error: Memory allocation for dict_var failed\n");
        exit(ERR_FILE_ERROR);
    }
    strncpy(dict_var->name, name, sizeof(dict_var->name));
    dict_var->count = 0;

    size_t entries_count;
    if (fread(&entries_count, sizeof(size_t), 1, input) != 1) {
        fprintf(stderr, "Error reading dict entries count\n");
        free(dict_var);
        exit(ERR_FILE_ERROR);
    }

    if (entries_count > 100) {
        fprintf(stderr, "Error: Too many dict entries\n");
        free(dict_var);
        exit(ERR_FILE_ERROR);
    }

    for (size_t i = 0; i < entries_count; i++) {
        size_t key_len;
        if (fread(&key_len, sizeof(size_t), 1, input) != 1) {
            fprintf(stderr, "Error reading dict key length\n");
            free(dict_var);
            exit(ERR_FILE_ERROR);
        }

        char key[32];
        if (key_len >= sizeof(key)) {
            fprintf(stderr, "Error: Dict key too long\n");
            free(dict_var);
            exit(ERR_FILE_ERROR);
        }

        if (fread(key, sizeof(char), key_len, input) != key_len) {
            fprintf(stderr, "Error reading dict key\n");
            free(dict_var);
            exit(ERR_FILE_ERROR);
        }
        key[key_len] = '\0';

        DictEntry *entry = &dict_var->entries[dict_var->count];
        strncpy(entry->key, key, sizeof(entry->key));

        VariableType type;
        if (fread(&type, sizeof(VariableType), 1, input) != 1) {
            fprintf(stderr, "Error reading dict value type\n");
            free(dict_var);
            exit(ERR_FILE_ERROR);
        }

        entry->value.type = type;
        switch (type) {
            case TYPE_STRING: {
                size_t len;
                if (fread(&len, sizeof(size_t), 1, input) != 1) {
                    fprintf(stderr, "Error reading string value length\n");
                    free(dict_var);
                    exit(ERR_FILE_ERROR);
                }
                char *value = malloc(len + 1);
                if (!value) {
                    fprintf(stderr, "Error allocating memory for string value\n");
                    free(dict_var);
                    exit(ERR_FILE_ERROR);
                }
                if (fread(value, sizeof(char), len, input) != len) {
                    fprintf(stderr, "Error reading string value\n");
                    free(value);
                    free(dict_var);
                    exit(ERR_FILE_ERROR);
                }
                value[len] = '\0';
                entry->value.string_value = value;
            } break;

            case TYPE_INT:
                if (fread(&entry->value.int_value, sizeof(int), 1, input) != 1) {
                    fprintf(stderr, "Error reading int value\n");
                    free(dict_var);
                    exit(ERR_FILE_ERROR);
                }
                break;

            case TYPE_FLOAT:
                if (fread(&entry->value.float_value, sizeof(float), 1, input) != 1) {
                    fprintf(stderr, "Error reading float value\n");
                    free(dict_var);
                    exit(ERR_FILE_ERROR);
                }
                break;

            case TYPE_CHAR:
                if (fread(&entry->value.char_value, sizeof(char), 1, input) != 1) {
                    fprintf(stderr, "Error reading char value\n");
                    free(dict_var);
                    exit(ERR_FILE_ERROR);
                }
                break;

            default:
                fprintf(stderr, "Error: Unsupported dict value type\n");
                free(dict_var);
                exit(ERR_FILE_ERROR);
        }

        dict_var->count++;
    }

    if (set_variable(name, TYPE_DICT, dict_var) != 0) {
        fprintf(stderr, "Error: Could not register dict '%s'\n", name);
        free(dict_var);
        exit(ERR_UNINITIALIZED_VARIABLE);
    }
}
