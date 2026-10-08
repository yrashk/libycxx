// libycxx hosted runtime: the run-time message storage of the <stdexcept> classes, their key
// functions for -fno-rtti programs, and the out-of-line throw hook.
#include <stdexcept>
#include <new>
#include <string>
#include <ycxx/core/error.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

namespace {
// Block layout: [refcount (size_t)] [characters ... '\0']. The text pointer points at the
// characters.
struct __header {
  __SIZE_TYPE__ __refs;
};
__header* header_of(const char* __text) noexcept {
  return reinterpret_cast<__header*>(const_cast<char*>(__text)) - 1;
}
} // namespace

const char* __message_create(const char* s, std::size_t n) {
  void* __mem = ::operator new(sizeof(__header) + n + 1);
  auto* h = ::new (__mem) __header{1};
  char* __text = reinterpret_cast<char*>(h + 1);
  __builtin_memcpy(__text, s, n);
  __text[n] = '\0';
  return __text;
}
void __message_retain(const char* __text) noexcept { __atomic_fetch_add(&header_of(__text)->__refs, 1, __ATOMIC_RELAXED); }
void __message_release(const char* __text) noexcept {
  __header* h = header_of(__text);
  if (__atomic_fetch_sub(&h->__refs, 1, __ATOMIC_ACQ_REL) == 1)
    ::operator delete(h);
}

[[noreturn]] void __throw_std(ycxx_error_kind kind, const char* what) {
  switch (kind) {
  case ycxx_error_logic_error: throw std::logic_error(what);
  case ycxx_error_domain_error: throw std::domain_error(what);
  case ycxx_error_invalid_argument: throw std::invalid_argument(what);
  case ycxx_error_length_error: throw std::length_error(what);
  case ycxx_error_out_of_range: throw std::out_of_range(what);
  case ycxx_error_runtime_error: throw std::runtime_error(what);
  case ycxx_error_range_error: throw std::range_error(what);
  case ycxx_error_overflow_error: throw std::overflow_error(what);
  case ycxx_error_underflow_error: throw std::underflow_error(what);
  // Exception classes defined in core headers are thrown there (raise_with), never via raise(),
  // so no other kind reaches this point; report it rather than throw something unrelated.
  default: ::ycxx_error_handler(kind, what);
  }
}

}} // namespace __ycxx::__detail

// This file is built with _YCXX_EXCEPTION_KEY_FUNCTIONS (CMakeLists.txt), so the classes declare
// their destructors out of line here, and this translation unit, built with RTTI, emits their
// vtables for programs whose other translation units are built without RTTI (stdexcept.hpp).
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
logic_error::~logic_error() {}
runtime_error::~runtime_error() {}
domain_error::~domain_error() {}
invalid_argument::~invalid_argument() {}
length_error::~length_error() {}
out_of_range::~out_of_range() {}
range_error::~range_error() {}
overflow_error::~overflow_error() {}
underflow_error::~underflow_error() {}
}} // namespace std
