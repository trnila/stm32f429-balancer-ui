#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f4xx.h"
#include "uart.h"
#include "utils.h"


int x, y;

void uartTask(void* arg) {
	uart_init();

	char buffer[BUFFER_MAX];
	for(;;) {
		xQueueReceive(rx_queue, buffer, portMAX_DELAY);
		x = buffer[0];
		y = buffer[1];
	}
}

void display_init() {
	LCD_Init();
	LCD_LayerInit();
	LTDC_Cmd(ENABLE);
	LCD_SetLayer(LCD_FOREGROUND_LAYER);
	TP_Config();
}

void mainTask(void* arg) {
  TP_STATE* TP_State;

  int layer = 0;
  while (1)
  {
	  vTaskDelay(30);

    TP_State = IOE_TP_GetState();
    
    //s("DA", 2);

    //while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));
    LTDC_LayerCmd(LTDC_Layer1 + layer, ENABLE);
    layer ^= 0x1;
    LCD_SetLayer(layer);
    LTDC_LayerCmd(LTDC_Layer1 + layer, DISABLE);



    LCD_SetTextColor(LCD_COLOR_YELLOW);
    LCD_DrawFullRect(0, 0, LCD_PIXEL_WIDTH, LCD_PIXEL_HEIGHT);

    LCD_SetTextColor(LCD_COLOR_BLACK);
	  LCD_DrawFullCircle(x * (LCD_PIXEL_WIDTH - 20) / 255 + 10, y * (LCD_PIXEL_HEIGHT - 20) / 255 + 10, 10);

    if((TP_State->TouchDetected) && ((TP_State->Y < LCD_PIXEL_HEIGHT - 3) && (TP_State->Y >= 3)))
    {
      if((TP_State->X >= LCD_PIXEL_WIDTH - 3) || (TP_State->X < 3))
      {}
      else
      {
    	  ctrl_sendtarget(TP_State->X * 255 / LCD_PIXEL_WIDTH, TP_State->Y * 255 / LCD_PIXEL_HEIGHT);
      }
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

	rx_queue = xQueueCreate(BUFFER_MAX, 10);
	assert_param(rx_queue);

	xTaskCreate(mainTask, "display", 512, NULL, 1, NULL);
	xTaskCreate(uartTask, "uart", 512, NULL, 2, NULL);
	vTaskStartScheduler();

	halt();
}
