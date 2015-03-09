#ifndef __SCHEDULER_HPP__
#define __SCHEDULER_HPP__
#include "TCB_List.hpp"
#include "sleepList.hpp"

//#include 
template <int NUM_PRIORITIES, int MAX_NUM_THREADS>
class Scheduler{
private:
  Tcb_List PriorityList[NUM_PRIORITIES+1];
  List<Tcb_t*, MAXNUMTHREADS> SleepingList;
  idleThread;
public:

/**
 * @brief Get the next scheduled thread
 * @details [long description]
 * @return [description]
 */
Tcb_t* GetNext(void){
  int i;
  Tcb_t* thread = NULL;
  for(i = 0; i <= NUM_PRIORITIES; i++){
    if(!(PriorityList[i].isEmpty())){
      thread = PriorityList[i].Head();
      PriorityList.MoveHead();
      return thread;
    }
  }
  /*Should not get here*/
}

/**
 * @brief Remove a thread from the priority list and sleep it
 * @details Does no safety checks
 */
void Sleep(Tcb_t* thread){
  SleepingList.push_back(Remove(thread));
}
/**
 * @brief Remove from Priority list
 * @details Decouples the scheduling
 * 
 * @param thread [description]
 * @return [description]
 */
Tcb_t* Remove(Tcb_t* thread){
  return PriorityList[thread->priority].remove_node(thread);
} 

void push_back(Tcb_t* thread){
  PriorityList[thread->priority].push_back(thread);
}

void UpdateSleeping(void){
    List<Tcb_t*, MAXNUMTHREADS>::iterator iter;
  Tcb_t* thread;
  for(iter = SleepingList.begin(); iter != SleepingList.end(); ++iter){
    ((*iter)->state_sleep)--;
    if((*iter)->state_sleep < 1){
      // DEBUGprintf("Found Ready Thread\n");
      thread = *iter;

      thread->next = thread;    /// configure thread node
      thread->prev = thread;    /// configure thread node
      thread->state_sleep = 0;  /// configure thread node
      
      iter.mark4Delete();       /// Mark Thread for removal from sleepinglist
      push_back(thread);
    }
  }
  SleepingList.clean();      /// Update the sleeping lists, remove entries marked 
                              /// for deletion
  }
};
#endif /*__SCHEDULER_HPP__*/