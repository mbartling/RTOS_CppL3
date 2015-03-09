#include "OS.h"
#include "inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "ADC.h"
//#include "UART2.h"
#include <string.h> 
#include <stdio.h> 
#include "Timer.h"
#include "interpreter.h"
#include "Perf.h"

//#define TESTMAIN 4
//#define Task1
//#define Task1_5 // to be able to run this task properly, make sure to comment out the taks run in Timer2A_Handler
//#define Task2
//#define Task3
//#define Task4
//#define Task5
// #define Task6 //testing the fft filter
//#define mainTaskLab2
#define Testmain5
//*********Prototype for FFT in cr4_fft_64_stm32.s, STMicroelectronics
#ifdef __cplusplus
extern "C" {
#endif
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);
short PID_stm32(short Error, short *Coeff);
#ifdef __cplusplus
}
#endif
//*********Prototype for PID in PID_stm32.s, STMicroelectronics
void Interpreter() {
    while(1) {
        interpreter();
    }
}
void Jitter(void){
  int i;
  for(i = 0; i < 64; i++){
    printf("%d\t", jitter);
  }
  
}
//unsigned long NumCreated;   // number of foreground threads created
//unsigned long PIDWork;      // current number of PID calculations finished
//unsigned long FilterWork;   // number of digital filter calculations finished
//unsigned long NumSamples;   // incremented every ADC sample, in Producer
#define FS 400            // producer/consumer sampling
#define RUNLENGTH (20*FS) // display results and quit when NumSamples==RUNLENGTH

// 20-sec finite time experiment duration 

#define PERIOD TIME_500US // DAS 2kHz sampling period in system time units
//long x[64],y[64];         // input and output arrays for FFT

//---------------------User debugging-----------------------
//unsigned long DataLost;     // data sent by Producer, but not received by Consumer
//long MaxJitter;             // largest time jitter between interrupts in usec
//#define JITTERSIZE 64
//unsigned long const JitterSize=JITTERSIZE;
unsigned long JitterHistogram[JITTERSIZE]={0,};
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
  GPIO_PORTE_AFSEL_R &= ~0x0F;   // disable alt funct on PE3-0
  GPIO_PORTE_DEN_R |= 0x0F;     // enable digital I/O on PE3-0
  GPIO_PORTE_PCTL_R = ~0x0000FFFF;
  GPIO_PORTE_AMSEL_R &= ~0x0F;;      // disable analog functionality on PF
}
////------------------Task 1--------------------------------
// 2 kHz sampling ADC channel 1, using software start trigger
// background thread executed at 2 kHz
// 60-Hz notch high-Q, IIR filter, assuming fs=2000 Hz
// y(n) = (256x(n) -503x(n-1) + 256x(n-2) + 498y(n-1)-251y(n-2))/256 (2k sampling)
// y(n) = (256x(n) -476x(n-1) + 256x(n-2) + 471y(n-1)-251y(n-2))/256 (1k sampling)
long Filter(long data){
static long x[6]; // this MACQ needs twice
static long y[6];
static unsigned long n=3;   // 3, 4, or 5
  n++;
  if(n==6) n=3;     
  x[n] = x[n-3] = data;  // two copies of new data
  y[n] = (256*(x[n]+x[n-2])-503*x[n-1]+498*y[n-1]-251*y[n-2]+128)/256;
  y[n-3] = y[n];         // two copies of filter outputs too
  return y[n];
} 
//******** DAS *************** 
// background thread, calculates 60Hz notch filter
// runs 2000 times/sec
// samples channel 4, PD3,
// inputs:  none
// outputs: none
int RandomDelay;
int JitterValues[100];
unsigned long DASoutput;
void DAS(void){ 
unsigned long input;  
unsigned static long LastTime;  // time at previous ADC sample
unsigned long thisTime;         // time at current ADC sample
//long jitter;                    // time between measured and expected, in us
    if(NumSamples < RUNLENGTH){   // finite time run
    PE0 ^= 0x01;
    input = ADC_In();           // channel set when calling ADC_Init
    PE0 ^= 0x01;
    thisTime = OS_Time();       // current time, 12.5 ns
    DASoutput = Filter(input);
    FilterWork++;        // calculation finished
    //adding to the jitter time 
    if(FilterWork>1){    // ignore timing of first interrupt
      unsigned long diff = OS_TimeDifference(LastTime,thisTime);
      if(diff>PERIOD){
          jitter = (diff-PERIOD+4)/8;  // in 0.1 usec
      }else{
        jitter = (PERIOD-diff+4)/8;  // in 0.1 usec
      }
      if(jitter > MaxJitter){
        MaxJitter = jitter; // in usec
      }       // jitter should be 0
      if(jitter >= JitterSize){
        jitter = JITTERSIZE-1;
      }
      JitterHistogram[jitter]++; 
    }
     
    LastTime = thisTime;
    PE0 ^= 0x01;
  }
}
//--------------end of Task 1-----------------------------
//
////------------------Task 2--------------------------------
//// background thread executes with SW1 button
//// one foreground task created with button push
//// foreground treads run for 2 sec and die
// ***********ButtonWork*************
 void print(int x,int y, char *z, int value){
     printf("%s: ", z); 
     printf("%d\t", value);
 }

