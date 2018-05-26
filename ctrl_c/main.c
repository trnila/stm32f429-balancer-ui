#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <pthread.h>
#include <sys/select.h>
#include "api.h"
#include "serial.h"
#include "protocol.h"
#include "utils.h"

typedef struct {
	Serial *serial;
	Api *api;
} Context;

int width = 0, height = 0;
int targetX, targetY;

pthread_mutex_t announce_mutex = PTHREAD_MUTEX_INITIALIZER;


int extract_num(const char *str, const char* key) {
	char needle[strlen(key) + 3];
	strcpy(needle, key);
	strcat(needle, "\":");

	char *s = strstr(str, needle);
	if(!s) {
		fprintf(stderr, "Could not find '%s'\n", needle);
		return -1;
	}
	s += strlen(needle);

	int x;
	if(sscanf(s, "%d", &x) != 1) {
		fprintf(stderr, "Could not parse '%s'\n", needle);
		return -1;
	}

	return x;
}

void announce_target(Serial* s) {
	char pos[] = {targetX * 255 / width, targetY * 255 / height};
	pthread_mutex_lock(&announce_mutex);
	serial_send(s, CMD_TARGET, pos, sizeof(pos));	
	pthread_mutex_unlock(&announce_mutex);
}

void* process_events(void *arg) {
	Context *ctx = (Context*) arg;

	for(;;) {
		Event evt = api_get_event(ctx->api);
		if(width > 0 && height > 0 && strcmp(evt.event, "measurement") == 0) {
			int x = extract_num(evt.data, "POSX");
			int y = extract_num(evt.data, "POSY");

			if(x >= 0 && y >= 0) {
//				DEBUG("setting position [%d, %d]\n", x, y);
				char pos[] = {x * 255 / width, y * 255 / height};
				serial_send(ctx->serial, CMD_POS, pos, sizeof(pos));	
			}
		} else if(strcmp(evt.event, "dimension") == 0) {
			width = extract_num(evt.data, "Width");
			height = extract_num(evt.data, "Height");

			DEBUG("set dimension to %dx%d\n", width, height); 
		} else if(strcmp(evt.event, "target_position") == 0) {
			int x = extract_num(evt.data, "X");
			int y = extract_num(evt.data, "Y");

			if(x >= 0 && y >= 0) {
				DEBUG("setting target [%d, %d]\n", x, y);
				targetX = x;
				targetY = y;

				announce_target(ctx->serial);
			}
		} else {
			DEBUG("unknown event '%s'\n", evt.event);
		}
	}
}


void* send_touches(void* arg) {
	Context *ctx = (Context*) arg;
	char data[128];
	for(;;) {
		serial_read(ctx->serial, data);
		if(data[0] == CMD_SET_TARGET) {
		       api_set_target(ctx->api, data[1] * width / 255, data[2] * height / 255);
		} else if(data[0] == CMD_CONNECT) {
			DEBUG("Display connected\n");
			announce_target(ctx->serial);
	      	} else {
			DEBUG("Unknown CMD received from display: %d\n", data[0]);
		}
	}
}



int main(int argc, char *argv[]) {
	if(argc != 2) {
		fprintf(stderr, "Usage: %s api_host\n", argv[0]);
		return -1;
	}

	Api *api = api_init(argv[1]); 

	Serial *serial = serial_open("/dev/ttyAMA0", B9600);
	if(!serial) {
		fprintf(stderr, "could not open serial\n");
		return 1;
	}

	// spawn api_worker process, that keeps HTTP connection to api server open
	// receives target position from packet stream IPC and sends them over HTTP
	if(fork() == 0) {
		api_worker(api);
		return 0;
	}


	Context ctx = {.serial = serial, .api = api};
	pthread_t t;
	pthread_create(&t, NULL, send_touches, &ctx);
	pthread_detach(t);
	pthread_create(&t, NULL, process_events, &ctx);
	pthread_detach(t);

	api_listen_events(api);
	return 0;
}
