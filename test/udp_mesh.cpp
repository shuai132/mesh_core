#include <iostream>
#include <utility>

#include "asio.hpp"
#include "mesh_core.hpp"

static asio::io_context s_io_context;
static asio::ip::udp::socket s_socket(s_io_context);
const asio::ip::udp::endpoint broadcast_endpoint(asio::ip::address_v4::broadcast(), 12345);

static void udp_broadcast(std::string data);
static mesh_core::addr_t self_addr;

struct Impl {
  mesh_core::recv_handle_t recv_handle;

  static void broadcast(std::string data) {
    udp_broadcast(std::move(data));
  }

  void set_recv_handle(mesh_core::recv_handle_t handle) {
    recv_handle = std::move(handle);
  }

  static mesh_core::timestamp_t get_timestamp_ms() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return static_cast<mesh_core::timestamp_t>(ms & 0xFFFFFFFF);
  }

  static void run_delay(std::function<void()> handle, uint32_t ms) {
    auto timer = std::make_shared<asio::steady_timer>(s_io_context);
    timer->expires_after(std::chrono::milliseconds(ms));
    timer->async_wait([handle = std::move(handle), timer](const asio::error_code&) mutable {
      handle();
      timer = nullptr;
    });
  }
};

static void udp_broadcast(std::string data) {
  MESH_CORE_LOGD("UDP send size: %zu", data.size());
  MESH_CORE_LOGD_HEX_H(data.data(), data.size());
  MESH_CORE_LOGD_HEX_D(data.data(), data.size());
  MESH_CORE_LOGD_HEX_C(data.data(), data.size());
  s_socket.async_send_to(asio::buffer(data), broadcast_endpoint, [](const asio::error_code& ec, std::size_t) {
    if (ec) {
      MESH_CORE_LOGE("Send error: %s", ec.message().c_str());
    }
  });
}

static void start_recv(Impl& impl) {
  static std::array<char, 1024> recv_buffer{};
  static asio::ip::udp::endpoint sender_endpoint;
  s_socket.async_receive_from(asio::buffer(recv_buffer), sender_endpoint, [&](const asio::error_code& ec, std::size_t bytes_received) {
    if (!ec) {
      std::string payload = std::string(recv_buffer.data(), bytes_received);
      MESH_CORE_LOGD("UDP recv size: %zu", payload.size());
      MESH_CORE_LOGD_HEX_H(payload.data(), payload.size());
      MESH_CORE_LOGD_HEX_D(payload.data(), payload.size());
      MESH_CORE_LOGD_HEX_C(payload.data(), payload.size());
      {
        // ignore msg which addr diff >= 10, for test routing
        bool ok;
        auto msg = mesh_core::message::deserialize(payload, ok);
        if (ok) {
          auto recv_from = static_cast<mesh_core::addr_t>(msg.ts >> 24);
          if (std::abs(recv_from - self_addr) < 10) {
            impl.recv_handle(payload, 0);
          } else {
            MESH_CORE_LOGD("ignore from: 0x%02X", recv_from);
          }
        } else {
          MESH_CORE_LOGD("deserialize error");
        }
      }
      start_recv(impl);
    } else {
      MESH_CORE_LOGE("UDP error: %s", ec.message().c_str());
    }
  });
}

static void init_udp_impl(Impl& impl) {
  s_socket.open(asio::ip::udp::v4());
  s_socket.set_option(asio::socket_base::broadcast(true));
  s_socket.set_option(asio::socket_base::reuse_address(true));
  s_socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 12345));
  start_recv(impl);
}

int main() {
  int addr;
  std::cout << "set addr: ";
  std::cin >> addr;
  printf("addr is: 0x%02X\n", addr);
  self_addr = addr;

  using namespace mesh_core;
  Impl impl;
  init_udp_impl(impl);
  mesh<Impl> mesh(&impl);
  mesh.init(addr);
  mesh.on_recv([](addr_t addr, const data_t& data) {
    MESH_CORE_LOG("addr: 0x%02X, data: %s", addr, data.c_str());
  });
#ifdef MESH_CORE_ENABLE_TIME_SYNC
  mesh.on_sync_time([](timestamp_t ts) {
    MESH_CORE_LOG("on_sync_time: 0x%08X", ts);
  });
#endif
#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
  mesh.on_recv_debug([](addr_t addr, const data_t& data) {
    MESH_CORE_LOG("debug: addr: 0x%02X, data: %s", addr, data.c_str());
  });
#endif

  std::thread([&] {
    for (;;) {
      int input;
      std::string message;
      std::cout << "to addr: ";
      std::cin >> input;

      /// test sync_time
      if (input == -1) {
        asio::post(s_io_context, [&mesh] {
          mesh.sync_time();
          MESH_CORE_LOG("time: 0x%08X", mesh.get_timestamp());
        });
        continue;
      }

      /// test broadcast
      else if (input == -2) {
        std::cout << "message: ";
        std::cin >> message;
        printf("send broadcast: %s\n", message.c_str());
        asio::post(s_io_context, [=, &mesh] {
          mesh.broadcast(message);
        });
      }

      /// test dump_debug
      else if (input == -3) {
        printf("dump_debug: \n");
        asio::post(s_io_context, [=, &mesh] {
          mesh.dump_debug();
        });
      }

      /// debug route
      else if (input == -4) {
#ifdef MESH_CORE_ENABLE_ROUTE_DEBUG
        printf("debug route: \n");
        std::cout << "to addr: ";
        std::cin >> input;
        addr_t dst = input;
        printf("send to addr: 0x%02X\n", dst);
        asio::post(s_io_context, [=, &mesh] {
          mesh.send_route_debug(dst);
        });
#endif
      }

      /// test send message
      else {
        addr_t dst = input;
        std::cout << "message: ";
        std::cin >> message;
        printf("send to addr: 0x%02X, message: %s\n", dst, message.c_str());
        asio::post(s_io_context, [=, &mesh] {
          mesh.send(dst, message);
        });
      }
    }
  }).detach();

  auto worker = asio::make_work_guard(s_io_context);
  s_io_context.run();
  return 0;
}
