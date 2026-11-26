#include <ycxx/execution/stack_thread.hpp>
#include <ycxx/execution/detail/thread_stack.h>
#include <unistd.h>

namespace ycxx::execution {

stack_thread::~stack_thread() { join(); }

void stack_thread::__start(std::size_t __stack, void* (*__entry)(void*), void* __data) {
  if (joinable())
    throw std::system_error(std::make_error_code(std::errc::device_or_resource_busy), "starting a worker thread");
  pthread_attr_t __attr;
  int __error = ::pthread_attr_init(&__attr);
  if (__error == 0) {
    if (__stack != 0)
      (void)::pthread_attr_setstacksize(&__attr,
          ::__ycxx_thread_stack_size(__stack, ::sysconf(_SC_THREAD_STACK_MIN), ::sysconf(_SC_PAGESIZE)));
    __error = ::pthread_create(&__thread_, &__attr, __entry, __data);
    ::pthread_attr_destroy(&__attr);
  }
  if (__error != 0)
    throw std::system_error(__error, std::generic_category(), "starting a worker thread");
  __active_ = true;
}

void stack_thread::join() {
  if (!joinable())
    return;
  if (const int __error = ::pthread_join(__thread_, nullptr); __error != 0)
    throw std::system_error(__error, std::generic_category(), "joining a worker thread");
  __active_ = false;
}

void stack_thread::detach() {
  if (!joinable())
    throw std::system_error(std::make_error_code(std::errc::invalid_argument), "detaching a worker thread");
  if (const int __error = ::pthread_detach(__thread_); __error != 0)
    throw std::system_error(__error, std::generic_category(), "detaching a worker thread");
  __active_ = false;
}

} // namespace ycxx::execution
