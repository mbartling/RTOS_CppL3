#include "TCB.h"
#include "Pool.hpp"
#include <stdio.h>
#include "sleepList.hpp"
#include "Exception.hpp"
#include "Perf.h"
// #include <iostream>
#define DEBUGprintf(...) /**/

#define NUM_PRIORITIES 7
#define IDLE_THREAD_PRIORITY ((NUM_PRIORITIES)+ 1)

List<Tcb_t*, MAXNUMTHREADS> SleepingList;
Pool<Tcb_t, MAXNUMTHREADS> ThreadPool;
Tcb_t* RunningThread = NULL;
Tcb_t* SleepingThread = NULL; // Sleeping thread root
TcbListC_t ThreadList;

int totalThreadCount;
#define AGE_PRIORITY_0 0
#define AGE_PRIORITY_1 1
#define AGE_PRIORITY_2 2
#define AGE_PRIORITY_3 3
#define AGE_PRIORITY_4 4
#define AGE_PRIORITY_5 5
#define AGE_PRIORITY_6 6
#define AGE_PRIORITY_7 7

const int AGE_FROM[] = {
  AGE_PRIORITY_0,
  AGE_PRIORITY_1,
  AGE_PRIORITY_2,
  AGE_PRIORITY_3,
  AGE_PRIORITY_4,
  AGE_PRIORITY_5,
  AGE_PRIORITY_6,
  AGE_PRIORITY_7
};
int Current_AGE[] = {
  AGE_PRIORITY_0,
  AGE_PRIORITY_1,
  AGE_PRIORITY_2,
  AGE_PRIORITY_3,
  AGE_PRIORITY_4,
  AGE_PRIORITY_5,
  AGE_PRIORITY_6,
  AGE_PRIORITY_7
};

List<Tcb_t*, MAXNUMTHREADS> PriorityList[IDLE_THREAD_PRIORITY];


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

Tcb_t* idleThread = NULL;
uint32_t idleTime = 0;

void Idle(void){
  while(1){
    //Idle
    idleTime++;
  }
}

/**
 * @brief Modulate priorities based on age
 * @details If sufficiently old then temporarily promotennnkk priority
 */
