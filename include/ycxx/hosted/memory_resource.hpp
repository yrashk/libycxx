// libycxx hosted: the pool resources ([mem.res.pool]) and monotonic_buffer_resource
// ([mem.res.monotonic.buffer]). Their members are defined out of line in the hosted runtime
// (src/hosted/memory_resource.cpp); synchronized_pool_resource locks through the PAL's
// address wait/wake.
//
// Pools (__ycxx::__detail::__pool_core): one pool per power-of-two block size from 8 bytes up to
// options().largest_required_pool_block (rounded up to a power of two). A request is served by
// the pool of the smallest block that holds max(bytes, alignment); each block is aligned to its
// size, since every chunk is allocated from upstream with that alignment. A pool hands out
// blocks from its free list first, then from the unused tail of its newest chunk; when both are
// empty it allocates a chunk with twice as many blocks as the previous one (bounded by
// max_blocks_per_chunk and by a per-chunk byte limit). Larger requests go directly upstream and
// are kept on a doubly-linked list, so release() can return them too.
//
// monotonic_buffer_resource: bump allocation from the current buffer; a new buffer from
// upstream is max(bytes, next_buffer_size) and next_buffer_size then doubles. Buffers from
// upstream are chained through a footer at their end.
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/memory_resource.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std { namespace pmr {

// [mem.res.pool.options]
struct pool_options {
  size_t max_blocks_per_chunk = 0;
  size_t largest_required_pool_block = 0;
};

}} // namespace std::pmr

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __pool_free_block;
struct __pool_chunk_footer;
struct __pool_big_footer;

// One pool: the blocks of one size.
struct __pool_bin {
  __pool_free_block* free = nullptr; // deallocated blocks
  char* cur = nullptr;             // unused tail of the newest chunk: [cur, end)
  char* end = nullptr;
  __pool_chunk_footer* __chunks = nullptr;
  std::size_t __next_blocks = 0; // blocks in the next chunk (0: not yet chosen)
};

// The state and algorithms shared by both pool resources.
class __pool_core {
public:
  // Block sizes 8, 16, ..., 2^(min_shift + max_bins - 1).
  static constexpr unsigned __min_shift = 3;
  static constexpr unsigned __max_bins = 14; // up to 64 KiB
  static constexpr std::size_t __largest_block_limit = std::size_t(1) << (__min_shift + __max_bins - 1);
  static constexpr std::size_t __max_blocks_limit = std::size_t(1) << 16;

  __pool_core(const std::pmr::pool_options& __opts, std::pmr::memory_resource* __upstream) noexcept;
  __pool_core(const __pool_core&) = delete;
  __pool_core& operator=(const __pool_core&) = delete;
  ~__pool_core() { release(); }

  void* allocate(std::size_t __bytes, std::size_t alignment);
  void deallocate(void* p, std::size_t __bytes, std::size_t alignment) noexcept;
  void release() noexcept;
  std::pmr::memory_resource* __upstream() const noexcept { return __upstream_; }
  std::pmr::pool_options options() const noexcept { return __opts_; }

private:
  void* __refill(__pool_bin& __bin, std::size_t block);

  std::pmr::memory_resource* __upstream_;
  std::pmr::pool_options __opts_;
  unsigned __bins_; // pools in use: block sizes up to opts_.largest_required_pool_block
  __pool_bin __bin_[__max_bins];
  __pool_big_footer* __big_ = nullptr; // allocations made directly upstream
};

