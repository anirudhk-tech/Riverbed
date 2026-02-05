

#include <atomic>
namespace Riverbed {

template <typename T>
class RingBuffer {
  public:
    bool push(T&);
    bool pop(T&);

  private:
    T* buffer;
    size_t capacity;
    std::atomic<size_t> head, tail;
};

class River {
  
};

} // namespace Riverbed
