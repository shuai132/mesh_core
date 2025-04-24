#pragma once

// first include
#include "mesh_core/config.hpp"

// other include
#include "mesh_core/detail/copyable.hpp"
#include "mesh_core/detail/noncopyable.hpp"
#include "mesh_core/type.hpp"

// std
#include <string>

namespace mesh_core {
namespace detail {

struct message : detail::copyable {
  // header
  addr_t src{};
  addr_t dest{};
  seq_t seq{};
  ttl_t ttl{};
  // data
  data_t data;

 public:
  static const int header_size = sizeof(src) + sizeof(dest) + sizeof(seq) + sizeof(ttl);

 public:
  msg_uuid_t cal_uuid() {
    return (src << sizeof(seq)) + seq;
  }

  std::string serialize() {
    std::string payload;
    payload.reserve(header_size + data.size());
    payload.append((char*)&src, sizeof(src));
    payload.append((char*)&dest, sizeof(dest));
    payload.append((char*)&seq, sizeof(seq));
    payload.append((char*)&ttl, sizeof(ttl));
    payload.append(data);
    return payload;
  }

  static message deserialize(const std::string& payload, bool& ok) {
    message msg;
    if (payload.size() < header_size) {
      ok = false;
      return msg;
    }
    char* p = (char*)payload.data();
    const char* pend = payload.data() + payload.size();
    msg.src = *(decltype(src)*)p;
    p += sizeof(src);
    msg.dest = *(decltype(dest)*)p;
    p += sizeof(dest);
    msg.seq = *(decltype(seq)*)p;
    p += sizeof(seq);
    msg.ttl = *(decltype(ttl)*)p;
    p += sizeof(ttl);

    msg.data.assign(p, pend - p);
    ok = true;
    return msg;
  }
};

}  // namespace detail
}  // namespace mesh_core
