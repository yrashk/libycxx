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
#include <ycxx/core/compare.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/node_handle.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/seq_support.hpp>
#include <ycxx/core/sorted_tags.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/utility_base.hpp>

namespace ycxx::adl_free {

struct rb_node_base {
  rb_node_base* parent;
  rb_node_base* left;
  rb_node_base* right;
  bool red;
};

template <class V>
struct rb_node : rb_node_base {
  union {
    V value;
  };
  constexpr rb_node() noexcept : rb_node_base{nullptr, nullptr, nullptr, false} {}
  rb_node(const rb_node&) = delete;
  constexpr ~rb_node() {}
};

} // namespace ycxx::adl_free

namespace ycxx::detail {

using rb_base = ::ycxx::adl_free::rb_node_base;

constexpr rb_base* rb_min(rb_base* x) noexcept {
  while (x->left)
    x = x->left;
  return x;
}
constexpr rb_base* rb_max(rb_base* x) noexcept {
  while (x->right)
    x = x->right;
  return x;
}
// In-order successor; the successor of the rightmost node is the header.
constexpr rb_base* rb_next(rb_base* x) noexcept {
  if (x->right)
    return ::ycxx::detail::rb_min(x->right);
  rb_base* p = x->parent;
  // p->parent is null only for the header, whose right link is not a child link.
  while (p->parent && x == p->right) {
    x = p;
    p = p->parent;
  }
  return p;
}
// In-order predecessor; the predecessor of the header is the rightmost node.
constexpr rb_base* rb_prev(rb_base* x) noexcept {
  if (!x->parent)
    return x->right;
  if (x->left)
    return ::ycxx::detail::rb_max(x->left);
  rb_base* p = x->parent;
  while (x == p->left) {
    x = p;
    p = p->parent;
  }
  return p;
}

// Rotations. The root is the header's left child, so the parent link update needs no special
// case for it.
constexpr void rb_rotate_left(rb_base* x) noexcept {
  rb_base* y = x->right;
  x->right = y->left;
  if (y->left)
    y->left->parent = x;
  y->parent = x->parent;
  if (x == x->parent->left)
    x->parent->left = y;
  else
    x->parent->right = y;
  y->left = x;
  x->parent = y;
}
constexpr void rb_rotate_right(rb_base* x) noexcept {
  rb_base* y = x->left;
  x->left = y->right;
  if (y->right)
    y->right->parent = x;
  y->parent = x->parent;
  if (x == x->parent->left)
    x->parent->left = y;
  else
    x->parent->right = y;
  y->right = x;
  x->parent = y;
}

// Links z as the left or right child of parent (the header: z becomes the root of an empty
// tree), maintains the leftmost node `first` and the rightmost node (header->right), and
// restores the red-black properties.
constexpr void rb_insert(rb_base* z, rb_base* parent, bool left, rb_base* header, rb_base*& first) noexcept {
  z->parent = parent;
  z->left = z->right = nullptr;
  z->red = true;
  if (parent == header) {
    header->left = header->right = z;
    first = z;
  } else if (left) {
    parent->left = z;
    if (parent == first)
      first = z;
  } else {
    parent->right = z;
    if (parent == header->right)
      header->right = z;
  }
  // A red parent is not the root (the root is black), so the grandparent is a node.
  while (z != header->left && z->parent->red) {
    rb_base* p = z->parent;
    rb_base* g = p->parent;
    if (p == g->left) {
      rb_base* u = g->right;
      if (u && u->red) {
        p->red = u->red = false;
        g->red = true;
        z = g;
      } else {
        if (z == p->right) {
          z = p;
          ::ycxx::detail::rb_rotate_left(z);
          p = z->parent;
        }
        p->red = false;
        g->red = true;
        ::ycxx::detail::rb_rotate_right(g);
      }
    } else {
      rb_base* u = g->left;
      if (u && u->red) {
        p->red = u->red = false;
        g->red = true;
        z = g;
      } else {
        if (z == p->left) {
          z = p;
          ::ycxx::detail::rb_rotate_right(z);
          p = z->parent;
        }
        p->red = false;
        g->red = true;
        ::ycxx::detail::rb_rotate_left(g);
      }
    }
  }
  header->left->red = false;
}

// Replaces the subtree rooted at u by the one rooted at v (possibly null) in u's parent.
constexpr void rb_transplant(rb_base* u, rb_base* v) noexcept {
  if (u == u->parent->left)
    u->parent->left = v;
  else
    u->parent->right = v;
  if (v)
    v->parent = u->parent;
}

// Unlinks z, which is not the only node, maintaining the leftmost node `first` and the
// rightmost node and restoring the red-black properties. Other nodes keep their addresses.
constexpr void rb_erase(rb_base* z, rb_base* header, rb_base*& first) noexcept {
  if (z == first)
    first = ::ycxx::detail::rb_next(z);
  if (z == header->right)
    header->right = ::ycxx::detail::rb_prev(z);
  rb_base* x;  // the node that moves into the removed position (may be null)
  rb_base* xp; // its parent
  bool removed_red = z->red;
  if (!z->left) {
    x = z->right;
    xp = z->parent;
    ::ycxx::detail::rb_transplant(z, z->right);
  } else if (!z->right) {
    x = z->left;
    xp = z->parent;
    ::ycxx::detail::rb_transplant(z, z->left);
  } else {
    rb_base* y = ::ycxx::detail::rb_min(z->right);
    removed_red = y->red;
    x = y->right;
    if (y->parent == z) {
      xp = y;
    } else {
      xp = y->parent;
      ::ycxx::detail::rb_transplant(y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }
    ::ycxx::detail::rb_transplant(z, y);
    y->left = z->left;
    y->left->parent = y;
    y->red = z->red;
  }
  if (removed_red)
    return;
  // x carries an extra black. Its sibling w exists: the removed black node had a black height
  // of at least one on the sibling's side.
  while (x != header->left && (!x || !x->red)) {
    if (x == xp->left) {
      rb_base* w = xp->right;
      if (w->red) {
        w->red = false;
        xp->red = true;
        ::ycxx::detail::rb_rotate_left(xp);
        w = xp->right;
      }
      if ((!w->left || !w->left->red) && (!w->right || !w->right->red)) {
        w->red = true;
        x = xp;
        xp = xp->parent;
      } else {
        if (!w->right || !w->right->red) {
          w->left->red = false;
          w->red = true;
          ::ycxx::detail::rb_rotate_right(w);
          w = xp->right;
        }
        w->red = xp->red;
        xp->red = false;
        if (w->right)
          w->right->red = false;
        ::ycxx::detail::rb_rotate_left(xp);
        x = header->left;
        break;
      }
    } else {
      rb_base* w = xp->left;
      if (w->red) {
        w->red = false;
        xp->red = true;
        ::ycxx::detail::rb_rotate_right(xp);
        w = xp->left;
      }
      if ((!w->left || !w->left->red) && (!w->right || !w->right->red)) {
        w->red = true;
        x = xp;
        xp = xp->parent;
      } else {
        if (!w->left || !w->left->red) {
          w->right->red = false;
          w->red = true;
          ::ycxx::detail::rb_rotate_left(w);
          w = xp->left;
        }
        w->red = xp->red;
        xp->red = false;
        if (w->left)
          w->left->red = false;
        ::ycxx::detail::rb_rotate_right(xp);
        x = header->left;
        break;
      }
    }
  }
  if (x)
    x->red = false;
}

template <class Compare>
concept transparent_compare = requires { typename Compare::is_transparent; };

// [associative.reqmts.general]/180: the heterogeneous erase and extract.
template <class Compare, class K, class It, class CIt>
concept transparent_non_iter =
    transparent_compare<Compare> && !std::is_convertible_v<K&&, It> && !std::is_convertible_v<K&&, CIt>;

// The key of the element a set (IsMap false) or map would build from args, when it can be read
// from the arguments without constructing an element: a single key argument (set), a key first
// argument of two (map), or a pair whose first member is a key (map). A key argument is a
// key_type, or an arithmetic value when key_type is arithmetic (converted as the element's
// construction converts it; such a temporary key is not observable).
template <class Key, class A>
inline constexpr bool key_like = std::is_same_v<std::remove_cvref_t<A>, Key> ||
                                 (std::is_arithmetic_v<Key> && std::is_arithmetic_v<std::remove_cvref_t<A>>);
template <class P>
struct pair_first {
  using type = void;
};
template <class X, class Y>
struct pair_first<std::pair<X, Y>> {
  using type = X;
};
template <class Key, bool IsMap, class... Args>
inline constexpr bool key_in_args = false;
template <class Key, class A>
inline constexpr bool key_in_args<Key, false, A> = key_like<Key, A>;
template <class Key, class A, class B>
inline constexpr bool key_in_args<Key, true, A, B> = key_like<Key, A>;
template <class Key, class P>
  requires(!std::is_void_v<typename pair_first<std::remove_cvref_t<P>>::type>)
inline constexpr bool key_in_args<Key, true, P> = key_like<Key, typename pair_first<std::remove_cvref_t<P>>::type>;

template <class Key, class A>
constexpr decltype(auto) as_key(const A& a) noexcept {
  if constexpr (std::is_same_v<A, Key>)
    return a;
  else
    return static_cast<Key>(a);
}
template <class Key, bool IsMap, class A, class... Rest>
constexpr decltype(auto) key_arg(const A& a, const Rest&...) noexcept {
  if constexpr (IsMap && sizeof...(Rest) == 0)
    return ::ycxx::detail::as_key<Key>(a.first);
  else
    return ::ycxx::detail::as_key<Key>(a);
}

} // namespace ycxx::detail

namespace ycxx::adl_free {

// T is the element type, possibly const.
template <class T, class Diff>
class rb_iter {
  using V = std::remove_const_t<T>;
  using base = rb_node_base;
  base* n_ = nullptr;

