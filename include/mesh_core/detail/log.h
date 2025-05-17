// Version: 1.0.1
//
// 1. global control
// L_O_G_NDEBUG                 disable debug log(auto by NDEBUG)
// L_O_G_SHOW_DEBUG             force enable debug log
// L_O_G_DISABLE_ALL            force disable all log
// L_O_G_DISABLE_HEX            disable hex function
// L_O_G_DISABLE_COLOR          disable color
// L_O_G_DISABLE_VERSION_CHECK  disable version check
// L_O_G_LINE_END_CRLF
// L_O_G_SHOW_FULL_PATH
// L_O_G_FOR_MCU
// L_O_G_FREERTOS
// L_O_G_NOT_EXIT_ON_FATAL
//
// C++11 enable default:
// L_O_G_ENABLE_THREAD_SAFE     thread safety
// L_O_G_ENABLE_THREAD_ID       show thread id
// L_O_G_ENABLE_DATE_TIME       show data time
// can disable by define:
// L_O_G_DISABLE_THREAD_SAFE
// L_O_G_DISABLE_THREAD_ID
// L_O_G_DISABLE_DATE_TIME
//
// 2. custom implements
// L_O_G_PRINTF_CUSTOM          int L_O_G_PRINTF_CUSTOM(const char *fmt, ...)
// L_O_G_GET_TID_CUSTOM         uint32_t L_O_G_GET_TID_CUSTOM()
// L_O_G_GET_TIME_CUSTOM        std::string L_O_G_GET_TIME_CUSTOM()
//
// 3. use in library
// 3.1. rename `MESH_CORE_LOG` to library name
// 3.2. will define `MESH_CORE_LOG_HIDE_DEBUG`
// 3.3. configuration options
// MESH_CORE_LOG_SHOW_DEBUG
// MESH_CORE_LOG_SHOW_VERBOSE
// MESH_CORE_LOG_DISABLE_ALL

#pragma once

// clang-format off

// version
#define MESH_CORE_LOG_VER_MAJOR 1
#define MESH_CORE_LOG_VER_MINOR 0
#define MESH_CORE_LOG_VER_PATCH 1
#define MESH_CORE_LOG_TO_VERSION(major, minor, patch) (major * 10000 + minor * 100 + patch)
#define MESH_CORE_LOG_VERSION MESH_CORE_LOG_TO_VERSION(MESH_CORE_LOG_VER_MAJOR, MESH_CORE_LOG_VER_MINOR, MESH_CORE_LOG_VER_PATCH)

// version check
#ifndef L_O_G_VER_MAJOR
#define L_O_G_VER_MAJOR MESH_CORE_LOG_VER_MAJOR
#else
#if !defined(L_O_G_DISABLE_VERSION_CHECK) && (L_O_G_VER_MAJOR != MESH_CORE_LOG_VER_MAJOR)
#error "version incompatible"
#endif
#endif

// auto define MESH_CORE_LOG_HIDE_DEBUG in library
#define MESH_CORE_LOG_CHECK_RENAME_HELPER 1
#define MESH_CORE_LOG_CHECK_RENAME L##OG_CHECK_RENAME_HELPER
#if MESH_CORE_LOG_CHECK_RENAME != MESH_CORE_LOG_CHECK_RENAME_HELPER
#define MESH_CORE_LOG_HIDE_DEBUG
#endif

// inline for global function
#ifdef __cplusplus
#define L_O_G_FUNCTION extern "C" inline
#else
#define L_O_G_FUNCTION static inline
#endif

// suppress compile warnings, ensure params will be used
#ifndef L_O_G_VOID
#define L_O_G_VOID L_O_G_VOID
L_O_G_FUNCTION void L_O_G_VOID(const char *fmt, ...) {
  (void)(fmt);
}
#endif

#if defined(MESH_CORE_LOG_DISABLE_ALL) || defined(L_O_G_DISABLE_ALL)

#ifndef L_O_G_PRINTF
#define L_O_G_PRINTF(fmt, ...)  L_O_G_VOID(fmt, ##__VA_ARGS__)
#endif

