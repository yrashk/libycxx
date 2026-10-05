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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// src/hosted/syncstream.cpp: locks the lock of the stream buffer at `key` (blocking), and
// returns the handle that syncbuf_unlock releases.
void* syncbuf_lock(const void* key) noexcept;
void syncbuf_unlock(void* handle) noexcept;
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [syncstream.syncbuf]
template <class charT, class traits, class Allocator>
class basic_syncbuf : public ycxx::adl_free::syncbuf_base<charT, traits> {
  using base = ycxx::adl_free::syncbuf_base<charT, traits>;
  using string_type = basic_string<charT, traits, Allocator>;

public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;
  using allocator_type = Allocator;
  using streambuf_type = basic_streambuf<charT, traits>;

  // [syncstream.syncbuf.cons]
  basic_syncbuf() : basic_syncbuf(nullptr) {}
  explicit basic_syncbuf(streambuf_type* obuf) : basic_syncbuf(obuf, Allocator()) {}
  basic_syncbuf(streambuf_type* obuf, const Allocator& allocator) : wrapped_(obuf), buf_(allocator) {}
  basic_syncbuf(basic_syncbuf&& other)
      : base(other), wrapped_(other.wrapped_), sync_pending_(other.sync_pending_),
        buf_(static_cast<string_type&&>(other.take_output())) {
    this->setp(nullptr, nullptr);
    put_over(buf_.size());
    other.wrapped_ = nullptr;
    other.sync_pending_ = false;
  }
  ~basic_syncbuf() override {
    if constexpr (ycxx::detail::cfg::exceptions) {
      try {
        emit();
      } catch (...) {
      }
    } else {
      emit();
    }
  }

  // [syncstream.syncbuf.assign]
  basic_syncbuf& operator=(basic_syncbuf&& rhs) {
    emit();
    if (this == __builtin_addressof(rhs))
      return *this;
    base::operator=(rhs);
    wrapped_ = rhs.wrapped_;
    sync_pending_ = rhs.sync_pending_;
    this->setp(nullptr, nullptr);
    buf_ = rhs.take_output(); // allocator propagation as for basic_string
    put_over(buf_.size());
    rhs.wrapped_ = nullptr;
    rhs.sync_pending_ = false;
    return *this;
  }
  void swap(basic_syncbuf& other) {
    ycxx::detail::precondition(allocator_traits<Allocator>::propagate_on_container_swap::value ||
                                   get_allocator() == other.get_allocator(),
                               "std::basic_syncbuf::swap: unequal allocators that do not propagate");
    if (this == __builtin_addressof(other))
      return;
    string_type mine = take_output();
    string_type theirs = other.take_output();
    streambuf_type::swap(other); // the locales (both put areas are empty now)
    const bool e = this->emit_on_sync_;
    this->emit_on_sync_ = other.emit_on_sync_;
    other.emit_on_sync_ = e;
    buf_.swap(theirs); // allocator propagation as for basic_string
    other.buf_.swap(mine);
    put_over(buf_.size());
    other.put_over(other.buf_.size());
    streambuf_type* w = wrapped_;
    wrapped_ = other.wrapped_;
    other.wrapped_ = w;
    const bool s = sync_pending_;
    sync_pending_ = other.sync_pending_;
    other.sync_pending_ = s;
  }

