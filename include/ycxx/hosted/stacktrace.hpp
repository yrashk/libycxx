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
// formatter<stacktrace_entry> and formatter<basic_stacktrace<A>> ([stacktrace.format]) are not
// provided here yet: they belong with <format>.
#pragma once

#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/hosted/iosfwd.hpp>
#include <ycxx/core/memory_resource.hpp>
#include <ycxx/core/vector.hpp>

namespace std {
class stacktrace_entry;
}

namespace ycxx::detail {

// src/hosted/stacktrace.cpp. Writes to buf the addresses of at most n frames of the calling
// thread's stack, starting with the frame whose return address is ra (the caller of
// basic_stacktrace::current) and skipping the first skip of those; returns how many it wrote.
// With a null buf it only counts them.
std::size_t stacktrace_capture(const void* ra, std::size_t skip, std::uintptr_t* buf, std::size_t n) noexcept;

// The queries of [stacktrace.entry.query] for the address pc; what selects the result.
enum class stacktrace_query { description, source_file };
std::string stacktrace_describe(std::uintptr_t pc, stacktrace_query what);
std::uint_least32_t stacktrace_line(std::uintptr_t pc);

} // namespace ycxx::detail

namespace std {

// [stacktrace.entry]
class stacktrace_entry {
public:
  using native_handle_type = uintptr_t;

  constexpr stacktrace_entry() noexcept = default;
  constexpr stacktrace_entry(const stacktrace_entry& other) noexcept = default;
  constexpr stacktrace_entry& operator=(const stacktrace_entry& other) noexcept = default;
  ~stacktrace_entry() = default;

  constexpr native_handle_type native_handle() const noexcept { return pc_; }
  constexpr explicit operator bool() const noexcept { return pc_ != 0; }

  string description() const {
    return pc_ != 0 ? ycxx::detail::stacktrace_describe(pc_, ycxx::detail::stacktrace_query::description) : string();
  }
  string source_file() const {
    return pc_ != 0 ? ycxx::detail::stacktrace_describe(pc_, ycxx::detail::stacktrace_query::source_file) : string();
  }
  uint_least32_t source_line() const { return pc_ != 0 ? ycxx::detail::stacktrace_line(pc_) : 0; }

  friend constexpr bool operator==(const stacktrace_entry& x, const stacktrace_entry& y) noexcept {
    return x.pc_ == y.pc_;
  }
  friend constexpr strong_ordering operator<=>(const stacktrace_entry& x, const stacktrace_entry& y) noexcept {
    return x.pc_ <=> y.pc_;
  }

private:
  template <class Allocator>
  friend class basic_stacktrace;
  constexpr explicit stacktrace_entry(uintptr_t pc) noexcept : pc_(pc) {}

  uintptr_t pc_ = 0;
};

// [stacktrace.basic]
template <class Allocator>
class basic_stacktrace {
  using frames_type = vector<stacktrace_entry, Allocator>;

public:
  using value_type = stacktrace_entry;
  using const_reference = const value_type&;
  using reference = value_type&;
  using const_iterator = typename frames_type::const_iterator;
  using iterator = const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using difference_type = typename frames_type::difference_type;
  using size_type = typename frames_type::size_type;
  using allocator_type = Allocator;

  // ---- [stacktrace.basic.cons] ----
  [[gnu::noinline]] static basic_stacktrace current(const allocator_type& alloc = allocator_type()) noexcept {
    return capture(__builtin_return_address(0), 0, static_cast<size_type>(-1), alloc);
  }
  [[gnu::noinline]] static basic_stacktrace current(size_type skip,
                                                    const allocator_type& alloc = allocator_type()) noexcept {
    return capture(__builtin_return_address(0), skip, static_cast<size_type>(-1), alloc);
  }
  [[gnu::noinline]] static basic_stacktrace current(size_type skip, size_type max_depth,
                                                    const allocator_type& alloc = allocator_type()) noexcept {
    ycxx::detail::precondition(skip <= skip + max_depth, "std::basic_stacktrace::current: skip + max_depth overflows");
    return capture(__builtin_return_address(0), skip, max_depth, alloc);
  }

  basic_stacktrace() noexcept(is_nothrow_default_constructible_v<allocator_type>) = default;
  explicit basic_stacktrace(const allocator_type& alloc) noexcept : frames_(alloc) {}
  basic_stacktrace(const basic_stacktrace& other) = default;
  basic_stacktrace(basic_stacktrace&& other) noexcept = default;
  basic_stacktrace(const basic_stacktrace& other, const allocator_type& alloc) : frames_(other.frames_, alloc) {}
  basic_stacktrace(basic_stacktrace&& other, const allocator_type& alloc)
      : frames_(static_cast<frames_type&&>(other.frames_), alloc) {}
  basic_stacktrace& operator=(const basic_stacktrace& other) = default;
  basic_stacktrace& operator=(basic_stacktrace&& other) noexcept(
      allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
      allocator_traits<Allocator>::is_always_equal::value) = default;
  ~basic_stacktrace() = default;

