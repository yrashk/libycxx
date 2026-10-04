// libycxx core: the node handle ([container.node]) and insert-return-type
// ([container.insert.return]) of the node-based associative containers.
//
// node_handle<Node, Allocator, IsMap> owns one container node: Node is the container's node
// type, which keeps its element in a member `value` (a union member, so the node can exist
// without its element); Allocator is the container's allocator_type. With IsMap the element is
// a pair<const Key, T> and the handle exposes key()/mapped(), otherwise value(). Containers with
// compatible nodes (Table 75) use the same Node and Allocator, so they share the handle type.
// The element is destroyed and the node freed through the allocator rebound to Node, the
// allocator the container built them with. The handle holds the node as a raw pointer and the
// allocator in a union member, alive exactly while the handle is non-empty.
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/seq_support.hpp>
#include <ycxx/core/swap.hpp>

namespace ycxx::detail {

// The containers' access to a handle's node: take(nh) empties nh and returns its node; make
// builds a handle owning n, allocated with an allocator equal to a.
struct node_handle_access {
  template <class NH>
  static constexpr auto* take(NH& nh) noexcept {
    auto* n = nh.ptr_;
    if (n) {
      std::destroy_at(__builtin_addressof(nh.alloc_));
      nh.ptr_ = nullptr;
    }
    return n;
  }
  template <class NH, class Node, class A>
  static constexpr NH make(Node* n, const A& a) noexcept {
    NH nh;
    std::construct_at(__builtin_addressof(nh.alloc_), a);
    nh.ptr_ = n;
    return nh;
  }
  template <class NH>
  static constexpr auto* peek(const NH& nh) noexcept {
    return nh.ptr_;
  }
};

} // namespace ycxx::detail

namespace ycxx::adl_free {

// The member types of a map node handle (key_type, mapped_type) or a set node handle
// (value_type).
template <class V, bool IsMap>
struct node_handle_types {
  using value_type = V;
};
template <class V>
struct node_handle_types<V, true> {
  using key_type = std::remove_const_t<typename V::first_type>;
  using mapped_type = typename V::second_type;
};

template <class Node, class Allocator, bool IsMap>
class node_handle : public node_handle_types<std::remove_cvref_t<decltype(std::declval<Node&>().value)>, IsMap> {
  using V = std::remove_cvref_t<decltype(std::declval<Node&>().value)>;

public:
  using allocator_type = Allocator;

private:
  using ator_traits = std::allocator_traits<allocator_type>;
  using node_alloc = typename ator_traits::template rebind_alloc<Node>;
  using node_traits = std::allocator_traits<node_alloc>;
  static constexpr bool pocma = ator_traits::propagate_on_container_move_assignment::value;
  static constexpr bool pocs = ator_traits::propagate_on_container_swap::value;
  static constexpr bool always_equal = ator_traits::is_always_equal::value;

  friend struct ::ycxx::detail::node_handle_access;

  Node* ptr_ = nullptr;
  union {
    allocator_type alloc_; // alive iff ptr_ != nullptr
  };

  constexpr void destroy_node() noexcept {
    node_alloc na(alloc_);
    node_traits::destroy(na, __builtin_addressof(ptr_->value));
    std::destroy_at(ptr_);
    node_traits::deallocate(na, ::ycxx::detail::to_alloc_pointer<typename node_traits::pointer>(ptr_), 1);
  }

public:
  // ---- [container.node.cons] ----
  constexpr node_handle() noexcept {}
  constexpr node_handle(node_handle&& nh) noexcept : ptr_(nh.ptr_) {
    if (ptr_) {
      std::construct_at(__builtin_addressof(alloc_), static_cast<allocator_type&&>(nh.alloc_));
      std::destroy_at(__builtin_addressof(nh.alloc_));
      nh.ptr_ = nullptr;
    }
  }
  constexpr node_handle& operator=(node_handle&& nh) {
    if (this == __builtin_addressof(nh)) {
      // [container.node.cons]/3 applied to the same object: the element is destroyed and the
      // handle ends up empty.
      if (ptr_) {
        destroy_node();
        std::destroy_at(__builtin_addressof(alloc_));
        ptr_ = nullptr;
      }
      return *this;
    }
    if constexpr (!pocma && !always_equal)
      ::ycxx::detail::precondition(!ptr_ || !nh.ptr_ || alloc_ == nh.alloc_,
                                   "node handle move assignment: unequal allocators that do not propagate");
    if (ptr_) {
      destroy_node();
      ptr_ = nullptr;
      if (nh.ptr_ && pocma) {
        alloc_ = static_cast<allocator_type&&>(nh.alloc_);
      } else if (!nh.ptr_) {
        std::destroy_at(__builtin_addressof(alloc_));
      }
      // Otherwise (a non-propagating allocator that compares equal) alloc_ is kept.
    } else if (nh.ptr_) {
      std::construct_at(__builtin_addressof(alloc_), static_cast<allocator_type&&>(nh.alloc_));
    }
    if (nh.ptr_) {
      ptr_ = nh.ptr_;
      std::destroy_at(__builtin_addressof(nh.alloc_));
      nh.ptr_ = nullptr;
    }
    return *this;
  }
  node_handle(const node_handle&) = delete;
  node_handle& operator=(const node_handle&) = delete;

  // ---- [container.node.dtor] ----
  constexpr ~node_handle() {
    if (ptr_) {
      destroy_node();
      std::destroy_at(__builtin_addressof(alloc_));
    }
  }

  // ---- [container.node.observers] ----
  constexpr V& value() const
    requires(!IsMap)
  {
    ::ycxx::detail::precondition(ptr_ != nullptr, "node handle value(): empty node handle");
    return ptr_->value;
  }
  // Not constexpr ([container.node.overview]): it modifies a const object.
  auto& key() const
    requires IsMap
  {
    ::ycxx::detail::precondition(ptr_ != nullptr, "node handle key(): empty node handle");
    return const_cast<std::remove_const_t<typename V::first_type>&>(ptr_->value.first);
  }
  constexpr auto& mapped() const
    requires IsMap
  {
    ::ycxx::detail::precondition(ptr_ != nullptr, "node handle mapped(): empty node handle");
    return ptr_->value.second;
  }
  constexpr allocator_type get_allocator() const {
    ::ycxx::detail::precondition(ptr_ != nullptr, "node handle get_allocator(): empty node handle");
    return alloc_;
  }
  constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }
  [[nodiscard]] constexpr bool empty() const noexcept { return ptr_ == nullptr; }

  // ---- [container.node.modifiers] ----
  constexpr void swap(node_handle& nh) noexcept(pocs || always_equal) {
    if (this == __builtin_addressof(nh))
      return;
    if constexpr (!pocs && !always_equal)
      ::ycxx::detail::precondition(!ptr_ || !nh.ptr_ || alloc_ == nh.alloc_,
                                   "node handle swap: unequal allocators that do not propagate");
    if (ptr_ && nh.ptr_) {
      if constexpr (pocs)
        ::ycxx::detail::swap_adl::do_swap(alloc_, nh.alloc_);
    } else if (ptr_) {
      std::construct_at(__builtin_addressof(nh.alloc_), static_cast<allocator_type&&>(alloc_));
      std::destroy_at(__builtin_addressof(alloc_));
    } else if (nh.ptr_) {
      std::construct_at(__builtin_addressof(alloc_), static_cast<allocator_type&&>(nh.alloc_));
      std::destroy_at(__builtin_addressof(nh.alloc_));
    }
    Node* t = ptr_;
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

} // namespace ycxx::adl_free
