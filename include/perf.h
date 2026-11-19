#ifndef PERF_H
#define PERF_H

#include <stddef.h>

/* Purpose: Run synthetic performance measurements.
   Parameters: output text buffer and size.
   Return: 1 on completion, 0 for invalid output buffer.
   Time complexity: Varies; intentionally benchmark-scale. */
int perf_run_all(char *report, size_t reportSize);

#endif
