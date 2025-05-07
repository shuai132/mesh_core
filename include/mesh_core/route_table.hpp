#pragma once

// config
#include "config.hpp"

// std
#include <cstdint>
#include <functional>
#include <list>

namespace mesh_core {

#pragma pack(1)
struct route_msg : detail::copyable {
  addr_t dst{};
  addr_t next_hop{};
  uint8_t metric{};
};
#pragma pack()

struct route_info : detail::copyable {
  addr_t dst{};
  addr_t next_hop{};
  uint8_t metric{};

  lqs_t snr{};
  timestamp_t expired{};
};

// [ {dst, next_hop, metric}, ...]
class route_table : detail::noncopyable {
 public:
  route_info* find_node(addr_t dst) {
    auto it = std::find_if(table_.begin(), table_.end(), [dst](const route_info& info) {
      return info.dst == dst;
    });
    if (it == table_.cend()) {
      return nullptr;
    } else {
      return &*it;
    }
  }

  void add(route_info info) {
    table_.push_back(std::move(info));
  }

  const std::list<route_info>& get_table() {
    return table_;
  }

  void check_expired(timestamp_t ts) {
    table_.remove_if([&ts](route_info& info) {
      if (info.metric == 0) {  // is self
        return false;
      }
      if (ts - info.expired > MESH_CORE_ROUTE_EXPIRED_MS) {
        MESH_CORE_LOGD("route expired: 0x%02X", info.dst);
        return true;
      }
      if (info.metric >= MESH_CORE_TTL_DEFAULT) {
        MESH_CORE_LOGD("metric expired");
        return true;
      }
      return false;
    });
  }

 private:
  std::list<route_info> table_;
};

}  // namespace mesh_core