  template <class, class>
  friend class rb_iter;
  template <class, class, class, class, bool, bool>
  friend class rb_tree;

public:
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = V;
  using difference_type = Diff;
  using pointer = T*;
  using reference = T&;

  constexpr rb_iter() noexcept = default;
  constexpr explicit rb_iter(base* n) noexcept : n_(n) {}
  template <class U>
    requires std::is_same_v<const U, T> && (!std::is_same_v<U, T>)
  constexpr rb_iter(const rb_iter<U, Diff>& o) noexcept : n_(o.n_) {}

  constexpr reference operator*() const noexcept { return static_cast<rb_node<V>*>(n_)->value; }
  constexpr pointer operator->() const noexcept { return __builtin_addressof(static_cast<rb_node<V>*>(n_)->value); }
  constexpr rb_iter& operator++() noexcept {
    n_ = ::ycxx::detail::rb_next(n_);
    return *this;
  }
  constexpr rb_iter operator++(int) noexcept {
    rb_iter t = *this;
    n_ = ::ycxx::detail::rb_next(n_);
    return t;
  }
  constexpr rb_iter& operator--() noexcept {
    n_ = ::ycxx::detail::rb_prev(n_);
    return *this;
  }
  constexpr rb_iter operator--(int) noexcept {
    rb_iter t = *this;
    n_ = ::ycxx::detail::rb_prev(n_);
    return t;
  }
  friend constexpr bool operator==(const rb_iter& a, const rb_iter& b) noexcept { return a.n_ == b.n_; }
};

// The common part of map, multimap (IsMap: V is pair<const Key, T>), set and multiset (V is
// Key; iterator is const_iterator). Key is passed separately so that V is not instantiated:
// map<K, T> may be named while T is incomplete. Multi: equivalent keys. The public members here are those
// the four containers share with identical semantics; the containers add constructors,
// assignment, insertion and their own members, built on the protected interface.
template <class Key, class V, class Compare, class Allocator, bool IsMap, bool Multi>
class rb_tree {
  template <class, class, class, class, bool, bool>
  friend class rb_tree;

