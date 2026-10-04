// libycxx hosted runtime: <stdexcept> members and the out-of-line throw hook.
#include <stdexcept>
#include <new>
#include <ycxx/core/error.hpp>

namespace ycxx::detail {

namespace {
// Block layout: [refcount (size_t)] [characters ... '\0']. text_ points at the characters.
struct header {
  __SIZE_TYPE__ refs;
};
header* header_of(const char* text) noexcept {
  return reinterpret_cast<header*>(const_cast<char*>(text)) - 1;
}
const char* make(const char* s, std::size_t n) {
  void* mem = ::operator new(sizeof(header) + n + 1);
  auto* h = ::new (mem) header{1};
  char* text = reinterpret_cast<char*>(h + 1);
  __builtin_memcpy(text, s, n);
  text[n] = '\0';
  return text;
}
} // namespace

shared_message::shared_message(const char* s) : text_(make(s, __builtin_strlen(s))) {}
shared_message::shared_message(const char* s, std::size_t n) : text_(make(s, n)) {}
shared_message::shared_message(const shared_message& o) noexcept : text_(o.text_) {
  __atomic_fetch_add(&header_of(text_)->refs, 1, __ATOMIC_RELAXED);
}
shared_message& shared_message::operator=(const shared_message& o) noexcept {
  if (text_ != o.text_) {
    shared_message tmp(o);
    const char* t = tmp.text_;
    tmp.text_ = text_;
    text_ = t;
  }
  return *this;
}
shared_message::~shared_message() {
  header* h = header_of(text_);
  if (__atomic_fetch_sub(&h->refs, 1, __ATOMIC_ACQ_REL) == 1)
    ::operator delete(h);
}

[[noreturn]] void throw_std(ycxx_error_kind kind, const char* what) {
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
  default: ::ycxx_error_handler(kind, what);
  }
}

} // namespace ycxx::detail

namespace std {
logic_error::logic_error(const char* s) : msg_(s) {}
logic_error::~logic_error() noexcept = default;
const char* logic_error::what() const noexcept { return msg_.c_str(); }
runtime_error::runtime_error(const char* s) : msg_(s) {}
runtime_error::~runtime_error() noexcept = default;
const char* runtime_error::what() const noexcept { return msg_.c_str(); }
domain_error::~domain_error() noexcept = default;
invalid_argument::~invalid_argument() noexcept = default;
length_error::~length_error() noexcept = default;
out_of_range::~out_of_range() noexcept = default;
range_error::~range_error() noexcept = default;
overflow_error::~overflow_error() noexcept = default;
underflow_error::~underflow_error() noexcept = default;
} // namespace std
