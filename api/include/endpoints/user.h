#pragma once

#include <lib/mongoose.h>

void send_users_res(struct mg_connection *c, struct mg_http_message *msg);

void send_user_res(struct mg_connection *c, struct mg_http_message *msg, int id);
