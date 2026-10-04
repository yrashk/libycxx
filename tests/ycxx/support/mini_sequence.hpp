// A minimal program-defined sequence container for the container adaptor tests
// ([container.adaptors]): fixed capacity, front / back / push_back / pop_back / pop_front /
// emplace_back, with or without append_range (WithAppend). append_range calls are counted
// so a test can tell which way an adaptor's push_range inserted the elements
// ([queue.mod]/1, [stack.mod]/1, [priqueue.members]/3).
#pragma once
#include <cstddef>
#include <ranges>
#include <utility>

template <class T, bool WithAppend>
struct MiniSeq {
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = T*;
  using const_iterator = const T*;
  T data[64] = {};
  std::size_t first = 0, last = 0;
  int append_calls = 0;
  constexpr MiniSeq() = default;
  constexpr T* begin() { return data + first; }
  constexpr T* end() { return data + last; }
  constexpr const T* begin() const { return data + first; }
  constexpr const T* end() const { return data + last; }
  constexpr bool empty() const { return first == last; }
  constexpr std::size_t size() const { return last - first; }
  constexpr T& front() { return data[first]; }
  constexpr const T& front() const { return data[first]; }
  constexpr T& back() { return data[last - 1]; }
  constexpr const T& back() const { return data[last - 1]; }
  constexpr void push_back(const T& x) { data[last++] = x; }
  constexpr void push_back(T&& x) { data[last++] = std::move(x); }
  template <class... A>
  constexpr T& emplace_back(A&&... a) {
    data[last] = T(std::forward<A>(a)...);
    return data[last++];
  }
  constexpr void pop_back() { --last; }
  constexpr void pop_front() { ++first; }
  template <class R>
    requires WithAppend
  constexpr void append_range(R&& r) {
    ++append_calls;
    for (auto&& x : r) push_back(x);
  }
  friend constexpr bool operator==(const MiniSeq& a, const MiniSeq& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
      if (!(a.data[a.first + i] == b.data[b.first + i])) return false;
    return true;
  }
};
