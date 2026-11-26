// A joined thread with a stack-size hint for hosted builds.
#pragma once

#include <ycxx/execution/detail/standard_headers.hpp>

namespace ycxx::execution {

class stack_thread {
public:
  stack_thread() noexcept = default;
  stack_thread(const stack_thread&) = delete;
  stack_thread& operator=(const stack_thread&) = delete;

  ~stack_thread();

  template <class _Fn>
  void start(std::size_t __stack, _Fn&& __f) {
    using _Task = std::decay_t<_Fn>;
    auto* __task = new _Task(static_cast<_Fn&&>(__f));
    try {
      __start(__stack, &__run<_Task>, __task);
    } catch (...) {
      delete __task;
      throw;
    }
  }

  bool joinable() const noexcept { return __active_; }
  void join();
  void detach();

private:
  void __start(std::size_t, void* (*)(void*), void*);

  template <class _Task>
  static void* __run(void* __data) noexcept {
    std::unique_ptr<_Task> __task(static_cast<_Task*>(__data));
    std::invoke(std::move(*__task));
    return nullptr;
  }

  pthread_t __thread_{};
  bool __active_ = false;
};

using worker_thread = stack_thread;

template <class _Fn>
void start_worker(worker_thread& __worker, std::size_t __stack, _Fn&& __f) {
  __worker.start(__stack, static_cast<_Fn&&>(__f));
}

} // namespace ycxx::execution
