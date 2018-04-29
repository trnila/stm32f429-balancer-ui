#pragma once
#include "FreeRTOS.h"
#include "queue.h"

#define BUFFER_MAX 8

typedef struct {
	uint8_t buffer[BUFFER_MAX];
	size_t size;
} Frame;

void uart_init();
void ctrl_sendtarget(int x, int y);
void uart_send(const uint8_t *data, int size);

extern QueueHandle_t rx_queue;
