#include <iostream>
#include <cstring>

#include "Frame.hpp"
#include "Protocol.hpp"

bool Protocol::connect() {
  _connected = true;
  return true;
}

bool Protocol::send(int32_t value) {
  Frame frame = {.type = Frame::Type::REQUEST, .length = 4};
  ::memcpy(frame.content, &value, sizeof(value));

  if (_ackCallback) _ackCallback(false);
  return true;
}

void Protocol::onAck(AckCallback const &cb) {
  _ackCallback = cb;
}
