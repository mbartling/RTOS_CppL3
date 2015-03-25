
#ifndef __DEBUG_SETTINGS_H__
#define __DEBUG_SETTINGS_H__

//Top Definitions
#define DEBUG_ENABLED  0
#define DEBUG_DISABLED 0




// There are several modes of debug available:
// Log to console
// Blink LED
//
#define DEBUG_MODE_2_CONSOLE 1
#define DEBUG_MODE_2_LED     2

#define DEBUG_UART0      DEBUG_ENABLED
#define DEBUG_UART0_MODE DEBUG_LOG_2_CONSOLE


#define DEBUG_ST7735      DEBUG_ENABLED
#define DEBUG_ST7735_MODE DEBUG_LOG_2_CONSOLE



#endif /*__DEBUG_SETTINGS_H__*/
