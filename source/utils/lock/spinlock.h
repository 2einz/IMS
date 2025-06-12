#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>
#include <thread>

namespace reinz {
class SpinLock {
 public:
  SpinLock() = default;
  ~SpinLock() = default;

  // 禁止拷贝与赋值
  SpinLock(const SpinLock &) = delete;
  SpinLock &operator=(const SpinLock &) = delete;

  void lock() noexcept {
    // memory_order_acquire:后面访存指令勿重排至此条指令之前
    while (!flag_.test_and_set(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  }

  void unlock() noexcept {
    // memory_order_release:前面访存指令勿重排到此条指令之后
    flag_.clear(std::memory_order_release);
  }

  bool try_lock() noexcept {
    return !flag_.test_and_set(std::memory_order_acquire);
  }

 private:
  std::atomic_flag flag_;
};

class SpinlockGuard {
 public:
  explicit SpinlockGuard(SpinLock &lock) noexcept : lock_(lock) { lock.lock(); }

  ~SpinlockGuard() noexcept { lock_.unlock(); }

  SpinlockGuard(const SpinlockGuard &) = delete;
  SpinlockGuard &operator=(const SpinlockGuard &) = delete;

 private:
  SpinLock &lock_;
};
}  // namespace reinz

#endif  // SPINLOCK_H