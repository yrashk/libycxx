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

// Bidirectional (not random-access) iterator over a pointer range; counts dereferences.
template <class T>
struct BidiIter {
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = std::remove_cv_t<T>;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;
  T* p = nullptr;
  int* derefs = nullptr;
  constexpr BidiIter() = default;
  constexpr explicit BidiIter(T* q, int* d = nullptr) : p(q), derefs(d) {}
  constexpr reference operator*() const {
    if (derefs) ++*derefs;
    return *p;
  }
  constexpr BidiIter& operator++() {
    ++p;
    return *this;
  }
  constexpr BidiIter operator++(int) {
    BidiIter t = *this;
    ++p;
    return t;
  }
  constexpr BidiIter& operator--() {
    --p;
    return *this;
  }
  constexpr BidiIter operator--(int) {
    BidiIter t = *this;
    --p;
    return t;
  }
  friend constexpr bool operator==(const BidiIter& a, const BidiIter& b) { return a.p == b.p; }
};

// Random-access (not contiguous) iterator over a pointer range, a class type rather than a
// pointer.
template <class T>
struct RandomIter {
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept = std::random_access_iterator_tag;
  using value_type = std::remove_cv_t<T>;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;
  T* p = nullptr;
  int* derefs = nullptr;
  constexpr RandomIter() = default;
  constexpr explicit RandomIter(T* q, int* d = nullptr) : p(q), derefs(d) {}
  constexpr reference operator*() const {
    if (derefs) ++*derefs;
    return *p;
  }
  constexpr reference operator[](difference_type n) const { return *(*this + n); }
  constexpr RandomIter& operator++() {
    ++p;
    return *this;
  }
  constexpr RandomIter operator++(int) {
    RandomIter t = *this;
    ++p;
    return t;
  }
  constexpr RandomIter& operator--() {
    --p;
    return *this;
  }
  constexpr RandomIter operator--(int) {
    RandomIter t = *this;
    --p;
    return t;
  }
  constexpr RandomIter& operator+=(difference_type n) {
    p += n;
    return *this;
  }
  constexpr RandomIter& operator-=(difference_type n) {
    p -= n;
    return *this;
  }
  friend constexpr RandomIter operator+(RandomIter i, difference_type n) { return i += n; }
  friend constexpr RandomIter operator+(difference_type n, RandomIter i) { return i += n; }
  friend constexpr RandomIter operator-(RandomIter i, difference_type n) { return i -= n; }
  friend constexpr difference_type operator-(const RandomIter& a, const RandomIter& b) { return a.p - b.p; }
  friend constexpr bool operator==(const RandomIter& a, const RandomIter& b) { return a.p == b.p; }
  friend constexpr auto operator<=>(const RandomIter& a, const RandomIter& b) { return a.p <=> b.p; }
};

// A bidirectional_range that is not random-access and not common.
template <class T>
struct BidiRange {
  T* b;
  T* e;
  constexpr BidiIter<T> begin() const { return BidiIter<T>(b); }
  constexpr PtrSentinel<T> end() const { return PtrSentinel<T>{e}; }
};

// A random_access_range that is not common (sentinel_for but not sized_sentinel_for).
template <class T>
struct RandomRange {
  T* b;
  T* e;
  constexpr RandomIter<T> begin() const { return RandomIter<T>(b); }
  constexpr PtrSentinel<T> end() const { return PtrSentinel<T>{e}; }
};
