#include "median.h"

int compare(const void* a, const void* b){
	if ( *(uint32_t *) a  < *(uint32_t *) b) return -1;
	if ( *(uint32_t *) a  == *(uint32_t *) b) return 0;
	if ( *(uint32_t *) a  > *(uint32_t *) b) return 1;
}

uint32_t median(uint32_t* buffer, int numInts){
	qsort(buffer, numInts, sizeof(uint32_t), compare);
	return buffer[numInts/2];
}

uint32_t median_filt(uint32_t* buffer, int numInts){
	uint32_t buff[numInts];
	for(int i = 0; i < numInts; i++){
		buff[i] = buffer[i];
	}
	return median(buff, numInts);
}