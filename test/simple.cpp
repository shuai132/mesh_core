#include <utility>

#include "mesh_core.hpp"
#include "mesh_core/detail/log.h"

struct Impl {
  std::function<void(std::string)> handle_;

  void broadcast(std::string data) {
    (void)(data);
  }

  void set_recv_handle(std::function<void(std::string)> handle) {
    handle_ = std::move(handle);
  }

  void run_delay(std::function<void()> handle, int ms) {
    (void)(handle);
    (void)(ms);
  }

  static int random(int l, int r) {
    (void)(l);
    (void)(r);
    return 0;
  }
};

int main() {
  using namespace mesh_core;
  Impl impl;
  mesh<Impl> mesh(&impl);
  mesh.set_addr(0x00);
  mesh.on_recv([](addr_t addr, const data_t& data) {
    MESH_CORE_LOG("addr: 0x%02X, data: %s", addr, data.c_str());
  });
  mesh.send(0x01, "hello");
  return 0;
}
