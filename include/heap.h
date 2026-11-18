#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

typedef struct HeapNode {
    int userId;
    int score;
} HeapNode;

typedef struct BinaryHeap {
    HeapNode *items;
    size_t size;
    size_t capacity;
} BinaryHeap;

/* Purpose: Initialize an array-backed max heap.
   Parameters: heap, initial capacity.
   Return: 1 on success, 0 on failure.
   Time complexity: O(1). */
int heap_init(BinaryHeap *heap, size_t capacity);

/* Purpose: Release heap storage.
   Parameters: heap.
   Return: void.
   Time complexity: O(1). */
void heap_free(BinaryHeap *heap);

/* Purpose: Insert a heap node.
   Parameters: heap, node.
   Return: 1 on success, 0 on failure.
   Time complexity: O(log n). */
int heap_insert(BinaryHeap *heap, HeapNode node);

/* Purpose: Extract the highest-priority node.
   Parameters: heap, destination.
   Return: 1 on success, 0 if empty.
   Time complexity: O(log n). */
int heap_extract_max(BinaryHeap *heap, HeapNode *result);

/* Purpose: Sift an item upward.
   Parameters: heap, starting index.
   Return: void.
   Time complexity: O(log n). */
void heap_up(BinaryHeap *heap, size_t index);

/* Purpose: Sift an item downward.
   Parameters: heap, starting index.
   Return: void.
   Time complexity: O(log n). */
void heap_down(BinaryHeap *heap, size_t index);

/* Purpose: Build a heap in linear time.
   Parameters: heap.
   Return: void.
   Time complexity: O(n). */
void heap_build(BinaryHeap *heap);

/* Purpose: Display heap array storage.
   Parameters: heap.
   Return: void.
   Time complexity: O(n). */
void heap_display(const BinaryHeap *heap);

#endif