void ButtonWork(void){
unsigned long myId = OS_Id(); 
  PE1 ^= 0x02;
  ST7735_Message(1,0,"NumCreated =",NumCreated); 
  PE1 ^= 0x02;
  OS_Sleep(50);     // set this to sleep for 50msec
  ST7735_Message(1,1,"PIDWork     =",PIDWork);
  ST7735_Message(1,2,"DataLost    =",DataLost);
  ST7735_Message(1,3,"Jitter 0.1us=",MaxJitter);
  PE1 ^= 0x02;
  OS_Kill();  // done, OS does not return from a Kill
} 

//************SW1Push*************
// Called when SW1 Button pushed
// Adds another foreground task
// background threads execute once and return
void SW1Push(void){
    if(OS_MsTime() > 20){ // debounce
    if(OS_AddThread(&ButtonWork,100,4)){
      NumCreated++; 
    }
    OS_ClearMsTime();  // at least 20ms between touches
  }
}
void print_hist(void){
	printf("Jitter Histogram\n");
	for(int i = 0; i < 64; i++){
		printf("%ld\t", JitterHistogram[i]);
	}
	OS_Kill();
}
//************SW2Push*************

 //Called when SW2 Button pushed, Lab 3 only
 //Adds another foreground task
 //background threads execute once and return
void SW2Push(void){
   fputc('c', stdout); 
    if(OS_MsTime() > 20){ // debounce
    if(OS_AddThread(&print_hist,100,5)){
      NumCreated++; 
    }
    OS_ClearMsTime();  // at least 20ms between touches
  }
}
//--------------end of Task 2-----------------------------


//------------------Task 3--------------------------------
// hardware timer-triggered ADC sampling at 400Hz
// Producer runs as part of ADC ISR
// Producer uses fifo to transmit 400 samples/sec to Consumer
// every 64 samples, Consumer calculates FFT
// every 2.5ms*64 = 160 ms (6.25 Hz), consumer sends data to Display via mailbox
// Display thread updates LCD with measurement

//******** Producer *************** 
// The Producer in this lab will be called from your ADC ISR
// A timer runs at 400Hz, started by your ADC_Collect
// The timer triggers the ADC, creating the 400Hz sampling
// Your ADC ISR runs when ADC data is ready
// Your ADC ISR calls this function with a 12-bit sample 
// sends data to the consumer, runs periodically at 400Hz
// inputs:  none
// outputs: none
void Producer(unsigned long data){  
    if(NumSamples < RUNLENGTH){   // finite time run
    NumSamples++;               // number of samples
    if(OS_Fifo_Put(data) == 0){ // send to consumer
        DataLost++;
    } 
  } 
}
void Display(void); 

