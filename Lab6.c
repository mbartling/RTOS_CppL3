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
#include "Mailbox.hpp"

#define TooClose 1600
#define Close 1200

#define TooFar 600
#define OKRangeMax 1200
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



#define LEDS      (*((volatile uint32_t *)0x40025038))

#define RED       0x02
#define BLUE      0x04
#define GREEN     0x08
#define WHITE     0x0F

Sema4Type ADC_Collection;

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
void GPIOPortA_Handler(void);
#ifdef __cplusplus
}
#endif

/*----------------------------------------------
SENSOR BOARD CODE
-----------------------------------------------*/
Mailbox<uint32_t> Ping1Mail;
Mailbox<uint32_t> Ping2Mail;

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
  GPIO_PORTD_IS_R &= ~(1 << 6);   // PD6 is edge-sensitive
  GPIO_PORTD_IBE_R |= (1 << 6);   // PD6 is sensitive to both edges
  GPIO_PORTD_IEV_R |= (1 << 6);   // PD6 rising edge event (Dont care)
  GPIO_PORTD_ICR_R = (1 << 6); //Clear the flag
  GPIO_PORTD_IM_R |= (1 << 6);    // enable interrupt on PD6

  //Priority Level 3
  NVIC_PRI0_R = (NVIC_PRI0_R&0x00FFFFFF)| (3 << 29);
  NVIC_EN0_R = 1 << 3;
 
}

void GPIOPortA_Handler(void){
  GPIO_PORTA_ICR_R = (1 << 6);    // acknowledge flag6
  //SW2 = 1;
  static uint32_t pingStartTime; 
  static uint32_t pingEndTime;

  if(PA6 != 0)  pingStartTime = OS_GetUsTime();
  else if(PA6 == 0){
    pingEndTime = OS_GetUsTime();
    uint32_t delta =  ((pingEndTime - pingStartTime) /*>> 1*/)*34300/1000000;
    if(pingEndTime > pingStartTime){
      Ping1Mail.Send(delta);
    }
  }
}
void GPIOPortC_Handler(void){
  GPIO_PORTC_ICR_R = (1 << 6);    // acknowledge flag6
  //SW2 = 1;
  static uint32_t pingStartTime; 
  static uint32_t pingEndTime;

  if(PA6 != 0)  pingStartTime = OS_GetUsTime();
  else if(PA6 == 0){
    pingEndTime = OS_GetUsTime();
    uint32_t delta =  ((pingEndTime - pingStartTime) /*>> 1*/)*34300/1000000;
    if(pingEndTime > pingStartTime){
      Ping2Mail.Send(delta);
    }
  }
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

  GPIO_PORTC_IS_R &= ~(1 << 6);   // PA6 is edge-sensitive
  GPIO_PORTC_IBE_R |= (1 << 6);   // PA6 is sensitive to both edges
  GPIO_PORTC_IEV_R |= (1 << 6);   // PA6 rising edge event (Dont care)
  GPIO_PORTC_ICR_R = (1 << 6); //Clear the flag
  GPIO_PORTC_IM_R |= (1 << 6);    // enable interrupt on PA6

  //Priority Level 3
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFF00FFFFF)| (3 << 21);
  NVIC_EN0_R = 1 << 2;
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

  GPIO_PORTA_IS_R &= ~(1 << 6);   // PA6 is edge-sensitive
  GPIO_PORTA_IBE_R |= (1 << 6);   // PA6 is sensitive to both edges
  GPIO_PORTA_IEV_R |= (1 << 6);   // PA6 rising edge event (Dont care)
  GPIO_PORTA_ICR_R = (1 << 6); //Clear the flag
  GPIO_PORTA_IM_R |= (1 << 6);    // enable interrupt on PA6

  //Priority Level 3
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFFFFFFF00)| (3 << 5);
  NVIC_EN0_R = 1 << 0;
}
uint32_t Ping1(void){
	// uint32_t pingStartTime; 
	// uint32_t pingEndTime;
	//Write GPIO_Pin High
	// long sr = StartCritical();
  PA7 = 0x80;
	OS_DelayUS(5);
	//Write GPIO Pin Low
	PA7 = 0x00;
	//Could move into a ISR
	//while(gpio_Pin_Low){}
	// while(PA6 == 0){}
	// pingStartTime = OS_GetUsTime();

	// //while(gpio_Pin_High){}
	// while(PA6 != 0){}

	// pingEndTime = OS_GetUsTime();
	// OS_DelayUS(200);
	// //Speed of sound in air is approx c = 343 m/s = 340 m/s * 1000m.0m/m * (1s/100000us)
	// //Then the ping time is c*dT/2
	// 	EndCritical(sr);
	// return ((pingEndTime - pingStartTime) /*>> 1*/)*34300/1000000;

  uint32_t delta;
  Ping1Mail.Receive(delta);
  return delta;
}


