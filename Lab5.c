//*****************************************************************************
//
// Lab5.c - user programs, File system, stream data onto disk
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
#include "edisk.h"
#include "efile.h"
#include "Perf.h"

//unsigned long NumCreated;   // number of foreground threads created
//unsigned long NumSamples;   // incremented every sample
//unsigned long DataLost;     // data sent by Producer, but not received by Consumer

int Running;                // true while robot is running

#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

#define GPIO_PF0  (*((volatile unsigned long *)0x40025004))
#define GPIO_PF1  (*((volatile unsigned long *)0x40025008))
#define GPIO_PF2  (*((volatile unsigned long *)0x40025010))
#define GPIO_PF3  (*((volatile unsigned long *)0x40025020))
#define GPIO_PG1  (*((volatile unsigned long *)0x40026008))

#define PE0  (*((volatile unsigned long *)0x40024004))
#define PE1  (*((volatile unsigned long *)0x40024008))
#define PE2  (*((volatile unsigned long *)0x40024010))
#define PE3  (*((volatile unsigned long *)0x40024020))
void PortE_Init(void){ unsigned long volatile delay;
  //SYSCTL_RCGC2_R |= 0x10;       // activate port E
  SYSCTL_RCGCGPIO_R |= 0x10;       
  delay = SYSCTL_RCGC2_R;        
  delay = SYSCTL_RCGC2_R;         
  GPIO_PORTE_DIR_R |= 0x0F;    // make PE3-0 output heartbeats
	GPIO_PORTE_DIR_R &= ~0x04;
  GPIO_PORTE_AFSEL_R &= ~0x0F;   // disable alt funct on PE3-0
  GPIO_PORTE_DEN_R |= 0x0F;     // enable digital I/O on PE3-0
  GPIO_PORTE_PCTL_R = ~0x0000FFFF;
  GPIO_PORTE_AMSEL_R &= ~0x0F;;      // disable analog functionality on PF
}

// PF1/IDX1 is user input select switch
// PE1/PWM5 is user input down switch 
// PF0/PWM0 is debugging output on Systick
// PF2/LED1 is debugging output 
// PF3/LED0 is debugging output 
// PG1/PWM1 is debugging output 


//******** Robot *************** 
// foreground thread, accepts data from producer
// inputs:  none
// outputs: none
void Robot(void){   
unsigned long data;      // ADC sample, 0 to 1023
unsigned long voltage;   // in mV,      0 to 3000
unsigned long time;      // in 10msec,  0 to 1000 
unsigned long t=0;
  OS_ClearMsTime();    
  DataLost = 0;          // new run with no lost data 
  printf("Robot running...");
  eFile_RedirectToFile("Robot");
  printf("time(sec)\tdata(volts)\n\r");
  do{
    t++;
    time=OS_MsTime();            // 10ms resolution in this OS
    data = OS_Fifo_Get();        // 1000 Hz sampling get from producer
    voltage = (300*data)/1024;   // in mV
    printf("%0u.%02u\t%0u.%03u\n\r",time/100,time%100,voltage/1000,voltage%1000);
  }
  while(time < 1000);       // change this to mean 10 seconds
  eFile_EndRedirectToFile();
  printf("done.\n\r");
  Running = 0;                // robot no longer running
  OS_Kill();
}
  
//************ButtonPush*************
// Called when Select Button pushed
// background threads execute once and return
void ButtonPush(void){
  if(Running==0){
    Running = 1;  // prevents you from starting two robot threads
    NumCreated += OS_AddThread(&Robot,128,1);  // start a 20 second run
  }
}
//************DownPush*************
// Called when Down Button pushed
// background threads execute once and return
void DownPush(void){

}



//******** Producer *************** 
// The Producer in this lab will be called from your ADC ISR
// A timer runs at 1 kHz, started by your ADC_Collect
// The timer triggers the ADC, creating the 1 kHz sampling
// Your ADC ISR runs when ADC data is ready
// Your ADC ISR calls this function with a 10-bit sample 
// sends data to the Robot, runs periodically at 1 kHz
// inputs:  none
// outputs: none
void Producer(unsigned long data){  
  if(Running){
    if(OS_Fifo_Put(data)){     // send to Robot
      NumSamples++;
    } else{ 
      DataLost++;
    } 
  }
}
 
//******** IdleTask  *************** 
// foreground thread, runs when no other work needed
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none
unsigned long Idlecount=0;
void IdleTask(void){ 
  while(1) { 
    Idlecount++;        // debugging 
  }
}


