#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

namespace Riverbed {

template <typename T>
class RingBuffer {
  public:
    explicit RingBuffer (size_t capacity);

    ~RingBuffer() { std::allocator<T>{}.deallocate(buffer, capacity); }

    bool push(:T value);
    bool pop(T& out);

  private:
    alignas(64) T* buffer;
    size_t capacity;
    alignas(64) std::atomic<size_t> write, read;
};

class River {
  
};

template <typename T>
RingBuffer<T>::RingBuffer(size_t capacity) 
  : buffer(buffer(static_cast<T*>(std::allocator<T>{}.allocate(capacity)))),
    capacity(capacity),
    write(0),
    read(0)
  {}

template <typename T>
bool RingBuffer<T>::push(const T& value) {
  size_t write_idx = write.load();
  size_t read_idx = read.load();

  size_t next = (write_idx + 1) % capacity;

  if (next == read_idx) {
    return false;
  }
  
  buffer[write_idx] = value;
  write.store(next);
  return true;
}

template <typename T>
bool RingBuffer<T>::pop(T& out) {
  size_t write_idx = write.load();
  size_t read_idx = read.load();

  if (write_idx == read_idx) {
    return false;
  }

  out = buffer[read_idx];
  size_t next = (read_idx + 1) % capacity;
  read.store(next);
  return true;
}


} // namespace Riverbed
