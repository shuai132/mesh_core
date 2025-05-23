#pragma once

// first include
#include "mesh_core/config.hpp"

// other include
#include "mesh_core/detail/copyable.hpp"
#include "mesh_core/detail/log.h"
#include "mesh_core/detail/lru_record.hpp"
#include "mesh_core/detail/noncopyable.hpp"
#include "mesh_core/message.hpp"
#include "mesh_core/route_table.hpp"
#include "mesh_core/type.hpp"
#include "mesh_core/utils.hpp"

// std
#include <cinttypes>
#include <functional>
#include <string>

namespace mesh_core {

/// interceptor
#ifdef MESH_CORE_ENABLE_BROADCAST_INTERCEPTOR
using broadcast_interceptor_t = std::function<bool(message&)>;
#endif

#ifdef MESH_CORE_ENABLE_DISPATCH_INTERCEPTOR
using dispatch_interceptor_t = std::function<bool(message&)>;
#endif

template <typename Impl>
class mesh : detail::noncopyable {
 public:
  explicit mesh(Impl* impl) : impl_(impl) {}

  void init(addr_t addr, bool enable_dv_routing = true) {
    addr_ = addr;
    init_(enable_dv_routing);
  }

  addr_t addr() {
    return addr_;
  }

  void send(addr_t dst, data_t data) {
    if (data.size() > message::DataSizeMax) {
      MESH_CORE_LOGE("data size > %d", message::DataSizeMax);
      return;
    }

    message m = create_message(message_type::user_data, dst);
    auto info = route_table_.find_node(dst);
    m.next_hop = info ? info->next_hop : addr_;
    m.data = std::move(data);
    broadcast(std::move(m));
  }

#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
  void send_route_debug(addr_t dst, bool is_send = true) {
    auto type = is_send ? message_type::route_debug_send : message_type::route_debug_back;
    message m = create_message(type, dst);
    auto info = route_table_.find_node(dst);
    m.next_hop = info ? info->next_hop : addr_;
    m.data = std::to_string(addr_);
    broadcast(std::move(m));
  }
#endif

  void broadcast(data_t data) {
    if (data.size() > message::DataSizeMax) {
      MESH_CORE_LOGE("data size > %d", message::DataSizeMax);
      return;
    }
    message m = create_message(message_type::broadcast, {});
    m.data = std::move(data);
    broadcast(std::move(m));
  }

  void on_recv(on_recv_handle_t handle) {
    on_recv_handle_ = std::move(handle);
  }

#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
  void on_recv_debug(on_recv_debug_handle_t handle) {
    on_recv_debug_handle_ = std::move(handle);
  }
#endif

#ifdef MESH_CORE_ENABLE_TIME_SYNC
  void on_sync_time(time_sync_handle_t handle) {
    time_sync_handle_ = std::move(handle);
  }
#endif

  timestamp_t get_timestamp() {
    return impl_->get_timestamp_ms();
  }

#ifdef MESH_CORE_ENABLE_TIME_SYNC
  uint32_t sync_time() {
    message m = create_message(message_type::sync_time, {});
    broadcast(std::move(m));
    return m.ts;
  }
#endif

  void add_static_route(addr_t dst, addr_t next_hop) {
    route_info info;
    info.dst = dst;
    info.next_hop = next_hop;
    info.metric = 1;
    info.type = route_type::STATIC;
    route_table_.add(info);
  }

#ifdef MESH_CORE_ENABLE_BROADCAST_INTERCEPTOR
  /**
   * @param interceptor return true for continue
   */
  void set_broadcast_interceptor(broadcast_interceptor_t interceptor) {
    broadcast_interceptor_ = std::move(interceptor);
  }
#endif

#ifdef MESH_CORE_ENABLE_DISPATCH_INTERCEPTOR
  /**
   * @param interceptor return true for continue
   */
  void set_dispatch_interceptor(dispatch_interceptor_t interceptor) {
    dispatch_interceptor_ = std::move(interceptor);
  }
#endif

