// libycxx hosted runtime: <memory_resource> ([mem.res]): memory_resource's key function, the
// global resources and the default resource pointer ([mem.res.global]), the pool resources
// ([mem.res.pool]) and monotonic_buffer_resource ([mem.res.monotonic.buffer]). The data
// structures are described in ycxx/hosted/memory_resource.hpp.
#include <memory_resource>
#include <new>
#include <ycxx/core/bit.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/single_threaded.hpp>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// Lives at the start of a free pool block.
struct __pool_free_block {
  __pool_free_block* next;
};
// Lives at the end of a chunk (pools) or buffer (monotonic_buffer_resource) obtained from
// upstream; `bytes` and `align` are the arguments of that upstream allocate call.
struct __pool_chunk_footer {
  __pool_chunk_footer* next;
  std::size_t bytes;
  std::size_t align;
};
// Lives at the end of an allocation pool_core passed straight to upstream.
// `bytes` and `align` are the arguments of the upstream allocate call, as for chunks.
struct __pool_big_footer {
  __pool_big_footer* prev;
  __pool_big_footer* next;
  std::size_t bytes;
  std::size_t align;
};

namespace {

constexpr std::size_t size_max = static_cast<std::size_t>(-1);

// A request too large to add the resource's bookkeeping to. do_allocate of the pool and
// monotonic resources "Throws: nothing unless upstream_resource()->allocate() throws"
// ([mem.res.pool.mem], [mem.res.monotonic.buffer.mem]): ask upstream for size_max bytes, which it
// cannot provide, so that its exception is the one thrown.
[[noreturn]] void request_impossible(std::pmr::memory_resource& __upstream, std::size_t align) {
  void* p = __upstream.allocate(size_max, align);
  __upstream.deallocate(p, size_max, align); // an upstream that claims success: still no room
  __ycxx::__detail::__throw_bad_alloc();
}

constexpr std::size_t round_up(std::size_t n, std::size_t a) noexcept { return (n + a - 1) & ~(a - 1); }

// The largest chunk a pool asks for, in bytes (a pool of large blocks still gets at least one
// block per chunk).
constexpr std::size_t max_chunk_bytes = std::size_t(1) << 20;
// The first chunk of a pool holds about this many bytes.
constexpr std::size_t first_chunk_bytes = 1024;

// Where an upstream allocation of `bytes` usable bytes keeps its footer: just past the bytes,
// rounded up to the footer's alignment. Also the total size to request, minus sizeof(Footer).
template <class Footer>
constexpr std::size_t footer_offset(std::size_t bytes) noexcept {
  return round_up(bytes, alignof(Footer));
}

void free_chunks(__pool_chunk_footer* c, std::pmr::memory_resource* __upstream) noexcept {
  while (c != nullptr) {
    __pool_chunk_footer* next = c->next;
    const std::size_t bytes = c->bytes, align = c->align;
    void* base = reinterpret_cast<char*>(c) + sizeof(__pool_chunk_footer) - bytes;
    __upstream->deallocate(base, bytes, align);
    c = next;
  }
}

} // namespace

// ---- pool_core -----------------------------------------------------------------------------

__pool_core::__pool_core(const std::pmr::pool_options& __opts, std::pmr::memory_resource* __upstream) noexcept
    : __upstream_(__upstream), __opts_(__opts) {
  __ycxx::__detail::__precondition(__upstream != nullptr, "pool resource: null upstream resource");
  if (__opts_.max_blocks_per_chunk == 0 || __opts_.max_blocks_per_chunk > __max_blocks_limit)
    __opts_.max_blocks_per_chunk = __max_blocks_limit;
  std::size_t largest = __opts_.largest_required_pool_block;
  if (largest == 0 || largest > __largest_block_limit)
    largest = __largest_block_limit;
  if (largest < (std::size_t(1) << __min_shift))
    largest = std::size_t(1) << __min_shift;
  largest = std::bit_ceil(largest);
  __opts_.largest_required_pool_block = largest;
  __bins_ = static_cast<unsigned>(std::countr_zero(largest)) - __min_shift + 1;
}

void* __pool_core::__refill(__pool_bin& __bin, std::size_t block) {
  if (__bin.__next_blocks == 0) {
    std::size_t n = first_chunk_bytes / block;
    __bin.__next_blocks = n == 0 ? 1 : n;
  }
  std::size_t __limit = max_chunk_bytes / block;
  if (__limit == 0)
    __limit = 1;
  if (__limit > __opts_.max_blocks_per_chunk)
    __limit = __opts_.max_blocks_per_chunk;
  const std::size_t n = __bin.__next_blocks < __limit ? __bin.__next_blocks : __limit;
  // Blocks first, footer after them; block >= alignof(footer), so the footer is aligned.
  const std::size_t bytes = n * block + sizeof(__pool_chunk_footer);
  char* base = static_cast<char*>(__upstream_->allocate(bytes, block));
  auto* footer = ::new (static_cast<void*>(base + n * block)) __pool_chunk_footer{__bin.__chunks, bytes, block};
  __bin.__chunks = footer;
  __bin.cur = base + block;
  __bin.end = base + n * block;
  __bin.__next_blocks = n < __limit ? n * 2 : __limit;
  return base;
}

