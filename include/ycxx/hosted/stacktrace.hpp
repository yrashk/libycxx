// libycxx hosted: <stacktrace> ([stacktrace]).
//
// A stacktrace_entry is the address of an instruction: for a caller's frame, an address inside
// its call instruction (the return address minus one), so that symbolization and the line
// table find the call itself. The runtime (src/hosted/stacktrace.cpp) captures the frames with
// the toolchain unwinder (_Unwind_Backtrace) and symbolizes an address on request: the
// function's name from the object's ELF symbol table (demangled), the file and line from its
// DWARF line table (.debug_line, versions 2-5), read from the object file through the PAL.
//
// current() is never inlined and passes its own return address to the runtime, which drops
// every frame up to the one of current()'s caller: the result does not depend on how the
// library's own frames were inlined or tail-called.
//
// formatter<stacktrace_entry> and formatter<basic_stacktrace<A>> ([stacktrace.format]) are in
// stacktrace_format.hpp.
#pragma once

#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/hosted/iosfwd.hpp>
#include <ycxx/core/memory_resource.hpp>
#include <ycxx/core/vector.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
class stacktrace_entry;
}}

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// src/hosted/stacktrace.cpp. Writes to buf the addresses of at most n frames of the calling
// thread's stack, starting with the frame whose return address is ra (the caller of
// basic_stacktrace::current) and skipping the first skip of those; returns how many it wrote.
// With a null buf it only counts them.
std::size_t __stacktrace_capture(const void* __ra, std::size_t __skip, std::uintptr_t* __buf, std::size_t n) noexcept;

// The queries of [stacktrace.entry.query] for the address pc; what selects the result.
enum class __stacktrace_query { description, source_file };
std::string __stacktrace_describe(std::uintptr_t __pc, __stacktrace_query what);
std::uint_least32_t __stacktrace_line(std::uintptr_t __pc);

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [stacktrace.entry]
class stacktrace_entry {
public:
  using native_handle_type = uintptr_t;

  constexpr stacktrace_entry() noexcept = default;
  constexpr stacktrace_entry(const stacktrace_entry& other) noexcept = default;
  constexpr stacktrace_entry& operator=(const stacktrace_entry& other) noexcept = default;
  ~stacktrace_entry() = default;

  constexpr native_handle_type native_handle() const noexcept { return __pc_; }
  constexpr explicit operator bool() const noexcept { return __pc_ != 0; }

  string description() const {
    return __pc_ != 0 ? __ycxx::__detail::__stacktrace_describe(__pc_, __ycxx::__detail::__stacktrace_query::description) : string();
  }
  string source_file() const {
    return __pc_ != 0 ? __ycxx::__detail::__stacktrace_describe(__pc_, __ycxx::__detail::__stacktrace_query::source_file) : string();
  }
  uint_least32_t source_line() const { return __pc_ != 0 ? __ycxx::__detail::__stacktrace_line(__pc_) : 0; }

  friend constexpr bool operator==(const stacktrace_entry& __x, const stacktrace_entry& y) noexcept {
    return __x.__pc_ == y.__pc_;
  }
  friend constexpr strong_ordering operator<=>(const stacktrace_entry& __x, const stacktrace_entry& y) noexcept {
    return __x.__pc_ <=> y.__pc_;
  }

private:
  template <class _Allocator>
  friend class basic_stacktrace;
  constexpr explicit stacktrace_entry(uintptr_t __pc) noexcept : __pc_(__pc) {}

  uintptr_t __pc_ = 0;
};

// [stacktrace.basic]
template <class _Allocator>
class basic_stacktrace {
  using __frames_type = vector<stacktrace_entry, _Allocator>;

public:
  using value_type = stacktrace_entry;
  using const_reference = const value_type&;
  using reference = value_type&;
  using const_iterator = typename __frames_type::const_iterator;
  using iterator = const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using difference_type = typename __frames_type::difference_type;
  using size_type = typename __frames_type::size_type;
  using allocator_type = _Allocator;

