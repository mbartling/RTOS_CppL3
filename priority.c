#include "priority.h"
//int Timer0APriority = 0xA0000000; //setting the priority to 5 // (bits 29 -32)
int Timer1APriority =  (0x00008000);  //setting the priority to 2 (bits 13-15) 
int Timer2APriority = 0xA0000000; //setting the priority to 5 // (bits 29 -32)
int UART0Priority = 0x00004000; //setting the UART priotiy to 2 (bits 13 - 15)
int PendSVPriority = 0x00E00000; //setting the PendSV priotiy to 7 (bits 21 - 23)
int SysTickPriority =  0xC0000000; //setting the SysTick priority to 6 (bits 29-31
