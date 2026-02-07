#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <cassert>
#include <type_traits>

namespace Riverbed {

template <typename T>
class SpscQueue {
  static_assert(std::is_trivially_copyable_v<T> &&
                std::is_trivially_destructible_v<T>,
                "SpscQueue requires trivially copyable/destructible T");
  public:
    explicit SpscQueue (size_t capacity);

    ~SpscQueue() { std::allocator<T>{}.deallocate(buffer, capacity); }

    bool push(T value);
    bool pop(T& out);

  private:
    alignas(64) T* buffer;
    size_t capacity;
    size_t mask;
    alignas(64) std::atomic<size_t> write, read;
};

class River {
  
};

template <typename T>
SpscQueue<T>::SpscQueue(size_t capacity) 
  : buffer(static_cast<T*>(std::allocator<T>{}.allocate(capacity))),
    capacity(capacity),
    mask(capacity - 1),
    write(0),
    read(0)
  {
    assert((capacity & mask) == 0);
  }

template <typename T>
bool SpscQueue<T>::push(T value) {
  size_t write_idx = write.load(std::memory_order_relaxed);
  size_t read_idx = read.load(std::memory_order_acquire);

  size_t next = (write_idx + 1) & mask;

  if (next == read_idx) {
    return false;
  }
  
  buffer[write_idx] = value;
  write.store(next, std::memory_order_release);
  return true;
}

template <typename T>
bool SpscQueue<T>::pop(T& out) {
  size_t write_idx = write.load(std::memory_order_acquire);
  size_t read_idx = read.load(std::memory_order_relaxed);

  if (write_idx == read_idx) {
    return false;
  }

  out = buffer[read_idx];
  size_t next = (read_idx + 1) & mask;

  read.store(next, std::memory_order_release);
  return true;
}


} // namespace Riverbed
