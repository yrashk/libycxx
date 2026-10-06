// libycxx core: <bitset> ([template.bitset]).
//
// Storage: ceil(N / 64) 64-bit words, bit i in word i / 64 at position i % 64. Invariant: the
// unused high bits of the last word are always zero, so count(), ==, all() and to_ullong() can
// work on whole words.
//
// The basic_string constructor and to_string() are written against a declaration of
// basic_string; being templates, they work once <string> is included. The stream operators come
// with the iostreams.
#pragma once

#include <ycxx/core/bit.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/string_view.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class __charT, class __traits, class _Allocator>
class basic_string;
template <class _Tp>
struct hash;

template <size_t _Np>
class bitset {
  using __word = unsigned long long;
  static constexpr size_t __word_bits = 64;
  static constexpr size_t _Wp = (_Np + __word_bits - 1) / __word_bits;
  static constexpr size_t __storage_words = _Wp == 0 ? 1 : _Wp;
  // Mask of the valid bits in the last word.
  static constexpr __word __last_mask = _Np % __word_bits == 0 ? ~__word(0) : (__word(1) << (_Np % __word_bits)) - 1;

  __word __w_[__storage_words] = {};

  constexpr void __sanitize() noexcept {
    if constexpr (_Wp != 0)
      __w_[_Wp - 1] &= __last_mask;
  }
  constexpr bool get(size_t __pos) const noexcept { return (__w_[__pos / __word_bits] >> (__pos % __word_bits)) & 1; }
  constexpr void put(size_t __pos, bool __v) noexcept {
    const __word m = __word(1) << (__pos % __word_bits);
    if (__v)
      __w_[__pos / __word_bits] |= m;
    else
      __w_[__pos / __word_bits] &= ~m;
  }
  static constexpr void __check_pos(size_t __pos, const char* what) {
    if (__pos >= _Np)
      __ycxx::__detail::__throw_out_of_range(what);
  }

  template <class __charT, class __traits>
  constexpr void __init_from(basic_string_view<__charT, __traits> str, size_t __pos, size_t n, __charT zero, __charT __one) {
    if (__pos > str.size())
      __ycxx::__detail::__throw_out_of_range("std::bitset: pos > str.size()");
    const size_t __rlen = n < str.size() - __pos ? n : str.size() - __pos;
    for (size_t i = 0; i < __rlen; ++i) {
      const __charT c = str[__pos + i];
      if (!__traits::eq(c, zero) && !__traits::eq(c, __one))
        __ycxx::__detail::__throw_invalid_argument("std::bitset: character is neither zero nor one");
    }
    const size_t m = _Np < __rlen ? _Np : __rlen;
    // Character position pos + m - 1 is bit 0.
    for (size_t b = 0; b < m; ++b)
      if (__traits::eq(str[__pos + m - 1 - b], __one))
        put(b, true);
  }

public:
  class reference {
    friend class bitset;
    bitset* __b_;
    size_t __pos_;
    constexpr reference(bitset* b, size_t __pos) noexcept : __b_(b), __pos_(__pos) {}

  public:
    constexpr reference(const reference&) noexcept = default;
    constexpr ~reference() = default;
    constexpr reference& operator=(bool __x) noexcept {
      __b_->put(__pos_, __x);
      return *this;
    }
    constexpr reference& operator=(const reference& __x) noexcept {
      __b_->put(__pos_, bool(__x));
      return *this;
    }
    constexpr const reference& operator=(bool __x) const noexcept {
      __b_->put(__pos_, __x);
      return *this;
    }
    constexpr operator bool() const noexcept { return __b_->get(__pos_); }
    constexpr bool operator~() const noexcept { return !__b_->get(__pos_); }
    constexpr reference& flip() noexcept {
      __b_->put(__pos_, !__b_->get(__pos_));
      return *this;
    }
    friend constexpr void swap(reference __x, reference y) noexcept {
      bool b = __x;
      __x = bool(y);
      y = b;
    }
    friend constexpr void swap(reference __x, bool& y) noexcept {
      bool b = __x;
      __x = y;
      y = b;
    }
    friend constexpr void swap(bool& __x, reference y) noexcept {
      bool b = __x;
      __x = bool(y);
      y = b;
    }
  };

