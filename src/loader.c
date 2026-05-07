#include "../include/loader.h"
#include "../include/memory.h"
#include "../include/parser.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int instruction_count = 0;

static int is_blank_line(const char *line) {
    while (*line != '\0') {
        if (!isspace((unsigned char)*line)) {
            return 0;
        }
        line++;
    }
    return 1;
}

static void trim_whitespace(char *text) {
    if (text == NULL) {
        return;
    }

    char *start = text;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)*(end - 1))) {
        end--;
    }
    *end = '\0';
}

static int path_is_absolute(const char *path) {
    if (path == NULL || *path == '\0') {
        return 0;
    }

    if (path[0] == '/' || path[0] == '\\') {
        return 1;
    }

    if (path[1] == ':' && (path[2] == '/' || path[2] == '\\')) {
        return 1;
    }

    return 0;
}

static FILE *open_program_file(const char *filename, char *resolved_path, size_t resolved_size) {
    if (resolved_path != NULL && resolved_size > 0) {
        resolved_path[0] = '\0';
    }

    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        if (resolved_path != NULL && resolved_size > 0) {
            strncpy(resolved_path, filename, resolved_size - 1);
            resolved_path[resolved_size - 1] = '\0';
        }
        return file;
    }

#ifdef PROJECT_SOURCE_DIR
    if (!path_is_absolute(filename)) {
        if (resolved_path != NULL && resolved_size > 0) {
            int written = snprintf(resolved_path, resolved_size, "%s/%s", PROJECT_SOURCE_DIR, filename);
            if (written < 0 || (size_t)written >= resolved_size) {
                return NULL;
            }
            file = fopen(resolved_path, "r");
            if (file != NULL) {
                return file;
            }
        }
    }
#endif

    return NULL;
}

int load_program(const char *filename) {
    char line[256];
    char path[PATH_MAX];
    uint32_t buffer[INSTRUCTION_SEGMENT_END];
    int line_number = 0;
    int loaded_count = 0;

    if (filename == NULL) {
        fprintf(stderr, "Error: program filename is NULL.\n");
        return 0;
    }

    FILE *file = open_program_file(filename, path, sizeof(path));
    if (file == NULL) {
        fprintf(stderr, "Error opening file '%s': %s\n", path[0] != '\0' ? path : filename, strerror(errno));
        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;
        trim_whitespace(line);

        if (is_blank_line(line)) {
            continue;
        }

        if (loaded_count >= INSTRUCTION_SEGMENT_END) {
            fprintf(stderr, "Error on line %d: too many instructions (maximum %d).\n", line_number, INSTRUCTION_SEGMENT_END);
            fclose(file);
            return 0;
        }

        char error_message[128] = {0};
        uint32_t encoded_value = 0;

        if (!parse_and_encode_instruction(line, &encoded_value, error_message, sizeof(error_message))) {
            fprintf(stderr, "Error on line %d: %s: '%s'\n", line_number,
                    error_message[0] != '\0' ? error_message : "Invalid instruction format",
                    line);
            fclose(file);
            return 0;
        }

        buffer[loaded_count++] = encoded_value;
    }

    fclose(file);

    for (int index = 0; index < loaded_count; ++index) {
        write_memory((uint32_t)index, buffer[index]);
    }

    instruction_count = loaded_count;
    return 1;
}

int get_instruction_count(void) {
    return instruction_count;
}