#define MESH_CORE_LOG(fmt, ...)           L_O_G_VOID(fmt, ##__VA_ARGS__)
#define MESH_CORE_LOGT(tag, fmt, ...)     L_O_G_VOID(fmt, ##__VA_ARGS__)
#define MESH_CORE_LOGI(fmt, ...)          L_O_G_VOID(fmt, ##__VA_ARGS__)
#define MESH_CORE_LOGW(fmt, ...)          L_O_G_VOID(fmt, ##__VA_ARGS__)
#define MESH_CORE_LOGE(fmt, ...)          L_O_G_VOID(fmt, ##__VA_ARGS__)
#define MESH_CORE_LOGF(fmt, ...)          L_O_G_VOID(fmt, ##__VA_ARGS__)
#define MESH_CORE_LOGD(fmt, ...)          L_O_G_VOID(fmt, ##__VA_ARGS__)
#define MESH_CORE_LOGV(fmt, ...)          L_O_G_VOID(fmt, ##__VA_ARGS__)

#define MESH_CORE_LOGR(fmt, ...)          L_O_G_VOID(fmt, ##__VA_ARGS__)
#define MESH_CORE_LOGLN()                 ((void)0)
#define MESH_CORE_LOGRLN(fmt, ...)        L_O_G_VOID(fmt, ##__VA_ARGS__)

#define MESH_CORE_LOGD_HEX(...)           ((void)0)
#define MESH_CORE_LOGD_HEX_H(...)         ((void)0)
#define MESH_CORE_LOGD_HEX_C(...)         ((void)0)
#define MESH_CORE_LOGD_HEX_D(...)         ((void)0)

#else

#ifdef __cplusplus
#include <cstring>
#include <cstdlib>
#include <cinttypes>

#if __cplusplus >= 201103L || defined(_MSC_VER)

#if !defined(L_O_G_DISABLE_THREAD_SAFE) && !defined(L_O_G_ENABLE_THREAD_SAFE)
#define L_O_G_ENABLE_THREAD_SAFE
#endif

#if !defined(L_O_G_DISABLE_THREAD_ID) && !defined(L_O_G_ENABLE_THREAD_ID)
#define L_O_G_ENABLE_THREAD_ID
#endif

#if !defined(L_O_G_DISABLE_DATE_TIME) && !defined(L_O_G_ENABLE_DATE_TIME)
#define L_O_G_ENABLE_DATE_TIME
#endif

#endif
#else
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#endif

#ifdef  L_O_G_LINE_END_CRLF
#define MESH_CORE_LOG_LINE_END            "\r\n"
#else
#define MESH_CORE_LOG_LINE_END            "\n"
#endif

#ifdef L_O_G_NOT_EXIT_ON_FATAL
#define MESH_CORE_LOG_EXIT_PROGRAM()
#else
#ifdef L_O_G_FOR_MCU
#define MESH_CORE_LOG_EXIT_PROGRAM()      do{ for(;;); } while(0)
#else
#define MESH_CORE_LOG_EXIT_PROGRAM()      exit(EXIT_FAILURE)
#endif
#endif

#ifdef L_O_G_SHOW_FULL_PATH
#define MESH_CORE_LOG_BASE_FILENAME       (__FILE__)
#else
#ifdef __FILE_NAME__
#define MESH_CORE_LOG_BASE_FILENAME       (__FILE_NAME__)
#else
#define MESH_CORE_LOG_BASE_FILENAME       (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif
#endif

#define MESH_CORE_LOG_WITH_COLOR

#if defined(_WIN32) || (defined(__ANDROID__) && !defined(ANDROID_STANDALONE)) || defined(L_O_G_FOR_MCU) || defined(ESP_PLATFORM)
#undef MESH_CORE_LOG_WITH_COLOR
#endif

#ifdef L_O_G_DISABLE_COLOR
#undef MESH_CORE_LOG_WITH_COLOR
#endif

