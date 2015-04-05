#include <stdio.h>
#include <string.h>
#include "inc/hw_types.h"
//#include "serial.h"
#include "adc.h"
#include "os.h"

void Thread1(void){
  int i = 0;
  while(1){
    printf("Hello World! %d\n\r", i++);
    i++;
  }
  OS_Kill();
}
void Thread2(void){
  int i = 0;
  while(1){
    printf("Fuck the World! %d\n\r", i++);
    i++;
  }
  OS_Kill();
}

int main(void){
  OS_Init();           // initialize, disable interrupts

  OS_AddThread(&Thread1,128,0);
  OS_AddThread(&Thread2,128,0); 
	
  OS_Launch(TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}