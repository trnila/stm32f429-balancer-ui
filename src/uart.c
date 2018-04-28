#include "stm32f4xx.h"
#include "uart.h"

uint8_t buffer[BUFFER_MAX];
int pos = 0;

int x = 0, y = 0;
QueueHandle_t rx_queue;


void uart_send(const uint8_t *data, int size) {
	while(size--) {
		while( !(USART6->SR & 0x00000040) );
		USART_SendData(USART6, *data);
		data++;
	}
}

void ctrl_sendtarget(int x, int y) {
	uint8_t data[] = {x, y};
	uart_send(data, sizeof(data));
}

void uart_init() {
	USART_InitTypeDef usart1_init_struct;
	GPIO_InitTypeDef gpioa_init_struct;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);


	/* GPIOA PIN9 alternative function Tx */
	gpioa_init_struct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_14;
	gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpioa_init_struct.GPIO_Mode = GPIO_Mode_AF;
	gpioa_init_struct.GPIO_PuPd = GPIO_PuPd_UP;
	gpioa_init_struct.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOG, &gpioa_init_struct);

	GPIO_PinAFConfig(GPIOG, GPIO_PinSource9, GPIO_AF_USART6);
		GPIO_PinAFConfig(GPIOG, GPIO_PinSource14, GPIO_AF_USART6);

	usart1_init_struct.USART_BaudRate = 9600;
	usart1_init_struct.USART_WordLength = USART_WordLength_8b;
	usart1_init_struct.USART_StopBits = USART_StopBits_1;
	usart1_init_struct.USART_Parity = USART_Parity_No ;
	usart1_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart1_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	/* Configure USART2 */
	USART_Init(USART6, &usart1_init_struct);

	/* Enable RXNE interrupt */

	NVIC_InitTypeDef NVIC_InitStructure; // this is used to configure the NVIC (nested vector interrupt controller)
	NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;		 // we want to configure the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;// this sets the priority group of the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		 // this sets the subpriority inside the group
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			 // the USART1 interrupts are globally enabled
	NVIC_Init(&NVIC_InitStructure);

	NVIC_SetPriority(USART6_IRQn, 15);

	USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);

	/* Enable USART2 global interrupt */
	NVIC_EnableIRQ(USART6_IRQn);


	USART_Cmd(USART6, ENABLE);
}

void USART6_IRQHandler() {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	volatile int ret = 111;

	if( USART_GetITStatus(USART6, USART_IT_RXNE) ){
		buffer[pos] = USART6->DR;

		if(pos >= 1) {
			//x = buffer[0];
			//y = buffer[1];
			pos = 0;

			ret = xQueueSendFromISR(rx_queue, buffer, &xHigherPriorityTaskWoken);
		} else {
			pos = (pos + 1) % sizeof(buffer);
		}
	}

	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
