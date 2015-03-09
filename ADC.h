// ADC.h
// Runs on LM4F120/TM4C123
// Basic ADC driver. Includes support for selecting ADC0-11
// Michael Bartling
// January 2015

#ifndef __ADC_H__
#define __ADC_H__

/**
 * @brief Set up ADC on specified channel number
 * @details The parameters are default as follows
 * Timer0A: enabled
 * Mode: 32-bit, down counting
 * One-shot or periodic: periodic
 * Interval value: programmable using 32-bit period
 * Sample time is busPeriod*period
 * sample rate: <=125,000 samples/second
 * 
 * @param channelNum the desired ADC channel
 * @return 1 if successful, -1 for device driver error
 * likely indicating driver already configured. 
 */
int ADC_Open(unsigned int channelNum);

/**
 * @brief ADC_In gets one sample from the current ADC driver
 * @details Retrieve a 10 bit scaled value from the ADC driver
 * Must run ADC_Open before calling ADC_In, else returns error codes
 * @return 10 bit scaled sample from configured ADC. Error Codes are 
 * indicated by masking with 0xFC00. 0xFC00 denotes device not initialized.
 * Other error codes are reserved.
 */

unsigned short ADC_In(void);

/**
 * @brief Collect a sequence of ADC values
 * 
 * @param channelNum Channel to configure
 * @param fs Sample Frequency (Hz) between 100 and 10kHz
 * @param buffer Array to buffer data, does not bounds check buffer 
 * @param numberOfSamples number of samples to record
 * 	must be even (multiple of two)
 * @return 1 if initialization successful
 * 
 * To be safe, buffer must be global buffer
 */
//int ADC_Collect(unsigned int channelNum, unsigned int fs, unsigned short buffer[], unsigned int numberOfSamples);

int ADC_Collect(unsigned int channelNum, unsigned int fs, void (*task)(unsigned long));

/**
 * @brief returns 0 when ADC_collect finishes
 * @return 0 if ADC_collect is complete, , else returns the remaining
 * number of samples to record
 */
int ADC_Status(void);

#endif /*__ADC_H__*/
