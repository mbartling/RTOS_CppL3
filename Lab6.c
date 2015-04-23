//*****************************************************************************
//
// Lab5.c - user programs, File system, stream data onto disk
// Jonathan Valvano, March 16, 2011, EE345M
//     You may implement Lab 5 without the oLED display
//*****************************************************************************
// PF1/IDX1 is user input select switch
// PE1/PWM5 is user input down switch 
#include <stdio.h>
#include <string.h>
#include "inc/hw_types.h"
#include "ADC.h"
#include "os.h"
#include "inc/tm4c123gh6pm.h"
#include "Perf.h"
#include "can0.h"
#include "median.h"
#include "PWMDual.h"


//unsigned long NumCreated;   // number of foreground threads created
//unsigned long NumSamples;   // incremented every sample
//unsigned long DataLost;     // data sent by Producer, but not received by Consumer

int Running;                // true while robot is running

#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

#define GPIO_PF0  (*((volatile unsigned long *)0x40025004))
#define GPIO_PF1  (*((volatile unsigned long *)0x40025008))
#define GPIO_PF2  (*((volatile unsigned long *)0x40025010))
#define GPIO_PF3  (*((volatile unsigned long *)0x40025020))
#define GPIO_PG1  (*((volatile unsigned long *)0x40026008))

#define PD6  (*((volatile unsigned long *)0x40007100)) //Echo
#define PD7  (*((volatile unsigned long *)0x40007200)) //Trigger
#define PC6  (*((volatile unsigned long *)0x40006100))
#define PC7  (*((volatile unsigned long *)0x40006200))
#define PA6  (*((volatile unsigned long *)0x40004100))
#define PA7  (*((volatile unsigned long *)0x40004200))
#define PING_L_ID 1
#define PING_R_ID 2
#define IR_0_ID   3
#define IR_1_ID   4
#define IR_2_ID   5
#define IR_3_ID   6

#ifdef __cplusplus
extern "C" {
#endif
//comment the following line to deactivate Systick
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
// void PendSV_Handler(void);
#ifdef __cplusplus
}
#endif

/*----------------------------------------------
SENSOR BOARD CODE
-----------------------------------------------*/

void PortD_Init(void){ unsigned long volatile delay;
  //SYSCTL_RCGC2_R |= 0x10;       // activate port D
  SYSCTL_RCGCGPIO_R |= 0x08;       
  delay = SYSCTL_RCGC2_R;        
  delay = SYSCTL_RCGC2_R;         
  GPIO_PORTD_DIR_R |= 0xC0;    // make PD7 output
	GPIO_PORTD_DIR_R &= ~0x40;   //PD6 is echo
  GPIO_PORTD_AFSEL_R &= ~0xC0;   // disable alt funct on PD6-7
  GPIO_PORTD_DEN_R |= 0xC0;     // enable digital I/O on PD6-7
  //GPIO_PORTD_PCTL_R = ~0x0000FFFF;
  //GPIO_PORTD_AMSEL_R &= ~0x0F;;      // disable analog functionality on PF
}
void PortC_SensorBoard_Init(void){ unsigned long volatile delay;
  //SYSCTL_RCGC2_R |= 0x10;       // activate port c
  SYSCTL_RCGCGPIO_R |= 0x04;       
  delay = SYSCTL_RCGC2_R;        
  delay = SYSCTL_RCGC2_R;         
  GPIO_PORTC_DIR_R |= 0xC0;    // make PC7 output
  GPIO_PORTC_DIR_R &= ~0x40;   //PC6 is echo
  GPIO_PORTC_AFSEL_R &= ~0xC0;   // disable alt funct on PC6-7
  GPIO_PORTC_DEN_R |= 0xC0;     // enable digital I/O on PC6-7

}

void PortA_SensorBoard_Init(void){ unsigned long volatile delay;
  //SYSCTL_RCGC2_R |= 0x10;       // activate port A
  SYSCTL_RCGCGPIO_R |= 0x01;       
  delay = SYSCTL_RCGC2_R;        
  delay = SYSCTL_RCGC2_R;         
  GPIO_PORTA_DIR_R |= 0xC0;    // make PA7 output
  GPIO_PORTA_DIR_R &= ~0x40;   //PA6 is echo
  GPIO_PORTA_AFSEL_R &= ~0xC0;   // disable alt funct on PA6-7
  GPIO_PORTA_DEN_R |= 0xC0;     // enable digital I/O on PA6-7

}
uint32_t Ping1(void){
	uint32_t pingStartTime; 
	uint32_t pingEndTime;
	//Write GPIO_Pin High
	long sr = StartCritical();
  PA7 = 0x80;
	OS_DelayUS(5);
	//Write GPIO Pin Low
	PA7 = 0x00;
	//Could move into a ISR
	//while(gpio_Pin_Low){}
	while(PA6 == 0){}
	pingStartTime = OS_GetUsTime();

	//while(gpio_Pin_High){}
	while(PA6 != 0){}

	pingEndTime = OS_GetUsTime();
	OS_DelayUS(200);
	//Speed of sound in air is approx c = 343 m/s = 340 m/s * 1000m.0m/m * (1s/100000us)
	//Then the ping time is c*dT/2
		EndCritical(sr);
	return ((pingEndTime - pingStartTime) /*>> 1*/)*34300/1000000;
}


