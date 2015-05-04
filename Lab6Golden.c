//*****************************************************************************
//
// Lab6M.c - user programs, File system, stream data onto disk
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

#define TooClose 1200
#define TooFar 600
#define OkRangeMin 600
#define OkRangeMax 1200

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
void GPIOPortC_Handler(void);
#ifdef __cplusplus
}
#endif


/*----------------------------------------------
SENSOR BOARD CODE
-----------------------------------------------*/
BGMailbox<uint32_t> Ping1Mail;
BGMailbox<uint32_t> Ping2Mail;

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
  NVIC_PRI0_R = (NVIC_PRI0_R&0x00FFFFFF)| (0 << 29);
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
    uint32_t delta =  ((pingEndTime - pingStartTime) >> 1)*34300/1000000;
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

  if(PC6 != 0)  pingStartTime = OS_GetUsTime();
  else if(PC6 == 0){
    pingEndTime = OS_GetUsTime();
    uint32_t delta =  ((pingEndTime - pingStartTime) >> 1)*34300/1000000;
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
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFF00FFFFF)| (1 << 21);
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
    long sr = StartCritical();
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
        EndCritical(sr);
    // return ((pingEndTime - pingStartTime) /*>> 1*/)*34300/1000000;

  uint32_t delta;
  Ping1Mail.Receive(delta);
  return delta;
}


uint32_t Ping2(void){
  // uint32_t pingStartTime; 
  // uint32_t pingEndTime;
  //Write GPIO_Pin High
    long sr = StartCritical();
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

    EndCritical(sr);

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

uint32_t RcvCount=0;

uint32_t PingRVal;
uint32_t PingLVal;
uint32_t IR0Val = 4096;
uint32_t IR1Val= 4096;
uint32_t IR2Val= 4096;
uint32_t IR3Val= 4096;


#define buffSIZE 8
/*
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
        LEDS = GREEN;
  }
  OS_Kill();
}
*/
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
    //msg.mId = IR_1_ID;
    //msg.data = SendData;
    //CanMessage2Buff(&msg, byteMe);
    //CAN0_SendData(byteMe);
    IR1Val = SendData; 
    //LEDS = RED;
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
//    msg.mId = IR_2_ID;
//    msg.data = SendData;
//        CanMessage2Buff(&msg, byteMe);
//    CAN0_SendData(byteMe);
//    
    IR2Val = SendData; 
    //LEDS = BLUE;
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
//    msg.mId = IR_3_ID;
//    msg.data = SendData;
//        CanMessage2Buff(&msg, byteMe);
//    CAN0_SendData(byteMe);
    IR3Val = SendData; 
    //LEDS = RED;
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
  LEDS = 0;     // turn all LEDs off
}


/**
 * CAN Receiver Code
 */
void CAN_Listener(void){
  uint8_t RcvData[5];
//  uint32_t rxDat;
  CanMessage_t rxDat;
  //LEDS = WHITE;
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
                    //LEDS = GREEN;
                    break;
                case IR_2_ID: 
                    IR2Val = rxDat.data;
                    //LEDS = RED;
                    break;
                case IR_3_ID: 
                    IR3Val = rxDat.data;
                    //LEDS = WHITE;
                   break;
                default:
                    break;
            } //End Switch
    
    } // End if CAN0_GetMail
  }// End While 
}//End CAN_Listener
//int turned_left = 0;
int turningMode = 0;
#define TURNTHRESHR 20
#define TURNTHRESHL 60
uint32_t WobbleR[] = {200, 205, 200, 195};
uint32_t WobbleL[] = {200, 195, 200, 205};
//#define frontClose 1100

int leftMotorSpeed = 0;
int rightMotorSpeed = 0;
#define lowSpeedFixed 30
#define superLowSpeedFixed 30 
#define highestSpeedFixed 300 

//int smoothnessWeightCoefOriginal = 2;
//int leftWeightCoefOriginal = 10;
//int rightWeightCoefOriginal = 10;
//int smoothnessWeightCoef = smoothnessWeightCoefOriginal;
//int leftWeightCoef = leftWeightCoefOriginal;
//int rightWeightCoef = rightWeightCoefOriginal;


