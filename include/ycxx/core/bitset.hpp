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

namespace std {

template <class charT, class traits, class Allocator>
class basic_string;
template <class T>
struct hash;

template <size_t N>
class bitset {
  using word = unsigned long long;
  static constexpr size_t word_bits = 64;
  static constexpr size_t W = (N + word_bits - 1) / word_bits;
  static constexpr size_t storage_words = W == 0 ? 1 : W;
  // Mask of the valid bits in the last word.
  static constexpr word last_mask = N % word_bits == 0 ? ~word(0) : (word(1) << (N % word_bits)) - 1;

  word w_[storage_words] = {};

  constexpr void sanitize() noexcept {
    if constexpr (W != 0)
      w_[W - 1] &= last_mask;
  }
  constexpr bool get(size_t pos) const noexcept { return (w_[pos / word_bits] >> (pos % word_bits)) & 1; }
  constexpr void put(size_t pos, bool v) noexcept {
    const word m = word(1) << (pos % word_bits);
    if (v)
      w_[pos / word_bits] |= m;
    else
      w_[pos / word_bits] &= ~m;
  }
  static constexpr void check_pos(size_t pos, const char* what) {
    if (pos >= N)
      ycxx::detail::throw_out_of_range(what);
  }

  template <class charT, class traits>
  constexpr void init_from(basic_string_view<charT, traits> str, size_t pos, size_t n, charT zero, charT one) {
    if (pos > str.size())
      ycxx::detail::throw_out_of_range("std::bitset: pos > str.size()");
    const size_t rlen = n < str.size() - pos ? n : str.size() - pos;
    for (size_t i = 0; i < rlen; ++i) {
      const charT c = str[pos + i];
      if (!traits::eq(c, zero) && !traits::eq(c, one))
        ycxx::detail::throw_invalid_argument("std::bitset: character is neither zero nor one");
    }
    const size_t m = N < rlen ? N : rlen;
    // Character position pos + m - 1 is bit 0.
    for (size_t b = 0; b < m; ++b)
      if (traits::eq(str[pos + m - 1 - b], one))
        put(b, true);
  }

public:
  class reference {
    friend class bitset;
    bitset* b_;
    size_t pos_;
    constexpr reference(bitset* b, size_t pos) noexcept : b_(b), pos_(pos) {}

  public:
    constexpr reference(const reference&) noexcept = default;
    constexpr ~reference() = default;
    constexpr reference& operator=(bool x) noexcept {
      b_->put(pos_, x);
      return *this;
    }
    constexpr reference& operator=(const reference& x) noexcept {
      b_->put(pos_, bool(x));
      return *this;
    }
    constexpr const reference& operator=(bool x) const noexcept {
      b_->put(pos_, x);
      return *this;
    }
    constexpr operator bool() const noexcept { return b_->get(pos_); }
    constexpr bool operator~() const noexcept { return !b_->get(pos_); }
    constexpr reference& flip() noexcept {
      b_->put(pos_, !b_->get(pos_));
      return *this;
    }
    friend constexpr void swap(reference x, reference y) noexcept {
      bool b = x;
      x = bool(y);
      y = b;
    }
    friend constexpr void swap(reference x, bool& y) noexcept {
      bool b = x;
      x = y;
      y = b;
    }
    friend constexpr void swap(bool& x, reference y) noexcept {
      bool b = x;
      x = bool(y);
      y = b;
    }
  };

  // ---- [bitset.cons] ----
  constexpr bitset() noexcept = default;
  constexpr bitset(unsigned long long val) noexcept {
    if constexpr (W != 0) {
      w_[0] = val;
      sanitize();
    }
  }
  template <class charT, class traits, class Allocator>
  constexpr explicit bitset(const basic_string<charT, traits, Allocator>& str,
                            typename basic_string<charT, traits, Allocator>::size_type pos = 0,
                            typename basic_string<charT, traits, Allocator>::size_type n =
                                basic_string<charT, traits, Allocator>::npos,
                            charT zero = charT('0'), charT one = charT('1')) {
    init_from(basic_string_view<charT, traits>(str.data(), str.size()), pos, n, zero, one);
  }
  template <class charT, class traits>
  constexpr explicit bitset(basic_string_view<charT, traits> str,
                            typename basic_string_view<charT, traits>::size_type pos = 0,
                            typename basic_string_view<charT, traits>::size_type n = basic_string_view<charT, traits>::npos,
                            charT zero = charT('0'), charT one = charT('1')) {
    init_from(str, pos, n, zero, one);
  }
  template <class charT>
    requires(!is_array_v<charT>) && is_trivially_copyable_v<charT> && is_standard_layout_v<charT> &&
            is_trivially_default_constructible_v<charT>
  constexpr explicit bitset(const charT* str,
                            typename basic_string_view<charT>::size_type n = basic_string_view<charT>::npos,
                            charT zero = charT('0'), charT one = charT('1')) {
    init_from(n == basic_string_view<charT>::npos ? basic_string_view<charT>(str) : basic_string_view<charT>(str, n), 0,
              n, zero, one);
  }

