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
#include <ycxx/core/seq_support.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/utility_base.hpp>

namespace ycxx::detail {

struct hash_table_access;

struct hash_node_base {
  hash_node_base* next;
};

template <class V>
struct hash_node : hash_node_base {
  std::size_t hash;
  union {
    V value;
  };
  constexpr hash_node() noexcept : hash_node_base{nullptr}, hash(0) {}
  hash_node(const hash_node&) = delete;
  constexpr ~hash_node() {}
};

// The bucket of hash h in a table of 2^(digits - shift) buckets.
constexpr std::size_t hash_bucket_index(std::size_t h, int shift) noexcept {
  constexpr std::size_t golden =
      std::numeric_limits<std::size_t>::digits == 64 ? static_cast<std::size_t>(0x9e3779b97f4a7c15ull) : 0x9e3779b9u;
  return static_cast<std::size_t>(h * golden) >> shift;
}

template <class H, class P>
concept transparent_hash_pred = requires {
  typename H::is_transparent;
  typename P::is_transparent;
};

// A pair (possibly cv- or ref-qualified) whose first member type is Key, up to cv and ref.
template <class A, class Key>
concept pair_with_key = is_pair_v<std::remove_cvref_t<A>> &&
                        std::is_same_v<std::remove_cvref_t<typename std::remove_cvref_t<A>::first_type>, Key>;

// [unord.req.general]/245: K is not convertible to either iterator type.
template <class K, class It, class CIt>
concept not_iterator_arg = !std::is_convertible_v<K&&, It> && !std::is_convertible_v<K&&, CIt>;

// [associative.general]/2: the types the deduction guides deduce.
template <class I>
using unord_iter_key_t = std::remove_cvref_t<std::tuple_element_t<0, iter_value_type<I>>>;
template <class I>
using unord_iter_mapped_t = std::remove_cvref_t<std::tuple_element_t<1, iter_value_type<I>>>;
template <class I>
using unord_iter_alloc_t = std::pair<const unord_iter_key_t<I>, unord_iter_mapped_t<I>>;
template <class R>
using unord_range_key_t = std::remove_cvref_t<std::tuple_element_t<0, std::ranges::range_value_t<R>>>;
template <class R>
using unord_range_mapped_t = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<R>>>;
template <class R>
using unord_range_alloc_t = std::pair<const unord_range_key_t<R>, unord_range_mapped_t<R>>;

// [unord.req.general]/246: what a deduced Hash or Pred must not be.
template <class H>
concept unord_hash_arg = !std::is_integral_v<H> && !qualifies_as_allocator<H>;
template <class P>
concept unord_pred_arg = !qualifies_as_allocator<P>;

} // namespace ycxx::detail

namespace ycxx::adl_free {

template <class Key, class Value, class Hash, class Pred, class Alloc, bool Multi>
class hash_table;

// T is the element type, possibly const.
template <class V, class T, class Diff>
class hash_iter {
  using base = ::ycxx::detail::hash_node_base;
  base* n_ = nullptr;

  template <class, class, class>
  friend class hash_iter;
  template <class, class, class, class, class, bool>
  friend class hash_table;

  constexpr explicit hash_iter(base* n) noexcept : n_(n) {}

public:
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::forward_iterator_tag;
  using value_type = V;
  using difference_type = Diff;
  using pointer = T*;
  using reference = T&;

  constexpr hash_iter() noexcept = default;
  template <class U>
    requires std::is_same_v<const U, T> && (!std::is_same_v<U, T>)
  constexpr hash_iter(const hash_iter<V, U, Diff>& o) noexcept : n_(o.n_) {}

  constexpr reference operator*() const noexcept { return static_cast<::ycxx::detail::hash_node<V>*>(n_)->value; }
  constexpr pointer operator->() const noexcept {
    return __builtin_addressof(static_cast<::ycxx::detail::hash_node<V>*>(n_)->value);
  }
  constexpr hash_iter& operator++() noexcept {
    n_ = n_->next;
    return *this;
  }
  constexpr hash_iter operator++(int) noexcept {
    hash_iter t = *this;
    n_ = n_->next;
    return t;
  }
  friend constexpr bool operator==(const hash_iter& a, const hash_iter& b) noexcept { return a.n_ == b.n_; }
  // iterator == const_iterator without a conversion, so that no operator== of the value type
  // taking an arbitrary argument competes.
  template <class U>
    requires std::is_same_v<std::remove_const_t<U>, V> && (!std::is_same_v<U, T>)
  friend constexpr bool operator==(const hash_iter& a, const hash_iter<V, U, Diff>& b) noexcept {
    return a.n_ == hash_iter::node_of(b);
  }

private:
  template <class U>
  static constexpr base* node_of(const hash_iter<V, U, Diff>& i) noexcept {
    return i.n_;
  }
};

// Iterates one bucket: stops at the first node of another bucket.
template <class V, class T, class Diff>
class hash_local_iter {
  using base = ::ycxx::detail::hash_node_base;
  base* n_ = nullptr;
  std::size_t bucket_ = 0;
  int shift_ = 0;

  template <class, class, class>
  friend class hash_local_iter;
  template <class, class, class, class, class, bool>
  friend class hash_table;

  constexpr hash_local_iter(base* n, std::size_t b, int shift) noexcept : n_(n), bucket_(b), shift_(shift) {}

public:
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::forward_iterator_tag;
  using value_type = V;
  using difference_type = Diff;
  using pointer = T*;
  using reference = T&;

  constexpr hash_local_iter() noexcept = default;
  template <class U>
    requires std::is_same_v<const U, T> && (!std::is_same_v<U, T>)
  constexpr hash_local_iter(const hash_local_iter<V, U, Diff>& o) noexcept
      : n_(o.n_), bucket_(o.bucket_), shift_(o.shift_) {}

