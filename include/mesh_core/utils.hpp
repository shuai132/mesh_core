#pragma once

// config
#include "config.hpp"

// std
#include <cstdint>

namespace mesh_core {
namespace utils {

uint16_t time_based_random(uint16_t timestamp, uint16_t l, uint16_t r) {
  uint32_t hash = timestamp;
  hash ^= hash << 13;
  hash ^= hash >> 7;
  hash ^= hash << 5;
  hash = (uint16_t)hash;
  uint16_t range = r - l + 1;
  return l + (hash % range);
}

}  // namespace utils
}  // namespace mesh_core
