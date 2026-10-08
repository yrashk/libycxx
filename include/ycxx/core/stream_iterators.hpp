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

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// The library's extraction and insertion loops (num_get, money_get, ...) reach the buffer behind
// a stream-buffer iterator through this class, to work on whole runs of characters.
struct __streambuf_iter_access;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [istream.iterator]
template <class _Tp, class __charT = char, class __traits = char_traits<__charT>, class _Distance = ptrdiff_t>
class istream_iterator {
public:
  using iterator_category = input_iterator_tag;
  using value_type = _Tp;
  using difference_type = _Distance;
  using pointer = const _Tp*;
  using reference = const _Tp&;
  using char_type = __charT;
  using traits_type = __traits;
  using istream_type = basic_istream<__charT, __traits>;

  constexpr istream_iterator() : __in_stream_(nullptr), __value_() {}
  constexpr istream_iterator(default_sentinel_t) : __in_stream_(nullptr), __value_() {}
  istream_iterator(istream_type& s) : __in_stream_(__builtin_addressof(s)), __value_() { ++*this; }
  constexpr istream_iterator(const istream_iterator& __x) = default;
  ~istream_iterator() = default;
  istream_iterator& operator=(const istream_iterator&) = default;

  const _Tp& operator*() const {
    __ycxx::__detail::__precondition(__in_stream_ != nullptr, "std::istream_iterator::operator*: end-of-stream iterator");
    return __value_;
  }
  const _Tp* operator->() const {
    __ycxx::__detail::__precondition(__in_stream_ != nullptr, "std::istream_iterator::operator->: end-of-stream iterator");
    return __builtin_addressof(__value_);
  }
  istream_iterator& operator++() {
    __ycxx::__detail::__precondition(__in_stream_ != nullptr, "std::istream_iterator::operator++: end-of-stream iterator");
    if (!(*__in_stream_ >> __value_))
      __in_stream_ = nullptr;
    return *this;
  }
  istream_iterator operator++(int) {
    istream_iterator __tmp = *this;
    ++*this;
    return __tmp;
  }

  friend bool operator==(const istream_iterator& i, default_sentinel_t) { return !i.__in_stream_; }

private:
  template <class _T2, class __charT2, class __traits2, class _Distance2>
  friend bool operator==(const istream_iterator<_T2, __charT2, __traits2, _Distance2>& __x,
                         const istream_iterator<_T2, __charT2, __traits2, _Distance2>& y);

  basic_istream<__charT, __traits>* __in_stream_;
  _Tp __value_;
};

// [iterator.synopsis] declares the comparison of two iterators as a namespace-scope template
// (only the default_sentinel_t comparison is a hidden friend), so std::operator== names it.
template <class _Tp, class __charT, class __traits, class _Distance>
bool operator==(const istream_iterator<_Tp, __charT, __traits, _Distance>& __x,
                const istream_iterator<_Tp, __charT, __traits, _Distance>& y) {
  return __x.__in_stream_ == y.__in_stream_;
}

// [ostream.iterator]
template <class _Tp, class __charT = char, class __traits = char_traits<__charT>>
class ostream_iterator {
public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using char_type = __charT;
  using traits_type = __traits;
  using ostream_type = basic_ostream<__charT, __traits>;

  ostream_iterator(ostream_type& s) : __out_stream_(__builtin_addressof(s)), __delim_(nullptr) {}
  ostream_iterator(ostream_type& s, const __charT* __delimiter) : __out_stream_(__builtin_addressof(s)), __delim_(__delimiter) {}
  ostream_iterator(const ostream_iterator& __x) = default;
  ~ostream_iterator() = default;
  ostream_iterator& operator=(const ostream_iterator&) = default;

  ostream_iterator& operator=(const _Tp& value) {
    *__out_stream_ << value;
    if (__delim_)
      *__out_stream_ << __delim_;
    return *this;
  }
  ostream_iterator& operator*() { return *this; }
  ostream_iterator& operator++() { return *this; }
  ostream_iterator& operator++(int) { return *this; }

private:
  basic_ostream<__charT, __traits>* __out_stream_;
  const __charT* __delim_;
};