  void dump_debug() {
    MESH_CORE_LOGD("route table: 0x%02X: %" PRIu32, addr_, (uint32_t)route_table_.get_table().size());
    for (const auto& item : route_table_.get_table()) {
      MESH_CORE_LOGD("dst: 0x%02X, next_hop: 0x%02X, metric: %d, lqs: %d, expired: 0x%08" PRIX32, item.dst, item.next_hop, item.metric, item.lqs,
                     item.expired);
    }
  }

  void sync_route(bool request = false) {
#ifdef MESH_CORE_DISABLE_ROUTE
    // only report self
    auto type = request ? message_type::route_info_and_request : message_type::route_info;
    message m = create_message(type, {});
    route_msg rm;
    rm.dst = addr_;
    rm.next_hop = addr_;
    rm.metric = 0;
    m.data.append(reinterpret_cast<const char*>(&rm), sizeof(rm));
    broadcast(std::move(m));
#else
    const auto& table = route_table_.get_table();
    const int max_per_msg = (int)(message::DataSizeMax / sizeof(route_msg));
    auto it = table.begin();
    while (it != table.end()) {
      message m = create_message(message_type::route_info, {});
      int count = 0;
      route_msg rm;
      while (it != table.end() && count < max_per_msg) {
        const auto& item = *it;
        rm.dst = item.dst;
        rm.next_hop = addr_;
        rm.metric = item.metric;
        m.data.append(reinterpret_cast<const char*>(&rm), sizeof(rm));
        ++it;
        ++count;
      }

      if (it == table.cend() && request) {
        m.type = message_type::route_info_and_request;
      }
      broadcast(std::move(m));
    }
#endif
  }

 private:
  void init_(bool enable_dv_routing) {
    impl_->set_recv_handle([this](const std::string& payload, lqs_t lqs) {
      bool ok = false;
      auto msg = message::deserialize(payload, ok);
      if (ok) {
        this->dispatch(std::move(msg), lqs);
      } else {
        MESH_CORE_LOGV("deserialize error");
      }
    });

    if (enable_dv_routing) {
      route_info info;
      info.dst = addr_;
      info.metric = 0;
      route_table_.add(info);

      sync_route(true);
      run_interval(
          [this] {
            sync_route();
          },
          MESH_CORE_ROUTE_SYNC_INTERVAL_MS);

      run_interval(
          [this] {
            route_table_.check_expired(get_timestamp());
          },
          MESH_CORE_ROUTE_CHECK_EXPIRED_INTERVAL_MS);
    }
  }

  void run_interval(std::function<void()> handle, uint32_t ms) {
    auto task = new std::function<void()>();  // no delete/cancel for now
    *task = [this, handle = std::move(handle), ms, task]() {
      handle();
      impl_->run_delay(*task, ms);
    };
    impl_->run_delay(*task, ms);
  }

  message create_message(message_type type, addr_t dst) {
    message m;
    m.type = type;
    m.src = addr_;
    m.dst = dst;
    m.seq = seq_++;
    m.ttl = TTL_DEFAULT;
    m.ts = impl_->get_timestamp_ms();
    return m;
  }

  void broadcast(message msg) {
#ifdef MESH_CORE_ENABLE_BROADCAST_INTERCEPTOR
    /// broadcast interceptor
    if (broadcast_interceptor_) {
      bool should_continue = broadcast_interceptor_(msg);
      if (!should_continue) {
        MESH_CORE_LOGD("broadcast: interceptor abort");
        return;
      }
    }
#endif

    /// broadcast message
    bool ok;
    auto payload = msg.serialize(ok);
    if (ok) {
      impl_->broadcast(std::move(payload));
    } else {
      MESH_CORE_LOGE("data size > %d", message::DataSizeMax);
    }
  }

