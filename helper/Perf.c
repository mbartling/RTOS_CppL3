#include "Perf.h"
#include <stdio.h>
unsigned long NumCreated;   // number of foreground threads created
unsigned long PIDWork;      // current number of PID calculations finished
unsigned long FilterWork;   // number of digital filter calculations finished
unsigned long NumSamples;   // incremented every ADC sample, in Producer
unsigned long DataLost;     // data sent by Producer, but not received by Consumer
long MaxJitter;             // largest time jitter between interrupts in usec
//#define JITTERSIZE 64
//unsigned long const JitterSize=JITTERSIZE;
long x[64],y[64];         // input and output arrays for FFT
long jitter;                    // time between measured and expected, in us

const char* traceNameStr[NUM_TRACE_ENUM] = {
  "TRACE_THREAD_0",
  "TRACE_THREAD_1",
  "TRACE_THREAD_2",
  "TRACE_THREAD_3",
  "TRACE_THREAD_4",
  "TRACE_THREAD_5",
  "TRACE_THREAD_6",
  "TRACE_THREAD_7",
  "TRACE_THREAD_8",
  "TRACE_THREAD_9",
  "TRACE_THREAD_10",
  "TRACE_THREAD_11",
  "TRACE_THREAD_12",
  "TRACE_MAIN",
  "TRACE_ADD_THREAD",
  "TRACE_SLEEP",
  "TRACE_WAIT",
  "TRACE_SIGNAL",
  "TRACE_CS",
  "TRACE_TX_PUT",
  "TRACE_TX_GET",
  "TRACE_RX_PUT",
  "TRACE_RX_GET",
  "TRACE_SYSTICK",
  "TRACE_SCHEDULER",
  "TRACE_LAUNCH",
  "TRACE_KILL"
};

volatile int traceLoc = 0;
#define TRACE_DEPTH 1000
trace_t sys_trace[TRACE_DEPTH];

void add_trace(traceName trName){
  if(traceLoc == TRACE_DEPTH){
    traceLoc = 0;
  }
  trace_t *trace = &sys_trace[traceLoc];
  trace->trName = trName;
  trace->tid = OS_Id();
  trace->time = OS_Time(); 
  traceLoc++;
} 

void dump_trace(){
	int i = 0;
	for(i = 0; i < TRACE_DEPTH; i++){
		sys_trace[i].print();
	}
	printf("\n=============\n");
	printf("Trace Loc: %d", traceLoc);
}

void HardFault_Handler(void){
	dump_trace();
	
	while(1){
	}
	
}