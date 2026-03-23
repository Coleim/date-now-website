#include <utils.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lib/sqlite3.h>
#include <structs.h>
#include <sql/user.h>
#include <enums.h>
#include <macros/colors.h>
#include <macros/sql.h>

extern sqlite3 *db;

int user_exists(int id) {
	printf(TERMINAL_SQL_MESSAGE("=== USER_EXISTS SQL ==="));

	int users_count = 0;

	char *query_tmp = "SELECT COUNT(*) FROM User WHERE id = ?;";

	sqlite3_stmt *stmt_count;
	sqlite3_prepare_v2(db, query_tmp, -1, &stmt_count, NULL);

	// Binding
	sqlite3_bind_int(stmt_count, 1, id);

	GET_EXPANDED_QUERY(stmt_count);

	int query_rc = sqlite3_step(stmt_count);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	while(query_rc != SQLITE_DONE) {
		if(sqlite3_column_type(stmt_count, 0) == SQLITE_INTEGER) {
			users_count = sqlite3_column_int(stmt_count, 0);
		}

		query_rc = sqlite3_step(stmt_count);
	}

	sqlite3_finalize(stmt_count);

	return users_count > 0;
}

int user_email_exists(char *username, char *email) {
	if(email == NULL) {
		return -1;
	}

	printf(TERMINAL_SQL_MESSAGE("=== USER EMAIL EXISTS SQL ==="));

	int users_count = 0;

	char *query_tmp = "SELECT COUNT(*) FROM User WHERE username = ? OR email = ?;";

	sqlite3_stmt *stmt_count;
	sqlite3_prepare_v2(db, query_tmp, -1, &stmt_count, NULL);

	// Binding
	sqlite3_bind_text(stmt_count, 1, username, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt_count, 2, email, -1, SQLITE_STATIC);

	GET_EXPANDED_QUERY(stmt_count);

	int query_rc = sqlite3_step(stmt_count);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	while(query_rc != SQLITE_DONE) {
		if(sqlite3_column_type(stmt_count, 0) == SQLITE_INTEGER) {
			users_count = sqlite3_column_int(stmt_count, 0);
		}

		query_rc = sqlite3_step(stmt_count);
	}

	sqlite3_finalize(stmt_count);

	return users_count > 0;
}

int get_users_len(const struct mg_str *q) {
	printf(TERMINAL_SQL_MESSAGE("=== GET USERS COUNT SQL ==="));

	char *q_str = NULL;

	char *query_tmp = "SELECT COUNT(*) FROM User";
	int query_len = strlen(query_tmp) + 2;
	if(q->len > 0) {
		q_str = malloc(q->len+2);
		sprintf(q_str, "%%%.*s%%", (int)q->len, q->buf);

		query_len += strlen(" WHERE username LIKE ?100 OR email LIKE ?100 OR link LIKE ?100");
	}

	char *query = malloc(query_len);
	strcpy(query, query_tmp);
	if(q->len > 0) {
		strcat(query, " WHERE username LIKE ?100 OR email LIKE ?100 OR link LIKE ?100");
	}
	strcat(query, ";");

	int users_count = 0;

	sqlite3_stmt *stmt_count;
	sqlite3_prepare_v2(db, query, -1, &stmt_count, NULL);

	// Binding
	if(q_str != NULL) {
		sqlite3_bind_text(stmt_count, 100, q_str, -1, SQLITE_STATIC);
	}

	GET_EXPANDED_QUERY(stmt_count);

	int query_rc = sqlite3_step(stmt_count);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		free(q_str);
		return query_rc;
	}

	while(query_rc != SQLITE_DONE) {
		if(sqlite3_column_type(stmt_count, 0) == SQLITE_INTEGER) {
			users_count = sqlite3_column_int(stmt_count, 0);
		}

		query_rc = sqlite3_step(stmt_count);
	}

	sqlite3_finalize(stmt_count);
	free(q_str);
	free(query);

	return users_count;
}

