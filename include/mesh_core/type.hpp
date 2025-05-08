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
using timestamp_t = uint32_t;
using msg_uuid_t = uint32_t;
using lqs_t = int8_t;  // link quality score

/// assert
static_assert(std::is_trivial<addr_t>::value, "");
static_assert(std::is_trivial<seq_t>::value, "");
static_assert(std::is_trivial<ttl_t>::value, "");
static_assert(std::is_trivial<msg_uuid_t>::value, "");
static_assert(sizeof(msg_uuid_t) >= sizeof(addr_t) + sizeof(seq_t), "msg_uuid: [src, seq, ts]");

/// handle
using recv_handle_t = std::function<void(std::string, mesh_core::lqs_t)>;
using on_recv_handle_t = std::function<void(addr_t, data_t)>;
using time_sync_handle_t = std::function<void(timestamp_t)>;

/// default value
const ttl_t TTL_DEFAULT = MESH_CORE_TTL_DEFAULT;
const int LRU_RECORD_SIZE = MESH_CORE_LRU_RECORD_SIZE;
const int DELAY_MIN = MESH_CORE_DELAY_MS_MIN;
const int DELAY_MAX = MESH_CORE_DELAY_MS_MAX;

}  // namespace mesh_core
