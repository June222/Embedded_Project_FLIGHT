/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include  <includes.h>
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx.h"
/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/
#define  APP_TASK_EQ_0_ITERATION_NBR 

/*
*********************************************************************************************************
*                                            TYPES DEFINITIONS
*********************************************************************************************************
*/

// Task struct
typedef struct
{
    CPU_CHAR* name;
    OS_TASK_PTR func;
    OS_PRIO prio;
    CPU_STK* pStack;
    OS_TCB* pTcb;
}task_t;

// Task name
typedef enum {
    TASK_ROLLING,
    TASK_YAWING,
    TASK_PITCHING,
    TASK_LED,
    TASK_USART,

    TASK_N
}task_e;

// Command name
typedef enum {
    MOTOR,
    LED,

    CMD_N
}command;

// motor that i work
typedef enum {
    ROLLING,
    YAWING,
    PITCHING,

    MOTOR_N
}motor_name;

// direction of flght
typedef enum {
    UP,
    DOWN,
    LEFT,
    RIGHT,

    DIRECTION_N
}flight_dir;

// structure for LED Command (LED OFF, LED BLINK)
typedef enum {
    OFF,
    BLINK
}led_cmd;

typedef struct {
    command cmd;
    motor_name m_name;
    flight_dir f_dir;
    led_cmd l_cmd;
}task_cmd;

// Clock wise name
typedef enum {
    CLOCK_WISE,
    COUNTER_CLOCK_WISE,
    OFF,

    CLOCK_WISE_N
}clock_wise;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart(void* p_arg);
static  void  AppTaskCreate(void);
static  void  AppObjCreate(void);

// Motor Task
static void AppTask_Rolling(void* p_arg);
static void AppTask_Yawing(void* p_arg);
static void AppTask_Pitching(void* p_arg);

// LED Task
static void AppTask_LED(void* p_arg);

// USART Task
static void AppTask_USART(void* p_arg);

// Setup GPIO Port
static void Setup_Gpio(void);


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

/* ----------------- APPLICATION GLOBALS -------------- */
static  OS_TCB   AppTaskStartTCB;
static  CPU_STK  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

// Motor Task TCB
static  OS_TCB       Task_Rolling_TCB;
static  OS_TCB       Task_Yawing_TCB;
static  OS_TCB       Task_Pitching_TCB;

// LED Task TCB
static  OS_TCB       Task_LED_TCB;

// USART Task TCB
static  OS_TCB       Task_USART_TCB;

// Stack for Motor Task
static  CPU_STK  Task_Rolling_Stack[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK  Task_Yawing_Stack[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK  Task_Pitching_Stack[APP_CFG_TASK_START_STK_SIZE];

// Stack for LED Task
static  CPU_STK  Task_LED_Stack[APP_CFG_TASK_START_STK_SIZE];

// Stack for USART Task
static  CPU_STK  Task_USART[APP_CFG_TASK_START_STK_SIZE];

task_t cyclic_tasks[TASK_N] = {
   {"Task_Rolling" , AppTask_Rolling,	1, &Task_Rolling_Stack[0] ,	&Task_Rolling_TCB},
   {"Task_Yawing",	 AppTask_Yawing,	1, &Task_Yawing_Stack[0],	&Task_Yawing_TCB},
   {"Task_Pitching", AppTask_Pitching,	1, &Task_Pitching_Stack[0], &Task_Pitching_TCB},
   {"Task_LED",		 AppTask_LED,		1, &Task_LED_Stack[0],		&Task_LED_TCB},
   {"Task_USART",	 AppTask_USART,		0, &Task_USART[0],			&Task_USART_TCB}
};

//typedef struct {
//    command cmd;
//    motor_name m_name;
//    flight_dir dir;
//    led_cmd l_cmd;
//}task_cmd;

task_cmd motor_task_arr[MOTOR_N] = {
    {MOTOR, ROLLING,  UP, OFF},
    {MOTOR, YAWING,   UP, OFF},
    {MOTOR, PITCHING, UP, OFF}
};

task_cmd led_task_arr = { LED,MOTOR_N,DIRECTION_N,OFF };


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

    // SystemCoreClockUpdate();
    Setup_Gpio();

    /* BSP Init */
    BSP_IntDisAll();                                            /* Disable all interrupts.                              */

    CPU_Init();                                                 /* Initialize the uC/CPU Services                       */
    Mem_Init();                                                 /* Initialize Memory Management Module                  */
    Math_Init();                                                /* Initialize Mathematical Module                       */

    /* OS Init */
    OSInit(&err);                                               /* Init uC/OS-III.                                      */

    OSTaskCreate((OS_TCB*)&AppTaskStartTCB,              /* Create the start task                                */
        (CPU_CHAR*)"App Task Start",
        (OS_TASK_PTR)AppTaskStart,
        (void*)0u,
        (OS_PRIO)APP_CFG_TASK_START_PRIO,
        (CPU_STK*)&AppTaskStartStk[0u],
        (CPU_STK_SIZE)AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE / 10u],
        (CPU_STK_SIZE)APP_CFG_TASK_START_STK_SIZE,
        (OS_MSG_QTY)0u,
        (OS_TICK)0u,
        (void*)0u,
        (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
        (OS_ERR*)&err
    );

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
static  void  AppTaskStart(void* p_arg)
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

    APP_TRACE_DBG(("Creating Application Kernel Objects\n\r"));
    AppObjCreate();                                             /* Create Applicaiton kernel objects                    */

    APP_TRACE_DBG(("Creating Application Tasks\n\r"));
    AppTaskCreate();                                            /* Create Application tasks                             */
}

