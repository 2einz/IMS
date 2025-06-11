#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>
#include <thread>

class SpinLock {
public:
  SpinLock() = default;
  ~SpinLock() = default;

  // 禁止拷贝与赋值
  SpinLock(const SpinLock &) = delete;
  SpinLock &operator=(const SpinLock &) = delete;

  void lock() {
    int wait_time = 0;
    // memory_order_acquire:后面访存指令勿重排至此条指令之前
    while (!locked_.exchange(true, std::memory_order_acquire)) {
      // 开始自旋
      if (wait_time < max_spin_count_) {
        ++wait_time;
      } else {
        // 让出cpu
        std::this_thread::yield();
        wait_time = 0;
      }
    }
  }

  void unlock() {
    // memory_order_release:前面访存指令勿重排到此条指令之后
    locked_.store(false, std::memory_order_release);
  }

  bool try_lock() { return !locked_.exchange(true, std::memory_order_acquire); }

private:
  std::atomic<bool> locked_ = ATOMIC_VAR_INIT(false);
  const int max_spin_count_ = 100;
};

#endif // SPINLOCK_H