#include "Perf.h"

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
