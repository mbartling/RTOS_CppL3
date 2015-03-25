#ifndef __ST7735_DEBUG_H__
#define __ST7735_DEBUG_H__

#include "debug_settings.h"

#if DEBUG_ST7735 
#include <stdio.h> 
#define DEBUG_ST7735_PRINTF(fmt, ... ) printf( "[ST7735] %s:%d:%s() >> " fmt, \
        __FILE__, __LINE__, __func__, __VA_ARGS__ ) 
#else 
#define DEBUG_ST7735_PRINTF(fmt, ... ) /*DUMMY*/ 
#endif /* DEBUG_ST7735 */

#endif /*__ST7735_DEBUG_H__*/