  // ---- [bitset.cons] ----
  constexpr bitset() noexcept = default;
  constexpr bitset(unsigned long long __val) noexcept {
    if constexpr (_Wp != 0) {
      __w_[0] = __val;
      __sanitize();
    }
  }
  template <class __charT, class __traits, class _Allocator>
  constexpr explicit bitset(const basic_string<__charT, __traits, _Allocator>& str,
                            typename basic_string<__charT, __traits, _Allocator>::size_type __pos = 0,
                            typename basic_string<__charT, __traits, _Allocator>::size_type n =
                                basic_string<__charT, __traits, _Allocator>::npos,
                            __charT zero = __charT('0'), __charT __one = __charT('1')) {
    __init_from(basic_string_view<__charT, __traits>(str.data(), str.size()), __pos, n, zero, __one);
  }
  template <class __charT, class __traits>
  constexpr explicit bitset(basic_string_view<__charT, __traits> str,
                            typename basic_string_view<__charT, __traits>::size_type __pos = 0,
                            typename basic_string_view<__charT, __traits>::size_type n = basic_string_view<__charT, __traits>::npos,
                            __charT zero = __charT('0'), __charT __one = __charT('1')) {
    __init_from(str, __pos, n, zero, __one);
  }
  template <class __charT>
    requires(!is_array_v<__charT>) && is_trivially_copyable_v<__charT> && is_standard_layout_v<__charT> &&
            is_trivially_default_constructible_v<__charT>
  constexpr explicit bitset(const __charT* str,
                            typename basic_string_view<__charT>::size_type n = basic_string_view<__charT>::npos,
                            __charT zero = __charT('0'), __charT __one = __charT('1')) {
    __init_from(n == basic_string_view<__charT>::npos ? basic_string_view<__charT>(str) : basic_string_view<__charT>(str, n), 0,
              n, zero, __one);
  }

  // ---- [bitset.members] ----
  constexpr bitset& operator&=(const bitset& __rhs) noexcept {
    for (size_t i = 0; i < _Wp; ++i)
      __w_[i] &= __rhs.__w_[i];
    return *this;
  }
  constexpr bitset& operator|=(const bitset& __rhs) noexcept {
    for (size_t i = 0; i < _Wp; ++i)
      __w_[i] |= __rhs.__w_[i];
    return *this;
  }
  constexpr bitset& operator^=(const bitset& __rhs) noexcept {
    for (size_t i = 0; i < _Wp; ++i)
      __w_[i] ^= __rhs.__w_[i];
    return *this;
  }
  constexpr bitset& operator<<=(size_t __pos) noexcept {
    if (__pos >= _Np) {
      reset();
      return *this;
    }
    const size_t ws = __pos / __word_bits, __bs = __pos % __word_bits;
    for (size_t i = _Wp; i-- > 0;) {
      __word __v = i >= ws ? __w_[i - ws] << __bs : 0;
      if (__bs != 0 && i >= ws + 1)
        __v |= __w_[i - ws - 1] >> (__word_bits - __bs);
      __w_[i] = __v;
    }
    __sanitize();
    return *this;
  }
  constexpr bitset& operator>>=(size_t __pos) noexcept {
    if (__pos >= _Np) {
      reset();
      return *this;
    }
    const size_t ws = __pos / __word_bits, __bs = __pos % __word_bits;
    for (size_t i = 0; i < _Wp; ++i) {
      __word __v = i + ws < _Wp ? __w_[i + ws] >> __bs : 0;
      if (__bs != 0 && i + ws + 1 < _Wp)
        __v |= __w_[i + ws + 1] << (__word_bits - __bs);
      __w_[i] = __v;
    }
    return *this;
  }
  constexpr bitset operator<<(size_t __pos) const noexcept { return bitset(*this) <<= __pos; }
  constexpr bitset operator>>(size_t __pos) const noexcept { return bitset(*this) >>= __pos; }

  constexpr bitset& set() noexcept {
    for (size_t i = 0; i < _Wp; ++i)
      __w_[i] = ~__word(0);
    __sanitize();
    return *this;
  }
  constexpr bitset& set(size_t __pos, bool __val = true) {
    __check_pos(__pos, "std::bitset::set: pos out of range");
    put(__pos, __val);
    return *this;
  }
  constexpr bitset& reset() noexcept {
    for (size_t i = 0; i < _Wp; ++i)
      __w_[i] = 0;
    return *this;
  }
  constexpr bitset& reset(size_t __pos) {
    __check_pos(__pos, "std::bitset::reset: pos out of range");
    put(__pos, false);
    return *this;
  }
  constexpr bitset operator~() const noexcept { return bitset(*this).flip(); }
  constexpr bitset& flip() noexcept {
    for (size_t i = 0; i < _Wp; ++i)
      __w_[i] = ~__w_[i];
    __sanitize();
    return *this;
  }
  constexpr bitset& flip(size_t __pos) {
    __check_pos(__pos, "std::bitset::flip: pos out of range");
    put(__pos, !get(__pos));
    return *this;
  }

