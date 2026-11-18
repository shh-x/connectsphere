#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include "../include/utils.h"

char *trim_string(char *text) {
    char *end;
    if (!text) return NULL;
    while (isspace((unsigned char)*text)) text++;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return text;
}

int read_line(const char *prompt, char *buffer, size_t size) {
    char *trimmed;
    if (prompt && *prompt) printf("%s", prompt);
    if (!fgets(buffer, (int)size, stdin)) return 0;
    trimmed = trim_string(buffer);
    if (trimmed != buffer) memmove(buffer, trimmed, strlen(trimmed) + 1);
    return 1;
}

int read_int(const char *prompt, int minimum, int maximum) {
    char buffer[128], *end;
    long value;
    while (1) {
        if (!read_line(prompt, buffer, sizeof(buffer))) return 0;
        errno = 0;
        value = strtol(buffer, &end, 10);
        if (errno || end == buffer || *trim_string(end) != '\0' ||
            value < minimum || value > maximum) {
            printf("Invalid integer. Enter a value from %d to %d.\n", minimum, maximum);
            continue;
        }
        return (int)value;
    }
}

int name_compare(const char *a, const char *b) {
    while (*a && *b) {
        unsigned char ca = (unsigned char)tolower((unsigned char)*a);
        unsigned char cb = (unsigned char)tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return (unsigned char)tolower((unsigned char)*a) -
           (unsigned char)tolower((unsigned char)*b);
}

int normalize_name(char *input, char *output, size_t size) {
    char *text = trim_string(input);
    size_t i, len;
    if (!text || !*text) return 0;
    len = strlen(text);
    if (len >= size) return 0;
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)text[i];
        if (!isalpha(c) && c != ' ' && c != '\'' && c != '.' && c != '-') return 0;
    }
    snprintf(output, size, "%s", text);
    return 1;
}

int normalize_city(char *input, char *output, size_t size) {
    char *text = trim_string(input);
    if (!text || !*text || strlen(text) >= size) return 0;
    snprintf(output, size, "%s", text);
    return 1;
}