//******** Interpreter **************
// your intepreter from Lab 4 
// foreground thread, accepts input from serial port, outputs to serial port
// inputs:  none
// outputs: none
extern void Interpreter(void);
void Interpreter(){
	char buff[10];
	printf(">>\n\r");
	scanf("%s\n", buff);
	printf("%s\n\r", buff);
}
// add the following commands, remove commands that do not make sense anymore
// 1) format 
// 2) directory 
// 3) print file
// 4) delete file
// execute   eFile_Init();  after periodic interrupts have started

//*******************lab 5 main **********
int realmain(void){        // lab 5 real main
  OS_Init();           // initialize, disable interrupts
  Running = 0;         // robot not running
  DataLost = 0;        // lost data between producer and consumer
  NumSamples = 0;

//********initialize communication channels
  OS_Fifo_Init(512);    // ***note*** 4 is not big enough*****
  ADC_Collect(0, 1000, &Producer); // start ADC sampling, channel 0, 1000 Hz

//*******attach background tasks***********
//  OS_AddButtonTask(&ButtonPush,2);
//  OS_AddButtonTask(&DownPush,3);
  OS_AddPeriodicThread(disk_timerproc,10*TIME_1MS,5);

  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&Interpreter,128,2); 
  NumCreated += OS_AddThread(&IdleTask,128,7);  // runs when nothing useful to do
 
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}


//*****************test programs*************************
unsigned char buffer[512];
#define MAXBLOCKS 100
void diskError(char* errtype, unsigned long n){
  printf(errtype);
  printf(" disk error %u",n);
  OS_Kill();
}
void TestDisk(void){  DSTATUS result;  unsigned short block;  int i; unsigned long n;
  // simple test of eDisk
  printf("\n\rEE345M/EE380L, Lab 5 eDisk test\n\r");
  result = eDisk_Init(0);  // initialize disk
  if(result) diskError("eDisk_Init",result);
  printf("Writing blocks\n\r");
  n = 1;    // seed
  for(block = 0; block < MAXBLOCKS; block++){
    for(i=0;i<512;i++){
      n = (16807*n)%2147483647; // pseudo random sequence
      buffer[i] = 0xFF&n;        
    }
    GPIO_PF3 = 0x08;     // PF3 high for 100 block writes
    if(eDisk_WriteBlock(buffer,block))diskError("eDisk_WriteBlock",block); // save to disk
    GPIO_PF3 = 0x00;     
  }  
  printf("Reading blocks\n\r");
  n = 1;  // reseed, start over to get the same sequence
  for(block = 0; block < MAXBLOCKS; block++){
    GPIO_PF2 = 0x04;     // PF2 high for one block read
    if(eDisk_ReadBlock(buffer,block))diskError("eDisk_ReadBlock",block); // read from disk
    GPIO_PF2 = 0x00;
    for(i=0;i<512;i++){
      n = (16807*n)%2147483647; // pseudo random sequence
      if(buffer[i] != (0xFF&n)){
        printf("Read data not correct, block=%u, i=%u, expected %u, read %u\n\r",block,i,(0xFF&n),buffer[i]);
        OS_Kill();
      }      
    }
  }  
  printf("Successful test of %u blocks\n\r",MAXBLOCKS);
  OS_Kill();
}
void RunTest(void){
  NumCreated += OS_AddThread(&TestDisk,128,1);  
}
//******************* test main1 **********
// SYSTICK interrupts, period established by OS_Launch
// Timer interrupts, period established by first call to OS_AddPeriodicThread
#ifdef testmain1
int main(void){   // testmain1
  OS_Init();           // initialize, disable interrupts

//*******attach background tasks***********
  OS_AddPeriodicThread(&disk_timerproc,10*TIME_1MS,0);   // time out routines for disk
//  OS_AddButtonTask(&RunTest,2);
  
  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&TestDisk,128,1);  
  //NumCreated += OS_AddThread(&IdleTask,128,3); 
 
  OS_Launch(10*TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}
#endif
void TestFile(void){   int i; char data; 
  printf("\n\rEE345M/EE380L, Lab 5 eFile test\n\r");
  // simple test of eFile
  if(eFile_Init())              diskError("eFile_Init",0); 
  if(eFile_Format())            diskError("eFile_Format",0); 
//  eFile_Directory(&Serial_OutChar);
  if(eFile_Create("file1"))     diskError("eFile_Create",0);
  if(eFile_WOpen("file1"))      diskError("eFile_WOpen",0);
  for(i=0;i<1000;i++){
    if(eFile_Write('a'+i%26))   diskError("eFile_Write",i);
    if(i%52==51){
      if(eFile_Write('\n'))     diskError("eFile_Write",i);  
      if(eFile_Write('\r'))     diskError("eFile_Write",i);
    }
  }
  if(eFile_WClose())            diskError("eFile_Close",0);
//  eFile_Directory(&Serial_OutChar);
  if(eFile_ROpen("file1"))      diskError("eFile_ROpen",0);
  for(i=0;i<1000;i++){
    if(eFile_ReadNext(&data))   diskError("eFile_ReadNext",i);
//    Serial_OutChar(data);
  }
  if(eFile_Delete("file1"))     diskError("eFile_Delete",0);
//  eFile_Directory(&Serial_OutChar);
  printf("Successful test of creating a file\n\r");
  OS_Kill();
}

