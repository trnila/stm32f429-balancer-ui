#pragma once

typedef struct {
	int fd;
	int processed;
	int read;
	char buffer[128];
} Serial;

Serial* serial_open(const char* path, int baud);
void serial_send(Serial* s, char cmd, char *data, int size);
char serial_read(Serial *s, char* data);
