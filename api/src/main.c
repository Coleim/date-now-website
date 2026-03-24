#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <lib/mongoose.h>
#include <lib/sqlite3.h>
#include <structs.h>
#include <utils.h>
#include <macros/colors.h>
#include <endpoints/user.h>
#include <endpoints/tag.h>

sqlite3 *db;

void clean_db() {
	sqlite3_close(db);
}

void sigTerm(int code) {
	printf(">>> SIGTERM received [%d]\n", code);
	clean_db();

	exit(EXIT_SUCCESS);
}

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
	struct error_reply *error_reply = NULL;

	if (ev == MG_EV_HTTP_MSG) {
		struct mg_http_message *http_msg = (struct mg_http_message *) ev_data;
		struct mg_str endpoint_cap[2];

		if(mg_match(http_msg->uri, mg_str("/api/#"), endpoint_cap)) {
			printf("endpoint:  %.*s\n", (int)endpoint_cap[0].len, endpoint_cap[0].buf);

			if(mg_strcmp(endpoint_cap[0], mg_str("user")) >= 0) {
				struct mg_str user_cap[2];

				if(mg_match(endpoint_cap[0], mg_str("user/*"), user_cap)) {
					int id;
					int id_parsed = mg_str_to_num(user_cap[0], 10, &id, sizeof(int));
					if(!id_parsed) {
						mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{ \"code\": 400, \"error\": \"ID is not a number.\" }");
						return;
					}

					send_user_res(c, http_msg, id, error_reply);
				}
				else if(mg_strcmp(endpoint_cap[0], mg_str("user")) == 0) {
					send_users_res(c, http_msg, error_reply);
				}
			}
			if(mg_strcmp(endpoint_cap[0], mg_str("tag")) >= 0) {
				struct mg_str tag_cap[2];

				if(mg_match(endpoint_cap[0], mg_str("tag/*"), tag_cap)) {
					char *name = malloc(tag_cap[0].len + 1);
					mg_str_to_str(name, tag_cap[0]);

					send_tag_res(c, http_msg, name, error_reply);
					free(name);
				}
				else if(mg_strcmp(endpoint_cap[0], mg_str("tag")) == 0) {
					send_tags_res(c, http_msg, error_reply);
				}
			}
		}
		else {
			struct mg_http_serve_opts opts = { .root_dir = ".", .fs = &mg_fs_posix };
			mg_http_serve_dir(c, http_msg, &opts);
		}
	}
	else if (ev == MG_EV_ERROR) {
		mg_http_reply(c, 500, "", "%m", MG_ESC("error"));
	}

	if(error_reply != NULL) {
		free(error_reply->json);
		free(error_reply->message);
	}
	free(error_reply);
}

int main() {
	printf("\n\n");
	printf(ANSI_COLOR_RGB(221, 36, 118) " ____ ____ ____ ____ ____ ____ ____ ____ ____ ____ " ANSI_RESET_ALL "\n");
	printf(ANSI_COLOR_RGB(231, 50, 97) "||D |||A |||T |||E |||. |||N |||O |||W |||( |||) ||" ANSI_RESET_ALL "\n");
	printf(ANSI_COLOR_RGB(246, 69, 67) "||__|||__|||__|||__|||__|||__|||__|||__|||__|||__||" ANSI_RESET_ALL "\n");
	printf(ANSI_COLOR_RGB(255, 81, 47) "|/__\\|/__\\|/__\\|/__\\|/__\\|/__\\|/__\\|/__\\|/__\\|/__\\|" ANSI_RESET_ALL "\n");
	printf("\n\n");

	printf("===\tDB - opening...\t===\n" );

	int atexit_return_code = atexit(clean_db);
	if(atexit_return_code != 0) {
		fprintf(stderr, "Couldn't correctly add atexit function. Error code: %d\n", atexit_return_code);
		return EXIT_FAILURE;
	}

	signal(SIGTERM, &sigTerm);

	int db_return_code = sqlite3_open("uwu.db", &db);
	if(SQLITE_OK != db_return_code) {
		fprintf(stderr, "DB - Can't open Database UwU: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return EXIT_FAILURE;
	}

	printf("===\tDB - successfully opened!\t===\n");

	struct mg_mgr mgr;
	mg_log_set(MG_LL_DEBUG);
	mg_mgr_init(&mgr);
	mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);

	for(;;) {
		mg_mgr_poll(&mgr, 1000);
	}

	return EXIT_SUCCESS;
}
