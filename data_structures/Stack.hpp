#ifndef __STACK_HPP__
#define __STACK_HPP__

#include <stdint.h>
#include <cstddef>
#include <string.h>

template <int N> class Stack
{
private:
  uint8_t storage[N];
  int numUsed;
public:
  Stack(void) : numUsed(0) {}

  void* get(int n){   //Allocate n bytes from the stack
    uint8_t* ptr;
    if((numUsed + n + sizeof(int)) > N ){
      return NULL;
    }
    ptr = &storage[numUsed];
    memcpy(&storage[n+numUsed], &n, sizeof(int));
    numUsed += n + sizeof(int);
    return ( void* )ptr;
  }

  void free(){    //Return the last value returned
                      //by get from the stack
    numUsed -= sizeof(int);
    int num2Free;
    memcpy(&num2Free, &storage[numUsed], sizeof(int));
    numUsed -= num2Free;
    return;
  }
  int available(void) const{
    return N - numUsed - sizeof(int);
  }
 
};

#endif /*__STACK_HPP__*/
