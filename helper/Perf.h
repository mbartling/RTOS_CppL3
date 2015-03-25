#ifndef __PERF_H__
#define __PERF_H__
#include <stdint.h>
#include <stdio.h>
#include "../os/os.h"
extern unsigned long NumCreated;   // number of foreground threads created
extern unsigned long PIDWork;      // current number of PID calculations finished
extern unsigned long FilterWork;   // number of digital filter calculations finished
extern unsigned long NumSamples;   // incremented every ADC sample, in Producer
extern unsigned long DataLost;     // data sent by Producer, but not received by Consumer
extern long MaxJitter;             // largest time jitter between interrupts in usec
#define JITTERSIZE 150
unsigned long const JitterSize=JITTERSIZE;
extern long x[64],y[64];         // input and output arrays for FFT
extern long jitter;                    // time between measured and expected, in us
#ifdef __cplusplus
extern "C" {
#endif
void HardFault_Handler(void);	
#ifdef __cplusplus
}
#endif
enum traceName {
  TRACE_THREAD_0,
  TRACE_THREAD_1,
  TRACE_THREAD_2,
  TRACE_THREAD_3,
  TRACE_THREAD_4,
  TRACE_THREAD_5,
  TRACE_THREAD_6,
  TRACE_THREAD_7,
  TRACE_THREAD_8,
  TRACE_THREAD_9,
  TRACE_THREAD_10,
  TRACE_THREAD_11,
  TRACE_THREAD_12,
  TRACE_MAIN,
  TRACE_ADD_THREAD,
  TRACE_SLEEP,
  TRACE_WAIT,
  TRACE_SIGNAL,
  TRACE_CS,
  TRACE_TX_PUT,
  TRACE_TX_GET,
  TRACE_RX_PUT,
  TRACE_RX_GET,
  TRACE_SYSTICK,
  TRACE_SCHEDULER,
  TRACE_LAUNCH,
  TRACE_KILL,
	NUM_TRACE_ENUM
};

extern const char* traceNameStr[NUM_TRACE_ENUM];

struct trace_t {
  traceName trName;
  int tid;
  unsigned long time;

  void print(void) const{
    printf("%s\t%d : %lu\n", traceNameStr[trName], tid, time);
  }
};


void add_trace(traceName trName);
#endif /*#ifndef __PERF_H__*/
