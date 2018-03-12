#ifndef MAIN_TASKS_H_
#define MAIN_TASKS_H_

/* hlavicky systemu FreeRTOS -------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/* definice delky fronty (v prvcich, ne v bytech) */
#define ADC_QUEUE_LENGTH 5
#define PAR_QUEUE_LENGTH 5

/* KONSTANTY PRO PSD REGULATOR -----------------------------------------------*/
/* definice konstant pro regulator */
#define PSD_IN_MIN  0           /* minimalni hodnota vstupniho signalu */
#define PSD_IN_MAX  3           /* maximalni hodnota vstupniho signalu */
#define PSD_IN_RES  12          /* rozliseni vstupniho A/D prevodniku v bitech*/

#define PSD_OUT_MIN 0           /* minimalni hodnota vystupniho signalu */
#define PSD_OUT_MAX 3           /* maximalni hodnota vystupniho signalu */
#define PSD_OUT_RES 12          /* rozliseni vstupniho D/A prevodniku v bitech*/

/* vypocet rozsahu signalu */
#define PSD_IN_RNG  (PSD_IN_MAX - PSD_IN_MIN)
#define PSD_OUT_RNG  (PSD_OUT_MAX - PSD_OUT_MIN)

/* parametry regulatoru (casove konstanty jsou v [s], vzorkovaci perioda v [ms]) */
#define T_S    100    /* vzorkovaci perioda [ms] */
#define T_D    0.1f   /* derivacni cas. konstanta [s] */
#define T_I    1.0f   /* integracni cas. konstanta [s] */
#define T_T    0.1f   /* cas. konstanta pro anti wind-up [s] */
#define K      0.5f   /* zesileni */
#define N      (uint8_t)3      /* parametr filtru derivacni slozky */

/*----------------------------------------------------------------------------*/

/* globalni promenne definovane v main_tasks.c */
extern xQueueHandle xADCQueue;
extern xQueueHandle xParQueue;

/* prototypy uloh */
void vPSDTask( void *pvParameters );

/* definice datoveho typu struktury pro frontu pro predavani parametru */
typedef struct
{
  float fSetPointMsg;
  float f_TD_Msg;
  float f_TI_Msg;
  float f_TT_Msg;
  float f_K_Msg;
  uint8_t ui_N_Msg;
  
} xParQueueMessage_t;

#endif /* MAIN_TASKS_H_ */