  // ---- [stacktrace.basic.obs] ----
  allocator_type get_allocator() const noexcept { return frames_.get_allocator(); }
  const_iterator begin() const noexcept { return frames_.begin(); }
  const_iterator end() const noexcept { return frames_.end(); }
  const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(cend()); }
  const_reverse_iterator rend() const noexcept { return const_reverse_iterator(cbegin()); }
  const_iterator cbegin() const noexcept { return frames_.cbegin(); }
  const_iterator cend() const noexcept { return frames_.cend(); }
  const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
  const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }
  [[nodiscard]] bool empty() const noexcept { return frames_.empty(); }
  size_type size() const noexcept { return frames_.size(); }
  size_type max_size() const noexcept { return frames_.max_size(); }
  const_reference operator[](size_type frame_no) const {
    ycxx::detail::precondition(frame_no < size(), "std::basic_stacktrace::operator[]: index out of range");
    return frames_[frame_no];
  }
  const_reference at(size_type frame_no) const {
    if (frame_no >= size())
      ycxx::detail::throw_out_of_range("std::basic_stacktrace::at: index out of range");
    return frames_[frame_no];
  }

  // ---- [stacktrace.basic.cmp] ----
  template <class Allocator2>
  friend bool operator==(const basic_stacktrace& x, const basic_stacktrace<Allocator2>& y) noexcept {
    if (x.size() != y.size())
      return false;
    for (size_type i = 0; i < x.size(); ++i)
      if (!(x.frames_[i] == y[i]))
        return false;
    return true;
  }
  template <class Allocator2>
  friend strong_ordering operator<=>(const basic_stacktrace& x, const basic_stacktrace<Allocator2>& y) noexcept {
    if (x.size() != y.size())
      return x.size() <=> y.size();
    for (size_type i = 0; i < x.size(); ++i)
      if (const strong_ordering c = x.frames_[i] <=> y[i]; c != 0)
        return c;
    return strong_ordering::equal;
  }

  // ---- [stacktrace.basic.mod] ----
  void swap(basic_stacktrace& other) noexcept(allocator_traits<Allocator>::propagate_on_container_swap::value ||
                                             allocator_traits<Allocator>::is_always_equal::value) {
    frames_.swap(other.frames_);
  }

private:
  frames_type frames_;

  // The frames of the stack from the caller of current() (whose return address is ra) on, after
  // skip, at most max_depth of them; empty if they cannot be stored.
  static basic_stacktrace capture(const void* ra, size_type skip, size_type max_depth,
                                  const allocator_type& alloc) noexcept {
    basic_stacktrace st(alloc);
    const auto fill = [&] {
      // The exact number of frames first: one allocation of exactly that many entries.
      const size_t total = ycxx::detail::stacktrace_capture(ra, skip, nullptr, max_depth);
      st.frames_.reserve(total);
      constexpr size_t chunk = 64;
      uintptr_t buf[chunk];
      size_t pos = skip;
      while (st.frames_.size() < total) {
        const size_t left = total - st.frames_.size();
        const size_t want = left < chunk ? left : chunk;
        const size_t got = ycxx::detail::stacktrace_capture(ra, pos, buf, want);
        for (size_t i = 0; i < got; ++i)
          st.frames_.push_back(stacktrace_entry(buf[i]));
        if (got < want)
          break;
        pos += got;
      }
    };
    if constexpr (ycxx::detail::cfg::exceptions) {
      try {
        fill();
      } catch (...) {
        return basic_stacktrace(alloc);
      }
    } else {
      fill();
    }
    return st;
  }
};

using stacktrace = basic_stacktrace<allocator<stacktrace_entry>>;
namespace pmr {
using stacktrace = basic_stacktrace<polymorphic_allocator<stacktrace_entry>>;
}

// ---- [stacktrace.basic.nonmem] ----
template <class Allocator>
void swap(basic_stacktrace<Allocator>& a, basic_stacktrace<Allocator>& b) noexcept(noexcept(a.swap(b))) {
  a.swap(b);
}

// "DESCRIPTION at FILE:LINE", leaving out what is not known (src/hosted/stacktrace.cpp).
string to_string(const stacktrace_entry& f);

// One line per entry: its number, right-aligned in four columns, "# " and to_string of the entry.
template <class Allocator>
string to_string(const basic_stacktrace<Allocator>& st) {
  string s;
  for (typename basic_stacktrace<Allocator>::size_type i = 0; i < st.size(); ++i) {
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
    s += std::to_string(st[i]);
  }
  return s;
}

// Defined in the hosted runtime (with <ostream>).
ostream& operator<<(ostream& os, const stacktrace_entry& f);
template <class Allocator>
ostream& operator<<(ostream& os, const basic_stacktrace<Allocator>& st) {
  return os << std::to_string(st);
}

// ---- [stacktrace.basic.hash] ----
template <>
struct hash<stacktrace_entry> {
  size_t operator()(const stacktrace_entry& e) const noexcept { return hash<uintptr_t>()(e.native_handle()); }
};
template <class Allocator>
struct hash<basic_stacktrace<Allocator>> {
  size_t operator()(const basic_stacktrace<Allocator>& st) const noexcept {
    size_t h = st.size();
    for (const stacktrace_entry& e : st)
      h = h * 0x9e3779b97f4a7c15ull + hash<stacktrace_entry>()(e);
    return h;
  }
};

} // namespace std
