#pragma once

// config
#include "config.hpp"

// std
#include <cstdint>

namespace mesh_core {
namespace utils {

inline uint16_t time_based_random(uint16_t timestamp, uint16_t l, uint16_t r) {
  uint32_t hash = timestamp;
  hash ^= hash << 13;
  hash ^= hash >> 7;
  hash ^= hash << 5;
  hash = (uint16_t)hash;
  uint16_t range = r - l + 1;
  return l + (hash % range);
}

/**
 * CRC-16/CCITT-FALSE
 * @param data
 * @param size
 * @return
 */
inline uint16_t crc16(const void* data, uint16_t size) {
  auto d = (uint8_t*)data;
  uint16_t crc = 0xFFFF;
  static const uint16_t crc_table[16] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
                                         0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF};
  while (size--) {
    crc = (crc << 4) ^ crc_table[((crc >> 12) ^ (*d >> 4)) & 0x0F];
    crc = (crc << 4) ^ crc_table[((crc >> 12) ^ (*d & 0x0F)) & 0x0F];
    d++;
  }
  return crc & 0xFFFF;
}

}  // namespace utils
}  // namespace mesh_core
