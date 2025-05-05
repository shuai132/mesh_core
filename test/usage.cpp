#include "mesh_core.hpp"

/**
 * supply your implementation:
 *
 * NOTE:
 * 1. broadcast and recv_handle should ensure packet is complete
 * 2. all methods can be static or non-static
 */
struct Impl {
  /**
   * @param data binary data to broadcast
   */
  static void broadcast(std::string data) {
    (void)(data);
  }

  /**
   * @param handle store it, call it on receive broadcast.
   */
  static void set_recv_handle(std::function<void(std::string, mesh_core::snr_t)> handle) {
    (void)(handle);
  }

  /**
   * @return millisecond timestamp
   */
  static mesh_core::timestamp_t get_timestamp_ms() {
    return 0;
  }

  /**
   * @param handle call it after `ms`
   * @param ms milliseconds
   */
  static void run_delay(std::function<void()> handle, uint32_t ms) {
    (void)(handle);
    (void)(ms);
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
