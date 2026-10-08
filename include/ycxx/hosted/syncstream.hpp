// libycxx hosted: basic_syncbuf and basic_osyncstream ([syncstream]).
//
// The associated output is kept in a basic_string with the syncbuf's allocator, used as the put
// area. emit() transfers it while holding a lock that belongs to the wrapped stream buffer
// alone: the hosted runtime (src/hosted/syncstream.cpp) keeps one lock per stream buffer that is
// being emitted to, found by its address, so emits to different buffers never wait for each
// other, and an emit whose wrapped buffer is itself a syncbuf can emit onward without deadlock.
// The locks are built on the PAL's wait/wake (no <mutex> dependency).
#pragma once

#include <ycxx/core/basic_string.hpp>
#include <ycxx/hosted/ostream.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// src/hosted/syncstream.cpp: locks the lock of the stream buffer at `key` (blocking), and
// returns the handle that syncbuf_unlock releases.
void* __syncbuf_lock(const void* key) noexcept;
void __syncbuf_unlock(void* handle) noexcept;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [syncstream.syncbuf]
template <class __charT, class __traits, class _Allocator>
class basic_syncbuf : public __ycxx::__adl_free::__syncbuf_base<__charT, __traits> {
  using base = __ycxx::__adl_free::__syncbuf_base<__charT, __traits>;
  using string_type = basic_string<__charT, __traits, _Allocator>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;
  using allocator_type = _Allocator;
  using streambuf_type = basic_streambuf<__charT, __traits>;

  // [syncstream.syncbuf.cons]
  basic_syncbuf() : basic_syncbuf(nullptr) {}
  explicit basic_syncbuf(streambuf_type* __obuf) : basic_syncbuf(__obuf, _Allocator()) {}
  basic_syncbuf(streambuf_type* __obuf, const _Allocator& allocator) : __wrapped_(__obuf), __buf_(allocator) {}
  basic_syncbuf(basic_syncbuf&& other)
      : base(other), __wrapped_(other.__wrapped_), __sync_pending_(other.__sync_pending_),
        __buf_(static_cast<string_type&&>(other.__take_output())) {
    this->setp(nullptr, nullptr);
    __put_over(__buf_.size());
    other.__wrapped_ = nullptr;
    other.__sync_pending_ = false;
  }
  ~basic_syncbuf() override {
    if constexpr (__ycxx::__detail::__cfg::exceptions) {
      try {
        emit();
      } catch (...) {
      }
    } else {
      emit();
    }
  }

  // [syncstream.syncbuf.assign]
  basic_syncbuf& operator=(basic_syncbuf&& __rhs) {
    emit();
    if (this == __builtin_addressof(__rhs))
      return *this;
    base::operator=(__rhs);
    __wrapped_ = __rhs.__wrapped_;
    __sync_pending_ = __rhs.__sync_pending_;
    this->setp(nullptr, nullptr);
    __buf_ = __rhs.__take_output(); // allocator propagation as for basic_string
    __put_over(__buf_.size());
    __rhs.__wrapped_ = nullptr;
    __rhs.__sync_pending_ = false;
    return *this;
  }
  void swap(basic_syncbuf& other) {
    __ycxx::__detail::__precondition(allocator_traits<_Allocator>::propagate_on_container_swap::value ||
                                   get_allocator() == other.get_allocator(),
                               "std::basic_syncbuf::swap: unequal allocators that do not propagate");
    if (this == __builtin_addressof(other))
      return;
    string_type __mine = __take_output();
    string_type __theirs = other.__take_output();
    streambuf_type::swap(other); // the locales (both put areas are empty now)
    const bool e = this->__emit_on_sync_;
    this->__emit_on_sync_ = other.__emit_on_sync_;
    other.__emit_on_sync_ = e;
    __buf_.swap(__theirs); // allocator propagation as for basic_string
    other.__buf_.swap(__mine);
    __put_over(__buf_.size());
    other.__put_over(other.__buf_.size());
    streambuf_type* __w = __wrapped_;
    __wrapped_ = other.__wrapped_;
    other.__wrapped_ = __w;
    const bool s = __sync_pending_;
    __sync_pending_ = other.__sync_pending_;
    other.__sync_pending_ = s;
  }

