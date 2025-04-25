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
using time_sync_handle_t = std::function<void(timestamps_t)>;

/// default value
const addr_t ADDR_DEFAULT = MESH_CORE_BROADCAST_ADDR_DEFAULT;
const addr_t ADDR_BROADCAST = MESH_CORE_BROADCAST_ADDR_BROADCAST;
const ttl_t TTL_DEFAULT = MESH_CORE_BROADCAST_TTL_DEFAULT;
const int LRU_RECORD_SIZE = MESH_CORE_BROADCAST_LRU_RECORD_SIZE;

}  // namespace mesh_core
