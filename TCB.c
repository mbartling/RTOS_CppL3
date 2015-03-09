#include "TCB.h"
#include "Pool.hpp"
#include <stdio.h>
#include "sleepList.hpp"
#include "Exception.hpp"
#include "Perf.h"
// #include <iostream>
#define DEBUGprintf(...) /**/

#define NUMPRIORITIES 7
// #define IDLE_THREAD_PRIORITY ((NUM_PRIORITIES)+ 1)

Pool<Tcb_t, MAXNUMTHREADS> ThreadPool;
Tcb_t* RunningThread = NULL;
Tcb_t* RunningThreadNext = NULL;

int totalThreadCount;


// List<Tcb_t*, MAXNUMTHREADS> PriorityList[IDLE_THREAD_PRIORITY];
Scheduler<NUMPRIORITIES, MAXNUMTHREADS>

#ifdef __cplusplus
extern "C" {
#endif
//comment the following line to deactivate Systick
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
#ifdef __cplusplus
}
#endif


/**
 * @brief Configure the idle thread
 * @details The idle thread exists when the scheduler is empty.
 * This way the code does not hardfault on an invalid context switch. In
 * other words, there are no threads to switch to in the list and we do 
 * not want to context switch to null.
 */
 void TCB_Configure_IdleThread(void){

  // RunningThread = idleThread;
  // RunningThreadNext = idleThread;
  RunningThread = Scheduler.GetNext();
}


void TCB_PushBackRunning(void){
  Scheduler.push_back(RunningThread); 
}
void TCB_PushBackThread(Tcb_t* thread){
  Scheduler.push_back(thread); 
}
/**
 * @brief The scheduler just picks the next thread to run
 * @details nothing more, nothing less
 */
void TCB_Scheduler(void){
  add_trace(TRACE_SCHEDULER);
  RunningThreadNext = Scheduler.GetNext();
  return;
}

int TCB_Available(void){
  return ThreadPool.available();
}

Tcb_t* TCB_GetNewThread(void){
  Tcb_t* thread =  ThreadPool.get();
  thread->next = thread;
  thread->prev = thread;
  thread->id = ThreadPool.getId();
  return thread;
}


void TCB_InsertNodeBeforeRoot(Tcb_t* node)
{
  Scheduler.push_back(node);
}

Tcb_t* TCB_RemoveThread(Tcb_t* thread){
  return Scheduler.Remove(thread);
}

void TCB_RemoveRunningThread(void) {
  // long status;
  // status = StartCritical();
  RunningThread = Scheduler.Remove(RunningThread);
  totalThreadCount--; 
  ThreadPool.free(RunningThread); // Free the thread in memory
  // EndCritical(status);
}

/**
 * @brief [brief description]
 * @details Assumes Idle thread never sleeps
 */
void TCB_RemoveRunningAndSleep(void) {
  Scheduler.Sleep(RunningThread);
}

Tcb_t* TCB_GetRunningThread(void){
      return RunningThread;
}

void TCB_UpdateSleeping(void) {
  Scheduler.UpdateSleeping();
}

void TCB_SetInitialStack(Tcb_t* pTcb)
{
  int32_t* stack = pTcb->stack;
  pTcb->sp = &stack[STACKSIZE - 16];
  stack[STACKSIZE-1] = 0x01000000; // Set Thumb bit
  //Rest of stack registers are currently random
  stack[STACKSIZE - 3]  = 0x14141414;  //<! R14 Dummy Value for Debugging 
  stack[STACKSIZE - 4]  = 0x12121212;  //<! R12 Dummy Value for Debugging
  stack[STACKSIZE - 5]  = 0x03030303;  //<! R03 Dummy Value for Debugging
  stack[STACKSIZE - 6]  = 0x02020202;  //<! R02 Dummy Value for Debugging
  stack[STACKSIZE - 7]  = 0x01010101;  //<! R01 Dummy Value for Debugging
  stack[STACKSIZE - 8]  = 0x00000000;  //<! R00 Dummy Value for Debugging
  stack[STACKSIZE - 9]  = 0x11111111;  //<! R11 Dummy Value for Debugging
  stack[STACKSIZE - 10] = 0x10101010;  //<! R10 Dummy Value for Debugging
  stack[STACKSIZE - 11] = 0x09090909;  //<! R09 Dummy Value for Debugging
  stack[STACKSIZE - 12] = 0x08080808;  //<! R08 Dummy Value for Debugging
  stack[STACKSIZE - 13] = 0x07070707;  //<! R07 Dummy Value for Debugging
  stack[STACKSIZE - 14] = 0x06060606;  //<! R06 Dummy Value for Debugging
  stack[STACKSIZE - 15] = 0x05050505;  //<! R05 Dummy Value for Debugging
  stack[STACKSIZE - 16] = 0x04040404;  //<! R04 Dummy Value for Debugging

}