#include "Riverbed/river.hpp"

#include <iostream>

int main () {
  Riverbed::RingBuffer<int> rb(4);

  rb.push(0);
  rb.push(3);
  rb.push(4);
  rb.push(5);

  bool result = rb.push(6);

  if (!result) { 
    std::cout << "Push failed for 6. \n";
  }

  int num;
  rb.pop(num);
  
  std::cout << "Number popped: " << num;
}