  constexpr bool operator[](size_t __pos) const {
    __ycxx::__detail::__precondition(__pos < _Np, "std::bitset::operator[]: pos out of range");
    return get(__pos);
  }
  constexpr reference operator[](size_t __pos) {
    __ycxx::__detail::__precondition(__pos < _Np, "std::bitset::operator[]: pos out of range");
    return reference(this, __pos);
  }

  constexpr unsigned long to_ulong() const {
    const unsigned long long __v = to_ullong();
    if (__v > static_cast<unsigned long long>(static_cast<unsigned long>(-1)))
      __ycxx::__detail::__throw_overflow_error("std::bitset::to_ulong: value does not fit in unsigned long");
    return static_cast<unsigned long>(__v);
  }
  constexpr unsigned long long to_ullong() const {
    for (size_t i = 1; i < _Wp; ++i)
      if (__w_[i] != 0)
        __ycxx::__detail::__throw_overflow_error("std::bitset::to_ullong: value does not fit in unsigned long long");
    return _Wp == 0 ? 0 : __w_[0];
  }
  template <class __charT = char, class __traits = char_traits<__charT>, class _Allocator = allocator<__charT>>
  constexpr basic_string<__charT, __traits, _Allocator> to_string(__charT zero = __charT('0'), __charT __one = __charT('1')) const {
    basic_string<__charT, __traits, _Allocator> s(_Np, zero);
    for (size_t b = 0; b < _Np; ++b)
      if (get(b))
        __traits::assign(s[_Np - 1 - b], __one);
    return s;
  }

  constexpr size_t count() const noexcept {
    size_t c = 0;
    for (size_t i = 0; i < _Wp; ++i)
      c += static_cast<size_t>(std::popcount(__w_[i]));
    return c;
  }
  constexpr size_t size() const noexcept { return _Np; }
  constexpr bool operator==(const bitset& __rhs) const noexcept {
    for (size_t i = 0; i < _Wp; ++i)
      if (__w_[i] != __rhs.__w_[i])
        return false;
    return true;
  }
  constexpr bool test(size_t __pos) const {
    __check_pos(__pos, "std::bitset::test: pos out of range");
    return get(__pos);
  }
  constexpr bool all() const noexcept {
    if constexpr (_Wp == 0)
      return true;
    else {
      for (size_t i = 0; i + 1 < _Wp; ++i)
        if (__w_[i] != ~__word(0))
          return false;
      return __w_[_Wp - 1] == __last_mask;
    }
  }
  constexpr bool any() const noexcept {
    for (size_t i = 0; i < _Wp; ++i)
      if (__w_[i] != 0)
        return true;
    return false;
  }
  constexpr bool none() const noexcept { return !any(); }

private:
  // hash<bitset<N>> hashes the words; the invariant keeps the unused bits zero.
  friend struct hash<bitset>;
  constexpr size_t hash_value() const noexcept { return static_cast<size_t>(__ycxx::__detail::__hash_chars(__w_, _Wp)); }
};

// ---- [bitset.operators] ----
template <size_t _Np>
constexpr bitset<_Np> operator&(const bitset<_Np>& __lhs, const bitset<_Np>& __rhs) noexcept {
  return bitset<_Np>(__lhs) &= __rhs;
}
template <size_t _Np>
constexpr bitset<_Np> operator|(const bitset<_Np>& __lhs, const bitset<_Np>& __rhs) noexcept {
  return bitset<_Np>(__lhs) |= __rhs;
}
template <size_t _Np>
constexpr bitset<_Np> operator^(const bitset<_Np>& __lhs, const bitset<_Np>& __rhs) noexcept {
  return bitset<_Np>(__lhs) ^= __rhs;
}

// [bitset.operators]: the stream operators, declared here and defined with the streams
// (ycxx/hosted/istream.hpp, ycxx/hosted/ostream.hpp).
template <class __charT, class __traits, size_t _Np>
basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, bitset<_Np>& __x);
template <class __charT, class __traits, size_t _Np>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const bitset<_Np>& __x);

// ---- [bitset.hash] ----
template <size_t _Np>
struct hash<bitset<_Np>> {
  [[nodiscard]] size_t operator()(const bitset<_Np>& b) const noexcept { return b.hash_value(); }
};

} // namespace std
