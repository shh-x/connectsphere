#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/* Purpose: Read a trimmed line safely.
   Parameters: prompt text, destination, destination size.
   Return: 1 on success, 0 on EOF.
   Time complexity: O(n). */
int read_line(const char *prompt, char *buffer, size_t size);

/* Purpose: Read an integer in an inclusive range.
   Parameters: prompt, minimum, maximum.
   Return: parsed integer, or 0 on repeated invalid/EOF input.
   Time complexity: O(n) per attempt. */
int read_int(const char *prompt, int minimum, int maximum);

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
