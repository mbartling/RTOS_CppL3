// PWMDual.c
// Runs on TM4C123
// Use PWM0/PB6 and PWM1/PB7 to generate two square waves with 50% duty cycle
// and 180 degrees out of phase.
// Daniel Valvano
// September 10, 2013

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014
  Program 6.7, section 6.3.2

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
#include "inc/tm4c123gh6pm.h"

#define PC4       (*((volatile uint32_t *)0x40006040))
#define PC5       (*((volatile uint32_t *)0x40006080))
#define PC6       (*((volatile uint32_t *)0x40006100))
#define PC7       (*((volatile uint32_t *)0x40006200))

#define PWM_ENABLE_PWM1EN       0x00000002  // PWM1 Output Enable
#define PWM_ENABLE_PWM0EN       0x00000001  // PWM0 Output Enable
#define PWM_1_CTL_MODE          0x00000002  // Counter Mode
#define PWM_1_CTL_ENABLE        0x00000001  // PWM Block Enable
#define PWM_0_GENA_ACTLOAD_ZERO 0x00000008  // Set the output signal to 0
#define PWM_0_GENA_ACTZERO_ONE  0x00000003  // Set the output signal to 1
#define PWM_1_GENB_ACTCMPAD_ZERO                                              \
                                0x00000030  // Set the output signal to 1
#define PWM_1_GENB_ACTCMPAU_ONE 0x00000080  // Set the output signal to 0
#define PWM_1_GENA_ACTCMPAD_ZERO                                              \
                                0x00000030  // Set the output signal to 1
#define PWM_1_GENA_ACTCMPAU_ONE 0x00000080  // Set the output signal to 0

#define SYSCTL_RCC_USEPWMDIV    0x00100000  // Enable PWM Clock Divisor
#define SYSCTL_RCC_PWMDIV_M     0x000E0000  // PWM Unit Clock Divisor
#define SYSCTL_RCC_PWMDIV_64    0x000A0000  // PWM clock /64
#define SYSCTL_RCGC0_PWM0       0x00100000  // PWM Clock Gating Control
#define SYSCTL_RCGC2_GPIOB      0x00000002  // Port B Clock Gating Control

#define MOVE 1
#define STOP 0
#define FORWARD 2 
#define REVERSE 3
#define LEFTMOTOR 5
#define RIGHTMOTOR 6 
void motorMovement(int motorPosition, int command , int dir) {
    if(motorPosition == LEFTMOTOR) {
        if(command == MOVE)   
        { 
            if(dir == FORWARD)
            { 
                PC4= 0x10;
                PC5 = 0x00;
            }else if (dir == REVERSE) {
                PC5= 0x20;
                PC4 = 0x00;
            }
        } else if(command == STOP){
            PC5= 0x00;
            PC4 = 0x00;
        }
  }else if(motorPosition == RIGHTMOTOR) {
        if(command == MOVE)   
        { 
            if(dir == FORWARD)
            { 
                PC6= 0x00;
                PC7 = 0x80;
            }else if (dir == REVERSE) {
                PC7= 0x00;
                PC6 = 0x40;
            }
        } else if(command == STOP){
            PC6= 0x00;
            PC7 = 0x00;
        }
  }
}    

void PortC_Init(void){ unsigned long volatile delay;
  //SYSCTL_RCGC2_R |= 0x10;       // activate port c
  SYSCTL_RCGCGPIO_R |= 0x04;       
  delay = SYSCTL_RCGC2_R;        
  delay = SYSCTL_RCGC2_R;         
  GPIO_PORTC_DIR_R |= 0xF0;    // make PC7 output
  //GPIO_PORTC_DIR_R &= ~0xF0;   //PC6 is echo
  GPIO_PORTC_AFSEL_R &= ~0xF0;   // disable alt funct on PC6-7
  GPIO_PORTC_DEN_R |= 0xF0;     // enable digital I/O on PC6-7
}

// period is 16-bit number of PWM clock cycles in one period (3<=period)
// PWM clock rate = processor clock rate/SYSCTL_RCC_PWMDIV
//                = BusClock/64 
//                = 3.2 MHz/64 = 50 kHz (in this example)
void PWM0Dual_Init(uint16_t period){
  volatile uint32_t delay;
  SYSCTL_RCGCPWM_R |= 0x02;             // 1) activate PWM0
  SYSCTL_RCGCGPIO_R |= 0x01;            // 2) activate port B
  delay = SYSCTL_RCGCGPIO_R;            // allow time to finish activating
  GPIO_PORTA_AFSEL_R |= 0xC0;           // enable alt funct on PA7-6
  GPIO_PORTA_PCTL_R &= ~0xFF000000;     // configure PA7-6 as PWM0
  GPIO_PORTA_PCTL_R |= 0x44000000;
  GPIO_PORTA_AMSEL_R &= ~0xC0;          // disable analog functionality on PA7-6
  GPIO_PORTA_DEN_R |= 0xC0;             // enable digital I/O on PA7-6
  SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV; // 3) use PWM divider
  SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M; //    clear PWM divider field
  SYSCTL_RCC_R += SYSCTL_RCC_PWMDIV_64; //    configure for /64 divider
  PWM1_1_CTL_R = 0;                     // 4) disable PWM while initializing
  //PWM0, Generator A (PWM0/PB6) goes to 0 when count==reload and 1 when count==0
  PWM1_1_GENA_R = (PWM_1_GENA_ACTCMPAD_ZERO|PWM_1_GENA_ACTCMPAU_ONE);
  //PWM0, Generator B (PWM1/PB7) goes to 0 when count==CMPA counting down and 1 when count==CMPA counting up
  PWM1_1_GENB_R = (PWM_1_GENB_ACTCMPAD_ZERO|PWM_1_GENB_ACTCMPAU_ONE);
  PWM1_1_LOAD_R = (period - 1)/2;       // 5) count from zero to this number and back to zero in (period - 1) cycles
  PWM1_1_CMPA_R = (period - 1)/16;       // 6) count value when PWM1/PB7 toggles
  PWM1_1_CMPB_R = (period - 1)/16;       // 6) count value when PWM1/PB7 toggles
  
  PWM1_1_CTL_R |= (PWM_1_CTL_MODE|PWM_1_CTL_ENABLE);
                                        // enable PWM1-0
  PWM1_ENABLE_R |= (PWM_ENABLE_PWM1EN|PWM_ENABLE_PWM1EN);


  //initializing the PORTC Pins for motor
  PortC_Init();
 PC6= 0x40;
  PC7 = 0x00;
}
// change period
// period is 16-bit number of PWM clock cycles in one period (3<=period)
void PWM0Dual_Period(uint16_t period){
  PWM1_1_LOAD_R = (period - 1)/2;       // 5) count from zero to this number and back to zero in (period - 1) cycles
  PWM1_1_CMPA_R = (period - 1)/4;       // 6) count value when PWM1/PB7 toggles
}
