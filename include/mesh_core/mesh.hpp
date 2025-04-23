#pragma once

// first include
#include "mesh_core/config.hpp"

// other include
#include "mesh_core/detail/copyable.hpp"
#include "mesh_core/detail/log.h"
#include "mesh_core/detail/noncopyable.hpp"
#include "mesh_core/lru_record.hpp"
#include "mesh_core/message.hpp"
#include "mesh_core/type.hpp"

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

  void send(addr_t addr, data_t msg) {
    message m;
    m.src = addr_;
    m.dest = addr;
    m.seq = seq_++;
    m.ttl = TTL_DEFAULT;
    m.data = std::move(msg);
    broadcast(std::move(m));
  }

  void on_recv(recv_handle_t handle) {
    recv_handle_ = std::move(handle);
  }

 private:
  void init() {
    impl_->set_recv_handle([this](const std::string& payload) {
      bool ok = false;
      auto msg = message::deserialize(payload, ok);
      if (ok) {
        this->dispatch(std::move(msg));
      } else {
        MESH_CORE_LOGE("message deserialize error");
      }
    });
  }

  void broadcast(message data) {
    auto payload = data.serialize();
    impl_->broadcast(payload);
  }

  void dispatch(message message) {
    MESH_CORE_LOGD("=>: self: 0x%02X, src: 0x%02X, dest: 0x%02X, seq: %u, ttl: %u, data: %s", addr_, message.src, message.dest, message.seq,
                   message.ttl, message.data.c_str());
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
      MESH_CORE_LOGD("drop: msg is old, src: 0x%02X, seq: %u", message.src, message.seq);
      return;
    }

    if (message.dest == this->addr_) {
      msg_uuid_cache_.put(uuid);
      if (recv_handle_) {
        recv_handle_(message.src, std::move(message.data));
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
          impl_->random(10, 300));
    }
  }

 private:
  recv_handle_t recv_handle_;
  addr_t addr_ = ADDR_DEFAULT;
  seq_t seq_ = 0;
  lru_record<msg_uuid_t> msg_uuid_cache_{LRU_RECORD_SIZE};
  Impl* impl_ = nullptr;
};

}  // namespace mesh_core
