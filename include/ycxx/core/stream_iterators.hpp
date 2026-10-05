// libycxx core: the stream iterators of <iterator> ([stream.iterators]): istream_iterator,
// ostream_iterator, istreambuf_iterator, ostreambuf_iterator.
//
// They use only the streams' interface, through the declarations of ycxx/core/iosfwd.hpp: the
// stream and stream-buffer classes must be complete where an iterator's members are
// instantiated (anything that has a stream to give the iterator has included them).
#pragma once

#include <ycxx/core/char_traits.hpp>
#include <ycxx/core/iosfwd.hpp>
#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/iterator_ops.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// The library's extraction and insertion loops (num_get, money_get, ...) reach the buffer behind
// a stream-buffer iterator through this class, to work on whole runs of characters.
struct streambuf_iter_access;
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [istream.iterator]
template <class T, class charT = char, class traits = char_traits<charT>, class Distance = ptrdiff_t>
class istream_iterator {
public:
  using iterator_category = input_iterator_tag;
  using value_type = T;
  using difference_type = Distance;
  using pointer = const T*;
  using reference = const T&;
  using char_type = charT;
  using traits_type = traits;
  using istream_type = basic_istream<charT, traits>;

  constexpr istream_iterator() : in_stream_(nullptr), value_() {}
  constexpr istream_iterator(default_sentinel_t) : in_stream_(nullptr), value_() {}
  istream_iterator(istream_type& s) : in_stream_(__builtin_addressof(s)), value_() { ++*this; }
  constexpr istream_iterator(const istream_iterator& x) = default;
  ~istream_iterator() = default;
  istream_iterator& operator=(const istream_iterator&) = default;

  const T& operator*() const {
    ycxx::detail::precondition(in_stream_ != nullptr, "std::istream_iterator::operator*: end-of-stream iterator");
    return value_;
  }
  const T* operator->() const {
    ycxx::detail::precondition(in_stream_ != nullptr, "std::istream_iterator::operator->: end-of-stream iterator");
    return __builtin_addressof(value_);
  }
  istream_iterator& operator++() {
    ycxx::detail::precondition(in_stream_ != nullptr, "std::istream_iterator::operator++: end-of-stream iterator");
    if (!(*in_stream_ >> value_))
      in_stream_ = nullptr;
    return *this;
  }
  istream_iterator operator++(int) {
    istream_iterator tmp = *this;
    ++*this;
    return tmp;
  }

  friend bool operator==(const istream_iterator& i, default_sentinel_t) { return !i.in_stream_; }

private:
  template <class T2, class charT2, class traits2, class Distance2>
  friend bool operator==(const istream_iterator<T2, charT2, traits2, Distance2>& x,
                         const istream_iterator<T2, charT2, traits2, Distance2>& y);

  basic_istream<charT, traits>* in_stream_;
  T value_;
};

// [iterator.synopsis] declares the comparison of two iterators as a namespace-scope template
// (only the default_sentinel_t comparison is a hidden friend), so std::operator== names it.
template <class T, class charT, class traits, class Distance>
bool operator==(const istream_iterator<T, charT, traits, Distance>& x,
                const istream_iterator<T, charT, traits, Distance>& y) {
  return x.in_stream_ == y.in_stream_;
}

// [ostream.iterator]
template <class T, class charT = char, class traits = char_traits<charT>>
class ostream_iterator {
public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using char_type = charT;
  using traits_type = traits;
  using ostream_type = basic_ostream<charT, traits>;

  ostream_iterator(ostream_type& s) : out_stream_(__builtin_addressof(s)), delim_(nullptr) {}
  ostream_iterator(ostream_type& s, const charT* delimiter) : out_stream_(__builtin_addressof(s)), delim_(delimiter) {}
  ostream_iterator(const ostream_iterator& x) = default;
  ~ostream_iterator() = default;
  ostream_iterator& operator=(const ostream_iterator&) = default;

  ostream_iterator& operator=(const T& value) {
    *out_stream_ << value;
    if (delim_)
      *out_stream_ << delim_;
    return *this;
  }
  ostream_iterator& operator*() { return *this; }
  ostream_iterator& operator++() { return *this; }
  ostream_iterator& operator++(int) { return *this; }

private:
  basic_ostream<charT, traits>* out_stream_;
  const charT* delim_;
};