  // ---- [bitset.members] ----
  constexpr bitset& operator&=(const bitset& rhs) noexcept {
    for (size_t i = 0; i < W; ++i)
      w_[i] &= rhs.w_[i];
    return *this;
  }
  constexpr bitset& operator|=(const bitset& rhs) noexcept {
    for (size_t i = 0; i < W; ++i)
      w_[i] |= rhs.w_[i];
    return *this;
  }
  constexpr bitset& operator^=(const bitset& rhs) noexcept {
    for (size_t i = 0; i < W; ++i)
      w_[i] ^= rhs.w_[i];
    return *this;
  }
  constexpr bitset& operator<<=(size_t pos) noexcept {
    if (pos >= N) {
      reset();
      return *this;
    }
    const size_t ws = pos / word_bits, bs = pos % word_bits;
    for (size_t i = W; i-- > 0;) {
      word v = i >= ws ? w_[i - ws] << bs : 0;
      if (bs != 0 && i >= ws + 1)
        v |= w_[i - ws - 1] >> (word_bits - bs);
      w_[i] = v;
    }
    sanitize();
    return *this;
  }
  constexpr bitset& operator>>=(size_t pos) noexcept {
    if (pos >= N) {
      reset();
      return *this;
    }
    const size_t ws = pos / word_bits, bs = pos % word_bits;
    for (size_t i = 0; i < W; ++i) {
      word v = i + ws < W ? w_[i + ws] >> bs : 0;
      if (bs != 0 && i + ws + 1 < W)
        v |= w_[i + ws + 1] << (word_bits - bs);
      w_[i] = v;
    }
    return *this;
  }
  constexpr bitset operator<<(size_t pos) const noexcept { return bitset(*this) <<= pos; }
  constexpr bitset operator>>(size_t pos) const noexcept { return bitset(*this) >>= pos; }

  constexpr bitset& set() noexcept {
    for (size_t i = 0; i < W; ++i)
      w_[i] = ~word(0);
    sanitize();
    return *this;
  }
  constexpr bitset& set(size_t pos, bool val = true) {
    check_pos(pos, "std::bitset::set: pos out of range");
    put(pos, val);
    return *this;
  }
  constexpr bitset& reset() noexcept {
    for (size_t i = 0; i < W; ++i)
      w_[i] = 0;
    return *this;
  }
  constexpr bitset& reset(size_t pos) {
    check_pos(pos, "std::bitset::reset: pos out of range");
    put(pos, false);
    return *this;
  }
  constexpr bitset operator~() const noexcept { return bitset(*this).flip(); }
  constexpr bitset& flip() noexcept {
    for (size_t i = 0; i < W; ++i)
      w_[i] = ~w_[i];
    sanitize();
    return *this;
  }
  constexpr bitset& flip(size_t pos) {
    check_pos(pos, "std::bitset::flip: pos out of range");
    put(pos, !get(pos));
    return *this;
  }

  constexpr bool operator[](size_t pos) const {
    ycxx::detail::precondition(pos < N, "std::bitset::operator[]: pos out of range");
    return get(pos);
  }
  constexpr reference operator[](size_t pos) {
    ycxx::detail::precondition(pos < N, "std::bitset::operator[]: pos out of range");
    return reference(this, pos);
  }

  constexpr unsigned long to_ulong() const {
    const unsigned long long v = to_ullong();
    if (v > static_cast<unsigned long long>(static_cast<unsigned long>(-1)))
      ycxx::detail::throw_overflow_error("std::bitset::to_ulong: value does not fit in unsigned long");
    return static_cast<unsigned long>(v);
  }
  constexpr unsigned long long to_ullong() const {
    for (size_t i = 1; i < W; ++i)
      if (w_[i] != 0)
        ycxx::detail::throw_overflow_error("std::bitset::to_ullong: value does not fit in unsigned long long");
    return W == 0 ? 0 : w_[0];
  }
  template <class charT = char, class traits = char_traits<charT>, class Allocator = allocator<charT>>
  constexpr basic_string<charT, traits, Allocator> to_string(charT zero = charT('0'), charT one = charT('1')) const {
    basic_string<charT, traits, Allocator> s(N, zero);
    for (size_t b = 0; b < N; ++b)
      if (get(b))
        traits::assign(s[N - 1 - b], one);
    return s;
  }

  constexpr size_t count() const noexcept {
    size_t c = 0;
    for (size_t i = 0; i < W; ++i)
      c += static_cast<size_t>(std::popcount(w_[i]));
    return c;
  }
  constexpr size_t size() const noexcept { return N; }
  constexpr bool operator==(const bitset& rhs) const noexcept {
    for (size_t i = 0; i < W; ++i)
      if (w_[i] != rhs.w_[i])
        return false;
    return true;
  }
  constexpr bool test(size_t pos) const {
    check_pos(pos, "std::bitset::test: pos out of range");
    return get(pos);
  }
  constexpr bool all() const noexcept {
    if constexpr (W == 0)
      return true;
    else {
      for (size_t i = 0; i + 1 < W; ++i)
        if (w_[i] != ~word(0))
          return false;
      return w_[W - 1] == last_mask;
    }
  }
  constexpr bool any() const noexcept {
    for (size_t i = 0; i < W; ++i)
      if (w_[i] != 0)
        return true;
    return false;
  }
  constexpr bool none() const noexcept { return !any(); }

private:
  // hash<bitset<N>> hashes the words; the invariant keeps the unused bits zero.
  friend struct hash<bitset>;
  constexpr size_t hash_value() const noexcept { return static_cast<size_t>(ycxx::detail::hash_chars(w_, W)); }
};

// ---- [bitset.operators] ----
template <size_t N>
constexpr bitset<N> operator&(const bitset<N>& lhs, const bitset<N>& rhs) noexcept {
  return bitset<N>(lhs) &= rhs;
}
template <size_t N>
constexpr bitset<N> operator|(const bitset<N>& lhs, const bitset<N>& rhs) noexcept {
  return bitset<N>(lhs) |= rhs;
}
template <size_t N>
constexpr bitset<N> operator^(const bitset<N>& lhs, const bitset<N>& rhs) noexcept {
  return bitset<N>(lhs) ^= rhs;
}

// ---- [bitset.hash] ----
template <size_t N>
struct hash<bitset<N>> {
  [[nodiscard]] size_t operator()(const bitset<N>& b) const noexcept { return b.hash_value(); }
};

} // namespace std
