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

/// ┌─────────┬──────────────┬───────────────────┬───────────────────────────────┐
/// │ Bytes   │ Field        │ Default/Example   │ Description                   │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 1       │ head         │ 0x3C              │ Header identifier: Mesh Core  │
/// │ 1       │ ver          │ 0x00              │ Protocol version              │
/// │ 1       │ len          │ 0x00              │ Payload length in bytes       │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 1/2     │ type         │ 0x0               │ Message type                  │
/// │ 1/2     │ ttl          │ 0x0               │ Hops                          │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 1       │ src          │ 0x00              │ Source address                │
/// │ 1       │ dst          │ 0x00              │ Destination address           │
/// │ 1       │ seq          │ 0x00              │ Sequence number               │
/// │ 4       │ ts           │ 0x00000000        │ Timestamp for milliseconds    │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ n       │ route_infos  │ [variable]        │ Route info list               │
/// │---------│--------------│-------------------│-------------------------------│
/// │ 1       │ next_hop     │ 0x00              │ Next hop                      │
/// │ n       │ data         │ [variable]        │ Payload for user data         │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 2       │ crc          │ 0x0000            │ CRC-16 of all preceding fields│
/// └─────────┴──────────────┴───────────────────┴───────────────────────────────┘

enum class message_type : uint8_t {
  route_info = 0,
  route_info_and_request = 1,
  sync_time = 2,
  broadcast = 3,
  user_data = 4,
  route_debug_send = 5,
  route_debug_back = 6,
};

struct message : detail::copyable {
  // header
  uint8_t head = MESH_CORE_MSG_MAGIC;  // Mesh Core
  uint8_t ver = MESH_CORE_PROTO_VER;
  uint8_t len{};
  message_type type{};
  ttl_t ttl{};
  addr_t src{};
  addr_t dst{};
  seq_t seq{};
  timestamp_t ts{};

  addr_t next_hop{};  // only for userdata and some type
  data_t data;        // userdata or route_infos

  uint16_t crc{};

 public:
  // clang-format off
  // message min size(without data): 13 bytes
  static const uint8_t SizeMin = sizeof(head) + sizeof(ver) + sizeof(len) + sizeof(ttl) + sizeof(src) + sizeof(dst)+ sizeof(seq) + sizeof(ts) + sizeof(crc);
  // clang-format on
  static const uint8_t SizeNotInLen = sizeof(head) + sizeof(ver) + sizeof(len);
  // message data max size: 250 bytes
  static const uint8_t DataSizeMax = (sizeof(len) << 8) - SizeNotInLen - sizeof(crc) - sizeof(next_hop);
  static const uint16_t SizeMax = SizeMin + DataSizeMax;

 public:
  msg_uuid_t cal_uuid() const {
    static_assert(std::is_same<msg_uuid_t, uint32_t>::value, "");
    static_assert(std::is_same<ttl_t, uint8_t>::value, "");
    static_assert(std::is_same<timestamp_t, uint32_t>::value, "");
    // uuid: {src|seq|ts}, ts is for ensure message is unique after reboot
    return (src << 24) | (seq << 16) | (uint16_t(ts & 0x0000FFFF));
  }

  std::string serialize(bool& ok) {
    if (data.size() > DataSizeMax) {
      ok = false;
      return {};
    }
    finalize();
    std::string payload;
    payload.reserve(SizeMin + data.size());
    payload.append((char*)&head, sizeof(head));
    payload.append((char*)&ver, sizeof(ver));
    payload.append((char*)&len, sizeof(len));
    uint8_t type_ttl = ((uint8_t)type << 4) | ttl;
    payload.append((char*)&type_ttl, sizeof(type_ttl));
    payload.append((char*)&src, sizeof(src));
    payload.append((char*)&dst, sizeof(dst));
    payload.append((char*)&seq, sizeof(seq));
    payload.append((char*)&ts, sizeof(ts));
    if (has_next_hop(*this)) {
      payload.append((char*)&next_hop, sizeof(next_hop));
    }
    payload.append(data);
    crc = utils::crc16(payload.data(), payload.size());
    payload.append((char*)&crc, sizeof(crc));
    ok = true;
    return payload;
  }

  static message deserialize(const std::string& payload, bool& ok) {
    message msg;
    if (payload.size() < SizeMin || payload.size() > SizeMax) {
      MESH_CORE_LOGE("size error");
      ok = false;
      return msg;
    }
    char* p = (char*)payload.data();
    const char* pend = payload.data() + payload.size();
    msg.head = *(decltype(head)*)p;
    p += sizeof(head);
    if (msg.head != MESH_CORE_MSG_MAGIC) {
      MESH_CORE_LOGE("head error");
      ok = false;
      return msg;
    }
    msg.ver = *(decltype(ver)*)p;
    p += sizeof(ver);
    if (msg.ver != MESH_CORE_PROTO_VER) {
      MESH_CORE_LOGE("version error");
      ok = false;
      return msg;
    }
    msg.len = *(decltype(len)*)p;
    p += sizeof(len);
    if (msg.len != payload.size() - SizeNotInLen) {
      MESH_CORE_LOGE("len error");
      ok = false;
      return msg;
    }
    uint8_t type_ttl = *(uint8_t*)p;
    msg.type = static_cast<message_type>(type_ttl >> 4);
    msg.ttl = type_ttl & 0x0F;
    p += sizeof(type_ttl);
    msg.src = *(decltype(src)*)p;
    p += sizeof(src);
    msg.dst = *(decltype(dst)*)p;
    p += sizeof(dst);
    msg.seq = *(decltype(seq)*)p;
    p += sizeof(seq);
    msg.ts = *(decltype(ts)*)p;
    p += sizeof(ts);
    if (has_next_hop(msg)) {
      msg.next_hop = *(decltype(next_hop)*)p;
      p += sizeof(next_hop);
    }

    // crc
    msg.crc = *(uint16_t*)(pend - sizeof(crc));
    uint16_t crc = utils::crc16(payload.data(), payload.size() - sizeof(crc));
    if (msg.crc != crc) {
      MESH_CORE_LOGE("crc error");
      ok = false;
      return msg;
    }

    // data
    msg.data.assign(p, pend - p - sizeof(crc));
    ok = true;
    return msg;
  }

  void finalize() {
    len = SizeMin + data.size() - SizeNotInLen;
    if (has_next_hop(*this)) {
      ++len;
    }
  }

  static bool has_next_hop(const message& msg) {
    return msg.type == message_type::user_data || msg.type == message_type::route_debug_send || msg.type == message_type::route_debug_back;
  }
};

}  // namespace mesh_core
