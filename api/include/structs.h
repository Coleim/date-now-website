#pragma once

#include <stdlib.h>

struct media {
	unsigned id;
	char *alternative_text;
	char *url;
	double width;
	double height;
};

struct user {
	int id;
	char *username;
	char *email;
	char *role;

	char totp_seed[255];

	struct media *picture;
	char *link;

	int subscribed_at;
	int is_supporter;

	int created_at;
};
