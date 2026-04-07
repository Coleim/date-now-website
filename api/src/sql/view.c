
#include <enums.h>
#include <lib/sqlite3.h>
#include <macros/colors.h>
#include <macros/sql.h>
#include <sql/view.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <structs.h>
#include <utils.h>

extern sqlite3 *db;

#define QUERY_COUNT_TMP "SELECT COUNT(*) FROM View"
#define QUERY_SELECT_TMP                                                       \
  "SELECT "                                                                    \
  "u.id, UNIXEPOCH(u.timestamp), u.hashedIp, u.issueId "                       \
  "FROM View u";
#define QUERY_SORT_TMP " ORDER BY i.timestamp COLLATE NOCASE %s"
#define QUERY_PAGINATION_TMP " LIMIT ?102 OFFSET ?103"

#define QUERY_POST_TMP                                                         \
  "INSERT INTO View (timestamp, hased_ip, issue_id)"                           \
  "VALUES (CURRENT_TIMESTAMP, ?, ?);";

int get_views_len(const struct mg_str *q) {
  printf(TERMINAL_SQL_MESSAGE("=== GET VIEWS COUNT SQL ==="));

  int query_rc = SQLITE_ROW;

  char *query_tmp = QUERY_COUNT_TMP;
  int query_len = strlen(query_tmp) + 2;
  char *query = malloc(query_len);
  strcpy(query, query_tmp);
  strcat(query, ";");

  int views_count = 0;

  sqlite3_stmt *stmt;
  query_rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
  if (query_rc != SQLITE_OK) {
    fprintf(stderr, TERMINAL_ERROR_MESSAGE("prepare error: %s\n"),
            sqlite3_errmsg(db));
    sqlite3_finalize(stmt);

    return query_rc;
  }

  GET_EXPANDED_QUERY(stmt);

  query_rc = sqlite3_step(stmt);

  if (query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
    fprintf(stderr, TERMINAL_ERROR_MESSAGE("prepare error: %s\n"),
            sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return query_rc;
  }

  while (query_rc != SQLITE_DONE) {
    if (sqlite3_column_type(stmt, 0) == SQLITE_INTEGER) {
      views_count = sqlite3_column_int(stmt, 0);
    }

    query_rc = sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
  free(query);

  return views_count;
}

int get_views(size_t len, struct view **arr, const struct mg_str *q,
              const struct mg_str *sort, int page, int page_size) {
  printf(TERMINAL_SQL_MESSAGE("=== GET VIEWS SQL ==="));

  int query_rc = SQLITE_ROW;

  char *query_tmp = QUERY_SELECT_TMP;
  char *query_pagination_tmp = QUERY_PAGINATION_TMP;

  // Sort tmp
  const char *sort_keyword = "ASC";
  char *query_sort_tmp = NULL;
  if (sort->len > 0) {
    if (strncasecmp(sort->buf, "desc", sort->len) == 0) {
      sort_keyword = "DESC";
    } else if (strncasecmp(sort->buf, "asc", sort->len) == 0) {
      sort_keyword = "ASC";
    } else {
      fprintf(stderr, TERMINAL_ERROR_MESSAGE("WRONG VALUE FOR SORTING"));
      return HTTP_BAD_REQUEST;
    }

    query_sort_tmp =
        malloc(snprintf(NULL, 0, QUERY_SORT_TMP, sort_keyword) + 1);
    sprintf(query_sort_tmp, QUERY_SORT_TMP, sort_keyword);
  }

  int query_len = strlen(query_tmp) + 2;

  if (sort->len > 0) {
    query_len += strlen(query_sort_tmp);
  }
  if (page > 0) {
    query_len += strlen(query_pagination_tmp);
  }

  char *query = malloc(query_len);
  strcpy(query, query_tmp);
  if (sort->len > 0) {
    strcat(query, query_sort_tmp);
  }
  if (page > 0) {
    strcat(query, query_pagination_tmp);
  }
  strcat(query, ";");
  free(query_sort_tmp);

  sqlite3_stmt *stmt;
  query_rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
  if (query_rc != SQLITE_OK) {
    fprintf(stderr, TERMINAL_ERROR_MESSAGE("prepare error: %s\n"),
            sqlite3_errmsg(db));
    sqlite3_finalize(stmt);

    return query_rc;
  }

  // Binding
  if (page > 0) {
    int offset = (page - 1) * page_size;
    sqlite3_bind_int(stmt, 102, page_size);
    sqlite3_bind_int(stmt, 103, offset);
  }

  GET_EXPANDED_QUERY(stmt);

  query_rc = sqlite3_step(stmt);

  if (query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
    fprintf(stderr, TERMINAL_ERROR_MESSAGE("prepare error: %s\n"),
            sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return query_rc;
  }

  size_t count = 0;
  while (query_rc == SQLITE_ROW && count < len) {
    struct view *u = NULL;
    u = malloc(sizeof(struct view));

    int view_init_rc = view_init(u);
    if (view_init_rc != 0) {
      fprintf(stderr, TERMINAL_ERROR_MESSAGE("The view is NULL"));
      return HTTP_INTERNAL_ERROR;
    }

    struct media *m = NULL;
    m = malloc(sizeof(struct media));

    int view_rc = view_map(u, stmt, 0, 3);
    if (view_rc != 0) {
      free(u);

      count += 1;
      query_rc = sqlite3_step(stmt);
      fprintf(stderr,
              TERMINAL_ERROR_MESSAGE("Error at line: %ld. Error code: %d"),
              count, query_rc);
      continue;
    }

    printf("\n");

    // Add a to arr
    arr[count] = u;

    count += 1;
    query_rc = sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);

  return 0;
}

int add_view(struct view *view) {
  printf(TERMINAL_SQL_MESSAGE("=== ADD VIEW SQL ==="));

  int query_rc = SQLITE_ROW;

  char *query_tmp = QUERY_POST_TMP;

  sqlite3_stmt *stmt;
  query_rc = sqlite3_prepare_v2(db, query_tmp, -1, &stmt, NULL);
  if (query_rc != SQLITE_OK) {
    fprintf(stderr, TERMINAL_ERROR_MESSAGE("prepare error: %s\n"),
            sqlite3_errmsg(db));
    sqlite3_finalize(stmt);

    return query_rc;
  }

  // Binding
  sqlite3_bind_text(stmt, 1, view->hashed_ip, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, view->issue_id);

  GET_EXPANDED_QUERY(stmt);

  query_rc = sqlite3_step(stmt);

  if (query_rc != SQLITE_ROW && query_rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    return query_rc;
  }

  sqlite3_finalize(stmt);

  return 0;
}
