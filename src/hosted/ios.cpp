// libycxx hosted runtime: ios_base's members, ios_base::failure and iostream_category()
// ([ios.base], [ios.failure], [error.reporting]).
#include <ios>
#include <new>

namespace {

class iostream_error_category final : public std::error_category {
public:
  constexpr iostream_error_category() noexcept {}
  const char* name() const noexcept override { return "iostream"; }
  std::string message(int __ev) const override {
    if (__ev == static_cast<int>(std::io_errc::stream))
      return "iostream stream error";
    return "unknown iostream error";
  }
};

template <class _Tp>
union immortal {
  _Tp __object;
  constexpr immortal() noexcept : __object() {}
  ~immortal() {}
};
constinit immortal<iostream_error_category> iostream_object;

int next_index = 0; // [ios.base.storage]: index ++ (atomic)

} // namespace

// The iword/pword arrays and the registered callbacks, allocated on first use.
struct std::ios_base::__storage {
  long* iarray = nullptr;
  std::size_t isize = 0;
  void** parray = nullptr;
  std::size_t psize = 0;
  __y_callback* callbacks = nullptr;
  std::size_t ncallbacks = 0;
  std::size_t callback_cap = 0;

  ~__storage() {
    ::operator delete[](iarray, std::nothrow);
    ::operator delete[](parray, std::nothrow);
    ::operator delete[](callbacks, std::nothrow);
  }
};

