#pragma once

// first include
#include "mesh_core/config.hpp"

// other include
#include "mesh_core/detail/copyable.hpp"
#include "mesh_core/detail/noncopyable.hpp"
#include "mesh_core/type.hpp"
#include "mesh_core/utils.hpp"

// std
#include <string>

namespace mesh_core {
namespace detail {

/// ┌─────────┬──────────────┬───────────────────┬───────────────────────────────┐
/// │ Bytes   │ Field        │ Default/Example   │ Description                   │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 1       │ head         │ 0x3C              │ Header identifier: Mesh Core  │
/// │ 1       │ ver          │ 0x00              │ Protocol version              │
/// │ 1       │ len          │ 0x00              │ Payload length in bytes       │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 1       │ src          │ 0x00              │ Source address                │
/// │ 1       │ dest         │ 0x00              │ Destination address           │
/// │ 1       │ seq          │ 0x00              │ Sequence number               │
/// │ 1       │ ttl          │ 0x00              │ Time To Live (hops)           │
/// │ 2       │ ts           │ 0x0000            │ Timestamp (16-bit)            │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ n       │ data         │ [variable]        │ Payload data (len bytes)      │
/// │ 2       │ crc          │ 0x0000            │ CRC-16 of all preceding fields│
/// └─────────┴──────────────┴───────────────────┴───────────────────────────────┘
/// 11 bytes without data
struct message : detail::copyable {
  // header
  uint8_t head = MESH_CORE_MSG_MAGIC;  // Mesh Core
  uint8_t ver = MESH_CORE_PROTO_VER;
  uint8_t len{};
  addr_t src{};
  addr_t dest{};
  seq_t seq{};
  ttl_t ttl{};
  timestamp_t ts{};
  // data
  data_t data;
  uint16_t crc{};

 public:
  // clang-format off
  // message min size(without data): 11 bytes
  static const uint8_t SizeMin = sizeof(head) + sizeof(ver) + sizeof(len) + sizeof(src) + sizeof(dest) + sizeof(seq) + sizeof(ttl) + sizeof(ts) + sizeof(crc);
  // clang-format on
  static const uint8_t SizeNotInLen = sizeof(head) + sizeof(ver);
  // message data max size: 252 bytes
  static const uint8_t DataSizeMax = (sizeof(len) << 8) - SizeNotInLen - sizeof(crc);
  static const uint16_t SizeMax = SizeMin + DataSizeMax;

 public:
  msg_uuid_t cal_uuid() const {
    static_assert(std::is_same<msg_uuid_t, uint32_t>::value, "");
    static_assert(std::is_same<ttl_t, uint8_t>::value, "");
    static_assert(std::is_same<timestamp_t, uint16_t>::value, "");
    // uuid: {src|seq|ts}
    return (src << 24) | (seq << 16) | (ts);
  }

  void finalize() {
    len = SizeMin + data.size() - SizeNotInLen;
  }

  std::string serialize(bool& ok) {
    if (data.size() > DataSizeMax) {
      ok = false;
      return {};
    }
    std::string payload;
    payload.reserve(SizeMin + data.size());
    payload.append((char*)&head, sizeof(head));
    payload.append((char*)&ver, sizeof(ver));
    payload.append((char*)&len, sizeof(len));
    payload.append((char*)&src, sizeof(src));
    payload.append((char*)&dest, sizeof(dest));
    payload.append((char*)&seq, sizeof(seq));
    payload.append((char*)&ttl, sizeof(ttl));
    payload.append((char*)&ts, sizeof(ts));
    payload.append(data);
    uint16_t crc16 = utils::crc16(payload.data(), payload.size());
    payload.append((char*)&crc16, sizeof(crc16));
    ok = true;
    return payload;
  }

  static message deserialize(const std::string& payload, bool& ok) {
    message msg;
    if (payload.size() < SizeMin || payload.size() > SizeMax) {
      MESH_CORE_LOGD("size error");
      ok = false;
      return msg;
    }
    char* p = (char*)payload.data();
    const char* pend = payload.data() + payload.size();
    msg.head = *(decltype(head)*)p;
    p += sizeof(head);
    if (msg.head != MESH_CORE_MSG_MAGIC) {
      MESH_CORE_LOGD("head error");
      ok = false;
      return msg;
    }
    msg.ver = *(decltype(ver)*)p;
    p += sizeof(ver);
    if (msg.ver != MESH_CORE_PROTO_VER) {
      MESH_CORE_LOGD("version error");
      ok = false;
      return msg;
    }
    msg.len = *(decltype(len)*)p;
    p += sizeof(len);
    if (msg.len != payload.size() - SizeNotInLen) {
      MESH_CORE_LOGD("size error");
      ok = false;
      return msg;
    }
    msg.src = *(decltype(src)*)p;
    p += sizeof(src);
    msg.dest = *(decltype(dest)*)p;
    p += sizeof(dest);
    msg.seq = *(decltype(seq)*)p;
    p += sizeof(seq);
    msg.ttl = *(decltype(ttl)*)p;
    p += sizeof(ttl);
    msg.ts = *(decltype(ts)*)p;
    p += sizeof(ts);

    // crc
    msg.crc = *(uint16_t*)(pend - sizeof(crc));
    uint16_t crc = utils::crc16(payload.data(), payload.size() - sizeof(crc));
    if (msg.crc != crc) {
      MESH_CORE_LOGD("crc error");
      ok = false;
      return msg;
    }

    // data
    msg.data.assign(p, pend - p - sizeof(crc));
    ok = true;
    return msg;
  }
};

}  // namespace detail
}  // namespace mesh_core
