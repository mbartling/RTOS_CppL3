#ifndef __FIFO_HPP__
#define __FIFO_HPP__

#include "os.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value

#ifdef __cplusplus
 }
#endif

#define SUCCESS true
#define FAIL false

/**
 * @brief Base Class Fifo
 * @details Todo: make get, put, and getsize virtual
 * 
 * @tparam T type of Fifo
 */
//template <typename T>
 class Fifo{
 	// enum Status {FAIL=-1, SUCCESS=0};
  Sema4Type s1;
  Sema4Type s2;

 public:
	Fifo(void){
    OS_InitSemaphore(&s1, 1);
		OS_InitSemaphore(&s2, 0);
	 }
  Fifo(int bW, int W){
    OS_InitSemaphore(&s1, bW);
    OS_InitSemaphore(&s2, W);
   }
  //using SUCCESS = true;
  //using FAIL = false;

 
//  virtual bool Put(T data){
//    return FAIL; 
//  }
//  virtual bool Get(T* data){
//    return (FAIL);
//  }
  inline void Wait_s2(){
    OS_Wait(&s2);
  }
  inline void Wait_s1(){
    OS_Wait(&s1);
  }
  inline void Signal_s2(){
    OS_Signal(&s2);
  }
  inline void Signal_s1(){
    OS_Signal(&s1);
  }
  inline void set_s1(int val){
    OS_InitSemaphore(&s1, val);
  }
  inline void set_s2(int val){
    OS_InitSemaphore(&s2, val);
  }
//  virtual unsigned short getSize(void){
//    return (unsigned short) 0;
//  }
 };
 /**
 * @brief Pointer type Fifo
 * @details Never fails, good for multiple consumers and multiple
 * producers. Doesnt work well for background thread because it blocks
 * both put and get.
 *  
 *  Wait_s2 uses Room Left
 *  Wait_s1 Uses CurrentSize 
 * @tparam T Type of data in Fifo
 * @tparam Size maximum size of fifo
 */
template <typename T, int Size>
class FifoP : public Fifo{
  T volatile * PutPt;
  T volatile * GetPt;
  T volatile FifoData[Size];
  unsigned long FifoSize;
  uint32_t LostData;
  Sema4Type mutex;
  // enum Status {FAIL=-1, SUCCESS=0};
  inline void Slot(void){
    OS_Wait(&mutex);
  }
  inline void freeMutex(void){
    OS_Signal(&mutex);
  }
public:
  FifoP() : Fifo(0, Size){
    long sr;
    sr = StartCritical();
    FifoSize = Size;
    PutPt = GetPt = &FifoData[0];
    EndCritical(sr);
    OS_InitSemaphore(&mutex, 1);
  }

  bool Put(T data){
  
    this->Wait_s2(); //Wait for room left
    Slot();       //wait for mutex
    *(PutPt++) = data;
    if(PutPt == &FifoData[FifoSize]){
      PutPt = &FifoData[0];
    }
    freeMutex();   // free mutex
    this->Signal_s1();  //Update CurrentSize
    return SUCCESS;
  }

  bool Get(T *data){

    this->Wait_s1(); //Wait till have something available
//    this->Wait_s1();
		Slot();        //Wait for mutex
    *data = *(GetPt++);
    if(GetPt == &FifoData[FifoSize]){
      GetPt = &FifoData[0];
    }
    freeMutex();     //free mutex
    this->Signal_s2(); 
    // this->Signal_s2();
    return SUCCESS;
  }
  void setSize(unsigned long newSize){
    FifoSize = newSize;
    set_s2(newSize);
  }
  unsigned long getSize(void){
    if(PutPt < GetPt){
      return (unsigned long) (PutPt - GetPt + FifoSize*sizeof(T))/sizeof(T);
    }
    return (unsigned long)(PutPt - GetPt)/sizeof(T);
  }
  void Flush(void){
    PutPt = GetPt = &FifoData[0];
  }
};

/**
 * @brief Pointer type Fifo Better for Single Producer Multiple Consumers
 * @details [long description]
 * 
 * @tparam T Type of data in Fifo
 * @tparam Size maximum size of fifo
 */
