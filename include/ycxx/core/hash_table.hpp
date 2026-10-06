// libycxx core: the hash table shared by unordered_map, unordered_multimap, unordered_set and
// unordered_multiset ([unord.req]), their iterators and node type.
//
// Representation: separate chaining over one null-terminated singly linked list of all nodes.
// Each node caches its key's hash. The nodes of a bucket are adjacent in the list, and a
// bucket cell holds the node *before* the bucket's first node (null for an empty bucket), so a
// node can be unlinked with an O(bucket size) walk from its bucket cell. The list's
// before-begin node is one extra cell at the end of the bucket array, so no heap node ever
// points into the container object itself: moving or swapping a table exchanges pointers only,
// and constant evaluation never has to follow a pointer into an NRVO-returned object (GCC 16
// mis-evaluates those, see list.hpp). A default-constructed table owns no bucket array.
//
// The bucket count is zero or a power of two (at least 2); the bucket of hash h is the top
// bits of h * 2^64/phi (Fibonacci hashing), so identity hashes of integers spread well.
// Rehashing never calls the hash function (cached hashes) and only relinks nodes, keeping the
// relative order of the nodes of each new bucket: elements with equivalent keys stay adjacent
// and in order, and references and pointers stay valid ([unord.req.general]/9). It allocates
// the new array first, so a failed rehash has no effect ([unord.req.except]/4).
// Single-element insertion builds the node, hashes, searches, grows the table and only then
// links the node, so an exception leaves the table unchanged ([unord.req.except]/2). New
// elements with a key already present (equivalent-key containers) go after their group.
// Erasure, extraction and clear never call the hash function or predicate except erase(k) and
// extract(k), which search first. Hints are ignored ([unord.req.general]/89, /103, /127).
// Nodes are linked through raw pointers (obtained with to_address from the allocator); node
// handles hold the allocator's pointer type (node_handle.hpp).
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_nonmod.hpp>
#include <ycxx/core/bit.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/node_handle.hpp>
#include <ycxx/core/optional.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __hash_table_access;

struct __hash_node_base {
  __hash_node_base* next;
};

template <class _Vp>
struct __hash_node : __hash_node_base {
  std::size_t hash;
  union {
    _Vp value;
  };
  constexpr __hash_node() noexcept : __hash_node_base{nullptr}, hash(0) {}
  __hash_node(const __hash_node&) = delete;
  constexpr ~__hash_node() {}
};

// The bucket of hash h in a table of 2^(digits - shift) buckets.
constexpr std::size_t __hash_bucket_index(std::size_t h, int shift) noexcept {
  constexpr std::size_t __golden =
      std::numeric_limits<std::size_t>::digits == 64 ? static_cast<std::size_t>(0x9e3779b97f4a7c15ull) : 0x9e3779b9u;
  return static_cast<std::size_t>(h * __golden) >> shift;
}

template <class _Hp, class _Pp>
concept __transparent_hash_pred = requires {
  typename _Hp::is_transparent;
  typename _Pp::is_transparent;
};

// A pair (possibly cv- or ref-qualified) whose first member type is Key, up to cv and ref.
template <class _Ap, class _Key>
concept __pair_with_key = __is_pair_v<std::remove_cvref_t<_Ap>> &&
                        std::is_same_v<std::remove_cvref_t<typename std::remove_cvref_t<_Ap>::first_type>, _Key>;

// [unord.req.general]/245: K is not convertible to either iterator type.
template <class _Kp, class _It, class _CIt>
concept __not_iterator_arg = !std::is_convertible_v<_Kp&&, _It> && !std::is_convertible_v<_Kp&&, _CIt>;

// [associative.general]/2: the types the deduction guides deduce.
template <class _Ip>
using __unord_iter_key_t = std::remove_cvref_t<std::tuple_element_t<0, __iter_value_type<_Ip>>>;
template <class _Ip>
using __unord_iter_mapped_t = std::remove_cvref_t<std::tuple_element_t<1, __iter_value_type<_Ip>>>;
template <class _Ip>
using __unord_iter_alloc_t = std::pair<const __unord_iter_key_t<_Ip>, __unord_iter_mapped_t<_Ip>>;
template <class _Rp>
using __unord_range_key_t = std::remove_cvref_t<std::tuple_element_t<0, std::ranges::range_value_t<_Rp>>>;
template <class _Rp>
using __unord_range_mapped_t = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<_Rp>>>;
template <class _Rp>
using __unord_range_alloc_t = std::pair<const __unord_range_key_t<_Rp>, __unord_range_mapped_t<_Rp>>;

// [unord.req.general]/246: what a deduced Hash or Pred must not be.
template <class _Hp>
concept __unord_hash_arg = !std::is_integral_v<_Hp> && !__qualifies_as_allocator<_Hp>;
template <class _Pp>
concept __unord_pred_arg = !__qualifies_as_allocator<_Pp>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

template <class _Key, class _Value, class _Hash, class _Pred, class _Alloc, bool _Multi>
class __hash_table;

// T is the element type, possibly const.
template <class _Vp, class _Tp, class _Diff>
class __hash_iter {
  using base = ::__ycxx::__detail::__hash_node_base;
  base* __n_ = nullptr;

  template <class, class, class>
  friend class __hash_iter;
  template <class, class, class, class, class, bool>
  friend class __hash_table;

  constexpr explicit __hash_iter(base* n) noexcept : __n_(n) {}

public:
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::forward_iterator_tag;
  using value_type = _Vp;
  using difference_type = _Diff;
  using pointer = _Tp*;
  using reference = _Tp&;

  constexpr __hash_iter() noexcept = default;
  template <class _Up>
    requires std::is_same_v<const _Up, _Tp> && (!std::is_same_v<_Up, _Tp>)
  constexpr __hash_iter(const __hash_iter<_Vp, _Up, _Diff>& __o) noexcept : __n_(__o.__n_) {}

  constexpr reference operator*() const noexcept { return static_cast<::__ycxx::__detail::__hash_node<_Vp>*>(__n_)->value; }
  constexpr pointer operator->() const noexcept {
    return __builtin_addressof(static_cast<::__ycxx::__detail::__hash_node<_Vp>*>(__n_)->value);
  }
  constexpr __hash_iter& operator++() noexcept {
    __n_ = __n_->next;
    return *this;
  }
  constexpr __hash_iter operator++(int) noexcept {
    __hash_iter t = *this;
    __n_ = __n_->next;
    return t;
  }
  friend constexpr bool operator==(const __hash_iter& a, const __hash_iter& b) noexcept { return a.__n_ == b.__n_; }
  // iterator == const_iterator without a conversion, so that no operator== of the value type
  // taking an arbitrary argument competes.
  template <class _Up>
    requires std::is_same_v<std::remove_const_t<_Up>, _Vp> && (!std::is_same_v<_Up, _Tp>)
  friend constexpr bool operator==(const __hash_iter& a, const __hash_iter<_Vp, _Up, _Diff>& b) noexcept {
    return a.__n_ == __hash_iter::__node_of(b);
  }

private:
  template <class _Up>
  static constexpr base* __node_of(const __hash_iter<_Vp, _Up, _Diff>& i) noexcept {
    return i.__n_;
  }
};

