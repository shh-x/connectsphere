#ifndef PERF_H
#define PERF_H

#include <stddef.h>

/* Purpose: Run synthetic performance measurements.
   Parameters: output text buffer and size.
   Return: 1 on completion, 0 for invalid output buffer.
   Time complexity: Varies; intentionally benchmark-scale. */
int perf_run_all(char *report, size_t reportSize);

/* Purpose: Run synthetic measurements for one supported dataset size.
   Parameters: N, output text buffer and size.
   Return: 1 on completion, 0 for invalid size/allocation failure.
   Time complexity: O(N log N + QN + S^2), where Q<=1000 and S<=10000. */
int perf_run_for_size(int n, char *report, size_t reportSize);

#endif