void TCB_PromotePriority(void){
  for(int i = 1; i < NUM_PRIORITIES; ++i){
    if(!PriorityList[i].isEmpty()){
      if(--Current_AGE[i] <= 0){
        PriorityList[i-1].push_back(PriorityList[i].pop_front());
        Current_AGE[i] = AGE_FROM[i];
      }
    }
  }
}
void (*idleTask)(void);
/**
 * @brief Configure the idle thread
 * @details The idle thread exists when the scheduler is empty.
 * This way the code does not hardfault on an invalid context switch. In
 * other words, there are no threads to switch to in the list and we do 
 * not want to context switch to null.
 */
 void TCB_Configure_IdleThread(void){
  idleTask = Idle;
  idleThread = TCB_GetNewThread();
  TCB_SetInitialStack(idleThread);
  idleThread->stack[STACKSIZE-2] = (int32_t) (Idle); //return to IDLE
  //ThreadList.head = idleThread;
  idleThread->priority = IDLE_THREAD_PRIORITY - 1;
	idleThread->next = idleThread;
  RunningThread = idleThread;
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

void TCB_PushBackRunning(void){
    //push the running thread to end of its list if necessary
//  if(RunningThread != idleThread){
    //TODO push to sleeping list if necessary
    PriorityList[RunningThread->priority].push_back(RunningThread); 
 // }
}
void TCB_PushBackThread(Tcb_t* thread){
    //push the running thread to end of its list if necessary
  //if(thread != idleThread){
    //TODO push to sleeping list if necessary
    PriorityList[thread->priority].push_back(thread); 
  //}
}
/**
 * @brief The scheduler just picks the next thread to run
 * @details nothing more, nothing less
 */
void TCB_Scheduler(void){
  // Pick Running Thread Next
//  if(RunningThread->next == RunningThread) {
  add_trace(TRACE_SCHEDULER);
      for(int i = 0;  i < IDLE_THREAD_PRIORITY; ++i){
//          if((!PriorityList[i].isEmpty()) && (ThreadList.count!=0)){
          if((!PriorityList[i].isEmpty())) {
              Tcb_t* thread = PriorityList[i].pop_front();
              thread->next = thread;
              RunningThread->next = thread;
              break;    
          }
      }
//  } 
  //Post Process such as pushing something to sleeping list
  //
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
  long status;
  status = StartCritical();
  // Tcb_t* thread = RunningThread->next->prev;  //<! Cheap trick to help maintain list
  //                                             //<! on Context switch

  // if(ThreadList.count > 0){
  //   node->next = thread;
  //   node->prev = thread->prev;
  //   thread->prev = node;
  //   node->prev->next = node;
  // }else {
  //   ThreadList.head = node;  //<! Replace Idle Thread with node
  //   RunningThread->next = node;
  //   RunningThread->prev = node;
  // }
  PriorityList[node->priority].push_back(node); 
  totalThreadCount++;
  EndCritical(status);

}

void TCB_RemoveThread(Tcb_t* thread){
  EX_method_not_implemented.fail();
}
void TCB_RemoveThreadAndSleep(Tcb_t* thread) {
  EX_method_not_implemented.fail();

}
void TCB_RemoveRunningThread(void) {
  long status;
  status = StartCritical();
  //make sure this is safe (the assumption is that OS_Kill would never be called in a situation where we only have the idleThread inside the priority) 
  //Tcb_t* thread = RunningThread;
//  if(totalcount == 0){
//   EndCritical(status);
//
//   return; // Never remove idle thread
//  }

//  if(ThreadList.count > 1){
//  // (RunningThread->prev)->next = RunningThread->next;  //Update thread list
//  // (RunningThread->next)->prev = RunningThread->prev;  //Update thread list
//
//    ThreadList.count--;
//  }else if(ThreadList.count == 1){
//  
//  idleThread->next = idleThread;
//  idleThread->prev = idleThread;
//  RunningThread->next = idleThread; //Make Sure to never call sleep on idle thread
//  ThreadList.head = idleThread;     //Make Sure to never call sleep on idle thread 
//
//  ThreadList.count--;
//  } 
  totalThreadCount--; 
  ThreadPool.free(RunningThread); // Free the thread in memory
  EndCritical(status);
}

/**
 * @brief [brief description]
 * @details Assumes Idle thread never sleeps
 */
void TCB_RemoveRunningAndSleep(void) {
     long status;
     status = StartCritical();
//     if(ThreadList.count > 1){
//      // (RunningThread->prev)->next = RunningThread->next;
//      // (RunningThread->next)->prev = RunningThread->prev;
//      ThreadList.count--;
//    }else if(ThreadList.count == 1){
//      idleThread->next = idleThread;
//      idleThread->prev = idleThread;
//      RunningThread->next = idleThread;   //Make Sure to never call sleep on idle thread
//      ThreadList.head = idleThread; //Make Sure to never call sleep on idle thread 
//      ThreadList.count--;
//    } 
//     totalThreadCount--;   
     DEBUGprintf("Sleeping\n");

      SleepingList.push_back(RunningThread);
      EndCritical(status);
}

Tcb_t* TCB_GetRunningThread(void){
      return RunningThread;
}
//
//int TCB_threadListEmpty(void){
//      return (ThreadList.count == 0);
//}

void TCB_RemoveSleepingNode(Tcb_t* thread){
      EX_method_not_implemented.fail();

}

void TCB_UpdateSleeping(void) {
  List<Tcb_t*, MAXNUMTHREADS>::iterator iter;
  Tcb_t* thread;
  for(iter = SleepingList.begin(); iter != SleepingList.end(); ++iter){
    ((*iter)->state_sleep)--;
    if((*iter)->state_sleep < 1){
      DEBUGprintf("Found Ready Thread\n");
      thread = *iter;

      thread->next = thread;    /// configure thread node
      thread->prev = thread;    /// configure thread node
      thread->state_sleep = 0;  /// configure thread node
      
      iter.mark4Delete();       /// Mark Thread for removal from sleepinglist
      TCB_InsertNodeBeforeRoot(thread);
    }
  }
  SleepingList.clean();      /// Update the sleeping lists, remove entries marked 
                              /// for deletion
}

void TCB_CheckSleeping(void) {
  EX_method_not_implemented.fail();

}