  // ---- [stacktrace.basic.cons] ----
  [[__gnu__::__noinline__]] static basic_stacktrace current(const allocator_type& __alloc = allocator_type()) noexcept {
    return __capture(__builtin_return_address(0), 0, static_cast<size_type>(-1), __alloc);
  }
  [[__gnu__::__noinline__]] static basic_stacktrace current(size_type __skip,
                                                    const allocator_type& __alloc = allocator_type()) noexcept {
    return __capture(__builtin_return_address(0), __skip, static_cast<size_type>(-1), __alloc);
  }
  [[__gnu__::__noinline__]] static basic_stacktrace current(size_type __skip, size_type __max_depth,
                                                    const allocator_type& __alloc = allocator_type()) noexcept {
    __ycxx::__detail::__precondition(__skip <= __skip + __max_depth, "std::basic_stacktrace::current: skip + max_depth overflows");
    return __capture(__builtin_return_address(0), __skip, __max_depth, __alloc);
  }

  basic_stacktrace() noexcept(is_nothrow_default_constructible_v<allocator_type>) = default;
  explicit basic_stacktrace(const allocator_type& __alloc) noexcept : __frames_(__alloc) {}
  basic_stacktrace(const basic_stacktrace& other) = default;
  basic_stacktrace(basic_stacktrace&& other) noexcept = default;
  basic_stacktrace(const basic_stacktrace& other, const allocator_type& __alloc) : __frames_(other.__frames_, __alloc) {}
  basic_stacktrace(basic_stacktrace&& other, const allocator_type& __alloc)
      : __frames_(static_cast<__frames_type&&>(other.__frames_), __alloc) {}
  basic_stacktrace& operator=(const basic_stacktrace& other) = default;
  basic_stacktrace& operator=(basic_stacktrace&& other) noexcept(
      allocator_traits<_Allocator>::propagate_on_container_move_assignment::value ||
      allocator_traits<_Allocator>::is_always_equal::value) = default;
  ~basic_stacktrace() = default;

  // ---- [stacktrace.basic.obs] ----
  allocator_type get_allocator() const noexcept { return __frames_.get_allocator(); }
  const_iterator begin() const noexcept { return __frames_.begin(); }
  const_iterator end() const noexcept { return __frames_.end(); }
  const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(cend()); }
  const_reverse_iterator rend() const noexcept { return const_reverse_iterator(cbegin()); }
  const_iterator cbegin() const noexcept { return __frames_.cbegin(); }
  const_iterator cend() const noexcept { return __frames_.cend(); }
  const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
  const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }
  [[nodiscard]] bool empty() const noexcept { return __frames_.empty(); }
  size_type size() const noexcept { return __frames_.size(); }
  size_type max_size() const noexcept { return __frames_.max_size(); }
  const_reference operator[](size_type __frame_no) const {
    __ycxx::__detail::__precondition(__frame_no < size(), "std::basic_stacktrace::operator[]: index out of range");
    return __frames_[__frame_no];
  }
  const_reference at(size_type __frame_no) const {
    if (__frame_no >= size())
      __ycxx::__detail::__throw_out_of_range("std::basic_stacktrace::at: index out of range");
    return __frames_[__frame_no];
  }

  // ---- [stacktrace.basic.cmp] ----
  template <class _Allocator2>
  friend bool operator==(const basic_stacktrace& __x, const basic_stacktrace<_Allocator2>& y) noexcept {
    if (__x.size() != y.size())
      return false;
    for (size_type i = 0; i < __x.size(); ++i)
      if (!(__x.__frames_[i] == y[i]))
        return false;
    return true;
  }
  template <class _Allocator2>
  friend strong_ordering operator<=>(const basic_stacktrace& __x, const basic_stacktrace<_Allocator2>& y) noexcept {
    if (__x.size() != y.size())
      return __x.size() <=> y.size();
    for (size_type i = 0; i < __x.size(); ++i)
      if (const strong_ordering c = __x.__frames_[i] <=> y[i]; c != 0)
        return c;
    return strong_ordering::equal;
  }

  // ---- [stacktrace.basic.mod] ----
  void swap(basic_stacktrace& other) noexcept(allocator_traits<_Allocator>::propagate_on_container_swap::value ||
                                             allocator_traits<_Allocator>::is_always_equal::value) {
    __frames_.swap(other.__frames_);
  }