  void dispatch(message msg, lqs_t lqs) {
    // clang-format off
    MESH_CORE_LOGD("=>: self: 0x%02X, type: %d, src: 0x%02X, dst: 0x%02X, next_hop: 0x%02X, seq: %u, ttl: %u, ts: 0x%08" PRIX32 ", lqs: %d, data: %s",
                   addr_, (int)msg.type, msg.src, msg.dst, msg.next_hop, msg.seq, msg.ttl, msg.ts, lqs, msg.data.c_str());
    // clang-format on

#ifdef MESH_CORE_ENABLE_DISPATCH_INTERCEPTOR
    /// interceptor
    if (dispatch_interceptor_) {
      bool should_continue = dispatch_interceptor_(msg);
      if (!should_continue) {
        MESH_CORE_LOGD("dispatch: interceptor abort");
        return;
      }
    }
#endif

    /// filter
    if (!message_filter(msg)) {
      return;
    }

    /// dispatch
    switch (msg.type) {
      case message_type::route_info:
      case message_type::route_info_and_request: {
        dispatch_route_info(msg, lqs);
#ifdef MESH_CORE_LOG_SHOW_DEBUG
        dump_debug();
#endif
        if (msg.type == message_type::route_info_and_request) {
          sync_route(false);
        }
        return;
      } break;
      case message_type::route_debug_send:
      case message_type::route_debug_back:
      case message_type::user_data: {
        dispatch_userdata(std::move(msg));
        return;
      } break;
      case message_type::broadcast:
      case message_type::sync_time: {
        dispatch_any_broadcast(std::move(msg));
        return;
      } break;
    }
  }

  void dispatch_route_info(const message& message, lqs_t lqs) {
    auto route_msg_ptr = (route_msg*)(message.data.data());
    int route_msg_num = (int)(message.data.size() / sizeof(route_msg));

    MESH_CORE_LOGD("update route info: size: %d", route_msg_num);
    for (int i = 0; i < route_msg_num; ++i) {
      auto route_msg = route_msg_ptr + i;
      MESH_CORE_LOGD("dst: 0x%02X, next_hop: 0x%02X, metric: %d", route_msg->dst, route_msg->next_hop, route_msg->metric);
      if (route_msg->next_hop == this->addr_) {
        MESH_CORE_LOGD("ignore next_hop is self");
        continue;
      }
      if (route_msg->metric >= MESH_CORE_TTL_DEFAULT) {
        MESH_CORE_LOGD("ignore metric max");
        continue;
      }
      auto info_old = route_table_.find_node(route_msg->dst);
      route_info info_new;
      info_new.dst = route_msg->dst;
      info_new.next_hop = route_msg->next_hop;
      info_new.metric = route_msg->metric + 1;
      info_new.lqs = lqs;
      info_new.expired = get_timestamp();
      if (info_old == nullptr) {
        route_table_.add(info_new);
      } else {
        if ((info_new.metric < info_old->metric) || (info_new.metric == info_old->metric && info_new.lqs > info_old->lqs)) {
          *info_old = info_new;
        } else if (info_new.metric == info_old->metric) {
          info_old->expired = get_timestamp();
        } else {
          MESH_CORE_LOGD("ignore route item");
        }
      }
    }
  }