// [istreambuf.iterator]. An iterator found at end of stream drops its buffer pointer, so it
// becomes the end-of-stream value ([istreambuf.iterator.general]/1).
template <class charT, class traits>
class istreambuf_iterator {
public:
  using iterator_category = input_iterator_tag;
  using value_type = charT;
  using difference_type = typename traits::off_type;
  using reference = charT;
  using char_type = charT;
  using traits_type = traits;
  using int_type = typename traits::int_type;
  using streambuf_type = basic_streambuf<charT, traits>;
  using istream_type = basic_istream<charT, traits>;

  // [istreambuf.iterator.proxy]: the result of it++.
  class proxy {
    friend istreambuf_iterator;
    charT keep_;
    streambuf_type* sbuf_;
    proxy(charT c, streambuf_type* sbuf) : keep_(c), sbuf_(sbuf) {}

  public:
    charT operator*() { return keep_; }
  };
  using pointer = charT*;

  constexpr istreambuf_iterator() noexcept : sbuf_(nullptr) {}
  constexpr istreambuf_iterator(default_sentinel_t) noexcept : sbuf_(nullptr) {}
  istreambuf_iterator(const istreambuf_iterator&) noexcept = default;
  ~istreambuf_iterator() = default;
  istreambuf_iterator(istream_type& s) noexcept : sbuf_(s.rdbuf()) {}
  istreambuf_iterator(streambuf_type* s) noexcept : sbuf_(s) {}
  istreambuf_iterator(const proxy& p) noexcept : sbuf_(p.sbuf_) {}
  istreambuf_iterator& operator=(const istreambuf_iterator&) noexcept = default;

  charT operator*() const { return traits::to_char_type(sbuf_->sgetc()); }
  istreambuf_iterator& operator++() {
    sbuf_->sbumpc();
    return *this;
  }
  proxy operator++(int) { return proxy(traits::to_char_type(sbuf_->sbumpc()), sbuf_); }

  bool equal(const istreambuf_iterator& b) const { return at_end() == b.at_end(); }
  friend bool operator==(const istreambuf_iterator& i, default_sentinel_t) { return i.at_end(); }

private:
  friend ycxx::detail::streambuf_iter_access;
  bool at_end() const {
    if (sbuf_ && traits::eq_int_type(sbuf_->sgetc(), traits::eof()))
      sbuf_ = nullptr;
    return sbuf_ == nullptr;
  }
  mutable streambuf_type* sbuf_;
};

template <class charT, class traits>
bool operator==(const istreambuf_iterator<charT, traits>& a, const istreambuf_iterator<charT, traits>& b) {
  return a.equal(b);
}

// [ostreambuf.iterator]
template <class charT, class traits>
class ostreambuf_iterator {
public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using char_type = charT;
  using traits_type = traits;
  using streambuf_type = basic_streambuf<charT, traits>;
  using ostream_type = basic_ostream<charT, traits>;

  ostreambuf_iterator(ostream_type& s) noexcept : sbuf_(s.rdbuf()) {
    ycxx::detail::precondition(sbuf_ != nullptr, "std::ostreambuf_iterator: s.rdbuf() is null");
  }
  ostreambuf_iterator(streambuf_type* s) noexcept : sbuf_(s) {
    ycxx::detail::precondition(s != nullptr, "std::ostreambuf_iterator: null stream buffer");
  }
  ostreambuf_iterator& operator=(charT c) {
    if (!failed_ && traits::eq_int_type(sbuf_->sputc(c), traits::eof()))
      failed_ = true;
    return *this;
  }
  ostreambuf_iterator& operator*() { return *this; }
  ostreambuf_iterator& operator++() { return *this; }
  ostreambuf_iterator& operator++(int) { return *this; }
  bool failed() const noexcept { return failed_; }

private:
  friend ycxx::detail::streambuf_iter_access;
  streambuf_type* sbuf_;
  bool failed_ = false;
};

} // namespace std
