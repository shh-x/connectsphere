#ifndef STORAGE_H
#define STORAGE_H

#include "user.h"

/* Purpose: Save users and friendships in the requested text formats.
   Parameters: directory, output directories optional.
   Return: 1 on full success, 0 on failure.
   Time complexity: O(V + E). */
int storage_save(const Directory *directory, const char *usersFile,
                 const char *friendshipsFile);

/* Purpose: Load validated text data and rebuild every structure.
   Parameters: directory, input files.
   Return: 1 on success, 0 on missing/corrupted data.
   Time complexity: O(V + E + rebuild costs). */
int storage_load(Directory *directory, const char *usersFile,
                 const char *friendshipsFile);

/* Purpose: Clear the directory and create realistic demonstration data.
   Parameters: directory.
   Return: 1 on success, 0 on failure.
   Time complexity: O(V + E). */
int storage_load_sample(Directory *directory);

#endif