namespace {

// Grows a zero-filled array of T to hold index idx; false if the memory is not available.
template <class _Tp>
bool __grow(_Tp*& array, std::size_t& size, std::size_t __idx) noexcept {
  if (__idx < size)
    return true;
  std::size_t n = size == 0 ? 8 : size;
  while (n <= __idx)
    n *= 2;
  _Tp* p = static_cast<_Tp*>(::operator new[](n * sizeof(_Tp), std::nothrow));
  if (p == nullptr)
    return false;
  for (std::size_t i = 0; i < n; ++i)
    p[i] = i < size ? array[i] : _Tp();
  ::operator delete[](array, std::nothrow);
  array = p;
  size = n;
  return true;
}

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] std {

const error_category& iostream_category() noexcept { return iostream_object.__object; }

ios_base::failure::failure(const string& __msg, const error_code& ec) : system_error(ec, __msg) {}
ios_base::failure::failure(const char* __msg, const error_code& ec) : system_error(ec, __msg) {}
ios_base::failure::~failure() {}

ios_base::~ios_base() {
  __call_callbacks(erase_event);
  delete __store_;
}

locale ios_base::imbue(const locale& __loc) {
  locale __old = __loc_;
  __loc_ = __loc;
  __call_callbacks(imbue_event);
  return __old;
}

int ios_base::xalloc() { return __atomic_fetch_add(&next_index, 1, __ATOMIC_RELAXED); }

long& ios_base::iword(int __idx) {
  static thread_local long dummy;
  if (__idx >= 0) {
    if (__store_ == nullptr)
      __store_ = new (nothrow) __storage;
    if (__store_ != nullptr && __grow(__store_->iarray, __store_->isize, static_cast<size_t>(__idx)))
      return __store_->iarray[__idx];
  }
  __storage_failed();
  dummy = 0;
  return dummy;
}

void*& ios_base::pword(int __idx) {
  static thread_local void* dummy;
  if (__idx >= 0) {
    if (__store_ == nullptr)
      __store_ = new (nothrow) __storage;
    if (__store_ != nullptr && __grow(__store_->parray, __store_->psize, static_cast<size_t>(__idx)))
      return __store_->parray[__idx];
  }
  __storage_failed();
  dummy = nullptr;
  return dummy;
}

void ios_base::register_callback(event_callback __fn, int __idx) {
  if (__store_ == nullptr)
    __store_ = new __storage;
  if (__store_->ncallbacks == __store_->callback_cap) {
    const size_t n = __store_->callback_cap == 0 ? 4 : __store_->callback_cap * 2;
    __y_callback* p = static_cast<__y_callback*>(::operator new[](n * sizeof(__y_callback)));
    for (size_t i = 0; i < __store_->ncallbacks; ++i)
      p[i] = __store_->callbacks[i];
    ::operator delete[](__store_->callbacks, nothrow);
    __store_->callbacks = p;
    __store_->callback_cap = n;
  }
  __store_->callbacks[__store_->ncallbacks++] = __y_callback{__fn, __idx};
}

void ios_base::__init_base(bool __has_buf) {
  __flags_ = skipws | dec;
  __state_ = __has_buf ? goodbit : badbit;
  __except_ = goodbit;
  __prec_ = 6;
  __width_ = 0;
  __loc_ = locale();
  delete __store_;
  __store_ = nullptr;
}

// [ios.base.callback]: in opposite order of registration; callbacks registered meanwhile wait
// for the next event.
void ios_base::__call_callbacks(event __ev) noexcept {
  if (__store_ == nullptr)
    return;
  for (size_t i = __store_->ncallbacks; i-- != 0;) {
    const __y_callback __cb = __store_->callbacks[i];
    __cb.__fn(__ev, *this, __cb.__idx);
    if (__store_ == nullptr)
      return;
  }
}

// [basic.ios.members]/16: everything but rdstate, exceptions and rdbuf (and tie and fill, which
// basic_ios copies). The arrays are copied first, so a failed allocation leaves *this as it was
// (and frees what the copy had allocated).
void ios_base::__copy_base(const ios_base& __rhs) {
  struct __owner {
    __storage* p = nullptr;
    ~__owner() { delete p; }
  } __guard;
  __storage*& copy = __guard.p;
  if (__rhs.__store_ != nullptr) {
    copy = new __storage;
    const __storage& s = *__rhs.__store_;
    if (s.isize != 0) {
      copy->iarray = static_cast<long*>(::operator new[](s.isize * sizeof(long)));
      copy->isize = s.isize;
      for (size_t i = 0; i < s.isize; ++i)
        copy->iarray[i] = s.iarray[i];
    }
    if (s.psize != 0) {
      copy->parray = static_cast<void**>(::operator new[](s.psize * sizeof(void*)));
      copy->psize = s.psize;
      for (size_t i = 0; i < s.psize; ++i)
        copy->parray[i] = s.parray[i];
    }
    if (s.ncallbacks != 0) {
      copy->callbacks = static_cast<__y_callback*>(::operator new[](s.ncallbacks * sizeof(__y_callback)));
      copy->ncallbacks = copy->callback_cap = s.ncallbacks;
      for (size_t i = 0; i < s.ncallbacks; ++i)
        copy->callbacks[i] = s.callbacks[i];
    }
  }
  __flags_ = __rhs.__flags_;
  __prec_ = __rhs.__prec_;
  __width_ = __rhs.__width_;
  __loc_ = __rhs.__loc_;
  delete __store_;
  __store_ = copy;
  copy = nullptr;
}

void ios_base::__move_base(ios_base& __rhs) noexcept {
  __flags_ = __rhs.__flags_;
  __state_ = __rhs.__state_;
  __except_ = __rhs.__except_;
  __prec_ = __rhs.__prec_;
  __width_ = __rhs.__width_;
  __loc_ = __rhs.__loc_;
  delete __store_;
  __store_ = __rhs.__store_;
  __rhs.__store_ = nullptr;
}

void ios_base::__swap_base(ios_base& __rhs) noexcept {
  const fmtflags __f = __flags_;
  __flags_ = __rhs.__flags_;
  __rhs.__flags_ = __f;
  const iostate s = __state_;
  __state_ = __rhs.__state_;
  __rhs.__state_ = s;
  const iostate e = __except_;
  __except_ = __rhs.__except_;
  __rhs.__except_ = e;
  const streamsize p = __prec_;
  __prec_ = __rhs.__prec_;
  __rhs.__prec_ = p;
  const streamsize __w = __width_;
  __width_ = __rhs.__width_;
  __rhs.__width_ = __w;
  locale __l = __loc_;
  __loc_ = __rhs.__loc_;
  __rhs.__loc_ = __l;
  __storage* __st = __store_;
  __store_ = __rhs.__store_;
  __rhs.__store_ = __st;
}

void ios_base::__storage_failed() {
  __add_state(badbit);
  if (__except_ & badbit)
    ::__ycxx::__detail::__raise_ios_failure("std::ios_base::iword/pword: cannot allocate the storage");
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

void __throw_ios_failure(const char* what) { throw std::ios_base::failure(what); }

}} // namespace __ycxx::__detail