  constexpr reference operator*() const noexcept { return static_cast<::ycxx::detail::hash_node<V>*>(n_)->value; }
  constexpr pointer operator->() const noexcept {
    return __builtin_addressof(static_cast<::ycxx::detail::hash_node<V>*>(n_)->value);
  }
  constexpr hash_local_iter& operator++() noexcept {
    n_ = n_->next;
    if (n_ && ::ycxx::detail::hash_bucket_index(static_cast<::ycxx::detail::hash_node<V>*>(n_)->hash, shift_) !=
                  bucket_)
      n_ = nullptr;
    return *this;
  }
  constexpr hash_local_iter operator++(int) noexcept {
    hash_local_iter t = *this;
    ++*this;
    return t;
  }
  friend constexpr bool operator==(const hash_local_iter& a, const hash_local_iter& b) noexcept {
    return a.n_ == b.n_;
  }
  template <class U>
    requires std::is_same_v<std::remove_const_t<U>, V> && (!std::is_same_v<U, T>)
  friend constexpr bool operator==(const hash_local_iter& a, const hash_local_iter<V, U, Diff>& b) noexcept {
    return a.n_ == hash_local_iter::node_of(b);
  }

private:
  template <class U>
  static constexpr base* node_of(const hash_local_iter<V, U, Diff>& i) noexcept {
    return i.n_;
  }
};

// The common part of the four unordered containers. Value is Key for the sets and
// pair<const Key, T> for the maps; Multi selects equivalent keys. The public members are
// those of [unord.req]; the derived std:: classes add constructors, assignment, swap, merge and
// the map- or set-specific members.
template <class Key, class Value, class Hash, class Pred, class Alloc, bool Multi>
class hash_table {
  template <class, class, class, class, class, bool>
  friend class hash_table;
  friend struct ::ycxx::detail::hash_table_access;

  // Diagnosed preconditions: Hash meets Cpp17Hash ([unord.req.general]/3) and Pred
  // Cpp17CopyConstructible (/20).
  static_assert(std::is_copy_constructible_v<Hash>, "unordered container: Hash must be copy constructible");
  static_assert(std::is_copy_constructible_v<Pred>, "unordered container: Pred must be copy constructible");

protected:
  using info = ::ycxx::detail::alloc_info<Alloc>;
  static constexpr bool is_map = !std::is_same_v<Key, Value>;
  using node = ::ycxx::detail::hash_node<Value>;
  using node_base = ::ycxx::detail::hash_node_base;
  using node_alloc = typename info::template rebind<node>;
  using node_traits = std::allocator_traits<node_alloc>;
  using cell_alloc = typename info::template rebind<node_base>;
  using cell_traits = std::allocator_traits<cell_alloc>;
  using nh_access = ::ycxx::detail::node_handle_access;

public:
  using key_type = Key;
  using value_type = Value;
  using hasher = Hash;
  using key_equal = Pred;
  using allocator_type = Alloc;
  using pointer = typename info::pointer;
  using const_pointer = typename info::const_pointer;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = typename info::size_type;
  using difference_type = typename info::difference_type;
  using iterator = hash_iter<Value, std::conditional_t<is_map, Value, const Value>, difference_type>;
  using const_iterator = hash_iter<Value, const Value, difference_type>;
  using local_iterator = hash_local_iter<Value, std::conditional_t<is_map, Value, const Value>, difference_type>;
  using const_local_iterator = hash_local_iter<Value, const Value, difference_type>;
  using node_type = node_handle<::ycxx::detail::hash_node<Value>, Alloc, is_map>;

protected:
  using emplace_result = std::conditional_t<Multi, iterator, std::pair<iterator, bool>>;
  static constexpr bool transparent = ::ycxx::detail::transparent_hash_pred<Hash, Pred>;
  static constexpr bool distinct_iterators = !std::is_same_v<iterator, const_iterator>;

  node_base* cells_ = nullptr; // bucket_count_ bucket cells, then the before-begin node
  size_type bucket_count_ = 0;
  size_type size_ = 0;
  int shift_ = 0;
  float mlf_ = 1.0f;
  [[no_unique_address]] Hash hash_;
  [[no_unique_address]] Pred pred_;
  [[no_unique_address]] node_alloc na_;

  // ---- nodes ----
  static constexpr iterator to_iter(node_base* n) noexcept { return iterator(n); }
  static constexpr node* as_node(node_base* p) noexcept { return static_cast<node*>(p); }
  static constexpr const Key& key_of(const Value& v) noexcept {
    if constexpr (is_map)
      return v.first;
    else
      return v;
  }
  constexpr node_base* head() const noexcept { return cells_ + bucket_count_; }
  constexpr node_base* first() const noexcept { return cells_ ? cells_[bucket_count_].next : nullptr; }
  constexpr size_type bucket_of(std::size_t h) const noexcept {
    return static_cast<size_type>(::ycxx::detail::hash_bucket_index(h, shift_));
  }
  template <class K>
  constexpr std::size_t hash_of(const K& k) const {
    return static_cast<std::size_t>(hash_(k));
  }
  template <class K>
  constexpr bool equal(const K& k, const node* n) const {
    return static_cast<bool>(pred_(k, key_of(n->value)));
  }

