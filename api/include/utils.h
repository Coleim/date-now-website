#pragma once

#include <lib/mongoose.h>
#include <lib/sqlite3.h>
#include <structs.h>
#include <stddef.h>

const char *get_method(const char *buffer);
int mg_str_to_str(char *dest, struct mg_str src);

/** JSON */
void error_reply_to_json(struct error_reply *err);
void list_reply_to_json(struct list_reply *reply);

size_t media_to_json_len(struct media *media);
char *media_to_json(struct media *media);

size_t user_to_json_len(struct user *user);
char *user_to_json(struct user *user);
char *users_to_json(struct user **users, size_t len);

size_t tag_to_json_len(struct tag *tag);
char *tag_to_json(struct tag *tag);
char *tags_to_json(struct tag **tags, size_t len);

/** FREE */
int free_media(struct media *media);
int free_user(struct user *user);
int free_tag(struct tag *tag);

int free_users(struct user **user, size_t len);
int free_tags(struct tag **tag, size_t len);

/** MAPPING */
int error_reply_map(struct error_reply *err, int code, char *message, int code_http);
int media_map(struct media *media, sqlite3_stmt *stmt, int start_index, int end_index);
int user_map(struct user *user, sqlite3_stmt *stmt, int start_index, int end_index);
int tag_map(struct tag *tag, sqlite3_stmt *stmt, int start_index, int end_index);

/** HYDRATE */
void user_hydrate(struct mg_http_message *msg, struct user *user);
void tag_hydrate(struct mg_http_message *msg, struct tag *tag);

/** INIT */
int user_init(struct user *user);
int tag_init(struct tag *tag);