  void dispatch_userdata(message msg) {
#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
    if (msg.type == message_type::route_debug_send) {
      msg.data.append(">" + std::to_string(addr_));
    } else if (msg.type == message_type::route_debug_back) {
      msg.data.append("<" + std::to_string(addr_));
    }
#endif

    if (msg.dst == this->addr_) {
      if (msg.type == message_type::user_data) {
        if (on_recv_handle_) on_recv_handle_(msg.src, std::move(msg.data));
      }
#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
      else if (msg.type == message_type::route_debug_send) {
        if (on_recv_debug_handle_) on_recv_debug_handle_(msg.src, std::move(msg.data));
        send_route_debug(msg.src, false);
      } else if (msg.type == message_type::route_debug_back) {
        if (on_recv_debug_handle_) on_recv_debug_handle_(msg.src, std::move(msg.data));
      }
#endif
      return;
    }

#ifdef MESH_CORE_DISABLE_ROUTE
    MESH_CORE_LOGD("drop: disable route");
    return;
#else
    if (--msg.ttl == 0) {
      MESH_CORE_LOGD("drop: ttl=0, src: 0x%02X, seq: %u", msg.src, msg.seq);
      return;
    }
    if (msg.next_hop != this->addr_) {
      MESH_CORE_LOGD("drop: route not me");
      return;
    }
    auto info = route_table_.find_node(msg.dst);
    if (info == nullptr) {
      MESH_CORE_LOGD("drop: no route");
      return;
    }
    msg.next_hop = info->next_hop;
    MESH_CORE_LOGD("next hop: 0x%02X, ttl = %u", msg.next_hop, msg.ttl);
    broadcast(std::move(msg));
#endif
  }

  void dispatch_any_broadcast(message msg) {
    /// special message check
    if (msg.type == message_type::broadcast) {
      if (on_recv_handle_) on_recv_handle_(msg.src, msg.data);  // do not move data
    }
#ifdef MESH_CORE_ENABLE_TIME_SYNC
    else if (msg.type == message_type::sync_time) {
      MESH_CORE_LOGD("sync ts: ttl=0, src: 0x%02X, seq: %u", msg.src, msg.seq);
      if (time_sync_handle_) time_sync_handle_(msg.ts);
    }
#endif

    /// rebroadcast message
#ifndef MESH_CORE_DISABLE_ROUTE
    {
      if (--msg.ttl == 0) {
        MESH_CORE_LOGD("drop: ttl=0, src: 0x%02X, seq: %u", msg.src, msg.seq);
        return;
      }

      MESH_CORE_LOGD("rebroadcast: ttl = %u", msg.ttl);
      impl_->run_delay(
          [this, msg = std::move(msg)]() mutable {
            broadcast(std::move(msg));
          },
          random(DELAY_MIN, DELAY_MAX));
    }
#endif
  }

  uint32_t random(uint32_t l, uint32_t r) {
    return utils::time_based_random(impl_->get_timestamp_ms() + addr_ + seq_, l, r);
  }

  bool message_filter(message& msg) {
    /// self check
    if (msg.src == this->addr_) {
      MESH_CORE_LOGD("filter: self msg");
      return false;
    }

    /// ttl check
    if (msg.ttl > TTL_DEFAULT) {
      MESH_CORE_LOGD("filter: ttl error: %u", msg.ttl);
      return false;
    }

    /// cache manager
    auto uuid = msg.cal_uuid();
    if (msg_uuid_cache_.exists(uuid)) {
      MESH_CORE_LOGD("filter: msg is old, src: 0x%02X, seq: %u, uuid: 0x%08" PRIX32, msg.src, msg.seq, uuid);
      return false;
    }
    msg_uuid_cache_.put(uuid);
    return true;
  }

 private:
  Impl* impl_{};
  addr_t addr_{};
  seq_t seq_{};
  detail::lru_record<msg_uuid_t> msg_uuid_cache_{LRU_RECORD_SIZE};
  route_table route_table_;
  on_recv_handle_t on_recv_handle_;

#ifdef MESH_CORE_ENABLE_TIME_SYNC
  time_sync_handle_t time_sync_handle_;
#endif

#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
  on_recv_debug_handle_t on_recv_debug_handle_;
#endif

#ifdef MESH_CORE_ENABLE_BROADCAST_INTERCEPTOR
  broadcast_interceptor_t broadcast_interceptor_;
#endif

#ifdef MESH_CORE_ENABLE_DISPATCH_INTERCEPTOR
  dispatch_interceptor_t dispatch_interceptor_;
#endif
};

}  // namespace mesh_core
