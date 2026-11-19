#include <stdio.h>
#include <string.h>
#include <ctype.h>
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
