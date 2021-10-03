#pragma once
#include <functional>
#include <cstdint>

class Protocol {
 public:
  using AckCallback = std::function<void (bool)>;

  bool connect();
  bool send(int32_t value);
  void onAck(AckCallback const &cb);
 private:
  bool        _connected = false;
  AckCallback _ackCallback = AckCallback();
};
