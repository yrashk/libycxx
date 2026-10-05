// libycxx core: vector ([vector]) for element types other than bool, its non-member functions
// ([vector.syn], [vector.erasure]) and pmr::vector. vector<bool> is in vector_bool.hpp.
//
// Representation: three T* pointers {first_, last_, cap_} into one allocation obtained with
// allocate_at_least (all null while nothing is allocated), plus the allocator. Elements are
// constructed and destroyed only through allocator_traits ([container.alloc.reqmts]/2). At run
// time a trivially copyable T whose allocator does not customize construct/destroy is copied
// and relocated with memcpy.
//
// Exception safety ([vector.modifiers]/2, [vector.capacity]): a reallocation builds the new
// elements in the new storage first (so arguments that refer to old elements stay valid), then
// relocates the old elements by move_if_noexcept semantics; until the new storage is adopted,
// guards undo everything, so the old elements and storage are untouched on failure. Inserting
// several elements in the middle without reallocation constructs them at the end and rotates
// them into place with move assignments (the temporary is constructed through the allocator).
#pragma once

#include <initializer_list>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[gnu::visibility("hidden")]] std {

template <class T, class Allocator = allocator<T>>
class vector;

template <class T, class Allocator>
class vector {
  static_assert(is_same_v<typename Allocator::value_type, T>,
                "std::vector: Allocator::value_type must be T ([container.alloc.reqmts]/5)");

  using alloc_traits = allocator_traits<Allocator>;

public:
  // ---- types ----
  using value_type = T;
  using allocator_type = Allocator;
  using pointer = typename alloc_traits::pointer;
  using const_pointer = typename alloc_traits::const_pointer;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = typename alloc_traits::size_type;
  using difference_type = typename alloc_traits::difference_type;
  using iterator = ycxx::adl_free::contiguous_iter<T, vector, difference_type>;
  using const_iterator = ycxx::adl_free::contiguous_iter<const T, vector, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  static constexpr bool pocca = alloc_traits::propagate_on_container_copy_assignment::value;
  static constexpr bool pocma = alloc_traits::propagate_on_container_move_assignment::value;
  static constexpr bool pocs = alloc_traits::propagate_on_container_swap::value;
  static constexpr bool always_equal = alloc_traits::is_always_equal::value;

  T* first_ = nullptr;
  T* last_ = nullptr;
  T* cap_ = nullptr;
  [[no_unique_address]] Allocator alloc_;

  // ---- element policies (functions, so that T need only be complete when they are used) ----
  // Elements may be copied as bytes at run time.
  static consteval bool bitwise() {
    return is_trivially_copyable_v<T> && !requires(Allocator& a, T* p) { a.destroy(p); } &&
           !requires(Allocator& a, T* p, const T& x) { a.construct(p, x); } &&
           !requires(Allocator& a, T* p, T&& x) { a.construct(p, static_cast<T&&>(x)); };
  }
  // Relocation moves unless the move may throw and a copy is possible (move_if_noexcept).
  static consteval bool relocate_by_move() { return is_nothrow_move_constructible_v<T> || !is_copy_constructible_v<T>; }
  // [first, last) can be memcpy'd into the storage: besides T being trivially copyable, the
  // constructor and assignment that *i selects must be trivial (a template taking T& can be
  // chosen over the trivial copy operations; [sequence.reqmts] constructs from *i).
  template <class It, class Sent>
  static consteval bool bitwise_source() {
    if constexpr (is_same_v<It, Sent> && ycxx::detail::is_plain_contiguous<It>)
      return bitwise() && is_same_v<remove_cvref_t<iter_reference_t<It>>, T> &&
             is_lvalue_reference_v<iter_reference_t<It>> && !is_volatile_v<remove_reference_t<iter_reference_t<It>>> &&
             is_trivially_constructible_v<T, iter_reference_t<It>> && is_trivially_assignable_v<T&, iter_reference_t<It>>;
    else
      return false;
  }

  // ---- storage ----
  struct block {
    T* p;
    size_type cap;
  };
  static constexpr pointer to_pointer(T* p) noexcept {
    if constexpr (is_same_v<pointer, T*>)
      return p;
    else
      return pointer_traits<pointer>::pointer_to(*p);
  }
  constexpr block allocate_block(size_type n) {
    auto r = alloc_traits::allocate_at_least(alloc_, n);
    return {std::to_address(r.ptr), static_cast<size_type>(r.count)};
  }
  // Returns a block to the allocator on scope exit unless released (p == nullptr).
  struct block_guard {
    Allocator& a;
    block b;
    constexpr ~block_guard() {
      if (b.p)
        alloc_traits::deallocate(a, vector::to_pointer(b.p), b.cap);
    }
    constexpr block release() noexcept {
      block r = b;
      b.p = nullptr;
      return r;
    }
  };
  // Destroys [first, last) through the allocator on scope exit; released by first = last.
  struct destroy_guard {
    Allocator& a;
    T* first;
    T* last;
    constexpr ~destroy_guard() {
      for (; first != last; ++first)
        alloc_traits::destroy(a, first);
    }
  };

