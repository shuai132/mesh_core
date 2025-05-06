# mesh_core

[![Build Status](https://github.com/shuai132/mesh_core/workflows/CI/badge.svg)](https://github.com/shuai132/mesh_core/actions?workflow=CI)

A lightweight mesh library, based on distance-vector routing algorithm.

## Features

* Header-Only
* Flooding based broadcast
* Distance vector routing algorithm
* Easy to use, understand, and debug
* Support most microchips, e.g. STM32, ESP32...

## Protocol

```text
/// ┌─────────┬──────────────┬───────────────────┬───────────────────────────────┐
/// │ Bytes   │ Field        │ Default/Example   │ Description                   │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 1       │ head         │ 0x3C              │ Header identifier: Mesh Core  │
/// │ 1       │ ver          │ 0x00              │ Protocol version              │
/// │ 1       │ len          │ 0x00              │ Payload length in bytes       │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 1/2     │ type         │ 0x0               │ Message type                  │
/// │ 1/2     │ ttl          │ 0x0               │ Hops                          │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 1       │ src          │ 0x00              │ Source address                │
/// │ 1       │ dst          │ 0x00              │ Destination address           │
/// │ 1       │ seq          │ 0x00              │ Sequence number               │
/// │ 4       │ ts           │ 0x00000000        │ Timestamp for milliseconds    │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ n       │ route_infos  │ [variable]        │ Route info list               │
/// │---------│--------------│-------------------│-------------------------------│
/// │ 1       │ next_hop     │ 0x00              │ Next hop                      │
/// │ n       │ data         │ [variable]        │ Payload for user data         │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 2       │ crc          │ 0x0000            │ CRC-16 of all preceding fields│
/// └─────────┴──────────────┴───────────────────┴───────────────────────────────┘
```

## Usage

* usage template

  [test/usage.cpp](test/usage.cpp)

```c++
#include "mesh_core.hpp"

/**
 * supply your implementation:
 *
 * NOTE:
 * 1. broadcast and recv_handle should ensure packet is complete
 * 2. all methods can be static
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
```

* unittest

  [test/unittest.cpp](test/unittest.cpp)

* UDP demo

  [test/udp_mesh.cpp](test/udp_mesh.cpp)
