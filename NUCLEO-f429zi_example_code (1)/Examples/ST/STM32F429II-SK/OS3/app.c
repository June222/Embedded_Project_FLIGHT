/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2013; Micrium, Inc.; Weston, FL
*
*                   All rights reserved.  Protected by international copyright laws.
*                   Knowledge of the source code may not be used to write a similar
*                   product.  This file may only be used in accordance with a license
*                   and should not be redistributed in any way.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                       IAR Development Kits
*                                              on the
*
*                                    STM32F429II-SK KICKSTART KIT
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : YS
*                 DC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <includes.h>
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define APP_TASK_EQ_0_ITERATION_NBR              16u
#define PULSE_PERIOD 							1000u
#define PULSE_PRESCALE							1680u
#define MSG_SIZE								30u
/*
*********************************************************************************************************
*                                            TYPES DEFINITIONS
*********************************************************************************************************
*/
typedef enum {
   TASK_USART,
   TASK_ACTION,
   TASK_PRINT,

   TASK_N
}task_e;

typedef enum{
	ROLLING,
	YAWING,
	PITCHING,
	LED,

	ACTION_N
}action_t;

typedef enum{
	RIGHT,
	LEFT,
	UP,
	DOWN,
	NEUTRAL,

	DIR_N
}dir_t;

typedef enum{
	ON,
	OFF
}led_status;

typedef struct
{
   CPU_CHAR* name;
   OS_TASK_PTR func;
   OS_PRIO prio;
   CPU_STK* pStack;
   OS_TCB* pTcb;
}task_t;

typedef struct{
	action_t action;
	dir_t dir;
	led_status led_stat;
}action_infomation;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static  void  AppTaskStart          (void     *p_arg);
static  void  AppTaskCreate         (void);
static  void  AppObjCreate          (void);

/* ----------------- Task -------------- */
static void AppTask_USART(void *p_arg);
static void AppTask_ACTION(void *p_arg);
static void AppTask_PRINT(void *p_arg);
/* ----------------- Task -------------- */


/* ----------------- Setup -------------- */
static void Setup_Gpio(void);
static void Setup_TIM(void);
static void RCC_TIM_Config(void);
static void GPIO_TIM_Config(void);
static void TIM_Config(void);
static void USART_Config(void);
/* ----------------- Setup -------------- */


/* ----------------- Action -------------- */
static void ChangeDutyCycle(uint32_t pulse);

static void Motor1(uint32_t pulse);
static void Motor2(uint32_t pulse);
static void Motor3(uint32_t pulse);
static void Motor4(uint32_t pulse);
/* ----------------- Action -------------- */

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
/* ----------------- APPLICATION GLOBALS -------------- */
static  OS_TCB   AppTaskStartTCB;
static  CPU_STK  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB       Task_USART_TCB;
static  OS_TCB       Task_ACTION_TCB;
static  OS_TCB       Task_PRINT_TCB;

