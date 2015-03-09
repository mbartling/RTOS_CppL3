#ifndef __MAILBOX_HPP__
#define __MAILBOX_HPP__
#include "os.h"

/**
 * @brief Basic Mailbox prototype
 * @details Simple send and receive functions to hide implementation
 * 
 * @tparam T Type of Mailbox data
 */
template <class T>
class Mailbox{
private:
  Sema4Type semaSend;
  Sema4Type semaAck;
  T Mail;
public:
  Mailbox(void) {
    OS_InitSemaphore(&semaSend, 0);
    OS_InitSemaphore(&semaAck, 0);
  }
  /**
   * @brief Send data via a mailbox
   * 
   * @param data Data to send
   */
  inline void Send(T& data){
    Mail = data;
    OS_Signal(&semaSend);
    OS_Wait(&semaAck);
  }

  /**
   * @brief Receive data via a mailbox
   * 
   * @param data Data to receive
   */
  inline void Receive(T& data){
    OS_Wait(&semaSend);
    data = Mail;
    OS_Signal(&semaAck);
  }

};
#endif /*__MAILBOX_HPP__*/