// Iterates one bucket: stops at the first node of another bucket.
template <class _Vp, class _Tp, class _Diff>
class __hash_local_iter {
  using base = ::__ycxx::__detail::__hash_node_base;
  base* __n_ = nullptr;
  std::size_t __bucket_ = 0;
  int __shift_ = 0;

  template <class, class, class>
  friend class __hash_local_iter;
  template <class, class, class, class, class, bool>
  friend class __hash_table;

  constexpr __hash_local_iter(base* n, std::size_t b, int shift) noexcept : __n_(n), __bucket_(b), __shift_(shift) {}

public:
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::forward_iterator_tag;
  using value_type = _Vp;
  using difference_type = _Diff;
  using pointer = _Tp*;
  using reference = _Tp&;

  constexpr __hash_local_iter() noexcept = default;
  template <class _Up>
    requires std::is_same_v<const _Up, _Tp> && (!std::is_same_v<_Up, _Tp>)
  constexpr __hash_local_iter(const __hash_local_iter<_Vp, _Up, _Diff>& __o) noexcept
      : __n_(__o.__n_), __bucket_(__o.__bucket_), __shift_(__o.__shift_) {}

  constexpr reference operator*() const noexcept { return static_cast<::__ycxx::__detail::__hash_node<_Vp>*>(__n_)->value; }
  constexpr pointer operator->() const noexcept {
    return __builtin_addressof(static_cast<::__ycxx::__detail::__hash_node<_Vp>*>(__n_)->value);
  }
  constexpr __hash_local_iter& operator++() noexcept {
    __n_ = __n_->next;
    if (__n_ && ::__ycxx::__detail::__hash_bucket_index(static_cast<::__ycxx::__detail::__hash_node<_Vp>*>(__n_)->hash, __shift_) !=
                  __bucket_)
      __n_ = nullptr;
    return *this;
  }
  constexpr __hash_local_iter operator++(int) noexcept {
    __hash_local_iter t = *this;
    ++*this;
    return t;
  }
  friend constexpr bool operator==(const __hash_local_iter& a, const __hash_local_iter& b) noexcept {
    return a.__n_ == b.__n_;
  }
  template <class _Up>
    requires std::is_same_v<std::remove_const_t<_Up>, _Vp> && (!std::is_same_v<_Up, _Tp>)
  friend constexpr bool operator==(const __hash_local_iter& a, const __hash_local_iter<_Vp, _Up, _Diff>& b) noexcept {
    return a.__n_ == __hash_local_iter::__node_of(b);
  }

private:
  template <class _Up>
  static constexpr base* __node_of(const __hash_local_iter<_Vp, _Up, _Diff>& i) noexcept {
    return i.__n_;
  }
};

// The common part of the four unordered containers. Value is Key for the sets and
// pair<const Key, T> for the maps; Multi selects equivalent keys. The public members are
// those of [unord.req]; the derived std:: classes add constructors, assignment, swap, merge and
// the map- or set-specific members.
template <class _Key, class _Value, class _Hash, class _Pred, class _Alloc, bool _Multi>
class __hash_table {
  template <class, class, class, class, class, bool>
  friend class __hash_table;
  friend struct ::__ycxx::__detail::__hash_table_access;

  // Diagnosed preconditions: Hash meets Cpp17Hash ([unord.req.general]/3) and Pred
  // Cpp17CopyConstructible (/20).
  static_assert(std::is_copy_constructible_v<_Hash>, "unordered container: Hash must be copy constructible");
  static_assert(std::is_copy_constructible_v<_Pred>, "unordered container: Pred must be copy constructible");

protected:
  using info = ::__ycxx::__detail::__alloc_info<_Alloc>;
  static constexpr bool __is_map = !std::is_same_v<_Key, _Value>;
  using __node = ::__ycxx::__detail::__hash_node<_Value>;
  using __node_base = ::__ycxx::__detail::__hash_node_base;
  using __node_alloc = typename info::template rebind<__node>;
  using __node_traits = std::allocator_traits<__node_alloc>;
  using __cell_alloc = typename info::template rebind<__node_base>;
  using __cell_traits = std::allocator_traits<__cell_alloc>;
  using __nh_access = ::__ycxx::__detail::__node_handle_access;

public:
  using key_type = _Key;
  using value_type = _Value;
  using hasher = _Hash;
  using key_equal = _Pred;
  using allocator_type = _Alloc;
  using pointer = typename info::pointer;
  using const_pointer = typename info::const_pointer;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = typename info::size_type;
  using difference_type = typename info::difference_type;
  using iterator = __hash_iter<_Value, std::conditional_t<__is_map, _Value, const _Value>, difference_type>;
  using const_iterator = __hash_iter<_Value, const _Value, difference_type>;
  using local_iterator = __hash_local_iter<_Value, std::conditional_t<__is_map, _Value, const _Value>, difference_type>;
  using const_local_iterator = __hash_local_iter<_Value, const _Value, difference_type>;
  using node_type = __node_handle<::__ycxx::__detail::__hash_node<_Value>, _Alloc, __is_map>;

protected:
  using __emplace_result = std::conditional_t<_Multi, iterator, std::pair<iterator, bool>>;
  static constexpr bool __transparent = ::__ycxx::__detail::__transparent_hash_pred<_Hash, _Pred>;
  static constexpr bool __distinct_iterators = !std::is_same_v<iterator, const_iterator>;

  __node_base* __cells_ = nullptr; // bucket_count_ bucket cells, then the before-begin node
  size_type __bucket_count_ = 0;
  size_type __size_ = 0;
  int __shift_ = 0;
  float __mlf_ = 1.0f;
  [[no_unique_address]] _Hash __hash_;
  [[no_unique_address]] _Pred __pred_;
  [[no_unique_address]] __node_alloc __na_;

