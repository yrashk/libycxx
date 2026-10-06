// libycxx core: the red-black tree shared by map, multimap, set and multiset ([associative]).
//
// Representation: nodes {parent, left, right, red} with the element in a union member, so a
// node can exist before and after its element (node handles). A header node closes the tree:
// header.left is the root, header.right the rightmost node, header.parent null (the only node
// whose parent is null, which is how iteration recognises it), root.parent is the header. The
// header acts as a node greater than every element: the in-order successor of the rightmost
// node is the header, which is end(). The leftmost node (begin()) is kept in the tree object.
// While the tree holds no element, header.left is null, header.right the header itself, and the
// leftmost pointer the header. hdr_ is null until the first insertion. At run time the header is
// a member; during constant evaluation it is allocated with std::allocator instead (GCC 16
// mis-evaluates pointers into an object returned with NRVO, as for list). Moves and swaps
// relink the root to the other header; nodes are never copied by moves.
//
// Elements are constructed and destroyed through the allocator rebound to the node type.
// Insertions find the position first and link a fully built node, so a throwing constructor or
// comparison leaves the tree unchanged. Unique-key insertions read the key from the arguments
// when it is a key_type (or a pair whose first member is one), and construct nothing when the
// key is present. A hinted insertion costs O(1) comparisons when the hint is right, so building
// from sorted input (hint end()) is linear: red-black insertion rebalances in amortised O(1).
// Copies clone the shape of the source tree (linear).
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/assoc_support.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/node_handle.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

struct __rb_node_base {
  __rb_node_base* __parent;
  __rb_node_base* left;
  __rb_node_base* right;
  bool __red;
};

