/**
  ******************************************************************************
  * @file    FMC_SDRAM/stm32f4xx_it.c 
  * @author  MCD Application Team
  * @version V1.0.1
  * @date    11-November-2013
  * @brief   Main Interrupt Service Routines.
  *         This file provides template for all exceptions handler and
  *         peripherals interrupt service routine.
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
#include "stm32f4xx_it.h"
#include "./camera/bsp_ov5640.h"
#include "./systick/bsp_SysTick.h"
#include "./key/bsp_exti.h"
#include "image_processing.h"
#include "include.h"

extern unsigned int Task_Delay[];

/** @addtogroup STM32F429I_DISCOVERY_Examples
  * @{
  */

/** @addtogroup FMC_SDRAM
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
	printf("\r\n hardfault!");
	}
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{}

/**
  * @brief  This function handles PendSV_Handler exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{}

	
extern uint32_t Task_Delay[];
/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	uint8_t i;
	
	TimingDelay_Decrement();	//延时函数的中断服务
	
	for(i=0;i<NumOfTask;i++)
	{
		if(Task_Delay[i])
		{
			Task_Delay[i]--;
		}
	}
}



/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f429_439xx.s).                         */
/******************************************************************************/

extern uint16_t lcd_width, lcd_height;
extern uint16_t img_width, img_height;
extern uint8_t fps_temp;

//DCMI --> 缓存
static uint16_t line_num =0;	//记录传输了多少行
void DMA2_Stream1_IRQHandler(void)
{
  if(  DMA_GetITStatus(DMA2_Stream1,DMA_IT_TCIF1) == SET )    
  {
		/*行计数*/
		line_num++;

		if(line_num==img_height)
		{
			/*传输完一帧,计数复位*/
			line_num=0;
		}
		
		/*DMA 一行一行传输*/
		//OV5640_DMA_Config(FSMC_LCD_ADDRESS+(lcd_width*2*(lcd_height-line_num-1)),img_width*2/4);
		OV5640_DMA_Config(FSMC_LCD_ADDRESS,img_height*img_width*2/4);

		DMA_ClearITPendingBit(DMA2_Stream1,DMA_IT_TCIF1);
	}
}


//使用帧中断重置line_num,可防止有时掉数据的时候DMA传送行数出现偏移
//表示帧捕获结束，也就是一帧图像传输完成，全部数据已经通过DMA传输到显存中，DCMI->CR被清零，此时可以调用DCMI_CaptureCmd(ENABLE)读取下一帧
void DCMI_IRQHandler(void)	
{

	if(  DCMI_GetITStatus (DCMI_IT_FRAME) == SET )    
	{
		/*传输完一帧，计数复位*/
		line_num=0;
		
		fps_temp++; //帧率计数
		
		image_updata_flag = 1;	//表示一帧图像已经全部存入缓存
		
		DCMI_ClearITPendingBit(DCMI_IT_FRAME); 
	}
}


extern int flag;
//KEY1外部中断
void KEY1_IRQHandler(void)	//核心板上的按钮
{
	//确保是否产生了EXTI Line中断
	if(EXTI_GetITStatus(KEY1_INT_EXTI_LINE) != RESET) 
	{
		
		Mode_Change();
		
		//清除中断标志位
		EXTI_ClearITPendingBit(KEY1_INT_EXTI_LINE);
	}  
}

//KEY2外部中断
void KEY2_IRQHandler(void)
{
	//确保是否产生了EXTI Line中断
	if(EXTI_GetITStatus(KEY2_INT_EXTI_LINE) != RESET) 
	{
		if(flag == 1)
			flag = 0;
		else
			flag = 1;
			
//		printf("Key2\n");
		
		//清除中断标志位
		EXTI_ClearITPendingBit(KEY2_INT_EXTI_LINE);     
	}  
}

//		printf("Key1\n");
		
//		if(flag != 0)
//		{
//			Image_Process();
//		}


/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
