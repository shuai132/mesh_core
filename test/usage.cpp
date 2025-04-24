#include "mesh_core.hpp"

/**
 * supply your implementation:
 *
 * NOTE:
 * 1. broadcast and recv_handle should ensure packet is complete
 * 2. all methods can be static
 * 3. `run_delay` and `random` is optional, but it is strongly recommended to implement them.
 */
struct Impl {
  /**
   * @param data binary data to broadcast
   */
  void broadcast(std::string data) {
    (void)(data);
  }

  /**
   * @param handle store it, call it on receive broadcast.
   */
  void set_recv_handle(std::function<void(std::string)> handle) {
    (void)(handle);
  }

  /**
   * @param handle call it after `ms`
   * @param ms milliseconds
   */
  void run_delay(std::function<void()> handle, int ms) {
    (void)(handle);
    (void)(ms);
  }

  /**
   * @return value between `l` and `r`
   */
  static int random(int l, int r) {
    (void)(l);
    (void)(r);
    return 0;
  }
};

int main() {
  Impl impl;
  mesh_core::mesh<Impl> mesh(&impl);
  mesh.set_addr(0x00);
  mesh.on_recv([](mesh_core::addr_t addr, const mesh_core::data_t& data) {
    MESH_CORE_LOG("addr: 0x%02X, data: %s", addr, data.c_str());
  });
  mesh.send(0x01, "hello");
  return 0;
}
