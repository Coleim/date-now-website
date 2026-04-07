
#pragma once

#include <lib/mongoose.h>
#include <stddef.h>
#include <structs.h>

int view_exists(int id);
int get_views_len(const struct mg_str *q);
int get_views(size_t len, struct view **arr, const struct mg_str *q,
              const struct mg_str *sort, int page, int page_size);
int get_view(struct view *view, int id);
int add_view(struct view *view);