  // ---- nodes ----
  static constexpr iterator __to_iter(__node_base* n) noexcept { return iterator(n); }
  static constexpr __node* __as_node(__node_base* p) noexcept { return static_cast<__node*>(p); }
  static constexpr const _Key& __key_of(const _Value& __v) noexcept {
    if constexpr (__is_map)
      return __v.first;
    else
      return __v;
  }
  constexpr __node_base* __head() const noexcept { return __cells_ + __bucket_count_; }
  constexpr __node_base* first() const noexcept { return __cells_ ? __cells_[__bucket_count_].next : nullptr; }
  constexpr size_type __bucket_of(std::size_t h) const noexcept {
    return static_cast<size_type>(::__ycxx::__detail::__hash_bucket_index(h, __shift_));
  }
  template <class _Kp>
  constexpr std::size_t __hash_of(const _Kp& k) const {
    return static_cast<std::size_t>(__hash_(k));
  }
  template <class _Kp>
  constexpr bool equal(const _Kp& k, const __node* n) const {
    return static_cast<bool>(__pred_(k, __key_of(n->value)));
  }

  template <class... _Args>
  constexpr __node* __make_node(_Args&&... __args) {
    __node* n = std::to_address(__node_traits::allocate(__na_, 1));
    std::construct_at(n);
    ::__ycxx::__detail::__rollback __rb{[&] {
      std::destroy_at(n);
      __node_traits::deallocate(__na_, ::__ycxx::__detail::__to_alloc_pointer<typename __node_traits::pointer>(n), 1);
    }};
    __node_traits::construct(__na_, __builtin_addressof(n->value), static_cast<_Args&&>(__args)...);
    __rb.release();
    return n;
  }
  constexpr void __free_node(__node* n) noexcept {
    __node_traits::destroy(__na_, __builtin_addressof(n->value));
    std::destroy_at(n);
    __node_traits::deallocate(__na_, ::__ycxx::__detail::__to_alloc_pointer<typename __node_traits::pointer>(n), 1);
  }
  // Frees a node unless released: the node of an insertion that may still fail.
  struct __node_guard {
    __hash_table* t;
    __node* n;
    constexpr ~__node_guard() {
      if (n)
        t->__free_node(n);
    }
    constexpr __node* release() noexcept {
      __node* r = n;
      n = nullptr;
      return r;
    }
  };

  // ---- the bucket array ----
  constexpr __node_base* __alloc_cells(size_type count) {
    __cell_alloc __ca(__na_);
    __node_base* c = std::to_address(__cell_traits::allocate(__ca, count + 1));
    for (size_type i = 0; i <= count; ++i)
      std::construct_at(c + i, __node_base{nullptr});
    return c;
  }
  constexpr void __free_cells(__node_base* c, size_type count) noexcept {
    if (!c)
      return;
    __cell_alloc __ca(__na_); // the cells are trivially destructible
    __cell_traits::deallocate(__ca, ::__ycxx::__detail::__to_alloc_pointer<typename __cell_traits::pointer>(c), count + 1);
  }
  static constexpr int __shift_for(size_type count) noexcept {
    return std::numeric_limits<std::size_t>::digits - std::countr_zero(static_cast<std::size_t>(count));
  }
  // The bucket count to use for at least n buckets: 0, or a power of two >= 2.
  constexpr size_type __round_buckets(size_type n) const {
    if (n == 0)
      return 0;
    if (n > max_bucket_count())
      ::__ycxx::__detail::__throw_length_error("unordered container: too many buckets");
    return n <= 2 ? size_type(2) : static_cast<size_type>(std::bit_ceil(static_cast<std::size_t>(n)));
  }
  // ceil(n / max_load_factor()).
  constexpr size_type __buckets_for(size_type n) const {
    const float __f = static_cast<float>(n) / __mlf_;
    if (!(__f < static_cast<float>(max_bucket_count())))
      ::__ycxx::__detail::__throw_length_error("unordered container: too many buckets");
    size_type c = static_cast<size_type>(__f);
    if (static_cast<float>(c) < __f)
      ++c;
    return c;
  }

  // Relinks every node into a new array of count buckets (0, or a power of two >= 2; 0 only
  // when empty). Never calls the hash function; only the allocation can throw, before any
  // change.
  constexpr void __rehash_to(size_type count) {
    if (count == __bucket_count_)
      return;
    if (count == 0) {
      __free_cells(__cells_, __bucket_count_);
      __cells_ = nullptr;
      __bucket_count_ = 0;
      __shift_ = 0;
      return;
    }
    __node_base* __nc = __alloc_cells(count);
    const int ns = __shift_for(count);
    __node_base* __nh = __nc + count;
    __node_base* p = first();
    // Pass 1: append each node after the last node of its new bucket (or at the end of the
    // list for a new bucket); nc[b] temporarily holds the bucket's last node.
    __node_base* __tail = __nh;
    while (p) {
      __node_base* const next = p->next;
      const std::size_t b = ::__ycxx::__detail::__hash_bucket_index(__as_node(p)->hash, ns);
      __node_base* const last = __nc[b].next;
      if (last) {
        p->next = last->next;
        last->next = p;
        if (__tail == last)
          __tail = p;
      } else {
        p->next = nullptr;
        __tail->next = p;
        __tail = p;
      }
      __nc[b].next = p;
      p = next;
    }
    // Pass 2: the buckets' runs are contiguous; each cell gets the node before its run.
    std::size_t __prev_bucket = static_cast<std::size_t>(-1);
    for (__node_base *prev = __nh, *__q = __nh->next; __q; prev = __q, __q = __q->next) {
      const std::size_t b = ::__ycxx::__detail::__hash_bucket_index(__as_node(__q)->hash, ns);
      if (b != __prev_bucket) {
        __nc[b].next = prev;
        __prev_bucket = b;
      }
    }
    __free_cells(__cells_, __bucket_count_);
    __cells_ = __nc;
    __bucket_count_ = count;
    __shift_ = ns;
  }
  // Makes room for n more elements without exceeding the maximum load factor.
  constexpr void __grow_for(size_type n) {
    const size_type __want = __size_ + n;
    if (__bucket_count_ != 0 && !(static_cast<float>(__want) > static_cast<float>(__bucket_count_) * __mlf_))
      return;
    size_type c = __buckets_for(__want);
    const size_type __twice = __bucket_count_ * 2;
    if (c < __twice)
      c = __twice;
    if (c < 8)
      c = 8;
    __rehash_to(__round_buckets(c));
  }

