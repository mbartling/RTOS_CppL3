// Timer0APWM.c
// Runs on LM4F120/TM4C123
// Use Timer0A in PWM mode to generate a square wave of a given
// period and duty cycle.  Timer0A can use PF0 or PB6.  PB6 is
// used because PF0 is connected to SW2 on the Launchpad.
// NOTE:
// By default PB6 is connected to PD0 by a zero ohm R9, and PB7
// is connected to PD1 by a zero ohm R10.  These jumpers allow
// the LM4F120 Launchpad to work with certain legacy Booster
// Packs and can safely be removed.  The following program
// assumes that R9 and R10 have been removed or it is being used
// as a module in a larger program that knows not to try to use
// both PB6 and PD0 or PB7 and PD1 at the same time.
// Daniel Valvano
// September 12, 2013

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#include <stdint.h>
#include <stdio.h>
#include "inc/tm4c123gh6pm.h"
#include "Timer.h"
#include "Priority.h"
//#define TIMER_MR_TACDIR                  0x1         //determining the direction of the counter (in this case up)
#define TIMER_MR_MR                      0x2 //timer mode (periodic)
#define TIMER_CFG_16_BIT        0x00000004  // 16-bit timer configuration,
                                            // function is controlled by bits
                                            // 1:0 of GPTMTAMR and GPTMTBMR
#define TIMER_TAMR_TAAMS        0x00000008  // GPTM TimerA Alternate Mode
                                            // Select
#define TIMER_TAMR_TAMR_PERIOD  0x00000002  // Periodic Timer mode
#define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
#define TIMER_TAILR_TAILRL_M    0x0000FFFF  // GPTM TimerA Interval Load
                                            // Register Low
#define TIMER_TBILR_TBILRL_M    0x0000FFFF  // GPTM TimerB Interval Load
                                            // Register


// period is number of clock cycles in PWM period ((1/clockfreq) units)
// high is number of clock cycles output is high ((1/clockfreq) units)
// duty cycle = high/period
// assumes that period>high>(approx 3)
//void Timer0A_Init(uint32_t period){
//  volatile unsigned long delay;
//  SYSCTL_RCGCTIMER_R |= 0x01;      // activate timer0
//  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
//  TIMER0_CFG_R = 0x00000000;       //32 bit timer configuration
//  TIMER0_TAMR_R = (TIMER_TAMR_TAMR_PERIOD); //condfgure the timer as a periodic timer
//  TIMER0_TAILR_R = period - 1;       // timer start value (when ever timer gets to zero, it reloads this value)
//  //TIMER0_TAMATCHR_R = period-high-1; // duty cycle = high/period
//  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b (this step is needed to enable the timer (do right before wanting the timer 
//  TIMER0_IMR_R  = 0x1;
//  NVIC_PRI4_R = Timer0APriority;
//  NVIC_EN0_R =  1 <<19;
//}

void Timer2A_Init(uint32_t period){
  volatile unsigned long delay;
  SYSCTL_RCGCTIMER_R |= 0x04;      // activate timer0
  TIMER2_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER2_CFG_R = 0x00000000;       //32 bit timer configuration
   
  TIMER2_TAMR_R = TIMER_TAMR_TAMR_1_SHOT; //condfgure the timer as a one shot timer
  TIMER2_TAILR_R = period - 1;       // timer start value (when ever timer gets to zero, it reloads this value)
  //TIMER0_TAMATCHR_R = period-high-1; // duty cycle = high/period
  TIMER2_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b (this step is needed to enable the timer (do right before wanting the timer 
  TIMER2_IMR_R  = 0x1;
  NVIC_PRI4_R = Timer2APriority;
  NVIC_EN0_R =  1 <<23;
}
void Timer1A_Init(uint32_t period){
  volatile unsigned long delay;
  SYSCTL_RCGCTIMER_R |= 0x02;      // activate timer1    
  TIMER1_CTL_R &= ~TIMER_CTL_TAEN; // disable timer1A during setup
  TIMER1_CFG_R = 0x00000000;       //32 bit timer configuration
  TIMER1_TAMR_R = (TIMER_TAMR_TAMR_PERIOD); //condfgure the timer as a periodic timer
  TIMER1_TAILR_R = period - 1;       // timer start value (when ever timer gets to zero, it reloads this value)
  //TIMER0_TAMATCHR_R = period-high-1; // duty cycle = high/period
  TIMER1_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b (this step is needed to enable the timer (do right before wanting the timer 
  TIMER1_IMR_R  = 0x1;
  //NVIC_PRI5_R = (0x1<<13); 
  NVIC_PRI5_R = Timer1APriority;
  NVIC_EN0_R =  1 <<21;
}


