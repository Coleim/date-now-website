#include <lib/mongoose.h>
#include <lib/sqlite3.h>
#include <macros/colors.h>
#include <macros/utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <structs.h>
#include <string.h>
#include <utils.h>

#define METHODS_LEN 4

#define MAP_DOUBLE(dest, stmt, index, required) \
	if(sqlite3_column_type(stmt, index) == SQLITE_FLOAT) { \
		double d = sqlite3_column_double(stmt, index); \
		printf("%s: %f, ", sqlite3_column_name(stmt, index), d); \
		dest = d; \
	} \
	else if(required) { \
		return 1; \
	}

#define MAP_TEXT(dest, stmt, index, required) \
	if(sqlite3_column_type(stmt, index) == SQLITE_TEXT) { \
		const char *str = sqlite3_column_text(stmt, index); \
		printf("%s: %s, ", sqlite3_column_name(stmt, index), str); \
		dest = strndup(str, strlen(str)); \
	} \
	else if(required) { \
		return 1; \
	}

#define MAP_INT(dest, stmt, index, required) \
	if(sqlite3_column_type(stmt, index) == SQLITE_INTEGER) { \
		int integer = sqlite3_column_int(stmt, index); \
		printf("%s: %d, ", sqlite3_column_name(stmt, index), integer); \
		dest = integer; \
	} \
	else if(required) { \
		return 1; \
	}

#define ERROR_REPLY_JSON "{\"code\":%d,\"message\":\"%s\"}"
#define LIST_REPLY_JSON "{\"data\":%s,\"count\":%d,\"total\":%d,\"totalPages\":%d}"
#define MEDIA_JSON "{\"id\":%d,\"alt\":\"%s\",\"url\":\"%s\",\"width\":%f,\"height\":%f}"
#define USER_JSON "{\"id\":%d,\"username\":\"%s\",\"email\":\"%s\",\"role\":\"%s\",\"link\":%s,\"picture\":%s,\"subscribedAt\":%d,\"isSupporter\":%d,\"createdAt\":%d}"
#define TAG_JSON "{\"name\":\"%s\",\"color\":\"%s\"}"

const size_t NULL_SIZE = strlen("null") * sizeof(char);
const size_t DOUBLE_QUOTES_SIZE = strlen("\"\"") * sizeof(char);
const size_t COMMA_SIZE = strlen(",") * sizeof(char);

const char *get_method(const char *method_buf) {
	const char *methods[METHODS_LEN] = {"GET", "POST", "PUT", "DELETE"};

	for(int i = 0; i < METHODS_LEN; i++) {
		if(strncmp(method_buf, methods[i], strlen(methods[i])) == 0) {
			return methods[i];
		}
	}

	return NULL;
}


int mg_str_to_str(char *dest, struct mg_str src) {
	sprintf(dest, STR_FMT, (int)src.len, src.buf);
}

/** JSON PARSE UTILS */
void error_reply_to_json(struct error_reply *err) {
	size_t len = snprintf(NULL, 0, ERROR_REPLY_JSON, err->code, err->message) + 1;

	err->json = malloc(len);
	sprintf(err->json, ERROR_REPLY_JSON, err->code, err->message);
	printf("%s - %ld\n", err->json, len);
}

void list_reply_to_json(struct list_reply *reply) {
	size_t len = snprintf(NULL, 0, LIST_REPLY_JSON, reply->data, reply->count, reply->total, reply->total_pages) + 1;

	reply->json = malloc(len);
	sprintf(reply->json, LIST_REPLY_JSON, reply->data, reply->count, reply->total, reply->total_pages);
}

size_t media_to_json_len(struct media *media) {
	if(media == NULL) {
		return NULL_SIZE;
	}

	return snprintf(NULL, 0, MEDIA_JSON, media->id, media->alternative_text, media->url, media->width, media->height) + 1;
}

char *media_to_json(struct media *media) {
	if(media == NULL) {
		return "null";
	}

	char *json = NULL;
	json = malloc(media_to_json_len(media));

	int len = sprintf(json, MEDIA_JSON, media->id, media->alternative_text, media->url, media->width, media->height);

	return json;
}

size_t user_to_json_len(struct user *user) {
	if(user == NULL) {
		return NULL_SIZE;
	}

	char *link = "null";
	if(user->link != NULL) {
		link = malloc(snprintf(NULL, 0, "\"%s\"", user->link) + 1);
		sprintf(link, "\"%s\"", user->link);
	}

	int len = snprintf(NULL, 0, USER_JSON, user->id, user->username, user->email, user->role, link, media_to_json(user->picture), user->subscribed_at, user->is_supporter, user->created_at) + 1;

	if(strcmp(link, "null") != 0) free(link);

	return len;
}

