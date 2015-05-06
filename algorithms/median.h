#ifndef __MEDIAN_H__
#define __MEDIAN_H__
#include <stdlib.h>
#include <stdint.h>

#define numInts 5
#define MEDIAN_FILTER_SIZE numInts
// uint32_t median(uint32_t* buffer, int numInts);
uint32_t median_filt(uint16_t* buffer);
#endif /*__MEDIAN_H__*/
