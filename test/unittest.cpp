#include <unordered_map>
#include <utility>

#include "asio.hpp"
#include "assert_def.h"
#include "mesh_core.hpp"
#include "mesh_core/utils.hpp"

static asio::io_context s_io_context;
static std::vector<mesh_core::recv_handle_t> recv_handles;

struct Impl {
  mesh_core::addr_t addr;

  explicit Impl(mesh_core::addr_t addr) : addr(addr) {}

  void broadcast(const std::string& data) const {
    for (int i = 0; i < (int)recv_handles.size(); ++i) {
      if (std::abs(addr - i) > 1) {
        continue;
      }
      recv_handles[i](data, 0);
    }
  }

  static void set_recv_handle(mesh_core::recv_handle_t handle) {
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

  bool TEST_FLAG_RECV_HELLO = false;
  bool TEST_FLAG_RECV_WORLD = false;
  std::unordered_map<int, mesh_core::timestamp_t> TEST_FLAG_BROADCAST{};

#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
  bool TEST_FLAG_ROUTE_DEBUG_SEND = false;
  bool TEST_FLAG_ROUTE_DEBUG_BACK = false;
#endif

#ifdef MESH_CORE_ENABLE_TIME_SYNC
  std::unordered_map<int, mesh_core::timestamp_t> TEST_FLAG_SYNC_TIME{};
  mesh_core::timestamp_t TEST_FLAG_SYNC_TIME_TS{};
#endif

  std::vector<std::unique_ptr<Impl>> impl_list;
  std::vector<std::unique_ptr<mesh_core::mesh<Impl>>> mesh_list;
  for (int i = 0; i < 10; ++i) {
    auto impl = std::make_unique<Impl>(i);
    auto mesh = std::make_unique<mesh_core::mesh<Impl>>(impl.get());
    mesh->init(i);
    mesh->on_recv([&, i](mesh_core::addr_t addr, const mesh_core::data_t& data) {
      MESH_CORE_LOG("id: %d: recv from addr: 0x%02X, data: %s", i, addr, data.c_str());
      if (data == "broadcast") {
        ASSERT(TEST_FLAG_BROADCAST[i] == false);
        TEST_FLAG_BROADCAST[i] = true;
        return;
      }
      switch (addr) {
        case 0: {
          ASSERT(i == 9);
          ASSERT(data == "hello");
          ASSERT(!TEST_FLAG_RECV_HELLO);
          TEST_FLAG_RECV_HELLO = true;
        } break;
        case 9: {
          ASSERT(i == 0);
          ASSERT(data == "world");
          ASSERT(!TEST_FLAG_RECV_WORLD);
          TEST_FLAG_RECV_WORLD = true;
        } break;
        default: {
          ASSERT(false);
        }
      }
    });
#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
    mesh->on_recv_debug([&, i](mesh_core::addr_t addr, const mesh_core::data_t& data) {
      MESH_CORE_LOG("id: %d: recv debug from addr: 0x%02X, data: %s", i, addr, data.c_str());
      switch (addr) {
        case 0: {
          ASSERT(i == 9);
          ASSERT(data == "0>1>2>3>4>5>6>7>8>9");
          ASSERT(!TEST_FLAG_ROUTE_DEBUG_SEND);
          TEST_FLAG_ROUTE_DEBUG_SEND = true;
        } break;
        case 9: {
          ASSERT(i == 0);
          ASSERT(data == "9<8<7<6<5<4<3<2<1<0");
          ASSERT(!TEST_FLAG_ROUTE_DEBUG_BACK);
          TEST_FLAG_ROUTE_DEBUG_BACK = true;
        } break;
        default: {
          ASSERT(false);
        }
      }
    });
#endif
#ifdef MESH_CORE_ENABLE_TIME_SYNC
    mesh->on_sync_time([&, i](mesh_core::timestamp_t ts) {
      MESH_CORE_LOG("id: %d: on_sync_time: 0x%08X", i, ts);
      ASSERT(TEST_FLAG_SYNC_TIME[i] == 0);
      TEST_FLAG_SYNC_TIME[i] = ts;
    });
#endif
    impl_list.push_back(std::move(impl));
    mesh_list.push_back(std::move(mesh));
  }

  asio::post(s_io_context, [&] {
    // ensure route is synced
    for (int i = 0; i < 10; ++i) {
      mesh_list[9 - i]->sync_route();
    }
    // start send test
    mesh_list[0]->send(9, "hello");
    mesh_list[9]->send(0, "world");
    mesh_list[0]->broadcast("broadcast");
#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
    mesh_list[0]->send_route_debug(9);
#endif
#ifdef MESH_CORE_ENABLE_TIME_SYNC
    TEST_FLAG_SYNC_TIME_TS = mesh_list[0]->sync_time();
    MESH_CORE_LOG("TEST_FLAG_SYNC_TIME_TS: 0x%08X", TEST_FLAG_SYNC_TIME_TS);
#endif
  });

  asio::steady_timer timer(s_io_context);
  timer.expires_after(std::chrono::milliseconds(300));
  timer.async_wait([&](const asio::error_code&) {
    ASSERT(TEST_FLAG_RECV_HELLO);
    ASSERT(TEST_FLAG_RECV_WORLD);
    ASSERT(TEST_FLAG_BROADCAST.size() == 9);
    for (size_t i = 1; i < TEST_FLAG_BROADCAST.size(); ++i) {
      ASSERT(TEST_FLAG_BROADCAST[i] == true);
    }
#ifdef MESH_CORE_ENABLE_TIME_SYNC
    ASSERT(TEST_FLAG_SYNC_TIME.size() == 9);
    for (size_t i = 1; i < TEST_FLAG_SYNC_TIME.size(); ++i) {
      ASSERT(TEST_FLAG_SYNC_TIME[i] == TEST_FLAG_SYNC_TIME_TS);
    }
#endif
#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
    ASSERT(TEST_FLAG_ROUTE_DEBUG_SEND);
    ASSERT(TEST_FLAG_ROUTE_DEBUG_BACK);
#endif
    MESH_CORE_LOG("All Test Passed!");
    s_io_context.stop();
  });

  auto worker = asio::make_work_guard(s_io_context);
  s_io_context.run();
  return 0;
}