// [istreambuf.iterator]. An iterator found at end of stream drops its buffer pointer, so it
// becomes the end-of-stream value ([istreambuf.iterator.general]/1).
template <class __charT, class __traits>
class istreambuf_iterator {
public:
  using iterator_category = input_iterator_tag;
  using value_type = __charT;
  using difference_type = typename __traits::off_type;
  using reference = __charT;
  using char_type = __charT;
  using traits_type = __traits;
  using int_type = typename __traits::int_type;
  using streambuf_type = basic_streambuf<__charT, __traits>;
  using istream_type = basic_istream<__charT, __traits>;

  // [istreambuf.iterator.proxy]: the result of it++.
  class proxy {
    friend istreambuf_iterator;
    __charT __keep_;
    streambuf_type* __sbuf_;
    proxy(__charT c, streambuf_type* __sbuf) : __keep_(c), __sbuf_(__sbuf) {}

  public:
    __charT operator*() { return __keep_; }
  };
  using pointer = __charT*;

  constexpr istreambuf_iterator() noexcept : __sbuf_(nullptr) {}
  constexpr istreambuf_iterator(default_sentinel_t) noexcept : __sbuf_(nullptr) {}
  istreambuf_iterator(const istreambuf_iterator&) noexcept = default;
  ~istreambuf_iterator() = default;
  istreambuf_iterator(istream_type& s) noexcept : __sbuf_(s.rdbuf()) {}
  istreambuf_iterator(streambuf_type* s) noexcept : __sbuf_(s) {}
  istreambuf_iterator(const proxy& p) noexcept : __sbuf_(p.__sbuf_) {}
  istreambuf_iterator& operator=(const istreambuf_iterator&) noexcept = default;

  __charT operator*() const { return __traits::to_char_type(__sbuf_->sgetc()); }
  istreambuf_iterator& operator++() {
    __sbuf_->sbumpc();
    return *this;
  }
  proxy operator++(int) { return proxy(__traits::to_char_type(__sbuf_->sbumpc()), __sbuf_); }

  bool equal(const istreambuf_iterator& b) const { return __at_end() == b.__at_end(); }
  friend bool operator==(const istreambuf_iterator& i, default_sentinel_t) { return i.__at_end(); }

private:
  friend __ycxx::__detail::__streambuf_iter_access;
  bool __at_end() const {
    if (__sbuf_ && __traits::eq_int_type(__sbuf_->sgetc(), __traits::eof()))
      __sbuf_ = nullptr;
    return __sbuf_ == nullptr;
  }
  mutable streambuf_type* __sbuf_;
};

template <class __charT, class __traits>
bool operator==(const istreambuf_iterator<__charT, __traits>& a, const istreambuf_iterator<__charT, __traits>& b) {
  return a.equal(b);
}

// [ostreambuf.iterator]
template <class __charT, class __traits>
class ostreambuf_iterator {
public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using char_type = __charT;
  using traits_type = __traits;
  using streambuf_type = basic_streambuf<__charT, __traits>;
  using ostream_type = basic_ostream<__charT, __traits>;

  ostreambuf_iterator(ostream_type& s) noexcept : __sbuf_(s.rdbuf()) {
    __ycxx::__detail::__precondition(__sbuf_ != nullptr, "std::ostreambuf_iterator: s.rdbuf() is null");
  }
  ostreambuf_iterator(streambuf_type* s) noexcept : __sbuf_(s) {
    __ycxx::__detail::__precondition(s != nullptr, "std::ostreambuf_iterator: null stream buffer");
  }
  ostreambuf_iterator& operator=(__charT c) {
    if (!__failed_ && __traits::eq_int_type(__sbuf_->sputc(c), __traits::eof()))
      __failed_ = true;
    return *this;
  }
  ostreambuf_iterator& operator*() { return *this; }
  ostreambuf_iterator& operator++() { return *this; }
  ostreambuf_iterator& operator++(int) { return *this; }
  bool failed() const noexcept { return __failed_; }

private:
  friend __ycxx::__detail::__streambuf_iter_access;
  streambuf_type* __sbuf_;
  bool __failed_ = false;
};

}} // namespace std