  using info = ::ycxx::detail::alloc_info<Allocator>;
  using alloc_traits = std::allocator_traits<Allocator>;

protected:
  using node = rb_node<V>;
  using node_base = rb_node_base;
  using node_alloc = typename info::template rebind<node>;
  using node_traits = std::allocator_traits<node_alloc>;

public:
  using key_type = Key;
  using value_type = V;
  using key_compare = Compare;
  using allocator_type = Allocator;
  using pointer = typename info::pointer;
  using const_pointer = typename info::const_pointer;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = typename info::size_type;
  using difference_type = typename info::difference_type;
  using iterator = rb_iter<std::conditional_t<IsMap, V, const V>, difference_type>;
  using const_iterator = rb_iter<const V, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using node_type = node_handle<node, Allocator, IsMap>;

protected:
  static constexpr bool pocca = info::pocca;
  static constexpr bool pocma = info::pocma;
  static constexpr bool pocs = info::pocs;
  static constexpr bool always_equal = info::always_equal;

  node_base head_ = {nullptr, nullptr, nullptr, false};
  node_base* hdr_ = nullptr;   // the header (end()), or null before the first insertion
  node_base* first_ = nullptr; // the leftmost node (begin()); the header when empty
  size_type size_ = 0;
  [[no_unique_address]] Compare comp_;
  [[no_unique_address]] node_alloc na_;

  // ---- nodes ----
  static constexpr node_base* nd(const_iterator it) noexcept { return it.n_; }
  static constexpr V& value(node_base* n) noexcept { return static_cast<node*>(n)->value; }
  static constexpr const key_type& key(node_base* n) noexcept {
    if constexpr (IsMap)
      return static_cast<node*>(n)->value.first;
    else
      return static_cast<node*>(n)->value;
  }
  template <class A, class B>
  constexpr bool lt(const A& a, const B& b) const {
    return static_cast<bool>(comp_(a, b));
  }

  constexpr node_base* header() {
    if (!hdr_) {
      if consteval {
        hdr_ = std::allocator<node_base>().allocate(1);
        std::construct_at(hdr_, node_base{nullptr, nullptr, nullptr, false});
      } else {
        hdr_ = __builtin_addressof(head_);
      }
      hdr_->right = first_ = hdr_;
    }
    return hdr_;
  }
  constexpr node_base* root() const noexcept { return hdr_ ? hdr_->left : nullptr; }
  constexpr node_base* begin_node() const noexcept { return first_; }
  constexpr node_base* end_node() const noexcept { return hdr_; }
  constexpr node_base* last_node() const noexcept { return hdr_->right; }

