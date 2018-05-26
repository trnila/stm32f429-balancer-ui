#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <assert.h>
#include "serial.h"
#include "codec.h"

Serial* serial_open(const char* path, int speed) {
	int fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		return NULL;
	}

	struct termios tty;
	memset(&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0) {
		return NULL;
	}

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
	tty.c_iflag &= ~IGNBRK;
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VMIN]  = 0;
	tty.c_cc[VTIME] = 5;

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= 0;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0) {
		return NULL;
	}

	Serial *ret = (Serial*) malloc(sizeof(Serial));
	ret->fd = fd;
	ret->read = 0;
	ret->processed = 0;
	return ret;
}

void serial_send(Serial* s, char cmd, char *data, int size) {
	char prepare[128], encoded[128];
	prepare[0] = cmd;
	memcpy(prepare + 1, data, size);

	size = cobs_encode(prepare, size + 1, encoded);
	encoded[size++] = 0;
	write(s->fd, encoded, size);
}

char serial_read(Serial *s, char* data) {
	char line[128];
	int l = 0;
	for(;;) {
		for(; s->processed < s->read; s->processed++) {
			if(s->buffer[s->processed] == 0) {
				memset(data, 0, 30);
				cobs_decode(line, l, data);

				s->processed++;
				return 0;
			} else {
				line[l++] = s->buffer[s->processed];
			}
		}

		s->read = read(s->fd, s->buffer, sizeof(s->buffer));
		s->processed = 0;
	}

}

