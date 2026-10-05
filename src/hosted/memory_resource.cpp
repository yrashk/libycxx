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

namespace ycxx::detail {

// Lives at the start of a free pool block.
struct pool_free_block {
  pool_free_block* next;
};
// Lives at the end of a chunk (pools) or buffer (monotonic_buffer_resource) obtained from
// upstream; `bytes` and `align` are the arguments of that upstream allocate call.
struct pool_chunk_footer {
  pool_chunk_footer* next;
  std::size_t bytes;
  std::size_t align;
};
// Lives at the end of an allocation pool_core passed straight to upstream.
// `bytes` and `align` are the arguments of the upstream allocate call, as for chunks.
struct pool_big_footer {
  pool_big_footer* prev;
  pool_big_footer* next;
  std::size_t bytes;
  std::size_t align;
};

namespace {

constexpr std::size_t size_max = static_cast<std::size_t>(-1);

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

void free_chunks(pool_chunk_footer* c, std::pmr::memory_resource* upstream) noexcept {
  while (c != nullptr) {
    pool_chunk_footer* next = c->next;
    const std::size_t bytes = c->bytes, align = c->align;
    void* base = reinterpret_cast<char*>(c) + sizeof(pool_chunk_footer) - bytes;
    upstream->deallocate(base, bytes, align);
    c = next;
  }
}

} // namespace

// ---- pool_core -----------------------------------------------------------------------------

pool_core::pool_core(const std::pmr::pool_options& opts, std::pmr::memory_resource* upstream) noexcept
    : upstream_(upstream), opts_(opts) {
  ycxx::detail::precondition(upstream != nullptr, "pool resource: null upstream resource");
  if (opts_.max_blocks_per_chunk == 0 || opts_.max_blocks_per_chunk > max_blocks_limit)
    opts_.max_blocks_per_chunk = max_blocks_limit;
  std::size_t largest = opts_.largest_required_pool_block;
  if (largest == 0 || largest > largest_block_limit)
    largest = largest_block_limit;
  if (largest < (std::size_t(1) << min_shift))
    largest = std::size_t(1) << min_shift;
  largest = std::bit_ceil(largest);
  opts_.largest_required_pool_block = largest;
  bins_ = static_cast<unsigned>(std::countr_zero(largest)) - min_shift + 1;
}

void* pool_core::refill(pool_bin& bin, std::size_t block) {
  if (bin.next_blocks == 0) {
    std::size_t n = first_chunk_bytes / block;
    bin.next_blocks = n == 0 ? 1 : n;
  }
  std::size_t limit = max_chunk_bytes / block;
  if (limit == 0)
    limit = 1;
  if (limit > opts_.max_blocks_per_chunk)
    limit = opts_.max_blocks_per_chunk;
  const std::size_t n = bin.next_blocks < limit ? bin.next_blocks : limit;
  // Blocks first, footer after them; block >= alignof(footer), so the footer is aligned.
  const std::size_t bytes = n * block + sizeof(pool_chunk_footer);
  char* base = static_cast<char*>(upstream_->allocate(bytes, block));
  auto* footer = ::new (static_cast<void*>(base + n * block)) pool_chunk_footer{bin.chunks, bytes, block};
  bin.chunks = footer;
  bin.cur = base + block;
  bin.end = base + n * block;
  bin.next_blocks = n < limit ? n * 2 : limit;
  return base;
}

void* pool_core::allocate(std::size_t bytes, std::size_t alignment) {
  std::size_t need = bytes > alignment ? bytes : alignment;
  if (need <= opts_.largest_required_pool_block) {
    if (need < (std::size_t(1) << min_shift))
      need = std::size_t(1) << min_shift;
    const unsigned shift = static_cast<unsigned>(std::bit_width(need - 1));
    const std::size_t block = std::size_t(1) << shift;
    pool_bin& bin = bin_[shift - min_shift];
    if (pool_free_block* b = bin.free) {
      bin.free = b->next;
      return b;
    }
    if (bin.cur != bin.end) {
      char* p = bin.cur;
      bin.cur += block;
      return p;
    }
    return refill(bin, block);
  }
  // Directly from upstream, with a footer linking it into big_.
  const std::size_t align = alignment > alignof(pool_big_footer) ? alignment : alignof(pool_big_footer);
  if (bytes > size_max - sizeof(pool_big_footer) - alignof(pool_big_footer))
    ycxx::detail::throw_bad_alloc();
  const std::size_t off = footer_offset<pool_big_footer>(bytes);
  const std::size_t total = off + sizeof(pool_big_footer);
  char* base = static_cast<char*>(upstream_->allocate(total, align));
  auto* f = ::new (static_cast<void*>(base + off)) pool_big_footer{nullptr, big_, total, align};
  if (big_ != nullptr)
    big_->prev = f;
  big_ = f;
  return base;
}

void pool_core::deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
  std::size_t need = bytes > alignment ? bytes : alignment;
  if (need <= opts_.largest_required_pool_block) {
    if (need < (std::size_t(1) << min_shift))
      need = std::size_t(1) << min_shift;
    pool_bin& bin = bin_[std::bit_width(need - 1) - min_shift];
    bin.free = ::new (p) pool_free_block{bin.free};
    return;
  }
  auto* f = reinterpret_cast<pool_big_footer*>(static_cast<char*>(p) + footer_offset<pool_big_footer>(bytes));
  (f->prev != nullptr ? f->prev->next : big_) = f->next;
  if (f->next != nullptr)
    f->next->prev = f->prev;
  upstream_->deallocate(p, f->bytes, f->align);
}

