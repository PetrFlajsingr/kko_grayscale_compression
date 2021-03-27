//
// Created by petr on 3/26/21.
//

#include "BinaryEncoder.h"
#include <iostream>
int main() {
  [[maybe_unused]] auto test = BinaryEncoder<uint8_t>();
  test.pushBack(true);
  test[0] = false;
  return 0;
}