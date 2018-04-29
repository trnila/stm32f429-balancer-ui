#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f4xx.h"
#include "uart.h"
#include "utils.h"


int x, y;

int display_layer = 0;

void uartTask(void* arg) {
	uart_init();

	Frame frame;
	char decoded[2 * BUFFER_MAX];
	for(;;) {
		xQueueReceive(rx_queue, &frame, portMAX_DELAY);

		unstuffData(frame.buffer, frame.size, decoded);

		if(decoded[0] == 0x01) {
			x = decoded[1];
			y = decoded[2];
		}
	}
}

void display_init() {
	LCD_Init();
	LCD_LayerInit();
	LTDC_Cmd(ENABLE);
	LCD_SetLayer(LCD_FOREGROUND_LAYER);
	TP_Config();
}

void swap_buffers() {
	//while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));
	LTDC_LayerCmd(LTDC_Layer1 + display_layer, ENABLE);
	display_layer ^= 0x1;
	LCD_SetLayer(display_layer);
	LTDC_LayerCmd(LTDC_Layer1 + display_layer, DISABLE);
}

void mainTask(void* arg) {
	TP_STATE* TP_State;


	for(;;) {
		TP_State = IOE_TP_GetState();

		// use double buffering - render from one buffer, draw to another one
		swap_buffers();

		// clear screen
		LCD_SetTextColor(LCD_COLOR_YELLOW);
		LCD_DrawFullRect(0, 0, LCD_PIXEL_WIDTH, LCD_PIXEL_HEIGHT);

		// draw circle
		LCD_SetTextColor(LCD_COLOR_BLACK);
		LCD_DrawFullCircle(x * (LCD_PIXEL_WIDTH - 20) / 255 + 10, y * (LCD_PIXEL_HEIGHT - 20) / 255 + 10, 10);

		if(TP_State->TouchDetected) {
			ctrl_sendtarget(TP_State->X * 255 / LCD_PIXEL_WIDTH, TP_State->Y * 255 / LCD_PIXEL_HEIGHT);
		}

		vTaskDelay(30);
	}
}

void TP_Config(void) {
  LCD_Clear(LCD_COLOR_WHITE);
  
  if (IOE_Config() != IOE_OK) {
    LCD_Clear(LCD_COLOR_RED);
    LCD_SetTextColor(LCD_COLOR_BLACK); 
    LCD_DisplayStringLine(LCD_LINE_6,(uint8_t*)"   IOE NOT OK      ");
    LCD_DisplayStringLine(LCD_LINE_7,(uint8_t*)"Reset the board   ");
    LCD_DisplayStringLine(LCD_LINE_8,(uint8_t*)"and try again     ");
    for(;;);
  }
}


int main() {
	NVIC_SetPriority(PendSV_IRQn, 15);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	display_init();

	xTaskCreate(mainTask, "display", 512, NULL, 1, NULL);
	xTaskCreate(uartTask, "uart", 512, NULL, 2, NULL);
	vTaskStartScheduler();

	halt();
}