uint32_t Ping2(void){
  // uint32_t pingStartTime; 
  // uint32_t pingEndTime;
  //Write GPIO_Pin High
  PC7 = 0x80;
  OS_DelayUS(5);
  //Write GPIO Pin Low
  PC7 = 0x00;
  //Could move into a ISR
  //while(gpio_Pin_Low){}
  // while(PC6 == 0){}
  // pingStartTime = OS_GetUsTime();

  // //while(gpio_Pin_High){}
  // while(PC6 != 0){}

  // pingEndTime = OS_GetUsTime();
  // OS_DelayUS(200);
  // //Speed of sound in air is approx c = 343 m/s = 340 m/s * 1000m.0m/m * (1s/100000us)
  // //Then the ping time is c*dT/2
  // return ((pingEndTime - pingStartTime) /*>> 1*/)*34300/1000000;
  uint32_t delta;
  Ping2Mail.Receive(delta);
  return delta;
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


#define buffSIZE 16
uint16_t Res_buffer0[buffSIZE];
void IR0(void){
	
  uint32_t SendData;
  CanMessage_t msg;
	uint8_t byteMe[5];
  ADC_init_channel(0, 100);
	while(1){
		SendData = 0;
		OS_Wait(&ADC_Collection);
    ADC_Collect(0, 100, Res_buffer0, 2*buffSIZE); //128, to bring down sampling rate from 100 to 50
    while(ADC_Status(0)){}
		OS_Signal(&ADC_Collection);
    for(int i = 0; i < buffSIZE-MEDIAN_FILTER_SIZE; i++){
      SendData += median_filt(&Res_buffer0[i]);
    }
    SendData = SendData/(buffSIZE-MEDIAN_FILTER_SIZE);
    msg.mId = IR_0_ID;
    msg.data = SendData;
		CanMessage2Buff(&msg, byteMe);
    CAN0_SendData(byteMe);
		LEDS ^= GREEN;
  }
  OS_Kill();
}

uint16_t Res_buffer1[64];
void IR1(void){
	
  uint32_t SendData;
  CanMessage_t msg;
	uint8_t byteMe[5];
  ADC_init_channel(1, 100);
	while(1){
		SendData = 0;
		OS_Wait(&ADC_Collection);
    ADC_Collect(1, 100, Res_buffer1, 2*buffSIZE); //128, to bring down sampling rate from 100 to 50
    while(ADC_Status(1)){}
		OS_Signal(&ADC_Collection);
    for(int i = 0; i < buffSIZE-MEDIAN_FILTER_SIZE; i++){
      SendData += median_filt(&Res_buffer1[i]);
    }
    SendData = SendData/(buffSIZE-MEDIAN_FILTER_SIZE);
    msg.mId = IR_1_ID;
    msg.data = SendData;
		CanMessage2Buff(&msg, byteMe);
    CAN0_SendData(byteMe);
		LEDS ^= RED;
  }
  OS_Kill();
}

uint16_t Res_buffer2[64];
void IR2(void){
		
  uint32_t SendData;
  CanMessage_t msg;
	uint8_t byteMe[5];
  ADC_init_channel(2, 100);
	while(1){
		SendData = 0;
		OS_Wait(&ADC_Collection);
    ADC_Collect(2, 100, Res_buffer2, 2*buffSIZE); //128, to bring down sampling rate from 100 to 50
    while(ADC_Status(2)){}
    OS_Signal(&ADC_Collection);
		for(int i = 0; i < buffSIZE-MEDIAN_FILTER_SIZE; i++){
      SendData += median_filt(&Res_buffer2[i]);
    }
    SendData = SendData/(buffSIZE-MEDIAN_FILTER_SIZE);
    msg.mId = IR_2_ID;
    msg.data = SendData;
		CanMessage2Buff(&msg, byteMe);
    CAN0_SendData(byteMe);
		LEDS ^= BLUE;
  }
  OS_Kill();
}

uint16_t Res_buffer3[64];
void IR3(void){
  uint32_t SendData;
  CanMessage_t msg;
	uint8_t byteMe[5];
  ADC_init_channel(4, 100);
	while(1){
		SendData = 0;
		OS_Wait(&ADC_Collection);
    ADC_Collect(4, 100, Res_buffer3, 2*buffSIZE); //128, to bring down sampling rate from 100 to 50
    while(ADC_Status(3)){}
		OS_Signal(&ADC_Collection);
    for(int i = 0; i < buffSIZE-MEDIAN_FILTER_SIZE; i++){
      SendData += median_filt(&Res_buffer3[i]);
    }
    SendData = SendData/(buffSIZE-MEDIAN_FILTER_SIZE);
    msg.mId = IR_3_ID;
    msg.data = SendData;
		CanMessage2Buff(&msg, byteMe);
    CAN0_SendData(byteMe);
		LEDS ^= WHITE;
  }
  OS_Kill();
}


/*----------------------------------------------
MOTOR BOARD CODE
-----------------------------------------------*/

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
uint32_t IR0Val = 4096;
uint32_t IR1Val= 0;
uint32_t IR2Val= 4096;
uint32_t IR3Val= 4096;

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
                    //LEDS = BLUE;
					break;
				case IR_1_ID: 
					IR1Val = rxDat.data;
                    LEDS ^= GREEN;
					break;
				case IR_2_ID: 
					IR2Val = rxDat.data;
                    LEDS = RED;
					break;
				case IR_3_ID: 
					IR3Val = rxDat.data;
                    LEDS = WHITE;
					break;
				default:
					LEDS = BLUE;
                    break;
			} //End Switch
    } // End if CAN0_GetMail
  }// End While 
}//End CAN_Listener
//int turned_left = 0;
//void Controller(void){
//	int turned_left = 0;
//    int turned_right = 0;
//
//    while(1){
//		  
//			int difference = IR3Val-IR2Val;
//			int threshold =200;
//		     //right wheel close 
//            if((IR2Val > TooClose))
//            {
//                if(!turned_left){
//                    motorMovement(LEFTMOTOR, STOP, FORWARD,50);
//                    motorMovement(RIGHTMOTOR, MOVE, FORWARD,200);
//                    OS_DelayUS(500);
//                    turned_left = 1;
//                }
//                turned_right = 0;
//                motorMovement(LEFTMOTOR, MOVE, FORWARD,50);
//                motorMovement(RIGHTMOTOR, MOVE, FORWARD,200);
//            }
//            //left too close 
//            else if(IR3Val > TooClose) //IR0 is on Right side and IR1 in front
//            {
//                if(!turned_right){
//                    motorMovement(LEFTMOTOR, MOVE, FORWARD,200);
//                    motorMovement(RIGHTMOTOR, STOP, FORWARD,50);
//                    OS_DelayUS(500);
//                    turned_right = 1;
//                }
//                turned_left = 0;
//                motorMovement(LEFTMOTOR, MOVE, FORWARD,200);
//                motorMovement(RIGHTMOTOR, MOVE, FORWARD,50);
//            }
//            else if((IR2Val > Close && IR2Val < TooClose))
//            {
//                motorMovement(LEFTMOTOR, MOVE, FORWARD,50);
//                motorMovement(RIGHTMOTOR, MOVE, FORWARD,200);
//            }
//            //left too close 
//            else if((IR3Val > Close && IR3Val < TooClose))
//            {
//                motorMovement(LEFTMOTOR, MOVE, FORWARD,200);
//                motorMovement(RIGHTMOTOR, MOVE, FORWARD,50);
//            }
//             
//           //go straight 
//            else 
//            {
//                turned_right = 0;
//                turned_left = 0;
//                motorMovement(LEFTMOTOR, MOVE, FORWARD,200);
//                motorMovement(RIGHTMOTOR, MOVE, FORWARD,200);
//            }
//            OS_Sleep(1);
//    }
//	OS_Kill();
//}