  template <class... Args>
  constexpr node* make_node(Args&&... args) {
    node* n = std::to_address(node_traits::allocate(na_, 1));
    std::construct_at(n);
    ::ycxx::detail::rollback rb{[&] {
      std::destroy_at(n);
      node_traits::deallocate(na_, ::ycxx::detail::to_alloc_pointer<typename node_traits::pointer>(n), 1);
    }};
    node_traits::construct(na_, __builtin_addressof(n->value), static_cast<Args&&>(args)...);
    rb.release();
    return n;
  }
  constexpr void free_node(node* n) noexcept {
    node_traits::destroy(na_, __builtin_addressof(n->value));
    std::destroy_at(n);
    node_traits::deallocate(na_, ::ycxx::detail::to_alloc_pointer<typename node_traits::pointer>(n), 1);
  }
  // Frees a node unless released: the node of an insertion that may still fail.
  struct node_guard {
    hash_table* t;
    node* n;
    constexpr ~node_guard() {
      if (n)
        t->free_node(n);
    }
    constexpr node* release() noexcept {
      node* r = n;
      n = nullptr;
      return r;
    }
  };

  // ---- the bucket array ----
  constexpr node_base* alloc_cells(size_type count) {
    cell_alloc ca(na_);
    node_base* c = std::to_address(cell_traits::allocate(ca, count + 1));
    for (size_type i = 0; i <= count; ++i)
      std::construct_at(c + i, node_base{nullptr});
    return c;
  }
  constexpr void free_cells(node_base* c, size_type count) noexcept {
    if (!c)
      return;
    cell_alloc ca(na_); // the cells are trivially destructible
    cell_traits::deallocate(ca, ::ycxx::detail::to_alloc_pointer<typename cell_traits::pointer>(c), count + 1);
  }
  static constexpr int shift_for(size_type count) noexcept {
    return std::numeric_limits<std::size_t>::digits - std::countr_zero(static_cast<std::size_t>(count));
  }
  // The bucket count to use for at least n buckets: 0, or a power of two >= 2.
  constexpr size_type round_buckets(size_type n) const {
    if (n == 0)
      return 0;
    if (n > max_bucket_count())
      ::ycxx::detail::throw_length_error("unordered container: too many buckets");
    return n <= 2 ? size_type(2) : static_cast<size_type>(std::bit_ceil(static_cast<std::size_t>(n)));
  }
  // ceil(n / max_load_factor()).
  constexpr size_type buckets_for(size_type n) const {
    const float f = static_cast<float>(n) / mlf_;
    if (!(f < static_cast<float>(max_bucket_count())))
      ::ycxx::detail::throw_length_error("unordered container: too many buckets");
    size_type c = static_cast<size_type>(f);
    if (static_cast<float>(c) < f)
      ++c;
    return c;
  }

  // Relinks every node into a new array of count buckets (0, or a power of two >= 2; 0 only
  // when empty). Never calls the hash function; only the allocation can throw, before any
  // change.
  constexpr void rehash_to(size_type count) {
    if (count == bucket_count_)
      return;
    if (count == 0) {
      free_cells(cells_, bucket_count_);
      cells_ = nullptr;
      bucket_count_ = 0;
      shift_ = 0;
      return;
    }
    node_base* nc = alloc_cells(count);
    const int ns = shift_for(count);
    node_base* nh = nc + count;
    node_base* p = first();
    // Pass 1: append each node after the last node of its new bucket (or at the end of the
    // list for a new bucket); nc[b] temporarily holds the bucket's last node.
    node_base* tail = nh;
    while (p) {
      node_base* const next = p->next;
      const std::size_t b = ::ycxx::detail::hash_bucket_index(as_node(p)->hash, ns);
      node_base* const last = nc[b].next;
      if (last) {
        p->next = last->next;
        last->next = p;
        if (tail == last)
          tail = p;
      } else {
        p->next = nullptr;
        tail->next = p;
        tail = p;
      }
      nc[b].next = p;
      p = next;
    }
    // Pass 2: the buckets' runs are contiguous; each cell gets the node before its run.
    std::size_t prev_bucket = static_cast<std::size_t>(-1);
    for (node_base *prev = nh, *q = nh->next; q; prev = q, q = q->next) {
      const std::size_t b = ::ycxx::detail::hash_bucket_index(as_node(q)->hash, ns);
      if (b != prev_bucket) {
        nc[b].next = prev;
        prev_bucket = b;
      }
    }
    free_cells(cells_, bucket_count_);
    cells_ = nc;
    bucket_count_ = count;
    shift_ = ns;
  }
  // Makes room for n more elements without exceeding the maximum load factor.
  constexpr void grow_for(size_type n) {
    const size_type want = size_ + n;
    if (bucket_count_ != 0 && !(static_cast<float>(want) > static_cast<float>(bucket_count_) * mlf_))
      return;
    size_type c = buckets_for(want);
    const size_type twice = bucket_count_ * 2;
    if (c < twice)
      c = twice;
    if (c < 8)
      c = 8;
    rehash_to(round_buckets(c));
  }

  // ---- linking ----
  // Links n as the first node of bucket b.
  constexpr void link_front(node* n, size_type b) noexcept {
    node_base* const prev = cells_[b].next;
    if (prev) {
      n->next = prev->next;
      prev->next = n;
    } else {
      node_base* const h = head();
      n->next = h->next;
      h->next = n;
      cells_[b].next = h;
      if (n->next)
        cells_[bucket_of(as_node(n->next)->hash)].next = n;
    }
    ++size_;
  }
  // Links n after pos, a node of bucket b.
  constexpr void link_after(node_base* pos, node* n, size_type b) noexcept {
    n->next = pos->next;
    pos->next = n;
    if (n->next) {
      const size_type nb = bucket_of(as_node(n->next)->hash);
      if (nb != b)
        cells_[nb].next = n;
    }
    ++size_;
  }
  // Unlinks the node after prev, which is in bucket b.
  constexpr node* unlink_after(node_base* prev, size_type b) noexcept {
    node* const n = as_node(prev->next);
    node_base* const next = n->next;
    if (cells_[b].next == prev) { // n is the first node of its bucket
      if (!next) {
        cells_[b].next = nullptr;
      } else {
        const size_type nb = bucket_of(as_node(next)->hash);
        if (nb != b) { // n was the only node of its bucket
          cells_[nb].next = prev;
          cells_[b].next = nullptr;
        }
      }
    } else if (next) {
      const size_type nb = bucket_of(as_node(next)->hash);
      if (nb != b)
        cells_[nb].next = prev;
    }
    prev->next = next;
    --size_;
    return n;
  }
  // The node before n.
  constexpr node_base* prev_of(const node_base* n) const noexcept {
    node_base* p = cells_[bucket_of(as_node(const_cast<node_base*>(n))->hash)].next;
    while (p->next != n)
      p = p->next;
    return p;
  }

