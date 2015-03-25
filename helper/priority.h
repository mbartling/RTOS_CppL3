#ifndef __Priority_H
#define __Priority_H 1


#include "priority.h"
extern int Timer0APriority ; //setting the priority to 1 // (bits 29 -32)
extern int Timer1APriority ;  //setting the priority to 5 (bits 13-15) 
extern int Timer2APriority ; //setting the priority to 1 // (bits 29 -32)
extern int UART0Priority ; //setting the UART priotiy to 2 (bits 13 - 15)
extern int PendSVPriority; //setting the PendSV priotiy to 7 (bits 21 - 23)
extern int SysTickPriority; //setting the PendSV priotiy to 7 (bits 21 - 23)
#endif

