#include <assert.h>
#include "../include/heap.h"

int main(void) {
    BinaryHeap heap;
    HeapNode result;
    assert(heap_init(&heap, 4));
    assert(heap_insert(&heap, (HeapNode){3, 5}));
    assert(heap_insert(&heap, (HeapNode){1, 5}));
    assert(heap_insert(&heap, (HeapNode){2, 7}));
    assert(heap_extract_max(&heap, &result));
    assert(result.userId == 2);
    assert(heap_extract_max(&heap, &result));
    assert(result.userId == 1);
    heap_free(&heap);
    return 0;
}
