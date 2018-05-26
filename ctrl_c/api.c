#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include "api.h"
#include "utils.h"

#define IPC_CALLER  0
#define IPC_SERVICE 1

Api *api_init(const char *host) {
	Api *api = malloc(sizeof(Api));
	api->host = strdup(host);
	api->http_fd = -1;

	// create fd-aware bidirectional packet IPC
	if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, api->ipc) != 0) {
		perror("socketpair");
		return 0;
	}

	return api;
}

void api_send(Api *api, char *buffer) {
	if(api->http_fd < 0) {
		api->http_fd = try_connect(api->host);
	}

	if(api->http_fd >= 0) {
		write(api->http_fd, buffer, strlen(buffer));
	}
}

int try_connect(const char* host) {
    struct addrinfo hints, *res, *p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(host, "80", &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

	int fd = -1;
    for(p = res;p != NULL; p = p->ai_next) {
        void *addr;
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

	fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	if(!fd) {
		perror("failed to create socket");
		continue;
	}

	if(connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
		break;
	}

	perror("failed to connect");

	close(fd);
	fd = -1;
    }

    freeaddrinfo(res);
    return fd;
}

void api_set_target(Api *api, int x, int y) {
	int packet[] = {x, y};
	write(api->ipc[IPC_CALLER], packet, sizeof(packet));
}

void api_worker(Api *api) {
	int comm = api->ipc[IPC_SERVICE];

	char buffer[256];
	char payload[128];

	fd_set fds;
	for(;;) {
		FD_ZERO(&fds);
		FD_SET(comm, &fds);
		FD_SET(api->http_fd, &fds);

		struct timeval timeout = {.tv_sec = 3, .tv_usec = 0};
		if(select(100, &fds, NULL, NULL, &timeout) == 0) {
			snprintf(buffer, sizeof(buffer), "GET /api/ping HTTP/1.1\nHost: %s\n\n", api->host);
			api_send(api, buffer);
		}

		if(FD_ISSET(comm, &fds)) {
			int packet[2];
			read(comm, packet, sizeof(packet));

			DEBUG("comm: %d %d\n", packet[0], packet[1]);
			snprintf(payload, sizeof(payload), "{\"X\":%d,\"Y\":%d}\n", packet[0], packet[1]);
			snprintf(buffer, sizeof(buffer), "PUT /api/set_target HTTP/1.1\nContent-Length: %d\nHost: %s\n\n%s", strlen(payload), api->host,  payload);

			api_send(api, buffer);
		}
		if(FD_ISSET(api->http_fd, &fds)) {
			if(read(api->http_fd, buffer, sizeof(buffer)) == 0) {
				DEBUG("connection closed, reopening...\n");
				// reopen connection
				api->http_fd = try_connect(api->host);
				if(api->http_fd < 0) {
					DEBUG("could not reopen connection\n");
				}
			}
		}
	}
}


static int connect_sse(Api *api) {
	int fd = try_connect(api->host);
	if(fd < 0) {
		DEBUG("Could not connect to SSE stream\n");
		return -1;
	}

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "GET /events/measurements HTTP/1.1\nHost: %s\n\n", api->host);
	write(fd, buffer, strlen(buffer));

	return fd;
}


void api_listen_events(Api *api) {
	typedef enum {
		FIELD_NONE,
		FIELD_EVENT,
		FIELD_DATA
	} Field;

	Event evt;
	char data[512];
	char buffer[1024];
	int data_pos = 0;
	int drop = 0;
	Field field;
	int trim = 0;
	int fd = -1;
	while(1) {
		if(fd <= 0) {
			fd = connect_sse(api);
			if(fd < 0) {
				sleep(1);
			}
			continue;
		}

		int n = read(fd, buffer, sizeof(buffer));
		if(n <= 0) {
			DEBUG("Disconnected from SSE stream, reconnecting...");
			fd = -1;
			continue;
		}

		for(int i = 0; i < n; i++) {
			if(buffer[i] == '\n') {
				if(field == FIELD_EVENT) {
					data[data_pos++] = 0;
					strncpy(evt.event, data, data_pos);
				} else if(field == FIELD_DATA) {
					data[data_pos++] = 0;
					strncpy(evt.data, data, data_pos);

					// publish event to the queue
					write(api->ipc[IPC_SERVICE], &evt, sizeof(evt));
				}

				data_pos = 0;
				drop = 0;
				field = FIELD_NONE;
				trim = 0;
				memset(data, 0, sizeof(data));
				continue;
			}

			if(drop || (trim && buffer[i] == ' ')) {
				continue;
			}

			trim = 0;

			if(field == FIELD_NONE) {
				if(buffer[i] == ':') {
					if(strncmp(data, "event", data_pos) == 0) {
						field = FIELD_EVENT;
						data_pos = 0;
						trim = 1;
						continue;
					} else if(strncmp(data, "data", data_pos) == 0) {
						field = FIELD_DATA;
						data_pos = 0;
						trim = 1;
						continue;
					} else {
						drop = 1;
					}
				}
			}
			data[data_pos++] = buffer[i];
		}

	}
}

Event api_get_event(Api *api) {
	Event evt;
	read(api->ipc[IPC_CALLER], &evt, sizeof(Event));
	return evt;
}
