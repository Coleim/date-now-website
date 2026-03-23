#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <lib/mongoose.h>
#include <lib/sqlite3.h>
#include <endpoints/user.h>

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
	if (ev == MG_EV_HTTP_MSG) {
		struct mg_http_message *http_msg = (struct mg_http_message *) ev_data;
		struct mg_str endpoint_cap[2];

		if(mg_match(http_msg->uri, mg_str("/api/#"), endpoint_cap)) {
			printf("endpoint:  %.*s\n", (int)endpoint_cap[0].len, endpoint_cap[0].buf);

			if(mg_strcmp(endpoint_cap[0], mg_str("user")) >= 0) {
				struct mg_str user_cap[2];
				printf("Author endpoints\n");

				if(mg_match(endpoint_cap[0], mg_str("user/*"), user_cap)) {
					printf("USER SINGLE - ID: %.*s\n", (int)user_cap[0].len, user_cap[0].buf);
					int id;
					int id_parsed = mg_str_to_num(user_cap[0], 10, &id, sizeof(int));
					if(!id_parsed) {
						mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{ \"code\": 400, \"error\": \"ID is not a number.\" }");
						return;
					}

					send_user_res(c, http_msg, id);
				}
				else if(mg_strcmp(endpoint_cap[0], mg_str("user")) == 0) {
					printf("USERS LIST\n");
					send_users_res(c, http_msg);
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
}

int main() {
	printf("DB - opening...\n" );

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

	printf("DB - successfully opened!\n");

	struct mg_mgr mgr;
	mg_log_set(MG_LL_DEBUG);
	mg_mgr_init(&mgr);
	mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);

	for(;;) {
		mg_mgr_poll(&mgr, 1000);
	}

	return EXIT_SUCCESS;
}