  // [syncstream.syncbuf.members]
  bool emit() override {
    if (__wrapped_ == nullptr)
      return false;
    const streamsize n = this->pptr() - this->pbase();
    bool ok = true;
    void* const lock = ::__ycxx::__detail::__syncbuf_lock(__wrapped_);
    struct __unlock_at_exit {
      void* h;
      ~__unlock_at_exit() { ::__ycxx::__detail::__syncbuf_unlock(h); }
    } __guard{lock};
    // the output is gone even if the transfer fails or throws (no duplicate on a later emit)
    struct __clear_at_exit {
      basic_syncbuf& __self;
      ~__clear_at_exit() {
        if (__self.pbase() != nullptr)
          __self.setp(__self.pbase(), __self.epptr());
      }
    } clear{*this};
    if (n != 0 && __wrapped_->sputn(this->pbase(), n) != n)
      ok = false;
    if (__sync_pending_) {
      __sync_pending_ = false;
      if (__wrapped_->pubsync() == -1)
        ok = false;
    }
    return ok;
  }
  streambuf_type* get_wrapped() const noexcept { return __wrapped_; }
  allocator_type get_allocator() const noexcept { return __buf_.get_allocator(); }
  void set_emit_on_sync(bool b) noexcept { base::set_emit_on_sync(b); }

protected:
  // [syncstream.syncbuf.virtuals]
  int sync() override {
    __sync_pending_ = true;
    if (this->__emit_on_sync_ && !emit())
      return -1;
    return 0;
  }
  int_type overflow(int_type c = __traits::eof()) override {
    if (__traits::eq_int_type(c, __traits::eof()))
      return __traits::not_eof(c);
    const size_t __y_used = static_cast<size_t>(this->pptr() - this->pbase());
    if (this->pptr() == this->epptr()) {
      // the string grows (its size is the put area); used characters are kept
      const size_t __cap = __buf_.size() < 64 ? 128 : __buf_.size() * 2;
      __buf_.resize(__cap > __buf_.max_size() ? __buf_.max_size() : __cap);
      if (__buf_.size() == __y_used)
        return __traits::eof();
      __put_over(__y_used);
    }
    *this->pptr() = __traits::to_char_type(c);
    this->pbump(1);
    return c;
  }
  streamsize xsputn(const char_type* s, streamsize n) override {
    if (n <= 0)
      return 0;
    const size_t __y_used = static_cast<size_t>(this->pptr() - this->pbase());
    const size_t __room = static_cast<size_t>(this->epptr() - this->pptr());
    if (static_cast<size_t>(n) > __room) {
      size_t __cap = __buf_.size() < 64 ? 128 : __buf_.size() * 2;
      if (__cap < __y_used + static_cast<size_t>(n))
        __cap = __y_used + static_cast<size_t>(n);
      __buf_.resize(__cap);
      __put_over(__y_used);
    }
    __traits::copy(this->pptr(), s, static_cast<size_t>(n));
    __put_over(__y_used + static_cast<size_t>(n));
    return n;
  }

private:
  // The put area: all of buf_, with the first `__y_used` characters written.
  void __put_over(size_t __y_used) noexcept {
    if (__buf_.empty()) {
      this->setp(nullptr, nullptr);
      return;
    }
    __charT* const p = __buf_.data();
    this->setp(p, p + __buf_.size());
    while (__y_used > 0) {
      const int k = __y_used > static_cast<size_t>(__INT_MAX__) ? __INT_MAX__ : static_cast<int>(__y_used);
      this->pbump(k);
      __y_used -= static_cast<size_t>(k);
    }
  }
  // Moves the associated output out (as a string of exactly those characters) and leaves this
  // syncbuf with none.
  string_type __take_output() noexcept {
    const size_t __y_used = static_cast<size_t>(this->pptr() - this->pbase());
    __buf_.resize(__y_used); // shrinking: no allocation
    this->setp(nullptr, nullptr);
    string_type r(static_cast<string_type&&>(__buf_));
    __buf_.clear();
    return r;
  }

  streambuf_type* __wrapped_;
  bool __sync_pending_ = false; // a sync() since the last emit()
  string_type __buf_;
};

template <class __charT, class __traits, class _Allocator>
void swap(basic_syncbuf<__charT, __traits, _Allocator>& a, basic_syncbuf<__charT, __traits, _Allocator>& b) {
  a.swap(b);
}

// [syncstream.osyncstream]
template <class __charT, class __traits, class _Allocator>
class basic_osyncstream : public basic_ostream<__charT, __traits> {
public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;
  using allocator_type = _Allocator;
  using streambuf_type = basic_streambuf<__charT, __traits>;
  using syncbuf_type = basic_syncbuf<__charT, __traits, _Allocator>;

  // [syncstream.osyncstream.cons]
  basic_osyncstream(streambuf_type* __buf, const _Allocator& allocator)
      : basic_ostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(__buf, allocator) {}
  explicit basic_osyncstream(streambuf_type* __obuf) : basic_osyncstream(__obuf, _Allocator()) {}
  basic_osyncstream(basic_ostream<__charT, __traits>& __os, const _Allocator& allocator)
      : basic_osyncstream(__os.rdbuf(), allocator) {}
  explicit basic_osyncstream(basic_ostream<__charT, __traits>& __os) : basic_osyncstream(__os, _Allocator()) {}
  basic_osyncstream(basic_osyncstream&& other) noexcept
      : basic_ostream<__charT, __traits>(static_cast<basic_ostream<__charT, __traits>&&>(other)),
        __sb_(static_cast<syncbuf_type&&>(other.__sb_)) {
    basic_ostream<__charT, __traits>::set_rdbuf(__builtin_addressof(__sb_));
  }
  ~basic_osyncstream() override {}

  basic_osyncstream& operator=(basic_osyncstream&& __rhs) {
    basic_ostream<__charT, __traits>::operator=(static_cast<basic_ostream<__charT, __traits>&&>(__rhs));
    __sb_ = static_cast<syncbuf_type&&>(__rhs.__sb_);
    return *this;
  }

  // [syncstream.osyncstream.members]
  void emit() {
    ios_base::iostate __err = ios_base::goodbit;
    if (typename basic_ostream<__charT, __traits>::sentry ok{*this}) {
      __ycxx::__detail::__guarded_io(*this, [&] {
        if (!__sb_.emit())
          __err |= ios_base::badbit;
      });
    }
    if (__err)
      this->setstate(__err);
  }
  streambuf_type* get_wrapped() const noexcept { return __sb_.get_wrapped(); }
  syncbuf_type* rdbuf() const noexcept { return const_cast<syncbuf_type*>(__builtin_addressof(__sb_)); }

private:
  syncbuf_type __sb_;
};

}} // namespace std
