#pragma once

#include <stddef.h>
#include <structs.h>
#include <lib/mongoose.h>

int user_exists(int id);
int get_users_len();
int get_users(size_t len, struct user **arr);
int get_user(struct user *user, int id);
int add_user(struct user *user);
int edit_user(struct user *user);
int delete_user(int id);
