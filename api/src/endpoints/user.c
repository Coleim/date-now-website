#include <enums.h>
#include <stdio.h>
#include <utils.h>
#include <stddef.h>
#include <structs.h>
#include <sql/user.h>
#include <lib/mongoose.h>

void send_users_res(struct mg_connection *c, struct mg_http_message *msg) {
	if(mg_match(msg->method, mg_str("GET"), NULL)) {
		size_t users_len = get_users_len();
		struct user **users = NULL;

		if(users_len > 0) {
			users = malloc(users_len * sizeof(struct user *));
			int query_code = get_users(users_len, users);

			if(query_code != 0) {
				if(query_code == HTTP_INTERNAL_ERROR) {
					mg_http_reply(c, 500, "", "{ \"code\": 500, \"error\": \"Internal error\" }");
				}
				else {
					fprintf(stderr, "ERROR SQL QUERY: %d\n", query_code);

					char *query_error_response = NULL;
					query_error_response = malloc(strlen("{\"code\":,\"error\":\"Error SQL query\"}") + sizeof(int));
					sprintf(query_error_response, "{\"code\":%d,\"error\":\"Error SQL query\"}", query_code);

					mg_http_reply(c, 500, "Content-Type: application/json\r\n", "%s\n", query_error_response);

					free(query_error_response);
				}

				return;
			}

		}

		char *result = users_to_json(users, users_len);
		printf("%s\n", result);

		mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", result);

		if(users_len > 0) free_users(users, users_len);
		free(result);
	}
	else if(mg_match(msg->method, mg_str("POST"), NULL)) {
		// Body validation
		int offset, length;

		// Email required
		offset = mg_json_get(msg->body, "$.email", &length);
		if(offset < 0) {
			mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"code\": 400, \"error\": \"Missing 'Email' property.\"}");
			return;
		}

		// Check Role value
		offset = mg_json_get(msg->body, "$.role", &length);
		char role[10];
		if(length > 10) {
			mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"code\": 400, \"error\": \"Value of 'role' should be 'USER' or 'AUTHOR'.\"}");

			return;
		}
		if(offset >= 0) {
			strncpy(role, msg->body.buf + offset+1, length-2);
			role[length-2] = '\0';

			printf("ROLE: %s\n", role);
			printf("COMPARISON: %d %d\n", strcmp(role, "USER"), strcmp(role, "AUTHOR"));

			if(strcmp(role, "USER") != 0 && strcmp(role, "AUTHOR") != 0) {
				mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"code\": 400, \"error\": \"Value of 'role' should be 'USER' or 'AUTHOR'.\"}");

				return;
			}
		}

		// Hydrate
		struct user *user = malloc(sizeof(struct user));
		int user_init_rc = user_init(user);
		if(!user_init_rc) {
			fprintf(stderr, "The user is NULL");
			mg_http_reply(c, 500, "", "{ \"code\": 500, \"error\": \"Internal error\" }");
			return;
		}

		struct mg_str key, val;

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
		}

		// Store in DB
		int query_code = add_user(user);
		if(query_code != 0) {
			fprintf(stderr, "ERROR SQL QUERY: %d\n", query_code);

			char *query_error_response = NULL;
			query_error_response = malloc(strlen("{\"code\":,\"error\":\"Error SQL query\"}") + sizeof(int));
			sprintf(query_error_response, "{\"code\":%d,\"error\":\"Error SQL query\"}", query_code);

			mg_http_reply(c, 500, "Content-Type: application/json\r\n", "%s\n", query_error_response);

			free(query_error_response);
		}
		else {
			mg_http_reply(c, 201, "Content-Type: application/json\r\n", "{ \"message\": \"Author successfully created\" }");
		}

		free_user(user);
	}
	else {
		mg_http_reply(c, 405, "", "Method not allowed");
	}
}

void send_user_res(struct mg_connection *c, struct mg_http_message *msg, int id) {
	if(mg_match(msg->method, mg_str("GET"), NULL)) {
		struct user *user = NULL;
		user = malloc(sizeof(struct user));

		int query_code = get_user(user, id);

		if(query_code != 0) {
			if(query_code == HTTP_NOT_FOUND) {
				mg_http_reply(c, 404, "Content-Type: application/json\r\n", "{ \"code\": 404, \"error\": \"Author not found\" }");
			}
			else if (query_code == HTTP_BAD_REQUEST) {
				mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{ \"code\": 404, \"error\": \"Bad request\" }");
			}
			else {
				fprintf(stderr, "ERROR SQL QUERY: %d\n", query_code);

				char *query_error_response = NULL;
				query_error_response = malloc(strlen("{\"code\":,\"error\":\"Error SQL query\"}") + sizeof(int));
				sprintf(query_error_response, "{\"code\":%d,\"error\":\"Error SQL query\"}", query_code);

				mg_http_reply(c, 500, "Content-Type: application/json\r\n", "%s\n", query_error_response);
				free(query_error_response);
			}
		}
		else {
			mg_http_reply(c, 201, "Content-Type: application/json\r\n", "{ \"message\": \"Author successfully created\" }");
		}

		free_user(user);
	}
	else if (mg_match(msg->method, mg_str("PUT"), NULL)) {
		// Hydrate
		struct user *user = malloc(sizeof(struct user));

		int user_init_rc = user_init(user);
		if(!user_init_rc) {
			fprintf(stderr, "The user is NULL");
			mg_http_reply(c, 500, "", "{ \"code\": 500, \"error\": \"Internal error\" }");
			return;
		}

		struct mg_str key, val;

		size_t ofs = 0;
		while ((ofs = mg_json_next(msg->body, ofs, &key, &val)) > 0) {
			printf("%.*s -> %.*s\n", (int) key.len, key.buf, (int) val.len, val.buf);

			if(mg_strcmp(key, mg_str("\"link\"")) == 0) {
				user->link = malloc(val.len);
				sprintf(user->link, "%.*s", (int) val.len-2, val.buf+1);
			}
		}

		// Check if exists
		int exists = user_exists(user->id);
		if(!exists) {
			mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{ \"code\": 400, \"error\": \"No user with this username exists\" }");
			return;
		}

		// Store in DB
		int query_code = edit_user(user);
		if(query_code != 0) {
			fprintf(stderr, "ERROR SQL QUERY: %d\n", query_code);

			char *query_error_response = NULL;
			query_error_response = malloc(strlen("{\"code\":,\"error\":\"Error SQL query\"}") + sizeof(int));
			sprintf(query_error_response, "{\"code\":%d,\"error\":\"Error SQL query\"}", query_code);

			mg_http_reply(c, 500, "Content-Type: application/json\r\n", "%s\n", query_error_response);

			free(query_error_response);
		}
		else {
			mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{ \"message\": \"Author successfully edited\" }");
		}

		free_user(user);

	}
	else if (mg_match(msg->method, mg_str("DELETE"), NULL)) {
		// Check if exists
		int exists = user_exists(id);
		if(!exists) {
			mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{ \"code\": 400, \"error\": \"No user with this username exists\" }");
		}
	}
	else {
		mg_http_reply(c, 405, "", "Method not allowed");
	}
}
