#include "../include/parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t encode_r_type(uint32_t opcode, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t shamt) {
    return (opcode << 28) | (r1 << 23) | (r2 << 18) | (r3 << 13) | (shamt & 0x1FFF);
}

static uint32_t encode_i_type(uint32_t opcode, uint32_t r1, uint32_t r2, int32_t imm) {
    return (opcode << 28) | (r1 << 23) | (r2 << 18) | ((uint32_t)imm & 0x3FFFF);
}

static uint32_t encode_j_type(uint32_t opcode, uint32_t addr) {
    return (opcode << 28) | (addr & 0x0FFFFFFF);
}

static int strings_equal_ignore_case(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static void normalize_instruction(char *output, const char *input) {
    char *write = output;
    int last_was_space = 1;

    while (*input != '\0') {
        if (*input == ',') {
            if (!last_was_space) {
                *write++ = ' ';
                last_was_space = 1;
            }
        } else if (isspace((unsigned char)*input)) {
            if (!last_was_space) {
                *write++ = ' ';
                last_was_space = 1;
            }
        } else {
            *write++ = *input;
            last_was_space = 0;
        }
        input++;
    }

    if (write > output && *(write - 1) == ' ') {
        write--;
    }
    *write = '\0';
}

static int split_tokens(char *text, char **tokens, size_t max_tokens) {
    size_t count = 0;
    char *token = strtok(text, " ");
    while (token != NULL && count < max_tokens) {
        tokens[count++] = token;
        token = strtok(NULL, " ");
    }
    return (int)count;
}

static int parse_register(const char *reg) {
    if (reg == NULL || (reg[0] != 'R' && reg[0] != 'r')) {
        return -1;
    }

    char *end = NULL;
    long value = strtol(reg + 1, &end, 10);
    if (*end != '\0' || value < 0 || value > 31) {
        return -1;
    }
    return (int)value;
}

static int parse_signed_18(const char *text, int32_t *result) {
    char *end = NULL;
    long value = strtol(text, &end, 0);
    if (text[0] == '\0' || *end != '\0' || value < -131072L || value > 131071L) {
        return 0;
    }
    *result = (int32_t)value;
    return 1;
}

static int parse_unsigned_28(const char *text, uint32_t *result) {
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 0);
    if (text[0] == '\0' || *end != '\0' || value > 0x0FFFFFFFUL) {
        return 0;
    }
    *result = (uint32_t)value;
    return 1;
}

static int parse_shamt_13(const char *text, uint32_t *result) {
    if (text == NULL || *text == '\0') {
        return 0;
    }

    char *end = NULL;
    unsigned long value = strtoul(text, &end, 0);
    if (*end != '\0' || value > 0x1FFFUL) {
        return 0;
    }
    *result = (uint32_t)value;
    return 1;
}

static void set_error_message(const char *format,
                              const char *target,
                              char *error_message,
                              size_t error_message_size) {
    if (error_message == NULL || error_message_size == 0) {
        return;
    }

    if (target != NULL) {
        snprintf(error_message, error_message_size, format, target);
    } else {
        strncpy(error_message, format, error_message_size - 1);
        error_message[error_message_size - 1] = '\0';
    }
}

