#ifndef CLI_INPUT_H
#define CLI_INPUT_H

#include <stddef.h>

int cli_read_line(const char *prompt, char *buffer, size_t size);
int cli_read_int(const char *prompt, int minimum, int maximum);

#endif
