//
// Created by petr on 3/26/21.
//

#include "BinaryEncoder.h"
#include "utils.h"
#include <iostream>
int main() {
  forEachBit_n(static_cast<uint8_t>(0xF0u), 5, [](bool a) { std::cout << std::boolalpha << a << std::endl; });
  return 0;
}