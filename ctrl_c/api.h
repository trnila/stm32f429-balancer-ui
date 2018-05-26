#pragma once

typedef struct {
	char event[32];
	char data[256];
} Event;

#define MAX_EVENTS 5

typedef struct {
	char *host;
	int http_fd;
	int ipc[2];
} Api;


Api *api_init(const char *host);
int try_connect(const char* host);
void api_send(Api *api, char *buffer);
void api_worker(Api* api);
void api_listen_events(Api *api);
Event api_get_event(Api *api);