  // ---- linking ----
  // Links n as the first node of bucket b.
  constexpr void __link_front(__node* n, size_type b) noexcept {
    __node_base* const prev = __cells_[b].next;
    if (prev) {
      n->next = prev->next;
      prev->next = n;
    } else {
      __node_base* const h = __head();
      n->next = h->next;
      h->next = n;
      __cells_[b].next = h;
      if (n->next)
        __cells_[__bucket_of(__as_node(n->next)->hash)].next = n;
    }
    ++__size_;
  }
  // Links n after pos, a node of bucket b.
  constexpr void __link_after(__node_base* __pos, __node* n, size_type b) noexcept {
    n->next = __pos->next;
    __pos->next = n;
    if (n->next) {
      const size_type __nb = __bucket_of(__as_node(n->next)->hash);
      if (__nb != b)
        __cells_[__nb].next = n;
    }
    ++__size_;
  }
  // Unlinks the node after prev, which is in bucket b.
  constexpr __node* __unlink_after(__node_base* prev, size_type b) noexcept {
    __node* const n = __as_node(prev->next);
    __node_base* const next = n->next;
    if (__cells_[b].next == prev) { // n is the first node of its bucket
      if (!next) {
        __cells_[b].next = nullptr;
      } else {
        const size_type __nb = __bucket_of(__as_node(next)->hash);
        if (__nb != b) { // n was the only node of its bucket
          __cells_[__nb].next = prev;
          __cells_[b].next = nullptr;
        }
      }
    } else if (next) {
      const size_type __nb = __bucket_of(__as_node(next)->hash);
      if (__nb != b)
        __cells_[__nb].next = prev;
    }
    prev->next = next;
    --__size_;
    return n;
  }
  // The node before n.
  constexpr __node_base* __prev_of(const __node_base* n) const noexcept {
    __node_base* p = __cells_[__bucket_of(__as_node(const_cast<__node_base*>(n))->hash)].next;
    while (p->next != n)
      p = p->next;
    return p;
  }

  // ---- search ----
  // The node before the first node with key equivalent to k (hash h), or null.
  template <class _Kp>
  constexpr __node_base* __find_prev(const _Kp& k, std::size_t h) const {
    if (__size_ == 0)
      return nullptr;
    const size_type b = __bucket_of(h);
    __node_base* prev = __cells_[b].next;
    if (!prev)
      return nullptr;
    for (__node_base* p = prev->next; p; prev = p, p = p->next) {
      const __node* const n = __as_node(p);
      if (n->hash == h) {
        if (equal(k, n))
          return prev;
      } else if (__bucket_of(n->hash) != b) {
        break;
      }
    }
    return nullptr;
  }
  template <class _Kp>
  constexpr __node_base* __find_node(const _Kp& k) const {
    if (__size_ == 0)
      return nullptr;
    __node_base* const prev = __find_prev(k, __hash_of(k));
    return prev ? prev->next : nullptr;
  }
  // The last node of the group starting at first (key k, hash h).
  template <class _Kp>
  constexpr __node_base* __group_last(__node_base* first, const _Kp& k, std::size_t h) const {
    __node_base* last = first;
    for (__node_base* p = first->next; p && __as_node(p)->hash == h && equal(k, __as_node(p)); p = p->next)
      last = p;
    return last;
  }
  // The number of nodes of the group starting at first.
  template <class _Kp>
  constexpr size_type __group_size(__node_base* first, const _Kp& k, std::size_t h) const {
    size_type c = 1;
    for (__node_base* p = first->next; p && __as_node(p)->hash == h && equal(k, __as_node(p)); p = p->next)
      ++c;
    return c;
  }

  // ---- insertion ----
  // Links the detached node n (hash h, already searched for: prev is the node before its
  // group, or null). Grows the table first; the only failure is that allocation.
  constexpr __node* __link_new(__node* n, std::size_t h, __node_base* __group_prev) {
    __node_base* last = nullptr;
    if constexpr (_Multi) {
      if (__group_prev)
        last = __group_last(__group_prev->next, __key_of(n->value), h);
    }
    __grow_for(1);
    n->hash = h;
    if (last)
      __link_after(last, n, __bucket_of(h));
    else
      __link_front(n, __bucket_of(h));
    return n;
  }
  constexpr __emplace_result __make_result(__node_base* n, bool __inserted) noexcept {
    if constexpr (_Multi)
      return iterator(n);
    else
      return __emplace_result(iterator(n), __inserted);
  }
  // Inserts a constructed node (freed if it is not kept).
  constexpr __emplace_result __insert_node(__node* n) {
    __node_guard __g{this, n};
    const std::size_t h = __hash_of(__key_of(n->value));
    __node_base* const prev = __find_prev(__key_of(n->value), h);
    if constexpr (!_Multi) {
      if (prev)
        return __make_result(prev->next, false);
    }
    __link_new(n, h, prev);
    return __make_result(__g.release(), true);
  }
  template <class... _Args>
  constexpr __emplace_result __emplace_impl(_Args&&... __args) {
    if constexpr (!_Multi && __key_arg<_Args...>)
      return __emplace_key(__arg_key(__args...), static_cast<_Args&&>(__args)...);
    else
      return __insert_node(__make_node(static_cast<_Args&&>(__args)...));
  }
  // Unique keys: emplace arguments whose key can be read without building the element (a
  // value_type, a pair whose first member is a Key, or for maps a Key followed by the mapped
  // value's argument), so that an existing key costs no allocation.
  template <class... _Args>
  static constexpr bool __key_arg = false;
  template <class _Ap>
  static constexpr bool __key_arg<_Ap> =
      std::is_same_v<std::remove_cvref_t<_Ap>, _Value> || (__is_map && ::__ycxx::__detail::__pair_with_key<_Ap, _Key>);
  template <class _Ap, class _Bp>
  static constexpr bool __key_arg<_Ap, _Bp> = __is_map && std::is_same_v<std::remove_cvref_t<_Ap>, _Key>;
  template <class _Ap>
  static constexpr const _Key& __arg_key(const _Ap& a) noexcept {
    if constexpr (std::is_same_v<std::remove_cvref_t<_Ap>, _Value>)
      return __key_of(a);
    else
      return a.first;
  }
  template <class _Ap, class _Bp>
  static constexpr const _Key& __arg_key(const _Ap& a, const _Bp&) noexcept {
    return a;
  }
  // Unique keys: inserts a node built from args unless an element with key k exists; the
  // node is built only if needed ([unord.map.modifiers]/6, /15).
  template <class _Kp, class... _Args>
  constexpr std::pair<iterator, bool> __emplace_key(const _Kp& k, _Args&&... __args) {
    const std::size_t h = __hash_of(k);
    if (__node_base* const prev = __find_prev(k, h))
      return {iterator(prev->next), false};
    __node_guard __g{this, __make_node(static_cast<_Args&&>(__args)...)};
    __link_new(__g.n, h, nullptr);
    return {iterator(__g.release()), true};
  }
  // Inserts a value; for unique keys, searches before copying or moving it.
  template <class _Vp>
  constexpr __emplace_result __insert_value(_Vp&& __v) {
    if constexpr (_Multi)
      return __emplace_impl(static_cast<_Vp&&>(__v));
    else
      return __emplace_key(__key_of(__v), static_cast<_Vp&&>(__v));
  }
  // Equivalent keys: the node at hint if it holds a key equivalent to k (hash h), else null.
  template <class _Kp>
  constexpr __node_base* __hint_pos(const_iterator __hint, const _Kp& k, std::size_t h) const {
    __node_base* const p = __hint.__n_;
    if (p && __as_node(p)->hash == h && equal(k, __as_node(p)))
      return p;
    return nullptr;
  }
  // Equivalent keys: links the detached node n right after the hint when the hint holds an
  // equivalent key, else at the end of its group.
  constexpr void __link_hinted(const_iterator __hint, __node* n) {
    const std::size_t h = __hash_of(__key_of(n->value));
    if (__node_base* const __pos = __hint_pos(__hint, __key_of(n->value), h)) {
      __grow_for(1);
      n->hash = h;
      __link_after(__pos, n, __bucket_of(h));
    } else {
      __link_new(n, h, __find_prev(__key_of(n->value), h));
    }
  }
  constexpr iterator __insert_node_hint(const_iterator __hint, __node* n) {
    __node_guard __g{this, n};
    __link_hinted(__hint, n);
    return iterator(__g.release());
  }
  template <class _It, class _Sent>
  constexpr void __insert_elems(_It first, _Sent last) {
    // Equivalent keys: every element is inserted, so the table can grow once. (Unique keys
    // cannot: a rehash for duplicates would invalidate iterators that [unord.req.general]/243
    // keeps valid.)
    if constexpr (_Multi && ::__ycxx::__detail::__multipass_iterator<_It> && std::is_same_v<_It, _Sent>) {
      const auto n = ::__ycxx::__detail::__iter_pair_distance(first, last);
      if (n > 0)
        __grow_for(static_cast<size_type>(n));
    }
    for (; first != last; ++first)
      __emplace_impl(*first);
  }