  constexpr void destroy_range(T* f, T* l) noexcept {
    for (; f != l; ++f)
      alloc_traits::destroy(alloc_, f);
  }
  // Destroys the elements and frees the storage; *this then owns nothing.
  constexpr void release_storage() noexcept {
    if (first_) {
      destroy_range(first_, last_);
      alloc_traits::deallocate(alloc_, to_pointer(first_), static_cast<size_type>(cap_ - first_));
      first_ = last_ = cap_ = nullptr;
    }
  }
  constexpr void adopt(block b, T* l) noexcept {
    first_ = b.p;
    last_ = l;
    cap_ = b.p + b.cap;
  }
  constexpr void take(vector& o) noexcept {
    first_ = o.first_;
    last_ = o.last_;
    cap_ = o.cap_;
    o.first_ = o.last_ = o.cap_ = nullptr;
  }
  constexpr size_type spare() const noexcept { return static_cast<size_type>(cap_ - last_); }
  constexpr T* mut(const_iterator p) const noexcept { return const_cast<T*>(p.base()); }

  constexpr void check_size(size_type n) const {
    if (n > max_size())
      ycxx::detail::throw_length_error("std::vector: size exceeds max_size()");
  }
  // Capacity for size() + extra elements (geometric growth).
  constexpr size_type grow_to(size_type extra) const {
    const size_type ms = max_size();
    const size_type sz = size();
    if (extra > ms - sz)
      ycxx::detail::throw_length_error("std::vector: size exceeds max_size()");
    const size_type c = capacity();
    const size_type nc = c <= ms / 2 ? 2 * c : ms;
    return nc < sz + extra ? sz + extra : nc;
  }

  // ---- constructing elements in raw storage; each either completes or constructs nothing ----
  // n elements at dst from [first, last) (or from first, n times, when Sent is unreachable).
  template <class It, class Sent>
  constexpr T* construct_from(T* dst, It first, Sent last, size_type n) {
    if constexpr (bitwise_source<It, Sent>()) {
      if !consteval {
        if (n != 0)
          __builtin_memcpy(static_cast<void*>(dst), static_cast<const void*>(ycxx::detail::plain_address(first)),
                           n * sizeof(T));
        return dst + n;
      }
    }
    (void)last;
    destroy_guard g{alloc_, dst, dst};
    for (size_type i = 0; i != n; ++i) {
      alloc_traits::construct(alloc_, g.last, *first);
      ++g.last;
      ++first;
    }
    g.first = g.last;
    return g.last;
  }
  // n elements at dst, each constructed from args (none: value-initialized; one: a copy).
  template <class... Args>
  constexpr T* construct_n(T* dst, size_type n, const Args&... args) {
    destroy_guard g{alloc_, dst, dst};
    for (size_type i = 0; i != n; ++i) {
      alloc_traits::construct(alloc_, g.last, args...);
      ++g.last;
    }
    g.first = g.last;
    return g.last;
  }
  // Relocates [f, l) into dst by move_if_noexcept (or memcpy); the sources stay to be destroyed.
  constexpr T* relocate(T* f, T* l, T* dst) {
    if constexpr (bitwise()) {
      if !consteval {
        const size_type n = static_cast<size_type>(l - f);
        if (n != 0)
          __builtin_memcpy(static_cast<void*>(dst), static_cast<const void*>(f), n * sizeof(T));
        return dst + n;
      }
    }
    destroy_guard g{alloc_, dst, dst};
    for (; f != l; ++f) {
      if constexpr (relocate_by_move())
        alloc_traits::construct(alloc_, g.last, static_cast<T&&>(*f));
      else
        alloc_traits::construct(alloc_, g.last, static_cast<const T&>(*f));
      ++g.last;
    }
    g.first = g.last;
    return g.last;
  }