//******** Consumer *************** 
// foreground thread, accepts data from producer
// calculates FFT, sends DC component to Display
// inputs:  none
// outputs: none
void Consumer(void){ 
unsigned long data,DCcomponent;   // 12-bit raw ADC sample, 0 to 4095
unsigned long t;                  // time in 2.5 ms
unsigned long myId = OS_Id(); 
  ADC_Collect(5, FS, &Producer); // start ADC sampling, channel 5, PD2, 400 Hz
  NumCreated += OS_AddThread(&Display,128,0); 
  while(NumSamples < RUNLENGTH) { 
    PE2 = 0x04;
    for(t = 0; t < 64; t++){   // collect 64 ADC samples
      data = OS_Fifo_Get();    // get from producer
      x[t] = data;             // real part is 0 to 4095, imaginary part is 0
    }
    PE2 = 0x00;
    cr4_fft_64_stm32(y,x,64);  // complex FFT of last 64 ADC values
    DCcomponent = y[0]&0xFFFF; // Real part at frequency 0, imaginary part should be zero
    OS_MailBox_Send(DCcomponent); // called every 2.5ms*64 = 160ms
  }
  OS_Kill();  // done
}
//******** Display *************** 
// foreground thread, accepts data from consumer
// displays calculated results on the LCD
// inputs:  none                            
// outputs: none
void Display(void){ 
unsigned long data,voltage;
  //ST7735_Message(0,1,"Run length = ",(RUNLENGTH)/FS);   // top half used for Display
  while(NumSamples < RUNLENGTH) { 
    data = OS_MailBox_Recv();
    voltage = 3000*data/4095;               // calibrate your device so voltage is in mV
    PE3 = 0x08;
    ST7735_Message(0,2,"v(mV) =",voltage);  
    PE3 = 0x00;
  } 
  OS_Kill();  // done
} 

//--------------end of Task 3-----------------------------

//short PID_stm32(short Error, short *Coeff) {
//    return 0;
//}
//------------------Task 4--------------------------------
// foreground thread that runs without waiting or sleeping
// it executes a digital controller 
//******** PID *************** 
// foreground thread, runs a PID controller
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none
short IntTerm;     // accumulated error, RPM-sec
short PrevError;   // previous error, RPM
short Coeff[3];    // PID coefficients
short Actuator;
void PID(void){ 
short err;  // speed error, range -100 to 100 RPM
unsigned long myId = OS_Id(); 
  PIDWork = 0;
  IntTerm = 0;
  PrevError = 0;
  Coeff[0] = 384;   // 1.5 = 384/256 proportional coefficient
  Coeff[1] = 128;   // 0.5 = 128/256 integral coefficient
  Coeff[2] = 64;    // 0.25 = 64/256 derivative coefficient*
  while(NumSamples < RUNLENGTH) { 
    for(err = -1000; err <= 1000; err++){    // made-up data
      Actuator = PID_stm32(err,Coeff)/256;
    }
    PIDWork++;        // calculation finished
  }
  for(;;){ }          // done
}
//--------------end of Task 4-----------------------------

//------------------Task 5--------------------------------
// UART background ISR performs serial input/output
// Two software fifos are used to pass I/O data to foreground
// The interpreter runs as a foreground thread
// The UART driver should call OS_Wait(&RxDataAvailable) when foreground tries to receive
// The UART ISR should call OS_Signal(&RxDataAvailable) when it receives data from Rx
// Similarly, the transmit channel waits on a semaphore in the foreground
// and the UART ISR signals this semaphore (TxRoomLeft) when getting data from fifo
// Modify your intepreter from Lab 1, adding commands to help debug 
// Interpreter is a foreground thread, accepts input from serial port, outputs to serial port
// inputs:  none
// outputs: none
void Interpreter(void);    // just a prototype, link to your interpreter
// add the following commands, leave other commands, if they make sense
// 1) print performance measures 
//    time-jitter, number of data points lost, number of calculations performed
//    i.e., NumSamples, NumCreated, MaxJitter, DataLost, FilterWork, PIDwork
      
