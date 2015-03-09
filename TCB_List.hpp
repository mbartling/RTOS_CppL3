#ifndef __TCB_LIST_HPP__
#define __TCB_LIST_HPP__
#include "TCB.h"

class Tcb_List {

  Tcb_t* head;
  int num_entries;
public:

  Tcb_List(void) : head(NULL), num_entries(0) {}

  void push_front(Tcb_t* node){
    insertX_beforeY(node, Head());
  }
  void push_back(Tcb_t* node){
    insertX_afterY(node, Head());
  }
  void insertX_beforeY(Tcb_t* node, Tcb_t* loc){
    if(head == NULL){
      node->next = node;
      node->prev = node;
      head = node;
    }
    else{
      node->next = loc;
      node->prev = loc->prev;
      loc->prev->next = node;
      loc->prev       = node;
    }
    num_entries++;
  }

  void insertX_afterY(Tcb_t* node, Tcb_t* loc){
    if(head == NULL){
      node->next = node;
      node->prev = node;
      head = node;
    }
    else{
      node->next = loc->next;
      node->prev = loc;
      loc->next->prev = node;
      loc->next       = node;
    }
    num_entries++;
  }

  Tcb_t* pop_front(void){
    return remove_node(Head());
  }

  Tcb_t* pop_back(void){
    return remove_node(Tail());
  }

  Tcb_t* remove_node(Tcb_t* node){
    Tcb_t* ret;
    if(isEmpty()){
      return NULL;
    }
    ret = node;
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->prev = node;
    if(node == Head()){
      head = head->next;
    }
    // if(num_entries == 1){
    if(head == head->next){
      //Clear the Head Pointer
      head = NULL;
    }
    num_entries--;
    return ret;
  }

  Tcb_t* Head(void){
    return head;
  }

  Tcb_t* Tail(void){
    return head->prev;
  }

  bool isEmpty(void){
    return (num_entries == 0);
    //return (head == NULL);
  }
  void MoveHead(void){
    head = head->next;
  }

};
#endif /* __TCB_LIST_HPP__*/