void* __pool_core::allocate(std::size_t bytes, std::size_t alignment) {
  std::size_t __need = bytes > alignment ? bytes : alignment;
  if (__need <= __opts_.largest_required_pool_block) {
    if (__need < (std::size_t(1) << __min_shift))
      __need = std::size_t(1) << __min_shift;
    const unsigned shift = static_cast<unsigned>(std::bit_width(__need - 1));
    const std::size_t block = std::size_t(1) << shift;
    __pool_bin& __bin = __bin_[shift - __min_shift];
    if (__pool_free_block* b = __bin.free) {
      __bin.free = b->next;
      return b;
    }
    if (__bin.cur != __bin.end) {
      char* p = __bin.cur;
      __bin.cur += block;
      return p;
    }
    return __refill(__bin, block);
  }
  // Directly from upstream, with a footer linking it into big_.
  const std::size_t align = alignment > alignof(__pool_big_footer) ? alignment : alignof(__pool_big_footer);
  if (bytes > size_max - sizeof(__pool_big_footer) - alignof(__pool_big_footer))
    __ycxx::__detail::request_impossible(*__upstream_, align);
  const std::size_t __off = footer_offset<__pool_big_footer>(bytes);
  const std::size_t __total = __off + sizeof(__pool_big_footer);
  char* base = static_cast<char*>(__upstream_->allocate(__total, align));
  auto* __f = ::new (static_cast<void*>(base + __off)) __pool_big_footer{nullptr, __big_, __total, align};
  if (__big_ != nullptr)
    __big_->prev = __f;
  __big_ = __f;
  return base;
}

void __pool_core::deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
  std::size_t __need = bytes > alignment ? bytes : alignment;
  if (__need <= __opts_.largest_required_pool_block) {
    if (__need < (std::size_t(1) << __min_shift))
      __need = std::size_t(1) << __min_shift;
    __pool_bin& __bin = __bin_[std::bit_width(__need - 1) - __min_shift];
    __bin.free = ::new (p) __pool_free_block{__bin.free};
    return;
  }
  auto* __f = reinterpret_cast<__pool_big_footer*>(static_cast<char*>(p) + footer_offset<__pool_big_footer>(bytes));
  (__f->prev != nullptr ? __f->prev->next : __big_) = __f->next;
  if (__f->next != nullptr)
    __f->next->prev = __f->prev;
  __upstream_->deallocate(p, __f->bytes, __f->align);
}

void __pool_core::release() noexcept {
  for (unsigned i = 0; i < __bins_; ++i) {
    free_chunks(__bin_[i].__chunks, __upstream_);
    __bin_[i] = __pool_bin{};
  }
  while (__big_ != nullptr) {
    __pool_big_footer* __f = __big_;
    __big_ = __f->next;
    const std::size_t bytes = __f->bytes, align = __f->align;
    __upstream_->deallocate(reinterpret_cast<char*>(__f) + sizeof(__pool_big_footer) - bytes, bytes, align);
  }
}

// ---- pal_lock --------------------------------------------------------------------------------

void __pal_lock::lock() noexcept {
  // Single-threaded (single_threaded.hpp): nobody else can hold or wait for the lock.
  if (::__ycxx::__detail::__single_threaded() && state == 0) {
    state = 1;
    return;
  }
  std::uint32_t expected = 0;
  if (__atomic_compare_exchange_n(&state, &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
    return;
  // Contended: mark the lock as having waiters and sleep until it is released.
  while (__atomic_exchange_n(&state, 2, __ATOMIC_ACQUIRE) != 0)
    __ycxx_pal_wait(&state, 2);
}

void __pal_lock::unlock() noexcept {
  if (::__ycxx::__detail::__single_threaded()) {
    state = 0;
    return;
  }
  if (__atomic_exchange_n(&state, 0, __ATOMIC_RELEASE) == 2)
    __ycxx_pal_wake_all(&state);
}

namespace {
struct lock_guard {
  __pal_lock& __l;
  explicit lock_guard(__pal_lock& __lk) noexcept : __l(__lk) { __l.lock(); }
  ~lock_guard() { __l.unlock(); }
  lock_guard(const lock_guard&) = delete;
};
} // namespace

}} // namespace __ycxx::__detail

