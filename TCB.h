#ifndef __TCB_H__
#define __TCB_H__

#include <stdint.h>

#define MAXNUMTHREADS 10  /** Maximum number of threads*/
#define STACKSIZE 100 /** number of 32bit words in stack */

#define NUM_PRIORITIES 7
/*Note: use 16 words per context switch*/

typedef struct _Tcb {
  int32_t* sp;        //!< Stack Pointer: OFFSET 0
                      //!< Valid for threads not running        
  struct _Tcb* next;  //!< Next TCB Element: OFFSET 4
  struct _Tcb* prev;  //!< Previous TCB element: OFFSET 8
  int32_t id;         //!< Thread ID: OFFSET 16
  int32_t state_sleep;//!< used to suspend execution: OFFSET 20
  uint32_t priority;   //!< Thread priority: OFFSET 24
  int32_t state_blocked; //!<Used in lab 3: OFFSET 28
  int32_t stack[STACKSIZE]; //!<Thread stack: OFFSET 32
} Tcb_t;


void TCB_SetInitialStack(Tcb_t* pTcb);
int TCB_Available(void);
Tcb_t* TCB_GetNewThread(void);
void TCB_InsertNodeBeforeRoot(Tcb_t* node);
Tcb_t* TCB_GetRunningThread(void);
void TCB_RemoveRunningThread(void);
void TCB_UpdateSleeping(void);
void TCB_RemoveRunningAndSleep(void);
void TCB_Configure_IdleThread(void);
void TCB_PushBackRunning(void);
void TCB_Scheduler(void);
// void TCB_PromotePriority(void);
void TCB_PushBackThread(Tcb_t* thread);
//void dummy(void); // Tests if function pointer set properly
#endif /*__TCB_H__*/