// 2) print debugging parameters 
//    i.e., x[], y[] 
////--------------end of Task 5-----------------------------
//
//
////*******************final user main DEMONTRATE THIS TO TA**********
#ifdef mainTaskLab2
int main(void){ 
  OS_Init();           // initialize, disable interrupts
  PortE_Init();
  DataLost = 0;        // lost data between producer and consumer
  NumSamples = 0;
  MaxJitter = 0;       // in 1us units
  ST7735_InitR(INITR_REDTAB); 
  ST7735_LCD_Init();
//********initialize communication channels
  OS_MailBox_Init();
  OS_Fifo_Init(32);    // ***note*** 4 is not big enough*****

//*******attach background tasks***********
  OS_AddSW1Task(&SW1Push,2);
  OS_AddSW2Task(&SW2Push,2);  // add this line in Lab 3
  ADC_Open(4);  // sequencer 3, channel 4, PD3, sampling in DAS()
  OS_AddPeriodicThread(&DAS,PERIOD,1); // 2 kHz real time sampling of PD3

  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&Interpreter,128,2); 
  NumCreated += OS_AddThread(&Consumer,128,1); 
  NumCreated += OS_AddThread(&PID,128,3);  // Lab 3, make this lowest priority
 
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}
#endif


//
//+++++++++++++++++++++++++DEBUGGING CODE++++++++++++++++++++++++
// ONCE YOUR RTOS WORKS YOU CAN COMMENT OUT THE REMAINING CODE
// 
//*******************Initial TEST**********
// This is the simplest configuration, test this first, (Lab 1 part 1)
// run this with 
// no UART interrupts
// no SYSTICK interrupts
// no timer interrupts
// no switch interrupts
// no ADC serial port or LCD output
// no calls to semaphores

unsigned long Count1;   // number of times thread1 loops
unsigned long Count2;   // number of times thread2 loops
unsigned long Count3;   // number of times thread3 loops
unsigned long Count4;   // number of times thread4 loops
unsigned long Count5;   // number of times thread5 loops

#if TESTMAIN == 1
void Thread1(void){
  Count1 = 0;          
  for(;;){
    PE0 ^= 0x01;       // heartbeat
    Count1++;
    //OS_Suspend();      // cooperative multitasking
		OS_Sleep(0);
  }
}
void Thread2(void){
  Count2 = 0;          
  for(;;){
    PE1 ^= 0x02;       // heartbeat
    Count2++;
    //OS_Suspend();      // cooperative multitasking
		OS_Sleep(0);
  }
}
void Thread3(void){
  Count3 = 0;          
  for(;;){
    PE2 ^= 0x04;       // heartbeat
    Count3++;
    //OS_Suspend();      // cooperative multitasking
		OS_Sleep(0);
  }
}

//int Testmain1(void){  // Testmain1
int main(void){  // Testmain1

  OS_Init();          // initialize, disable interrupts
  PortE_Init();       // profile user threads
  NumCreated = 0 ;
  NumCreated += OS_AddThread(&Thread1,128,1); 
  NumCreated += OS_AddThread(&Thread2,128,2); 
  NumCreated += OS_AddThread(&Thread3,128,3); 
  // Count1 Count2 Count3 should be equal or off by one at all times
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}
#endif
//*******************Second TEST**********
// Once the initalize test runs, test this (Lab 1 part 1)
// no UART interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// no timer interrupts
// no switch interrupts
// no ADC serial port or LCD output
// no calls to semaphores
#if TESTMAIN == 2

void Thread1b(void){
  Count1 = 0;          
  for(;;){
    PE0 ^= 0x01;       // heartbeat
    Count1++;
  }
}
void Thread2b(void){
  Count2 = 0;          
  for(;;){
    PE1 ^= 0x02;       // heartbeat
    Count2++;
  }
}
void Thread3b(void){
  Count3 = 0;          
  for(;;){
    PE2 ^= 0x04;       // heartbeat
    Count3++;
  }
}
//int Testmain2(void){  // Testmain2
int main(void){  // Testmain2
  OS_Init();           // initialize, disable interrupts
  PortE_Init();       // profile user threads
  NumCreated = 0 ;
  NumCreated += OS_AddThread(&Thread1b,128,1); 
  NumCreated += OS_AddThread(&Thread2b,128,2); 
  NumCreated += OS_AddThread(&Thread3b,128,3); 
  // Count1 Count2 Count3 should be equal on average
  // counts are larger than testmain1
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}
#endif
//*******************Third TEST**********
// Once the second test runs, test this (Lab 1 part 2)
// no UART1 interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// Timer interrupts, with or without period established by OS_AddPeriodicThread
// PortF GPIO interrupts, active low
// no ADC serial port or LCD output
// tests the spinlock semaphores, tests Sleep and Kill
#if TESTMAIN == 3

