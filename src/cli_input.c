#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../include/cli_input.h"
#include "../include/utils.h"

int cli_read_line(const char *prompt, char *buffer, size_t size) {
    char *trimmed;
    if (prompt && *prompt) printf("%s", prompt);
    if (!fgets(buffer, (int)size, stdin)) return 0;
    if (!strchr(buffer, '\n') && !feof(stdin)) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
        return 0;
    }
    trimmed = trim_string(buffer);
    if (trimmed != buffer) memmove(buffer, trimmed, strlen(trimmed) + 1);
    return 1;
}

int cli_read_int(const char *prompt, int minimum, int maximum) {
    char buffer[128], *end;
    long value;
    for (;;) {
        if (!cli_read_line(prompt, buffer, sizeof(buffer))) {
            if (feof(stdin)) return 0;
            printf("Input too long. Try again.\n");
            continue;
        }
        errno = 0;
        value = strtol(buffer, &end, 10);
        if (errno || end == buffer || *trim_string(end) != '\0' ||
            value < minimum || value > maximum) {
            printf("Invalid integer. Enter a value from %d to %d.\n",
                   minimum, maximum);
            continue;
        }
        return (int)value;
    }
}