  // ---- search ----
  // The node before the first node with key equivalent to k (hash h), or null.
  template <class K>
  constexpr node_base* find_prev(const K& k, std::size_t h) const {
    if (size_ == 0)
      return nullptr;
    const size_type b = bucket_of(h);
    node_base* prev = cells_[b].next;
    if (!prev)
      return nullptr;
    for (node_base* p = prev->next; p; prev = p, p = p->next) {
      const node* const n = as_node(p);
      if (n->hash == h) {
        if (equal(k, n))
          return prev;
      } else if (bucket_of(n->hash) != b) {
        break;
      }
    }
    return nullptr;
  }
  template <class K>
  constexpr node_base* find_node(const K& k) const {
    if (size_ == 0)
      return nullptr;
    node_base* const prev = find_prev(k, hash_of(k));
    return prev ? prev->next : nullptr;
  }
  // The last node of the group starting at first (key k, hash h).
  template <class K>
  constexpr node_base* group_last(node_base* first, const K& k, std::size_t h) const {
    node_base* last = first;
    for (node_base* p = first->next; p && as_node(p)->hash == h && equal(k, as_node(p)); p = p->next)
      last = p;
    return last;
  }
  // The number of nodes of the group starting at first.
  template <class K>
  constexpr size_type group_size(node_base* first, const K& k, std::size_t h) const {
    size_type c = 1;
    for (node_base* p = first->next; p && as_node(p)->hash == h && equal(k, as_node(p)); p = p->next)
      ++c;
    return c;
  }

  // ---- insertion ----
  // Links the detached node n (hash h, already searched for: prev is the node before its
  // group, or null). Grows the table first; the only failure is that allocation.
  constexpr node* link_new(node* n, std::size_t h, node_base* group_prev) {
    node_base* last = nullptr;
    if constexpr (Multi) {
      if (group_prev)
        last = group_last(group_prev->next, key_of(n->value), h);
    }
    grow_for(1);
    n->hash = h;
    if (last)
      link_after(last, n, bucket_of(h));
    else
      link_front(n, bucket_of(h));
    return n;
  }
  constexpr emplace_result make_result(node_base* n, bool inserted) noexcept {
    if constexpr (Multi)
      return iterator(n);
    else
      return emplace_result(iterator(n), inserted);
  }
  // Inserts a constructed node (freed if it is not kept).
  constexpr emplace_result insert_node(node* n) {
    node_guard g{this, n};
    const std::size_t h = hash_of(key_of(n->value));
    node_base* const prev = find_prev(key_of(n->value), h);
    if constexpr (!Multi) {
      if (prev)
        return make_result(prev->next, false);
    }
    link_new(n, h, prev);
    return make_result(g.release(), true);
  }
  template <class... Args>
  constexpr emplace_result emplace_impl(Args&&... args) {
    if constexpr (!Multi && key_arg<Args...>)
      return emplace_key(arg_key(args...), static_cast<Args&&>(args)...);
    else
      return insert_node(make_node(static_cast<Args&&>(args)...));
  }
  // Unique keys: emplace arguments whose key can be read without building the element (a
  // value_type, a pair whose first member is a Key, or for maps a Key followed by the mapped
  // value's argument), so that an existing key costs no allocation.
  template <class... Args>
  static constexpr bool key_arg = false;
  template <class A>
  static constexpr bool key_arg<A> =
      std::is_same_v<std::remove_cvref_t<A>, Value> || (is_map && ::ycxx::detail::pair_with_key<A, Key>);
  template <class A, class B>
  static constexpr bool key_arg<A, B> = is_map && std::is_same_v<std::remove_cvref_t<A>, Key>;
  template <class A>
  static constexpr const Key& arg_key(const A& a) noexcept {
    if constexpr (std::is_same_v<std::remove_cvref_t<A>, Value>)
      return key_of(a);
    else
      return a.first;
  }
  template <class A, class B>
  static constexpr const Key& arg_key(const A& a, const B&) noexcept {
    return a;
  }
  // Unique keys: inserts a node built from args unless an element with key k exists; the
  // node is built only if needed ([unord.map.modifiers]/6, /15).
  template <class K, class... Args>
  constexpr std::pair<iterator, bool> emplace_key(const K& k, Args&&... args) {
    const std::size_t h = hash_of(k);
    if (node_base* const prev = find_prev(k, h))
      return {iterator(prev->next), false};
    node_guard g{this, make_node(static_cast<Args&&>(args)...)};
    link_new(g.n, h, nullptr);
    return {iterator(g.release()), true};
  }
  // Inserts a value; for unique keys, searches before copying or moving it.
  template <class V>
  constexpr emplace_result insert_value(V&& v) {
    if constexpr (Multi)
      return emplace_impl(static_cast<V&&>(v));
    else
      return emplace_key(key_of(v), static_cast<V&&>(v));
  }
  // Equivalent keys: the node at hint if it holds a key equivalent to k (hash h), else null.
  template <class K>
  constexpr node_base* hint_pos(const_iterator hint, const K& k, std::size_t h) const {
    node_base* const p = hint.n_;
    if (p && as_node(p)->hash == h && equal(k, as_node(p)))
      return p;
    return nullptr;
  }
  // Equivalent keys: links the detached node n right after the hint when the hint holds an
  // equivalent key, else at the end of its group.
  constexpr void link_hinted(const_iterator hint, node* n) {
    const std::size_t h = hash_of(key_of(n->value));
    if (node_base* const pos = hint_pos(hint, key_of(n->value), h)) {
      grow_for(1);
      n->hash = h;
      link_after(pos, n, bucket_of(h));
    } else {
      link_new(n, h, find_prev(key_of(n->value), h));
    }
  }
  constexpr iterator insert_node_hint(const_iterator hint, node* n) {
    node_guard g{this, n};
    link_hinted(hint, n);
    return iterator(g.release());
  }
  template <class It, class Sent>
  constexpr void insert_elems(It first, Sent last) {
    // Equivalent keys: every element is inserted, so the table can grow once. (Unique keys
    // cannot: a rehash for duplicates would invalidate iterators that [unord.req.general]/243
    // keeps valid.)
    if constexpr (Multi && ::ycxx::detail::multipass_iterator<It> && std::is_same_v<It, Sent>) {
      const auto n = ::ycxx::detail::iter_pair_distance(first, last);
      if (n > 0)
        grow_for(static_cast<size_type>(n));
    }
    for (; first != last; ++first)
      emplace_impl(*first);
  }

