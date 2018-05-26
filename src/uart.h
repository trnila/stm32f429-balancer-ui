#pragma once
#include "FreeRTOS.h"
#include "queue.h"

#define BUFFER_MAX 8

#define CMD_POS 0x01
#define CMD_TARGET 0x02
#define CMD_SET_TARGET 0x82
#define CMD_CONNECT 0x83

typedef struct {
	uint8_t buffer[BUFFER_MAX];
	size_t size;
} Frame;

void uart_init();
void ctrl_sendtarget(int x, int y);
void ctrl_connect();
void uart_send(const uint8_t *data, int size);

extern QueueHandle_t rx_queue;