int get_users(size_t len, struct user **arr, const struct mg_str *q, const struct mg_str *sort, int page, int page_size) {
	printf(TERMINAL_SQL_MESSAGE("=== GET USERS SQL ==="));

	char *query_tmp = "SELECT "
		"u.id, u.username, u.email, u.role, u.link, UNIXEPOCH(u.subscribedAt), u.isSupporter, UNIXEPOCH(u.createdAt), m.id, m.textAlternatif, m.url, m.width, m.height "
		"FROM User u LEFT JOIN Media m ON m.id = u.picture";
	char *query_params_tmp = " WHERE username LIKE ?100 OR email LIKE ?100 OR link LIKE ?100";
	char *query_sort_tmp = " ORDER BY u.username ?101";
	char *query_pagination_tmp = " LIMIT ?102 OFFSET ?103";

	char *q_str = NULL;
	char *sort_str = NULL;

	int query_len = strlen(query_tmp) + 2;
	if(q->len > 0) {
		q_str = malloc(q->len+2);
		sprintf(q_str, "%%%.*s%%", (int)q->len, q->buf);

		query_len += strlen(query_params_tmp);
	}
	if(sort->len > 0) {
		sort_str = malloc(sort->len);
		sprintf(sort_str, "%.*s", (int)sort->len, sort->buf);

		query_len += strlen(query_sort_tmp);
	}
	if(page > 0) {
		query_len += strlen(query_pagination_tmp);
	}

	char *query = malloc(query_len);
	strcpy(query, query_tmp);
	if(q_str != NULL) {
		strcat(query, query_params_tmp);
	}
	if(sort_str != NULL) {
		strcat(query, query_sort_tmp);
	}
	if(page > 0) {
		strcat(query, query_pagination_tmp);
	}
	strcat(query, ";");

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(
			db,
			query,
			-1,
			&stmt,
			NULL
			);

	// Binding
	if(q_str != NULL) {
		sqlite3_bind_text(stmt, 100, q_str, -1, SQLITE_STATIC);
	}
	if(sort_str != NULL) {
		sqlite3_bind_text(stmt, 101, sort_str, -1, SQLITE_STATIC);
	}
	if(page > 0) {
		int offset = (page - 1) * page_size;
		sqlite3_bind_int(stmt, 102, page_size);
		sqlite3_bind_int(stmt, 103, offset);
	}


	GET_EXPANDED_QUERY(stmt);

	int query_rc = sqlite3_step(stmt);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		free(q_str);
		free(sort_str);
		return query_rc;
	}

	size_t count = 0;
	while(query_rc == SQLITE_ROW && count < len) {
		struct user *u = NULL;
		u = malloc(sizeof(struct user));

		int user_init_rc = user_init(u);
		if(user_init_rc != 0) {
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("The user is NULL"));
			free(q_str);
			free(sort_str);
			return HTTP_INTERNAL_ERROR;
		}

		struct media *m = NULL;
		m = malloc(sizeof(struct media));

		int user_rc = user_map(u, stmt, 0, 7);
		if(user_rc != 0) {
			free(u);

			count += 1;
			query_rc = sqlite3_step(stmt);
			fprintf(stderr, TERMINAL_ERROR_MESSAGE("Error at line: %ld. Error code: %d"), count, query_rc);
			continue;
		}

		// Picture
		int picture_rc = media_map(m, stmt, 8, 12);
		if(picture_rc != 0) {
			free(m);
		}
		else {
			u->picture = m;
		}

		printf("\n");

		// Add a to arr
		arr[count] = u;

		count += 1;
		query_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);
	free(q_str);
	free(sort_str);

	return 0;
}

int get_user(struct user *user, int id) {
	if(id < 0) {
		return HTTP_BAD_REQUEST;
	}

	printf(TERMINAL_SQL_MESSAGE("=== GET USER SQL ==="));

	char *query_tmp = "SELECT u.id, u.username, u.email, u.role, u.link, UNIXEPOCH(u.subscribedAt), u.isSupporter, UNIXEPOCH(u.createdAt), m.id, m.textAlternatif, m.url, m.width, m.height "
		"FROM User u LEFT JOIN Media m ON m.id = u.picture "
		"WHERE u.id = ?;";

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(
			db,
			query_tmp,
			-1,
			&stmt,
			NULL
			);

	// Binding
	sqlite3_bind_int(stmt, 1, id);

	GET_EXPANDED_QUERY(stmt);

	int query_rc = sqlite3_step(stmt);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}
	else if(query_rc == SQLITE_DONE) {
		return HTTP_NOT_FOUND;
	}

	while(query_rc == SQLITE_ROW) {
		int user_init_rc = user_init(user);
		if(user_init_rc != 0) {
			fprintf(stderr, "The user is NULL\n");
			return HTTP_INTERNAL_ERROR;
		}

		struct media *m = NULL;
		m = malloc(sizeof(struct media));

		int user_rc = user_map(user, stmt, 0, 7);
		if(user_rc != 0) {
			free(user);

			query_rc = sqlite3_step(stmt);
			continue;
		}

		// Picture
		int picture_rc = media_map(m, stmt, 8, 12);
		if(picture_rc != 0) {
			free(m);
		}
		else {
			user->picture = m;
		}

		printf("\n");
		query_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return 0;
}

int add_user(struct user *user) {
	printf(TERMINAL_SQL_MESSAGE("=== ADD USER SQL ==="));

	char *query_tmp =
		"INSERT INTO User (username, email, role, link)"
		"VALUES (?, ?, COALESCE(?, 'USER'), ?);";

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db, query_tmp, -1, &stmt, NULL);

	// Binding
	sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, user->email, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, user->role, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, user->link, -1, SQLITE_STATIC);

	GET_EXPANDED_QUERY(stmt);

	int query_rc = sqlite3_step(stmt);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	sqlite3_finalize(stmt);

	return 0;
}

int edit_user(struct user *user) {
	printf(TERMINAL_SQL_MESSAGE("=== EDIT USER SQL ==="));

	char *query_tmp =
		"UPDATE User "
		"SET username = ?, email = ?, role = COALESCE(?, 'USER'), link = ?, isSupporter = ? "
		"WHERE id = ?;";

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db, query_tmp, -1, &stmt, NULL);

	// Binding
	sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, user->email, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, user->role, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, user->link, -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 5, user->is_supporter);
	sqlite3_bind_int(stmt, 6, user->id);

	GET_EXPANDED_QUERY(stmt);

	int query_rc = sqlite3_step(stmt);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	sqlite3_finalize(stmt);

	return 0;
}

int delete_user(int id) {
	printf(TERMINAL_SQL_MESSAGE("=== DELETE USER SQL ==="));

	char *query_tmp =
		"DELETE FROM User "
		"WHERE id = ?;";

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db, query_tmp, -1, &stmt, NULL);

	// Binding
	sqlite3_bind_int(stmt, 1, id);

	GET_EXPANDED_QUERY(stmt);

	int query_rc = sqlite3_step(stmt);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	sqlite3_finalize(stmt);

	return 0;
}