  // Copies (or, with move_values, moves) the nodes of o in order into this empty table with the
  // same bucket count and the same hashes, without calling the hash function or predicate.
  template <bool move_values, class O>
  constexpr void clone_from(O& o) {
    if (o.size_ == 0)
      return;
    rehash_to(o.bucket_count_);
    node_base* tail = head();
    std::size_t prev_bucket = static_cast<std::size_t>(-1);
    for (node_base* p = o.first(); p; p = p->next) {
      node* const src = as_node(p);
      node* n;
      if constexpr (move_values)
        n = make_node(static_cast<Value&&>(src->value));
      else
        n = make_node(static_cast<const Value&>(src->value));
      n->hash = src->hash;
      n->next = nullptr;
      tail->next = n;
      const size_type b = bucket_of(n->hash);
      if (b != prev_bucket) {
        cells_[b].next = tail;
        prev_bucket = b;
      }
      tail = n;
      ++size_;
    }
  }
  // Takes o's nodes and bucket array.
  constexpr void steal(hash_table& o) noexcept {
    cells_ = o.cells_;
    bucket_count_ = o.bucket_count_;
    size_ = o.size_;
    shift_ = o.shift_;
    o.cells_ = nullptr;
    o.bucket_count_ = 0;
    o.size_ = 0;
    o.shift_ = 0;
  }
  constexpr void free_all() noexcept {
    clear();
    free_cells(cells_, bucket_count_);
    cells_ = nullptr;
    bucket_count_ = 0;
    shift_ = 0;
  }

  // ---- construction, assignment, swap (used by the derived classes) ----
  static constexpr bool nothrow_default = std::is_nothrow_default_constructible_v<Hash> &&
                                          std::is_nothrow_default_constructible_v<Pred> &&
                                          std::is_nothrow_default_constructible_v<Alloc>;
  // The move constructors copy the hash function and predicate, so that the moved-from
  // container keeps working ones.
  static constexpr bool nothrow_move =
      std::is_nothrow_copy_constructible_v<Hash> && std::is_nothrow_copy_constructible_v<Pred>;
  constexpr hash_table() noexcept(nothrow_default) : hash_(), pred_(), na_(Alloc()) {}
  constexpr hash_table(size_type n, const Hash& hf, const Pred& eql, const Alloc& a)
      : hash_(hf), pred_(eql), na_(a) {
    // No node is touched here, so a container of an incomplete class type can be a member
    // of that class (as with the other node-based containers).
    if (n > 0) {
      const size_type c = round_buckets(n);
      cells_ = alloc_cells(c);
      bucket_count_ = c;
      shift_ = shift_for(c);
    }
  }
  constexpr hash_table(const hash_table& o, const Alloc& a)
      : mlf_(o.mlf_), hash_(o.hash_), pred_(o.pred_), na_(a) {
    ::ycxx::detail::rollback rb{[this] { free_all(); }};
    clone_from<false>(o);
    rb.release();
  }
  constexpr hash_table(const hash_table& o)
      : hash_table(o, std::allocator_traits<Alloc>::select_on_container_copy_construction(Alloc(o.na_))) {}
  constexpr hash_table(hash_table&& o) noexcept(nothrow_move)
      : mlf_(o.mlf_), hash_(o.hash_), pred_(o.pred_),
        na_(static_cast<node_alloc&&>(o.na_)) {
    steal(o);
  }
  constexpr hash_table(hash_table&& o, const Alloc& a) noexcept(info::always_equal && nothrow_move)
      : mlf_(o.mlf_), hash_(o.hash_), pred_(o.pred_), na_(a) {
    if (info::always_equal || na_ == o.na_) {
      steal(o);
    } else {
      ::ycxx::detail::rollback rb{[this] { free_all(); }};
      clone_from<true>(o);
      rb.release();
      o.free_all();
    }
  }
  constexpr ~hash_table() { free_all(); }

