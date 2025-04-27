# mesh_core

[![Build Status](https://github.com/shuai132/mesh_core/workflows/CI/badge.svg)](https://github.com/shuai132/mesh_core/actions?workflow=CI)

A lightweight mesh solution, works best for small networks.

Sophisticated routing protocols can be overly complex and error-prone. For networks with limited nodes, using **flooding
with optimizations**  can actually create a more robust mesh system.

## Features

* Header-Only
* No route-table
* Easy to use, understand, and debug.

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
  static void run_delay(std::function<void()> handle, int ms) {
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

## Protocol

```text
/// ┌─────────┬──────────────┬───────────────────┬───────────────────────────────┐
/// │ Bytes   │ Field        │ Default/Example   │ Description                   │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 1       │ head         │ 0x3C              │ Header identifier: Mesh Core  │
/// │ 1       │ ver          │ 0x00              │ Protocol version              │
/// │ 1       │ len          │ 0x00              │ Payload length in bytes       │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ 1       │ src          │ 0x00              │ Source address                │
/// │ 1       │ dest         │ 0x00              │ Destination address           │
/// │ 1       │ seq          │ 0x00              │ Sequence number               │
/// │ 1       │ ttl          │ 0x00              │ Time To Live (hops)           │
/// │ 2       │ ts           │ 0x0000            │ Timestamp (16-bit)            │
/// ├─────────┼──────────────┼───────────────────┼───────────────────────────────┤
/// │ n       │ data         │ [variable]        │ Payload data (len bytes)      │
/// │ 2       │ crc          │ 0x0000            │ CRC-16 of all preceding fields│
/// └─────────┴──────────────┴───────────────────┴───────────────────────────────┘
/// 11 bytes without data
```

* unittest

  [test/unittest.cpp](test/unittest.cpp)

* UDP demo

  [test/udp_mesh.cpp](test/udp_mesh.cpp)
