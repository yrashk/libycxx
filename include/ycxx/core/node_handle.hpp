// libycxx core: the node handle of the node-based containers ([container.node]) and
// insert-return-type ([container.insert.return]).
//
// node_handle<Node, Allocator, IsMap> owns one container node. Node is the container's node
// type: it keeps its element in a member `value` (a union member, so the node can exist without
// its element); the containers construct the node object with std::construct_at and the element
// through the allocator. Allocator is the container's allocator_type. With IsMap the element is
// a pair<const Key, T> and the handle has key()/mapped(), otherwise value(). Containers with
// compatible nodes (Table 75) use the same Node and Allocator, so they share the handle type.
// The handle destroys the element with allocator_traits<Allocator>::destroy, ends the node's
// lifetime and deallocates it through the allocator rebound to Node ([container.node.dtor]).
// The node is held as rebind_traits<Node>::pointer ([container.node.overview]), so a class-type
// allocator pointer works; the allocator lives in a union member, alive exactly while the
// handle is non-empty. The containers create and empty handles through
// ycxx::detail::node_handle_access: make<NH>(node, alloc), take(nh) (empties nh, returns the
// node) and peek(nh).
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
struct node_handle_access;
}}

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// The key/mapped types of a map node handle and the value type of a set node handle.
template <class V, bool Map>
struct node_handle_types {
  using value_type = V;
};
template <class V>
struct node_handle_types<V, true> {
  using key_type = std::remove_const_t<typename V::first_type>;
  using mapped_type = typename V::second_type;
};

template <class Node, class Alloc, bool IsMap>
class node_handle : public node_handle_types<std::remove_cvref_t<decltype(std::declval<Node&>().value)>, IsMap> {
  using node = Node;
  using V = std::remove_cvref_t<decltype(std::declval<Node&>().value)>;
  using ator_traits = std::allocator_traits<Alloc>;
  using node_traits = typename ator_traits::template rebind_traits<node>;
  using node_pointer = typename node_traits::pointer;
  using node_alloc = typename ator_traits::template rebind_alloc<node>;

  friend struct ::ycxx::detail::node_handle_access;

public:
  using allocator_type = Alloc;

private:
  node_pointer ptr_ = node_pointer();
  // optional<allocator_type> alloc_, without the <optional> dependency: engaged iff ptr_ is
  // not null.
  union {
    Alloc alloc_;
  };

  constexpr node* raw() const noexcept { return ptr_ == nullptr ? nullptr : std::to_address(ptr_); }
  constexpr void destroy_node() noexcept {
    node* n = raw();
    ator_traits::destroy(alloc_, __builtin_addressof(n->value));
    std::destroy_at(n);
    node_alloc na(alloc_);
    node_traits::deallocate(na, ptr_, 1);
  }
  // Leaves *this empty without touching the node.
  constexpr void reset() noexcept {
    if (ptr_ != nullptr) {
      std::destroy_at(__builtin_addressof(alloc_));
      ptr_ = node_pointer();
    }
  }
  constexpr node_handle(node* n, const Alloc& a) noexcept
      : ptr_(::ycxx::detail::to_alloc_pointer<node_pointer>(n)) {
    std::construct_at(__builtin_addressof(alloc_), a);
  }

public:
  // ---- [container.node.cons] ----
  constexpr node_handle() noexcept {}
  constexpr node_handle(node_handle&& nh) noexcept : ptr_(nh.ptr_) {
    if (ptr_ != nullptr) {
      std::construct_at(__builtin_addressof(alloc_), static_cast<Alloc&&>(nh.alloc_));
      std::destroy_at(__builtin_addressof(nh.alloc_));
      nh.ptr_ = node_pointer();
    }
  }
  constexpr node_handle& operator=(node_handle&& nh) noexcept {
    if (this == __builtin_addressof(nh)) {
      // [container.node.cons]/3 applied to one object: the element is destroyed, and the
      // handle ends up empty.
      if (ptr_ != nullptr) {
        destroy_node();
        reset();
      }
      return *this;
    }
    if (ptr_ != nullptr) {
      ::ycxx::detail::precondition(ator_traits::propagate_on_container_move_assignment::value ||
                                       nh.ptr_ == nullptr || alloc_ == nh.alloc_,
                                   "node handle move assignment: unequal allocators that do not propagate");
      destroy_node();
    }
    if (nh.ptr_ == nullptr) {
      reset();
      return *this;
    }
    if (ptr_ == nullptr)
      std::construct_at(__builtin_addressof(alloc_), static_cast<Alloc&&>(nh.alloc_));
    else if constexpr (ator_traits::propagate_on_container_move_assignment::value)
      alloc_ = static_cast<Alloc&&>(nh.alloc_);
    ptr_ = nh.ptr_;
    nh.reset();
    return *this;
  }

