#ifndef __POOL_HPP__
#define __POOL_HPP__

#include <stdint.h>
#include <cstddef>

/**
 * @brief Create a Managed Pool of objects
 * @details Template storage class for fixed sized objects.
 * Helps us get around malloc
 * 
 * @param T generate storage container of type T elements
 * @param N Number of elements in Pool
 * 
 * @return Pointers to Pool elements
 */
template <class T, int N> class Pool
{
private:
  T storage[N];
  int8_t status[N];
  int numUsed;
  int lastAccessed;
public:
  /**
   * @brief Default constructor of Pool
   */
  Pool(void) : numUsed(0), lastAccessed(0){
    for(int i = 0; i < N; i++){
      status[i] = 0;
    }
  }

/**
 * @brief Get reference to next available element in Pool
 * @details Does not check for invalid access. Must check availability
 * with the available() function.
 * @return reference to an element in the pool
 */
  T* get(void){
    int nextAvail = 0;

    for(int i = 0; i < N; i++){
      if(status[i] == 0){
        nextAvail = i;
        break;
      }
    }

    T* res = &storage[nextAvail];
    status[nextAvail] = 1;
    lastAccessed = nextAvail;
    numUsed++;
    return res;
  }

  /**
   * @brief free up a used element in the Pool
   * @details will fail if element reference does not exist in Pool
   * Behaves similarly to fixed time free() (see malloc)
   * @param elem Reference to element to free
   */
  void free(T* elem){

    for(int i = 0; i < N; i++){
      if(elem == &storage[i]){
        status[i] = 0;
        numUsed--;
        elem = NULL;
        break;
      }
    }
    return;
  }
  /**
   * @brief Check if there are any available elements in pool
   * @return Number of available elements
   */
  int available(void) const{
    return N - numUsed;
  }
  /**
   * @brief Helper function for thread ID setting and debuggins
   * @return The Id of the last Pool element accessed
   */
  int getId(void){
    return lastAccessed;
  }

};

#endif /*__POOL_HPP__*/
