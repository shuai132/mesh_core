#include <iostream>
#include <random>
#include <utility>

#include "asio.hpp"
#include "mesh_core.hpp"
#include "mesh_core/detail/log.h"

static asio::io_context s_io_context;
static asio::ip::udp::socket s_socket(s_io_context);
const asio::ip::udp::endpoint broadcast_endpoint(asio::ip::address_v4::broadcast(), 12345);

static void udp_broadcast(std::string data);

struct Impl {
  std::function<void(std::string)> handle_;
  std::function<void(std::string)> broadcast_;

  static void broadcast(std::string data) {
    udp_broadcast(std::move(data));
  }

  void set_recv_handle(std::function<void(std::string)> handle) {
    handle_ = std::move(handle);
  }

  static void run_delay(std::function<void()> handle, int ms) {
    auto timer = std::make_shared<asio::steady_timer>(s_io_context);
    timer->expires_after(std::chrono::milliseconds(ms));
    timer->async_wait([handle = std::move(handle), timer](const asio::error_code&) mutable {
      handle();
      timer = nullptr;
    });
  }

  int random(int l, int r) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(l, r);
    return dist(gen);
  }
};

static void udp_broadcast(std::string data) {
  s_socket.set_option(asio::socket_base::broadcast(true));
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
      std::string message = std::string(recv_buffer.data(), bytes_received);
      MESH_CORE_LOGD("UPD message: %s", message.c_str());
      impl.handle_(message);
      start_recv(impl);
    } else {
      MESH_CORE_LOGE("UPD error: %s", ec.message().c_str());
    }
  });
}

void init_udp_impl(Impl& impl) {
  s_socket.open(asio::ip::udp::v4());
  s_socket.set_option(asio::socket_base::broadcast(true));
  s_socket.set_option(asio::socket_base::reuse_address(true));
  s_socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 12345));
  start_recv(impl);
}

void start_async_read(std::function<void(std::string)> handle = nullptr) {
  static std::function<void(std::string)> handle_;
  static asio::posix::stream_descriptor stdin_(s_io_context, ::dup(STDIN_FILENO));
  static std::string input_buffer;
  if (handle != nullptr) {
    handle_ = std::move(handle);
  }
  asio::async_read_until(stdin_, asio::dynamic_buffer(input_buffer), '\n', [](const asio::error_code& ec, std::size_t) {
    if (!ec) {
      input_buffer.pop_back();
      handle_(std::move(input_buffer));
      input_buffer.clear();
      start_async_read();
    } else {
      MESH_CORE_LOGE("read error: %s", ec.message().c_str());
    }
  });
}

int main() {
  uint8_t addr;
  uint8_t dest;
  std::cout << "input addr self:" << std::endl;
  std::cin >> addr;
  std::cout << "input addr dest:" << std::endl;
  std::cin >> dest;

  using namespace mesh_core;
  Impl impl;
  mesh<Impl> mesh(&impl);
  mesh.set_addr(addr);
  mesh.on_recv([](addr_t addr, const data_t& data) {
    MESH_CORE_LOG("addr: 0x%02X, data: %s", addr, data.c_str());
  });

  init_udp_impl(impl);

  start_async_read([&](std::string input) {
    mesh.send(dest, std::move(input));
  });

  auto worker = asio::make_work_guard(s_io_context);
  s_io_context.run();
  return 0;
}