//IR3 right
//IR2 Left
//IR1 center

int turned_left = 0;
//int ref_distance = ;

int RPWMError = 0; //how far Right Wheel is from the OKRangeMin
int LPWMError = 0; //how far left Wheel is from the OKRangeMin
int FPWMError = 0; //how far left Wheel is from the OKRangeMin
//void Controller(void){
//    while(1){
//            
//            RPWMError = IR2Val - OkRangeMin;
//            LPWMError = IR3Val - OkRangeMin;
//		    //right wheel close 
//            //if(IR2Val > OkRangeMin){
//                motorMovement(RIGHTMOTOR, MOVE, FORWARD,200);
//                motorMovement(LEFTMOTOR, MOVE , FORWARD,100);
//            //}
//    }	
//            OS_Kill();
//}

#define leftClose 850
#define rightClose 1050
#define tooClose 1200

int leftMotorSpeed = 0;
int rightMotorSpeed = 0;
#define lowSpeedFixed 80 
#define superLowSpeedFixed 80 
#define highSpeedFixed 200 
float weight = .25; //determines how sharp the turn is by tunning the difference
float baseSpeed = 0;
void Controller(void){
    while(1){
            RPWMError = IR2Val - rightClose;
            LPWMError = IR3Val - leftClose;
            FPWMError = IR1Val - tooClose;
            if(IR1Val > tooClose){
                //LEDS = RED;
                if(IR2Val > IR3Val){ //Turn left
                    baseSpeed = superLowSpeedFixed;
                    //leftMotorSpeed = superLowSpeed; 
                    rightMotorSpeed = superLowSpeedFixed+ (2*weight)*(FPWMError);
                    if(rightMotorSpeed > highSpeedFixed)
                        rightMotorSpeed = highSpeedFixed;
                    //rightMotorSpeed = highSpeedFixed - (highSpeedFixed/(weight*RPWMError));
                    motorMovement(LEFTMOTOR, STOP, FORWARD,leftMotorSpeed);
                    motorMovement(RIGHTMOTOR, MOVE, FORWARD,rightMotorSpeed);
                }
                else{ //Turn right
                    //rightMotorSpeed = superLowSpeed; 
                    leftMotorSpeed = superLowSpeedFixed + (2*weight*(FPWMError));
                    if(leftMotorSpeed > highSpeedFixed)
                        leftMotorSpeed = highSpeedFixed;
                    motorMovement(LEFTMOTOR, MOVE, FORWARD,leftMotorSpeed);
                    motorMovement(RIGHTMOTOR, STOP, FORWARD,rightMotorSpeed);
                }
            
            }
            //right wheel close 
            else if(IR2Val > rightClose){
                //LEDS = GREEN;
                baseSpeed = lowSpeedFixed;
                leftMotorSpeed = baseSpeed; 
                rightMotorSpeed = baseSpeed + weight*(RPWMError);
                if(rightMotorSpeed > highSpeedFixed) 
                     rightMotorSpeed = highSpeedFixed;
                motorMovement(LEFTMOTOR, MOVE, FORWARD,leftMotorSpeed);
                motorMovement(RIGHTMOTOR, MOVE, FORWARD,rightMotorSpeed);
            }
            //left too close 
            else if(IR3Val > leftClose){ //IR0 is on Right side and IR1 in front
                //LEDS = WHITE;
                baseSpeed = lowSpeedFixed;
                rightMotorSpeed = baseSpeed; 
                leftMotorSpeed = baseSpeed + weight*(LPWMError);
                 if(leftMotorSpeed > highSpeedFixed) 
                     leftMotorSpeed = highSpeedFixed;
                motorMovement(LEFTMOTOR, MOVE, FORWARD,leftMotorSpeed);
                motorMovement(RIGHTMOTOR, MOVE, FORWARD,rightMotorSpeed);
            } 
            else 
            {
                //LEDS = WHITE; 
                motorMovement(LEFTMOTOR, MOVE, FORWARD,highSpeedFixed);
                motorMovement(RIGHTMOTOR, MOVE, FORWARD,highSpeedFixed);
            }
            OS_Sleep(1);
    }
	OS_Kill();
}




