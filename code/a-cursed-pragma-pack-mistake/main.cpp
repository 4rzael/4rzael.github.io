#include <iostream>
#include "Protocol.hpp"

int main() {
  Protocol comm;

  comm.connect();
  comm.onAck([](bool success) {
    std::cout << "Success: " << success << std::endl;
  });

  comm.send(0);
  return 0;
}