Sema4Type Readyc;        // set in background
int Lost;
void BackgroundThread1c(void){   // called at 1000 Hz
  Count1++;
  OS_Signal(&Readyc);
}
void Thread5c(void){
  for(;;){
    OS_Wait(&Readyc);
    Count5++;   // Count2 + Count5 should equal Count1 
    Lost = Count1-Count5-Count2;
  }
}
void Thread2c(void){
  OS_InitSemaphore(&Readyc,0);
  Count1 = 0;    // number of times signal is called      
  Count2 = 0;    
  Count5 = 0;    // Count2 + Count5 should equal Count1  
  NumCreated += OS_AddThread(&Thread5c,128,3); 
  OS_AddPeriodicThread(&BackgroundThread1c,TIME_2MS,0); 
  for(;;){
    OS_Wait(&Readyc);
    Count2++;   // Count2 + Count5 should equal Count1
  }
}

void Thread3c(void){
  Count3 = 0;          
  for(;;){
    Count3++;
  }
}
void Thread4c(void){ int i;
  for(i=0;i<64;i++){
    Count4++;
    OS_Sleep(10);
  }
  OS_Kill();
  Count4 = 0;
}
void BackgroundThread5c(void){   // called when Select button pushed
  NumCreated += OS_AddThread(&Thread4c,128,3); 
	return;
}
      
int main(void){   // Testmain3
  Count4 = 0;          
  OS_Init();           // initialize, disable interrupts
// Count2 + Count5 should equal Count1
  NumCreated = 0 ;
  OS_AddSW1Task(&BackgroundThread5c,2);
  NumCreated += OS_AddThread(&Thread2c,128,2); 
  NumCreated += OS_AddThread(&Thread3c,128,3); 
  NumCreated += OS_AddThread(&Thread4c,128,3); 
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}
#endif
//*******************Fourth TEST**********
// Once the third test runs, run this example (Lab 1 part 2)
// Count1 should exactly equal Count2
// Count3 should be very large
// Count4 increases by 640 every time select is pressed
// NumCreated increase by 1 every time select is pressed

// no UART interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// Timer interrupts, with or without period established by OS_AddPeriodicThread
// Select switch interrupts, active low
// no ADC serial port or LCD output
// tests the spinlock semaphores, tests Sleep and Kill
#if TESTMAIN == 4
Sema4Type Readyd;        // set in background
void BackgroundThread1d(void){   // called at 1000 Hz
static int i=0;
  i++;
  if(i==50){
    i = 0;         //every 50 ms
    Count1++;
    //OS_bSignal(&Readyd);
		OS_Signal(&Readyd);
  }
}
void Thread2d(void){
  OS_InitSemaphore(&Readyd,0);
  Count1 = 0;          
  Count2 = 0;          
  for(;;){
    //OS_bWait(&Readyd);
		OS_Wait(&Readyd);
    Count2++;     
  }
}
void Thread3d(void){
  Count3 = 0;          
  for(;;){
    Count3++;
  }
}
void Thread4d(void){ int i;
  for(i=0;i<640;i++){
    Count4++;
    OS_Sleep(1);
  }
  OS_Kill();
}
void BackgroundThread5d(void){   // called when Select button pushed
  NumCreated += OS_AddThread(&Thread4d,128,3); 
}
int main(void){   // Testmain4
	//int Testmain4(void){   // Testmain4
  Count4 = 0;          
  OS_Init();           // initialize, disable interrupts
  NumCreated = 0 ;
  OS_AddPeriodicThread(&BackgroundThread1d,PERIOD,0); 
  OS_AddSW1Task(&BackgroundThread5d,2);
  NumCreated += OS_AddThread(&Thread2d,128,2); 
  NumCreated += OS_AddThread(&Thread3d,128,3); 
  NumCreated += OS_AddThread(&Thread4d,128,3); 
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}
#endif