  // Moves the elements to new storage of capacity >= new_cap, where make(p) constructs n new
  // elements at p (index off) first. Strong guarantee unless a throwing move of a
  // non-copyable T is used.
  template <class Make>
  constexpr void realloc_insert(size_type off, size_type n, size_type new_cap, Make&& make) {
    block_guard bg{alloc_, allocate_block(new_cap)};
    T* const np = bg.b.p;
    static_cast<Make&&>(make)(np + off);
    destroy_guard g{alloc_, np + off, np + off + n};
    relocate(first_, first_ + off, np);
    g.first = np;
    T* const nl = relocate(first_ + off, last_, np + off + n);
    g.first = g.last;
    const block b = bg.release();
    release_storage();
    adopt(b, nl);
  }

  // Fresh storage for exactly the n elements of [first, last); *this owns nothing yet.
  template <class It, class Sent>
  constexpr void init_counted(It first, Sent last, size_type n) {
    if (n == 0)
      return;
    check_size(n);
    block_guard bg{alloc_, allocate_block(n)};
    T* const e = construct_from(bg.b.p, static_cast<It&&>(first), static_cast<Sent&&>(last), n);
    adopt(bg.release(), e);
  }
  template <class... Args>
  constexpr void init_n(size_type n, const Args&... args) {
    if (n == 0)
      return;
    check_size(n);
    block_guard bg{alloc_, allocate_block(n)};
    T* const e = construct_n(bg.b.p, n, args...);
    adopt(bg.release(), e);
  }
  // Appends the elements of a single-pass sequence; on an exception the appended elements are
  // removed again.
  template <class It, class Sent>
  constexpr void append_input(It first, Sent last) {
    struct rollback {
      vector& v;
      size_type n;
      constexpr ~rollback() {
        if (n != size_type(-1)) {
          v.destroy_range(v.first_ + n, v.last_);
          v.last_ = v.first_ + n;
        }
      }
    } g{*this, size()};
    for (; first != last; ++first) {
      if (last_ == cap_) {
        // Growing frees the storage the rest of the range may refer to: append_range, unlike
        // insert_range, has no precondition that rg does not overlap *this ([sequence.reqmts]).
        // Collect the rest first, then move it in.
        vector rest(alloc_);
        for (; first != last; ++first)
          rest.emplace_back(*first);
        reserve(grow_to(rest.size()));
        for (T& x : rest)
          emplace_back(static_cast<T&&>(x));
        break;
      }
      emplace_back(*first);
    }
    g.n = size_type(-1);
  }

  // [first, last) holds n elements: replaces the contents with them.
  template <class It, class Sent>
  constexpr void assign_counted(It first, Sent last, size_type n) {
    if (n > capacity()) {
      check_size(n);
      block_guard bg{alloc_, allocate_block(n)};
      T* const e = construct_from(bg.b.p, static_cast<It&&>(first), static_cast<Sent&&>(last), n);
      const block b = bg.release();
      release_storage();
      adopt(b, e);
      return;
    }
    const size_type sz = size();
    T* p = first_;
    const size_type common = n < sz ? n : sz;
    for (size_type i = 0; i != common; ++i) {
      *p = *first;
      ++p;
      ++first;
    }
    if (n <= sz) {
      destroy_range(p, last_);
      last_ = p;
    } else {
      last_ = construct_from(last_, static_cast<It&&>(first), static_cast<Sent&&>(last), n - sz);
    }
  }
  template <class It, class Sent>
  constexpr void assign_input(It first, Sent last) {
    T* p = first_;
    for (; p != last_ && first != last; ++first) {
      *p = *first;
      ++p;
    }
    if (p != last_) {
      destroy_range(p, last_);
      last_ = p;
    } else {
      append_input(static_cast<It&&>(first), static_cast<Sent&&>(last));
    }
  }