//******************* test main2 **********
// SYSTICK interrupts, period established by OS_Launch
// Timer interrupts, period established by first call to OS_AddPeriodicThread
int testmain2(void){ 
  OS_Init();           // initialize, disable interrupts

//*******attach background tasks***********
  OS_AddPeriodicThread(&disk_timerproc,10*TIME_1MS,0);   // time out routines for disk
  
  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&TestFile,128,1);  
  //NumCreated += OS_AddThread(&IdleTask,128,3); 
 
  OS_Launch(10*TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}
void TestFAT(void){
  // simple test of eDisk
  eFile_Init();
  printf("\n\rEE345M/EE380L, Lab 5 eDisk test\n\r");

  printf("Successful test of %u blocks\n\r",MAXBLOCKS);
  OS_Kill();
}

uint32_t Ping(void){
	uint32_t pingStartTime; 
	uint32_t pingEndTime;
	//Write GPIO_Pin High
  PE3 = 0x08;
	OS_DelayUS(5);
	//Write GPIO Pin Low
	PE3 = 0x00;
	//Could move into a ISR
	//while(gpio_Pin_Low){}
  //while(PE2 == 0){OS_DelayUS(1);}
	while(PE2 == 0){}
	pingStartTime = OS_GetUsTime();
	//while(gpio_Pin_High){}
  //while(PE2 != 0){OS_DelayUS(1);}
	while(PE2 != 0){}
//	OS_DelayUS(18500); //For Simulation

	pingEndTime = OS_GetUsTime();
	OS_DelayUS(200);
	//Speed of sound in air is approx c = 343 m/s = 340 m/s * 1000m.0m/m * (1s/100000us)
	//Then the ping time is c*dT/2
	return ((pingEndTime - pingStartTime) /*>> 1*/)*34300/1000000;
	//return (uint32_t)(((float)((pingEndTime - pingStartTime) >> 1))*34000.0/1000000.0);
}
void TestUs(void){
	
	uint32_t currTime; 
	uint32_t currTime2;
	uint32_t distance;
	//uint32_t distance_buff[4];
	unsigned long id = OS_Id();
	uint32_t i = 0;
	uint32_t x = 5;
	x--;
	x--;
	x--;
	currTime = OS_GetUsTime();
	OS_DelayUS(5);
	currTime2 = OS_GetUsTime();
	printf("Verifying Timing Conditions\n");
	printf("%u\t%u\t%u\n", currTime, currTime2, currTime2-currTime);
	
	currTime = OS_GetUsTime();
	currTime2 = OS_GetUsTime();
	printf("Verifying Timing Conditions\n");
	printf("%u\t%u\t%u\n", currTime, currTime2, currTime2-currTime);
	
		currTime = OS_GetUsTime();
		currTime2 = OS_GetUsTime();
		printf("Verifying Timing Conditions\n");
		printf("%u\t%u\t%u\n", currTime, currTime2, currTime2-currTime);
	while(1){
		distance = 0;
		for(i = 0; i < 4; i++){
			currTime = OS_GetUsTime();
			//distance_buff[i] = Ping();
			distance += Ping();
			currTime2 = OS_GetUsTime();
		}
		distance = distance >> 2;
		printf("ID: %u\t\t%u\t\t%u\n\r", id, distance, currTime2-currTime);
	}
	OS_Kill();
}
int main(void){   // testmain1
  OS_Init();           // initialize, disable interrupts
  PortE_Init();
  PE3 = 0x00;
//*******attach background tasks***********
//  OS_AddPeriodicThread(&disk_timerproc,10*TIME_1MS,0);   // time out routines for disk
//  OS_AddButtonTask(&RunTest,2);
  
  NumCreated = 0 ;
// create initial foreground threads
//  NumCreated += OS_AddThread(&TestFAT,128,1);  
  //NumCreated += OS_AddThread(&IdleTask,128,3); 
  NumCreated += OS_AddThread(&TestUs, 128, 1);
	//NumCreated += OS_AddThread(&TestUs, 128, 1);
  OS_Launch(10*TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}
