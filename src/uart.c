#include <memory.h>
#include "stm32f4xx.h"
#include "uart.h"
#include "codec.h"

const GPIO_TypeDef *uart_gpio = GPIOG;
const USART_TypeDef *usart = USART6;

Frame current_frame;
QueueHandle_t rx_queue;
uint8_t encoded[BUFFER_MAX], prepare_buffer[BUFFER_MAX];


void uart_send(const uint8_t *data, int size) {
	while(size--) {
		// wait for transmission complete
		while(!(usart->SR & USART_FLAG_TC));

		// send next byte
		USART_SendData(usart, *data);
		data++;
	}
}

void ctrl_sendcmd(char cmd, char*data, int size) {
	if(size >= sizeof(prepare_buffer)) {
		halt();
	}

	prepare_buffer[0] = cmd;
	memcpy(prepare_buffer + 1, data, size);

	// encode frame
	int frame_len = stuffData(prepare_buffer, size + 1, encoded);
	encoded[frame_len++] = 0; // frame terminator
	uart_send(encoded, frame_len);
}

void ctrl_sendtarget(int x, int y) {
	uint8_t data[] = {x, y};
	ctrl_sendcmd(CMD_SET_TARGET, data, sizeof(data));
}

void ctrl_connect() {
	ctrl_sendcmd(CMD_CONNECT, NULL, 0);
}

void uart_init() {
	// enable clocks for gpio and uart peripheral
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);


	// configure gpio
	GPIO_InitTypeDef gpioa_init_struct;
	gpioa_init_struct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_14;
	gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpioa_init_struct.GPIO_Mode = GPIO_Mode_AF;
	gpioa_init_struct.GPIO_PuPd = GPIO_PuPd_UP;
	gpioa_init_struct.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(uart_gpio, &gpioa_init_struct);

	// configure alternate functions to uart
	GPIO_PinAFConfig(uart_gpio, GPIO_PinSource9, GPIO_AF_USART6);
	GPIO_PinAFConfig(uart_gpio, GPIO_PinSource14, GPIO_AF_USART6);



	// configure uart peripheral
	USART_InitTypeDef usart1_init_struct;
	usart1_init_struct.USART_BaudRate = 9600;
	usart1_init_struct.USART_WordLength = USART_WordLength_8b;
	usart1_init_struct.USART_StopBits = USART_StopBits_1;
	usart1_init_struct.USART_Parity = USART_Parity_No ;
	usart1_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart1_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(usart, &usart1_init_struct);

	// prepare uart rx interrupts
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_SetPriority(USART6_IRQn, 15);
	USART_ITConfig(usart, USART_IT_RXNE, ENABLE);
	NVIC_EnableIRQ(USART6_IRQn);

	// finally enable uart
	USART_Cmd(usart, ENABLE);

	// create queue for incoming frames
	rx_queue = xQueueCreate(sizeof(Frame), 10);
	assert_param(rx_queue);
}

void USART6_IRQHandler() {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	// is received?
	if(USART_GetITStatus(usart, USART_IT_RXNE)){
		// get byte and clear interrupt flag
		uint8_t rcv = usart->DR;
		current_frame.buffer[current_frame.size] = rcv;

		if(rcv == 0) {
			xQueueSendFromISR(rx_queue, &current_frame, &xHigherPriorityTaskWoken);
		} else {
			current_frame.size++;
			if(current_frame.size >= BUFFER_MAX) {
				current_frame.size = 0;
			}
		}
	}

	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
