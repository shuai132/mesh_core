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

  void set_addr(addr_t addr) {
    addr_ = addr;
  }

  addr_t get_addr() {
    return addr_;
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
    m.dest = ADDR_BROADCAST;
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
      impl_->broadcast(payload);
    } else {
      MESH_CORE_LOGE("data size > %d", detail::message::DataSizeMax);
    }
  }

  void dispatch(detail::message message) {
    MESH_CORE_LOGD("=>: self: 0x%02X, src: 0x%02X, dest: 0x%02X, seq: %u, ttl: %u, ts: 0x%04X, data: %s", addr_, message.src, message.dest,
                   message.seq, message.ttl, message.ts, message.data.c_str());
    if (message.src == this->addr_) {  // drop message
      MESH_CORE_LOGD("drop: self message");
      return;
    }

    if (message.ttl > TTL_DEFAULT) {  // message error
      MESH_CORE_LOGE("drop: ttl error: %u", message.ttl);
      return;
    }
    auto uuid = message.cal_uuid();
    if (msg_uuid_cache_.exists(uuid)) {
      MESH_CORE_LOGD("drop: msg is old, src: 0x%02X, seq: %u, uuid: 0x%08X", message.src, message.seq, uuid);
      return;
    }

    if (message.dest == this->addr_ || message.dest == ADDR_BROADCAST) {
      msg_uuid_cache_.put(uuid);
      if (message.dest == this->addr_) {
        if (recv_handle_) recv_handle_(message.src, std::move(message.data));
      } else {  // is broadcast
        // sync timestamp
        MESH_CORE_LOGD("sync ts: ttl=0, src: 0x%02X, seq: %u", message.src, message.seq);
        ts_ = message.ts;
        if (time_sync_handle_) time_sync_handle_(ts_);
      }
    } else {
      if (--message.ttl == 0) {  // drop message
        MESH_CORE_LOGD("drop: ttl=0, src: 0x%02X, seq: %u", message.src, message.seq);
        return;
      }

      MESH_CORE_LOGD("rebroadcast message, ttl = %u", message.ttl);
      msg_uuid_cache_.put(uuid);
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
  addr_t addr_{ADDR_DEFAULT};
  seq_t seq_{};
  detail::lru_record<msg_uuid_t> msg_uuid_cache_{LRU_RECORD_SIZE};
  timestamp_t ts_{};
  Impl* impl_{};
};

}  // namespace mesh_core