  template <class... Args>
  constexpr node* make_node(Args&&... args) {
    if (size_ == max_size())
      ::ycxx::detail::throw_length_error("associative container: size would exceed max_size()");
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
  constexpr void free_node(node_base* b) noexcept {
    node* n = static_cast<node*>(b);
    node_traits::destroy(na_, __builtin_addressof(n->value));
    std::destroy_at(n);
    node_traits::deallocate(na_, ::ycxx::detail::to_alloc_pointer<typename node_traits::pointer>(n), 1);
  }
  // Frees a node that is not (or no longer) linked, unless released: rollback on exceptions.
  struct node_guard {
    rb_tree* t;
    node_base* n;
    constexpr ~node_guard() {
      if (n)
        t->free_node(n);
    }
    constexpr node_base* release() noexcept {
      node_base* r = n;
      n = nullptr;
      return r;
    }
  };
  // Frees the subtree rooted at x. Recursion depth is the tree height.
  constexpr void destroy_subtree(node_base* x) noexcept {
    while (x) {
      destroy_subtree(x->right);
      node_base* l = x->left;
      free_node(x);
      x = l;
    }
  }

  // ---- linking ----
  struct pos {
    node_base* parent;   // the node to link below (the header for an empty tree)
    bool left;           // as its left child
    node_base* existing; // or, for unique keys, the element with an equivalent key
  };
  constexpr void link(node_base* n, pos p) noexcept {
    ::ycxx::detail::rb_insert(n, p.parent, p.left, hdr_, first_);
    ++size_;
  }
  // Unlinks n without freeing it.
  constexpr void unlink(node_base* n) noexcept {
    if (size_ == 1) {
      hdr_->left = nullptr;
      hdr_->right = first_ = hdr_;
    } else {
      ::ycxx::detail::rb_erase(n, hdr_, first_);
    }
    --size_;
  }

  // The position of a new element with key k: after the elements whose keys are not greater
  // (upper), or before those not less (lower).
  template <class K>
  constexpr pos pos_upper(const K& k) {
    node_base* y = header();
    node_base* x = y->left;
    bool go_left = true;
    while (x) {
      y = x;
      go_left = lt(k, key(x));
      x = go_left ? x->left : x->right;
    }
    return {y, go_left, nullptr};
  }
  template <class K>
  constexpr pos pos_lower(const K& k) {
    node_base* y = header();
    node_base* x = y->left;
    bool go_left = true;
    while (x) {
      y = x;
      go_left = !lt(key(x), k);
      x = go_left ? x->left : x->right;
    }
    return {y, go_left, nullptr};
  }
  // Unique keys: the position for key k, or the element with an equivalent key.
  template <class K>
  constexpr pos pos_unique(const K& k) {
    // The lower bound: of several elements equivalent to a heterogeneous k, the first is found.
    pos p = pos_lower(k);
    node_base* lb = p.left ? p.parent : ::ycxx::detail::rb_next(p.parent);
    if (lb != hdr_ && !lt(k, key(lb)))
      return {nullptr, false, lb};
    return p;
  }
  // Between the adjacent nodes a (or begin) and b (or end): the free child link.
  constexpr pos between(node_base* a, node_base* b) noexcept {
    if (a->right == nullptr)
      return {a, false, nullptr};
    return {b, true, nullptr};
  }
  // Unique keys with a hint: constant time when k belongs just before or just after hint.
  template <class K>
  constexpr pos pos_unique_hint(node_base* hint, const K& k) {
    node_base* h = header();
    if (!hint)
      hint = h;
    if (hint == h) {
      if (size_ > 0 && lt(key(h->right), k))
        return {h->right, false, nullptr};
      return pos_unique(k);
    }
    if (lt(k, key(hint))) {
      if (hint == first_)
        return {hint, true, nullptr};
      node_base* before = ::ycxx::detail::rb_prev(hint);
      if (lt(key(before), k))
        return between(before, hint);
      return pos_unique(k);
    }
    if (lt(key(hint), k)) {
      if (hint == h->right)
        return {hint, false, nullptr};
      node_base* after = ::ycxx::detail::rb_next(hint);
      if (lt(k, key(after)))
        return between(hint, after);
      return pos_unique(k);
    }
    return {nullptr, false, hint};
  }
  // Equivalent keys with a hint: as close as possible to the position just before hint.
  template <class K>
  constexpr pos pos_multi_hint(node_base* hint, const K& k) {
    node_base* h = header();
    if (!hint)
      hint = h;
    if (hint == h) {
      if (size_ == 0)
        return {h, true, nullptr};
      if (!lt(k, key(h->right)))
        return {h->right, false, nullptr};
      return pos_upper(k);
    }
    if (!lt(key(hint), k)) { // k <= *hint
      if (hint == first_)
        return {hint, true, nullptr};
      node_base* before = ::ycxx::detail::rb_prev(hint);
      if (!lt(k, key(before)))
        return between(before, hint);
      return pos_upper(k);
    }
    // *hint < k: the closest position is the start of k's equivalents.
    if (hint == h->right)
      return {hint, false, nullptr};
    node_base* after = ::ycxx::detail::rb_next(hint);
    if (!lt(key(after), k))
      return between(hint, after);
    return pos_lower(k);
  }

  // ---- insertion ----
  // Unique keys: inserts an element built from args unless its key is present; returns the
  // element with that key and whether it was inserted.
  template <class... Args>
  constexpr std::pair<node_base*, bool> emplace_unique(Args&&... args) {
    if constexpr (::ycxx::detail::key_in_args<key_type, IsMap, Args...>) {
      pos p = pos_unique(::ycxx::detail::key_arg<key_type, IsMap>(args...));
      if (p.existing)
        return {p.existing, false};
      node* n = make_node(static_cast<Args&&>(args)...);
      link(n, p);
      return {n, true};
    } else {
      node_guard g{this, make_node(static_cast<Args&&>(args)...)};
      pos p = pos_unique(key(g.n));
      if (p.existing)
        return {p.existing, false};
      node_base* n = g.release();
      link(n, p);
      return {n, true};
    }
  }
  template <class... Args>
  constexpr node_base* emplace_hint_unique(node_base* hint, Args&&... args) {
    if constexpr (::ycxx::detail::key_in_args<key_type, IsMap, Args...>) {
      pos p = pos_unique_hint(hint, ::ycxx::detail::key_arg<key_type, IsMap>(args...));
      if (p.existing)
        return p.existing;
      node* n = make_node(static_cast<Args&&>(args)...);
      link(n, p);
      return n;
    } else {
      node_guard g{this, make_node(static_cast<Args&&>(args)...)};
      pos p = pos_unique_hint(hint, key(g.n));
      if (p.existing)
        return p.existing;
      node_base* n = g.release();
      link(n, p);
      return n;
    }
  }
  template <class... Args>
  constexpr node_base* emplace_multi(Args&&... args) {
    node_guard g{this, make_node(static_cast<Args&&>(args)...)};
    pos p = pos_upper(key(g.n));
    node_base* n = g.release();
    link(n, p);
    return n;
  }
  template <class... Args>
  constexpr node_base* emplace_hint_multi(node_base* hint, Args&&... args) {
    node_guard g{this, make_node(static_cast<Args&&>(args)...)};
    pos p = pos_multi_hint(hint, key(g.n));
    node_base* n = g.release();
    link(n, p);
    return n;
  }
  template <class... Args>
  constexpr node_base* emplace_any(Args&&... args) {
    if constexpr (Multi)
      return emplace_multi(static_cast<Args&&>(args)...);
    else
      return emplace_unique(static_cast<Args&&>(args)...).first;
  }
  template <class... Args>
  constexpr node_base* emplace_hint_any(node_base* hint, Args&&... args) {
    if constexpr (Multi)
      return emplace_hint_multi(hint, static_cast<Args&&>(args)...);
    else
      return emplace_hint_unique(hint, static_cast<Args&&>(args)...);
  }
  // Unique keys, the key known up front (try_emplace, the heterogeneous insertions): builds the
  // element from args only if no element has a key equivalent to k. hint may be null.
  template <class K, class... Args>
  constexpr std::pair<node_base*, bool> emplace_key(node_base* hint, const K& k, Args&&... args) {
    pos p = hint ? pos_unique_hint(hint, k) : pos_unique(k);
    if (p.existing)
      return {p.existing, false};
    node* n = make_node(static_cast<Args&&>(args)...);
    link(n, p);
    return {n, true};
  }
  // Inserts every element of [first, last), each with the hint end(): linear for sorted input.
  template <class It, class Sent>
  constexpr void insert_elems(It first, Sent last) {
    for (; first != last; ++first)
      emplace_hint_any(header(), *first);
  }

  // ---- node handles ----
  constexpr node_type make_handle(node_base* n) noexcept {
    return ::ycxx::detail::node_handle_access::make<node_type>(static_cast<node*>(n), allocator_type(na_));
  }
  constexpr void check_handle(const node_type& nh) const noexcept {
    if constexpr (!always_equal)
      ::ycxx::detail::precondition(allocator_type(na_) == nh.get_allocator(),
                                   "associative container: node handle with an unequal allocator");
  }
  constexpr node_base* insert_handle_multi(node_base* hint, node_type& nh) {
    if (nh.empty())
      return hdr_;
    check_handle(nh);
    node_base* n = ::ycxx::detail::node_handle_access::peek(nh);
    pos p = hint ? pos_multi_hint(hint, key(n)) : pos_upper(key(n));
    ::ycxx::detail::node_handle_access::take(nh);
    link(n, p);
    return n;
  }
  // Unique keys: links nh's node unless its key is present; returns the element with its key.
  constexpr std::pair<node_base*, bool> insert_handle_unique(node_base* hint, node_type& nh) {
    if (nh.empty())
      return {hdr_, false};
    check_handle(nh);
    node_base* n = ::ycxx::detail::node_handle_access::peek(nh);
    pos p = hint ? pos_unique_hint(hint, key(n)) : pos_unique(key(n));
    if (p.existing)
      return {p.existing, false};
    ::ycxx::detail::node_handle_access::take(nh);
    link(n, p);
    return {n, true};
  }

  // ---- whole-tree operations ----
  struct shape {
    node_base* root = nullptr;
    node_base* first = nullptr;
    node_base* last = nullptr;
    size_type n = 0;
  };
  // The nodes of *this, detached: *this becomes empty.
  constexpr shape detach() noexcept {
    if (size_ == 0)
      return {};
    shape s{hdr_->left, first_, hdr_->right, size_};
    hdr_->left = nullptr;
    hdr_->right = first_ = hdr_;
    size_ = 0;
    return s;
  }
  // Takes the nodes of s; *this must be empty.
  constexpr void attach(shape s) {
    if (s.n == 0)
      return;
    node_base* h = header();
    h->left = s.root;
    s.root->parent = h;
    first_ = s.first;
    h->right = s.last;
    size_ = s.n;
  }
  constexpr void take(rb_tree& o) noexcept { attach(o.detach()); }

  // A copy of the subtree src below parent, each value built by gen(value).
  template <class Gen>
  constexpr node_base* clone(node_base* src, node_base* parent, Gen& gen) {
    node_base* top = gen(value(src));
    top->red = src->red;
    top->parent = parent;
    top->left = top->right = nullptr;
    ::ycxx::detail::rollback rb{[&] { destroy_subtree(top); }};
    if (src->right)
      top->right = clone(src->right, top, gen);
    parent = top;
    for (src = src->left; src; src = src->left) {
      node_base* y = gen(value(src));
      y->red = src->red;
      y->left = y->right = nullptr;
      y->parent = parent;
      parent->left = y;
      if (src->right)
        y->right = clone(src->right, y, gen);
      parent = y;
    }
    rb.release();
    return top;
  }
  // Fills the empty *this with the elements of o in o's shape, each built by gen(value).
  template <class Gen>
  constexpr void clone_from(const rb_tree& o, Gen gen) {
    if (o.size_ == 0)
      return;
    node_base* r = clone(o.hdr_->left, nullptr, gen);
    shape s{r, ::ycxx::detail::rb_min(r), ::ycxx::detail::rb_max(r), o.size_};
    attach(s);
  }
  constexpr void copy_from(const rb_tree& o) {
    clone_from(o, [this](const V& v) -> node_base* { return make_node(v); });
  }
  // Element-wise move: for maps, key_type and mapped_type are moved ([associative.reqmts.general]/8
  // puts the move-insertable requirement on them), so a move-only key is moved out of the source
  // node, whose element is destroyed afterwards without being compared again. During constant
  // evaluation a copyable key is copied instead.
  constexpr void move_from(rb_tree& o) {
    clone_from(o, [this](V& v) -> node_base* {
      if constexpr (IsMap) {
        using M = typename V::second_type;
        if constexpr (std::is_copy_constructible_v<Key>) {
          if consteval {
            return make_node(v.first, static_cast<M&&>(v.second));
          }
        }
        return make_node(static_cast<Key&&>(const_cast<Key&>(v.first)), static_cast<M&&>(v.second));
      } else {
        return make_node(static_cast<V&&>(v));
      }
    });
  }

  // ---- construction and assignment ----
  // noexcept strengthenings: default construction and the allocator-extended move with an
  // always-equal allocator allocate nothing.
  static constexpr bool nothrow_default = std::is_nothrow_default_constructible_v<Compare> &&
                                          std::is_nothrow_copy_constructible_v<Compare> &&
                                          std::is_nothrow_default_constructible_v<Allocator>;
  static constexpr bool nothrow_move_alloc = always_equal && std::is_nothrow_copy_constructible_v<Compare>;

  constexpr rb_tree(const Compare& c, const Allocator& a) noexcept(std::is_nothrow_copy_constructible_v<Compare>)
      : comp_(c), na_(a) {}
  constexpr rb_tree(const rb_tree& o, const Allocator& a) : comp_(o.comp_), na_(a) { copy_from(o); }
  // The comparison object is copied: the source stays usable.
  constexpr rb_tree(rb_tree&& o) noexcept(std::is_nothrow_copy_constructible_v<Compare>)
      : comp_(o.comp_), na_(static_cast<node_alloc&&>(o.na_)) {
    take(o);
  }
  constexpr rb_tree(rb_tree&& o, const Allocator& a) noexcept(nothrow_move_alloc) : comp_(o.comp_), na_(a) {
    if (always_equal || na_ == o.na_)
      take(o);
    else
      move_from(o);
  }
  constexpr ~rb_tree() {
    clear();
    if (hdr_ && hdr_ != __builtin_addressof(head_)) {
      std::destroy_at(hdr_);
      std::allocator<node_base>().deallocate(hdr_, 1);
    }
  }

  constexpr void copy_assign(const rb_tree& o) {
    if (this == __builtin_addressof(o))
      return;
    clear();
    if constexpr (pocca)
      na_ = o.na_;
    comp_ = o.comp_;
    copy_from(o);
  }
  constexpr void move_assign(rb_tree& o) {
    if (this == __builtin_addressof(o))
      return;
    clear();
    comp_ = static_cast<Compare&&>(o.comp_);
    if constexpr (pocma) {
      na_ = static_cast<node_alloc&&>(o.na_);
      take(o);
    } else if (always_equal || na_ == o.na_) {
      take(o);
    } else {
      move_from(o);
    }
  }
  constexpr void swap_tree(rb_tree& o) {
    if (this == __builtin_addressof(o))
      return;
    ::ycxx::detail::swap_adl::do_swap(comp_, o.comp_);
    if constexpr (pocs)
      ::ycxx::detail::swap_adl::do_swap(na_, o.na_);
    else
      ::ycxx::detail::precondition(always_equal || na_ == o.na_,
                                   "associative container swap: unequal allocators that do not propagate");
    const shape a = detach(), b = o.detach();
    attach(b);
    o.attach(a);
  }

  // Moves the elements of source into *this ([associative.reqmts.general]/112-117); with
  // unique keys, those whose keys are present stay behind.
  template <class C2, bool M2>
  constexpr void merge_from(rb_tree<Key, V, C2, Allocator, IsMap, M2>& source) {
    if (static_cast<void*>(this) == static_cast<void*>(__builtin_addressof(source)) || source.size_ == 0)
      return;
    if constexpr (!always_equal)
      ::ycxx::detail::precondition(allocator_type(na_) == allocator_type(source.na_),
                                   "associative container merge: unequal allocators");
    for (node_base* p = source.first_; p != source.hdr_;) {
      node_base* nx = ::ycxx::detail::rb_next(p);
      if constexpr (Multi) {
        pos at = pos_upper(key(p));
        source.unlink(p);
        link(p, at);
      } else {
        pos at = pos_unique(key(p));
        if (!at.existing) {
          source.unlink(p);
          link(p, at);
        }
      }
      p = nx;
    }
  }

  // ---- lookup ----
  template <class K>
  constexpr node_base* lower(const K& k) const {
    node_base* y = hdr_;
    for (node_base* x = root(); x;) {
      if (!lt(key(x), k)) {
        y = x;
        x = x->left;
      } else {
        x = x->right;
      }
    }
    return y;
  }
  template <class K>
  constexpr node_base* upper(const K& k) const {
    node_base* y = hdr_;
    for (node_base* x = root(); x;) {
      if (lt(k, key(x))) {
        y = x;
        x = x->left;
      } else {
        x = x->right;
      }
    }
    return y;
  }
  template <class K>
  constexpr node_base* find_node(const K& k) const {
    node_base* n = lower(k);
    if (n == hdr_ || lt(k, key(n)))
      return hdr_;
    return n;
  }
  template <class K>
  constexpr std::pair<node_base*, node_base*> range(const K& k) const {
    return {lower(k), upper(k)};
  }
  template <class K>
  constexpr size_type count_key(const K& k) const {
    std::pair<node_base*, node_base*> r = range(k);
    size_type n = 0;
    for (node_base* p = r.first; p != r.second; p = ::ycxx::detail::rb_next(p))
      ++n;
    return n;
  }
  template <class K>
  constexpr size_type erase_key(const K& k) {
    std::pair<node_base*, node_base*> r = range(k);
    size_type n = 0;
    for (node_base* p = r.first; p != r.second; ++n) {
      node_base* nx = ::ycxx::detail::rb_next(p);
      unlink(p);
      free_node(p);
      p = nx;
    }
    return n;
  }
  template <class K>
  constexpr node_type extract_key(const K& k) {
    node_base* n = find_node(k);
    if (n == hdr_)
      return node_type();
    unlink(n);
    return make_handle(n);
  }

public:
  constexpr allocator_type get_allocator() const noexcept { return allocator_type(na_); }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(first_); }
  constexpr const_iterator begin() const noexcept { return const_iterator(first_); }
  constexpr iterator end() noexcept { return iterator(hdr_); }
  constexpr const_iterator end() const noexcept { return const_iterator(hdr_); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- capacity ----
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type max_size() const noexcept {
    const auto a = static_cast<size_type>(node_traits::max_size(na_));
    const auto d = static_cast<size_type>(std::numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }

  // ---- modifiers ----
  constexpr node_type extract(const_iterator position) {
    ::ycxx::detail::precondition(position.n_ != hdr_, "associative container extract: end() iterator");
    unlink(position.n_);
    return make_handle(position.n_);
  }
  constexpr node_type extract(const key_type& x) { return extract_key(x); }
  template <class K>
    requires ::ycxx::detail::transparent_non_iter<Compare, K, iterator, const_iterator>
  constexpr node_type extract(K&& x) {
    return extract_key(x);
  }

  constexpr iterator erase(iterator position)
    requires(IsMap)
  {
    return erase(const_iterator(position));
  }
  constexpr iterator erase(const_iterator position) {
    ::ycxx::detail::precondition(position.n_ != hdr_, "associative container erase: end() iterator");
    node_base* n = position.n_;
    node_base* nx = ::ycxx::detail::rb_next(n);
    unlink(n);
    free_node(n);
    return iterator(nx);
  }
  constexpr size_type erase(const key_type& x) { return erase_key(x); }
  template <class K>
    requires ::ycxx::detail::transparent_non_iter<Compare, K, iterator, const_iterator>
  constexpr size_type erase(K&& x) {
    return erase_key(x);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    if (first.n_ == first_ && last.n_ == hdr_) {
      clear();
      return end();
    }
    node_base* p = first.n_;
    while (p != last.n_) {
      node_base* nx = ::ycxx::detail::rb_next(p);
      unlink(p);
      free_node(p);
      p = nx;
    }
    return iterator(last.n_);
  }
  constexpr void clear() noexcept {
    if (size_ == 0)
      return;
    destroy_subtree(hdr_->left);
    hdr_->left = nullptr;
    hdr_->right = first_ = hdr_;
    size_ = 0;
  }

  // ---- observers ----
  constexpr key_compare key_comp() const { return comp_; }

  // ---- lookup ----
  constexpr iterator find(const key_type& x) { return iterator(find_node(x)); }
  constexpr const_iterator find(const key_type& x) const { return const_iterator(find_node(x)); }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr iterator find(const K& x) {
    return iterator(find_node(x));
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr const_iterator find(const K& x) const {
    return const_iterator(find_node(x));
  }
  constexpr size_type count(const key_type& x) const {
    if constexpr (Multi)
      return count_key(x);
    else
      return find_node(x) == hdr_ ? 0 : 1;
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr size_type count(const K& x) const {
    return count_key(x);
  }
  constexpr bool contains(const key_type& x) const { return find_node(x) != hdr_; }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr bool contains(const K& x) const {
    return find_node(x) != hdr_;
  }
  constexpr iterator lower_bound(const key_type& x) { return iterator(lower(x)); }
  constexpr const_iterator lower_bound(const key_type& x) const { return const_iterator(lower(x)); }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr iterator lower_bound(const K& x) {
    return iterator(lower(x));
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr const_iterator lower_bound(const K& x) const {
    return const_iterator(lower(x));
  }
  constexpr iterator upper_bound(const key_type& x) { return iterator(upper(x)); }
  constexpr const_iterator upper_bound(const key_type& x) const { return const_iterator(upper(x)); }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr iterator upper_bound(const K& x) {
    return iterator(upper(x));
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr const_iterator upper_bound(const K& x) const {
    return const_iterator(upper(x));
  }
  constexpr std::pair<iterator, iterator> equal_range(const key_type& x) {
    auto r = range(x);
    return {iterator(r.first), iterator(r.second)};
  }
  constexpr std::pair<const_iterator, const_iterator> equal_range(const key_type& x) const {
    auto r = range(x);
    return {const_iterator(r.first), const_iterator(r.second)};
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr std::pair<iterator, iterator> equal_range(const K& x) {
    auto r = range(x);
    return {iterator(r.first), iterator(r.second)};
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr std::pair<const_iterator, const_iterator> equal_range(const K& x) const {
    auto r = range(x);
    return {const_iterator(r.first), const_iterator(r.second)};
  }
};

} // namespace ycxx::adl_free

namespace ycxx::detail {

// [associative.general]/2 and the deduction-guide constraints ([associative.reqmts.general]/181).
template <class I>
using iter_key_type = std::remove_cvref_t<std::tuple_element_t<0, iter_value_type<I>>>;
template <class I>
using iter_mapped_type = std::remove_cvref_t<std::tuple_element_t<1, iter_value_type<I>>>;
template <class I>
using iter_to_alloc_type = std::pair<const iter_key_type<I>, iter_mapped_type<I>>;
template <class R>
using range_key_type = std::remove_cvref_t<std::tuple_element_t<0, std::ranges::range_value_t<R>>>;
template <class R>
using range_mapped_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<R>>>;
template <class R>
using range_to_alloc_type = std::pair<const range_key_type<R>, range_mapped_type<R>>;

template <class C>
concept deducible_compare = !qualifies_as_allocator<C>;

} // namespace ycxx::detail
