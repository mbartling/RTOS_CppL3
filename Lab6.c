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

#define SENSOR_BOARD 1
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

#define PD6  (*((volatile unsigned long *)0x40023100)) //Echo
#define PD7  (*((volatile unsigned long *)0x40023200)) //Trigger
#define PC6  (*((volatile unsigned long *)0x40022100))
#define PC7  (*((volatile unsigned long *)0x40022200))

#define PING_L_ID 1
#define PING_R_ID 2

void PortD_Init(void){ unsigned long volatile delay;
  //SYSCTL_RCGC2_R |= 0x10;       // activate port D
  SYSCTL_RCGCGPIO_R |= 0x08;       
  delay = SYSCTL_RCGC2_R;        
  delay = SYSCTL_RCGC2_R;         
  GPIO_PORTD_DIR_R |= 0x30;    // make PD7 output
	GPIO_PORTD_DIR_R &= ~0x10;   //PD6 is echo
  GPIO_PORTD_AFSEL_R &= ~0x30;   // disable alt funct on PD6-7
  GPIO_PORTD_DEN_R |= 0x30;     // enable digital I/O on PD6-7
  //GPIO_PORTD_PCTL_R = ~0x0000FFFF;
  //GPIO_PORTD_AMSEL_R &= ~0x0F;;      // disable analog functionality on PF
}
void PortC_Init(void){ unsigned long volatile delay;
  //SYSCTL_RCGC2_R |= 0x10;       // activate port c
  SYSCTL_RCGCGPIO_R |= 0x04;       
  delay = SYSCTL_RCGC2_R;        
  delay = SYSCTL_RCGC2_R;         
  GPIO_PORTC_DIR_R |= 0x30;    // make PC7 output
  GPIO_PORTC_DIR_R &= ~0x10;   //PC6 is echo
  GPIO_PORTC_AFSEL_R &= ~0x30;   // disable alt funct on PC6-7
  GPIO_PORTC_DEN_R |= 0x30;     // enable digital I/O on PC6-7

}

uint32_t Ping1(void){
	uint32_t pingStartTime; 
	uint32_t pingEndTime;
	//Write GPIO_Pin High
  PD7 = 0x80;
	OS_DelayUS(5);
	//Write GPIO Pin Low
	PD7 = 0x00;
	//Could move into a ISR
	//while(gpio_Pin_Low){}
	while(PD6 == 0){}
	pingStartTime = OS_GetUsTime();

	//while(gpio_Pin_High){}
	while(PD6 != 0){}

	pingEndTime = OS_GetUsTime();
	OS_DelayUS(200);
	//Speed of sound in air is approx c = 343 m/s = 340 m/s * 1000m.0m/m * (1s/100000us)
	//Then the ping time is c*dT/2
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
    CAN0_SendData(CAST_CAN_2_UINT8P(msg));

  }
  OS_Kill();
}

void PingL(void){
  
  uint32_t distance;
  CanMessage_t msg;

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
    CAN0_SendData(CAST_CAN_2_UINT8P(msg));

  }
  OS_Kill();
}


uint32_t RcvCount=0;

void CAN_Listener(void){
  uint8_t RcvData[5];
//  uint32_t rxDat;
  CanMessage_t rxDat;
  while(1){
    if(CAN0_GetMailNonBlock(CAST_CAN_2_UINT8P(RcvData))){
      RcvCount++;
      //rxDat = CAST_UINT8_2_CAN(RcvData);
    }
  } 
}

#ifdef SENSOR_BOARD
int main(void){   // testmain1
  OS_Init();           // initialize, disable interrupts
  PortD_Init();
  PD7 = 0x00;
  CAN0_Open();

//*******attach background tasks***********
//  OS_AddPeriodicThread(&disk_timerproc,10*TIME_1MS,0);   // time out routines for disk
//  OS_AddButtonTask(&RunTest,2);
  
  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&PingR, 128, 1);

  OS_Launch(10*TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}

#else

int main(void){   // testmain1
  OS_Init();           // initialize, disable interrupts
  PortD_Init();
  CAN0_Open();


  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&CAN_Listener, 128, 1);

  OS_Launch(10*TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}

#endif
