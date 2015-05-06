// UART.c
// Runs on LM4F120/TM4C123
// Use UART0 to implement bidirectional data transfer to and from a
// computer running HyperTerminal.  This time, interrupts and FIFOs
// are used.
// Daniel Valvano
// September 11, 2013
// Modified by EE345L students Charlie Gough && Matt Hawk
// Modified by EE345M students Agustinus Darmawan && Mingjie Qiu

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014
   Program 5.11 Section 5.6, Program 3.10

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

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1
#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
//#include <stdio.h>

#include "FIFO.hpp"
#include "UART0.h"
#include "priority.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "inc/hw_memmap.h"

#ifdef __cplusplus
extern "C" {
#endif

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
void UART0_Handler(void);
#ifdef __cplusplus
}
#endif

#define FIFOSIZE  1024 // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1         // return value on success
#define FIFOFAIL    0         // return value on failure
                              // create index implementation FIFO (see FIFO.h)
// AddIndexFifo(Rx0, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)
// AddIndexFifo(Tx0, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)
//FifoP_SP2MC<char, FIFOSIZE> Rx0Fifo;
//FifoP_SP2MC<char, FIFOSIZE> Rx0Fifo;
//FifoP_MP2SC<char, FIFOSIZE> Tx0Fifo;
//
FifoP_loose<char, FIFOSIZE> Rx0Fifo;
FifoP_loose<char, FIFOSIZE> Tx0Fifo;

// FifoP<char, FIFOSIZE> Tx0Fifo;

// Initialize UART0
// Baud rate is 115200 bits/sec
void UART0_Init(void){
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);

  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE | UART_CONFIG_WLEN_8);	
}


// input ASCII character from UART
// spin if RxFifo is empty
char UART0_InChar(void){
  char letter;
  letter = UARTCharGet(UART0_BASE);
	if(letter == '\r') return('\n');
  return(letter );
}
// output ASCII character to UART
// spin if TxFifo is full
void UART0_OutChar(char data){
  UARTCharPut(UART0_BASE, data);
}
// at least one of three things has happened:
// hardware TX FIFO goes from 3 to 2 or less items
// hardware RX FIFO goes from 1 to 2 or more items
// UART receiver has timed out