#ifdef Task1
//this 
void printHistogram() {
    for (int i = 0; i < 64; i++){
        print(0, i, "jitter", JitterHistogram[i]);
        //ST7735_Message(0, i, "jitter", JitterHistogram[i]);
    }
    OS_Kill();
}

//launching the printStuff function
void addPrintHistogram() {
    NumCreated += OS_AddThread(&printHistogram,128,2); 
}

void printHistogramToTerminal() {
    for (int i = 0; i < 64; i++){
        printf("%u\t", JitterHistogram[i]); 
    }
    OS_Kill();
}

//launching the printStuff function
void addPrintHistogramToTerminal() {
    NumCreated += OS_AddThread(&printHistogramToTerminal,128,2); 
}


int main(void){   
  OS_Init();           // initialize, disable interrupts
  PortE_Init();       // profile user threads
  ST7735_InitR(INITR_REDTAB); 
  ST7735_LCD_Init();
  int channelNumber = 4;
  ADC_Open(channelNumber); 
  OS_AddPeriodicThread(&DAS, PERIOD, 0);
  OS_AddSW1Task(&addPrintHistogram,2);
  OS_AddSW2Task(&addPrintHistogramToTerminal,2);
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
} 
#endif






//the following task is designed to figure out the interference of timers together
#ifdef Task1_5 
int main(void){   // Testmain4
 //int Testmain4(void){   // Testmain4
  Count4 = 0;          
  OS_Init();           // initialize, disable interrupts
  PortE_Init();       // profile user threads
  Timer2A_Init((uint32_t)PERIOD);
  //EnableInterrupts();
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes

 
} 
#endif


#ifdef Task2 
int main(void){   // Testmain4
  Count4 = 0;          
  OS_Init();           // initialize, disable interrupts
  PortE_Init();       // profile user threads
  ST7735_InitR(INITR_REDTAB); 
  ST7735_LCD_Init();
  OS_AddSW1Task(&SW1Push,2);
  OS_AddSW2Task(&SW2Push,2);
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
 } 
#endif


#ifdef Task3
void printDataLost() {
    printf("the DataLost is %u\t", DataLost);
    OS_Kill();
}

//launching the printStuff function
void addPrintDataLost() {
    NumCreated += OS_AddThread(&printDataLost,128,2); 
}

int main(void){   // Testmain4
  OS_Init();           // initialize, disable interrupts
  PortE_Init();       // profile user threads
  ST7735_InitR(INITR_REDTAB); 
  ST7735_LCD_Init();
  NumCreated += OS_AddThread(&Consumer,128,2); 
  OS_AddSW1Task(&addPrintDataLost,2);
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
 } 
#endif

#ifdef Task4
void printPID() {
    printf("this is PID %d\n", PIDWork);
    OS_Kill();
}
void addPrintPID() {
    NumCreated += OS_AddThread(&printPID,128,2); 
}


int main(void){   // Testmain4
  OS_Init();           // initialize, disable interrupts
  PortE_Init();       // profile user threads
  NumCreated += OS_AddThread(&PID, 128,2); 
  OS_AddSW1Task(&addPrintPID,2);
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
 } 
#endif

//
#ifdef Task6
 void dummyThread(void){
	 int counter = 0;
	for(int j = 0; j < 100; j++){
		for (int i = 0 ; i < 64; i++) {
      x[i] = 0;
		}
		x[0] = j;
		cr4_fft_64_stm32(y,x,64);  // complex FFT of last 64 ADC values
		printf("y is %d\n", j);
		for(int i = 0; i < 64; i++){
			printf("(%d + i%d)\t",(short)(y[i] & 0xFFFF), (short)(y[i] >> 16));
			//printf("%d\t", counter);
			counter++;
		}
		printf("\n\n");
	//OS_Sleep(5);
}
	 OS_Kill();
 }