char *user_to_json(struct user *user) {
	if(user == NULL) {
		return "null";
	}

	char *link = "null";
	if(user->link != NULL) {
		link = malloc(snprintf(NULL, 0, "\"%s\"", user->link) + 1);
		sprintf(link, "\"%s\"", user->link);
	}

	char *json = NULL;
	json = malloc(user_to_json_len(user));

	sprintf(json, USER_JSON, user->id, user->username, user->email, user->role, link, media_to_json(user->picture), user->subscribed_at, user->is_supporter, user->created_at);

	if(strcmp(link, "null") != 0) free(link);

	return json;
}

char *users_to_json(struct user**users, size_t len) {
	char *json = NULL;

	size_t json_len = 0;
	for(int i = 0; i < len; i += 1) {
		json_len += user_to_json_len(users[i]);

		if(i < len - 1) {
			json_len += COMMA_SIZE;
		}
	}

	char *users_json = malloc(json_len+1);
	users_json[0] = '\0';
	for(int i = 0; i < len; i += 1) {
		char *user = user_to_json(users[i]);

		strcat(users_json, user);
		if(i < len-1) strcat(users_json, ",");

		if(strcmp(user, "null")) free(user);
	}

	if(len > 0) {
		json = malloc(snprintf(NULL, 0, "[%s]", users_json) + 1);
		sprintf(json, "[%s]", users_json);
	}
	else {
		json = "[]";
	}

	free(users_json);

	return json;
}

size_t tag_to_json_len(struct tag *tag) {
	if(tag == NULL) {
		return NULL_SIZE;
	}

	int len = snprintf(NULL, 0, TAG_JSON, tag->name, tag->color) + 1;

	return len;
}

char *tag_to_json(struct tag *tag) {
	if(tag == NULL) {
		return "null";
	}

	char *json = NULL;
	json = malloc(tag_to_json_len(tag));

	sprintf(json, TAG_JSON, tag->name, tag->color);

	return json;
}

char *tags_to_json(struct tag**tags, size_t len) {
	char *json = NULL;

	size_t json_len = 0;
	for(int i = 0; i < len; i += 1) {
		json_len += tag_to_json_len(tags[i]);

		if(i < len - 1) {
			json_len += COMMA_SIZE;
		}
	}

	char *tags_json = malloc(json_len+1);
	tags_json[0] = '\0';
	for(int i = 0; i < len; i += 1) {
		char *tag = tag_to_json(tags[i]);

		strcat(tags_json, tag);
		if(i < len - 1) strcat(tags_json, ",");

		if(strcmp(tag, "null")) free(tag);
	}

	if(len > 0) {
		json = malloc(snprintf(NULL, 0, "[%s]", tags_json) + 1);
		sprintf(json, "[%s]", tags_json);
	}
	else {
		json = "[]";
	}

	free(tags_json);

	return json;
}

/** FREE UTILS */

int free_media(struct media *media) {
	free(media->alternative_text);
	free(media->url);

	media->alternative_text = NULL;
	media->url = NULL;

	free(media);
	media = NULL;

	return 0;
}

int free_user(struct user *user) {
	free(user->username);
	free(user->email);
	free(user->role);
	free(user->link);

	if(user->picture != NULL) {
		free_media(user->picture);
	}

	user->username = NULL;
	user->email = NULL;
	user->role = NULL;
	user->link = NULL;

	free(user);
	user = NULL;

	return 0;
}

int free_tag(struct tag *tag) {
	free(tag->name);
	free(tag->color);

	tag->name = NULL;
	tag->color = NULL;

	free(tag);
	tag = NULL;

	return 0;
}

int free_users(struct user **users, size_t len) {
	int result_code = 0;
	for(int i = 0; i < len; i += 1) {
		if(users[i] != NULL) {
			result_code = free_user(users[i]);

			if(result_code != 0) {
				return result_code;
			}
		}
	}

	free(users);
	users = NULL;

	return result_code;
}

int free_tags(struct tag **tags, size_t len) {
	int result_code = 0;
	for(int i = 0; i < len; i += 1) {
		if(tags[i] != NULL) {
			result_code = free_tag(tags[i]);

			if(result_code != 0) {
				return result_code;
			}
		}
	}

	free(tags);
	tags = NULL;

	return result_code;
}

/** MAPPING */
int error_reply_map(struct error_reply *err, int code, char *message, int code_http) {
	if(err == NULL) return -1;

	err->code = code;
	err->code_http = code_http;
	if(message != NULL) err->message = message;

	error_reply_to_json(err);

	return 0;
}

