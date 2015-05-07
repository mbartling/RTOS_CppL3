//*****************************************************************************
//
// Lab6M.c - user programs, File system, stream data onto disk
// Jonathan Valvano, March 16, 2011, EE345M
//     You may implement Lab 5 without the oLED display
//*****************************************************************************
// PF1/IDX1 is user input select switch
// PE1/PWM5 is user input down switch 
#include <stdio.h>
#include <string.h>
#include "inc/hw_types.h"
#include "ADC.h"
#include "os.h"
#include "inc/tm4c123gh6pm.h"
#include "Perf.h"
#include "mFS.h"

void mThread(void){
	//mFS_initialize();
	myFS_init();
	format_FS();
	create_file("byteMe.txt");
	Fid_t fp = open("byteMe.txt");
	char c = 'A';
	for(int i = 0; i < 8196; i++){
		if(c == 'z') c = 'A';
		write(&fp, c);
		c++;
	}
	close(&fp);
	while(1){	}
	OS_Kill();
}

int main(void){   // testmain1
  OS_Init();           // initialize, disable interrupts

  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&mThread, 128, 1);
  OS_Launch(10*TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}