//        if (OS_MsTime() > (120000)){
//            LEDS = WHITE; 
//            motorMovement(LEFTMOTOR, STOP, FORWARD,leftMotorSpeed);
//            motorMovement(RIGHTMOTOR,STOP, FORWARD,rightMotorSpeed);
//            OS_Kill(); 
//}

//IR2 rightWheele. Turn left if higher 
//IR3 leftWheele. Turn right if higher 

//parameters to explore 
//
//
#define rightClose 550
#define leftClose 550 
float tooCloseThreadshold = 800;
float frontThreashold  = 600;
float rightThreashold  = 1250;
float leftThreashold  = 1250;
float supercloseFront = 2000;
float supercloseRight = 2300;
float supercloseLeft = 2300;
float KI = .04; 
float lowestP = .1;
///



int red_counter = 0;
float rightCorrection = 0;
float leftCorrection = 0;
int longSideIRValue = 0; //corresponds to the shorter Distance
int shortSideIRValue = 0; //corresponds to the longer Distance
int tooCloseInRightCounter = 0;
int tooCloseInLeftCounter = 0;
float rightError = 0;
float leftError = 0;
float frontError = 0;
bool turnRight = 0;
bool turnLeft = 0;
bool goStraight = 0;
int highestSpeed = 290;
int frontHighestSpeed = 150;
int sideHighestSpeed = 120;
int frontLowestSpeed = 60;
int lowestSpeed = 40;
int frontLowCap = 10;
               
bool tooCloseInFront = 0;
bool tooCloseInLeft = 0;
bool tooCloseInRight = 0;
//float smoothnessWeight = 1; //modifies sharpnees due to the an observed front close barrier
float P = 0;
float highCorrectionCap = highestSpeed *(1 - lowestP); 
float lowCorrectionCap = lowestSpeed - highestSpeed*(lowestP); 

