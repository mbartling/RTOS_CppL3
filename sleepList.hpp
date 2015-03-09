#ifndef __SLEEPLIST_HPP__
#define __SLEEPLIST_HPP__
#include "TCB.h"
#include "Pool.hpp"
// template <class T>
// struct Cell {
//     Cell* next;
//     Cell* prev;
//     T data;
//     Cell(void){
//       this->next = this;
//       this->prev = this;
//     }
//     // Cell(co{nst T& x): data(x) {}

//   };
template<class T, int N>
class List{
private:
  struct Cell;
  Pool<Cell,N> storage;
  int count;
  Cell* dummy;
  struct Cell {
    Cell* next;
    Cell* prev;
    T data;
    bool deleteMe;
    Cell(void){
      this->next = this;
      this->prev = this;
      deleteMe = false;
    }
    // Cell(co{nst T& x): data(x) {}

  };
  Cell* makeDummy(void){
    Cell* p = storage.get();
    p->next = p;
    p->prev = p;
		p->deleteMe = false;
    return p;
  }

  void deleteDummy(Cell* p){
    storage.free(p);
  }

public:

  List(void):count(0){
    dummy = makeDummy();
  }

  bool isEmpty(void) const {return head() == dummy;}
  void push_back(const T& elem){
    Cell* p = storage.get();
    p->data = elem;
		p->deleteMe = false;
    insertAfter(p, tail());
  }

  void push_front(const T& elem){
    Cell* p = storage.get();
    p->data = elem;
		p->deleteMe = false;
    insertAfter(p, head()->prev);
  }

  T pop_back(void){
    if(isEmpty()){
      //Do something
      return NULL;
    }
    T t = tail()->data;
    remove(tail());
    return t;
  }
  T pop_front(void){
    if(isEmpty()){
      //Do something
      return NULL;
    }
    T t = head()->data;
    remove(head());
    return t;
  }

  // T& Get(Cell* p){
  // // T& Get(iterator it){
  //   // Cell* p = it.getCell();
  //   // it++;
  //   T res = p->data;
  //   remove(p);
  //   return res;
  // }
  class iterator {
  private:
    Cell* position;
    friend class List;
  public:
    iterator(void) : position(NULL) {}
    iterator(Cell* p) : position(p) {}

    T& operator*(void) {return position->data;}
    iterator& operator++(void){
      position = position->next;
      return *this;
    } 
    iterator operator++(int){
      iterator t(*this);
      this->operator++();
      return t;
    }

    bool operator==(const iterator& rhs) const{
      return this->position == rhs.position;
    }
    bool operator!=(const iterator& rhs) const {
      return !(*this == rhs);
    }
    iterator& operator--(void){
      position = position->prev;
      return *this;
    }
    Cell* getCell(void){
      return position;
    }
    void mark4Delete(void){
      position->deleteMe = true;
    }
  };

  iterator begin(void){
    return iterator(head());
  }
  iterator end(void){
    //iterator result(this->tail());
    //++result;
    iterator result(dummy);
    return result;
  }
  iterator erase(iterator pos){
    Cell* p = pos.position;
    iterator iter(p->next);
    remove(p);
    return iter;
  }
// void remove(T elem){
//   for(Cell* iter = head(); iter->next != head(); iter = iter->next){
//     if(iter->data == elem){
//       remove(iter);
//     }
//   }
// }
  void clean(void){
    for(Cell* iter = head(); iter->next != head(); iter = iter->next){
      if(iter->deleteMe){
        remove(iter);
      }
    }
  }
private:
  Cell* head(void) const {return dummy->next;}
  Cell* tail(void) const {return dummy->prev;}

  void insertAfter(Cell* newCell, Cell* location){
    newCell->prev = location;
    newCell->next = location->next;
    location->next->prev = newCell;
    location->next = newCell;
    count++;
  }
  void remove(Cell* p){
    p->prev->next = p->next;
    p->next->prev = p->prev;
		p->data = NULL;
    storage.free(p);
    count--;
  }

};
#endif /*__SLEEPLIST_HPP__*/
