#include <utils.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lib/sqlite3.h>
#include <structs.h>
#include <sql/user.h>
#include <enums.h>

extern sqlite3 *db;

int user_exists(int id) {
	printf("Performing query...\n");

	int users_count = 0;

	char *query_tmp = "SELECT COUNT(*) FROM User WHERE id = ?;";

	sqlite3_stmt *stmt_count;
	sqlite3_prepare_v2(db, query_tmp, -1, &stmt_count, NULL);

	// Binding
	sqlite3_bind_int(stmt_count, 1, id);

	int query_rc = sqlite3_step(stmt_count);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	while(query_rc != SQLITE_DONE) {
		printf("count: %d\n", sqlite3_column_int(stmt_count, 0));
		if(sqlite3_column_type(stmt_count, 0) == SQLITE_INTEGER) {
			users_count = sqlite3_column_int(stmt_count, 0);
		}

		query_rc = sqlite3_step(stmt_count);
	}

	sqlite3_finalize(stmt_count);

	return users_count > 0;
}

int get_users_len() {
	printf("Performing query...\n");

	int users_count = 0;

	sqlite3_stmt *stmt_count;
	sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM User;", -1, &stmt_count, NULL);

	int query_rc = sqlite3_step(stmt_count);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	while(query_rc != SQLITE_DONE) {
		printf("count: %d\n", sqlite3_column_int(stmt_count, 0));
		if(sqlite3_column_type(stmt_count, 0) == SQLITE_INTEGER) {
			users_count = sqlite3_column_int(stmt_count, 0);
		}

		query_rc = sqlite3_step(stmt_count);
	}

	sqlite3_finalize(stmt_count);

	return users_count;
}

int get_users(size_t len, struct user **arr) {

	printf("Performing query...\n");

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(
			db,
			"SELECT "
			"u.id, u.username, u.email, u.role, u.link, UNIXEPOCH(u.subscribedAt), u.isSupporter, UNIXEPOCH(u.createdAt), m.id, m.textAlternatif, m.url, m.width, m.height "
			"FROM User u LEFT JOIN Media m ON m.id = u.picture;",
			-1,
			&stmt,
			NULL
			);

	printf("Got results:\n");

	int query_rc = sqlite3_step(stmt);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	size_t count = 0;
	while(query_rc == SQLITE_ROW && count < len) {
		int i;
		int num_cols = sqlite3_column_count(stmt);

		struct user *u = NULL;
		u = malloc(sizeof(struct user));

		int user_init_rc = user_init(u);
		if(user_init_rc != 0) {
			fprintf(stderr, "The user is NULL\n");
			return HTTP_INTERNAL_ERROR;
		}

		struct media *m = NULL;
		m = malloc(sizeof(struct media));

		int user_rc = user_map(u, stmt, 0, 7);
		if(user_rc != 0) {
			free(u);
			printf("\n");

			count += 1;
			query_rc = sqlite3_step(stmt);
			fprintf(stderr, "Error at line: %ld. Error code: %d\n", count, query_rc);
			printf("Row appended, row count: %ld\n", count - 1);
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
		printf("Row appended, row count: %ld\n", count - 1);
	}

	sqlite3_finalize(stmt);

	return 0;
}

int get_user(struct user *user, int id) {
	if(id < 0) {
		return HTTP_BAD_REQUEST;
	}

	printf("Performing query...\n");

	char *query_tmp = "SELECT u.id, u.username, u.role, u.link, u.subscribedAt, u.isSupporter, u.createdAt, m.id, m.textAlternatif, m.url, m.width, m.height "
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

	printf("Got results:\n");

	int query_rc = sqlite3_step(stmt);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}
	else if(query_rc == SQLITE_DONE) {
		return HTTP_NOT_FOUND;
	}

	while(query_rc == SQLITE_ROW) {
		int i;
		int num_cols = sqlite3_column_count(stmt);

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
	printf("Performing query...\n");

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

	int query_rc = sqlite3_step(stmt);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	sqlite3_finalize(stmt);

	return 0;
}

int edit_user(struct user *user) {
	printf("Performing query...\n");

	char *query_tmp =
		"UPDATE User "
		"SET link = ? "
		"WHERE id = ?;";

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db, query_tmp, -1, &stmt, NULL);

	// Binding
	sqlite3_bind_text(stmt, 1, user->link, -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 2, user->id);

	int query_rc = sqlite3_step(stmt);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	sqlite3_finalize(stmt);

	return 0;
}

int delete_user(int id) {
	printf("Performing query...\n");

	char *query_tmp =
		"DELETE FROM User "
		"WHERE id = ?;";

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db, query_tmp, -1, &stmt, NULL);

	// Binding
	sqlite3_bind_int(stmt, 1, id);

	int query_rc = sqlite3_step(stmt);

	if(query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
		return query_rc;
	}

	sqlite3_finalize(stmt);

	return 0;
}