int parse_and_encode_instruction(const char *line,
                                 uint32_t *encoded,
                                 char *error_message,
                                 size_t error_message_size) {
    if (line == NULL || encoded == NULL) {
        set_error_message("Invalid instruction format", NULL, error_message, error_message_size);
        return 0;
    }

    char normalized[256] = {0};
    normalize_instruction(normalized, line);

    char *tokens[5] = {0};
    int count = split_tokens(normalized, tokens, 5);
    if (count < 2) {
        set_error_message("Invalid instruction format", NULL, error_message, error_message_size);
        return 0;
    }

    const char *mnemonic = tokens[0];

    /* ---------------------------- R-Type Instructions ---------------------------- */

    if (strings_equal_ignore_case(mnemonic, "ADD") && count == 4) {
        int r1 = parse_register(tokens[1]);
        int r2 = parse_register(tokens[2]);
        int r3 = parse_register(tokens[3]);
        if (r1 < 0 || r2 < 0 || r3 < 0) {
            set_error_message("Invalid register %s", r1 < 0 ? tokens[1] : (r2 < 0 ? tokens[2] : tokens[3]), error_message, error_message_size);
            return 0;
        }
        *encoded = encode_r_type(OPCODE_ADD, (uint32_t)r1, (uint32_t)r2, (uint32_t)r3, 0);
        return 1;
    }

    if (strings_equal_ignore_case(mnemonic, "SUB") && count == 4) {
        int r1 = parse_register(tokens[1]);
        int r2 = parse_register(tokens[2]);
        int r3 = parse_register(tokens[3]);
        if (r1 < 0 || r2 < 0 || r3 < 0) {
            set_error_message("Invalid register %s", r1 < 0 ? tokens[1] : (r2 < 0 ? tokens[2] : tokens[3]), error_message, error_message_size);
            return 0;
        }
        *encoded = encode_r_type(OPCODE_SUB, (uint32_t)r1, (uint32_t)r2, (uint32_t)r3, 0);
        return 1;
    }

    if (strings_equal_ignore_case(mnemonic, "MUL") && count == 4) {
        int r1 = parse_register(tokens[1]);
        int r2 = parse_register(tokens[2]);
        int r3 = parse_register(tokens[3]);
        if (r1 < 0 || r2 < 0 || r3 < 0) {
            set_error_message("Invalid register %s", r1 < 0 ? tokens[1] : (r2 < 0 ? tokens[2] : tokens[3]), error_message, error_message_size);
            return 0;
        }
        *encoded = encode_r_type(OPCODE_MUL, (uint32_t)r1, (uint32_t)r2, (uint32_t)r3, 0);
        return 1;
    }

    if (strings_equal_ignore_case(mnemonic, "AND") && count == 4) {
        int r1 = parse_register(tokens[1]);
        int r2 = parse_register(tokens[2]);
        int r3 = parse_register(tokens[3]);
        if (r1 < 0 || r2 < 0 || r3 < 0) {
            set_error_message("Invalid register %s", r1 < 0 ? tokens[1] : (r2 < 0 ? tokens[2] : tokens[3]), error_message, error_message_size);
            return 0;
        }
        *encoded = encode_r_type(OPCODE_AND, (uint32_t)r1, (uint32_t)r2, (uint32_t)r3, 0);
        return 1;
    }

    if (strings_equal_ignore_case(mnemonic, "LSL") && count == 4) {
        int r1 = parse_register(tokens[1]);
        int r2 = parse_register(tokens[2]);
        uint32_t shamt = 0;
        if (r1 < 0 || r2 < 0) {
            set_error_message("Invalid register %s", r1 < 0 ? tokens[1] : tokens[2], error_message, error_message_size);
            return 0;
        }
        if (!parse_shamt_13(tokens[3], &shamt)) {
            set_error_message("Invalid shift amount %s", tokens[3], error_message, error_message_size);
            return 0;
        }
        *encoded = encode_r_type(OPCODE_LSL, (uint32_t)r1, (uint32_t)r2, 0, shamt);
        return 1;
    }

    if (strings_equal_ignore_case(mnemonic, "LSR") && count == 4) {
        int r1 = parse_register(tokens[1]);
        int r2 = parse_register(tokens[2]);
        uint32_t shamt = 0;
        if (r1 < 0 || r2 < 0) {
            set_error_message("Invalid register %s", r1 < 0 ? tokens[1] : tokens[2], error_message, error_message_size);
            return 0;
        }
        if (!parse_shamt_13(tokens[3], &shamt)) {
            set_error_message("Invalid shift amount %s", tokens[3], error_message, error_message_size);
            return 0;
        }
        *encoded = encode_r_type(OPCODE_LSR, (uint32_t)r1, (uint32_t)r2, 0, shamt);
        return 1;
    }

    /* ---------------------------- I-Type Instructions ---------------------------- */

    if (strings_equal_ignore_case(mnemonic, "MOVI") && count == 3) {
        int r1 = parse_register(tokens[1]);
        int32_t imm = 0;
        if (r1 < 0) {
            set_error_message("Invalid register %s", tokens[1], error_message, error_message_size);
            return 0;
        }
        if (!parse_signed_18(tokens[2], &imm)) {
            set_error_message("Invalid immediate %s", tokens[2], error_message, error_message_size);
            return 0;
        }
        *encoded = encode_i_type(OPCODE_MOVI, (uint32_t)r1, 0, imm);
        return 1;
    }

    if (strings_equal_ignore_case(mnemonic, "ORI") && count == 4) {
        int r1 = parse_register(tokens[1]);
        int r2 = parse_register(tokens[2]);
        int32_t imm = 0;
        if (r1 < 0 || r2 < 0) {
            set_error_message("Invalid register %s", r1 < 0 ? tokens[1] : tokens[2], error_message, error_message_size);
            return 0;
        }
        if (!parse_signed_18(tokens[3], &imm)) {
            set_error_message("Invalid immediate %s", tokens[3], error_message, error_message_size);
            return 0;
        }
        *encoded = encode_i_type(OPCODE_ORI, (uint32_t)r1, (uint32_t)r2, imm);
        return 1;
    }

    if (strings_equal_ignore_case(mnemonic, "JEQ") && count == 4) {
        int r1 = parse_register(tokens[1]);
        int r2 = parse_register(tokens[2]);
        int32_t imm = 0;
        if (r1 < 0 || r2 < 0) {
            set_error_message("Invalid register %s", r1 < 0 ? tokens[1] : tokens[2], error_message, error_message_size);
            return 0;
        }
        if (!parse_signed_18(tokens[3], &imm)) {
            set_error_message("Invalid immediate %s", tokens[3], error_message, error_message_size);
            return 0;
        }
        *encoded = encode_i_type(OPCODE_JEQ, (uint32_t)r1, (uint32_t)r2, imm);
        return 1;
    }

    if (strings_equal_ignore_case(mnemonic, "MOVR") && count == 4) {
        int r1 = parse_register(tokens[1]);
        int r2 = parse_register(tokens[2]);
        int32_t imm = 0;
        if (r1 < 0 || r2 < 0) {
            set_error_message("Invalid register %s", r1 < 0 ? tokens[1] : tokens[2], error_message, error_message_size);
            return 0;
        }
        if (!parse_signed_18(tokens[3], &imm)) {
            set_error_message("Invalid immediate %s", tokens[3], error_message, error_message_size);
            return 0;
        }
        *encoded = encode_i_type(OPCODE_MOVR, (uint32_t)r1, (uint32_t)r2, imm);
        return 1;
    }

    if (strings_equal_ignore_case(mnemonic, "MOVM") && count == 4) {
        int r1 = parse_register(tokens[1]);
        int r2 = parse_register(tokens[2]);
        int32_t imm = 0;
        if (r1 < 0 || r2 < 0) {
            set_error_message("Invalid register %s", r1 < 0 ? tokens[1] : tokens[2], error_message, error_message_size);
            return 0;
        }
        if (!parse_signed_18(tokens[3], &imm)) {
            set_error_message("Invalid immediate %s", tokens[3], error_message, error_message_size);
            return 0;
        }
        *encoded = encode_i_type(OPCODE_MOVM, (uint32_t)r1, (uint32_t)r2, imm);
        return 1;
    }

    /* ---------------------------- J-Type Instructions ---------------------------- */

    if (strings_equal_ignore_case(mnemonic, "JMP") && count == 2) {
        uint32_t addr = 0;
        if (!parse_unsigned_28(tokens[1], &addr)) {
            set_error_message("Invalid jump address %s", tokens[1], error_message, error_message_size);
            return 0;
        }
        *encoded = encode_j_type(OPCODE_JMP, addr);
        return 1;
    }

    set_error_message("Invalid mnemonic %s", mnemonic, error_message, error_message_size);
    return 0;
}