  constexpr void copy_assign(const hash_table& o) {
    if (this == __builtin_addressof(o))
      return;
    free_all();
    if constexpr (info::pocca)
      na_ = o.na_;
    hash_ = o.hash_;
    pred_ = o.pred_;
    mlf_ = o.mlf_;
    clone_from<false>(o);
  }
  constexpr void move_assign(hash_table& o) {
    if (this == __builtin_addressof(o))
      return;
    free_all();
    hash_ = static_cast<Hash&&>(o.hash_);
    pred_ = static_cast<Pred&&>(o.pred_);
    mlf_ = o.mlf_;
    if constexpr (info::pocma) {
      na_ = static_cast<node_alloc&&>(o.na_);
      steal(o);
    } else if (info::always_equal || na_ == o.na_) {
      steal(o);
    } else {
      clone_from<true>(o);
      o.free_all();
    }
  }
  constexpr void swap_impl(hash_table& o) {
    if (this == __builtin_addressof(o))
      return;
    if constexpr (info::pocs)
      ::ycxx::detail::swap_adl::do_swap(na_, o.na_);
    else
      ::ycxx::detail::precondition(info::always_equal || na_ == o.na_,
                                   "unordered container swap: unequal allocators that do not propagate");
    ::ycxx::detail::swap_adl::do_swap(hash_, o.hash_);
    ::ycxx::detail::swap_adl::do_swap(pred_, o.pred_);
    node_base* const c = cells_;
    cells_ = o.cells_;
    o.cells_ = c;
    const size_type bc = bucket_count_;
    bucket_count_ = o.bucket_count_;
    o.bucket_count_ = bc;
    const size_type s = size_;
    size_ = o.size_;
    o.size_ = s;
    const int sh = shift_;
    shift_ = o.shift_;
    o.shift_ = sh;
    const float m = mlf_;
    mlf_ = o.mlf_;
    o.mlf_ = m;
  }
  constexpr void assign_il(std::initializer_list<Value> il) {
    clear();
    insert_elems(il.begin(), il.end());
  }

  // Moves the nodes of src into *this ([unord.req.general]/145-146).
  template <class K2, class H2, class P2, bool M2>
  constexpr void merge_from(hash_table<K2, Value, H2, P2, Alloc, M2>& src) {
    if (static_cast<void*>(this) == static_cast<void*>(__builtin_addressof(src)))
      return;
    if constexpr (!info::always_equal)
      ::ycxx::detail::precondition(na_ == src.na_, "unordered container merge: unequal allocators");
    node_base* prev = src.first() ? src.head() : nullptr;
    while (prev && prev->next) {
      node* const n = as_node(prev->next);
      const std::size_t h = hash_of(key_of(n->value));
      node_base* const gp = find_prev(key_of(n->value), h);
      if constexpr (!Multi) {
        if (gp) {
          prev = n;
          continue;
        }
      }
      node_base* last = nullptr;
      if constexpr (Multi) {
        if (gp)
          last = group_last(gp->next, key_of(n->value), h);
      }
      grow_for(1);
      src.unlink_after(prev, src.bucket_of(n->hash));
      n->hash = h;
      if (last)
        link_after(last, n, bucket_of(h));
      else
        link_front(n, bucket_of(h));
    }
  }

  // Erases the count nodes after prev.
  constexpr void erase_after(node_base* prev, size_type count) noexcept {
    for (; count > 0; --count)
      free_node(unlink_after(prev, bucket_of(as_node(prev->next)->hash)));
  }

  // ---- [unord.req] equality ----
  constexpr bool equal_to_table(const hash_table& o) const {
    if (size_ != o.size_)
      return false;
    for (node_base* p = first(); p;) {
      const Key& k = key_of(as_node(p)->value);
      node_base* const op = o.find_node(k);
      if (!op)
        return false;
      if constexpr (!Multi) {
        if (!(as_node(p)->value == as_node(op)->value))
          return false;
        p = p->next;
      } else {
        // the group [p, pe) here and [op, oe) there
        node_base* pe = p->next;
        size_type n = 1;
        const std::size_t h = as_node(p)->hash;
        while (pe && as_node(pe)->hash == h && equal(k, as_node(pe))) {
          pe = pe->next;
          ++n;
        }
        node_base* oe = op->next;
        size_type m = 1;
        const std::size_t oh = as_node(op)->hash;
        while (oe && as_node(oe)->hash == oh && o.equal(k, as_node(oe))) {
          oe = oe->next;
          ++m;
        }
        if (n != m || !std::is_permutation(const_iterator(p), const_iterator(pe), const_iterator(op), const_iterator(oe)))
          return false;
        p = pe;
      }
    }
    return true;
  }

public:
  constexpr allocator_type get_allocator() const noexcept { return allocator_type(na_); }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(first()); }
  constexpr const_iterator begin() const noexcept { return const_iterator(first()); }
  constexpr iterator end() noexcept { return iterator(nullptr); }
  constexpr const_iterator end() const noexcept { return const_iterator(nullptr); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }

