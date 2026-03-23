#include <lib/sqlite3.h>
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
		dest = strdup(str); \
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

#define MEDIA_JSON "{\"id\":%d,\"alt\":\"%s\",\"url\":\"%s\",\"width\":%f,\"height\":%f}"
#define USER_JSON "{\"id\":%d,\"username\":\"%s\",\"role\":\"%s\",\"link\":%s,\"picture\":%s,\"subscribedAt\":%d,\"isSupporter\":%d,\"createdAt\":%d}"

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

/** JSON PARSE UTILS */

size_t media_to_json_len(struct media *media) {
	if(media == NULL) {
		return NULL_SIZE;
	}

	return snprintf(NULL, 0, MEDIA_JSON, media->id, media->alternative_text, media->url, media->width, media->height) - 2;
}

char *media_to_json(struct media *media) {
	if(media == NULL) {
		return "null";
	}

	char *json = NULL;
	json = malloc(media_to_json_len(media));

	int len = sprintf(json, MEDIA_JSON, media->id, media->alternative_text, media->url, media->width, media->height);
	printf("to json len: %d\n", len);

	return json;
}

size_t user_to_json_len(struct user *user) {
	if(user == NULL) {
		return NULL_SIZE;
	}

	return snprintf(NULL, 0, USER_JSON, user->id, user->username, user->role, user->link, media_to_json(user->picture), user->subscribed_at, user->is_supporter, user->created_at) - 2;
}

char *user_to_json(struct user *user) {
	if(user == NULL) {
		return "null";
	}

	char *link = "null";
	if(user->link != NULL) {
		link = malloc(sizeof(char) * strlen(user->link) + 3);
		sprintf(link, "\"%s\"", user->link);
	}

	char *json = NULL;
	json = malloc(user_to_json_len(user));

	int len = sprintf(json, USER_JSON, user->id, user->username, user->role, link, media_to_json(user->picture), user->subscribed_at, user->is_supporter, user->created_at);
	printf("to json len: %d\n", len);

	if(strcmp(link, "null")) {
		free(link);
	}

	return json;
}

char *users_to_json(struct user**users, size_t len) {
	char *json = NULL;

	size_t json_len = strlen("[]") * sizeof(char) + 1;

	for(int i = 0; i < len; i += 1) {
		json_len += user_to_json_len(users[i]);
		printf("user len: %ld\n", user_to_json_len(users[i]));

		if(i < len - 1) {
			json_len += COMMA_SIZE;
		}
	}

	json = malloc(json_len);
	json[0] = '\0';

	strcat(json, "[");

	for(int i = 0; i < len; i += 1) {
		char *user = user_to_json(users[i]);
		printf("user json len: %ld\n", strlen(user));

		strcat(json, user);
		strcat(json, ",");

		if(strcmp(user, "null")) free(user);
	}

	if(len > 0) {
		json[json_len-2] = ']';
	}
	else {
		strcat(json, "]");
	}

	return json;
}

/** FREE UTILS */

int free_media(struct media *media) {
	free(media->alternative_text);
	free(media->url);

	free(media);

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

	free(user);

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

	return result_code;
}

/** MAPPING */


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

	user->is_supporter = 0;

	return 0;
}
