#include <assert.h>
#include <stdio.h>
#include "../include/user.h"
#include "../include/storage.h"

int main(void) {
    Directory directory;
    int i;
    assert(directory_init(&directory));
    assert(directory_add_user(&directory, 1, "Alice", 25, "Pune"));
    assert(directory_add_user(&directory, 2, "Bob", 30, "Mumbai"));
    assert(directory_add_friendship(&directory, 1, 2));
    assert(dsu_find(&directory.dsu, 1) == dsu_find(&directory.dsu, 2));
    assert(directory_delete_user(&directory, 1));
    assert(directory_find_id(&directory, 1) == 0);
    assert(directory_find_id(&directory, 2) != 0);
    for (i = 3; i <= 40; i++) {
        char name[50];
        snprintf(name, sizeof(name), "Person %c%c",
                 'A' + (i - 3) / 26, 'A' + (i - 3) % 26);
        assert(directory_add_user(&directory, i, name, 25, "Pune"));
    }
    assert(directory_find_id(&directory, 40) != 0);
    assert(directory_find_id(&directory, 40)->id == 40);
    assert(dsu_count_groups(&directory.dsu) == 39);
    assert(directory_update_user(&directory, 40, "Renamed Member", 31, "Delhi"));
    assert(directory_find_name(&directory, "renamed member") != 0);
    assert(directory_find_name(&directory, "Person BL") == 0);
    assert(directory_add_user(&directory, 1, "Alice", 25, "Pune"));
    assert(directory_add_friendship(&directory, 1, 2));
    assert(storage_save(&directory, "tests/integration_users.txt",
                        "tests/integration_friendships.txt"));
    {
        Directory reloaded;
        assert(directory_init(&reloaded));
        assert(storage_load(&reloaded, "tests/integration_users.txt",
                            "tests/integration_friendships.txt"));
        assert(directory_find_id(&reloaded, 1) != 0);
        assert(directory_find_id(&reloaded, 2) != 0);
        assert(dsu_find(&reloaded.dsu, 1) == dsu_find(&reloaded.dsu, 2));
        directory_free(&reloaded);
    }
    directory_free(&directory);
    return 0;
}