void pool_core::release() noexcept {
  for (unsigned i = 0; i < bins_; ++i) {
    free_chunks(bin_[i].chunks, upstream_);
    bin_[i] = pool_bin{};
  }
  while (big_ != nullptr) {
    pool_big_footer* f = big_;
    big_ = f->next;
    const std::size_t bytes = f->bytes, align = f->align;
    upstream_->deallocate(reinterpret_cast<char*>(f) + sizeof(pool_big_footer) - bytes, bytes, align);
  }
}

// ---- pal_lock --------------------------------------------------------------------------------

void pal_lock::lock() noexcept {
  // Single-threaded (single_threaded.hpp): nobody else can hold or wait for the lock.
  if (::ycxx::detail::single_threaded() && state == 0) {
    state = 1;
    return;
  }
  std::uint32_t expected = 0;
  if (__atomic_compare_exchange_n(&state, &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
    return;
  // Contended: mark the lock as having waiters and sleep until it is released.
  while (__atomic_exchange_n(&state, 2, __ATOMIC_ACQUIRE) != 0)
    ycxx_pal_wait(&state, 2);
}

void pal_lock::unlock() noexcept {
  if (::ycxx::detail::single_threaded()) {
    state = 0;
    return;
  }
  if (__atomic_exchange_n(&state, 0, __ATOMIC_RELEASE) == 2)
    ycxx_pal_wake_all(&state);
}

namespace {
struct lock_guard {
  pal_lock& l;
  explicit lock_guard(pal_lock& lk) noexcept : l(lk) { l.lock(); }
  ~lock_guard() { l.unlock(); }
  lock_guard(const lock_guard&) = delete;
};
} // namespace

} // namespace ycxx::detail

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
  void* do_allocate(std::size_t, std::size_t) override { ycxx::detail::throw_bad_alloc(); }
  void do_deallocate(void*, std::size_t, std::size_t) override {}
  bool do_is_equal(const memory_resource& other) const noexcept override { return this == &other; }

public:
  constexpr null_memory_resource_t() noexcept = default;
};

// Constant-initialized, so usable from other translation units' dynamic initializers. Their
// destructors do nothing, so use during static destruction is fine as well.
constinit new_delete_resource_t new_delete_instance;
constinit null_memory_resource_t null_instance;
// The default resource pointer: accessed only with __atomic builtins ([mem.res.global]/6:
// set_default_resource synchronizes with later set/get calls).
constinit std::pmr::memory_resource* default_resource = &new_delete_instance;

} // namespace

