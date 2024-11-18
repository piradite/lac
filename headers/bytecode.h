#ifndef BYTECODE_H
#define BYTECODE_H
#include <stdio.h>
#include <stdbool.h>

typedef enum {
    PRINT_OP = 0x01,
    INT_OP = 0x02,
    FLOAT_OP = 0x03,
    STRING_OP = 0x04,
    CHAR_OP = 0x05,
    BOOL_OP = 0x06,
    LIST_OP = 0x10,
    GROUP_OP = 0x11,
    ENUM_OP = 0x12,
} OpcodeEnum;

typedef enum {
    ERR_UNKNOWN_INSTRUCTION = -1,
    ERR_FILE_ERROR = -2,
    ERR_MALFORMED_PRINT = -3,
    ERR_UNINITIALIZED_VARIABLE = -4,
    ERR_MALFORMED_LIST = -5,
    ERR_OUT_OF_BOUNDS = -6
} ErrorCode;

typedef void (*InstructionHandler)(const char *args, FILE * output);
typedef void (*OpcodeHandler)(FILE * input);

typedef struct {
    const char *name;
    unsigned char opcode;
    InstructionHandler handler;
} Instruction;

typedef struct {
    unsigned char opcode;
    OpcodeHandler handler;
} Opcode;

typedef struct {
    char name[32];
    size_t count;
    struct {
        char key[32];
        int value;
    } entries[100];
} Enum;

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_CHAR,
    TYPE_BOOL,
    TYPE_LIST,
    TYPE_GROUP,
    TYPE_ENUM,
} VariableType;

struct Variable;

typedef struct {
    struct Variable **elements;
    size_t count;
} List;

typedef struct {
    VariableType element_type;
    struct Variable **elements;
    size_t count;
} Group;


typedef struct Variable {
    char name[32];
    VariableType type;
    union {
	int int_value;
	float float_value;
	char *string_value;
	char char_value;
	bool bool_value;
	List *list_value;
    Group *group_value;
    Enum *enum_value;
    };
} Variable;

extern Variable variables[100];
extern size_t variable_count;
extern Instruction instruction_set[];
extern size_t INSTRUCTION_COUNT;
extern Opcode opcode_set[];
extern size_t OPCODE_COUNT;

void handle_print(const char *args, FILE * output);
void handle_print_opcode(FILE * input);

void handle_int(const char *args, FILE * output);
void handle_int_opcode(FILE * input);

void handle_float(const char *args, FILE * output);
void handle_float_opcode(FILE * input);

void handle_string(const char *args, FILE * output);
void handle_string_opcode(FILE * input);

void handle_char(const char *args, FILE * output);
void handle_char_opcode(FILE * input);

void handle_bool(const char *args, FILE * output);
void handle_bool_opcode(FILE * input);

void handle_list(const char *args, FILE * output);
void handle_list_opcode(FILE * input);

void handle_group(const char *args, FILE * output);
void handle_group_opcode(FILE * input);

void handle_enum(const char *args, FILE * output);
void handle_enum_opcode(FILE * input);

int set_variable(const char *name, VariableType type, void *value);
int get_variable(const char *name, Variable * var);

void compile(const char *input_file, const char *output_file);

void log_error(const char *message);
void log_message(const char *message);

#endif
