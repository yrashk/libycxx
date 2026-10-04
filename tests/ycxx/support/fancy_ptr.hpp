// A class-type ("fancy") allocator pointer and an allocator using it, for libycxx's own suite.
// Written from [allocator.requirements.general]: XX::pointer meets the Cpp17NullablePointer,
// Cpp17RandomAccessIterator and contiguous iterator requirements; void_pointer and
// const_void_pointer meet Cpp17NullablePointer; pointer converts to const_pointer,
// void_pointer and const_void_pointer, and static_cast from void_pointer to pointer works;
// pointer_traits<pointer>::pointer_to(r) is valid. Everything is constexpr so the types can be
// used in constant evaluation. Independent of every other test suite.
#pragma once
#include <compare>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

template <class T>
struct FancyPtr;

// FancyPtr<void> and FancyPtr<const void>
template <class T>
  requires std::is_void_v<T>
struct FancyPtr<T> {
  using element_type = T;
  using difference_type = std::ptrdiff_t;
  T* p = nullptr;
  constexpr FancyPtr() = default;
  constexpr FancyPtr(std::nullptr_t) {}
  constexpr explicit FancyPtr(T* q) : p(q) {}
  template <class U>
    requires std::is_convertible_v<U*, T*>
  constexpr FancyPtr(const FancyPtr<U>& o) : p(o.p) {}
  constexpr explicit operator bool() const { return p != nullptr; }
  friend constexpr bool operator==(const FancyPtr&, const FancyPtr&) = default;
  friend constexpr bool operator==(const FancyPtr& a, std::nullptr_t) { return a.p == nullptr; }
};

template <class T>
struct FancyPtr {
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using difference_type = std::ptrdiff_t;
  using reference = T&;
  using pointer = T*;
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept = std::contiguous_iterator_tag;

  T* p = nullptr;
  constexpr FancyPtr() = default;
  constexpr FancyPtr(std::nullptr_t) {}
  constexpr explicit FancyPtr(T* q) : p(q) {}
  template <class U>
    requires(!std::is_void_v<U> && std::is_convertible_v<U*, T*>)
  constexpr FancyPtr(const FancyPtr<U>& o) : p(o.p) {}
  // static_cast from void_pointer / const_void_pointer
  template <class V>
    requires(std::is_void_v<V> && std::is_convertible_v<T*, V*>)
  constexpr explicit FancyPtr(const FancyPtr<V>& v) : p(static_cast<T*>(v.p)) {}

  static constexpr FancyPtr pointer_to(T& r) noexcept { return FancyPtr(std::addressof(r)); }

  constexpr T& operator*() const { return *p; }
  constexpr T* operator->() const { return p; }
  constexpr T& operator[](difference_type n) const { return p[n]; }
  constexpr FancyPtr& operator++() {
    ++p;
    return *this;
  }
  constexpr FancyPtr operator++(int) {
    FancyPtr t = *this;
    ++p;
    return t;
  }
  constexpr FancyPtr& operator--() {
    --p;
    return *this;
  }
  constexpr FancyPtr operator--(int) {
    FancyPtr t = *this;
    --p;
    return t;
  }
  constexpr FancyPtr& operator+=(difference_type n) {
    p += n;
    return *this;
  }
  constexpr FancyPtr& operator-=(difference_type n) {
    p -= n;
    return *this;
  }
  friend constexpr FancyPtr operator+(FancyPtr a, difference_type n) { return FancyPtr(a.p + n); }
  friend constexpr FancyPtr operator+(difference_type n, FancyPtr a) { return FancyPtr(a.p + n); }
  friend constexpr FancyPtr operator-(FancyPtr a, difference_type n) { return FancyPtr(a.p - n); }
  friend constexpr difference_type operator-(FancyPtr a, FancyPtr b) { return a.p - b.p; }
  constexpr explicit operator bool() const { return p != nullptr; }
  friend constexpr bool operator==(const FancyPtr&, const FancyPtr&) = default;
  friend constexpr auto operator<=>(const FancyPtr& a, const FancyPtr& b) { return a.p <=> b.p; }
  friend constexpr bool operator==(const FancyPtr& a, std::nullptr_t) { return a.p == nullptr; }
};

static_assert(std::contiguous_iterator<FancyPtr<int>>);
static_assert(std::contiguous_iterator<FancyPtr<const char>>);

// Allocator whose pointer is FancyPtr<T>. Stateless and always equal; counts nothing, so it
// can be used in constant evaluation.
template <class T>
struct FancyAlloc {
  using value_type = T;
  using pointer = FancyPtr<T>;
  using const_pointer = FancyPtr<const T>;
  using void_pointer = FancyPtr<void>;
  using const_void_pointer = FancyPtr<const void>;
  constexpr FancyAlloc() = default;
  template <class U>
  constexpr FancyAlloc(const FancyAlloc<U>&) noexcept {}
  constexpr pointer allocate(std::size_t n) { return pointer(std::allocator<T>{}.allocate(n)); }
  constexpr void deallocate(pointer p, std::size_t n) { std::allocator<T>{}.deallocate(p.p, n); }
  friend constexpr bool operator==(const FancyAlloc&, const FancyAlloc&) noexcept { return true; }
};