  // Copies (or, with move_values, moves) the nodes of o in order into this empty table with the
  // same bucket count and the same hashes, without calling the hash function or predicate.
  template <bool __move_values, class _Op>
  constexpr void __clone_from(_Op& __o) {
    if (__o.__size_ == 0)
      return;
    __rehash_to(__o.__bucket_count_);
    __node_base* __tail = __head();
    std::size_t __prev_bucket = static_cast<std::size_t>(-1);
    for (__node_base* p = __o.first(); p; p = p->next) {
      __node* const __src = __as_node(p);
      __node* n;
      if constexpr (__move_values)
        n = __make_node(static_cast<_Value&&>(__src->value));
      else
        n = __make_node(static_cast<const _Value&>(__src->value));
      n->hash = __src->hash;
      n->next = nullptr;
      __tail->next = n;
      const size_type b = __bucket_of(n->hash);
      if (b != __prev_bucket) {
        __cells_[b].next = __tail;
        __prev_bucket = b;
      }
      __tail = n;
      ++__size_;
    }
  }
  // Takes o's nodes and bucket array.
  constexpr void __steal(__hash_table& __o) noexcept {
    __cells_ = __o.__cells_;
    __bucket_count_ = __o.__bucket_count_;
    __size_ = __o.__size_;
    __shift_ = __o.__shift_;
    __o.__cells_ = nullptr;
    __o.__bucket_count_ = 0;
    __o.__size_ = 0;
    __o.__shift_ = 0;
  }
  constexpr void __free_all() noexcept {
    clear();
    __free_cells(__cells_, __bucket_count_);
    __cells_ = nullptr;
    __bucket_count_ = 0;
    __shift_ = 0;
  }

  // ---- construction, assignment, swap (used by the derived classes) ----
  static constexpr bool __nothrow_default = std::is_nothrow_default_constructible_v<_Hash> &&
                                          std::is_nothrow_default_constructible_v<_Pred> &&
                                          std::is_nothrow_default_constructible_v<_Alloc>;
  // The move constructors copy the hash function and predicate, so that the moved-from
  // container keeps working ones.
  static constexpr bool __nothrow_move =
      std::is_nothrow_copy_constructible_v<_Hash> && std::is_nothrow_copy_constructible_v<_Pred>;
  constexpr __hash_table() noexcept(__nothrow_default) : __hash_(), __pred_(), __na_(_Alloc()) {}
  constexpr __hash_table(size_type n, const _Hash& __hf, const _Pred& __eql, const _Alloc& a)
      : __hash_(__hf), __pred_(__eql), __na_(a) {
    // No node is touched here, so a container of an incomplete class type can be a member
    // of that class (as with the other node-based containers).
    if (n > 0) {
      const size_type c = __round_buckets(n);
      __cells_ = __alloc_cells(c);
      __bucket_count_ = c;
      __shift_ = __shift_for(c);
    }
  }
  constexpr __hash_table(const __hash_table& __o, const _Alloc& a)
      : __mlf_(__o.__mlf_), __hash_(__o.__hash_), __pred_(__o.__pred_), __na_(a) {
    ::__ycxx::__detail::__rollback __rb{[this] { __free_all(); }};
    __clone_from<false>(__o);
    __rb.release();
  }
  constexpr __hash_table(const __hash_table& __o)
      : __hash_table(__o, std::allocator_traits<_Alloc>::select_on_container_copy_construction(_Alloc(__o.__na_))) {}
  constexpr __hash_table(__hash_table&& __o) noexcept(__nothrow_move)
      : __mlf_(__o.__mlf_), __hash_(__o.__hash_), __pred_(__o.__pred_),
        __na_(static_cast<__node_alloc&&>(__o.__na_)) {
    __steal(__o);
  }
  constexpr __hash_table(__hash_table&& __o, const _Alloc& a) noexcept(info::__always_equal && __nothrow_move)
      : __mlf_(__o.__mlf_), __hash_(__o.__hash_), __pred_(__o.__pred_), __na_(a) {
    if (info::__always_equal || __na_ == __o.__na_) {
      __steal(__o);
    } else {
      ::__ycxx::__detail::__rollback __rb{[this] { __free_all(); }};
      __clone_from<true>(__o);
      __rb.release();
      __o.__free_all();
    }
  }
  constexpr ~__hash_table() { __free_all(); }

