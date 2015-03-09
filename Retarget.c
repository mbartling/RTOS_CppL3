/*----------------------------------------------------------------------------
 * Name:    Retarget.c
 * Purpose: 'Retarget' layer for target-dependent low level functions
 * Note(s):
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2011 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <rt_misc.h>
#include "UART0.h"

//#pragma import(__use_no_semihosting_swi)

#ifdef __cplusplus
namespace std {
extern "C" {
#endif


struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;
FILE __stderr;

// int fputc(int ch, FILE *f) {
//   UART0_OutChar(ch);
//   return (1);
// }


// int fgetc(FILE *f) {
//   return (UART0_InChar());
// }


// int ferror(FILE *f) {
//   /* Your implementation of ferror */
//   return EOF;
// }


// void _ttywrch(int c) {
//   UART0_OutChar(c);
// }

// Overriding fputc and fgetc for stdlib compatability
// Taken from UART.c in Scanf_UART_4C123

// Print a character to UART.
int fputc(int ch, FILE *f){
//  if((ch == 10) || (ch == 13) || (ch == 27)){
//     UART0_OutChar(13);
//     UART0_OutChar(10);
//     return 1;
//   }
  UART0_OutChar(ch);
  return 1;
}
// void _ttywrch(int ch){
//   UART0_OutChar(ch);
// }

// Get input from UART, echo
int fgetc (FILE *f){
  char ch = UART0_InChar();  // receive from keyboard
//  UART0_OutChar(ch);            // echo
  return ch;
}
// Function called when file error occurs.
int ferror(FILE *f){
  /* Your implementation of ferror */
  return EOF;
}

void _sys_exit(int return_code) {
label:  goto label;  /* endless loop */
}

#ifdef __cplusplus
}
}
#endif

