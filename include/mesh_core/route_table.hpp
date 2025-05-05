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

  snr_t snr{};
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
        return true;
      }
      return false;
    });
  }

  void debug_print() {
    MESH_CORE_LOG("route table: size: %u", (uint32_t)table_.size());
    for (const auto& item : table_) {
      MESH_CORE_LOG("item: dst: 0x%02X, next_hop: 0x%02X, metric: %d, snr: %d, expired: 0x%08X", item.dst, item.next_hop, item.metric, item.snr,
                    item.expired);
    }
  }

 private:
  std::list<route_info> table_;
};

}  // namespace mesh_core