uint32_t Ping2(void){
  uint32_t pingStartTime; 
  uint32_t pingEndTime;
  //Write GPIO_Pin High
  PC7 = 0x80;
  OS_DelayUS(5);
  //Write GPIO Pin Low
  PC7 = 0x00;
  //Could move into a ISR
  //while(gpio_Pin_Low){}
  while(PC6 == 0){}
  pingStartTime = OS_GetUsTime();

  //while(gpio_Pin_High){}
  while(PC6 != 0){}

  pingEndTime = OS_GetUsTime();
  OS_DelayUS(200);
  //Speed of sound in air is approx c = 343 m/s = 340 m/s * 1000m.0m/m * (1s/100000us)
  //Then the ping time is c*dT/2
  return ((pingEndTime - pingStartTime) /*>> 1*/)*34300/1000000;
}

void PingR(void){
  

  uint32_t distance;
  CanMessage_t msg;
	uint8_t byteMe[5];

  //uint32_t distance_buff[4];
  unsigned long id = OS_Id();
  uint32_t i = 0;
  while(1){
    distance = 0;
    for(i = 0; i < 4; i++){
      distance += Ping1();
    }
    distance = distance >> 2;
    //XmtData = (uint8_t *) &distance;
    msg.mId = PING_R_ID;
    msg.data = distance;
		CanMessage2Buff(&msg, byteMe);
    CAN0_SendData(byteMe);

  }
  OS_Kill();
}

void PingL(void){
  
  uint32_t distance;
  CanMessage_t msg;
	uint8_t byteMe[5];

  uint32_t i = 0;
  while(1){
    distance = 0;
    for(i = 0; i < 4; i++){
      distance += Ping2();
    }
    distance = distance >> 2;
    //XmtData = (uint8_t *) &distance;
    msg.mId = PING_L_ID;
    msg.data = distance;
		CanMessage2Buff(&msg, byteMe);

    //CAN0_SendData(CAST_CAN_2_UINT8P(msg));
		CAN0_SendData(byteMe);


  }
  OS_Kill();
}

#define buffSIZE 32
uint16_t Res_buffer0[buffSIZE];
void IR0(void){


  uint32_t SendData;
  CanMessage_t msg;
	uint8_t byteMe[5];
  while(1){
		SendData = 0;
    ADC_Collect0(0, 100, Res_buffer0, 2*buffSIZE); //128, to bring down sampling rate from 100 to 50
    while(ADC_Status(0)){}
    for(int i = 0; i < buffSIZE-MEDIAN_FILTER_SIZE; i++){
      SendData += median_filt(&Res_buffer0[i]);
    }
    SendData = SendData/(buffSIZE-MEDIAN_FILTER_SIZE);
    msg.mId = IR_0_ID;
    msg.data = SendData;
		CanMessage2Buff(&msg, byteMe);
    CAN0_SendData(byteMe);
  }
  OS_Kill();
}

uint16_t Res_buffer1[64];
void IR1(void){


  uint32_t SendData;
  CanMessage_t msg;
  while(1){
		SendData = 0;
    ADC_Collect1(1, 100, Res_buffer1, 128); //128, to bring down sampling rate from 100 to 50
    while(ADC_Status(1)){}
    for(int i = 0; i < 64-MEDIAN_FILTER_SIZE; i++){
      SendData += median_filt(&Res_buffer1[i]);
    }
    SendData = SendData/(64-MEDIAN_FILTER_SIZE);
    msg.mId = IR_1_ID;
    msg.data = SendData;
    CAN0_SendData(CAST_CAN_2_UINT8P(msg));
  }
  OS_Kill();
}

uint16_t Res_buffer2[64];
void IR2(void){

  uint32_t SendData;
  CanMessage_t msg;
  while(1){
		SendData = 0;
    ADC_Collect2(2, 100, Res_buffer2, 128); //128, to bring down sampling rate from 100 to 50
    while(ADC_Status(2)){}
    for(int i = 0; i < 64-MEDIAN_FILTER_SIZE; i++){
      SendData += median_filt(&Res_buffer2[i]);
    }
    SendData = SendData/(64-MEDIAN_FILTER_SIZE);
    msg.mId = IR_2_ID;
    msg.data = SendData;
    CAN0_SendData(CAST_CAN_2_UINT8P(msg));
  }
  OS_Kill();
}