  constexpr void __copy_assign(const __hash_table& __o) {
    if (this == __builtin_addressof(__o))
      return;
    __free_all();
    if constexpr (info::__pocca)
      __na_ = __o.__na_;
    __hash_ = __o.__hash_;
    __pred_ = __o.__pred_;
    __mlf_ = __o.__mlf_;
    __clone_from<false>(__o);
  }
  constexpr void __move_assign(__hash_table& __o) {
    if (this == __builtin_addressof(__o))
      return;
    __free_all();
    __hash_ = static_cast<_Hash&&>(__o.__hash_);
    __pred_ = static_cast<_Pred&&>(__o.__pred_);
    __mlf_ = __o.__mlf_;
    if constexpr (info::__pocma) {
      __na_ = static_cast<__node_alloc&&>(__o.__na_);
      __steal(__o);
    } else if (info::__always_equal || __na_ == __o.__na_) {
      __steal(__o);
    } else {
      __clone_from<true>(__o);
      __o.__free_all();
    }
  }
  constexpr void __swap_impl(__hash_table& __o) {
    if (this == __builtin_addressof(__o))
      return;
    if constexpr (info::__pocs)
      ::__ycxx::__detail::__swap_adl::__do_swap(__na_, __o.__na_);
    else
      ::__ycxx::__detail::__precondition(info::__always_equal || __na_ == __o.__na_,
                                   "unordered container swap: unequal allocators that do not propagate");
    ::__ycxx::__detail::__swap_adl::__do_swap(__hash_, __o.__hash_);
    ::__ycxx::__detail::__swap_adl::__do_swap(__pred_, __o.__pred_);
    __node_base* const c = __cells_;
    __cells_ = __o.__cells_;
    __o.__cells_ = c;
    const size_type __bc = __bucket_count_;
    __bucket_count_ = __o.__bucket_count_;
    __o.__bucket_count_ = __bc;
    const size_type s = __size_;
    __size_ = __o.__size_;
    __o.__size_ = s;
    const int __y_sh = __shift_;
    __shift_ = __o.__shift_;
    __o.__shift_ = __y_sh;
    const float m = __mlf_;
    __mlf_ = __o.__mlf_;
    __o.__mlf_ = m;
  }
  constexpr void __assign_il(std::initializer_list<_Value> il) {
    clear();
    __insert_elems(il.begin(), il.end());
  }

  // Moves the nodes of src into *this ([unord.req.general]/145-146).
  template <class _K2, class _H2, class _P2, bool _M2>
  constexpr void __merge_from(__hash_table<_K2, _Value, _H2, _P2, _Alloc, _M2>& __src) {
    if (static_cast<void*>(this) == static_cast<void*>(__builtin_addressof(__src)))
      return;
    if constexpr (!info::__always_equal)
      ::__ycxx::__detail::__precondition(__na_ == __src.__na_, "unordered container merge: unequal allocators");
    __node_base* prev = __src.first() ? __src.__head() : nullptr;
    while (prev && prev->next) {
      __node* const n = __as_node(prev->next);
      const std::size_t h = __hash_of(__key_of(n->value));
      __node_base* const __gp = __find_prev(__key_of(n->value), h);
      if constexpr (!_Multi) {
        if (__gp) {
          prev = n;
          continue;
        }
      }
      __node_base* last = nullptr;
      if constexpr (_Multi) {
        if (__gp)
          last = __group_last(__gp->next, __key_of(n->value), h);
      }
      __grow_for(1);
      __src.__unlink_after(prev, __src.__bucket_of(n->hash));
      n->hash = h;
      if (last)
        __link_after(last, n, __bucket_of(h));
      else
        __link_front(n, __bucket_of(h));
    }
  }

  // Erases the count nodes after prev.
  constexpr void erase_after(__node_base* prev, size_type count) noexcept {
    for (; count > 0; --count)
      __free_node(__unlink_after(prev, __bucket_of(__as_node(prev->next)->hash)));
  }

