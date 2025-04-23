#pragma once

#include <algorithm>
#include <cstddef>
#include <list>

namespace mesh_core {

template <typename T>
class lru_record {
 public:
  explicit lru_record(size_t max_size) : max_size_(max_size) {}

  void put(T key) {
    auto it = std::find(cache_.begin(), cache_.end(), key);
    if (it != cache_.end()) {
      cache_.erase(it);
    }
    cache_.push_front(key);

    if (cache_.size() > max_size_) {
      cache_.pop_back();
    }
  }

  bool exists(const T& key) {
    auto it = std::find(cache_.begin(), cache_.end(), key);
    if (it != cache_.end()) {
      cache_.erase(it);
      cache_.push_front(key);
      return true;
    }
    return false;
  }

  size_t size() const {
    return cache_.size();
  }

 private:
  std::list<T> cache_;
  size_t max_size_;
};

}  // namespace mesh_core
