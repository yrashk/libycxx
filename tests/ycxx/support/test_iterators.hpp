// Iterator and range archetypes for libycxx's own suite, written from
// [iterator.requirements] and [range.req]. Independent of every other test suite.
#pragma once
#include <cstddef>
#include <iterator>
#include <type_traits>

// Single-pass input iterator over a pointer range (Cpp17InputIterator and input_iterator,
// but not forward). Optionally counts dereferences through *derefs.
template <class T>
struct InputIter {
  using iterator_category = std::input_iterator_tag;
  using iterator_concept = std::input_iterator_tag;
  using value_type = std::remove_cv_t<T>;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;
  T* p = nullptr;
  int* derefs = nullptr;
  constexpr InputIter() = default;
  constexpr explicit InputIter(T* q, int* d = nullptr) : p(q), derefs(d) {}
  constexpr reference operator*() const {
    if (derefs) ++*derefs;
    return *p;
  }
  constexpr InputIter& operator++() {
    ++p;
    return *this;
  }
  constexpr InputIter operator++(int) {
    InputIter t = *this;
    ++p;
    return t;
  }
  friend constexpr bool operator==(const InputIter& a, const InputIter& b) { return a.p == b.p; }
};

// Forward iterator that counts dereferences.
template <class T>
struct ForwardIter {
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::remove_cv_t<T>;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;
  T* p = nullptr;
  int* derefs = nullptr;
  constexpr ForwardIter() = default;
  constexpr explicit ForwardIter(T* q, int* d = nullptr) : p(q), derefs(d) {}
  constexpr reference operator*() const {
    if (derefs) ++*derefs;
    return *p;
  }
  constexpr ForwardIter& operator++() {
    ++p;
    return *this;
  }
  constexpr ForwardIter operator++(int) {
    ForwardIter t = *this;
    ++p;
    return t;
  }
  friend constexpr bool operator==(const ForwardIter& a, const ForwardIter& b) { return a.p == b.p; }
};

// Sentinel for the iterators above, so ranges built from them are non-common.
template <class T>
struct PtrSentinel {
  T* p = nullptr;
  template <template <class> class It>
  friend constexpr bool operator==(const It<T>& i, const PtrSentinel& s) { return i.p == s.p; }
};

// An input_range that is neither forward nor sized.
template <class T>
struct InputRange {
  T* b;
  T* e;
  int* derefs = nullptr;
  constexpr InputIter<T> begin() const { return InputIter<T>(b, derefs); }
  constexpr PtrSentinel<T> end() const { return PtrSentinel<T>{e}; }
};

// A forward_range that is not sized and not common.
template <class T>
struct ForwardRange {
  T* b;
  T* e;
  int* derefs = nullptr;
  constexpr ForwardIter<T> begin() const { return ForwardIter<T>(b, derefs); }
  constexpr PtrSentinel<T> end() const { return PtrSentinel<T>{e}; }
};
