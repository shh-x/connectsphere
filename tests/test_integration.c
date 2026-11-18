#include <assert.h>
#include "../include/user.h"
#include "../include/storage.h"

int main(void) {
    Directory directory;
    assert(directory_init(&directory));
    assert(directory_add_user(&directory, 1, "Alice", 25, "Pune"));
    assert(directory_add_user(&directory, 2, "Bob", 30, "Mumbai"));
    assert(directory_add_friendship(&directory, 1, 2));
    assert(dsu_find(&directory.dsu, 1) == dsu_find(&directory.dsu, 2));
    assert(directory_delete_user(&directory, 1));
    assert(directory_find_id(&directory, 1) == 0);
    assert(directory_find_id(&directory, 2) != 0);
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
