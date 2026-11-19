#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/* Purpose: Compare names case-insensitively.
   Parameters: two strings.
   Return: negative, zero, or positive.
   Time complexity: O(length). */
int name_compare(const char *a, const char *b);

/* Purpose: Validate and normalize a name.
   Parameters: input, output, output size.
   Return: 1 if valid, 0 otherwise.
   Time complexity: O(n). */
int normalize_name(char *input, char *output, size_t size);

/* Purpose: Trim whitespace at both ends of a string.
   Parameters: mutable string.
   Return: pointer to the trimmed string.
   Time complexity: O(n). */
char *trim_string(char *text);

/* Purpose: Validate a city string.
   Parameters: input, output, output size.
   Return: 1 if valid, 0 otherwise.
   Time complexity: O(n). */
int normalize_city(char *input, char *output, size_t size);

#endif
