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