  // [syncstream.syncbuf.members]
  bool emit() override {
    if (wrapped_ == nullptr)
      return false;
    const streamsize n = this->pptr() - this->pbase();
    bool ok = true;
    void* const lock = ::ycxx::detail::syncbuf_lock(wrapped_);
    struct unlock_at_exit {
      void* h;
      ~unlock_at_exit() { ::ycxx::detail::syncbuf_unlock(h); }
    } guard{lock};
    // the output is gone even if the transfer fails or throws (no duplicate on a later emit)
    struct clear_at_exit {
      basic_syncbuf& self;
      ~clear_at_exit() {
        if (self.pbase() != nullptr)
          self.setp(self.pbase(), self.epptr());
      }
    } clear{*this};
    if (n != 0 && wrapped_->sputn(this->pbase(), n) != n)
      ok = false;
    if (sync_pending_) {
      sync_pending_ = false;
      if (wrapped_->pubsync() == -1)
        ok = false;
    }
    return ok;
  }
  streambuf_type* get_wrapped() const noexcept { return wrapped_; }
  allocator_type get_allocator() const noexcept { return buf_.get_allocator(); }
  void set_emit_on_sync(bool b) noexcept { base::set_emit_on_sync(b); }

protected:
  // [syncstream.syncbuf.virtuals]
  int sync() override {
    sync_pending_ = true;
    if (this->emit_on_sync_ && !emit())
      return -1;
    return 0;
  }
  int_type overflow(int_type c = traits::eof()) override {
    if (traits::eq_int_type(c, traits::eof()))
      return traits::not_eof(c);
    const size_t used = static_cast<size_t>(this->pptr() - this->pbase());
    if (this->pptr() == this->epptr()) {
      // the string grows (its size is the put area); used characters are kept
      const size_t cap = buf_.size() < 64 ? 128 : buf_.size() * 2;
      buf_.resize(cap > buf_.max_size() ? buf_.max_size() : cap);
      if (buf_.size() == used)
        return traits::eof();
      put_over(used);
    }
    *this->pptr() = traits::to_char_type(c);
    this->pbump(1);
    return c;
  }
  streamsize xsputn(const char_type* s, streamsize n) override {
    if (n <= 0)
      return 0;
    const size_t used = static_cast<size_t>(this->pptr() - this->pbase());
    const size_t room = static_cast<size_t>(this->epptr() - this->pptr());
    if (static_cast<size_t>(n) > room) {
      size_t cap = buf_.size() < 64 ? 128 : buf_.size() * 2;
      if (cap < used + static_cast<size_t>(n))
        cap = used + static_cast<size_t>(n);
      buf_.resize(cap);
      put_over(used);
    }
    traits::copy(this->pptr(), s, static_cast<size_t>(n));
    put_over(used + static_cast<size_t>(n));
    return n;
  }

private:
  // The put area: all of buf_, with the first `used` characters written.
  void put_over(size_t used) noexcept {
    if (buf_.empty()) {
      this->setp(nullptr, nullptr);
      return;
    }
    charT* const p = buf_.data();
    this->setp(p, p + buf_.size());
    while (used > 0) {
      const int k = used > static_cast<size_t>(__INT_MAX__) ? __INT_MAX__ : static_cast<int>(used);
      this->pbump(k);
      used -= static_cast<size_t>(k);
    }
  }
  // Moves the associated output out (as a string of exactly those characters) and leaves this
  // syncbuf with none.
  string_type take_output() noexcept {
    const size_t used = static_cast<size_t>(this->pptr() - this->pbase());
    buf_.resize(used); // shrinking: no allocation
    this->setp(nullptr, nullptr);
    string_type r(static_cast<string_type&&>(buf_));
    buf_.clear();
    return r;
  }

  streambuf_type* wrapped_;
  bool sync_pending_ = false; // a sync() since the last emit()
  string_type buf_;
};

template <class charT, class traits, class Allocator>
void swap(basic_syncbuf<charT, traits, Allocator>& a, basic_syncbuf<charT, traits, Allocator>& b) {
  a.swap(b);
}

// [syncstream.osyncstream]
template <class charT, class traits, class Allocator>
class basic_osyncstream : public basic_ostream<charT, traits> {
public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;
  using allocator_type = Allocator;
  using streambuf_type = basic_streambuf<charT, traits>;
  using syncbuf_type = basic_syncbuf<charT, traits, Allocator>;

  // [syncstream.osyncstream.cons]
  basic_osyncstream(streambuf_type* buf, const Allocator& allocator)
      : basic_ostream<charT, traits>(__builtin_addressof(sb_)), sb_(buf, allocator) {}
  explicit basic_osyncstream(streambuf_type* obuf) : basic_osyncstream(obuf, Allocator()) {}
  basic_osyncstream(basic_ostream<charT, traits>& os, const Allocator& allocator)
      : basic_osyncstream(os.rdbuf(), allocator) {}
  explicit basic_osyncstream(basic_ostream<charT, traits>& os) : basic_osyncstream(os, Allocator()) {}
  basic_osyncstream(basic_osyncstream&& other) noexcept
      : basic_ostream<charT, traits>(static_cast<basic_ostream<charT, traits>&&>(other)),
        sb_(static_cast<syncbuf_type&&>(other.sb_)) {
    basic_ostream<charT, traits>::set_rdbuf(__builtin_addressof(sb_));
  }
  ~basic_osyncstream() override {}

  basic_osyncstream& operator=(basic_osyncstream&& rhs) {
    basic_ostream<charT, traits>::operator=(static_cast<basic_ostream<charT, traits>&&>(rhs));
    sb_ = static_cast<syncbuf_type&&>(rhs.sb_);
    return *this;
  }

  // [syncstream.osyncstream.members]
  void emit() {
    ios_base::iostate err = ios_base::goodbit;
    if (typename basic_ostream<charT, traits>::sentry ok{*this}) {
      ycxx::detail::guarded_io(*this, [&] {
        if (!sb_.emit())
          err |= ios_base::badbit;
      });
    }
    if (err)
      this->setstate(err);
  }
  streambuf_type* get_wrapped() const noexcept { return sb_.get_wrapped(); }
  syncbuf_type* rdbuf() const noexcept { return const_cast<syncbuf_type*>(__builtin_addressof(sb_)); }

private:
  syncbuf_type sb_;
};

} // namespace std