  // Inserts the n elements of [first, last) at index off.
  template <class It, class Sent>
  constexpr iterator insert_counted(size_type off, It first, Sent last, size_type n) {
    if (n != 0) {
      if (n <= spare()) {
        T* const old_last = last_;
        last_ = construct_from(last_, static_cast<It&&>(first), static_cast<Sent&&>(last), n);
        ycxx::detail::rotate_elements<ycxx::detail::alloc_temp<T, Allocator>>(first_ + off, old_last, last_, alloc_);
      } else {
        realloc_insert(off, n, grow_to(n),
                       [&](T* d) { construct_from(d, static_cast<It&&>(first), static_cast<Sent&&>(last), n); });
      }
    }
    return begin() + static_cast<difference_type>(off);
  }
  // insert_counted at the end: nothing moves, so T need not be move-assignable (append_range).
  template <class It, class Sent>
  constexpr void append_counted(It first, Sent last, size_type n) {
    if (n <= spare())
      last_ = construct_from(last_, static_cast<It&&>(first), static_cast<Sent&&>(last), n);
    else
      realloc_insert(size(), n, grow_to(n),
                     [&](T* d) { construct_from(d, static_cast<It&&>(first), static_cast<Sent&&>(last), n); });
  }
  template <class It, class Sent>
  constexpr iterator insert_input(size_type off, It first, Sent last) {
    const size_type old_size = size();
    append_input(static_cast<It&&>(first), static_cast<Sent&&>(last));
    ycxx::detail::rotate_elements<ycxx::detail::alloc_temp<T, Allocator>>(first_ + off, first_ + old_size, last_,
                                                                          alloc_);
    return begin() + static_cast<difference_type>(off);
  }
  // Element moves by assignment are memmove for trivially copyable elements (outside constant
  // evaluation): the loops they replace stay for constant evaluation and other types.
  static constexpr bool memmove_assign = is_trivially_copyable_v<T> && is_trivially_move_assignable_v<T>;

  // Inserts at index off < size() with spare capacity, from a value that is not an element.
  constexpr void shift_in(size_type off, T&& x) {
    T* const p = first_ + off;
    alloc_traits::construct(alloc_, last_, static_cast<T&&>(last_[-1]));
    T* const old_last = last_;
    ++last_;
    if constexpr (memmove_assign) {
      if !consteval {
        __builtin_memmove(static_cast<void*>(p + 1), static_cast<const void*>(p),
                          static_cast<size_t>(old_last - 1 - p) * sizeof(T));
        *p = static_cast<T&&>(x);
        return;
      }
    }
    for (T* d = old_last - 1; d != p; --d)
      *d = static_cast<T&&>(d[-1]);
    *p = static_cast<T&&>(x);
  }