void Controller2(void){
     while(1) {  
//         for (int i =0; i< 100000; i++){
//             LEDS = BLUE; 
//         } 
//          for (int i =0; i< 100000; i++){
//             LEDS = RED; 
//         } 
         motorMovement(LEFTMOTOR, MOVE, FORWARD, 200);
         motorMovement(RIGHTMOTOR, MOVE, FORWARD, 200);
         OS_Sleep(1);
   }
    OS_Kill();
}
void Controller(void){
    while(1){
        
        if((IR1Val > supercloseFront) || (IR2Val > supercloseRight) || (IR3Val > supercloseLeft))
        {
            motorMovement(LEFTMOTOR, MOVE, REVERSE, 200);
            motorMovement(RIGHTMOTOR, MOVE, REVERSE, 200);
//            LEDS = GREEN + RED;
            OS_Sleep(1);
            continue;
        }
        
        //********************* 
        //compare the sides and assign size Values
        //run all three in parallel, since mutually exclusive 
        
        //rightWheele closer 
        if(IR2Val > IR3Val){
            longSideIRValue = IR3Val;
            shortSideIRValue = IR2Val;
            turnLeft = 1; 
            turnRight = 0; 
            goStraight = 0; 
            rightError = 1.0 - ((float)longSideIRValue)/((float)shortSideIRValue);
            leftError =  ((float)longSideIRValue)/((float)shortSideIRValue) - 1.0;
        }
        //left Wheele closer 
        if(IR3Val > IR2Val){
            longSideIRValue = IR2Val;
            shortSideIRValue = IR3Val;
            turnLeft = 0; 
            turnRight = 1; 
            goStraight = 0; 
            leftError = 1.0 - ((float)longSideIRValue)/((float)shortSideIRValue);
            rightError =  ((float)longSideIRValue)/((float)shortSideIRValue) - 1.0;
        }
        //equal distance 
        if(IR3Val == IR2Val){
            rightError = 0; 
            leftError = 0; 
            longSideIRValue = IR2Val;
            shortSideIRValue = IR3Val;
            turnLeft = 0; 
            turnRight = 0; 
            goStraight = 1; 
        }
         
        frontError = (((float)IR1Val - frontThreashold)/ (float)tooCloseThreadshold);
       
        //
        //is front too close
        if(IR1Val >= frontThreashold){
            tooCloseInFront = 1;
            //LEDS = RED;
        }else{
            tooCloseInFront = 0;
        }
        
        if(IR2Val >= rightThreashold + tooCloseInRightCounter){
            tooCloseInRight = 1;
        }else{
            tooCloseInRightCounter = 0;
            tooCloseInRight = 0;
        }
        
        
        if(IR3Val >= leftThreashold + tooCloseInLeftCounter){
            tooCloseInLeft = 1;
        }else{
            tooCloseInLeftCounter = 0;
            tooCloseInLeft = 0;
        }
        
        

        //setting up the P 
        // P = (shortSideIRValue - longSideIRValue)/ (shortSideIRValue + longSideIRValue);
        P = ((float)longSideIRValue)/ ((float)shortSideIRValue );
        
        //setting up the  I
        leftCorrection =  leftCorrection + KI * leftError; 
        rightCorrection =  rightCorrection + KI * rightError; 
        //capping corrections
        if (leftCorrection > highCorrectionCap) 
            leftCorrection = highCorrectionCap; 
        else if ( leftCorrection < lowCorrectionCap) 
            leftCorrection = lowCorrectionCap; 

        if (rightCorrection > highCorrectionCap) 
            rightCorrection = highCorrectionCap; 
        else if ( rightCorrection < lowCorrectionCap) 
            rightCorrection = lowCorrectionCap; 

        
        //navigate based on the sensor inputs so far
        //run all three in paralell, since mutually exclusive 
        //set the motorSpeed 
        if (turnLeft) {
            LEDS = GREEN; 
            rightMotorSpeed = highestSpeed;  //might be set to highestSpeed or Higher
            if (P > 1 ){ //regulate if multipliaction caps out
                leftMotorSpeed = highestSpeed - 40;
                LEDS = WHITE;
            }else{
                leftMotorSpeed = highestSpeed*(P) + rightCorrection;
            }
            
            //capping the speed 
            if(leftMotorSpeed > highestSpeed){ 
                leftMotorSpeed  = highestSpeed; 
                LEDS = WHITE;
            } else if (leftMotorSpeed < lowestSpeed) {
                LEDS = WHITE;
                leftMotorSpeed  = lowestSpeed; 
            }
        }
        if (turnRight) {
            LEDS = BLUE; 
            leftMotorSpeed = highestSpeed;  //might be set to highestSpeed or Higher
            if (P > 1 ){ //if this multiplication causes maxing out the speedg
                rightMotorSpeed = highestSpeed - 40;
            }else{
                rightMotorSpeed = highestSpeed*(P) + leftCorrection;
            }

            //capping the speed 
            if(rightMotorSpeed > highestSpeed){ 
                rightMotorSpeed  = highestSpeed; 
                LEDS = WHITE;
            }  else if (rightMotorSpeed < lowestSpeed) {
                rightMotorSpeed  = lowestSpeed; 
            }

        }
        
        if  (goStraight) {
            leftMotorSpeed = highestSpeed;  //might be set to highestSpeed or Higher
            rightMotorSpeed = highestSpeed ;
        }

        //run the Motors 
        //if too close, stop one wheele
        if(tooCloseInFront == 1) { 
            LEDS = RED; 
            if(turnRight) {
                leftMotorSpeed = frontHighestSpeed; 
                rightMotorSpeed = 60*(1.0 - frontError); 
                if (rightMotorSpeed < frontLowCap) {
                    LEDS= WHITE; 
                    //rightMotorSpeed = 0;
                    motorMovement(LEFTMOTOR, MOVE, FORWARD,leftMotorSpeed);
                    motorMovement(RIGHTMOTOR, STOP, FORWARD,rightMotorSpeed);
                }else {
                    motorMovement(RIGHTMOTOR, MOVE, FORWARD,rightMotorSpeed);
                    motorMovement(LEFTMOTOR, MOVE, FORWARD,leftMotorSpeed);
                }
                //rightMotorSpeed = 1/ 
                //motorMovement(RIGHTMOTOR, STOP, FORWARD,rightMotorSpeed);
            }
            if(turnLeft) {
                //motorMovement(LEFTMOTOR, STOP, FORWARD,leftMotorSpeed);
                rightMotorSpeed = frontHighestSpeed; 
                leftMotorSpeed = 60*(1.0 - frontError); 
                if (leftMotorSpeed < frontLowCap) {
                    LEDS= WHITE; 
                    //leftMotorSpeed = 0;
                    motorMovement(LEFTMOTOR, STOP, FORWARD,leftMotorSpeed);
                    motorMovement(RIGHTMOTOR, MOVE, FORWARD,rightMotorSpeed);
                }else {
                    motorMovement(RIGHTMOTOR, MOVE, FORWARD,rightMotorSpeed);
                    motorMovement(LEFTMOTOR, MOVE, FORWARD,leftMotorSpeed);
                }
            }
        } else if (tooCloseInRight  == 1){
            tooCloseInRightCounter +=30; 
            rightMotorSpeed = sideHighestSpeed; 
            leftMotorSpeed = 60*(1.0 - rightError); 
            if (leftMotorSpeed < frontLowCap) {
                //leftMotorSpeed = 0;
                LEDS= WHITE; 
                motorMovement(LEFTMOTOR, STOP, FORWARD,leftMotorSpeed);
                motorMovement(RIGHTMOTOR, MOVE, FORWARD,rightMotorSpeed);
            }else {
                motorMovement(RIGHTMOTOR, MOVE, FORWARD,rightMotorSpeed);
                motorMovement(LEFTMOTOR, MOVE, FORWARD,leftMotorSpeed);
                LEDS = RED + BLUE;
            }
        } else if (tooCloseInLeft  == 1){
            tooCloseInLeftCounter +=30; 
            leftMotorSpeed = sideHighestSpeed; 
            rightMotorSpeed = 60*(1.0 - leftError); 
            if (leftMotorSpeed < frontLowCap) {
                LEDS= WHITE; 
                //leftMotorSpeed = 0;
                motorMovement(LEFTMOTOR, MOVE, FORWARD,leftMotorSpeed);
                motorMovement(RIGHTMOTOR,STOP , FORWARD,rightMotorSpeed);
            }else {
                motorMovement(RIGHTMOTOR, MOVE, FORWARD,rightMotorSpeed);
                motorMovement(LEFTMOTOR, MOVE, FORWARD,leftMotorSpeed);
                LEDS = RED + BLUE;
            }
        }else{ 
            motorMovement(LEFTMOTOR, MOVE, FORWARD,leftMotorSpeed);
            motorMovement(RIGHTMOTOR, MOVE, FORWARD,rightMotorSpeed);
        }

        //go to sleep for a while  
        OS_Sleep(1);
    }
    OS_Kill();
}


