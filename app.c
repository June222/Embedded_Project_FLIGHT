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

static  void  AppTaskCreate(void)
{
    OS_ERR err;

    u8_t idx = 0;
    task_t* pTask_Cfg;
    for (idx = 0; idx < TASK_N; idx++)
    {
        pTask_Cfg = &cyclic_tasks[idx];

        OSTaskCreate(
            pTask_Cfg->pTcb,
            pTask_Cfg->name,
            pTask_Cfg->func,
            (void*)0u,
            pTask_Cfg->prio,
            pTask_Cfg->pStack,
            pTask_Cfg->pStack[APP_CFG_TASK_START_STK_SIZE / 10u],
            APP_CFG_TASK_START_STK_SIZE,
            (OS_MSG_QTY)0u,
            (OS_TICK)0u,
            (void*)0u,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR*)&err
        );
    }
}



/*
*********************************************************************************************************
*                                          AppTask_USART
*
* Description : USART Control function
*
* Arguments   : p_arg (unused)
*
* Returns     : none

*********************************************************************************************************
*/

static void AppTask_USART(void* p_arg)
{
    OS_ERR  err;
    uint16_t c;

    char command[20] = "";
    task_cmd task_command;

    led_c led_cmd;

    //unsigned short b_time = 1u;
    int i = 0;

    // flag variables to indicate the status of each type
    int led_flag = 0;
    int motor_flag = 0;
    int critical_flag = 0;
    int flight_dir_flag = 0;
    int reset_flag = 0;
    int blink_flag = 0;

    while (DEF_TRUE) {
        i = 0;
        motor_flag = 0;
        critical_flag = 0;
        flight_dir_flag = 0;
        led_flag = 0;
        reset_flag = 0;
        blink_flag = 0;
        c = 0;

        CPU_SR_ALLOC();

        // Part that receives command input through putty
        while (DEF_TRUE) {
            while (USART_GetFlagStatus(Nucleo_COM1, USART_FLAG_RXNE) == RESET) {
                OSTimeDlyHMSM(0u, 0u, 0u, 10u, OS_OPT_TIME_HMSM_STRICT, &err);
            }
            c = USART_ReceiveData(Nucleo_COM1);

            if (c == '\r') {
                break;
            }
            else if ((c == 127) && (i > 0)) {
                USART_SendData(Nucleo_COM1, 127);
                --i;
                command[i] = '\0';

            }
            else {
                USART_SendData(Nucleo_COM1, c);
                command[i++] = c;
            }
        }

        command[i] = '\0';

        // The part that goes to the next line in the serial terminal
        if (i) {
            send_string("\n\r");
        }

        USART_ClearITPendingBit(USART3, USART_IT_RXNE);

        // Check the LED number in the received command.
        if (strstr(command, "rolling") != NULL) {
            task_command.cmd = MOTOR;
            task_command.m_name = ROLLING;
            motor_flag = 1;
            critical_flag = 1;
        }
        else if (strstr(command, "yawing") != NULL) {
            task_command.cmd = MOTOR;
            task_command.m_name = YAWING;
            motor_flag = 1;
            critical_flag = 1;
        }
        else if (strstr(command, "pitching") != NULL) {
            task_command.cmd = MOTOR;
            task_command.m_name = PITCHING;
            motor_flag = 1;
            critical_flag = 1;
        }
        else {}

        if (strstr(command, "ledoff") != NULL) {
            task_command.cmd = LED;
            task_command.l_cmd = OFF;
            led_flag = 1;
            reset_flag = 1;
            critical_flag = 1;
        }
        else if (strstr(command, "ledblink") != NULL) {
            task_command.cmd = LED;
            task_command.l_cmd = BLINK;
            led_flag = 1;
            blink_flag = 1;
            critical_flag = 1;
        }
        else {}

        // Check the LED command to be executed among the received commands.
        // In the case of blink and reset, separate flags are created to perform the operation.
        if (strstr(command, "up") != NULL) {
            task_command.f_dir = UP;
            flight_dir_flag = 1;
            critical_flag = 1;
        }
        else if (strstr(command, "down") != NULL) {
            task_command.f_dir = DOWN;
            flight_dir_flag = 1;
            critical_flag = 1;
        }
        else if (strstr(command, "left") != NULL) {
            task_command.f_dir = LEFT;
            flight_dir_flag = 1;
            critical_flag = 1;
        }
        else if (strstr(command, "right") != NULL) {
            task_command.f_dir = RIGHT;
            flight_dir_flag = 1;
            critical_flag = 1;
        }
        else {}

        if (motor_flag && critical_flag) {
            // Enter critical section
            OS_CRITICAL_ENTER();

            // set direction of flight
            if (flight_dir_flag) {
                motor_task_arr[task_command.m_name].cmd = task_command.cmd;
                motor_task_arr[task_command.m_name].f_dir = task_command.f_dir;
            }

            // critical section escape
            OS_CRITICAL_EXIT();
        }

        if (led_flag && critical_flag) {
            // Enter critical section
            OS_CRITICAL_ENTER();

            if (blink_flag) {
                led_task_arr.cmd = LED;
                led_task_arr.l_cmd = BLINK;
            }
            else if (reset_flag) {
                led_task_arr.cmd = LED;
                led_task_arr.l_cmd = OFF;
            }
            else {}

            // critical section escape
            OS_CRITICAL_EXIT();
        }

        // command reset
        memset(command, '\0', 20 * sizeof(char));

        // Time delay
        OSTimeDlyHMSM(0u, 0u, 0u, 50u, OS_OPT_TIME_HMSM_STRICT, &err);


    }
}