uint16_t Res_buffer3[64];
void IR3(void){

  uint32_t SendData;
  CanMessage_t msg;
  while(1){
		SendData = 0;
    ADC_Collect3(3, 100, Res_buffer3, 128); //128, to bring down sampling rate from 100 to 50
    while(ADC_Status(3)){}
    for(int i = 0; i < 64-MEDIAN_FILTER_SIZE; i++){
      SendData += median_filt(&Res_buffer3[i]);
    }
    SendData = SendData/(64-MEDIAN_FILTER_SIZE);
    msg.mId = IR_3_ID;
    msg.data = SendData;
    CAN0_SendData(CAST_CAN_2_UINT8P(msg));
  }
  OS_Kill();
}


/*----------------------------------------------
MOTOR BOARD CODE
-----------------------------------------------*/


#define LEDS      (*((volatile uint32_t *)0x40025038))

#define RED       0x02
#define BLUE      0x04
#define GREEN     0x08

void PortF_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x20;       // activate port F
  while((SYSCTL_PRGPIO_R&0x0020) == 0){};// ready?
  GPIO_PORTF_DIR_R |= 0x0E;        // make PF3-1 output (PF3-1 built-in LEDs)
  GPIO_PORTF_AFSEL_R &= ~0x0E;     // disable alt funct on PF3-1
  GPIO_PORTF_DEN_R |= 0x0E;        // enable digital I/O on PF3-1
                                   // configure PF3-1 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFF000F)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;          // disable analog functionality on PF
  LEDS = 0;		// turn all LEDs off
}


/**
 * CAN Receiver Code
 */
uint32_t RcvCount=0;

uint32_t PingRVal;
uint32_t PingLVal;
uint32_t IR0Val;
uint32_t IR1Val;
uint32_t IR2Val;
uint32_t IR3Val;

void CAN_Listener(void){
  uint8_t RcvData[5];
//  uint32_t rxDat;
  CanMessage_t rxDat;
  while(1){
    if(CAN0_GetMailNonBlock(RcvData)){
			Buff2CanMessage(&rxDat, RcvData);
      RcvCount++;
      //rxDat = CAST_UINT8_2_CAN(RcvData);
			switch(rxDat.mId){
				case PING_L_ID:
					PingLVal = rxDat.data;
					break;
				
				case PING_R_ID:
					PingRVal = rxDat.data;
					break;
				case IR_0_ID: 
					IR0Val = rxDat.data;
					if(IR0Val > 1100){
						LEDS ^= BLUE; 
					}
					else if(IR0Val > 600){
						LEDS ^= RED; 
					}
					else{
						LEDS ^= GREEN;
					}
					break;
				case IR_1_ID: 
					IR1Val = rxDat.data;
					break;
				case IR_2_ID: 
					IR2Val = rxDat.data;
					break;
				case IR_3_ID: 
					IR3Val = rxDat.data;
					break;
				default:
					break;
			} //End Switch
    } // End if CAN0_GetMail
  }// End While 
}//End CAN_Listener

void Controller(void){
	while(1){
		/*if(IR0Val>1088){//Too close, stop
			motorMovement(LEFTMOTOR, STOP, FORWARD);
			motorMovement(RIGHTMOTOR, MOVE, REVERSE);
		} else*/
		if(PingRVal > 20){
			int i=0;
			
			while(i<1){
			  motorMovement(LEFTMOTOR, MOVE, FORWARD);
			  motorMovement(RIGHTMOTOR, STOP, REVERSE);
				OS_Sleep(50);
				motorMovement(LEFTMOTOR, MOVE, FORWARD);
			  motorMovement(RIGHTMOTOR, MOVE, FORWARD);
				OS_Sleep(50);
				i++;			
			}
		}
		else{ //Too close, stop
			motorMovement(LEFTMOTOR, MOVE, FORWARD);
			motorMovement(RIGHTMOTOR, MOVE, FORWARD);
		}
		OS_Sleep(10);
	}
	OS_Kill();
}



#if SENSOR_BOARD
int main(void){   // testmain1
  OS_Init();           // initialize, disable interrupts
  PortD_Init();
	PortC_SensorBoard_Init();
	PortA_SensorBoard_Init();
  PD7 = 0x00;
	PC7 = 0x00;
	PA7 = 0x00;
  CAN0_Open();

//*******attach background tasks***********
//  OS_AddPeriodicThread(&disk_timerproc,10*TIME_1MS,0);   // time out routines for disk
//  OS_AddButtonTask(&RunTest,2);
  
  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&PingR, 128, 1);
  NumCreated += OS_AddThread(&IR0, 128, 1);
//  NumCreated += OS_AddThread(&IR1, 128, 1);
//  NumCreated += OS_AddThread(&IR2, 128, 1);
//  NumCreated += OS_AddThread(&IR3, 128, 1);
  OS_Launch(10*TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}

#else

int main(void){   // testmain1
  OS_Init();           // initialize, disable interrupts
	PWM0Dual_Init(60000);            // initialize PWM1-0, 0.8333 Hz, 1 RPM
  PWM0Dual_Period(4000);           // 12.50 Hz, 15 RPM
  PortF_Init();
  CAN0_Open();


  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&CAN_Listener, 128, 2);
  NumCreated += OS_AddThread(&Controller, 128, 1);
  OS_Launch(10*TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}

#endif
