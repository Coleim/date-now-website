#include <math.h>
#include <enums.h>
#include <stdbool.h>
#include <stdio.h>
#include <utils.h>
#include <stddef.h>
#include <structs.h>
#include <sql/tag.h>
#include <lib/mongoose.h>
#include <macros/colors.h>
#include <macros/endpoints.h>

#define USER_EXISTS_MESSAGE "The tag already exists."

void send_tags_res(struct mg_connection *c, struct mg_http_message *msg, struct error_reply *error_reply) {
	int query_code;
	error_reply = malloc(sizeof(struct error_reply));

	if(mg_match(msg->method, mg_str("GET"), NULL)) {
		printf(TERMINAL_ENDPOINT_MESSAGE("=== GET AUTHOR LIST ==="));

		// Query params
		const struct mg_str q = mg_http_var(msg->query, mg_str("q"));
		const struct mg_str sort = mg_http_var(msg->query, mg_str("sort"));
		printf("QUERY PARAMS:\tQUERY - %.*s\t|\tSORT - %.*s\n", (int)q.len, q.buf, (int)sort.len, sort.buf);

		// Pagination
		int page, page_size;
		struct mg_str page_str = mg_http_var(msg->query, mg_str("page"));
		if(mg_str_to_num(page_str, 10, &page, sizeof(int)) == false) page = -1;
		else  {
			struct mg_str page_size_str = mg_http_var(msg->query, mg_str("page_size"));
			if(mg_str_to_num(page_size_str, 10, &page_size, sizeof(int)) == false) page_size = 10;
		}

		// Reply init
		struct list_reply *reply = malloc(sizeof(struct list_reply));
		reply->page = page;
		reply->page_size = page_size;
		reply->data = NULL;

		reply->total = reply->count = get_tags_len(&q);
		reply->total_pages = 0;
		printf("ARRAY COUNT:\tTOTAL - %d\t|\tCOUNT - %d\t|\tTOTAL PAGES - %d\n", reply->total, reply->count, reply->total_pages);
		// If pagination
		if(reply->page > 0) {
			// Cancel pagination if page size too big
			if(reply->total < reply->page_size) {
				reply->page = -1;
			}
			else {
				double tot_pages = (double)reply->total / (double)reply->page_size;
				reply->total_pages = (int) ceil(tot_pages);

				if(reply->total_pages < reply->page) {
					reply->page = reply->total_pages;
				}

				if(reply->page < reply->total_pages) {
					reply->count = reply->page_size;
				}
				else {
					int remainder = reply->total % reply->page_size;
					reply->count = remainder == 0 ? reply->page_size : remainder;
				}
			}
		}

		printf("PAGINATION:\tPAGE INDEX - %d\t|\tPAGE SIZE - %d\n", page, page_size);
		printf("ARRAY COUNT:\tTOTAL - %d\t|\tCOUNT - %d\t|\tTOTAL PAGES - %d\n", reply->total, reply->count, reply->total_pages);

		struct tag **tags = NULL;

		if(reply->count > 0) {
			tags = malloc(reply->count * sizeof(struct tag *));
			query_code = get_tags(reply->count, tags, &q, &sort, reply->page, reply->page_size);

			if(query_code != 0) {
				fprintf(stderr, TERMINAL_ERROR_MESSAGE("ERROR RETRIEVING USERS"));
				HANDLE_QUERY_CODE;

				free(reply->data);
				free(reply);
				return;
			}

		}

		reply->data = tags_to_json(tags, reply->count);
		list_reply_to_json(reply);

		mg_http_reply(c, 200, JSON_HEADER, "%s\n", reply->json);
		printf(TERMINAL_SUCCESS_MESSAGE("=== AUTHORS SUCCESSFULLY SENT ==="));

		if(reply->count > 0) {
			free_tags(tags, reply->count);
			free(reply->data);
			free(reply->json);
		}
		free(reply);
	}
	else if(mg_match(msg->method, mg_str("POST"), NULL)) {
		// Body validation
		int offset, length;

		// Email required
		offset = mg_json_get(msg->body, "$.email", &length);
		if(offset < 0) {
			ERROR_REPLY_400(EMAIL_REQUIRED_MESSAGE);
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("EMAIL REQUIRED"));
			return;
		}
		else {
			// Email and tagname not existing already
			char *email = malloc(length);
			strncpy(email, msg->body.buf + offset+1, length-2);

			char *tagname = NULL;
			offset = mg_json_get(msg->body, "$.tagname", &length);
			if(offset >= 0) {
				tagname = malloc(length);
				strncpy(tagname, msg->body.buf + offset+1, length-2);
			}

			int exists = tag_email_exists(tagname, email);
			if(exists != 0) {
				ERROR_REPLY_400(USER_EXISTS_MESSAGE);
				fprintf(stderr, TERMINAL_ERROR_MESSAGE("USER ALREADY EXISTS"));
				return;
			};
		}

		// Check Role value
		offset = mg_json_get(msg->body, "$.role", &length);
		char role[10];
		if(length > 10) {
			ERROR_REPLY_400(ROLE_FORMAT_MESSAGE);
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("WRONG ROLE"));

			return;
		}
		if(offset >= 0) {
			strncpy(role, msg->body.buf + offset+1, length-2);
			role[length-2] = '\0';

			if(strcmp(role, "USER") != 0 && strcmp(role, "AUTHOR") != 0) {
				ERROR_REPLY_400(ROLE_FORMAT_MESSAGE);
				fprintf(stderr, TERMINAL_ERROR_MESSAGE("WRONG ROLE"));

				return;
			}
		}

		// Hydrate
		struct tag *tag = malloc(sizeof(struct tag));
		int tag_init_rc = tag_init(tag);
		if(tag_init_rc != 0) {
			ERROR_REPLY_500;
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("USER IS NULL"));

			return;
		}

		tag_hydrate(msg, tag);

		// Store in DB
		query_code = add_tag(tag);
		if(query_code != 0) {
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("ERROR RETRIEVING USERS"));
			HANDLE_QUERY_CODE;

			return;
		}
		else {
			mg_http_reply(c, 201, JSON_HEADER, "{ \"message\": \"Author successfully created\" }");
			printf(TERMINAL_SUCCESS_MESSAGE("=== AUTHOR SUCCESSFULLY ADDED ==="));
		}

		free_tag(tag);
	}
	else {
		ERROR_REPLY_405;
	}
}

