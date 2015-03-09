#ifndef __UART0_DEBUG_H__
#define __UART0_DEBUG_H__

#include "debug_settings.h"

#if DEBUG_UART0 
#include <stdio.h> 
#define DEBUG_UART0_PRINTF(fmt, ... ) printf( "[UART0] %s:%d:%s() >> " fmt, \
        __FILE__, __LINE__, __func__, __VA_ARGS__ ) 
#else 
#define DEBUG_UART0_PRINTF(fmt, ... ) /*DUMMY*/ 
#endif /* DEBUG_UART0 */

#endif /*__UART0_DEBUG_H__*/
