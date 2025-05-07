#include <utility>

#include "asio.hpp"
#include "assert_def.h"
#include "mesh_core.hpp"
#include "mesh_core/utils.hpp"

static asio::io_context s_io_context;

struct Impl {
  std::vector<mesh_core::recv_handle_t> recv_handles;

  void broadcast(const std::string& data) {
    for (const auto& item : recv_handles) {
      asio::post(s_io_context, [&item, data] {
        item(data, 0);
      });
    }
  }

  void set_recv_handle(mesh_core::recv_handle_t handle) {
    recv_handles.push_back(std::move(handle));
  }

  static mesh_core::timestamp_t get_timestamp_ms() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return static_cast<mesh_core::timestamp_t>(ms & 0xFFFFFFFF);
  }

  static void run_delay(std::function<void()> handle, uint32_t ms) {
    auto timer = std::make_shared<asio::steady_timer>(s_io_context);
    timer->expires_after(std::chrono::milliseconds(ms));
    timer->async_wait([handle, timer](const asio::error_code&) mutable {
      handle();
      timer = nullptr;
    });
  }
};

static void test_message() {
  {
    // test cal_uuid
    mesh_core::message m;
    m.src = 0x12;
    m.seq = 0x34;
    m.ts = 0x5678;
    auto uuid = m.cal_uuid();
    ASSERT(uuid == 0x12345678);
  }
  {
    // test serialize
    mesh_core::message m;
    m.data = "hello";
    m.finalize();
    bool ok;
    auto payload = m.serialize(ok);
    MESH_CORE_LOG("payload:");
    MESH_CORE_LOG_HEX(payload.data(), payload.size());
    ASSERT(ok);
    auto m2 = mesh_core::message::deserialize(payload, ok);
    ASSERT(ok);
    ASSERT(m.data == m2.data);
    ASSERT(m.crc == m2.crc);
  }
}

static void test_random() {
  for (int i = 0; i < 10; ++i) {
    auto val = mesh_core::utils::time_based_random(0x1234 + i, 100, 300);
    MESH_CORE_LOG("random: 0x%04X, %u", val, val);
    ASSERT(val >= 100 && val <= 300);
  }
}

int main() {
  MESH_CORE_LOG("version: %d", MESH_CORE_VERSION);
  test_message();
  test_random();

  using namespace mesh_core;
  Impl impl;

  std::vector<std::unique_ptr<mesh<Impl>>> mesh_list;
  for (int i = 0; i < 10; ++i) {
    auto m = std::make_unique<mesh<Impl>>(&impl);
    m->init(i);
    m->on_recv([i](addr_t addr, const data_t& data) {
      MESH_CORE_LOG("id: %d: recv from addr: 0x%02X, data: %s", i, addr, data.c_str());
    });
    mesh_list.push_back(std::move(m));
  }

  asio::post(s_io_context, [&] {
    mesh_list[2]->send(0x05, "hello");
    mesh_list[5]->send(0x08, "world");
  });

  asio::steady_timer timer(s_io_context);
  timer.expires_after(std::chrono::seconds(1));
  timer.async_wait([](const asio::error_code&) {
    s_io_context.stop();
  });

  auto worker = asio::make_work_guard(s_io_context);
  s_io_context.run();
  return 0;
}
