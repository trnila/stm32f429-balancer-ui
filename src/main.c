#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f4xx.h"
#include "uart.h"
#include "utils.h"
#include "codec.h"


typedef struct {
	int x, y;
} Position;


int connected = 0;
Position pos = {.x = -1, .y = -1};
Position target = {.x = -1, .y = -1};
int display_layer = 0;

uint8_t iterations = 0;

void uartTask(void* arg) {
	Frame frame;
	char decoded[2 * BUFFER_MAX];
	for(;;) {
		xQueueReceive(rx_queue, &frame, portMAX_DELAY);

		unstuffData(frame.buffer, frame.size, decoded);

		if(decoded[0] == CMD_POS) {
			taskENTER_CRITICAL();
			pos.x = decoded[1];
			pos.y = decoded[2];

			connected = 1;
			iterations = 0;
			taskEXIT_CRITICAL();
		} else if(decoded[0] == CMD_TARGET) {
			taskENTER_CRITICAL();
			target.x = decoded[1];
			target.y = decoded[2];

			connected = 1;
			iterations = 0;
			taskEXIT_CRITICAL();
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
	LTDC_ReloadConfig(LTDC_IMReload);
}

void mainTask(void* arg) {
	TP_STATE* TP_State;


	for(;;) {
		taskENTER_CRITICAL();
		if(connected) {
			iterations = (iterations + 1);
			if(iterations > 100) {
				connected = 0;
			}
		}
		taskEXIT_CRITICAL();

		// use double buffering - render from one buffer, draw to another one
		swap_buffers();

		// clear screen
		LCD_SetTextColor(LCD_COLOR_YELLOW);
		LCD_DrawFullRect(0, 0, LCD_PIXEL_WIDTH, LCD_PIXEL_HEIGHT);
		LCD_SetBackColor(LCD_COLOR_YELLOW);

		if(!connected) {
			LCD_SetTextColor(LCD_COLOR_BLACK);
			LCD_DisplayStringLine(LCD_LINE_6, (uint8_t*) " connecting..");
		} else {
			TP_State = IOE_TP_GetState();

			// draw circle
			if(pos.x >= 0 && pos.y >= 0) {
				LCD_SetTextColor(LCD_COLOR_BLACK);
				LCD_DrawCircle(pos.x * (LCD_PIXEL_WIDTH - 20) / 255 + 10, pos.y * (LCD_PIXEL_HEIGHT - 20) / 255 + 10, 10);
			}

			if(target.x >= 0 && target.y >= 0) {
				LCD_SetTextColor(LCD_COLOR_BLACK);
				LCD_DrawFullCircle(target.x * (LCD_PIXEL_WIDTH - 20) / 255 + 10, target.y * (LCD_PIXEL_HEIGHT - 20) / 255 + 10, 10);
			}

			if(TP_State->TouchDetected) {
				int threshold = 10;
				int validTouch = TP_State->X > threshold && TP_State->X < LCD_PIXEL_WIDTH - threshold;
				validTouch &= TP_State->Y > threshold && TP_State->Y < LCD_PIXEL_HEIGHT - threshold;
				if(validTouch) {
					int X = TP_State->X * 255 / LCD_PIXEL_WIDTH;
					int Y = TP_State->Y * 255 / LCD_PIXEL_HEIGHT;

					// set current target optimistically
					taskENTER_CRITICAL();
					target.x = X;
					target.y = Y;
					taskEXIT_CRITICAL();

					ctrl_sendtarget(X, Y);
				}
			}

			vTaskDelay(30);
		}
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
	uart_init();

	xTaskCreate(mainTask, "display", 512, NULL, 1, NULL);
	xTaskCreate(uartTask, "uart", 512, NULL, 2, NULL);
	vTaskStartScheduler();

	halt();
}