static  CPU_STK  Task_USART_Stack[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK  Task_ACTION_Stack[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK  Task_PRINT_Stack[APP_CFG_TASK_START_STK_SIZE];

int count=0;

task_t cyclic_tasks[TASK_N] = {
   {"Task_USART" , 	AppTask_USART,  3, &Task_USART_Stack[0] , 	&Task_USART_TCB},
   {"Task_PRINT", 	AppTask_PRINT, 	4, &Task_PRINT_Stack[0], 	&Task_PRINT_TCB},
   {"Task_ACTION", 	AppTask_ACTION, 5, &Task_ACTION_Stack[0], 	&Task_ACTION_TCB},
};

OS_Q PRINT_Q;
OS_Q ACTION_Q;

CPU_TS ts;

action_infomation action_info;

char* MISSION_MSG = "Orders Received!\n\r";
char* MISSION_FAIL_MSG = "I can't build that!\n\r";

uint16_t order_flag = 1;
uint16_t direction_flag = 1;

/* ------------ FLOATING POINT TEST TASK -------------- */
/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR  err;

    /* Basic Init */
    RCC_DeInit();
//    SystemCoreClockUpdate();
    Setup_Gpio();
    USART_Config();
    Setup_TIM();

    /* BSP Init */
    BSP_IntDisAll();                                            /* Disable all interrupts.                              */

    CPU_Init();                                                 /* Initialize the uC/CPU Services                       */
    Mem_Init();                                                 /* Initialize Memory Management Module                  */
    Math_Init();                                                /* Initialize Mathematical Module                       */

    send_string("Welcome Captain!\n\r");

    /* OS Init */
    OSInit(&err);                                               /* Init uC/OS-III.                                      */

    OSQCreate(	(OS_Q*)&PRINT_Q,
    			(CPU_CHAR *)"PRINT QUEUE",
    			(OS_MSG_QTY)MSG_SIZE,
    			(OS_ERR *)&err				);



    OSTaskCreate((OS_TCB       *)&AppTaskStartTCB,              /* Create the start task                                */
                 (CPU_CHAR     *)"App Task Start",
                 (OS_TASK_PTR   )AppTaskStart,
                 (void         *)0u,
                 (OS_PRIO       )APP_CFG_TASK_START_PRIO,
                 (CPU_STK      *)&AppTaskStartStk[0u],
                 (CPU_STK_SIZE  )AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE  )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    )0u,
                 (OS_TICK       )0u,
                 (void         *)0u,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR       *)&err);

   OSStart(&err);   /* Start multitasking (i.e. give control to uC/OS-III). */

   (void)&err;

   return (0u);
}
/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/
static  void  AppTaskStart (void *p_arg)
{
    OS_ERR  err;

   (void)p_arg;

    BSP_Init();                                                 /* Initialize BSP functions                             */
    BSP_Tick_Init();                                            /* Initialize Tick Services.                            */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

   // BSP_LED_Off(0u);                                            /* Turn Off LEDs after initialization                   */

   APP_TRACE_DBG(("Creating Application Kernel Objects\n\r"));
   AppObjCreate();                                             /* Create Applicaiton kernel objects                    */

   APP_TRACE_DBG(("Creating Application Tasks\n\r"));
   AppTaskCreate();                                            /* Create Application tasks                             */
}

/*
*********************************************************************************************************
*                                          AppTask_500ms
*
* Description : Example of 500mS Task
*
* Arguments   : p_arg (unused)
*
* Returns     : none
*
* Note: Long period used to measure timing in person
*********************************************************************************************************
*/
static void AppTask_USART(void *p_arg)
{
    OS_ERR  err;
    uint16_t c;
    char command[MSG_SIZE] = "";
    int cmd_idx;
    USART_Config();

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
		cmd_idx = 0;
		order_flag = 1;
		direction_flag = 1;

        CPU_SR_ALLOC();

		while (DEF_TRUE) {
			while(USART_GetFlagStatus(Nucleo_COM1, USART_FLAG_RXNE) == RESET){
			}
			c = USART_ReceiveData(Nucleo_COM1);
			if(c == '\r'){
				break;
			}
			else if((c == 127) && (cmd_idx>0)){
				USART_SendData(Nucleo_COM1, 127);
				--cmd_idx;
				command[cmd_idx] = '\0';
			}
			else{
				USART_SendData(Nucleo_COM1, c);
				command[cmd_idx++] = c;
			}
		}
		command[cmd_idx] = '\0';
		if(cmd_idx){
			send_string("\n\r");
		}
		USART_ClearITPendingBit(USART3,USART_IT_RXNE);

		OS_CRITICAL_ENTER();

		if(strstr(command, "rolling") != NULL){
			action_info.action = ROLLING;
		}
		else if(strstr(command, "yawing") != NULL){
			action_info.action = YAWING;
		}
		else if(strstr(command, "pitching") != NULL){
			action_info.action = PITCHING;
		}
		else if(strstr(command, "on")!=NULL) {
			action_info.action = LED;
			action_info.led_stat = ON;
		}
		else if(strstr(command, "off")!=NULL){
			action_info.action = LED;
			action_info.led_stat = OFF;
		}
		else{order_flag = 0;}


		if(strstr(command, "right") != NULL){
			action_info.dir = RIGHT;
		}
		else if(strstr(command, "left") != NULL){
			action_info.dir = LEFT;
		}
		else if(strstr(command, "up")!=NULL){
			action_info.dir = UP;
		}
		else if(strstr(command, "down")!=NULL){
			action_info.dir = DOWN;
		}
		else if(strstr(command, "neutral")!=NULL){
			action_info.dir = NEUTRAL;
		}
		else {direction_flag = 0;}

		OS_CRITICAL_EXIT();

		if(order_flag){
			OSQPost(	&PRINT_Q,
						(char*) MISSION_MSG,
						MSG_SIZE,
						OS_OPT_POST_FIFO,
						&err					);
		}

		else{
			OSQPost(	&PRINT_Q,
						(char*) MISSION_FAIL_MSG,
						MSG_SIZE,
						OS_OPT_POST_FIFO,
						&err					);
		}

		OSTaskSemPost(	&Task_ACTION_TCB,
						OS_OPT_POST_NONE,
						&err			 	);

		OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
		memset(command, '\0',20*sizeof(char));
    }
}