//
//void Controller(void){
//    while(1){
//          
//            int difference = IR3Val-IR2Val;
//            int threshold = 400;
//            static int i = 0;
//          //IR1 - front. IR2 - right back IR3- right front
//          //Too close to front wall, turn left
//            /*if(IR1Val > TooClose) //IR0 is on Right side and IR1 in front
//            {
//                motorMovement(LEFTMOTOR, STOP, FORWARD,80);
//                motorMovement(RIGHTMOTOR, STOP, FORWARD,120);
//            }*/
////          else
///*
//        if((IR3Val>IR2Val) && ((IR3Val-IR2Val) > threshold))
//      { //Right front is closer, turn left
//                motorMovement(LEFTMOTOR, STOP, FORWARD,150);
//                motorMovement(RIGHTMOTOR, MOVE, FORWARD,200);   
//          }
//            else    if((IR2Val>IR3Val) && ((IR2Val-IR3Val) > threshold))
//            {
//        //Right back is closer, turn right  
//                motorMovement(LEFTMOTOR, MOVE, FORWARD,200);
//                motorMovement(RIGHTMOTOR, STOP, FORWARD,150);   
//            }
//            else
//            {
//                motorMovement(LEFTMOTOR, MOVE, FORWARD,200);
//              motorMovement(RIGHTMOTOR, MOVE, FORWARD,200);
//            }
//            */
//            if(!turningMode){
//            if( PingRVal < TURNTHRESHR && PingLVal < TURNTHRESHL ){
//                if(IR1Val> TooClose){
//                    motorMovement(LEFTMOTOR, MOVE, FORWARD, 200);
//                    motorMovement(RIGHTMOTOR, MOVE, FORWARD, 50);
//                    //turningMode = 1;
//                }else{
//                    if(PingRVal < 10 || IR2Val > rightClose){
//                    motorMovement(LEFTMOTOR, MOVE, FORWARD, 190);
//                    motorMovement(RIGHTMOTOR, MOVE, FORWARD, 210);
//                    }else if(PingLVal < 10 || IR3Val > leftClose){
//                    motorMovement(LEFTMOTOR, MOVE, FORWARD, 210);
//                    motorMovement(RIGHTMOTOR, MOVE, FORWARD, 190);  
//                    }else{
//                    motorMovement(LEFTMOTOR, MOVE, FORWARD, WobbleL[i]);
//                    motorMovement(RIGHTMOTOR, MOVE, FORWARD, WobbleR[i]);
//                    }
//                }
//        
//            } else{     
//            if(PingRVal >= TURNTHRESHR ){
//                //turningMode = 1;
//                motorMovement(LEFTMOTOR, MOVE, FORWARD, 180);
//                motorMovement(RIGHTMOTOR, MOVE, FORWARD, 70);
//            }else if(PingLVal >= TURNTHRESHL ){
//                //turningMode = 1;
//                motorMovement(LEFTMOTOR, MOVE, FORWARD, 70);
//                motorMovement(RIGHTMOTOR, MOVE, FORWARD, 180);
//            }
//        }
//            i = (i + 1) % 4;
//    } else{
//        
//    }
//
//            
//            /*
//            //Too close to right wall, turn left
//            else if((IR2Val > TooClose)&&(IR3Val < OkRangeMax)) //IR0 is on Right side and IR1 in front
//            {
//                motorMovement(LEFTMOTOR, MOVE, FORWARD,80);
//                motorMovement(RIGHTMOTOR, MOVE, FORWARD,120);
//            }
//            //Too far from right wall, turn right
//            else if(IR2Val < TooFar) //IR0 is on Right side and IR1 in front
//            {
//                motorMovement(LEFTMOTOR, MOVE, FORWARD,120);
//              motorMovement(RIGHTMOTOR, MOVE, FORWARD,40);
//            }
//            //All okay, go ahead
//            else if((IR2Val < OkRangeMax)&&(IR2Val > OkRangeMin)) //IR0 is on Right side and IR1 in front
//            {
//                motorMovement(LEFTMOTOR, MOVE, FORWARD,120);
//              motorMovement(RIGHTMOTOR, MOVE, FORWARD,120);
//            }*/
//        OS_Sleep(2);
//    }
//    OS_Kill();
//}
//

void brown_out(void){
   CAN0_Open();
//NVIC_APINT_R = (1 << 2);
    //while(1);
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
  //  OS_InitSemaphore(&ADC_Collection, 1);
// create initial foreground threads
//  NumCreated += OS_AddThread(&PingR, 128, 1);
//    NumCreated += OS_AddThread(&PingL, 128, 1);

 
//  NumCreated += OS_AddThread(&IR0, 128, 2);
//  NumCreated += OS_AddThread(&IR1, 128, 2);
//  NumCreated += OS_AddThread(&IR2, 128, 2);
//  NumCreated += OS_AddThread(&IR3, 128, 2);
  OS_Launch(TIME_1MS); // doesn't return, interrupts enabled in here
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
  //NumCreated += OS_AddThread(&Controller2, 128, 2);
  NumCreated += OS_AddThread(&Controller, 128, 2);
  OS_InitSemaphore(&ADC_Collection, 1);
  NumCreated += OS_AddThread(&IR1, 128, 2);
  NumCreated += OS_AddThread(&IR2, 128, 2);
  NumCreated += OS_AddThread(&IR3, 128, 2);
  OS_Launch(TIME_1MS/4); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}

#endif 