int main(void){   // Testmain4
  PortE_Init();       // profile user threads
  //OS_setupTest() ;
	OS_Init();
//	OS_AddThread(dummyThread,100,5);
	OS_AddThread(Interpreter,100,5);
	OS_Launch(TIME_2MS);

} 
#endif

//******************* Lab 3 Preparation 2**********
// Modify this so it runs with your RTOS (i.e., fix the time units to match your OS)
// run this with 
// UART0, 115200 baud rate, used to output results 
// SYSTICK interrupts, period established by OS_Launch
// first timer interrupts, period established by first call to OS_AddPeriodicThread
// second timer interrupts, period established by second call to OS_AddPeriodicThread
// SW1 no interrupts
// SW2 no interrupts
unsigned long CountA;   // number of times Task A called
unsigned long CountB;   // number of times Task B called
//unsigned long Count1;   // number of times thread1 loops

#define UART_OutString(...) printf(__VA_ARGS__)
//*******PseudoWork*************
// simple time delay, simulates user program doing real work
// Input: amount of work in 100ns units (free free to change units
// Output: none
void PseudoWork(unsigned short work){
unsigned short startTime;
 startTime = OS_Time();    // time in 100ns units
 while(OS_TimeDifference(startTime,OS_Time()) <= work){} 
}
void Thread6(void){  // foreground thread
 Count1 = 0;          
 for(;;){
   Count1++; 
   PE0 ^= 0x01;        // debugging toggle bit 0  
 }
}
extern void Jitter(void);   // prints jitter information (write this)
void Thread7(void){  // foreground thread
 UART_OutString("\n\rEE345M/EE380L, Lab 3 Preparation 2\n\r");
 OS_Sleep(5000);   // 10 seconds        
 Jitter();         // print jitter information
 UART_OutString("\n\r\n\r");
 OS_Kill();
}
#define workA 500       // {5,50,500 us} work in Task A
#define counts1us 10    // number of OS_Time counts per 1us
void TaskA(void){       // called every {1000, 2990us} in background
 PE1 = 0x02;      // debugging profile  
 CountA++;
 PseudoWork(workA*counts1us); //  do work (100ns time resolution)
 PE1 = 0x00;      // debugging profile  
}
#define workB 250       // 250 us work in Task B
void TaskB(void){       // called every pB in background
 PE2 = 0x04;      // debugging profile  
 CountB++;
 PseudoWork(workB*counts1us); //  do work (100ns time resolution)
 PE2 = 0x00;      // debugging profile  
}
#ifdef Testmain5
int main(void){       // Testmain5 Lab 3
 PortE_Init();
 OS_Init();           // initialize, disable interrupts
 NumCreated = 0 ;
 NumCreated += OS_AddThread(&Thread6,128,2); 
 NumCreated += OS_AddThread(&Thread7,128,1); 
 OS_AddPeriodicThread(&TaskA,TIME_1MS,0);           // 1 ms, higher priority
 OS_AddPeriodicThread(&TaskB,2*TIME_1MS,1);         // 2 ms, lower priority

 OS_Launch(TIME_2MS); // 2ms, doesn't return, interrupts enabled in here
 return 0;             // this never executes
}
#endif

