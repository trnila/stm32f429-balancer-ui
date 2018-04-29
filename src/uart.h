#pragma once
#include "FreeRTOS.h"
#include "queue.h"

#define BUFFER_MAX 8

void uart_init();
void ctrl_sendtarget(int x, int y);
void uart_send(const uint8_t *data, int size);

extern QueueHandle_t rx_queue;