  // ---- [unord.req] equality ----
  constexpr bool __equal_to_table(const __hash_table& __o) const {
    if (__size_ != __o.__size_)
      return false;
    for (__node_base* p = first(); p;) {
      const _Key& k = __key_of(__as_node(p)->value);
      __node_base* const op = __o.__find_node(k);
      if (!op)
        return false;
      if constexpr (!_Multi) {
        if (!(__as_node(p)->value == __as_node(op)->value))
          return false;
        p = p->next;
      } else {
        // the group [p, pe) here and [op, oe) there
        __node_base* __pe = p->next;
        size_type n = 1;
        const std::size_t h = __as_node(p)->hash;
        while (__pe && __as_node(__pe)->hash == h && equal(k, __as_node(__pe))) {
          __pe = __pe->next;
          ++n;
        }
        __node_base* __oe = op->next;
        size_type m = 1;
        const std::size_t __oh = __as_node(op)->hash;
        while (__oe && __as_node(__oe)->hash == __oh && __o.equal(k, __as_node(__oe))) {
          __oe = __oe->next;
          ++m;
        }
        if (n != m || !std::is_permutation(const_iterator(p), const_iterator(__pe), const_iterator(op), const_iterator(__oe)))
          return false;
        p = __pe;
      }
    }
    return true;
  }

public:
  constexpr allocator_type get_allocator() const noexcept { return allocator_type(__na_); }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(first()); }
  constexpr const_iterator begin() const noexcept { return const_iterator(first()); }
  constexpr iterator end() noexcept { return iterator(nullptr); }
  constexpr const_iterator end() const noexcept { return const_iterator(nullptr); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }

  // ---- capacity ----
  [[nodiscard]] constexpr bool empty() const noexcept { return __size_ == 0; }
  constexpr size_type size() const noexcept { return __size_; }
  constexpr size_type max_size() const noexcept {
    const auto a = static_cast<size_type>(__node_traits::max_size(__na_));
    const auto d = static_cast<size_type>(std::numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }

  // ---- modifiers ----
  template <class... _Args>
  constexpr __emplace_result emplace(_Args&&... __args) {
    return __emplace_impl(static_cast<_Args&&>(__args)...);
  }
  template <class... _Args>
  constexpr iterator emplace_hint(const_iterator __hint, _Args&&... __args) {
    if constexpr (_Multi)
      return __insert_node_hint(__hint, __make_node(static_cast<_Args&&>(__args)...));
    else
      return __emplace_impl(static_cast<_Args&&>(__args)...).first;
  }
  constexpr __emplace_result insert(const value_type& __obj) { return __insert_value(__obj); }
  constexpr __emplace_result insert(value_type&& __obj) { return __insert_value(static_cast<value_type&&>(__obj)); }
  constexpr iterator insert(const_iterator __hint, const value_type& __obj) {
    if constexpr (_Multi)
      return __insert_node_hint(__hint, __make_node(__obj));
    else
      return __insert_value(__obj).first;
  }
  constexpr iterator insert(const_iterator __hint, value_type&& __obj) {
    if constexpr (_Multi)
      return __insert_node_hint(__hint, __make_node(static_cast<value_type&&>(__obj)));
    else
      return __insert_value(static_cast<value_type&&>(__obj)).first;
  }
  template <class _InputIterator>
  constexpr void insert(_InputIterator first, _InputIterator last) {
    __insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <::__ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr void insert_range(_Rp&& __rg) {
    __insert_elems(std::ranges::begin(__rg), std::ranges::end(__rg));
  }
  constexpr void insert(std::initializer_list<value_type> il) { __insert_elems(il.begin(), il.end()); }

  constexpr node_type extract(const_iterator position) {
    ::__ycxx::__detail::__precondition(position.__n_ != nullptr, "unordered container extract: end() iterator");
    __node* const n = __unlink_after(__prev_of(position.__n_), __bucket_of(__as_node(position.__n_)->hash));
    return __nh_access::__make<node_type>(n, get_allocator());
  }
  constexpr node_type extract(const key_type& __x) { return __extract_key(__x); }
  template <class _Kp>
    requires __transparent && ::__ycxx::__detail::__not_iterator_arg<_Kp, iterator, const_iterator>
  constexpr node_type extract(_Kp&& __x) {
    return __extract_key(__x);
  }
  constexpr std::conditional_t<_Multi, iterator, insert_return_type<iterator, node_type>> insert(node_type&& __nh) {
    if (__nh.empty()) {
      if constexpr (_Multi)
        return end();
      else
        return {end(), false, node_type()};
    }
    ::__ycxx::__detail::__precondition(info::__always_equal || __nh.get_allocator() == get_allocator(),
                                 "unordered container insert(node_type&&): unequal allocators");
    __node* const n = __nh_access::peek(__nh);
    const std::size_t h = __hash_of(__key_of(n->value));
    __node_base* const prev = __find_prev(__key_of(n->value), h);
    if constexpr (!_Multi) {
      if (prev)
        return {iterator(prev->next), false, static_cast<node_type&&>(__nh)};
    }
    __link_new(n, h, prev);
    __nh_access::take(__nh);
    if constexpr (_Multi)
      return iterator(n);
    else
      return {iterator(n), true, node_type()};
  }
  constexpr iterator insert(const_iterator __hint, node_type&& __nh) {
    if (__nh.empty())
      return end();
    ::__ycxx::__detail::__precondition(info::__always_equal || __nh.get_allocator() == get_allocator(),
                                 "unordered container insert(node_type&&): unequal allocators");
    __node* const n = __nh_access::peek(__nh);
    if constexpr (_Multi) {
      __link_hinted(__hint, n);
      __nh_access::take(__nh);
      return iterator(n);
    }
    const std::size_t h = __hash_of(__key_of(n->value));
    __node_base* const prev = __find_prev(__key_of(n->value), h);
    if constexpr (!_Multi) {
      if (prev)
        return iterator(prev->next);
    }
    __link_new(n, h, prev);
    __nh_access::take(__nh);
    return iterator(n);
  }

  constexpr iterator erase(iterator position)
    requires __distinct_iterators
  {
    return erase(const_iterator(position));
  }
  constexpr iterator erase(const_iterator position) {
    ::__ycxx::__detail::__precondition(position.__n_ != nullptr, "unordered container erase: end() iterator");
    __node_base* const n = position.__n_;
    __node_base* const next = n->next;
    __free_node(__unlink_after(__prev_of(n), __bucket_of(__as_node(n)->hash)));
    return iterator(next);
  }
  constexpr size_type erase(const key_type& k) { return __erase_key(k); }
  template <class _Kp>
    requires __transparent && ::__ycxx::__detail::__not_iterator_arg<_Kp, iterator, const_iterator>
  constexpr size_type erase(_Kp&& __x) {
    return __erase_key(__x);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    if (first == last)
      return iterator(last.__n_);
    __node_base* const prev = __prev_of(first.__n_);
    while (prev->next != last.__n_)
      __free_node(__unlink_after(prev, __bucket_of(__as_node(prev->next)->hash)));
    return iterator(last.__n_);
  }
  constexpr void clear() noexcept {
    if (!__cells_)
      return;
    __node_base* p = __head()->next;
    while (p) {
      __node_base* const next = p->next;
      __free_node(__as_node(p));
      p = next;
    }
    for (size_type i = 0; i <= __bucket_count_; ++i)
      __cells_[i].next = nullptr;
    __size_ = 0;
  }

  // ---- observers ----
  constexpr hasher hash_function() const { return __hash_; }
  constexpr key_equal key_eq() const { return __pred_; }

  // ---- lookup ----
  constexpr iterator find(const key_type& k) { return iterator(__find_node(k)); }
  constexpr const_iterator find(const key_type& k) const { return const_iterator(__find_node(k)); }
  template <class _Kp>
    requires __transparent
  constexpr iterator find(const _Kp& k) {
    return iterator(__find_node(k));
  }
  template <class _Kp>
    requires __transparent
  constexpr const_iterator find(const _Kp& k) const {
    return const_iterator(__find_node(k));
  }
  constexpr size_type count(const key_type& k) const { return __count_key(k); }
  template <class _Kp>
    requires __transparent
  constexpr size_type count(const _Kp& k) const {
    return __count_key(k);
  }
  constexpr bool contains(const key_type& k) const { return __find_node(k) != nullptr; }
  template <class _Kp>
    requires __transparent
  constexpr bool contains(const _Kp& k) const {
    return __find_node(k) != nullptr;
  }
  constexpr std::pair<iterator, iterator> equal_range(const key_type& k) {
    auto [__f, __l] = __range_of(k);
    return {iterator(__f), iterator(__l)};
  }
  constexpr std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const {
    auto [__f, __l] = __range_of(k);
    return {const_iterator(__f), const_iterator(__l)};
  }
  template <class _Kp>
    requires __transparent
  constexpr std::pair<iterator, iterator> equal_range(const _Kp& k) {
    auto [__f, __l] = __range_of(k);
    return {iterator(__f), iterator(__l)};
  }
  template <class _Kp>
    requires __transparent
  constexpr std::pair<const_iterator, const_iterator> equal_range(const _Kp& k) const {
    auto [__f, __l] = __range_of(k);
    return {const_iterator(__f), const_iterator(__l)};
  }

  // ---- bucket interface ----
  constexpr size_type bucket_count() const noexcept { return __bucket_count_; }
  constexpr size_type max_bucket_count() const noexcept {
    const auto m = static_cast<std::size_t>(__cell_traits::max_size(__cell_alloc(__na_)) - 1);
    const auto d = static_cast<std::size_t>(std::numeric_limits<difference_type>::max());
    return static_cast<size_type>(std::bit_floor(m < d ? m : d));
  }
  constexpr size_type bucket_size(size_type n) const {
    ::__ycxx::__detail::__precondition(n < __bucket_count_, "unordered container bucket_size: bucket out of range");
    size_type c = 0;
    for (const_local_iterator i = begin(n), e = end(n); i != e; ++i)
      ++c;
    return c;
  }
  constexpr size_type bucket(const key_type& k) const { return __bucket_key(k); }
  template <class _Kp>
    requires __transparent
  constexpr size_type bucket(const _Kp& k) const {
    return __bucket_key(k);
  }
  constexpr local_iterator begin(size_type n) { return local_iterator(__bucket_first(n), n, __shift_); }
  constexpr const_local_iterator begin(size_type n) const { return const_local_iterator(__bucket_first(n), n, __shift_); }
  constexpr local_iterator end(size_type n) {
    ::__ycxx::__detail::__precondition(n < __bucket_count_, "unordered container end(n): bucket out of range");
    return local_iterator(nullptr, n, __shift_);
  }
  constexpr const_local_iterator end(size_type n) const {
    ::__ycxx::__detail::__precondition(n < __bucket_count_, "unordered container end(n): bucket out of range");
    return const_local_iterator(nullptr, n, __shift_);
  }
  constexpr const_local_iterator cbegin(size_type n) const { return begin(n); }
  constexpr const_local_iterator cend(size_type n) const { return end(n); }

  // ---- hash policy ----
  constexpr float load_factor() const noexcept {
    return __bucket_count_ == 0 ? 0.0f : static_cast<float>(__size_) / static_cast<float>(__bucket_count_);
  }
  constexpr float max_load_factor() const noexcept { return __mlf_; }
  constexpr void max_load_factor(float __z) {
    ::__ycxx::__detail::__precondition(__z > 0.0f, "unordered container max_load_factor: z must be positive");
    __mlf_ = __z;
  }
  constexpr void rehash(size_type n) {
    size_type c = __buckets_for(__size_);
    if (c < n)
      c = n;
    if (c == 0 && __size_ != 0)
      c = 1;
    __rehash_to(__round_buckets(c));
  }
  constexpr void reserve(size_type n) { rehash(__buckets_for(n)); }

protected:
  template <class _Kp>
  constexpr node_type __extract_key(const _Kp& k) {
    if (__size_ == 0)
      return node_type();
    const std::size_t h = __hash_of(k);
    __node_base* const prev = __find_prev(k, h);
    if (!prev)
      return node_type();
    return __nh_access::__make<node_type>(__unlink_after(prev, __bucket_of(h)), get_allocator());
  }
  template <class _Kp>
  constexpr size_type __erase_key(const _Kp& k) {
    if (__size_ == 0)
      return 0;
    const std::size_t h = __hash_of(k);
    __node_base* const prev = __find_prev(k, h);
    if (!prev)
      return 0;
    size_type n = 1;
    if constexpr (_Multi)
      n = __group_size(prev->next, k, h); // counted first: k may refer to an erased element
    erase_after(prev, n);
    return n;
  }
  template <class _Kp>
  constexpr size_type __count_key(const _Kp& k) const {
    if (__size_ == 0)
      return 0;
    const std::size_t h = __hash_of(k);
    __node_base* const prev = __find_prev(k, h);
    if (!prev)
      return 0;
    if constexpr (_Multi)
      return __group_size(prev->next, k, h);
    else
      return 1;
  }
  template <class _Kp>
  constexpr std::pair<__node_base*, __node_base*> __range_of(const _Kp& k) const {
    if (__size_ == 0)
      return {nullptr, nullptr};
    const std::size_t h = __hash_of(k);
    __node_base* const prev = __find_prev(k, h);
    if (!prev)
      return {nullptr, nullptr};
    if constexpr (_Multi)
      return {prev->next, __group_last(prev->next, k, h)->next};
    else
      return {prev->next, prev->next->next};
  }
  template <class _Kp>
  constexpr size_type __bucket_key(const _Kp& k) const {
    ::__ycxx::__detail::__precondition(__bucket_count_ > 0, "unordered container bucket: no buckets");
    return __bucket_of(__hash_of(k));
  }
  constexpr __node_base* __bucket_first(size_type n) const {
    ::__ycxx::__detail::__precondition(n < __bucket_count_, "unordered container begin(n): bucket out of range");
    __node_base* const prev = __cells_[n].next;
    return prev ? prev->next : nullptr;
  }
  template <class _Fp>
  constexpr size_type __erase_if_impl(_Fp& pred) {
    const size_type before = __size_;
    if (!__cells_)
      return 0;
    __node_base* prev = __head();
    while (__node_base* p = prev->next) {
      if (pred(*iterator(p)))
        __free_node(__unlink_after(prev, __bucket_of(__as_node(p)->hash)));
      else
        prev = p;
    }
    return before - __size_;
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// Lets the non-member operator== and erase_if use the table's internals.
struct __hash_table_access {
  template <class _Kp, class _Vp, class _Hp, class _Pp, class _Ap, bool _Mp>
  static constexpr bool equal(const ::__ycxx::__adl_free::__hash_table<_Kp, _Vp, _Hp, _Pp, _Ap, _Mp>& a,
                              const ::__ycxx::__adl_free::__hash_table<_Kp, _Vp, _Hp, _Pp, _Ap, _Mp>& b) {
    return a.__equal_to_table(b);
  }
  template <class _Kp, class _Vp, class _Hp, class _Pp, class _Ap, bool _Mp, class _Pred>
  static constexpr auto erase_if(::__ycxx::__adl_free::__hash_table<_Kp, _Vp, _Hp, _Pp, _Ap, _Mp>& c, _Pred& pred) {
    return c.__erase_if_impl(pred);
  }
};

}} // namespace __ycxx::__detail
