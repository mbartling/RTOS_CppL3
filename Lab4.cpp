#include "os.h"
#include "inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "ADC.h"
//#include "UART2.h"
#include <string.h> 
#include <stdio.h> 
#include "Timer.h"
#include "interpreter.h"
#include "Perf.h"
#include <math.h>
//#define TESTMAIN 4
//#define Task1
//#define Task1_5 // to be able to run this task properly, make sure to comment out the taks run in Timer2A_Handler
//#define Task2
//#define Task3
//#define Task4
//#define Task5
// #define Task6 //testing the fft filter
#define mainTaskLab2
//#define Testmain6
//#define Testmain8
//*********Prototype for FFT in cr4_fft_64_stm32.s, STMicroelectronics
#ifdef __cplusplus
extern "C" {
#endif
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);
//short PID_stm32(short Error, short *Coeff);
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
  //  if(NumSamples < RUNLENGTH){   // finite time run
    PE0 ^= 0x01;
    input = ADC_In();           // channel set when calling ADC_Init
    PE0 ^= 0x01;
    thisTime = OS_Time();       // current time, 12.5 ns
    //DASoutput = Filter(input);
    if(OS_Fifo_Put(input) == 0){ // send to consumer
        DataLost++;
    } 
    LastTime = thisTime;
    PE0 ^= 0x01;
//  }
}

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
int mode = 0;
void Consumer(void){ 
unsigned long data,DCcomponent;   // 12-bit raw ADC sample, 0 to 4095
unsigned long t;                  // time in 2.5 ms
unsigned long myId = OS_Id(); 
  ADC_Collect(5, FS, &Producer); // start ADC sampling, channel 5, PD2, 400 Hz
  NumCreated += OS_AddThread(&Display,128,1);
  while(1){ 
  while(NumSamples < RUNLENGTH) { 
    PE2 = 0x04;
    for(t = 0; t < 64; t++){   // collect 64 ADC samples
      data = OS_Fifo_Get();    // get from producer
      x[t] = data;             // real part is 0 to 4095, imaginary part is 0
    }
    PE2 = 0x00;
    cr4_fft_64_stm32(y,x,64);  // complex FFT of last 64 ADC values
    //DCcomponent = y[0]&0xFFFF; // Real part at frequency 0, imaginary part should be zero
    OS_MailBox_Send(mode); // called every 2.5ms*64 = 160ms
  }
	NumSamples = 0;
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
  while(1) { 
    data = OS_MailBox_Recv();
    voltage = 3000*data/4095;               // calibrate your device so voltage is in mV
    PE3 = 0x08;
    PE3 = 0x00;
    long* buff = (mode) ? &y[0] : &x[0];
    plot(buff, mode);
  } 
  OS_Kill();  // done
} 

//--------------end of Task 3-----------------------------


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
  grapics_init();
  PortE_Init();
  DataLost = 0;        // lost data between producer and consumer
  NumSamples = 0;
  MaxJitter = 0;       // in 1us units
  ST7735_InitR(INITR_REDTAB); 
  //ST7735_LCD_Init();
//********initialize communication channels
  OS_MailBox_Init();
  OS_Fifo_Init(64);    // ***note*** 4 is not big enough*****

//*******attach background tasks***********
  //OS_AddSW1Task(&SW1Push,2);
  //OS_AddSW2Task(&SW2Push,2);  // add this line in Lab 3
  //ADC_Open(4);  // sequencer 3, channel 4, PD3, sampling in DAS()
//  OS_AddPeriodicThread(&DAS,PERIOD,1); // 2 kHz real time sampling of PD3

  NumCreated = 0 ;
// create initial foreground threads

  //NumCreated += OS_AddThread(&Interpreter,128,2); 
  NumCreated += OS_AddThread(&Consumer,128,1); 
  //NumCreated += OS_AddThread(&PID,128,3);  // Lab 3, make this lowest priority
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}
#endif


//
//+++++++++++++++++++++++++DEBUGGING CODE++++++++++++++++++++++++
// ONCE YOUR RTOS WORKS YOU CAN COMMENT OUT THE REMAINING CODE
// 


#ifdef Testmain8
// int Testmain7(void){       // Testmain7
void produceCrap(void){
	unsigned long data = 0;
	while(1){
		//OS_MailBox_Send(data);
		if(OS_Fifo_Put(data) == 0){ // send to consumer
      DataLost++;
    } 
		//OS_Sleep(1);
		data++;
	}
}
void produceCrap2(void){
	unsigned long data = 0;
	while(1){
		OS_MailBox_Send(data);
		//if(OS_Fifo_Put(data) == 0){ // send to consumer
    //  DataLost++;
    //} 
		//OS_Sleep(1);
		data++;
	}
}
int numPlaces(int n){
	if(n == 0){
		return 1;
	}
	return (int) floor(log10(abs((float)n))) + 1;
}
unsigned long crapData1;
unsigned long crapData2;
void consumeCrap(void){
	unsigned long data = 0;
	int numBS = 0;
	while(1){
		//data = OS_MailBox_Recv();
		crapData1 = OS_Fifo_Get();
		numBS = numPlaces(crapData1);
//		for(int i = 0; i < numBS; i++){
//			putchar('\b');
//		}
//		printf("%d", crapData1);
	}
}
void consumeCrap2(void){
	unsigned long data = 0;
	int numBS = 0;
	while(1){
		crapData2 = OS_MailBox_Recv();
		//data = OS_Fifo_Get();
		numBS = numPlaces(crapData2);
//		for(int i = 0; i < numBS; i++){
//			putchar('\b');
//		}
//		printf("%d", data);
	}
}
void doCrap(void){
	int numBS = numPlaces(crapData1);
		for(int i = 0; i < numBS; i++){
			putchar('\b');
		}
		putchar('\b');
		numBS = numPlaces(crapData2);
		for(int i = 0; i < numBS; i++){
			putchar('\b');
		}
		printf("%d\t%d", crapData1, crapData2);

}
int main(void){
 PortE_Init();
 OS_Init();           // initialize, disable interrupts
 OS_MailBox_Init();
 OS_Fifo_Init(64);    // ***note*** 4 is not big enough*****

 NumCreated = 0 ;
 NumCreated += OS_AddThread(&produceCrap,128,2); 
 NumCreated += OS_AddThread(&consumeCrap,128,2); 
 NumCreated += OS_AddThread(&produceCrap,128,3); 
 NumCreated += OS_AddThread(&consumeCrap,128,3); 
 OS_AddPeriodicThread(&doCrap,(100*TIME_1MS),0);   // 10 ms, higher priority

 OS_Launch(TIME_1MS); // 100us, doesn't return, interrupts enabled in here
 return 0;             // this never executes
}
#endif