  template <class R>
  static constexpr bool counted_range = ranges::forward_range<R> || ranges::sized_range<R>;
  // The constructors that do work delegate to this one, so that the destructor cleans up if
  // they throw; it is not noexcept, so a (non-conforming) throwing allocator copy propagates.
  struct with_alloc {
    explicit with_alloc() = default;
  };
  constexpr vector(with_alloc, const Allocator& a) : alloc_(a) {}

public:
  // ---- [vector.cons] ----
  constexpr vector() noexcept(is_nothrow_default_constructible_v<Allocator>) : vector(Allocator()) {}
  constexpr explicit vector(const Allocator& a) noexcept : alloc_(a) {}
  constexpr explicit vector(size_type n, const Allocator& a = Allocator()) : vector(with_alloc{}, a) { init_n(n); }
  constexpr vector(ycxx::detail::alloc_size_t<Allocator> n, const T& value, const Allocator& a = Allocator())
      : vector(with_alloc{}, a) {
    init_n(n, value);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr vector(InputIterator first, InputIterator last, const Allocator& a = Allocator())
      : vector(with_alloc{}, a) {
    if constexpr (ycxx::detail::multipass_iterator<InputIterator>) {
      const auto n = ycxx::detail::iter_pair_distance(first, last);
      init_counted(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last), static_cast<size_type>(n));
    } else {
      append_input(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    }
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<T> R>
  constexpr vector(Tag, R&& rg, const Allocator& a = Allocator()) : vector(with_alloc{}, a) {
    if constexpr (counted_range<R>) {
      const auto n = ranges::distance(rg);
      init_counted(ranges::begin(rg), ranges::end(rg), static_cast<size_type>(n));
    } else {
      if constexpr (ranges::approximately_sized_range<R>) {
        const auto h = static_cast<size_type>(ranges::reserve_hint(rg));
        reserve(h < max_size() ? h : max_size());
      }
      append_input(ranges::begin(rg), ranges::end(rg));
    }
  }
  constexpr vector(const vector& x)
      : vector(with_alloc{}, alloc_traits::select_on_container_copy_construction(x.alloc_)) {
    init_counted(static_cast<const T*>(x.first_), static_cast<const T*>(x.last_), x.size());
  }
  constexpr vector(vector&& x) noexcept : alloc_(static_cast<Allocator&&>(x.alloc_)) { take(x); }
  constexpr vector(const vector& x, const type_identity_t<Allocator>& a) : vector(with_alloc{}, a) {
    init_counted(static_cast<const T*>(x.first_), static_cast<const T*>(x.last_), x.size());
  }
  constexpr vector(vector&& x, const type_identity_t<Allocator>& a) noexcept(always_equal) : vector(with_alloc{}, a) {
    if (always_equal || alloc_ == x.alloc_)
      take(x);
    else
      init_counted(std::move_iterator<T*>(x.first_), std::move_iterator<T*>(x.last_), x.size());
  }
  // Two overloads instead of a default argument, not delegating (init_counted leaves *this
  // owning nothing if it throws): GCC 16 crashes (ICE in cxx_eval_indirect_ref) on nested braced
  // initializers (vector<vector<vector<int>>>{{{1}}}) when the allocator is copied from the
  // default argument. Equivalent: the default argument is a value-initialized Allocator.
  constexpr vector(initializer_list<T> il) : alloc_() { init_counted(il.begin(), il.end(), il.size()); }
  constexpr vector(initializer_list<T> il, const Allocator& a) : alloc_(a) {
    init_counted(il.begin(), il.end(), il.size());
  }

  constexpr ~vector() { release_storage(); }

  constexpr vector& operator=(const vector& x) {
    if (this == __builtin_addressof(x))
      return *this;
    if constexpr (pocca) {
      // Storage from the old allocator must go back to it before the allocator is replaced.
      if (!always_equal && alloc_ != x.alloc_)
        release_storage();
      alloc_ = x.alloc_;
    }
    assign_counted(static_cast<const T*>(x.first_), static_cast<const T*>(x.last_), x.size());
    return *this;
  }
  constexpr vector& operator=(vector&& x) noexcept(pocma || always_equal) {
    if (this == __builtin_addressof(x))
      return *this;
    if constexpr (pocma || always_equal) {
      release_storage();
      if constexpr (pocma)
        alloc_ = static_cast<Allocator&&>(x.alloc_);
      take(x);
    } else {
      if (alloc_ == x.alloc_) {
        release_storage();
        take(x);
      } else {
        assign_counted(std::move_iterator<T*>(x.first_), std::move_iterator<T*>(x.last_), x.size());
      }
    }
    return *this;
  }
  constexpr vector& operator=(initializer_list<T> il) {
    assign_counted(il.begin(), il.end(), il.size());
    return *this;
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr void assign(InputIterator first, InputIterator last) {
    if constexpr (ycxx::detail::multipass_iterator<InputIterator>) {
      const auto n = ycxx::detail::iter_pair_distance(first, last);
      assign_counted(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      assign_input(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    }
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void assign_range(R&& rg) {
    static_assert(assignable_from<T&, ranges::range_reference_t<R>>,
                  "std::vector::assign_range: T& must be assignable from the range's reference type");
    if constexpr (counted_range<R>) {
      const auto n = ranges::distance(rg);
      assign_counted(ranges::begin(rg), ranges::end(rg), static_cast<size_type>(n));
    } else {
      assign_input(ranges::begin(rg), ranges::end(rg));
    }
  }
  constexpr void assign(size_type n, const T& u) {
    if (n > capacity()) {
      // u may be an element: build the new contents before releasing the old ones.
      check_size(n);
      block_guard bg{alloc_, allocate_block(n)};
      T* const e = construct_n(bg.b.p, n, u);
      const block b = bg.release();
      release_storage();
      adopt(b, e);
      return;
    }
    const size_type sz = size();
    const size_type common = n < sz ? n : sz;
    for (size_type i = 0; i != common; ++i)
      first_[i] = u;
    if (n <= sz) {
      destroy_range(first_ + n, last_);
      last_ = first_ + n;
    } else {
      last_ = construct_n(last_, n - sz, u);
    }
  }
  constexpr void assign(initializer_list<T> il) { assign_counted(il.begin(), il.end(), il.size()); }
  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(first_); }
  constexpr const_iterator begin() const noexcept { return const_iterator(first_); }
  constexpr iterator end() noexcept { return iterator(last_); }
  constexpr const_iterator end() const noexcept { return const_iterator(last_); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [vector.capacity] ----
  [[nodiscard]] constexpr bool empty() const noexcept { return first_ == last_; }
  constexpr size_type size() const noexcept { return static_cast<size_type>(last_ - first_); }
  constexpr size_type max_size() const noexcept {
    // No object is larger than PTRDIFF_MAX bytes.
    const size_type by_alloc = alloc_traits::max_size(alloc_);
    const auto diff_max =
        static_cast<make_unsigned_t<difference_type>>(numeric_limits<difference_type>::max()) / sizeof(T);
    return diff_max < by_alloc ? static_cast<size_type>(diff_max) : by_alloc;
  }
  constexpr size_type capacity() const noexcept { return static_cast<size_type>(cap_ - first_); }
  constexpr void resize(size_type sz) {
    const size_type cur = size();
    if (sz <= cur) {
      destroy_range(first_ + sz, last_);
      last_ = first_ + sz;
    } else if (sz - cur <= spare()) {
      last_ = construct_n(last_, sz - cur);
    } else {
      const size_type n = sz - cur;
      realloc_insert(cur, n, grow_to(n), [&](T* d) { construct_n(d, n); });
    }
  }
  constexpr void resize(size_type sz, const T& c) {
    const size_type cur = size();
    if (sz <= cur) {
      destroy_range(first_ + sz, last_);
      last_ = first_ + sz;
    } else if (sz - cur <= spare()) {
      last_ = construct_n(last_, sz - cur, c);
    } else {
      // c may be an element: the copies are made before the old elements move.
      const size_type n = sz - cur;
      realloc_insert(cur, n, grow_to(n), [&](T* d) { construct_n(d, n, c); });
    }
  }
  constexpr void reserve(size_type n) {
    if (n > max_size())
      ycxx::detail::throw_length_error("std::vector::reserve: argument exceeds max_size()");
    if (n > capacity())
      realloc_insert(size(), 0, n, [](T*) {});
  }
  constexpr void shrink_to_fit() {
    if (last_ == cap_)
      return;
    if (first_ == last_) {
      release_storage();
      return;
    }
    // A non-binding request: if the allocation fails, nothing changes and nothing is thrown.
    block b{nullptr, 0};
    if constexpr (ycxx::detail::cfg::exceptions) {
      try {
        b = allocate_block(size());
      } catch (...) {
        return;
      }
    } else {
      b = allocate_block(size());
    }
    block_guard bg{alloc_, b};
    if (b.cap >= capacity())
      return; // the allocator gave nothing back
    T* const e = relocate(first_, last_, b.p);
    bg.release();
    release_storage();
    adopt(b, e);
  }

  // ---- element access ----
  constexpr reference operator[](size_type n) {
    ycxx::detail::precondition(n < size(), "std::vector::operator[]: index out of range");
    return first_[n];
  }
  constexpr const_reference operator[](size_type n) const {
    ycxx::detail::precondition(n < size(), "std::vector::operator[]: index out of range");
    return first_[n];
  }
  constexpr reference at(size_type n) {
    if (n >= size())
      ycxx::detail::throw_out_of_range("std::vector::at: index out of range");
    return first_[n];
  }
  constexpr const_reference at(size_type n) const {
    if (n >= size())
      ycxx::detail::throw_out_of_range("std::vector::at: index out of range");
    return first_[n];
  }
  constexpr reference front() {
    ycxx::detail::precondition(first_ != last_, "std::vector::front: empty vector");
    return *first_;
  }
  constexpr const_reference front() const {
    ycxx::detail::precondition(first_ != last_, "std::vector::front: empty vector");
    return *first_;
  }
  constexpr reference back() {
    ycxx::detail::precondition(first_ != last_, "std::vector::back: empty vector");
    return last_[-1];
  }
  constexpr const_reference back() const {
    ycxx::detail::precondition(first_ != last_, "std::vector::back: empty vector");
    return last_[-1];
  }

  // ---- [vector.data] ----
  constexpr T* data() noexcept { return first_; }
  constexpr const T* data() const noexcept { return first_; }

  // ---- [vector.modifiers] ----
  template <class... Args>
  constexpr reference emplace_back(Args&&... args) {
    if (last_ != cap_) {
      alloc_traits::construct(alloc_, last_, static_cast<Args&&>(args)...);
      ++last_;
    } else {
      realloc_insert(size(), 1, grow_to(1),
                     [&](T* d) { alloc_traits::construct(alloc_, d, static_cast<Args&&>(args)...); });
    }
    return last_[-1];
  }
  constexpr void push_back(const T& x) { emplace_back(x); }
  constexpr void push_back(T&& x) { emplace_back(static_cast<T&&>(x)); }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void append_range(R&& rg) {
    if constexpr (counted_range<R>) {
      const auto n = ranges::distance(rg);
      append_counted(ranges::begin(rg), ranges::end(rg), static_cast<size_type>(n));
    } else {
      if constexpr (ranges::approximately_sized_range<R>) {
        const auto h = static_cast<size_type>(ranges::reserve_hint(rg));
        if (h > spare())
          reserve(size() + (h < max_size() - size() ? h : max_size() - size()));
      }
      append_input(ranges::begin(rg), ranges::end(rg));
    }
  }
  constexpr void pop_back() {
    ycxx::detail::precondition(first_ != last_, "std::vector::pop_back: empty vector");
    --last_;
    alloc_traits::destroy(alloc_, last_);
  }

  template <class... Args>
  constexpr iterator emplace(const_iterator position, Args&&... args) {
    const size_type off = static_cast<size_type>(position - cbegin());
    if (last_ == cap_) {
      realloc_insert(off, 1, grow_to(1),
                     [&](T* d) { alloc_traits::construct(alloc_, d, static_cast<Args&&>(args)...); });
    } else if (first_ + off == last_) {
      alloc_traits::construct(alloc_, last_, static_cast<Args&&>(args)...);
      ++last_;
    } else if constexpr (sizeof...(Args) == 1 && (is_same_v<Args, T> && ...)) {
      // A single non-const rvalue of type T: as insert(position, T&&), it is not an element
      // ([res.on.arguments]/1.3), so it is moved in directly without a temporary.
      shift_in(off, static_cast<Args&&>(args)...);
    } else {
      // The arguments may refer to elements that are about to move.
      ycxx::detail::alloc_temp<T, Allocator> tmp(alloc_, static_cast<Args&&>(args)...);
      shift_in(off, static_cast<T&&>(tmp.v));
    }
    return begin() + static_cast<difference_type>(off);
  }
  constexpr iterator insert(const_iterator position, const T& x) { return emplace(position, x); }
  constexpr iterator insert(const_iterator position, T&& x) {
    const size_type off = static_cast<size_type>(position - cbegin());
    if (last_ != cap_ && first_ + off != last_) { // x is not an element ([res.on.arguments]/1.3)
      shift_in(off, static_cast<T&&>(x));
      return begin() + static_cast<difference_type>(off);
    }
    return emplace(position, static_cast<T&&>(x));
  }
  constexpr iterator insert(const_iterator position, size_type n, const T& x) {
    const size_type off = static_cast<size_type>(position - cbegin());
    if (n == 0)
      return begin() + static_cast<difference_type>(off);
    if (n > spare()) {
      realloc_insert(off, n, grow_to(n), [&](T* d) { construct_n(d, n, x); });
      return begin() + static_cast<difference_type>(off);
    }
    // In place ([sequence.reqmts]: T is Cpp17CopyAssignable). x may be an element.
    ycxx::detail::alloc_temp<T, Allocator> tmp(alloc_, x);
    const T& v = tmp.v;
    T* const p = first_ + off;
    T* const old_last = last_;
    const size_type after = static_cast<size_type>(old_last - p);
    if (after > n) {
      destroy_guard g{alloc_, old_last, old_last};
      for (T* s = old_last - n; s != old_last; ++s) {
        alloc_traits::construct(alloc_, g.last, static_cast<T&&>(*s));
        ++g.last;
      }
      g.first = g.last;
      last_ = g.last;
      for (T *s = old_last - n, *d = old_last; s != p;)
        *--d = static_cast<T&&>(*--s);
      for (T* d = p; d != p + n; ++d)
        *d = v;
    } else {
      last_ = construct_n(old_last, n - after, v);
      destroy_guard g{alloc_, last_, last_};
      for (T* s = p; s != old_last; ++s) {
        alloc_traits::construct(alloc_, g.last, static_cast<T&&>(*s));
        ++g.last;
      }
      g.first = g.last;
      last_ = g.last;
      for (T* d = p; d != old_last; ++d)
        *d = v;
    }
    return begin() + static_cast<difference_type>(off);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr iterator insert(const_iterator position, InputIterator first, InputIterator last) {
    const size_type off = static_cast<size_type>(position - cbegin());
    if constexpr (ycxx::detail::multipass_iterator<InputIterator>) {
      const auto n = ycxx::detail::iter_pair_distance(first, last);
      return insert_counted(off, static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last),
                            static_cast<size_type>(n));
    } else {
      return insert_input(off, static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    }
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr iterator insert_range(const_iterator position, R&& rg) {
    const size_type off = static_cast<size_type>(position - cbegin());
    if constexpr (counted_range<R>) {
      const auto n = ranges::distance(rg);
      return insert_counted(off, ranges::begin(rg), ranges::end(rg), static_cast<size_type>(n));
    } else {
      if constexpr (ranges::approximately_sized_range<R>) {
        const auto h = static_cast<size_type>(ranges::reserve_hint(rg));
        if (h > spare())
          reserve(size() + (h < max_size() - size() ? h : max_size() - size()));
      }
      return insert_input(off, ranges::begin(rg), ranges::end(rg));
    }
  }
  constexpr iterator insert(const_iterator position, initializer_list<T> il) {
    return insert_counted(static_cast<size_type>(position - cbegin()), il.begin(), il.end(), il.size());
  }

  constexpr iterator erase(const_iterator position) {
    ycxx::detail::precondition(position.base() != last_, "std::vector::erase: iterator not dereferenceable");
    return erase(position, position + 1);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    T* const p = mut(first);
    T* const q = mut(last);
    if (p != q) {
      T* d = p;
      if constexpr (memmove_assign) {
        if !consteval {
          const auto n = static_cast<size_t>(last_ - q);
          __builtin_memmove(static_cast<void*>(p), static_cast<const void*>(q), n * sizeof(T));
          d = p + n;
        }
      }
      if (d == p)
        for (T* s = q; s != last_; ++s) {
          *d = static_cast<T&&>(*s);
          ++d;
        }
      destroy_range(d, last_);
      last_ = d;
    }
    return iterator(p);
  }
  constexpr void swap(vector& x) noexcept(pocs || always_equal) {
    if constexpr (pocs)
      ycxx::detail::swap_adl::do_swap(alloc_, x.alloc_);
    else
      ycxx::detail::precondition(always_equal || alloc_ == x.alloc_,
                                 "std::vector::swap: unequal allocators that do not propagate");
    T* const f = first_;
    T* const l = last_;
    T* const c = cap_;
    first_ = x.first_;
    last_ = x.last_;
    cap_ = x.cap_;
    x.first_ = f;
    x.last_ = l;
    x.cap_ = c;
  }
  constexpr void clear() noexcept {
    destroy_range(first_, last_);
    last_ = first_;
  }
};

// ---- deduction guides ([vector.overview]) ----
template <class InputIterator, class Allocator = allocator<typename iterator_traits<InputIterator>::value_type>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
vector(InputIterator, InputIterator,
       Allocator = Allocator()) -> vector<typename iterator_traits<InputIterator>::value_type, Allocator>;
template <ranges::input_range R, class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
vector(from_range_t, R&&, Allocator = Allocator()) -> vector<ranges::range_value_t<R>, Allocator>;

// ---- comparisons ([vector.syn], [container.reqmts]) ----
template <class T, class Allocator>
constexpr bool operator==(const vector<T, Allocator>& x, const vector<T, Allocator>& y) {
  const auto n = x.size();
  if (n != y.size())
    return false;
  const T* a = x.data();
  const T* b = y.data();
  for (decltype(x.size()) i = 0; i != n; ++i)
    if (!static_cast<bool>(a[i] == b[i]))
      return false;
  return true;
}
template <class T, class Allocator>
constexpr ycxx::detail::synth_three_way_result<T> operator<=>(const vector<T, Allocator>& x,
                                                              const vector<T, Allocator>& y) {
  const auto nx = x.size();
  const auto ny = y.size();
  const auto n = nx < ny ? nx : ny;
  const T* a = x.data();
  const T* b = y.data();
  for (decltype(x.size()) i = 0; i != n; ++i)
    if (auto c = ycxx::detail::synth_three_way(a[i], b[i]); c != 0)
      return c;
  return nx <=> ny;
}

template <class T, class Allocator>
constexpr void swap(vector<T, Allocator>& x, vector<T, Allocator>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [vector.erasure] ----
template <class T, class Allocator, class Predicate>
constexpr typename vector<T, Allocator>::size_type erase_if(vector<T, Allocator>& c, Predicate pred) {
  // remove_if, then erase the tail.
  T* const base = c.data();
  T* first = base;
  T* const last = base + c.size();
  while (first != last && !static_cast<bool>(pred(*first)))
    ++first;
  T* out = first;
  if (first != last) {
    for (++first; first != last; ++first)
      if (!static_cast<bool>(pred(*first))) {
        *out = static_cast<T&&>(*first);
        ++out;
      }
  }
  const auto r = static_cast<typename vector<T, Allocator>::size_type>(last - out);
  c.erase(c.begin() + (out - base), c.end());
  return r;
}
template <class T, class Allocator, class U = T>
constexpr typename vector<T, Allocator>::size_type erase(vector<T, Allocator>& c, const U& value) {
  return std::erase_if(c, [&value](const T& e) { return static_cast<bool>(e == value); });
}

namespace pmr {
template <class T>
using vector = std::vector<T, polymorphic_allocator<T>>;
} // namespace pmr

} // namespace std