private:
  __frames_type __frames_;

  // The frames of the stack from the caller of current() (whose return address is ra) on, after
  // skip, at most max_depth of them; empty if they cannot be stored.
  static basic_stacktrace __capture(const void* __ra, size_type __skip, size_type __max_depth,
                                  const allocator_type& __alloc) noexcept {
    basic_stacktrace __st(__alloc);
    const auto fill = [&] {
      // The exact number of frames first: one allocation of exactly that many entries.
      const size_t __total = __ycxx::__detail::__stacktrace_capture(__ra, __skip, nullptr, __max_depth);
      __st.__frames_.reserve(__total);
      constexpr size_t chunk = 64;
      uintptr_t __buf[chunk];
      size_t __pos = __skip;
      while (__st.__frames_.size() < __total) {
        const size_t left = __total - __st.__frames_.size();
        const size_t __want = left < chunk ? left : chunk;
        const size_t __got = __ycxx::__detail::__stacktrace_capture(__ra, __pos, __buf, __want);
        for (size_t i = 0; i < __got; ++i)
          __st.__frames_.push_back(stacktrace_entry(__buf[i]));
        if (__got < __want)
          break;
        __pos += __got;
      }
    };
    if constexpr (__ycxx::__detail::__cfg::exceptions) {
      try {
        fill();
      } catch (...) {
        return basic_stacktrace(__alloc);
      }
    } else {
      fill();
    }
    return __st;
  }
};

using stacktrace = basic_stacktrace<allocator<stacktrace_entry>>;
namespace pmr {
using stacktrace = basic_stacktrace<polymorphic_allocator<stacktrace_entry>>;
}

// ---- [stacktrace.basic.nonmem] ----
template <class _Allocator>
void swap(basic_stacktrace<_Allocator>& a, basic_stacktrace<_Allocator>& b) noexcept(noexcept(a.swap(b))) {
  a.swap(b);
}

// "DESCRIPTION at FILE:LINE", leaving out what is not known (src/hosted/stacktrace.cpp).
string to_string(const stacktrace_entry& __f);

// One line per entry: its number, right-aligned in four columns, "# " and to_string of the entry.
template <class _Allocator>
string to_string(const basic_stacktrace<_Allocator>& __st) {
  string s;
  for (typename basic_stacktrace<_Allocator>::size_type i = 0; i < __st.size(); ++i) {
    char num[24];
    char* const last = num + sizeof num;
    char* p = last;
    auto n = i;
    do {
      *--p = static_cast<char>('0' + n % 10);
      n /= 10;
    } while (n != 0);
    if (i != 0)
      s += '\n';
    if (last - p < 4)
      s.append(static_cast<size_t>(4 - (last - p)), ' ');
    s.append(p, last);
    s += "# ";
    s += std::to_string(__st[i]);
  }
  return s;
}

// Defined in the hosted runtime (with <ostream>).
ostream& operator<<(ostream& __os, const stacktrace_entry& __f);
template <class _Allocator>
ostream& operator<<(ostream& __os, const basic_stacktrace<_Allocator>& __st) {
  return __os << std::to_string(__st);
}

// ---- [stacktrace.basic.hash] ----
template <>
struct hash<stacktrace_entry> {
  size_t operator()(const stacktrace_entry& e) const noexcept { return hash<uintptr_t>()(e.native_handle()); }
};
template <class _Allocator>
struct hash<basic_stacktrace<_Allocator>> {
  size_t operator()(const basic_stacktrace<_Allocator>& __st) const noexcept {
    size_t h = __st.size();
    for (const stacktrace_entry& e : __st)
      h = h * 0x9e3779b97f4a7c15ull + hash<stacktrace_entry>()(e);
    return h;
  }
};

}} // namespace std
