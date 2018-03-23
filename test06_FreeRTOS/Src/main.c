/*------------------------------------------------------------------------------  
    PROGRAM PRO PSD REGULATOR V SYSTEMU FreeRTOS v 8.0.0
    
    main.c -  HLAVNI ZDROJOVY SOUBOR
              obsahuje funkci main(), obsluznou rutinu preruseni od A/D prevodniku
              ADC_IRQHandler() a hook funkci volanou pri preteceni zasobniku
              vApplicationStackOverflowHook()
    
    autor:  Adolf Gothard, xgotha01
            UAMT FEKT VUT v Brne, 2014
    
    vytvoreno v ramci diplomove prace
    "Implementace RTOS do mikrokontroleru STM32 s jadrem ARM Cortex-M4F"
    
------------------------------------------------------------------------------*/



/* systemove a aplikacni hlavicky --------------------------------------------*/
#include "stm32f4_discovery.h"
#include "stm32f4xx_conf.h"
#include "main_tasks.h"

/* hlavicky systemu FreeRTOS -------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"


/* inicializacni struktury ---------------------------------------------------*/
GPIO_InitTypeDef  GPIO_InitStructure;
ADC_InitTypeDef       ADC_InitStructure;
ADC_CommonInitTypeDef ADC_CommonInitStructure;
DAC_InitTypeDef   DAC_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;

/* definice priority uloh a velikosti zasobniku ------------------------------*/
#define mainPSDTask_PRIORITY ( tskIDLE_PRIORITY + 1 )
#define mainPSDTask_STACK_SIZE ( configMINIMAL_STACK_SIZE + 800 )



/* obsluzna rutina preruseni od ADC */
/*////////////////////////////////////////////////////////////////////////////*/
void ADC_IRQHandler(void)
{
  portBASE_TYPE lHigherPriorityTaskWoken = pdFALSE;
  uint16_t uiADCDataISR;
  
  if( (ADC1->SR & ADC_SR_EOC) == ADC_SR_EOC )   /* test priznaku ukonceni prevoodu ADC1 */
  {
    uiADCDataISR = (ADC1->DR & 0x0fff);
    
    if(xADCQueue != NULL)
    {
      /* pokud je fronta v poradku, posli data do fronty */
      if( xQueueSendFromISR( xADCQueue, &uiADCDataISR, &lHigherPriorityTaskWoken ) == pdTRUE )
      { /* data v poradku odeslana do fronty */
      
      }
    }
  } 
  
  portEND_SWITCHING_ISR( lHigherPriorityTaskWoken );
}
/*////////////////////////////////////////////////////////////////////////////*/



/* HLAVNI FUNKCE - vstupni bod programu po resetu */
int main(void)
{

/* konfigurace periferii mikrokontroleru  */
/*----------------------------------------------------------------------------*/ 

  /* Povoleni hodin pro periferie */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);   /* Port A */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);   /* Port C */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);   /* Port D */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);    /* A/D prevodnik ADC1 */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);     /* D/A prevodnik */
  
  /* Nastaveni pinu PD12, PD13, PD14 a PD15 jako vystupnich v push-pull modu */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  /* nastaveni pinu PC1 do Analogoveho modu - pro AD prevodnik */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* nastaveni PA4 a PA5 do Analogoveho modu - pro D/A prevodnik */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
/*----------------------------------------------------------------------------*/



/* konfigurace A/D prevodniku  */
/*----------------------------------------------------------------------------*/ 
  
  /* konfigurace AD prevodniku - spolecne*/
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);
  
  /* konfigurace prevodniku ADC 1 */
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC1, &ADC_InitStructure);
  
  ADC1->CR1 |= ADC_CR1_EOCIE; /* povoleni preruseni pri nastaveni priznaku EOC */
  ADC1->CR2 |= ADC_CR2_ADON;  /* zapnuti vlastniho prevodniku ADC1 */
  
  /* nastaveni kanalu 11 (PC1) a vzorkovacho casu */
  ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 1, ADC_SampleTime_15Cycles);
  
/*----------------------------------------------------------------------------*/ 

 
  
/* konfigurace D/A prevodniku  */
/*----------------------------------------------------------------------------*/ 
  
  /* konfigurace kanalu 1 D/A prevodniku */
  DAC_InitStructure.DAC_Trigger = DAC_Trigger_Software;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable; 
  DAC_Init( DAC_Channel_1, &DAC_InitStructure );
  
  /* povoleni kanalu 1 D/A prevodniku */
  DAC_Cmd( DAC_Channel_1, ENABLE );
  
/*----------------------------------------------------------------------------*/ 



/* konfigurace prerusovaciho systemu - DULEZITE */ 
/*----------------------------------------------------------------------------*/ 

  /* prirazeni vsech ctyr prioritnich bitu jako preemptivnich */
  /*  NUTNO VOLAT PRED FUNKCI NVIC_Init()             
      NVIC_Init() vyuziva registr AIRCR, ktery je spravne nastaven pomoci NVIC_PriorityGroupConfig() */
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

  /* konfigurace jednotky NVIC */
	NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;   /* vyber kanalu - A/D prevodnik */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0xf;  /* nastaveni priority */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0xf;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
  
/*----------------------------------------------------------------------------*/


  /* vytvoreni uloh */
  xTaskCreate( vPSDTask, "PSD1", mainPSDTask_STACK_SIZE, NULL, mainPSDTask_PRIORITY, NULL );
  
  /* start operacniho systemu */
  vTaskStartScheduler();
  
  for( ;; )
  {}
}

/* systemova funkce volana pro preteceni zasobniku */
void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed portCHAR *pcTaskName )
{
  /* indikace preteceni zasobniku - rozsviceni vsech LED */
  STM_EVAL_LEDOn(LED3);
  STM_EVAL_LEDOn(LED4);
  STM_EVAL_LEDOn(LED5);
  STM_EVAL_LEDOn(LED6);
  for( ;; );
}