int user_map(struct user *user, sqlite3_stmt *stmt, int start_index, int end_index) {
	if(start_index > end_index || user == NULL || stmt == NULL) {
		return -1;
	}

	int id_index = start_index;
	int username_index = start_index+1;
	int email_index = start_index+2;
	int role_index = start_index+3;
	int link_index = start_index+4;
	int subscribed_at_index = start_index+5;
	int is_supporter_index = start_index+6;
	int created_at_index = start_index+7;

	printf(ANSI_BACKGROUND_AMBER " USER " ANSI_RESET_ALL "\n");
	// ID
	MAP_INT(user->id, stmt, id_index, 1);
	// Username
	MAP_TEXT(user->username, stmt, username_index, 0);
	// Email
	MAP_TEXT(user->email, stmt, email_index, 1);
	// Role
	MAP_TEXT(user->role, stmt, role_index, 1);

	// Link
	MAP_TEXT(user->link, stmt, link_index, 0);

	// Subscribed at
	MAP_INT(user->subscribed_at, stmt, subscribed_at_index, 0);
	// Is supporter
	MAP_INT(user->is_supporter, stmt, is_supporter_index, 1);

	// Created at
	MAP_INT(user->created_at, stmt, created_at_index, 1);

	return 0;
}

int media_map(struct media *media, sqlite3_stmt *stmt, int start_index, int end_index) {
	if(start_index > end_index || media == NULL || stmt == NULL) {
		return -1;
	}

	int id_index = start_index;
	int alt_index = start_index+1;
	int url_index = start_index+2;
	int width_index = start_index+3;
	int height_index = start_index+4;

	MAP_INT(media->id, stmt, id_index, 1);
	MAP_TEXT(media->alternative_text, stmt, alt_index, 1);
	MAP_TEXT(media->url, stmt, url_index, 1);
	MAP_DOUBLE(media->width, stmt, width_index, 0);
	MAP_DOUBLE(media->height, stmt, height_index, 0);

	return 0;
}

int tag_map(struct tag *tag, sqlite3_stmt *stmt, int start_index, int end_index) {
	if(start_index > end_index || tag == NULL || stmt == NULL) {
		return -1;
	}

	int name_index = start_index;
	int color_index = start_index+1;

	MAP_TEXT(tag->name, stmt, name_index, 1);
	MAP_TEXT(tag->color, stmt, color_index, 1);

	return 0;
}

/** HYDRATE */
void user_hydrate(struct mg_http_message *msg, struct user *user) {
	struct mg_str key, val;
	int number;
	bool number_parsed;

	size_t ofs = 0;
	while ((ofs = mg_json_next(msg->body, ofs, &key, &val)) > 0) {
		printf("%.*s -> %.*s\n", (int) key.len, key.buf, (int) val.len, val.buf);

		if(mg_strcmp(key, mg_str("\"username\"")) == 0) {
			printf("USERNAME: %.*s\n", (int) val.len, val.buf);
			user->username = malloc(val.len);
			sprintf(user->username, "%.*s", (int) val.len-2, val.buf+1);
		}
		else if(mg_strcmp(key, mg_str("\"email\"")) == 0) {
			printf("EMAIL: %.*s\n", (int) val.len, val.buf);
			user->email = malloc(val.len);
			sprintf(user->email, "%.*s", (int) val.len-2, val.buf+1);
		}
		else if(mg_strcmp(key, mg_str("\"role\"")) == 0) {
			printf("ROLE: %.*s\n", (int) val.len, val.buf);
			user->role = malloc(val.len);
			sprintf(user->role, "%.*s", (int) val.len-2, val.buf+1);
		}
		else if(mg_strcmp(key, mg_str("\"link\"")) == 0) {
			printf("LINK: %.*s\n", (int) val.len, val.buf);
			user->link = malloc(val.len);
			sprintf(user->link, "%.*s", (int) val.len-2, val.buf+1);
		}
		else if(mg_strcmp(key, mg_str("\"isSupporter\"")) == 0) {
			number_parsed = mg_str_to_num(val, 10, &number, sizeof(int));
			if(number_parsed) {
				user->is_supporter = number;
			}
		}
	}
}

void tag_hydrate(struct mg_http_message *msg, struct tag *tag) {
	struct mg_str key, val;
	int number;
	bool number_parsed;

	size_t ofs = 0;
	while ((ofs = mg_json_next(msg->body, ofs, &key, &val)) > 0) {
		printf("%.*s -> %.*s\n", (int) key.len, key.buf, (int) val.len, val.buf);

		if(mg_strcmp(key, mg_str("\"name\"")) == 0) {
			printf("NAME: %.*s\n", (int) val.len, val.buf);
			tag->name = malloc(val.len);
			sprintf(tag->name, "%.*s", (int) val.len-2, val.buf+1);
		}
		else if(mg_strcmp(key, mg_str("\"color\"")) == 0) {
			printf("COLOR: %.*s\n", (int) val.len, val.buf);
			tag->color = malloc(val.len);
			sprintf(tag->color, "%.*s", (int) val.len-2, val.buf+1);
		}
	}
}

/** INIT */
int user_init(struct user *user) {
	if(user == NULL) {
		return -1;
	}

	user->username = NULL;
	user->email = NULL;
	user->role = NULL;

	user->link = NULL;
	user->picture = NULL;

	user->subscribed_at = 0;
	user->is_supporter = 0;

	return 0;
}

int tag_init(struct tag *tag) {
	if(tag == NULL) {
		return -1;
	}

	tag->name = NULL;
	tag->color = NULL;

	return 0;
}
