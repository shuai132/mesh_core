#pragma once

// first include
#include "mesh_core/config.hpp"

// other include
#include "mesh_core/detail/copyable.hpp"
#include "mesh_core/detail/log.h"
#include "mesh_core/detail/lru_record.hpp"
#include "mesh_core/detail/message.hpp"
#include "mesh_core/detail/noncopyable.hpp"
#include "mesh_core/type.hpp"
#include "mesh_core/utils.hpp"

// std
#include <functional>
#include <string>

namespace mesh_core {

template <typename Impl>
class mesh : detail::noncopyable {
 public:
  explicit mesh(Impl* impl) : impl_(impl) {
    init();
  };

  /**
   *
   * @param addr
   */
  void set_addr(addr_t addr) {
    if (addr >= MESH_CORE_ADDR_RESERVED_BEGIN) {
      MESH_CORE_LOGD("addr error");
    } else {
      addr_ = addr;
    }
  }

  addr_t get_addr() {
    return addr_;
  }

  void enable_routing(bool enable) {
    enable_routing_ = enable;
  }

  void router_only() {
    addr_ = MESH_CORE_ADDR_ROUTER;
    enable_routing(true);
  }

  bool is_router_only() {
    return addr_ == MESH_CORE_ADDR_ROUTER;
  }

  void send(addr_t addr, data_t data) {
    if (data.size() > detail::message::DataSizeMax) {
      MESH_CORE_LOGE("data size > %d", detail::message::DataSizeMax);
      return;
    }
    detail::message m;
    m.src = addr_;
    m.dest = addr;
    m.seq = seq_++;
    m.ttl = TTL_DEFAULT;
    m.ts = impl_->get_timestamp_ms();
    m.data = std::move(data);
    m.finalize();
    broadcast(std::move(m));
  }

  void on_recv(recv_handle_t handle) {
    recv_handle_ = std::move(handle);
  }

  void on_sync_time(time_sync_handle_t handle) {
    time_sync_handle_ = std::move(handle);
  }

  timestamp_t get_timestamp() {
    return ts_;
  }

  void sync_time() {
    ts_ = impl_->get_timestamp_ms();
    detail::message m;
    m.src = addr_;
    m.dest = MESH_CORE_ADDR_BROADCAST;
    m.seq = seq_++;
    m.ttl = TTL_DEFAULT;
    m.ts = ts_;
    broadcast(std::move(m));
  }

 private:
  void init() {
    impl_->set_recv_handle([this](const std::string& payload) {
      bool ok = false;
      auto msg = detail::message::deserialize(payload, ok);
      if (ok) {
        this->dispatch(std::move(msg));
      } else {
        MESH_CORE_LOGE("deserialize error");
      }
    });
  }

  void broadcast(detail::message data) {
    bool ok;
    auto payload = data.serialize(ok);
    if (ok) {
      impl_->broadcast(std::move(payload));
    } else {
      MESH_CORE_LOGE("data size > %d", detail::message::DataSizeMax);
    }
  }

  void dispatch(detail::message message) {
    MESH_CORE_LOGD("=>: self: 0x%02X, src: 0x%02X, dest: 0x%02X, seq: %u, ttl: %u, ts: 0x%04X, data: %s", addr_, message.src, message.dest,
                   message.seq, message.ttl, message.ts, message.data.c_str());
    /// self check
    if (message.src == this->addr_) {
      MESH_CORE_LOGD("drop: self message");
      return;
    }

    /// ttl check
    if (message.ttl > TTL_DEFAULT) {
      MESH_CORE_LOGD("drop: ttl error: %u", message.ttl);
      return;
    }

    /// cache manager
    auto uuid = message.cal_uuid();
    if (msg_uuid_cache_.exists(uuid)) {
      MESH_CORE_LOGD("drop: msg is old, src: 0x%02X, seq: %u, uuid: 0x%08X", message.src, message.seq, uuid);
      return;
    }
    msg_uuid_cache_.put(uuid);

    /// special message check
    bool need_rebroadcast = true;
    if (message.dest == this->addr_) {
      need_rebroadcast = false;
      if (recv_handle_) recv_handle_(message.src, std::move(message.data));
    } else if (message.dest == MESH_CORE_ADDR_BROADCAST) {
      if (recv_handle_) recv_handle_(message.src, message.data);  // do not move data
    } else if (message.dest == MESH_CORE_ADDR_SYNC_TIME) {
      MESH_CORE_LOGD("sync ts: ttl=0, src: 0x%02X, seq: %u", message.src, message.seq);
      ts_ = message.ts;
      if (time_sync_handle_) time_sync_handle_(ts_);
    }

    /// rebroadcast message
    if (enable_routing_ && need_rebroadcast) {
      if (--message.ttl == 0) {  // drop message
        MESH_CORE_LOGD("drop: ttl=0, src: 0x%02X, seq: %u", message.src, message.seq);
        return;
      }

      MESH_CORE_LOGD("rebroadcast: ttl = %u", message.ttl);
      impl_->run_delay(
          [this, message = std::move(message)]() mutable {
            broadcast(std::move(message));
          },
          random(DELAY_MIN, DELAY_MAX));
    }
  }

  uint16_t random(uint16_t l, uint16_t r) {
    return utils::time_based_random(impl_->get_timestamp_ms() + addr_ + seq_, l, r);
  }

 private:
  recv_handle_t recv_handle_;
  time_sync_handle_t time_sync_handle_;
  addr_t addr_{MESH_CORE_ADDR_ROUTER};
  seq_t seq_{};
  detail::lru_record<msg_uuid_t> msg_uuid_cache_{LRU_RECORD_SIZE};
  timestamp_t ts_{};
  bool enable_routing_{true};
  Impl* impl_{};
};

}  // namespace mesh_core