/*
*********************************************************************************************************
*                                          AppTask_500ms
*
* Description : Example of 500mS Task
*
* Arguments   : p_arg (unused)
*
* Returns     : none
*
* Note: Long period used to measure timing in person
*********************************************************************************************************
*/
static void AppTask_PRINT(void *p_arg)
{
    OS_ERR  err;
    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
		char* msg = OSQPend(	&PRINT_Q,
								0,
								OS_OPT_PEND_BLOCKING,
								sizeof(char*)*MSG_SIZE,
								&ts,
								&err					);
		send_string(msg);
    }
}

/*
*********************************************************************************************************
*                                          AppTask_500ms
*
* Description : Example of 500mS Task
*
* Arguments   : p_arg (unused)
*
* Returns     : none
*
* Note: Long period used to measure timing in person
*********************************************************************************************************
*/
static void AppTask_ACTION(void *p_arg)
{
    OS_ERR  err;
    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */

    	OSTaskSemPend(	0,
    					OS_OPT_PEND_BLOCKING,
						&ts,
						&err					);
    	if(order_flag && direction_flag) {
    		if(action_info.action == ROLLING){
				if(action_info.dir == RIGHT){
					Motor1(PULSE_PERIOD * 5 / 100);
				}
				else if(action_info.dir == LEFT){
					Motor1(PULSE_PERIOD * 9 / 100);
				}
				else if(action_info.dir == NEUTRAL){
					Motor1(PULSE_PERIOD * 7 / 100);
				}
				else{}
			}
			else if(action_info.action == YAWING){
				if(action_info.dir == LEFT){
					Motor2(PULSE_PERIOD * 9 / 100);
				}
				else if(action_info.dir == RIGHT){
					Motor2(PULSE_PERIOD * 5 / 100);
				}
				else if(action_info.dir == NEUTRAL){
					Motor2(PULSE_PERIOD * 7 / 100);
				}
				else{}
			}
			else if(action_info.action == PITCHING){
				if(action_info.dir == UP){
					Motor3(PULSE_PERIOD * 9 / 100);
					Motor4(PULSE_PERIOD * 5 / 100);
				}
				else if(action_info.dir == DOWN){
					Motor3(PULSE_PERIOD * 5 / 100);
					Motor4(PULSE_PERIOD * 9 / 100);
				}
				else if(action_info.dir == NEUTRAL){
					Motor3(PULSE_PERIOD * 7 / 100);
					Motor4(PULSE_PERIOD * 7 / 100);
				}
				else{}
			}
    	}
    	else if (order_flag && action_info.action == LED){
    		if(action_info.led_stat == ON){
    			GPIO_SetBits(GPIOE, GPIO_Pin_2);
    		}
    		else if(action_info.led_stat == OFF){
    			GPIO_ResetBits(GPIOE, GPIO_Pin_2);
    		}
    		else{}
    	}
    	else{

    	}


    	OSTimeDlyHMSM(	0u,0u,1u,0u,
						OS_OPT_TIME_HMSM_STRICT,
						&err						);

    	OSTaskSemPost(	&Task_USART_TCB,
    					OS_OPT_POST_NONE,
						&err						);

    }
}