namespace std::pmr {

memory_resource::~memory_resource() = default;

memory_resource* new_delete_resource() noexcept { return &new_delete_instance; }
memory_resource* null_memory_resource() noexcept { return &null_instance; }

memory_resource* set_default_resource(memory_resource* r) noexcept {
  if (r == nullptr)
    r = &new_delete_instance;
  return __atomic_exchange_n(&default_resource, r, __ATOMIC_ACQ_REL);
}
memory_resource* get_default_resource() noexcept { return __atomic_load_n(&default_resource, __ATOMIC_ACQUIRE); }

// ---- pool resources ([mem.res.pool]) ----------------------------------------------------------

synchronized_pool_resource::synchronized_pool_resource(const pool_options& opts, memory_resource* upstream)
    : core_(opts, upstream) {}
synchronized_pool_resource::~synchronized_pool_resource() { release(); }
void synchronized_pool_resource::release() {
  ycxx::detail::lock_guard g(lock_);
  core_.release();
}
memory_resource* synchronized_pool_resource::upstream_resource() const { return core_.upstream(); }
pool_options synchronized_pool_resource::options() const { return core_.options(); }
void* synchronized_pool_resource::do_allocate(size_t bytes, size_t alignment) {
  ycxx::detail::lock_guard g(lock_);
  return core_.allocate(bytes, alignment);
}
void synchronized_pool_resource::do_deallocate(void* p, size_t bytes, size_t alignment) {
  ycxx::detail::lock_guard g(lock_);
  core_.deallocate(p, bytes, alignment);
}
bool synchronized_pool_resource::do_is_equal(const memory_resource& other) const noexcept { return this == &other; }

unsynchronized_pool_resource::unsynchronized_pool_resource(const pool_options& opts, memory_resource* upstream)
    : core_(opts, upstream) {}
unsynchronized_pool_resource::~unsynchronized_pool_resource() { release(); }
void unsynchronized_pool_resource::release() { core_.release(); }
memory_resource* unsynchronized_pool_resource::upstream_resource() const { return core_.upstream(); }
pool_options unsynchronized_pool_resource::options() const { return core_.options(); }
void* unsynchronized_pool_resource::do_allocate(size_t bytes, size_t alignment) {
  return core_.allocate(bytes, alignment);
}
void unsynchronized_pool_resource::do_deallocate(void* p, size_t bytes, size_t alignment) {
  core_.deallocate(p, bytes, alignment);
}
bool unsynchronized_pool_resource::do_is_equal(const memory_resource& other) const noexcept { return this == &other; }

// ---- monotonic_buffer_resource ([mem.res.monotonic.buffer]) -----------------------------------

namespace {
// next_buffer_size when no initial size is given, and the growth factor.
constexpr size_t default_buffer_size = 1024;
constexpr size_t growth_factor = 2;

size_t grown(size_t n) noexcept { return n > size_t(-1) / growth_factor ? size_t(-1) : n * growth_factor; }
} // namespace

monotonic_buffer_resource::monotonic_buffer_resource(memory_resource* upstream)
    : monotonic_buffer_resource(default_buffer_size, upstream) {}

monotonic_buffer_resource::monotonic_buffer_resource(size_t initial_size, memory_resource* upstream)
    : upstream_rsrc(upstream), next_buffer_size(initial_size), initial_buffer_(nullptr), initial_buffer_size_(0),
      initial_next_size_(initial_size) {
  ycxx::detail::precondition(upstream != nullptr, "monotonic_buffer_resource: null upstream resource");
  ycxx::detail::precondition(initial_size > 0, "monotonic_buffer_resource: initial_size is zero");
  if (next_buffer_size == 0)
    next_buffer_size = initial_next_size_ = 1;
}

monotonic_buffer_resource::monotonic_buffer_resource(void* buffer, size_t buffer_size, memory_resource* upstream)
    : upstream_rsrc(upstream), cur_(static_cast<char*>(buffer)), end_(static_cast<char*>(buffer) + buffer_size),
      next_buffer_size(grown(buffer_size == 0 ? 1 : buffer_size)), initial_buffer_(buffer),
      initial_buffer_size_(buffer_size), initial_next_size_(next_buffer_size) {
  ycxx::detail::precondition(upstream != nullptr, "monotonic_buffer_resource: null upstream resource");
  if (buffer == nullptr)
    cur_ = end_ = nullptr;
}

monotonic_buffer_resource::~monotonic_buffer_resource() { release(); }

void monotonic_buffer_resource::release() {
  ycxx::detail::free_chunks(buffers_, upstream_rsrc);
  buffers_ = nullptr;
  cur_ = static_cast<char*>(initial_buffer_);
  end_ = cur_ == nullptr ? nullptr : cur_ + initial_buffer_size_;
  next_buffer_size = initial_next_size_;
}

memory_resource* monotonic_buffer_resource::upstream_resource() const { return upstream_rsrc; }

void* monotonic_buffer_resource::do_allocate(size_t bytes, size_t alignment) {
  // From the current buffer, if it fits.
  if (cur_ != nullptr) {
    const auto at = reinterpret_cast<uintptr_t>(cur_);
    const size_t pad = (alignment - (at & (alignment - 1))) & (alignment - 1);
    if (pad <= static_cast<size_t>(end_ - cur_) && bytes <= static_cast<size_t>(end_ - cur_) - pad) {
      char* p = cur_ + pad;
      cur_ = p + bytes;
      return p;
    }
  }
  // A new buffer: at least max(bytes, next_buffer_size) usable bytes, aligned to `alignment`,
  // with the chain footer after them.
  using footer = ycxx::detail::pool_chunk_footer;
  if (bytes > size_t(-1) - sizeof(footer) - alignof(footer))
    ycxx::detail::throw_bad_alloc();
  size_t usable = bytes > next_buffer_size ? bytes : next_buffer_size;
  if (usable > size_t(-1) - sizeof(footer) - alignof(footer))
    usable = bytes;
  usable = ycxx::detail::footer_offset<footer>(usable);
  const size_t total = usable + sizeof(footer);
  const size_t align = alignment > alignof(footer) ? alignment : alignof(footer);
  char* base = static_cast<char*>(upstream_rsrc->allocate(total, align));
  buffers_ = ::new (static_cast<void*>(base + usable)) footer{buffers_, total, align};
  next_buffer_size = grown(next_buffer_size);
  cur_ = base + bytes;
  end_ = base + usable;
  return base;
}

void monotonic_buffer_resource::do_deallocate(void*, size_t, size_t) {}

bool monotonic_buffer_resource::do_is_equal(const memory_resource& other) const noexcept { return this == &other; }

} // namespace std::pmr
