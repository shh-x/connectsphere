#include <stdlib.h>
#include "../include/heap.h"

static int better(HeapNode a, HeapNode b) {
    if (a.score != b.score) return a.score > b.score;
    return a.userId < b.userId;
}

int heap_init(BinaryHeap *heap, size_t capacity) {
    if (!heap) return 0;
    heap->items = (HeapNode *)calloc(capacity ? capacity : 16, sizeof(HeapNode));
    if (!heap->items) return 0;
    heap->size = 0;
    heap->capacity = capacity ? capacity : 16;
    return 1;
}

void heap_free(BinaryHeap *heap) {
    if (!heap) return;
    free(heap->items);
    heap->items = NULL;
    heap->size = heap->capacity = 0;
}

void heap_up(BinaryHeap *heap, size_t index) {
    while (index > 0) {
        size_t parent = (index - 1) / 2;
        if (!better(heap->items[index], heap->items[parent])) break;
        HeapNode temp = heap->items[parent];
        heap->items[parent] = heap->items[index];
        heap->items[index] = temp;
        index = parent;
    }
}

void heap_down(BinaryHeap *heap, size_t index) {
    for (;;) {
        size_t left = index * 2 + 1, right = left + 1, best = index;
        if (left < heap->size && better(heap->items[left], heap->items[best])) best = left;
        if (right < heap->size && better(heap->items[right], heap->items[best])) best = right;
        if (best == index) break;
        HeapNode temp = heap->items[index];
        heap->items[index] = heap->items[best];
        heap->items[best] = temp;
        index = best;
    }
}

int heap_insert(BinaryHeap *heap, HeapNode node) {
    if (heap->size == heap->capacity) {
        size_t newCapacity = heap->capacity * 2;
        HeapNode *larger = (HeapNode *)realloc(heap->items, newCapacity * sizeof(HeapNode));
        if (!larger) return 0;
        heap->items = larger;
        heap->capacity = newCapacity;
    }
    heap->items[heap->size] = node;
    heap_up(heap, heap->size);
    heap->size++;
    return 1;
}

int heap_extract_max(BinaryHeap *heap, HeapNode *result) {
    if (!heap->size) return 0;
    *result = heap->items[0];
    heap->items[0] = heap->items[--heap->size];
    if (heap->size) heap_down(heap, 0);
    return 1;
}

void heap_build(BinaryHeap *heap) {
    if (!heap->size) return;
    {
        size_t i = heap->size;
        while (i > 0) heap_down(heap, --i);
    }
}
