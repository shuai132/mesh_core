#pragma once

// config
#include "config.hpp"

// std
#include <cstdint>
#include <functional>
#include <string>

namespace mesh_core {

#define MESH_CORE_UNUSED(x) (void)x

/// type define
using addr_t = uint8_t;
using seq_t = uint8_t;
using ttl_t = uint8_t;
using data_t = std::string;
using timestamps_t = uint16_t;
using msg_uuid_t = uint32_t;

/// assert
static_assert(std::is_trivial<addr_t>::value, "");
static_assert(std::is_trivial<seq_t>::value, "");
static_assert(std::is_trivial<ttl_t>::value, "");
static_assert(std::is_trivial<msg_uuid_t>::value, "");
static_assert(sizeof(msg_uuid_t) >= sizeof(addr_t) + sizeof(seq_t), "msg_uuid: [src, seq]");

/// handle
using recv_handle_t = std::function<void(addr_t, data_t)>;

/// default value
const addr_t ADDR_DEFAULT = 0x00;
const addr_t ADDR_BROADCAST = 0xFF;
const ttl_t TTL_DEFAULT = 5;
const int LRU_RECORD_SIZE = 32;

}  // namespace mesh_core
