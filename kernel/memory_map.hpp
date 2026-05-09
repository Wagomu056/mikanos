#pragma once

#include <stdint.h>

struct MemoryMap {
  unsigned long long buffer_size;
  void *buffer;
  unsigned long long map_size;
  unsigned long long map_key;
  unsigned long long descriptor_size;
  uint32_t descriptor_version;
};
