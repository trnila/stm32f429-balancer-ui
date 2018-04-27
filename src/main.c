/**
  ******************************************************************************
  * @file    Touch_Panel/main.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    20-September-2013
  * @brief   This example describes how to configure and use the touch panel 
  *          mounted on STM32F429I-DISCO boards.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32F429I_DISCOVERY_Examples
  * @{
  */

/** @addtogroup Touch_Panel
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void TP_Config(void);

/* Private functions ---------------------------------------------------------*/

#include "stm32f4xx.h"

void s(const char *data, int size) {
	while(size--) {
		while( !(USART6->SR & 0x00000040) );
		USART_SendData(USART6, *data);
		data++;
	}

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

/*	GPIO_StructInit(&gpioa_init_struct);
	gpioa_init_struct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	gpioa_init_struct.GPIO_Speed = GPIO_Speed_25MHz;
	gpioa_init_struct.GPIO_Mode = GPIO_Mode_OUT;
	gpioa_init_struct.GPIO_OType = GPIO_OType_PP;
	gpioa_init_struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOG, &gpioa_init_struct);

	for(;;) {
		GPIO_ToggleBits(GPIOG, GPIO_Pin_10);
		for(int index = (10000); index != 0; index--);
		GPIO_ToggleBits(GPIOG, GPIO_Pin_11);
		for(int index = (10000); index != 0; index--);
	}*/




	/* GPIOA PIN9 alternative function Tx */
	gpioa_init_struct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_14;
	gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpioa_init_struct.GPIO_Mode = GPIO_Mode_AF;
	gpioa_init_struct.GPIO_PuPd = GPIO_PuPd_UP;
	gpioa_init_struct.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOG, &gpioa_init_struct);

	GPIO_PinAFConfig(GPIOG, GPIO_PinSource9, GPIO_AF_USART6);
		GPIO_PinAFConfig(GPIOG, GPIO_PinSource14, GPIO_AF_USART6);


	/* Enable USART2 */

	/* Baud rate 9600, 8-bit data, One stop bit
	 * No parity, Do both Rx and Tx, No HW flow control
	 */
	usart1_init_struct.USART_BaudRate = 9600;
	usart1_init_struct.USART_WordLength = USART_WordLength_8b;
	usart1_init_struct.USART_StopBits = USART_StopBits_1;
	usart1_init_struct.USART_Parity = USART_Parity_No ;
	usart1_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart1_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	/* Configure USART2 */
	USART_Init(USART6, &usart1_init_struct);

	/* Enable RXNE interrupt */
	USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure; // this is used to configure the NVIC (nested vector interrupt controller)
	NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;		 // we want to configure the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;// this sets the priority group of the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		 // this sets the subpriority inside the group
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			 // the USART1 interrupts are globally enabled
	NVIC_Init(&NVIC_InitStructure);

	/* Enable USART2 global interrupt */
	NVIC_EnableIRQ(USART6_IRQn);


	USART_Cmd(USART6, ENABLE);
}

uint8_t buffer[8];
int pos = 0;

int x = 0, y = 0;

void USART6_IRQHandler() {
	if( USART_GetITStatus(USART6, USART_IT_RXNE) ){
		buffer[pos] = USART6->DR;

		if(pos >= 1) {
			x = buffer[0];
			y = buffer[1];
			pos = 0;
		} else {
			pos = (pos + 1) % sizeof(buffer);
		}
	}
}

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  uint16_t linenum = 0;
  static TP_STATE* TP_State; 
    
  /*!< At this stage the microcontroller clock setting is already configured, 
  this is done through SystemInit() function which is called from startup
  file (startup_stm32f429_439xx.s) before to branch to application main.
  To reconfigure the default setting of SystemInit() function, refer to
  system_stm32f4xx.c file
  */



  /* LCD initiatization */
  LCD_Init();
  
  /* LCD Layer initiatization */
  LCD_LayerInit();
    
  /* Enable the LTDC */
  LTDC_Cmd(ENABLE);
  
  /* Set LCD foreground layer */
  LCD_SetLayer(LCD_FOREGROUND_LAYER);
  
  /* Touch Panel configuration */
  TP_Config();

  uart_init();

  int layer = 0;
  while (1)
  {
 
    TP_State = IOE_TP_GetState();
    
    s("DA", 2);

    while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));
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
    	  //LCD_Clear(LCD_COLOR_WHITE);
        LCD_DrawFullCircle(TP_State->X, TP_State->Y, 10);
      }
    }
  }
}

/**
* @brief  Configure the IO Expander and the Touch Panel.
* @param  None
* @retval None
*/
static void TP_Config(void)
{
  /* Clear the LCD */ 
  LCD_Clear(LCD_COLOR_WHITE);
  
  /* Configure the IO Expander */
  if (IOE_Config() == IOE_OK)
  {   
    //LCD_SetFont(&Font8x8);


    //LCD_SetBackColor(LCD_COLOR_BLUE2);
    //LCD_DrawRect(0, 0, LCD_PIXEL_WIDTH, LCD_PIXEL_HEIGHT);

  /*  LCD_DrawFullRect(40, 250, 30, 30);
    LCD_SetTextColor(LCD_COLOR_YELLOW); 
    LCD_DrawFullRect(75, 250, 30, 30);
    LCD_SetTextColor(LCD_COLOR_RED); 
    LCD_DrawFullRect(5, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_BLUE); 
    LCD_DrawFullRect(40, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_GREEN); 
    LCD_DrawFullRect(75, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_MAGENTA); 
    LCD_DrawFullRect(145, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_BLACK); 
    LCD_DrawFullRect(110, 288, 30, 30);
    LCD_DrawRect(180, 270, 48, 50);
    LCD_SetFont(&Font16x24);
    LCD_DisplayChar(LCD_LINE_12, 195, 0x43);
    LCD_DrawLine(0, 248, 240, LCD_DIR_HORIZONTAL);
    LCD_DrawLine(0, 284, 180, LCD_DIR_HORIZONTAL);
    LCD_DrawLine(1, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(37, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(72, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(107, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(142, 284, 36, LCD_DIR_VERTICAL);
    LCD_DrawLine(0, 319, 240, LCD_DIR_HORIZONTAL);
    */
  }  
  else
  {
    LCD_Clear(LCD_COLOR_RED);
    LCD_SetTextColor(LCD_COLOR_BLACK); 
    LCD_DisplayStringLine(LCD_LINE_6,(uint8_t*)"   IOE NOT OK      ");
    LCD_DisplayStringLine(LCD_LINE_7,(uint8_t*)"Reset the board   ");
    LCD_DisplayStringLine(LCD_LINE_8,(uint8_t*)"and try again     ");
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