template <class _Vp>
struct __rb_node : __rb_node_base {
  union {
    _Vp value;
  };
  constexpr __rb_node() noexcept : __rb_node_base{nullptr, nullptr, nullptr, false} {}
  __rb_node(const __rb_node&) = delete;
  constexpr ~__rb_node() {}
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

using __rb_base = ::__ycxx::__adl_free::__rb_node_base;

constexpr __rb_base* __rb_min(__rb_base* __x) noexcept {
  while (__x->left)
    __x = __x->left;
  return __x;
}
constexpr __rb_base* __rb_max(__rb_base* __x) noexcept {
  while (__x->right)
    __x = __x->right;
  return __x;
}
// In-order successor; the successor of the rightmost node is the header.
constexpr __rb_base* __rb_next(__rb_base* __x) noexcept {
  if (__x->right)
    return ::__ycxx::__detail::__rb_min(__x->right);
  __rb_base* p = __x->__parent;
  // p->parent is null only for the header, whose right link is not a child link.
  while (p->__parent && __x == p->right) {
    __x = p;
    p = p->__parent;
  }
  return p;
}
// In-order predecessor; the predecessor of the header is the rightmost node.
constexpr __rb_base* __rb_prev(__rb_base* __x) noexcept {
  if (!__x->__parent)
    return __x->right;
  if (__x->left)
    return ::__ycxx::__detail::__rb_max(__x->left);
  __rb_base* p = __x->__parent;
  while (__x == p->left) {
    __x = p;
    p = p->__parent;
  }
  return p;
}

// Rotations. The root is the header's left child, so the parent link update needs no special
// case for it.
constexpr void __rb_rotate_left(__rb_base* __x) noexcept {
  __rb_base* y = __x->right;
  __x->right = y->left;
  if (y->left)
    y->left->__parent = __x;
  y->__parent = __x->__parent;
  if (__x == __x->__parent->left)
    __x->__parent->left = y;
  else
    __x->__parent->right = y;
  y->left = __x;
  __x->__parent = y;
}
constexpr void __rb_rotate_right(__rb_base* __x) noexcept {
  __rb_base* y = __x->left;
  __x->left = y->right;
  if (y->right)
    y->right->__parent = __x;
  y->__parent = __x->__parent;
  if (__x == __x->__parent->left)
    __x->__parent->left = y;
  else
    __x->__parent->right = y;
  y->right = __x;
  __x->__parent = y;
}

// Links z as the left or right child of parent (the header: z becomes the root of an empty
// tree), maintains the leftmost node `first` and the rightmost node (header->right), and
// restores the red-black properties.
constexpr void __rb_insert(__rb_base* __z, __rb_base* __parent, bool left, __rb_base* __header, __rb_base*& first) noexcept {
  __z->__parent = __parent;
  __z->left = __z->right = nullptr;
  __z->__red = true;
  if (__parent == __header) {
    __header->left = __header->right = __z;
    first = __z;
  } else if (left) {
    __parent->left = __z;
    if (__parent == first)
      first = __z;
  } else {
    __parent->right = __z;
    if (__parent == __header->right)
      __header->right = __z;
  }
  // A red parent is not the root (the root is black), so the grandparent is a node.
  while (__z != __header->left && __z->__parent->__red) {
    __rb_base* p = __z->__parent;
    __rb_base* __g = p->__parent;
    if (p == __g->left) {
      __rb_base* __u = __g->right;
      if (__u && __u->__red) {
        p->__red = __u->__red = false;
        __g->__red = true;
        __z = __g;
      } else {
        if (__z == p->right) {
          __z = p;
          ::__ycxx::__detail::__rb_rotate_left(__z);
          p = __z->__parent;
        }
        p->__red = false;
        __g->__red = true;
        ::__ycxx::__detail::__rb_rotate_right(__g);
      }
    } else {
      __rb_base* __u = __g->left;
      if (__u && __u->__red) {
        p->__red = __u->__red = false;
        __g->__red = true;
        __z = __g;
      } else {
        if (__z == p->left) {
          __z = p;
          ::__ycxx::__detail::__rb_rotate_right(__z);
          p = __z->__parent;
        }
        p->__red = false;
        __g->__red = true;
        ::__ycxx::__detail::__rb_rotate_left(__g);
      }
    }
  }
  __header->left->__red = false;
}

// Replaces the subtree rooted at u by the one rooted at v (possibly null) in u's parent.
constexpr void __rb_transplant(__rb_base* __u, __rb_base* __v) noexcept {
  if (__u == __u->__parent->left)
    __u->__parent->left = __v;
  else
    __u->__parent->right = __v;
  if (__v)
    __v->__parent = __u->__parent;
}

// Unlinks z, which is not the only node, maintaining the leftmost node `first` and the
// rightmost node and restoring the red-black properties. Other nodes keep their addresses.
constexpr void __rb_erase(__rb_base* __z, __rb_base* __header, __rb_base*& first) noexcept {
  if (__z == first)
    first = ::__ycxx::__detail::__rb_next(__z);
  if (__z == __header->right)
    __header->right = ::__ycxx::__detail::__rb_prev(__z);
  __rb_base* __x;  // the node that moves into the removed position (may be null)
  __rb_base* __xp; // its parent
  bool __removed_red = __z->__red;
  if (!__z->left) {
    __x = __z->right;
    __xp = __z->__parent;
    ::__ycxx::__detail::__rb_transplant(__z, __z->right);
  } else if (!__z->right) {
    __x = __z->left;
    __xp = __z->__parent;
    ::__ycxx::__detail::__rb_transplant(__z, __z->left);
  } else {
    __rb_base* y = ::__ycxx::__detail::__rb_min(__z->right);
    __removed_red = y->__red;
    __x = y->right;
    if (y->__parent == __z) {
      __xp = y;
    } else {
      __xp = y->__parent;
      ::__ycxx::__detail::__rb_transplant(y, y->right);
      y->right = __z->right;
      y->right->__parent = y;
    }
    ::__ycxx::__detail::__rb_transplant(__z, y);
    y->left = __z->left;
    y->left->__parent = y;
    y->__red = __z->__red;
  }
  if (__removed_red)
    return;
  // x carries an extra black. Its sibling w exists: the removed black node had a black height
  // of at least one on the sibling's side.
  while (__x != __header->left && (!__x || !__x->__red)) {
    if (__x == __xp->left) {
      __rb_base* __w = __xp->right;
      if (__w->__red) {
        __w->__red = false;
        __xp->__red = true;
        ::__ycxx::__detail::__rb_rotate_left(__xp);
        __w = __xp->right;
      }
      if ((!__w->left || !__w->left->__red) && (!__w->right || !__w->right->__red)) {
        __w->__red = true;
        __x = __xp;
        __xp = __xp->__parent;
      } else {
        if (!__w->right || !__w->right->__red) {
          __w->left->__red = false;
          __w->__red = true;
          ::__ycxx::__detail::__rb_rotate_right(__w);
          __w = __xp->right;
        }
        __w->__red = __xp->__red;
        __xp->__red = false;
        if (__w->right)
          __w->right->__red = false;
        ::__ycxx::__detail::__rb_rotate_left(__xp);
        __x = __header->left;
        break;
      }
    } else {
      __rb_base* __w = __xp->left;
      if (__w->__red) {
        __w->__red = false;
        __xp->__red = true;
        ::__ycxx::__detail::__rb_rotate_right(__xp);
        __w = __xp->left;
      }
      if ((!__w->left || !__w->left->__red) && (!__w->right || !__w->right->__red)) {
        __w->__red = true;
        __x = __xp;
        __xp = __xp->__parent;
      } else {
        if (!__w->left || !__w->left->__red) {
          __w->right->__red = false;
          __w->__red = true;
          ::__ycxx::__detail::__rb_rotate_left(__w);
          __w = __xp->left;
        }
        __w->__red = __xp->__red;
        __xp->__red = false;
        if (__w->left)
          __w->left->__red = false;
        ::__ycxx::__detail::__rb_rotate_right(__xp);
        __x = __header->left;
        break;
      }
    }
  }
  if (__x)
    __x->__red = false;
}

// The key of the element a set (IsMap false) or map would build from args, when it can be read
// from the arguments without constructing an element: a single key argument (set), a key first
// argument of two (map), or a pair whose first member is a key (map). A key argument is a
// key_type, or an arithmetic value when key_type is arithmetic (converted as the element's
// construction converts it; such a temporary key is not observable).
template <class _Key, class _Ap>
inline constexpr bool __key_like = std::is_same_v<std::remove_cvref_t<_Ap>, _Key> ||
                                 (std::is_arithmetic_v<_Key> && std::is_arithmetic_v<std::remove_cvref_t<_Ap>>);
template <class _Pp>
struct __pair_first {
  using type = void;
};
template <class _Xp, class _Yp>
struct __pair_first<std::pair<_Xp, _Yp>> {
  using type = _Xp;
};
template <class _Key, bool _IsMap, class... _Args>
inline constexpr bool __key_in_args = false;
template <class _Key, class _Ap>
inline constexpr bool __key_in_args<_Key, false, _Ap> = __key_like<_Key, _Ap>;
template <class _Key, class _Ap, class _Bp>
inline constexpr bool __key_in_args<_Key, true, _Ap, _Bp> = __key_like<_Key, _Ap>;
template <class _Key, class _Pp>
  requires(!std::is_void_v<typename __pair_first<std::remove_cvref_t<_Pp>>::type>)
inline constexpr bool __key_in_args<_Key, true, _Pp> = __key_like<_Key, typename __pair_first<std::remove_cvref_t<_Pp>>::type>;

template <class _Key, class _Ap>
constexpr decltype(auto) __as_key(const _Ap& a) noexcept {
  if constexpr (std::is_same_v<_Ap, _Key>)
    return a;
  else
    return static_cast<_Key>(a);
}
template <class _Key, bool _IsMap, class _Ap, class... _Rest>
constexpr decltype(auto) __key_arg(const _Ap& a, const _Rest&...) noexcept {
  if constexpr (_IsMap && sizeof...(_Rest) == 0)
    return ::__ycxx::__detail::__as_key<_Key>(a.first);
  else
    return ::__ycxx::__detail::__as_key<_Key>(a);
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// T is the element type, possibly const.
template <class _Tp, class _Diff>
class __rb_iter {
  using _Vp = std::remove_const_t<_Tp>;
  using base = __rb_node_base;
  base* __n_ = nullptr;

  template <class, class>
  friend class __rb_iter;
  template <class, class, class, class, bool, bool>
  friend class __rb_tree;

public:
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = _Vp;
  using difference_type = _Diff;
  using pointer = _Tp*;
  using reference = _Tp&;

  constexpr __rb_iter() noexcept = default;
  constexpr explicit __rb_iter(base* n) noexcept : __n_(n) {}
  template <class _Up>
    requires std::is_same_v<const _Up, _Tp> && (!std::is_same_v<_Up, _Tp>)
  constexpr __rb_iter(const __rb_iter<_Up, _Diff>& __o) noexcept : __n_(__o.__n_) {}

  constexpr reference operator*() const noexcept { return static_cast<__rb_node<_Vp>*>(__n_)->value; }
  constexpr pointer operator->() const noexcept { return __builtin_addressof(static_cast<__rb_node<_Vp>*>(__n_)->value); }
  constexpr __rb_iter& operator++() noexcept {
    __n_ = ::__ycxx::__detail::__rb_next(__n_);
    return *this;
  }
  constexpr __rb_iter operator++(int) noexcept {
    __rb_iter t = *this;
    __n_ = ::__ycxx::__detail::__rb_next(__n_);
    return t;
  }
  constexpr __rb_iter& operator--() noexcept {
    __n_ = ::__ycxx::__detail::__rb_prev(__n_);
    return *this;
  }
  constexpr __rb_iter operator--(int) noexcept {
    __rb_iter t = *this;
    __n_ = ::__ycxx::__detail::__rb_prev(__n_);
    return t;
  }
  friend constexpr bool operator==(const __rb_iter& a, const __rb_iter& b) noexcept { return a.__n_ == b.__n_; }
};

// The common part of map, multimap (IsMap: V is pair<const Key, T>), set and multiset (V is
// Key; iterator is const_iterator). Key is passed separately so that V is not instantiated:
// map<K, T> may be named while T is incomplete. Multi: equivalent keys. The public members here are those
// the four containers share with identical semantics; the containers add constructors,
// assignment, insertion and their own members, built on the protected interface.
template <class _Key, class _Vp, class _Compare, class _Allocator, bool _IsMap, bool _Multi>
class __rb_tree {
  template <class, class, class, class, bool, bool>
  friend class __rb_tree;

  using info = ::__ycxx::__detail::__alloc_info<_Allocator>;
  using __alloc_traits = std::allocator_traits<_Allocator>;

protected:
  using node = __rb_node<_Vp>;
  using __node_base = __rb_node_base;
  using __node_alloc = typename info::template rebind<node>;
  using __node_traits = std::allocator_traits<__node_alloc>;

public:
  using key_type = _Key;
  using value_type = _Vp;
  using key_compare = _Compare;
  using allocator_type = _Allocator;
  using pointer = typename info::pointer;
  using const_pointer = typename info::const_pointer;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = typename info::size_type;
  using difference_type = typename info::difference_type;
  using iterator = __rb_iter<std::conditional_t<_IsMap, _Vp, const _Vp>, difference_type>;
  using const_iterator = __rb_iter<const _Vp, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using node_type = __node_handle<node, _Allocator, _IsMap>;

protected:
  static constexpr bool __pocca = info::__pocca;
  static constexpr bool __pocma = info::__pocma;
  static constexpr bool __pocs = info::__pocs;
  static constexpr bool __always_equal = info::__always_equal;

  __node_base __head_ = {nullptr, nullptr, nullptr, false};
  __node_base* __hdr_ = nullptr;   // the header (end()), or null before the first insertion
  __node_base* __first_ = nullptr; // the leftmost node (begin()); the header when empty
  size_type __size_ = 0;
  [[no_unique_address]] _Compare __comp_;
  [[no_unique_address]] __node_alloc __na_;

  // ---- nodes ----
  static constexpr __node_base* __nd(const_iterator __it) noexcept { return __it.__n_; }
  static constexpr _Vp& value(__node_base* n) noexcept { return static_cast<node*>(n)->value; }
  static constexpr const key_type& key(__node_base* n) noexcept {
    if constexpr (_IsMap)
      return static_cast<node*>(n)->value.first;
    else
      return static_cast<node*>(n)->value;
  }
  template <class _Ap, class _Bp>
  constexpr bool lt(const _Ap& a, const _Bp& b) const {
    return static_cast<bool>(__comp_(a, b));
  }

  constexpr __node_base* __header() {
    if (!__hdr_) {
      if consteval {
        __hdr_ = std::allocator<__node_base>().allocate(1);
        std::construct_at(__hdr_, __node_base{nullptr, nullptr, nullptr, false});
      } else {
        __hdr_ = __builtin_addressof(__head_);
      }
      __hdr_->right = __first_ = __hdr_;
    }
    return __hdr_;
  }
  constexpr __node_base* __root() const noexcept { return __hdr_ ? __hdr_->left : nullptr; }
  constexpr __node_base* __begin_node() const noexcept { return __first_; }
  constexpr __node_base* __end_node() const noexcept { return __hdr_; }
  constexpr __node_base* __last_node() const noexcept { return __hdr_->right; }

  template <class... _Args>
  constexpr node* __make_node(_Args&&... __args) {
    if (__size_ == max_size())
      ::__ycxx::__detail::__throw_length_error("associative container: size would exceed max_size()");
    node* n = std::to_address(__node_traits::allocate(__na_, 1));
    std::construct_at(n);
    ::__ycxx::__detail::__rollback __rb{[&] {
      std::destroy_at(n);
      __node_traits::deallocate(__na_, ::__ycxx::__detail::__to_alloc_pointer<typename __node_traits::pointer>(n), 1);
    }};
    __node_traits::construct(__na_, __builtin_addressof(n->value), static_cast<_Args&&>(__args)...);
    __rb.release();
    return n;
  }
  constexpr void __free_node(__node_base* b) noexcept {
    node* n = static_cast<node*>(b);
    __node_traits::destroy(__na_, __builtin_addressof(n->value));
    std::destroy_at(n);
    __node_traits::deallocate(__na_, ::__ycxx::__detail::__to_alloc_pointer<typename __node_traits::pointer>(n), 1);
  }
  // Frees a node that is not (or no longer) linked, unless released: rollback on exceptions.
  struct __node_guard {
    __rb_tree* t;
    __node_base* n;
    constexpr ~__node_guard() {
      if (n)
        t->__free_node(n);
    }
    constexpr __node_base* release() noexcept {
      __node_base* r = n;
      n = nullptr;
      return r;
    }
  };
  // Frees the subtree rooted at x. Recursion depth is the tree height.
  constexpr void __destroy_subtree(__node_base* __x) noexcept {
    while (__x) {
      __destroy_subtree(__x->right);
      __node_base* __l = __x->left;
      __free_node(__x);
      __x = __l;
    }
  }

  // ---- linking ----
  struct __pos {
    __node_base* __parent;   // the node to link below (the header for an empty tree)
    bool left;           // as its left child
    __node_base* __existing; // or, for unique keys, the element with an equivalent key
  };
  constexpr void __link(__node_base* n, __pos p) noexcept {
    ::__ycxx::__detail::__rb_insert(n, p.__parent, p.left, __hdr_, __first_);
    ++__size_;
  }
  // Unlinks n without freeing it.
  constexpr void __unlink(__node_base* n) noexcept {
    if (__size_ == 1) {
      __hdr_->left = nullptr;
      __hdr_->right = __first_ = __hdr_;
    } else {
      ::__ycxx::__detail::__rb_erase(n, __hdr_, __first_);
    }
    --__size_;
  }

  // The position of a new element with key k: after the elements whose keys are not greater
  // (upper), or before those not less (lower).
  template <class _Kp>
  constexpr __pos __pos_upper(const _Kp& k) {
    __node_base* y = __header();
    __node_base* __x = y->left;
    bool __go_left = true;
    while (__x) {
      y = __x;
      __go_left = lt(k, key(__x));
      __x = __go_left ? __x->left : __x->right;
    }
    return {y, __go_left, nullptr};
  }
  template <class _Kp>
  constexpr __pos __pos_lower(const _Kp& k) {
    __node_base* y = __header();
    __node_base* __x = y->left;
    bool __go_left = true;
    while (__x) {
      y = __x;
      __go_left = !lt(key(__x), k);
      __x = __go_left ? __x->left : __x->right;
    }
    return {y, __go_left, nullptr};
  }
  // Unique keys: the position for key k, or the element with an equivalent key.
  template <class _Kp>
  constexpr __pos __pos_unique(const _Kp& k) {
    // The lower bound: of several elements equivalent to a heterogeneous k, the first is found.
    __pos p = __pos_lower(k);
    __node_base* __lb = p.left ? p.__parent : ::__ycxx::__detail::__rb_next(p.__parent);
    if (__lb != __hdr_ && !lt(k, key(__lb)))
      return {nullptr, false, __lb};
    return p;
  }
  // Between the adjacent nodes a (or begin) and b (or end): the free child link.
  constexpr __pos __between(__node_base* a, __node_base* b) noexcept {
    if (a->right == nullptr)
      return {a, false, nullptr};
    return {b, true, nullptr};
  }
  // Unique keys with a hint: constant time when k belongs just before or just after hint.
  template <class _Kp>
  constexpr __pos __pos_unique_hint(__node_base* __hint, const _Kp& k) {
    __node_base* h = __header();
    if (!__hint)
      __hint = h;
    if (__hint == h) {
      if (__size_ > 0 && lt(key(h->right), k))
        return {h->right, false, nullptr};
      return __pos_unique(k);
    }
    if (lt(k, key(__hint))) {
      if (__hint == __first_)
        return {__hint, true, nullptr};
      __node_base* before = ::__ycxx::__detail::__rb_prev(__hint);
      if (lt(key(before), k))
        return __between(before, __hint);
      return __pos_unique(k);
    }
    if (lt(key(__hint), k)) {
      if (__hint == h->right)
        return {__hint, false, nullptr};
      __node_base* __after = ::__ycxx::__detail::__rb_next(__hint);
      if (lt(k, key(__after)))
        return __between(__hint, __after);
      return __pos_unique(k);
    }
    return {nullptr, false, __hint};
  }
  // Equivalent keys with a hint: as close as possible to the position just before hint.
  template <class _Kp>
  constexpr __pos __pos_multi_hint(__node_base* __hint, const _Kp& k) {
    __node_base* h = __header();
    if (!__hint)
      __hint = h;
    if (__hint == h) {
      if (__size_ == 0)
        return {h, true, nullptr};
      if (!lt(k, key(h->right)))
        return {h->right, false, nullptr};
      return __pos_upper(k);
    }
    if (!lt(key(__hint), k)) { // k <= *hint
      if (__hint == __first_)
        return {__hint, true, nullptr};
      __node_base* before = ::__ycxx::__detail::__rb_prev(__hint);
      if (!lt(k, key(before)))
        return __between(before, __hint);
      return __pos_upper(k);
    }
    // *hint < k: the closest position is the start of k's equivalents.
    if (__hint == h->right)
      return {__hint, false, nullptr};
    __node_base* __after = ::__ycxx::__detail::__rb_next(__hint);
    if (!lt(key(__after), k))
      return __between(__hint, __after);
    return __pos_lower(k);
  }

  // ---- insertion ----
  // Unique keys: inserts an element built from args unless its key is present; returns the
  // element with that key and whether it was inserted.
  template <class... _Args>
  constexpr std::pair<__node_base*, bool> __emplace_unique(_Args&&... __args) {
    if constexpr (::__ycxx::__detail::__key_in_args<key_type, _IsMap, _Args...>) {
      __pos p = __pos_unique(::__ycxx::__detail::__key_arg<key_type, _IsMap>(__args...));
      if (p.__existing)
        return {p.__existing, false};
      node* n = __make_node(static_cast<_Args&&>(__args)...);
      __link(n, p);
      return {n, true};
    } else {
      __node_guard __g{this, __make_node(static_cast<_Args&&>(__args)...)};
      __pos p = __pos_unique(key(__g.n));
      if (p.__existing)
        return {p.__existing, false};
      __node_base* n = __g.release();
      __link(n, p);
      return {n, true};
    }
  }
  template <class... _Args>
  constexpr __node_base* __emplace_hint_unique(__node_base* __hint, _Args&&... __args) {
    if constexpr (::__ycxx::__detail::__key_in_args<key_type, _IsMap, _Args...>) {
      __pos p = __pos_unique_hint(__hint, ::__ycxx::__detail::__key_arg<key_type, _IsMap>(__args...));
      if (p.__existing)
        return p.__existing;
      node* n = __make_node(static_cast<_Args&&>(__args)...);
      __link(n, p);
      return n;
    } else {
      __node_guard __g{this, __make_node(static_cast<_Args&&>(__args)...)};
      __pos p = __pos_unique_hint(__hint, key(__g.n));
      if (p.__existing)
        return p.__existing;
      __node_base* n = __g.release();
      __link(n, p);
      return n;
    }
  }
  template <class... _Args>
  constexpr __node_base* __emplace_multi(_Args&&... __args) {
    __node_guard __g{this, __make_node(static_cast<_Args&&>(__args)...)};
    __pos p = __pos_upper(key(__g.n));
    __node_base* n = __g.release();
    __link(n, p);
    return n;
  }
  template <class... _Args>
  constexpr __node_base* __emplace_hint_multi(__node_base* __hint, _Args&&... __args) {
    __node_guard __g{this, __make_node(static_cast<_Args&&>(__args)...)};
    __pos p = __pos_multi_hint(__hint, key(__g.n));
    __node_base* n = __g.release();
    __link(n, p);
    return n;
  }
  template <class... _Args>
  constexpr __node_base* __emplace_any(_Args&&... __args) {
    if constexpr (_Multi)
      return __emplace_multi(static_cast<_Args&&>(__args)...);
    else
      return __emplace_unique(static_cast<_Args&&>(__args)...).first;
  }
  template <class... _Args>
  constexpr __node_base* __emplace_hint_any(__node_base* __hint, _Args&&... __args) {
    if constexpr (_Multi)
      return __emplace_hint_multi(__hint, static_cast<_Args&&>(__args)...);
    else
      return __emplace_hint_unique(__hint, static_cast<_Args&&>(__args)...);
  }
  // Unique keys, the key known up front (try_emplace, the heterogeneous insertions): builds the
  // element from args only if no element has a key equivalent to k. hint may be null.
  template <class _Kp, class... _Args>
  constexpr std::pair<__node_base*, bool> __emplace_key(__node_base* __hint, const _Kp& k, _Args&&... __args) {
    __pos p = __hint ? __pos_unique_hint(__hint, k) : __pos_unique(k);
    if (p.__existing)
      return {p.__existing, false};
    node* n = __make_node(static_cast<_Args&&>(__args)...);
    __link(n, p);
    return {n, true};
  }
  // Inserts every element of [first, last), each with the hint end(): linear for sorted input.
  template <class _It, class _Sent>
  constexpr void __insert_elems(_It first, _Sent last) {
    for (; first != last; ++first)
      __emplace_hint_any(__header(), *first);
  }

  // ---- node handles ----
  constexpr node_type __make_handle(__node_base* n) noexcept {
    return ::__ycxx::__detail::__node_handle_access::__make<node_type>(static_cast<node*>(n), allocator_type(__na_));
  }
  constexpr void __check_handle(const node_type& __nh) const noexcept {
    if constexpr (!__always_equal)
      ::__ycxx::__detail::__precondition(allocator_type(__na_) == __nh.get_allocator(),
                                   "associative container: node handle with an unequal allocator");
  }
  constexpr __node_base* __insert_handle_multi(__node_base* __hint, node_type& __nh) {
    if (__nh.empty())
      return __hdr_;
    __check_handle(__nh);
    __node_base* n = ::__ycxx::__detail::__node_handle_access::peek(__nh);
    __pos p = __hint ? __pos_multi_hint(__hint, key(n)) : __pos_upper(key(n));
    ::__ycxx::__detail::__node_handle_access::take(__nh);
    __link(n, p);
    return n;
  }
  // Unique keys: links nh's node unless its key is present; returns the element with its key.
  constexpr std::pair<__node_base*, bool> __insert_handle_unique(__node_base* __hint, node_type& __nh) {
    if (__nh.empty())
      return {__hdr_, false};
    __check_handle(__nh);
    __node_base* n = ::__ycxx::__detail::__node_handle_access::peek(__nh);
    __pos p = __hint ? __pos_unique_hint(__hint, key(n)) : __pos_unique(key(n));
    if (p.__existing)
      return {p.__existing, false};
    ::__ycxx::__detail::__node_handle_access::take(__nh);
    __link(n, p);
    return {n, true};
  }

  // ---- whole-tree operations ----
  struct __shape {
    __node_base* __root = nullptr;
    __node_base* first = nullptr;
    __node_base* last = nullptr;
    size_type n = 0;
  };
  // The nodes of *this, detached: *this becomes empty.
  constexpr __shape detach() noexcept {
    if (__size_ == 0)
      return {};
    __shape s{__hdr_->left, __first_, __hdr_->right, __size_};
    __hdr_->left = nullptr;
    __hdr_->right = __first_ = __hdr_;
    __size_ = 0;
    return s;
  }
  // Takes the nodes of s; *this must be empty.
  constexpr void __attach(__shape s) {
    if (s.n == 0)
      return;
    __node_base* h = __header();
    h->left = s.__root;
    s.__root->__parent = h;
    __first_ = s.first;
    h->right = s.last;
    __size_ = s.n;
  }
  constexpr void take(__rb_tree& __o) noexcept { __attach(__o.detach()); }

  // A copy of the subtree src below parent, each value built by gen(value).
  template <class _Gen>
  constexpr __node_base* __clone(__node_base* __src, __node_base* __parent, _Gen& __gen) {
    __node_base* top = __gen(value(__src));
    top->__red = __src->__red;
    top->__parent = __parent;
    top->left = top->right = nullptr;
    ::__ycxx::__detail::__rollback __rb{[&] { __destroy_subtree(top); }};
    if (__src->right)
      top->right = __clone(__src->right, top, __gen);
    __parent = top;
    for (__src = __src->left; __src; __src = __src->left) {
      __node_base* y = __gen(value(__src));
      y->__red = __src->__red;
      y->left = y->right = nullptr;
      y->__parent = __parent;
      __parent->left = y;
      if (__src->right)
        y->right = __clone(__src->right, y, __gen);
      __parent = y;
    }
    __rb.release();
    return top;
  }
  // Fills the empty *this with the elements of o in o's shape, each built by gen(value).
  template <class _Gen>
  constexpr void __clone_from(const __rb_tree& __o, _Gen __gen) {
    if (__o.__size_ == 0)
      return;
    __node_base* r = __clone(__o.__hdr_->left, nullptr, __gen);
    __shape s{r, ::__ycxx::__detail::__rb_min(r), ::__ycxx::__detail::__rb_max(r), __o.__size_};
    __attach(s);
  }
  constexpr void __copy_from(const __rb_tree& __o) {
    __clone_from(__o, [this](const _Vp& __v) -> __node_base* { return __make_node(__v); });
  }
  // Element-wise move: for maps, key_type and mapped_type are moved ([associative.reqmts.general]/8
  // puts the move-insertable requirement on them), so a move-only key is moved out of the source
  // node, whose element is destroyed afterwards without being compared again. During constant
  // evaluation a copyable key is copied instead.
  constexpr void __move_from(__rb_tree& __o) {
    __clone_from(__o, [this](_Vp& __v) -> __node_base* {
      if constexpr (_IsMap) {
        using _Mp = typename _Vp::second_type;
        if constexpr (std::is_copy_constructible_v<_Key>) {
          if consteval {
            return __make_node(__v.first, static_cast<_Mp&&>(__v.second));
          }
        }
        return __make_node(static_cast<_Key&&>(const_cast<_Key&>(__v.first)), static_cast<_Mp&&>(__v.second));
      } else {
        return __make_node(static_cast<_Vp&&>(__v));
      }
    });
  }

  // ---- construction and assignment ----
  // noexcept strengthenings: default construction and the allocator-extended move with an
  // always-equal allocator allocate nothing.
  static constexpr bool __nothrow_default = std::is_nothrow_default_constructible_v<_Compare> &&
                                          std::is_nothrow_copy_constructible_v<_Compare> &&
                                          std::is_nothrow_default_constructible_v<_Allocator>;
  static constexpr bool __nothrow_move_alloc = __always_equal && std::is_nothrow_copy_constructible_v<_Compare>;

  constexpr __rb_tree(const _Compare& c, const _Allocator& a) noexcept(std::is_nothrow_copy_constructible_v<_Compare>)
      : __comp_(c), __na_(a) {}
  constexpr __rb_tree(const __rb_tree& __o, const _Allocator& a) : __comp_(__o.__comp_), __na_(a) { __copy_from(__o); }
  // The comparison object is copied: the source stays usable.
  constexpr __rb_tree(__rb_tree&& __o) noexcept(std::is_nothrow_copy_constructible_v<_Compare>)
      : __comp_(__o.__comp_), __na_(static_cast<__node_alloc&&>(__o.__na_)) {
    take(__o);
  }
  constexpr __rb_tree(__rb_tree&& __o, const _Allocator& a) noexcept(__nothrow_move_alloc) : __comp_(__o.__comp_), __na_(a) {
    if (__always_equal || __na_ == __o.__na_)
      take(__o);
    else
      __move_from(__o);
  }
  constexpr ~__rb_tree() {
    clear();
    if (__hdr_ && __hdr_ != __builtin_addressof(__head_)) {
      std::destroy_at(__hdr_);
      std::allocator<__node_base>().deallocate(__hdr_, 1);
    }
  }

  constexpr void __copy_assign(const __rb_tree& __o) {
    if (this == __builtin_addressof(__o))
      return;
    clear();
    if constexpr (__pocca)
      __na_ = __o.__na_;
    __comp_ = __o.__comp_;
    __copy_from(__o);
  }
  constexpr void __move_assign(__rb_tree& __o) {
    if (this == __builtin_addressof(__o))
      return;
    clear();
    __comp_ = static_cast<_Compare&&>(__o.__comp_);
    if constexpr (__pocma) {
      __na_ = static_cast<__node_alloc&&>(__o.__na_);
      take(__o);
    } else if (__always_equal || __na_ == __o.__na_) {
      take(__o);
    } else {
      __move_from(__o);
    }
  }
  constexpr void __swap_tree(__rb_tree& __o) {
    if (this == __builtin_addressof(__o))
      return;
    ::__ycxx::__detail::__swap_adl::__do_swap(__comp_, __o.__comp_);
    if constexpr (__pocs)
      ::__ycxx::__detail::__swap_adl::__do_swap(__na_, __o.__na_);
    else
      ::__ycxx::__detail::__precondition(__always_equal || __na_ == __o.__na_,
                                   "associative container swap: unequal allocators that do not propagate");
    const __shape a = detach(), b = __o.detach();
    __attach(b);
    __o.__attach(a);
  }

  // Moves the elements of source into *this ([associative.reqmts.general]/112-117); with
  // unique keys, those whose keys are present stay behind.
  template <class _C2, bool _M2>
  constexpr void __merge_from(__rb_tree<_Key, _Vp, _C2, _Allocator, _IsMap, _M2>& __source) {
    if (static_cast<void*>(this) == static_cast<void*>(__builtin_addressof(__source)) || __source.__size_ == 0)
      return;
    if constexpr (!__always_equal)
      ::__ycxx::__detail::__precondition(allocator_type(__na_) == allocator_type(__source.__na_),
                                   "associative container merge: unequal allocators");
    for (__node_base* p = __source.__first_; p != __source.__hdr_;) {
      __node_base* __nx = ::__ycxx::__detail::__rb_next(p);
      if constexpr (_Multi) {
        __pos at = __pos_upper(key(p));
        __source.__unlink(p);
        __link(p, at);
      } else {
        __pos at = __pos_unique(key(p));
        if (!at.__existing) {
          __source.__unlink(p);
          __link(p, at);
        }
      }
      p = __nx;
    }
  }

  // ---- lookup ----
  template <class _Kp>
  constexpr __node_base* lower(const _Kp& k) const {
    __node_base* y = __hdr_;
    for (__node_base* __x = __root(); __x;) {
      if (!lt(key(__x), k)) {
        y = __x;
        __x = __x->left;
      } else {
        __x = __x->right;
      }
    }
    return y;
  }
  template <class _Kp>
  constexpr __node_base* upper(const _Kp& k) const {
    __node_base* y = __hdr_;
    for (__node_base* __x = __root(); __x;) {
      if (lt(k, key(__x))) {
        y = __x;
        __x = __x->left;
      } else {
        __x = __x->right;
      }
    }
    return y;
  }
  template <class _Kp>
  constexpr __node_base* __find_node(const _Kp& k) const {
    __node_base* n = lower(k);
    if (n == __hdr_ || lt(k, key(n)))
      return __hdr_;
    return n;
  }
  template <class _Kp>
  constexpr std::pair<__node_base*, __node_base*> range(const _Kp& k) const {
    return {lower(k), upper(k)};
  }
  template <class _Kp>
  constexpr size_type __count_key(const _Kp& k) const {
    std::pair<__node_base*, __node_base*> r = range(k);
    size_type n = 0;
    for (__node_base* p = r.first; p != r.second; p = ::__ycxx::__detail::__rb_next(p))
      ++n;
    return n;
  }
  template <class _Kp>
  constexpr size_type __erase_key(const _Kp& k) {
    std::pair<__node_base*, __node_base*> r = range(k);
    size_type n = 0;
    for (__node_base* p = r.first; p != r.second; ++n) {
      __node_base* __nx = ::__ycxx::__detail::__rb_next(p);
      __unlink(p);
      __free_node(p);
      p = __nx;
    }
    return n;
  }
  template <class _Kp>
  constexpr node_type __extract_key(const _Kp& k) {
    __node_base* n = __find_node(k);
    if (n == __hdr_)
      return node_type();
    __unlink(n);
    return __make_handle(n);
  }

public:
  constexpr allocator_type get_allocator() const noexcept { return allocator_type(__na_); }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(__first_); }
  constexpr const_iterator begin() const noexcept { return const_iterator(__first_); }
  constexpr iterator end() noexcept { return iterator(__hdr_); }
  constexpr const_iterator end() const noexcept { return const_iterator(__hdr_); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- capacity ----
  [[nodiscard]] constexpr bool empty() const noexcept { return __size_ == 0; }
  constexpr size_type size() const noexcept { return __size_; }
  constexpr size_type max_size() const noexcept {
    const auto a = static_cast<size_type>(__node_traits::max_size(__na_));
    const auto d = static_cast<size_type>(std::numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }

  // ---- modifiers ----
  constexpr node_type extract(const_iterator position) {
    ::__ycxx::__detail::__precondition(position.__n_ != __hdr_, "associative container extract: end() iterator");
    __unlink(position.__n_);
    return __make_handle(position.__n_);
  }
  constexpr node_type extract(const key_type& __x) { return __extract_key(__x); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_non_iter<_Compare, _Kp, iterator, const_iterator>
  constexpr node_type extract(_Kp&& __x) {
    return __extract_key(__x);
  }

  constexpr iterator erase(iterator position)
    requires(_IsMap)
  {
    return erase(const_iterator(position));
  }
  constexpr iterator erase(const_iterator position) {
    ::__ycxx::__detail::__precondition(position.__n_ != __hdr_, "associative container erase: end() iterator");
    __node_base* n = position.__n_;
    __node_base* __nx = ::__ycxx::__detail::__rb_next(n);
    __unlink(n);
    __free_node(n);
    return iterator(__nx);
  }
  constexpr size_type erase(const key_type& __x) { return __erase_key(__x); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_non_iter<_Compare, _Kp, iterator, const_iterator>
  constexpr size_type erase(_Kp&& __x) {
    return __erase_key(__x);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    if (first.__n_ == __first_ && last.__n_ == __hdr_) {
      clear();
      return end();
    }
    __node_base* p = first.__n_;
    while (p != last.__n_) {
      __node_base* __nx = ::__ycxx::__detail::__rb_next(p);
      __unlink(p);
      __free_node(p);
      p = __nx;
    }
    return iterator(last.__n_);
  }
  constexpr void clear() noexcept {
    if (__size_ == 0)
      return;
    __destroy_subtree(__hdr_->left);
    __hdr_->left = nullptr;
    __hdr_->right = __first_ = __hdr_;
    __size_ = 0;
  }

  // ---- observers ----
  constexpr key_compare key_comp() const { return __comp_; }

  // ---- lookup ----
  constexpr iterator find(const key_type& __x) { return iterator(__find_node(__x)); }
  constexpr const_iterator find(const key_type& __x) const { return const_iterator(__find_node(__x)); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr iterator find(const _Kp& __x) {
    return iterator(__find_node(__x));
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr const_iterator find(const _Kp& __x) const {
    return const_iterator(__find_node(__x));
  }
  constexpr size_type count(const key_type& __x) const {
    if constexpr (_Multi)
      return __count_key(__x);
    else
      return __find_node(__x) == __hdr_ ? 0 : 1;
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr size_type count(const _Kp& __x) const {
    return __count_key(__x);
  }
  constexpr bool contains(const key_type& __x) const { return __find_node(__x) != __hdr_; }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr bool contains(const _Kp& __x) const {
    return __find_node(__x) != __hdr_;
  }
  constexpr iterator lower_bound(const key_type& __x) { return iterator(lower(__x)); }
  constexpr const_iterator lower_bound(const key_type& __x) const { return const_iterator(lower(__x)); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr iterator lower_bound(const _Kp& __x) {
    return iterator(lower(__x));
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr const_iterator lower_bound(const _Kp& __x) const {
    return const_iterator(lower(__x));
  }
  constexpr iterator upper_bound(const key_type& __x) { return iterator(upper(__x)); }
  constexpr const_iterator upper_bound(const key_type& __x) const { return const_iterator(upper(__x)); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr iterator upper_bound(const _Kp& __x) {
    return iterator(upper(__x));
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr const_iterator upper_bound(const _Kp& __x) const {
    return const_iterator(upper(__x));
  }
  constexpr std::pair<iterator, iterator> equal_range(const key_type& __x) {
    auto r = range(__x);
    return {iterator(r.first), iterator(r.second)};
  }
  constexpr std::pair<const_iterator, const_iterator> equal_range(const key_type& __x) const {
    auto r = range(__x);
    return {const_iterator(r.first), const_iterator(r.second)};
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr std::pair<iterator, iterator> equal_range(const _Kp& __x) {
    auto r = range(__x);
    return {iterator(r.first), iterator(r.second)};
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr std::pair<const_iterator, const_iterator> equal_range(const _Kp& __x) const {
    auto r = range(__x);
    return {const_iterator(r.first), const_iterator(r.second)};
  }
};

}} // namespace __ycxx::__adl_free