template <typename T, int Size>
class FifoP_SP2MC : public Fifo{
  T volatile * PutPt;
  T volatile * GetPt;
  T volatile FifoData[Size];
  unsigned long FifoSize;
  uint32_t LostData;
  // enum Status {FAIL=-1, SUCCESS=0};

public:
  FifoP_SP2MC() : Fifo(){
    long sr;
    sr = StartCritical();
    FifoSize = Size;
    PutPt = GetPt = &FifoData[0];
    EndCritical(sr);
  }

  bool Put(T data){
    T volatile *nextPutPt;
    nextPutPt = PutPt + 1;

    if(nextPutPt == &FifoData[FifoSize]){
      nextPutPt = &FifoData[0];
    }

    if(nextPutPt == GetPt){
      LostData++;
			this->Signal_s2();
			return(FAIL);

    }
   else{
      *(PutPt) = data;
      PutPt = nextPutPt;
      this->Signal_s2();
      return(SUCCESS);

    }
	 return(FAIL);
  }

  bool Get(T *data){
    this->Wait_s2(); //Wait till available

    this->Wait_s1(); //data mutex
    *data = *(GetPt++);
    if(GetPt == &FifoData[FifoSize]){
      GetPt = &FifoData[0];
    }
    this->Signal_s1();//end data mutex
    return SUCCESS;
  }
  void setSize(unsigned long newSize){
    FifoSize = newSize;
  }
  unsigned long getSize(void){
    if(PutPt < GetPt){
      return (unsigned long) (PutPt - GetPt + FifoSize*sizeof(T))/sizeof(T);
    }
    return (unsigned long)(PutPt - GetPt)/sizeof(T);
  }
  void Flush(void){
    PutPt = GetPt = &FifoData[0];
  }
};
/**
 * @brief Pointer type Fifo Better for Multiple producers single consumer
 * @details [long description]
 * 
 * @tparam T Type of data in Fifo
 * @tparam Size maximum size of fifo
 */
template <typename T, int Size>
class FifoP_MP2SC : public Fifo{
  T volatile * PutPt;
  T volatile * GetPt;
  T volatile FifoData[Size];
  unsigned long FifoSize;
  uint32_t LostData;
  // enum Status {FAIL=-1, SUCCESS=0};

public:
  FifoP_MP2SC() : Fifo(1,Size-1){
    long sr;
    sr = StartCritical();
    FifoSize = Size;
    PutPt = GetPt = &FifoData[0];
    EndCritical(sr);
  }

  bool Put(T data){
    T volatile *nextPutPt;
    nextPutPt = PutPt + 1;

    if(nextPutPt == &FifoData[FifoSize]){
      nextPutPt = &FifoData[0];
    }

      this->Wait_s2();
      this->Wait_s1();
      *(PutPt) = data;
      PutPt = nextPutPt;
      this->Signal_s1();
      return(SUCCESS);
  }

  bool Get(T *data){

    *data = *(GetPt++);
    if(GetPt == &FifoData[FifoSize]){
      GetPt = &FifoData[0];
    }
    this->Signal_s2(); //I have free space
    return SUCCESS;
  }
  void setSize(unsigned long newSize){
    FifoSize = newSize;
  }
  unsigned long getSize(void){
    if(PutPt < GetPt){
      return (unsigned long) (PutPt - GetPt + FifoSize*sizeof(T))/sizeof(T);
    }
    return (unsigned long)(PutPt - GetPt)/sizeof(T);
  }
  void Flush(void){
    PutPt = GetPt = &FifoData[0];
  }
};

/*
template<typename T, int Size> 
class FifoI : public Fifo{
  uint32_t volatile PutI;
  uint32_t volatile GetI;
  T FifoData[Size];
  // enum Status {FAIL=-1, SUCCESS=0};

public:
  FifoI(){
    long sr;
    sr = StartCritical();
    PutI = GetI = 0;
    EndCritical(sr);
  }

  bool Put(T data){
    if((PutI - GetI) & ~(Size - 1)){
      return FAIL;
    }
    FifoData[PutI & (Size - 1)] = data;
    PutI++;
    return SUCCESS;
  }

  bool Get(T* data){
    if(PutI == GetI){
      return(FAIL);
    }
    *data = FifoData[GetI & (Size - 1)];
    GetI++;
    return SUCCESS;
  }

  unsigned short getSize(void){
    return (unsigned short)(PutI - GetI);
  }

};
*/
#endif /*__FIFO_HPP__*/