void send_tag_res(struct mg_connection *c, struct mg_http_message *msg, int id, struct error_reply *error_reply) {
	int query_code;
	error_reply = malloc(sizeof(struct error_reply));

	if(mg_match(msg->method, mg_str("GET"), NULL)) {
		printf(TERMINAL_ENDPOINT_MESSAGE("=== GET AUTHOR ==="));

		struct tag *tag = NULL;
		tag = malloc(sizeof(struct tag));

		query_code = get_tag(tag, id);

		if(query_code != 0) {
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("ERROR RETRIEVING USER"));
			HANDLE_QUERY_CODE;

			return;
		}
		else {
			char *result = tag_to_json(tag);

			mg_http_reply(c, 200, JSON_HEADER, "%s\n", result);
			printf(TERMINAL_SUCCESS_MESSAGE("=== AUTHOR SUCCESSFULLY SENT ==="));
		}

		free_tag(tag);
	}
	else if (mg_match(msg->method, mg_str("PUT"), NULL)) {
		// Hydrate
		struct tag *tag = malloc(sizeof(struct tag));

		// Check if exists
		int exists = tag_exists(id);
		if(!exists) {
			ERROR_REPLY_404;
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("USER NOT FOUND"));
			return;
		}

		int offset, length;

		// Email required
		offset = mg_json_get(msg->body, "$.email", &length);
		if(offset >= 0) {
			// Email and tagname not existing already
			char *email = malloc(length);
			strncpy(email, msg->body.buf + offset+1, length-2);

			char *tagname = NULL;
			offset = mg_json_get(msg->body, "$.tagname", &length);
			if(offset >= 0) {
				tagname = malloc(length);
				strncpy(tagname, msg->body.buf + offset+1, length-2);
			}

			int exists = tag_email_exists(tagname, email);
			if(exists != 0) {
				ERROR_REPLY_400(USER_EXISTS_MESSAGE);
				fprintf(stderr, TERMINAL_ERROR_MESSAGE("USER ALREADY EXISTS"));
				return;
			};
		}

		// Check Role value
		offset = mg_json_get(msg->body, "$.role", &length);
		char role[10];
		if(length > 10) {
			ERROR_REPLY_400(ROLE_FORMAT_MESSAGE);
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("WRONG ROLE"));

			return;
		}
		if(offset >= 0) {
			strncpy(role, msg->body.buf + offset+1, length-2);
			role[length-2] = '\0';

			if(strcmp(role, "USER") != 0 && strcmp(role, "AUTHOR") != 0) {
				ERROR_REPLY_400(ROLE_FORMAT_MESSAGE);
				fprintf(stderr, TERMINAL_ERROR_MESSAGE("WRONG ROLE"));
				return;
			}
		}

		query_code = get_tag(tag, id);
		if(query_code != 0) {
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("ERROR RETRIEVING USERS"));
			HANDLE_QUERY_CODE;

			return;
		}

		tag_hydrate(msg, tag);
		tag->id = id;

		// Store in DB
		query_code = edit_tag(tag);
		if(query_code != 0) {
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("ERROR RETRIEVING USERS"));
			HANDLE_QUERY_CODE;

			return;
		}
		else {
			mg_http_reply(c, 200, JSON_HEADER, "{ \"message\": \"Author successfully edited\" }");
			printf(TERMINAL_SUCCESS_MESSAGE("=== AUTHOR SUCCESSFULLY EDITED ==="));
		}

		free_tag(tag);
	}
	else if (mg_match(msg->method, mg_str("DELETE"), NULL)) {
		// Check if exists
		int exists = tag_exists(id);
		if(!exists) {
			ERROR_REPLY_404;
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("USER NOT FOUND"));

			return;
		}

		int delete_rc = delete_tag(id);
		if(delete_rc != 0) {
			ERROR_REPLY_500;
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("COULDN'T DELETE USER"));
		}

		printf(TERMINAL_SUCCESS_MESSAGE("=== AUTHOR SUCCESSFULLY DELETE ==="));
		mg_http_reply(c, 200, JSON_HEADER, "{ \"message\": \"Author successfully deleted\" }");
	}
	else {
		ERROR_REPLY_405;
	}
}
