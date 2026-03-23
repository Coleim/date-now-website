#pragma once

#include <lib/sqlite3.h>
#include <structs.h>
#include <stddef.h>

const char *get_method(const char *buffer);

/** JSON */
size_t media_to_json_len(struct media *media);
char *media_to_json(struct media *media);

size_t user_to_json_len(struct user *user);
char *user_to_json(struct user *user);
char *users_to_json(struct user **users, size_t len);

/** FREE */
int free_media(struct media *media);
int free_user(struct user *user);

int free_users(struct user **user, size_t len);

/** MAPPING */
int user_map(struct user *user, sqlite3_stmt *stmt, int start_index, int end_index);
int media_map(struct media *media, sqlite3_stmt *stmt, int start_index, int end_index);

/** INIT */
int user_init(struct user *user);