#ifdef MESH_CORE_LOG_WITH_COLOR
#define MESH_CORE_LOG_COLOR_RED           "\033[31m"
#define MESH_CORE_LOG_COLOR_GREEN         "\033[32m"
#define MESH_CORE_LOG_COLOR_YELLOW        "\033[33m"
#define MESH_CORE_LOG_COLOR_BLUE          "\033[34m"
#define MESH_CORE_LOG_COLOR_CARMINE       "\033[35m"
#define MESH_CORE_LOG_COLOR_CYAN          "\033[36m"
#define MESH_CORE_LOG_COLOR_DEFAULT
#define MESH_CORE_LOG_COLOR_END           "\033[m"
#else
#define MESH_CORE_LOG_COLOR_RED
#define MESH_CORE_LOG_COLOR_GREEN
#define MESH_CORE_LOG_COLOR_YELLOW
#define MESH_CORE_LOG_COLOR_BLUE
#define MESH_CORE_LOG_COLOR_CARMINE
#define MESH_CORE_LOG_COLOR_CYAN
#define MESH_CORE_LOG_COLOR_DEFAULT
#define MESH_CORE_LOG_COLOR_END
#endif

#define MESH_CORE_LOG_END                 MESH_CORE_LOG_COLOR_END MESH_CORE_LOG_LINE_END

#ifndef L_O_G_PRINTF
#ifndef MESH_CORE_LOG_PRINTF_DEFAULT
#if defined(__ANDROID__) && !defined(ANDROID_STANDALONE)
#include <android/log.h>
#define MESH_CORE_LOG_PRINTF_DEFAULT(fmt, ...) __android_log_print(ANDROID_L##OG_DEBUG, "MESH_CORE_LOG", fmt, ##__VA_ARGS__)
#else
#define MESH_CORE_LOG_PRINTF_DEFAULT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif
#endif

#ifndef L_O_G_PRINTF_CUSTOM
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif
#ifdef L_O_G_ENABLE_THREAD_SAFE
#ifndef L_O_G_NS_MUTEX
#define L_O_G_NS_MUTEX L_O_G_NS_MUTEX
#include <mutex>
// 1. struct instead of namespace, ensure single instance
struct L_O_G_NS_MUTEX {
static std::mutex& mutex() {
  // 2. never delete, avoid destroy before user log
  // 3. static memory, avoid memory fragmentation
  static char memory[sizeof(std::mutex)];
  static std::mutex& mutex = *(new (memory) std::mutex());
  return mutex;
}
};
#endif
#define L_O_G_PRINTF(fmt, ...) { \
  std::lock_guard<std::mutex> L_O_G_NS_lock(L_O_G_NS_MUTEX::mutex()); \
  MESH_CORE_LOG_PRINTF_DEFAULT(fmt, ##__VA_ARGS__); \
}
#else
#define L_O_G_PRINTF(fmt, ...)  MESH_CORE_LOG_PRINTF_DEFAULT(fmt, ##__VA_ARGS__)
#endif
#else
extern int L_O_G_PRINTF_CUSTOM(const char *fmt, ...);
#define L_O_G_PRINTF(fmt, ...)  L_O_G_PRINTF_CUSTOM(fmt, ##__VA_ARGS__)
#endif
#endif

#ifdef L_O_G_ENABLE_THREAD_ID
#ifndef L_O_G_NS_GET_TID
#define L_O_G_NS_GET_TID L_O_G_NS_GET_TID
#include <cstdint>
#ifdef L_O_G_GET_TID_CUSTOM
extern uint32_t L_O_G_GET_TID_CUSTOM();
#elif defined(_WIN32)
#include <Windows.h>
#include <processthreadsapi.h>
struct L_O_G_NS_GET_TID {
static inline uint32_t get_tid() {
  return GetCurrentThreadId();
}
};
#elif defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>
struct L_O_G_NS_GET_TID {
static inline uint32_t get_tid() {
  return syscall(SYS_gettid);
}
};
#elif defined(L_O_G_FREERTOS) || defined(FREERTOS_CONFIG_H)
#include <freertos/FreeRTOS.h>
struct L_O_G_NS_GET_TID {
static inline uint32_t get_tid() {
  return (uint32_t)xTaskGetCurrentTaskHandle();
}
};
#else /* for mac, bsd.. */
#include <pthread.h>
struct L_O_G_NS_GET_TID {
static inline uint32_t get_tid() {
  uint64_t x;
  pthread_threadid_np(nullptr, &x);
  return (uint32_t)x;
}
};
#endif
#endif
#ifdef L_O_G_GET_TID_CUSTOM
#define MESH_CORE_LOG_THREAD_LABEL "%" PRIu32 " "
#define MESH_CORE_LOG_THREAD_VALUE ,L_O_G_GET_TID_CUSTOM()
#else
#define MESH_CORE_LOG_THREAD_LABEL "%" PRIu32 " "
#define MESH_CORE_LOG_THREAD_VALUE ,L_O_G_NS_GET_TID::get_tid()
#endif
#else
#define MESH_CORE_LOG_THREAD_LABEL
#define MESH_CORE_LOG_THREAD_VALUE
#endif