/*
*********************************************************************************************************
*                                          AppTask_LED
*
* Description : LED Control function
*
* Arguments   : p_arg (unused)
*
* Returns     : none

*********************************************************************************************************
*/
static void AppTask_LED(void* p_arg)
{
    OS_ERR  err;

    // LED operation appears differently depending on the command state of the LED.
    // In the case of Blink, blink is performed by the input time unit.
    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
        switch (led_arr[LED1].c) {
        case ON:
            BSP_LED_On(1);
            OSTimeDlyHMSM(0u, 0u, 0u, 50u, OS_OPT_TIME_HMSM_STRICT, &err);
            break;
        case OFF:
            BSP_LED_Off(1);
            OSTimeDlyHMSM(0u, 0u, 0u, 50u, OS_OPT_TIME_HMSM_STRICT, &err);
            break;
        case BLINK:
            BSP_LED_Toggle(1);
            OSTimeDlyHMSM(0u, 0u, led_arr[LED1].t, 0u, OS_OPT_TIME_HMSM_STRICT, &err);
            break;
        }

    }
}


/*
*********************************************************************************************************
*                                          AppTask_Rolling
*
* Description : Rolling Motor Control function
*
* Arguments   : p_arg (unused)
*
* Returns     : none

*********************************************************************************************************
*/
static void AppTask_Rolling(void* p_arg)
{
    OS_ERR  err;

    // LED operation appears differently depending on the command state of the LED.
    // In the case of Blink, blink is performed by the input time unit.
    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
        switch (led_arr[LED1].c) {
        case ON:
            BSP_LED_On(1);
            OSTimeDlyHMSM(0u, 0u, 0u, 50u, OS_OPT_TIME_HMSM_STRICT, &err);
            break;
        case OFF:
            BSP_LED_Off(1);
            OSTimeDlyHMSM(0u, 0u, 0u, 50u, OS_OPT_TIME_HMSM_STRICT, &err);
            break;
        case BLINK:
            BSP_LED_Toggle(1);
            OSTimeDlyHMSM(0u, 0u, led_arr[LED1].t, 0u, OS_OPT_TIME_HMSM_STRICT, &err);
            break;
        }
    }

}