  // ---- [container.node.dtor] ----
  constexpr ~node_handle() {
    if (ptr_ != nullptr) {
      destroy_node();
      std::destroy_at(__builtin_addressof(alloc_));
    }
  }

  // ---- [container.node.observers] ----
  constexpr V& value() const noexcept
    requires(!IsMap)
  {
    ::ycxx::detail::precondition(ptr_ != nullptr, "node handle: empty");
    return raw()->value;
  }
  // Not constexpr ([container.node.overview]): the key of a map element is a const object.
  auto& key() const noexcept
    requires IsMap
  {
    ::ycxx::detail::precondition(ptr_ != nullptr, "node handle: empty");
    return const_cast<std::remove_const_t<typename V::first_type>&>(raw()->value.first);
  }
  constexpr auto& mapped() const noexcept
    requires IsMap
  {
    ::ycxx::detail::precondition(ptr_ != nullptr, "node handle: empty");
    return raw()->value.second;
  }
  constexpr allocator_type get_allocator() const {
    ::ycxx::detail::precondition(ptr_ != nullptr, "node handle: empty");
    return alloc_;
  }
  constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }
  [[nodiscard]] constexpr bool empty() const noexcept { return ptr_ == nullptr; }

  // ---- [container.node.modifiers] ----
  constexpr void swap(node_handle& nh) noexcept(ator_traits::propagate_on_container_swap::value ||
                                                ator_traits::is_always_equal::value) {
    if (this == __builtin_addressof(nh))
      return;
    const bool a = ptr_ != nullptr, b = nh.ptr_ != nullptr;
    if (a && b) {
      if constexpr (ator_traits::propagate_on_container_swap::value)
        ::ycxx::detail::swap_adl::do_swap(alloc_, nh.alloc_);
      else
        ::ycxx::detail::precondition(ator_traits::is_always_equal::value || alloc_ == nh.alloc_,
                                     "node handle swap: unequal allocators that do not propagate");
    } else if (a) {
      std::construct_at(__builtin_addressof(nh.alloc_), static_cast<Alloc&&>(alloc_));
      std::destroy_at(__builtin_addressof(alloc_));
    } else if (b) {
      std::construct_at(__builtin_addressof(alloc_), static_cast<Alloc&&>(nh.alloc_));
      std::destroy_at(__builtin_addressof(nh.alloc_));
    }
    const node_pointer t = ptr_;
    ptr_ = nh.ptr_;
    nh.ptr_ = t;
  }
  friend constexpr void swap(node_handle& x, node_handle& y) noexcept(noexcept(x.swap(y))) { x.swap(y); }
};

// [container.insert.return]
template <class Iterator, class NodeType>
struct insert_return_type {
  Iterator position;
  bool inserted;
  NodeType node;
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// How the containers make, inspect and empty node handles.
struct node_handle_access {
  template <class NH, class Node, class Alloc>
  static constexpr NH make(Node* n, const Alloc& a) noexcept {
    return NH(n, a);
  }
  // The node owned by nh (null if empty).
  template <class NH>
  static constexpr auto* peek(const NH& nh) noexcept {
    return nh.raw();
  }
  // Takes the node out of nh, which becomes empty.
  template <class NH>
  static constexpr auto* take(NH& nh) noexcept {
    auto* n = nh.raw();
    nh.reset();
    return n;
  }
};

}} // namespace ycxx::detail
