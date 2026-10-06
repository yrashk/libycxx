// libycxx hosted runtime: the out-of-line parts of the thread support library ([thread]).
#include <cfenv>
#include <system_error>
#include <typeinfo>
#include <ycxx/core/atomic_base.hpp>
#include <ycxx/hosted/thread_support.hpp>
#include <ycxx/hosted/condition_variable.hpp>
#include <ycxx/pal.h>
#include "../runtime/atomic/wait_table.hpp"

namespace {

// What the trampoline of a new thread needs; the name is copied (null-terminated) behind it.
// [cfenv.syn]: a thread's floating-point environment starts as that of the thread constructing
// its std::thread or std::jthread object, at that time; some platforms (Linux) start threads
// so, others (macOS) with the default environment.
struct start_record {
  void (*run)(void*);
  void* arg;
  std::fenv_t fenv;
  bool named;
  char name[1];
};

void* trampoline(void* p) {
  start_record* r = static_cast<start_record*>(p);
  std::fesetenv(&r->fenv);
  if (r->named)
    ::ycxx_pal_thread_set_name(r->name);
  void (*run)(void*) = r->run;
  void* arg = r->arg;
  ::operator delete(r);
  run(arg);
  return nullptr;
}

} // namespace

// The C++ ABI runtime: the type of the exception being handled, null for a foreign one.
namespace [[__gnu__::__visibility__("hidden")]] __cxxabiv1 {
extern "C" [[__gnu__::__visibility__("hidden")]] std::type_info* __cxa_current_exception_type() noexcept;
}

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

bool __handling_foreign_exception() noexcept { return __cxxabiv1::__cxa_current_exception_type() == nullptr; }

[[noreturn]] void __throw_system_error(int __ev, const char* what) {
  throw std::system_error(std::error_code(__ev, std::generic_category()), what);
}

ycxx_pal_handle __thread_start(void (*run)(void*), void* arg, std::size_t __stack_size, const char* name,
                             std::size_t __name_size) {
  start_record* r = static_cast<start_record*>(::operator new(sizeof(start_record) + __name_size));
  r->run = run;
  r->arg = arg;
  std::fegetenv(&r->fenv);
  r->named = name != nullptr && __name_size != 0;
  if (r->named)
    __builtin_memcpy(r->name, name, __name_size);
  r->name[r->named ? __name_size : 0] = '\0';
  ycxx_pal_handle h = 0;
  if (int e = ::ycxx_pal_thread_create(&h, &trampoline, r, __stack_size); e != 0) {
    ::operator delete(r);
    ::__ycxx::__detail::__raise_system_error(static_cast<std::errc>(e), "thread: cannot start a thread");
  }
  return h;
}

void __at_thread_exit(void (*__f)(void*), void* arg) {
  if (int e = ::ycxx_pal_at_thread_end(__f, arg); e != 0)
    ::__ycxx::__detail::__raise_system_error(static_cast<std::errc>(e), "cannot register a thread-exit action");
}

// The timed form of atomic_wait_block (atomic_base.hpp): false when it returned because the
// deadline passed.
bool __atomic_wait_block_until(const volatile void* __addr, std::uint32_t __ticket, int clock, long long __sec,
                             long long __nsec) noexcept {
  std::uint32_t* __waiters = nullptr;
  std::uint32_t* version = ::__ycxx::__detail::__atomic_wait_entry(__addr, __waiters);
  const int r = ::ycxx_pal_wait_until(version, __ticket, clock, __sec, __nsec);
  __atomic_fetch_sub(__waiters, 1, __ATOMIC_RELAXED);
  return r == 0;
}

}} // namespace __ycxx::__detail