// A lock for synchronized_pool_resource: 0 unlocked, 1 locked, 2 locked with waiters.
struct __pal_lock {
  std::uint32_t state = 0;
  void lock() noexcept;
  void unlock() noexcept;
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace pmr {

// [mem.res.pool.overview]
class synchronized_pool_resource : public memory_resource {
public:
  synchronized_pool_resource(const pool_options& __opts, memory_resource* __upstream);
  synchronized_pool_resource() : synchronized_pool_resource(pool_options(), get_default_resource()) {}
  explicit synchronized_pool_resource(memory_resource* __upstream)
      : synchronized_pool_resource(pool_options(), __upstream) {}
  explicit synchronized_pool_resource(const pool_options& __opts)
      : synchronized_pool_resource(__opts, get_default_resource()) {}
  synchronized_pool_resource(const synchronized_pool_resource&) = delete;
  virtual ~synchronized_pool_resource();
  synchronized_pool_resource& operator=(const synchronized_pool_resource&) = delete;

  void release();
  memory_resource* upstream_resource() const;
  pool_options options() const;

protected:
  void* do_allocate(size_t __bytes, size_t alignment) override;
  void do_deallocate(void* p, size_t __bytes, size_t alignment) override;
  bool do_is_equal(const memory_resource& other) const noexcept override;

private:
  __ycxx::__detail::__pool_core __core_;
  mutable __ycxx::__detail::__pal_lock __lock_;
};

class unsynchronized_pool_resource : public memory_resource {
public:
  unsynchronized_pool_resource(const pool_options& __opts, memory_resource* __upstream);
  unsynchronized_pool_resource() : unsynchronized_pool_resource(pool_options(), get_default_resource()) {}
  explicit unsynchronized_pool_resource(memory_resource* __upstream)
      : unsynchronized_pool_resource(pool_options(), __upstream) {}
  explicit unsynchronized_pool_resource(const pool_options& __opts)
      : unsynchronized_pool_resource(__opts, get_default_resource()) {}
  unsynchronized_pool_resource(const unsynchronized_pool_resource&) = delete;
  virtual ~unsynchronized_pool_resource();
  unsynchronized_pool_resource& operator=(const unsynchronized_pool_resource&) = delete;

  void release();
  memory_resource* upstream_resource() const;
  pool_options options() const;

protected:
  void* do_allocate(size_t __bytes, size_t alignment) override;
  void do_deallocate(void* p, size_t __bytes, size_t alignment) override;
  bool do_is_equal(const memory_resource& other) const noexcept override;

private:
  __ycxx::__detail::__pool_core __core_;
};

// [mem.res.monotonic.buffer]
class monotonic_buffer_resource : public memory_resource {
public:
  explicit monotonic_buffer_resource(memory_resource* __upstream);
  monotonic_buffer_resource(size_t __initial_size, memory_resource* __upstream);
  monotonic_buffer_resource(void* __buffer, size_t __buffer_size, memory_resource* __upstream);
  monotonic_buffer_resource() : monotonic_buffer_resource(get_default_resource()) {}
  explicit monotonic_buffer_resource(size_t __initial_size)
      : monotonic_buffer_resource(__initial_size, get_default_resource()) {}
  monotonic_buffer_resource(void* __buffer, size_t __buffer_size)
      : monotonic_buffer_resource(__buffer, __buffer_size, get_default_resource()) {}
  monotonic_buffer_resource(const monotonic_buffer_resource&) = delete;
  virtual ~monotonic_buffer_resource();
  monotonic_buffer_resource& operator=(const monotonic_buffer_resource&) = delete;

  void release();
  memory_resource* upstream_resource() const;

protected:
  void* do_allocate(size_t __bytes, size_t alignment) override;
  void do_deallocate(void* p, size_t __bytes, size_t alignment) override;
  bool do_is_equal(const memory_resource& other) const noexcept override;

private:
  memory_resource* __upstream_rsrc;
  char* __cur_ = nullptr; // unused part of the current buffer: [cur_, end_)
  char* __end_ = nullptr;
  size_t __next_buffer_size;
  __ycxx::__detail::__pool_chunk_footer* __buffers_ = nullptr; // buffers obtained from upstream
  // Initial values, restored by release().
  void* __initial_buffer_;
  size_t __initial_buffer_size_;
  size_t __initial_next_size_;
};

}} // namespace std::pmr