#ifdef L_O_G_ENABLE_DATE_TIME
#include <string>
#ifdef L_O_G_GET_TIME_CUSTOM
extern std::string L_O_G_GET_TIME_CUSTOM();
#else
#include <chrono>
#include <sstream>
#include <iomanip> // std::put_time
#ifndef L_O_G_NS_GET_TIME
#define L_O_G_NS_GET_TIME L_O_G_NS_GET_TIME
struct L_O_G_NS_GET_TIME {
static inline std::string get_time() {
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  std::stringstream ss;
  std::tm dst; // NOLINT
#ifdef _MSC_VER
  ::localtime_s(&dst, &time);
#else
  dst = *std::localtime(&time);
#endif
  ss << std::put_time(&dst, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
  return ss.str();
}
};
#endif
#endif
#ifdef L_O_G_GET_TIME_CUSTOM
#define MESH_CORE_LOG_TIME_LABEL "%s "
#define MESH_CORE_LOG_TIME_VALUE ,L_O_G_GET_TIME_CUSTOM().c_str()
#else
#define MESH_CORE_LOG_TIME_LABEL "%s "
#define MESH_CORE_LOG_TIME_VALUE ,L_O_G_NS_GET_TIME::get_time().c_str()
#endif
#else
#define MESH_CORE_LOG_TIME_LABEL
#define MESH_CORE_LOG_TIME_VALUE
#endif

#define MESH_CORE_LOG(fmt, ...)           do{ L_O_G_PRINTF(MESH_CORE_LOG_COLOR_GREEN   MESH_CORE_LOG_TIME_LABEL MESH_CORE_LOG_THREAD_LABEL "[*]: %s:%d "       fmt MESH_CORE_LOG_END MESH_CORE_LOG_TIME_VALUE MESH_CORE_LOG_THREAD_VALUE, MESH_CORE_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define MESH_CORE_LOGT(tag, fmt, ...)     do{ L_O_G_PRINTF(MESH_CORE_LOG_COLOR_BLUE    MESH_CORE_LOG_TIME_LABEL MESH_CORE_LOG_THREAD_LABEL "[" tag "]: %s:%d " fmt MESH_CORE_LOG_END MESH_CORE_LOG_TIME_VALUE MESH_CORE_LOG_THREAD_VALUE, MESH_CORE_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define MESH_CORE_LOGI(fmt, ...)          do{ L_O_G_PRINTF(MESH_CORE_LOG_COLOR_YELLOW  MESH_CORE_LOG_TIME_LABEL MESH_CORE_LOG_THREAD_LABEL "[I]: %s:%d "       fmt MESH_CORE_LOG_END MESH_CORE_LOG_TIME_VALUE MESH_CORE_LOG_THREAD_VALUE, MESH_CORE_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define MESH_CORE_LOGW(fmt, ...)          do{ L_O_G_PRINTF(MESH_CORE_LOG_COLOR_CARMINE MESH_CORE_LOG_TIME_LABEL MESH_CORE_LOG_THREAD_LABEL "[W]: %s:%d [%s] "  fmt MESH_CORE_LOG_END MESH_CORE_LOG_TIME_VALUE MESH_CORE_LOG_THREAD_VALUE, MESH_CORE_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define MESH_CORE_LOGE(fmt, ...)          do{ L_O_G_PRINTF(MESH_CORE_LOG_COLOR_RED     MESH_CORE_LOG_TIME_LABEL MESH_CORE_LOG_THREAD_LABEL "[E]: %s:%d [%s] "  fmt MESH_CORE_LOG_END MESH_CORE_LOG_TIME_VALUE MESH_CORE_LOG_THREAD_VALUE, MESH_CORE_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define MESH_CORE_LOGF(fmt, ...)          do{ L_O_G_PRINTF(MESH_CORE_LOG_COLOR_CYAN    MESH_CORE_LOG_TIME_LABEL MESH_CORE_LOG_THREAD_LABEL "[!]: %s:%d [%s] "  fmt MESH_CORE_LOG_END MESH_CORE_LOG_TIME_VALUE MESH_CORE_LOG_THREAD_VALUE, MESH_CORE_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); MESH_CORE_LOG_EXIT_PROGRAM(); } while(0) // NOLINT(bugprone-lambda-function-name)

// for raw print
#define MESH_CORE_LOGR(fmt, ...)          do{ L_O_G_PRINTF(fmt, ##__VA_ARGS__);         } while(0)
#define MESH_CORE_LOGLN()                 MESH_CORE_LOGR(MESH_CORE_LOG_LINE_END)
#define MESH_CORE_LOGRLN(fmt, ...)        do{ L_O_G_PRINTF(fmt MESH_CORE_LOG_END, ##__VA_ARGS__); } while(0)

// for hex print, enable by default
#if !defined(L_O_G_DISABLE_HEX) && !defined(L_O_G_ENABLE_HEX)
#define L_O_G_ENABLE_HEX
#endif

#ifdef L_O_G_ENABLE_HEX
#define MESH_CORE_LOG_HEX                 L_O_G_HEX
#define MESH_CORE_LOG_HEX_H               L_O_G_HEX_H
#define MESH_CORE_LOG_HEX_C               L_O_G_HEX_C
#define MESH_CORE_LOG_HEX_D               L_O_G_HEX_D
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#ifdef ARDUINO
#include <Arduino.h>
#endif

#ifndef L_O_G_HEX
#define L_O_G_HEX L_O_G_HEX
L_O_G_FUNCTION void L_O_G_HEX(const void *data, size_t size) {
  const unsigned char *byte = (const unsigned char *)data;
  uint32_t offset = 0;
  while (offset < size) {
    L_O_G_PRINTF("%08" PRIX32 "  ", offset);
    char hex_buffer[16 * 3 + 1] = {0};
    char ascii_buffer[16 + 1] = {0};
    for (int i = 0; i < 16; i++) {
      if (offset + i < size) {
        unsigned char b = byte[offset + i];
        snprintf(hex_buffer + i * 3, 4, "%02" PRIX8 " ", b);
        ascii_buffer[i] = (char)(isprint(b) ? b : '.');
      } else {
        snprintf(hex_buffer + i * 3, 4, "   ");
        ascii_buffer[i] = ' ';
      }
    }
    L_O_G_PRINTF("%-48s %s" MESH_CORE_LOG_LINE_END, hex_buffer, ascii_buffer);
    offset += 16;
  }
}
#endif

#ifndef L_O_G_HEX_H
#define L_O_G_HEX_H L_O_G_HEX_H
L_O_G_FUNCTION void L_O_G_HEX_H(const void *data, size_t size) {
  const unsigned char *bytes = (const unsigned char *)data;
  for (size_t i = 0; i < size; ++i) {
    L_O_G_PRINTF("%02" PRIX8 " ", bytes[i]);
  }
  L_O_G_PRINTF(MESH_CORE_LOG_LINE_END);
}
#endif

#ifndef L_O_G_HEX_CHAR
#define L_O_G_HEX_CHAR L_O_G_HEX_CHAR
L_O_G_FUNCTION void L_O_G_HEX_CHAR(const char *fmt, const void *data, size_t size) {
  const unsigned char *bytes = (const unsigned char *)data;
  for (size_t i = 0; i < size; ++i) {
    unsigned char c = bytes[i];
    L_O_G_PRINTF(fmt, isprint(c) ? c : '.');
  }
  L_O_G_PRINTF(MESH_CORE_LOG_LINE_END);
}
#endif

#define L_O_G_HEX_C(data, size) L_O_G_HEX_CHAR("%c", data, size);
#define L_O_G_HEX_D(data, size) L_O_G_HEX_CHAR(" %c ", data, size);
#endif

// in-lib should define no-debug by default, if not enable by user
#if defined(MESH_CORE_LOG_HIDE_DEBUG) && !defined(MESH_CORE_LOG_SHOW_DEBUG) && !defined(L_O_G_NDEBUG)
#ifndef MESH_CORE_LOG_NDEBUG
#define MESH_CORE_LOG_NDEBUG
#endif
#endif

#if defined(L_O_G_NDEBUG)
#ifndef MESH_CORE_LOG_NDEBUG
#define MESH_CORE_LOG_NDEBUG
#endif
#endif

#if (defined(NDEBUG) || defined(MESH_CORE_LOG_NDEBUG)) && !defined(L_O_G_SHOW_DEBUG)
#ifndef MESH_CORE_LOG_NDEBUG
#define MESH_CORE_LOG_NDEBUG
#endif
#else
#ifndef MESH_CORE_LOG_SHOW_DEBUG
#define MESH_CORE_LOG_SHOW_DEBUG
#endif
#endif

#if defined(MESH_CORE_LOG_SHOW_DEBUG)
#define MESH_CORE_LOGD(fmt, ...)          do{ L_O_G_PRINTF(MESH_CORE_LOG_COLOR_DEFAULT MESH_CORE_LOG_TIME_LABEL MESH_CORE_LOG_THREAD_LABEL "[D]: %s:%d "       fmt MESH_CORE_LOG_END MESH_CORE_LOG_TIME_VALUE MESH_CORE_LOG_THREAD_VALUE, MESH_CORE_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define MESH_CORE_LOGD_HEX                L_O_G_HEX
#define MESH_CORE_LOGD_HEX_H              L_O_G_HEX_H
#define MESH_CORE_LOGD_HEX_C              L_O_G_HEX_C
#define MESH_CORE_LOGD_HEX_D              L_O_G_HEX_D
#else
#define MESH_CORE_LOGD(fmt, ...)          ((void)0)
#define MESH_CORE_LOGD_HEX(...)           ((void)0)
#define MESH_CORE_LOGD_HEX_H(...)         ((void)0)
#define MESH_CORE_LOGD_HEX_C(...)         ((void)0)
#define MESH_CORE_LOGD_HEX_D(...)         ((void)0)
#endif

#if defined(MESH_CORE_LOG_SHOW_VERBOSE)
#define MESH_CORE_LOGV(fmt, ...)          do{ L_O_G_PRINTF(MESH_CORE_LOG_COLOR_DEFAULT MESH_CORE_LOG_TIME_LABEL MESH_CORE_LOG_THREAD_LABEL "[V]: %s:%d "       fmt MESH_CORE_LOG_END MESH_CORE_LOG_TIME_VALUE MESH_CORE_LOG_THREAD_VALUE, MESH_CORE_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define MESH_CORE_LOGV_HEX                L_O_G_HEX
#define MESH_CORE_LOGV_HEX_H              L_O_G_HEX_H
#define MESH_CORE_LOGV_HEX_C              L_O_G_HEX_C
#define MESH_CORE_LOGV_HEX_D              L_O_G_HEX_C
#else
#define MESH_CORE_LOGV(fmt, ...)          ((void)0)
#define MESH_CORE_LOGV_HEX(...)           ((void)0)
#define MESH_CORE_LOGV_HEX_H(...)         ((void)0)
#define MESH_CORE_LOGV_HEX_C(...)         ((void)0)
#define MESH_CORE_LOGV_HEX_D(...)         ((void)0)
#endif

/// logic check
#if defined(L_O_G_SHOW_DEBUG) && !defined(MESH_CORE_LOG_SHOW_DEBUG)
#error
#endif

#if defined(L_O_G_NDEBUG) && !defined(MESH_CORE_LOG_NDEBUG)
#error
#endif

#if defined(L_O_G_DISABLE_ALL) && !defined(MESH_CORE_LOG_DISABLE_ALL)
#error
#endif

#if !defined(MESH_CORE_LOG_NDEBUG) && !defined(MESH_CORE_LOG_SHOW_DEBUG)
#error
#endif

#endif
