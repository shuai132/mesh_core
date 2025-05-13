#pragma once

// config
#include "config.hpp"
#include "detail/copyable.hpp"
#include "detail/log.h"
#include "detail/noncopyable.hpp"
#include "type.hpp"

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

enum class route_type {
  DYNAMIC = 0,
  STATIC = 1,
};

struct route_info : detail::copyable {
  addr_t dst{};
  addr_t next_hop{};
  uint8_t metric{};

  lqs_t lqs{};
  timestamp_t expired{};
  route_type type{route_type::DYNAMIC};
};

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
    auto old = find_node(info.dst);
    if (old == nullptr) {
      table_.push_back(info);
    } else {
      *old = info;
    }
  }

  void rm(addr_t dst) {
    table_.remove_if([dst](const route_info& i) {
      return i.dst == dst;
    });
  }

  const std::list<route_info>& get_table() {
    return table_;
  }

  void check_expired(timestamp_t ts) {
    table_.remove_if([&ts](route_info& info) {
      if (info.metric == 0) {  // skip self
        return false;
      }
      if (info.type == route_type::STATIC) {  // skip static route
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
