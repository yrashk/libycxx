// libycxx hosted runtime: ios_base's members, ios_base::failure and iostream_category()
// ([ios.base], [ios.failure], [error.reporting]).
#include <ios>
#include <new>

namespace {

class iostream_error_category final : public std::error_category {
public:
  constexpr iostream_error_category() noexcept {}
  const char* name() const noexcept override { return "iostream"; }
  std::string message(int ev) const override {
    if (ev == static_cast<int>(std::io_errc::stream))
      return "iostream stream error";
    return "unknown iostream error";
  }
};

template <class T>
union immortal {
  T object;
  constexpr immortal() noexcept : object() {}
  ~immortal() {}
};
constinit immortal<iostream_error_category> iostream_object;

int next_index = 0; // [ios.base.storage]: index ++ (atomic)

} // namespace

// The iword/pword arrays and the registered callbacks, allocated on first use.
struct std::ios_base::storage {
  long* iarray = nullptr;
  std::size_t isize = 0;
  void** parray = nullptr;
  std::size_t psize = 0;
  callback* callbacks = nullptr;
  std::size_t ncallbacks = 0;
  std::size_t callback_cap = 0;

  ~storage() {
    ::operator delete[](iarray, std::nothrow);
    ::operator delete[](parray, std::nothrow);
    ::operator delete[](callbacks, std::nothrow);
  }
};

namespace {

// Grows a zero-filled array of T to hold index idx; false if the memory is not available.
template <class T>
bool grow(T*& array, std::size_t& size, std::size_t idx) noexcept {
  if (idx < size)
    return true;
  std::size_t n = size == 0 ? 8 : size;
  while (n <= idx)
    n *= 2;
  T* p = static_cast<T*>(::operator new[](n * sizeof(T), std::nothrow));
  if (p == nullptr)
    return false;
  for (std::size_t i = 0; i < n; ++i)
    p[i] = i < size ? array[i] : T();
  ::operator delete[](array, std::nothrow);
  array = p;
  size = n;
  return true;
}

} // namespace

namespace std {

const error_category& iostream_category() noexcept { return iostream_object.object; }

ios_base::failure::failure(const string& msg, const error_code& ec) : system_error(ec, msg) {}
ios_base::failure::failure(const char* msg, const error_code& ec) : system_error(ec, msg) {}
ios_base::failure::~failure() {}

ios_base::~ios_base() {
  call_callbacks(erase_event);
  delete store_;
}

locale ios_base::imbue(const locale& loc) {
  locale old = loc_;
  loc_ = loc;
  call_callbacks(imbue_event);
  return old;
}

int ios_base::xalloc() { return __atomic_fetch_add(&next_index, 1, __ATOMIC_RELAXED); }

long& ios_base::iword(int idx) {
  static thread_local long dummy;
  if (idx >= 0) {
    if (store_ == nullptr)
      store_ = new (nothrow) storage;
    if (store_ != nullptr && grow(store_->iarray, store_->isize, static_cast<size_t>(idx)))
      return store_->iarray[idx];
  }
  storage_failed();
  dummy = 0;
  return dummy;
}

void*& ios_base::pword(int idx) {
  static thread_local void* dummy;
  if (idx >= 0) {
    if (store_ == nullptr)
      store_ = new (nothrow) storage;
    if (store_ != nullptr && grow(store_->parray, store_->psize, static_cast<size_t>(idx)))
      return store_->parray[idx];
  }
  storage_failed();
  dummy = nullptr;
  return dummy;
}

void ios_base::register_callback(event_callback fn, int idx) {
  if (store_ == nullptr)
    store_ = new storage;
  if (store_->ncallbacks == store_->callback_cap) {
    const size_t n = store_->callback_cap == 0 ? 4 : store_->callback_cap * 2;
    callback* p = static_cast<callback*>(::operator new[](n * sizeof(callback)));
    for (size_t i = 0; i < store_->ncallbacks; ++i)
      p[i] = store_->callbacks[i];
    ::operator delete[](store_->callbacks, nothrow);
    store_->callbacks = p;
    store_->callback_cap = n;
  }
  store_->callbacks[store_->ncallbacks++] = callback{fn, idx};
}

void ios_base::init_base(bool has_buf) {
  flags_ = skipws | dec;
  state_ = has_buf ? goodbit : badbit;
  except_ = goodbit;
  prec_ = 6;
  width_ = 0;
  loc_ = locale();
  delete store_;
  store_ = nullptr;
}

// [ios.base.callback]: in opposite order of registration; callbacks registered meanwhile wait
// for the next event.
void ios_base::call_callbacks(event ev) noexcept {
  if (store_ == nullptr)
    return;
  for (size_t i = store_->ncallbacks; i-- != 0;) {
    const callback cb = store_->callbacks[i];
    cb.fn(ev, *this, cb.idx);
    if (store_ == nullptr)
      return;
  }
}

// [basic.ios.members]/16: everything but rdstate, exceptions and rdbuf (and tie and fill, which
// basic_ios copies). The arrays are copied first, so a failed allocation leaves *this as it was.
void ios_base::copy_base(const ios_base& rhs) {
  storage* copy = nullptr;
  if (rhs.store_ != nullptr) {
    copy = new storage;
    const storage& s = *rhs.store_;
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
      copy->callbacks = static_cast<callback*>(::operator new[](s.ncallbacks * sizeof(callback)));
      copy->ncallbacks = copy->callback_cap = s.ncallbacks;
      for (size_t i = 0; i < s.ncallbacks; ++i)
        copy->callbacks[i] = s.callbacks[i];
    }
  }
  flags_ = rhs.flags_;
  prec_ = rhs.prec_;
  width_ = rhs.width_;
  loc_ = rhs.loc_;
  delete store_;
  store_ = copy;
}

void ios_base::move_base(ios_base& rhs) noexcept {
  flags_ = rhs.flags_;
  state_ = rhs.state_;
  except_ = rhs.except_;
  prec_ = rhs.prec_;
  width_ = rhs.width_;
  loc_ = rhs.loc_;
  delete store_;
  store_ = rhs.store_;
  rhs.store_ = nullptr;
}

void ios_base::swap_base(ios_base& rhs) noexcept {
  const fmtflags f = flags_;
  flags_ = rhs.flags_;
  rhs.flags_ = f;
  const iostate s = state_;
  state_ = rhs.state_;
  rhs.state_ = s;
  const iostate e = except_;
  except_ = rhs.except_;
  rhs.except_ = e;
  const streamsize p = prec_;
  prec_ = rhs.prec_;
  rhs.prec_ = p;
  const streamsize w = width_;
  width_ = rhs.width_;
  rhs.width_ = w;
  locale l = loc_;
  loc_ = rhs.loc_;
  rhs.loc_ = l;
  storage* st = store_;
  store_ = rhs.store_;
  rhs.store_ = st;
}

void ios_base::storage_failed() {
  state_ |= badbit;
  if (except_ & badbit)
    ::ycxx::detail::raise_ios_failure("std::ios_base::iword/pword: cannot allocate the storage");
}

} // namespace std

namespace ycxx::detail {

void throw_ios_failure(const char* what) { throw std::ios_base::failure(what); }

} // namespace ycxx::detail