  // ---- capacity ----
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type max_size() const noexcept {
    const auto a = static_cast<size_type>(node_traits::max_size(na_));
    const auto d = static_cast<size_type>(std::numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }

  // ---- modifiers ----
  template <class... Args>
  constexpr emplace_result emplace(Args&&... args) {
    return emplace_impl(static_cast<Args&&>(args)...);
  }
  template <class... Args>
  constexpr iterator emplace_hint(const_iterator hint, Args&&... args) {
    if constexpr (Multi)
      return insert_node_hint(hint, make_node(static_cast<Args&&>(args)...));
    else
      return emplace_impl(static_cast<Args&&>(args)...).first;
  }
  constexpr emplace_result insert(const value_type& obj) { return insert_value(obj); }
  constexpr emplace_result insert(value_type&& obj) { return insert_value(static_cast<value_type&&>(obj)); }
  constexpr iterator insert(const_iterator hint, const value_type& obj) {
    if constexpr (Multi)
      return insert_node_hint(hint, make_node(obj));
    else
      return insert_value(obj).first;
  }
  constexpr iterator insert(const_iterator hint, value_type&& obj) {
    if constexpr (Multi)
      return insert_node_hint(hint, make_node(static_cast<value_type&&>(obj)));
    else
      return insert_value(static_cast<value_type&&>(obj)).first;
  }
  template <class InputIterator>
  constexpr void insert(InputIterator first, InputIterator last) {
    insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <::ycxx::detail::container_compatible_range<value_type> R>
  constexpr void insert_range(R&& rg) {
    insert_elems(std::ranges::begin(rg), std::ranges::end(rg));
  }
  constexpr void insert(std::initializer_list<value_type> il) { insert_elems(il.begin(), il.end()); }

  constexpr node_type extract(const_iterator position) {
    ::ycxx::detail::precondition(position.n_ != nullptr, "unordered container extract: end() iterator");
    node* const n = unlink_after(prev_of(position.n_), bucket_of(as_node(position.n_)->hash));
    return nh_access::make<node_type>(n, get_allocator());
  }
  constexpr node_type extract(const key_type& x) { return extract_key(x); }
  template <class K>
    requires transparent && ::ycxx::detail::not_iterator_arg<K, iterator, const_iterator>
  constexpr node_type extract(K&& x) {
    return extract_key(x);
  }
  constexpr std::conditional_t<Multi, iterator, insert_return_type<iterator, node_type>> insert(node_type&& nh) {
    if (nh.empty()) {
      if constexpr (Multi)
        return end();
      else
        return {end(), false, node_type()};
    }
    ::ycxx::detail::precondition(info::always_equal || nh.get_allocator() == get_allocator(),
                                 "unordered container insert(node_type&&): unequal allocators");
    node* const n = nh_access::peek(nh);
    const std::size_t h = hash_of(key_of(n->value));
    node_base* const prev = find_prev(key_of(n->value), h);
    if constexpr (!Multi) {
      if (prev)
        return {iterator(prev->next), false, static_cast<node_type&&>(nh)};
    }
    link_new(n, h, prev);
    nh_access::take(nh);
    if constexpr (Multi)
      return iterator(n);
    else
      return {iterator(n), true, node_type()};
  }
  constexpr iterator insert(const_iterator hint, node_type&& nh) {
    if (nh.empty())
      return end();
    ::ycxx::detail::precondition(info::always_equal || nh.get_allocator() == get_allocator(),
                                 "unordered container insert(node_type&&): unequal allocators");
    node* const n = nh_access::peek(nh);
    if constexpr (Multi) {
      link_hinted(hint, n);
      nh_access::take(nh);
      return iterator(n);
    }
    const std::size_t h = hash_of(key_of(n->value));
    node_base* const prev = find_prev(key_of(n->value), h);
    if constexpr (!Multi) {
      if (prev)
        return iterator(prev->next);
    }
    link_new(n, h, prev);
    nh_access::take(nh);
    return iterator(n);
  }

  constexpr iterator erase(iterator position)
    requires distinct_iterators
  {
    return erase(const_iterator(position));
  }
  constexpr iterator erase(const_iterator position) {
    ::ycxx::detail::precondition(position.n_ != nullptr, "unordered container erase: end() iterator");
    node_base* const n = position.n_;
    node_base* const next = n->next;
    free_node(unlink_after(prev_of(n), bucket_of(as_node(n)->hash)));
    return iterator(next);
  }
  constexpr size_type erase(const key_type& k) { return erase_key(k); }
  template <class K>
    requires transparent && ::ycxx::detail::not_iterator_arg<K, iterator, const_iterator>
  constexpr size_type erase(K&& x) {
    return erase_key(x);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    if (first == last)
      return iterator(last.n_);
    node_base* const prev = prev_of(first.n_);
    while (prev->next != last.n_)
      free_node(unlink_after(prev, bucket_of(as_node(prev->next)->hash)));
    return iterator(last.n_);
  }
  constexpr void clear() noexcept {
    if (!cells_)
      return;
    node_base* p = head()->next;
    while (p) {
      node_base* const next = p->next;
      free_node(as_node(p));
      p = next;
    }
    for (size_type i = 0; i <= bucket_count_; ++i)
      cells_[i].next = nullptr;
    size_ = 0;
  }

  // ---- observers ----
  constexpr hasher hash_function() const { return hash_; }
  constexpr key_equal key_eq() const { return pred_; }

  // ---- lookup ----
  constexpr iterator find(const key_type& k) { return iterator(find_node(k)); }
  constexpr const_iterator find(const key_type& k) const { return const_iterator(find_node(k)); }
  template <class K>
    requires transparent
  constexpr iterator find(const K& k) {
    return iterator(find_node(k));
  }
  template <class K>
    requires transparent
  constexpr const_iterator find(const K& k) const {
    return const_iterator(find_node(k));
  }
  constexpr size_type count(const key_type& k) const { return count_key(k); }
  template <class K>
    requires transparent
  constexpr size_type count(const K& k) const {
    return count_key(k);
  }
  constexpr bool contains(const key_type& k) const { return find_node(k) != nullptr; }
  template <class K>
    requires transparent
  constexpr bool contains(const K& k) const {
    return find_node(k) != nullptr;
  }
  constexpr std::pair<iterator, iterator> equal_range(const key_type& k) {
    auto [f, l] = range_of(k);
    return {iterator(f), iterator(l)};
  }
  constexpr std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const {
    auto [f, l] = range_of(k);
    return {const_iterator(f), const_iterator(l)};
  }
  template <class K>
    requires transparent
  constexpr std::pair<iterator, iterator> equal_range(const K& k) {
    auto [f, l] = range_of(k);
    return {iterator(f), iterator(l)};
  }
  template <class K>
    requires transparent
  constexpr std::pair<const_iterator, const_iterator> equal_range(const K& k) const {
    auto [f, l] = range_of(k);
    return {const_iterator(f), const_iterator(l)};
  }

  // ---- bucket interface ----
  constexpr size_type bucket_count() const noexcept { return bucket_count_; }
  constexpr size_type max_bucket_count() const noexcept {
    const auto m = static_cast<std::size_t>(cell_traits::max_size(cell_alloc(na_)) - 1);
    const auto d = static_cast<std::size_t>(std::numeric_limits<difference_type>::max());
    return static_cast<size_type>(std::bit_floor(m < d ? m : d));
  }
  constexpr size_type bucket_size(size_type n) const {
    ::ycxx::detail::precondition(n < bucket_count_, "unordered container bucket_size: bucket out of range");
    size_type c = 0;
    for (const_local_iterator i = begin(n), e = end(n); i != e; ++i)
      ++c;
    return c;
  }
  constexpr size_type bucket(const key_type& k) const { return bucket_key(k); }
  template <class K>
    requires transparent
  constexpr size_type bucket(const K& k) const {
    return bucket_key(k);
  }
  constexpr local_iterator begin(size_type n) { return local_iterator(bucket_first(n), n, shift_); }
  constexpr const_local_iterator begin(size_type n) const { return const_local_iterator(bucket_first(n), n, shift_); }
  constexpr local_iterator end(size_type n) {
    ::ycxx::detail::precondition(n < bucket_count_, "unordered container end(n): bucket out of range");
    return local_iterator(nullptr, n, shift_);
  }
  constexpr const_local_iterator end(size_type n) const {
    ::ycxx::detail::precondition(n < bucket_count_, "unordered container end(n): bucket out of range");
    return const_local_iterator(nullptr, n, shift_);
  }
  constexpr const_local_iterator cbegin(size_type n) const { return begin(n); }
  constexpr const_local_iterator cend(size_type n) const { return end(n); }

  // ---- hash policy ----
  constexpr float load_factor() const noexcept {
    return bucket_count_ == 0 ? 0.0f : static_cast<float>(size_) / static_cast<float>(bucket_count_);
  }
  constexpr float max_load_factor() const noexcept { return mlf_; }
  constexpr void max_load_factor(float z) {
    ::ycxx::detail::precondition(z > 0.0f, "unordered container max_load_factor: z must be positive");
    mlf_ = z;
  }
  constexpr void rehash(size_type n) {
    size_type c = buckets_for(size_);
    if (c < n)
      c = n;
    if (c == 0 && size_ != 0)
      c = 1;
    rehash_to(round_buckets(c));
  }
  constexpr void reserve(size_type n) { rehash(buckets_for(n)); }

protected:
  template <class K>
  constexpr node_type extract_key(const K& k) {
    if (size_ == 0)
      return node_type();
    const std::size_t h = hash_of(k);
    node_base* const prev = find_prev(k, h);
    if (!prev)
      return node_type();
    return nh_access::make<node_type>(unlink_after(prev, bucket_of(h)), get_allocator());
  }
  template <class K>
  constexpr size_type erase_key(const K& k) {
    if (size_ == 0)
      return 0;
    const std::size_t h = hash_of(k);
    node_base* const prev = find_prev(k, h);
    if (!prev)
      return 0;
    size_type n = 1;
    if constexpr (Multi)
      n = group_size(prev->next, k, h); // counted first: k may refer to an erased element
    erase_after(prev, n);
    return n;
  }
  template <class K>
  constexpr size_type count_key(const K& k) const {
    if (size_ == 0)
      return 0;
    const std::size_t h = hash_of(k);
    node_base* const prev = find_prev(k, h);
    if (!prev)
      return 0;
    if constexpr (Multi)
      return group_size(prev->next, k, h);
    else
      return 1;
  }
  template <class K>
  constexpr std::pair<node_base*, node_base*> range_of(const K& k) const {
    if (size_ == 0)
      return {nullptr, nullptr};
    const std::size_t h = hash_of(k);
    node_base* const prev = find_prev(k, h);
    if (!prev)
      return {nullptr, nullptr};
    if constexpr (Multi)
      return {prev->next, group_last(prev->next, k, h)->next};
    else
      return {prev->next, prev->next->next};
  }
  template <class K>
  constexpr size_type bucket_key(const K& k) const {
    ::ycxx::detail::precondition(bucket_count_ > 0, "unordered container bucket: no buckets");
    return bucket_of(hash_of(k));
  }
  constexpr node_base* bucket_first(size_type n) const {
    ::ycxx::detail::precondition(n < bucket_count_, "unordered container begin(n): bucket out of range");
    node_base* const prev = cells_[n].next;
    return prev ? prev->next : nullptr;
  }
  template <class F>
  constexpr size_type erase_if_impl(F& pred) {
    const size_type before = size_;
    if (!cells_)
      return 0;
    node_base* prev = head();
    while (node_base* p = prev->next) {
      if (pred(*iterator(p)))
        free_node(unlink_after(prev, bucket_of(as_node(p)->hash)));
      else
        prev = p;
    }
    return before - size_;
  }
};

} // namespace ycxx::adl_free

namespace ycxx::detail {

// Lets the non-member operator== and erase_if use the table's internals.
struct hash_table_access {
  template <class K, class V, class H, class P, class A, bool M>
  static constexpr bool equal(const ::ycxx::adl_free::hash_table<K, V, H, P, A, M>& a,
                              const ::ycxx::adl_free::hash_table<K, V, H, P, A, M>& b) {
    return a.equal_to_table(b);
  }
  template <class K, class V, class H, class P, class A, bool M, class Pred>
  static constexpr auto erase_if(::ycxx::adl_free::hash_table<K, V, H, P, A, M>& c, Pred& pred) {
    return c.erase_if_impl(pred);
  }
};

} // namespace ycxx::detail
