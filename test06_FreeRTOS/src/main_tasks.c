/*------------------------------------------------------------------------------  
    PROGRAM PRO PSD REGULATOR V SYSTEMU FreeRTOS v 8.0.0
    
    main_tasks.c  - ZDROJOVY SOUBOR ULOH
                    obsahuje ulohu pro PSD regulator vPSDTask()
                    
    autor:  Adolf Gothard, xgotha01
            UAMT FEKT VUT v Brne, 2014
    
    vytvoreno v ramci diplomove prace
    "Implementace RTOS do mikrokontroleru STM32 s jadrem ARM Cortex-M4F"
    
------------------------------------------------------------------------------*/



#include <math.h>   /* matematicka knihovna - pro regulacni vypocty */

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

/* deklarace front */
xQueueHandle xADCQueue;     /* fronta pro predavani prevedenych dat z ISR od A/D prevodniku */
xQueueHandle xParQueue;     /* fronta pro predavani parametru z nadrazene ulohy */



/* ULOHA PRO PSD REGULATOR */
/*----------------------------------------------------------------------------*/
void vPSDTask( void *pvParameters )
{
  uint16_t uiInputData;               /* surova data z A/D prevodniku */
  uint16_t uiOutputData;              /* data pro D/A prevodnik */
  
  float fInputVal = 0.0f;             /* promenna pro vstupni signal, tj. vystup soustavy */
  float fErrorVal;                    /* promenna pro regulacni odchylku */
  float fOutputVal;                   /* promenna pro vystup regulatoru - akcni zasah */
  
  static float fInputLSB;             /* LSB vstupniho signalu */
  static float fOutputLSB;            /* LSB vystupniho signalu */
  static float fSumVal = 0.0f;        /* hodnota sumatoru */
  static float fDifVal = 0.0f;        /* diference */
  static float fSetPointVal = 0.0f;   /* promenna pro zadanou hodnotu */
  static float f_TS_Val = ( T_S / 1000.0f ); /* prepocet vzorkovaci periody na [s] */                            
  static float f_TD_Val = T_D;        /* promenna pro derivacni cas. konstantu */
  static float f_TI_Val = T_I;        /* promenna pro integracni cas. konstantu */
  static float f_TT_Val = T_T;        /* promenna pro cas. konstantu pro anti wind-up */
  static float f_K_Val = K;           /* promenna pro zesileni regulatoru */
  static uint8_t ui_N_Val = N;        /* promenna pro parametr filtru dreivacni slozky */
  
  xParQueueMessage_t xMsg1;           /* struktura pro prijem parametru */
  
  portTickType xActTime;              /* promenna pro aktualni cas */
  
  
  /* vzorkovaci perioda vypocet podle vztahu (8-1)*/
  const portTickType xFrequency = ( (1000 / configTICK_RATE_HZ) * T_S ); 
  
  /* vytvoreni fronty pro hodnoty z A/D prevodniku a test, zda byla vytvorena */
  xADCQueue = xQueueCreate( ADC_QUEUE_LENGTH, sizeof(uint16_t) );
  
  if(xADCQueue != 0) 
  { /* fronta byla uspesne vytvorena */
    STM_EVAL_LEDOn(LED6);
  }
  
  /* vytvoreni fronty pro predavani parametru regulatoru a test, zda byla vytvorena */
  xParQueue = xQueueCreate( PAR_QUEUE_LENGTH, sizeof( xParQueueMessage_t * ) );
  
  if(xParQueue != 0) 
  { /* fronta byla uspesne vytvorena */
      STM_EVAL_LEDOff(LED6);
  }
  
  /* vypocet LSB hodnoty vstupniho a vystupniho signalu podle konstant pro regulator - vztah (8-2) */
  fInputLSB = (float)( PSD_IN_RNG / ( (float)( (2 << ( PSD_IN_RES - 1 )) - 1 ) ) );
  fOutputLSB = (float)( PSD_OUT_RNG / ( (float)( (2 << ( PSD_OUT_RES - 1 )) - 1 ) ) );


  /* HLAVNI SMYCKA ULOHY */
  for( ;; )
  {
    xActTime = xTaskGetTickCount();  /* zjisteni aktualniho systemoveho casu */
    
    ADC1->CR2 |= ADC_CR2_SWSTART; /* spusteni A/D prevodu */
    
    /* CTENI PREVEDENYCH DAT Z FRONTY ----------------------------------------*/
    if(xADCQueue != 0)
    { /* fronta v poradku */
      /* cekej na data, pokud data nejsou k dispozici, dochazi k blokovani ulohy */
      if( xQueueReceive( xADCQueue, &uiInputData, portMAX_DELAY ) == pdTRUE )
      { /* data v poradku prijata */
        /* prepocet realne hodnoty vstupniho signalu podle vztahu (8-3)*/
        fInputVal = PSD_IN_MIN + ( uiInputData * fInputLSB );
        
        STM_EVAL_LEDToggle(LED4);   /* prepni zelenou LED, pouze pro kontrolu */
      }
    }
    
    /* KONTROLA ZMENY PARAMTERU REGULATORU -----------------------------------*/
    if(xParQueue != 0)
    { /* fronta v poradku */
      /* cti data, pokud data nejsou k dispozici behem 1 systemoveho tiku, pokracuj dale */
      if( xQueueReceive( xParQueue, &xMsg1, (portTickType)1 ) == pdTRUE )
      { /* data v poradku prijata, aktualizuj parametry */
        fSetPointVal = xMsg1.fSetPointMsg;
        f_TD_Val = xMsg1.f_TD_Msg;
        f_TI_Val = xMsg1.f_TI_Msg;
        f_TT_Val = xMsg1.f_TT_Msg;
        f_K_Val = xMsg1.f_K_Msg;
        ui_N_Val = xMsg1.ui_N_Msg;
      }
    }
    
    /* REGULACNI VYPOCET -----------------------------------------------------*/
    
     /* vypocet regulacni odchylky */
    fErrorVal = fSetPointVal - fInputVal;  
    
    /* novy akcni zasah */
    fOutputVal = f_K_Val * fErrorVal + fSumVal + ui_N_Val * ( f_K_Val * fErrorVal - fDifVal + fDifVal * expf(- ui_N_Val * f_TS_Val / f_TD_Val) );
    
    /* aktualizace stavu regulatoru */
    fDifVal = f_K_Val * fErrorVal + fDifVal * expf(- ui_N_Val * f_TS_Val / f_TD_Val);
    fSumVal += fErrorVal * f_K_Val * f_TS_Val / f_TI_Val;
    
    /* kontrola limitace vystupu */
    if( fOutputVal > PSD_OUT_MAX )
    {
      fSumVal += ( PSD_OUT_MAX - fOutputVal ) * f_K_Val * f_TS_Val / f_TT_Val;
      fOutputVal = PSD_OUT_MAX;
    }
    else if ( fOutputVal < PSD_OUT_MIN )
    {
      fSumVal += ( PSD_OUT_MIN - fOutputVal ) * f_K_Val * f_TS_Val / f_TT_Val;
      fOutputVal = PSD_OUT_MIN;
    }   
    
    /* VYSTUP REGULATORU -----------------------------------------------------*/
    
    /* prepocet akcni hodnoty podle vztahu (8-4) */
    uiOutputData = ( (uint16_t)(fOutputVal / fOutputLSB) & 0x0fff );
    
    /* poslani prepoctenych dat k prevodu na analogovou hodnotu */
    DAC_SetChannel1Data( DAC_Align_12b_R, (uint16_t)uiOutputData );
    DAC_SoftwareTriggerCmd( DAC_Channel_1, ENABLE );    
    
    
    /* pozastaveni vykonavani ulohy az do prichodu dalsi vzorkovaci periody */
    vTaskDelayUntil( &xActTime, xFrequency );
  }
}
/*----------------------------------------------------------------------------*/