#if SENSOR_BOARD
int main(void){   // testmain1
  OS_Init();           // initialize, disable interrupts
  PortD_Init();
	PortC_SensorBoard_Init();
	PortA_SensorBoard_Init();
  PortF_Init();
	PD7 = 0x00;
	PC7 = 0x00;
	PA7 = 0x00;
  CAN0_Open();

//*******attach background tasks***********
//  OS_AddPeriodicThread(&disk_timerproc,10*TIME_1MS,0);   // time out routines for disk
//  OS_AddButtonTask(&RunTest,2);
  
  NumCreated = 0 ;
	OS_InitSemaphore(&ADC_Collection, 1);
// create initial foreground threads
  NumCreated += OS_AddThread(&PingR, 128, 1);
  NumCreated += OS_AddThread(&IR0, 128, 1);
  NumCreated += OS_AddThread(&IR1, 128, 1);
  NumCreated += OS_AddThread(&IR2, 128, 1);
  NumCreated += OS_AddThread(&IR3, 128, 1);
  OS_Launch(2*TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}

#else

int main(void){   // testmain1
  OS_Init();           // initialize, disable interrupts
//  PWM0Dual_Period(40000);           // 12.50 Hz, 15 RPM
  PortF_Init();
  CAN0_Open();


	PWM0Dual_Init(60000);            // initialize PWM1-0, 0.8333 Hz, 1 RPM
  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&CAN_Listener, 128, 2);
  NumCreated += OS_AddThread(&Controller, 128, 1);
  OS_Launch(TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}

#endif
