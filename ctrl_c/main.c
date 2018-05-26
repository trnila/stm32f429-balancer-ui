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

void process_events(Context *ctx) {
	for(;;) {
		Event evt = api_get_event(ctx->api);
		if(width > 0 && height > 0 && strcmp(evt.event, "measurement") == 0) {
			int x = extract_num(evt.data, "POSX");
			int y = extract_num(evt.data, "POSY");

			if(x >= 0 && y >= 0) {
				DEBUG("setting position [%d, %d]\n", x, y);
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
				char pos[] = {x * 255 / width, y * 255 / height};
				serial_send(ctx->serial, CMD_TARGET, pos, sizeof(pos));	
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
	pthread_create(&t, NULL, process_events, &ctx);

	api_listen_events(api);
	return 0;
}
