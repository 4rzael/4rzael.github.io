#pragma once
#include <cstdint>

#pragma pack(1)
struct Frame {
  enum class Type : uint8_t {
    REQUEST = 0,
    RESPONSE = 1
  } type;

  uint16_t length;
  char content[32];
};