// ---- memory_resource and the global resources ([mem.res.global]) -------------------------------

namespace {

// Uses the aligned allocation functions only for over-aligned requests, as new-expressions do.
class new_delete_resource_t final : public std::pmr::memory_resource {
  void* do_allocate(std::size_t bytes, std::size_t alignment) override {
    if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
      return ::operator new(bytes, std::align_val_t(alignment));
    return ::operator new(bytes);
  }
  void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
    if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
      ::operator delete(p, bytes, std::align_val_t(alignment));
    else
      ::operator delete(p, bytes);
  }
  bool do_is_equal(const memory_resource& other) const noexcept override { return this == &other; }

public:
  constexpr new_delete_resource_t() noexcept = default;
};

class null_memory_resource_t final : public std::pmr::memory_resource {
  void* do_allocate(std::size_t, std::size_t) override { __ycxx::__detail::__throw_bad_alloc(); }
  void do_deallocate(void*, std::size_t, std::size_t) override {}
  bool do_is_equal(const memory_resource& other) const noexcept override { return this == &other; }

public:
  constexpr null_memory_resource_t() noexcept = default;
};

// Constant-initialized, so usable from other translation units' dynamic initializers, and never
// destroyed (the union's destructor does not destroy its member), so usable during static
// destruction and from atexit functions too ([basic.start.term]/7): destroying them would reset
// their vtable pointers to memory_resource's, whose functions are pure virtual.
template <class _Tp>
union immortal {
  _Tp __object;
  constexpr immortal() noexcept : __object() {}
  ~immortal() {}
};
constinit immortal<new_delete_resource_t> new_delete_storage;
constinit immortal<null_memory_resource_t> null_storage;
// The default resource pointer: accessed only with __atomic builtins ([mem.res.global]/6:
// set_default_resource synchronizes with later set/get calls).
constinit std::pmr::memory_resource* default_resource = &new_delete_storage.__object;

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] std { namespace pmr {

memory_resource::~memory_resource() = default;

memory_resource* new_delete_resource() noexcept { return &new_delete_storage.__object; }
memory_resource* null_memory_resource() noexcept { return &null_storage.__object; }

memory_resource* set_default_resource(memory_resource* r) noexcept {
  if (r == nullptr)
    r = &new_delete_storage.__object;
  return __atomic_exchange_n(&default_resource, r, __ATOMIC_ACQ_REL);
}
memory_resource* get_default_resource() noexcept { return __atomic_load_n(&default_resource, __ATOMIC_ACQUIRE); }

// ---- pool resources ([mem.res.pool]) ----------------------------------------------------------

synchronized_pool_resource::synchronized_pool_resource(const pool_options& __opts, memory_resource* __upstream)
    : __core_(__opts, __upstream) {}
synchronized_pool_resource::~synchronized_pool_resource() { release(); }
void synchronized_pool_resource::release() {
  __ycxx::__detail::lock_guard __g(__lock_);
  __core_.release();
}
memory_resource* synchronized_pool_resource::upstream_resource() const { return __core_.__upstream(); }
pool_options synchronized_pool_resource::options() const { return __core_.options(); }
void* synchronized_pool_resource::do_allocate(size_t bytes, size_t alignment) {
  __ycxx::__detail::lock_guard __g(__lock_);
  return __core_.allocate(bytes, alignment);
}
void synchronized_pool_resource::do_deallocate(void* p, size_t bytes, size_t alignment) {
  __ycxx::__detail::lock_guard __g(__lock_);
  __core_.deallocate(p, bytes, alignment);
}
bool synchronized_pool_resource::do_is_equal(const memory_resource& other) const noexcept { return this == &other; }

unsynchronized_pool_resource::unsynchronized_pool_resource(const pool_options& __opts, memory_resource* __upstream)
    : __core_(__opts, __upstream) {}
unsynchronized_pool_resource::~unsynchronized_pool_resource() { release(); }
void unsynchronized_pool_resource::release() { __core_.release(); }
memory_resource* unsynchronized_pool_resource::upstream_resource() const { return __core_.__upstream(); }
pool_options unsynchronized_pool_resource::options() const { return __core_.options(); }
void* unsynchronized_pool_resource::do_allocate(size_t bytes, size_t alignment) {
  return __core_.allocate(bytes, alignment);
}
void unsynchronized_pool_resource::do_deallocate(void* p, size_t bytes, size_t alignment) {
  __core_.deallocate(p, bytes, alignment);
}
bool unsynchronized_pool_resource::do_is_equal(const memory_resource& other) const noexcept { return this == &other; }

// ---- monotonic_buffer_resource ([mem.res.monotonic.buffer]) -----------------------------------

namespace {
// next_buffer_size when no initial size is given, and the growth factor.
constexpr size_t default_buffer_size = 1024;
constexpr size_t growth_factor = 2;

size_t grown(size_t n) noexcept { return n > size_t(-1) / growth_factor ? size_t(-1) : n * growth_factor; }
} // namespace

monotonic_buffer_resource::monotonic_buffer_resource(memory_resource* __upstream)
    : monotonic_buffer_resource(default_buffer_size, __upstream) {}

monotonic_buffer_resource::monotonic_buffer_resource(size_t __initial_size, memory_resource* __upstream)
    : __upstream_rsrc(__upstream), __next_buffer_size(__initial_size), __initial_buffer_(nullptr), __initial_buffer_size_(0),
      __initial_next_size_(__initial_size) {
  __ycxx::__detail::__precondition(__upstream != nullptr, "monotonic_buffer_resource: null upstream resource");
  __ycxx::__detail::__precondition(__initial_size > 0, "monotonic_buffer_resource: initial_size is zero");
  if (__next_buffer_size == 0)
    __next_buffer_size = __initial_next_size_ = 1;
}

monotonic_buffer_resource::monotonic_buffer_resource(void* __buffer, size_t __buffer_size, memory_resource* __upstream)
    : __upstream_rsrc(__upstream), __cur_(static_cast<char*>(__buffer)), __end_(static_cast<char*>(__buffer) + __buffer_size),
      __next_buffer_size(grown(__buffer_size == 0 ? 1 : __buffer_size)), __initial_buffer_(__buffer),
      __initial_buffer_size_(__buffer_size), __initial_next_size_(__next_buffer_size) {
  __ycxx::__detail::__precondition(__upstream != nullptr, "monotonic_buffer_resource: null upstream resource");
  if (__buffer == nullptr)
    __cur_ = __end_ = nullptr;
}

monotonic_buffer_resource::~monotonic_buffer_resource() { release(); }

void monotonic_buffer_resource::release() {
  __ycxx::__detail::free_chunks(__buffers_, __upstream_rsrc);
  __buffers_ = nullptr;
  __cur_ = static_cast<char*>(__initial_buffer_);
  __end_ = __cur_ == nullptr ? nullptr : __cur_ + __initial_buffer_size_;
  __next_buffer_size = __initial_next_size_;
}

memory_resource* monotonic_buffer_resource::upstream_resource() const { return __upstream_rsrc; }

void* monotonic_buffer_resource::do_allocate(size_t bytes, size_t alignment) {
  // "A pointer to allocated storage ([basic.stc.dynamic.allocation])": distinct for every
  // request, so a zero-size request still takes a byte.
  if (bytes == 0)
    bytes = 1;
  // From the current buffer, if it fits.
  if (__cur_ != nullptr) {
    const auto at = reinterpret_cast<uintptr_t>(__cur_);
    const size_t __pad = (alignment - (at & (alignment - 1))) & (alignment - 1);
    if (__pad <= static_cast<size_t>(__end_ - __cur_) && bytes <= static_cast<size_t>(__end_ - __cur_) - __pad) {
      char* p = __cur_ + __pad;
      __cur_ = p + bytes;
      return p;
    }
  }
  // A new buffer: at least max(bytes, next_buffer_size) usable bytes, aligned to `alignment`,
  // with the chain footer after them.
  using footer = __ycxx::__detail::__pool_chunk_footer;
  if (bytes > size_t(-1) - sizeof(footer) - alignof(footer))
    __ycxx::__detail::request_impossible(*__upstream_rsrc, alignment > alignof(footer) ? alignment : alignof(footer));
  size_t usable = bytes > __next_buffer_size ? bytes : __next_buffer_size;
  if (usable > size_t(-1) - sizeof(footer) - alignof(footer))
    usable = bytes;
  usable = __ycxx::__detail::footer_offset<footer>(usable);
  const size_t __total = usable + sizeof(footer);
  const size_t align = alignment > alignof(footer) ? alignment : alignof(footer);
  char* base = static_cast<char*>(__upstream_rsrc->allocate(__total, align));
  __buffers_ = ::new (static_cast<void*>(base + usable)) footer{__buffers_, __total, align};
  __next_buffer_size = grown(__next_buffer_size);
  __cur_ = base + bytes;
  __end_ = base + usable;
  return base;
}

void monotonic_buffer_resource::do_deallocate(void*, size_t, size_t) {}

bool monotonic_buffer_resource::do_is_equal(const memory_resource& other) const noexcept { return this == &other; }

}} // namespace std::pmr