//******************* Lab 3 Preparation 4**********
// Modify this so it runs with your RTOS used to test blocking semaphores
// run this with 
// UART0, 115200 baud rate,  used to output results 
// SYSTICK interrupts, period established by OS_Launch
// first timer interrupts, period established by first call to OS_AddPeriodicThread
// second timer interrupts, period established by second call to OS_AddPeriodicThread
// SW1 no interrupts, 
// SW2 no interrupts
Sema4Type s;            // test of this counting semaphore
unsigned long SignalCount1;   // number of times s is signaled
unsigned long SignalCount2;   // number of times s is signaled
unsigned long SignalCount3;   // number of times s is signaled
unsigned long WaitCount1;     // number of times s is successfully waited on
unsigned long WaitCount2;     // number of times s is successfully waited on
unsigned long WaitCount3;     // number of times s is successfully waited on
#define MAXCOUNT 20000
void OutputThread(void){  // foreground thread
 UART_OutString("\n\rEE345M/EE380L, Lab 3 Preparation 4\n\r");
 while(SignalCount1+SignalCount2+SignalCount3<100*MAXCOUNT){
   OS_Sleep(1000);   // 1 second
   UART_OutString(".");
 }       
 UART_OutString(" done\n\r");
 UART_OutString("Signalled=%d", SignalCount1+SignalCount2+SignalCount3);
 UART_OutString(", Waited=%d", WaitCount1+WaitCount2+WaitCount3);
 UART_OutString("\n\r");
 OS_Kill();
}
void Wait1(void){  // foreground thread
 for(;;){
   OS_Wait(&s);    // three threads waiting
   WaitCount1++; 
 }
}
void Wait2(void){  // foreground thread
 for(;;){
   OS_Wait(&s);    // three threads waiting
   WaitCount2++; 
 }
}
void Wait3(void){   // foreground thread
 for(;;){
   OS_Wait(&s);    // three threads waiting
   WaitCount3++; 
 }
}
void Signal1(void){      // called every 799us in background
 if(SignalCount1<MAXCOUNT){
   OS_Signal(&s);
   SignalCount1++;
 }
}
// edit this so it changes the periodic rate
void Signal2(void){       // called every 1111us in background
 if(SignalCount2<MAXCOUNT){
   OS_Signal(&s);
   SignalCount2++;
 }
}
void Signal3(void){       // foreground
 while(SignalCount3<98*MAXCOUNT){
   OS_Signal(&s);
   SignalCount3++;
 }
 OS_Kill();
}

long add(const long n, const long m){
static long result;
 result = m+n;
 return result;
}
#ifdef Testmain6
int main(void){      // Testmain6  Lab 3
 volatile unsigned long delay;
 OS_Init();           // initialize, disable interrupts
 delay = add(3,4);
 PortE_Init();
 SignalCount1 = 0;   // number of times s is signaled
 SignalCount2 = 0;   // number of times s is signaled
 SignalCount3 = 0;   // number of times s is signaled
 WaitCount1 = 0;     // number of times s is successfully waited on
 WaitCount2 = 0;     // number of times s is successfully waited on
 WaitCount3 = 0;     // number of times s is successfully waited on
 OS_InitSemaphore(&s,0);    // this is the test semaphore
 OS_AddPeriodicThread(&Signal1,(799*TIME_1MS)/1000,0);   // 0.799 ms, higher priority
 OS_AddPeriodicThread(&Signal2,(1111*TIME_1MS)/1000,1);  // 1.111 ms, lower priority
 NumCreated = 0 ;
 NumCreated += OS_AddThread(&Thread6,128,6);       // idle thread to keep from crashing
 NumCreated += OS_AddThread(&OutputThread,128,2);  // results output thread
 NumCreated += OS_AddThread(&Signal3,128,2);   // signalling thread
 NumCreated += OS_AddThread(&Wait1,128,2);     // waiting thread
 NumCreated += OS_AddThread(&Wait2,128,2);     // waiting thread
 NumCreated += OS_AddThread(&Wait3,128,2);     // waiting thread

 OS_Launch(TIME_1MS);  // 1ms, doesn't return, interrupts enabled in here
 return 0;             // this never executes
}
#endif

//******************* Lab 3 Measurement of context switch time**********
// Run this to measure the time it takes to perform a task switch
// UART0 not needed 
// SYSTICK interrupts, period established by OS_Launch
// first timer not needed
// second timer not needed
// SW1 not needed, 
// SW2 not needed
// logic analyzer on PF1 for systick interrupt (in your OS)
//                on PE0 to measure context switch time
void Thread8(void){       // only thread running
 while(1){
   PE0 ^= 0x01;      // debugging profile  
 }
}
#ifdef Testmain7
// int Testmain7(void){       // Testmain7
int main(void){
 PortE_Init();
 OS_Init();           // initialize, disable interrupts
 NumCreated = 0 ;
 NumCreated += OS_AddThread(&Thread8,128,2); 
 OS_Launch(TIME_1MS/10); // 100us, doesn't return, interrupts enabled in here
 return 0;             // this never executes
}
#endif