/*
*********************************************************************************************************
*                                          AppTaskCreate()
*
* Description : Create application tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppTaskCreate (void)
{
   OS_ERR  err;

   u8_t idx = 0;
   task_t* pTask_Cfg;
   for(idx = 0; idx < TASK_N; idx++)
   {
      pTask_Cfg = &cyclic_tasks[idx];

      OSTaskCreate(
            pTask_Cfg->pTcb,
            pTask_Cfg->name,
            pTask_Cfg->func,
            (void         *)0u,
            pTask_Cfg->prio,
            pTask_Cfg->pStack,
            pTask_Cfg->pStack[APP_CFG_TASK_START_STK_SIZE / 10u],
            APP_CFG_TASK_START_STK_SIZE,
            (OS_MSG_QTY    )0u,
            (OS_TICK       )0u,
            (void         *)0u,
            (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR       *)&err
      );
   }
}


/*
*********************************************************************************************************
*                                          AppObjCreate()
*
* Description : Create application kernel objects tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppObjCreate (void)
{

}

/*
*********************************************************************************************************
*                                          Setup_Gpio()
*
* Description : Configure LED GPIOs directly
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     :
*              LED1 PB0
*              LED2 PB7
*              LED3 PB14
*
*********************************************************************************************************
*/
static void Setup_Gpio(void)
{
   GPIO_InitTypeDef led_init = {0};

   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
   RCC_AHB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

   led_init.GPIO_Mode   = GPIO_Mode_OUT;
   led_init.GPIO_OType  = GPIO_OType_PP;
   led_init.GPIO_Speed  = GPIO_Speed_2MHz;
   led_init.GPIO_PuPd   = GPIO_PuPd_NOPULL;
   led_init.GPIO_Pin    = GPIO_Pin_2;

   GPIO_Init(GPIOE, &led_init);
}

static void Setup_TIM(void){
	RCC_TIM_Config();
	GPIO_TIM_Config();
	TIM_Config();
}

static void RCC_TIM_Config(void){
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
}

static void GPIO_TIM_Config(void){
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;


	GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_TIM3);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_TIM2);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_TIM4);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);


	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_TIM2);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}


static void TIM_Config(void){
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = PULSE_PERIOD;
	TIM_TimeBaseStructure.TIM_Prescaler = PULSE_PRESCALE;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);


	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM3, ENABLE);
	TIM_Cmd(TIM3, ENABLE);


	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM2, ENABLE);
	TIM_Cmd(TIM2, ENABLE);

	
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM4, ENABLE);
	TIM_Cmd(TIM4, ENABLE);


	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM2, ENABLE);
	TIM_Cmd(TIM2, ENABLE);
}

static void Motor1(uint32_t pulse){
	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = pulse;

	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
}

static void Motor2(uint32_t pulse){
	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = pulse;

	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
}

static void Motor3(uint32_t pulse){
	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = pulse;

	TIM_OC4Init(TIM4, &TIM_OCInitStructure);
}

static void Motor4(uint32_t pulse){
	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = pulse;

	TIM_OC3Init(TIM2, &TIM_OCInitStructure);
}

static void ChangeDutyCycle(uint32_t pulse){
	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = pulse;

	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
}


static void USART_Config(void)
{
  USART_InitTypeDef USART_InitStructure;

  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  STM_Nucleo_COMInit(COM1, &USART_InitStructure);
}
