#include <assert.h>
#include "../include/dsu.h"

int main(void) {
    DSU dsu;
    assert(dsu_init(&dsu, 20));
    assert(dsu_find(&dsu, 1) == 1);
    dsu_union(&dsu, 1, 2);
    dsu_union(&dsu, 2, 3);
    assert(dsu_find(&dsu, 1) == dsu_find(&dsu, 3));
    assert(dsu_group_size(&dsu, 1) == 3);
    assert(dsu_count_groups(&dsu) == 17);
    dsu_free(&dsu);
    return 0;
}
