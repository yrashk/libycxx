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
// __ycxx::__detail::__node_handle_access: make<NH>(node, alloc), take(nh) (empties nh, returns the
// node) and peek(nh).
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
struct __node_handle_access;
}}

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// The key/mapped types of a map node handle and the value type of a set node handle.
template <class _Vp, bool _Map>
struct __node_handle_types {
  using value_type = _Vp;
};
template <class _Vp>
struct __node_handle_types<_Vp, true> {
  using key_type = std::remove_const_t<typename _Vp::first_type>;
  using mapped_type = typename _Vp::second_type;
};

template <class _Node, class _Alloc, bool _IsMap>
class __node_handle : public __node_handle_types<std::remove_cvref_t<decltype(std::declval<_Node&>().value)>, _IsMap> {
  using __node = _Node;
  using _Vp = std::remove_cvref_t<decltype(std::declval<_Node&>().value)>;
  using __ator_traits = std::allocator_traits<_Alloc>;
  using __node_traits = typename __ator_traits::template rebind_traits<__node>;
  using __node_pointer = typename __node_traits::pointer;
  using __node_alloc = typename __ator_traits::template rebind_alloc<__node>;

  friend struct ::__ycxx::__detail::__node_handle_access;

public:
  using allocator_type = _Alloc;

private:
  __node_pointer __ptr_ = __node_pointer();
  // optional<allocator_type> alloc_, without the <optional> dependency: engaged iff ptr_ is
  // not null.
  union {
    _Alloc __alloc_;
  };

  constexpr __node* __raw() const noexcept { return __ptr_ == nullptr ? nullptr : std::to_address(__ptr_); }
  constexpr void __destroy_node() noexcept {
    __node* n = __raw();
    __ator_traits::destroy(__alloc_, __builtin_addressof(n->value));
    std::destroy_at(n);
    __node_alloc __na(__alloc_);
    __node_traits::deallocate(__na, __ptr_, 1);
  }
  // Leaves *this empty without touching the node.
  constexpr void reset() noexcept {
    if (__ptr_ != nullptr) {
      std::destroy_at(__builtin_addressof(__alloc_));
      __ptr_ = __node_pointer();
    }
  }
  constexpr __node_handle(__node* n, const _Alloc& a) noexcept
      : __ptr_(::__ycxx::__detail::__to_alloc_pointer<__node_pointer>(n)) {
    std::construct_at(__builtin_addressof(__alloc_), a);
  }

public:
  // ---- [container.node.cons] ----
  constexpr __node_handle() noexcept {}
  constexpr __node_handle(__node_handle&& __nh) noexcept : __ptr_(__nh.__ptr_) {
    if (__ptr_ != nullptr) {
      std::construct_at(__builtin_addressof(__alloc_), static_cast<_Alloc&&>(__nh.__alloc_));
      std::destroy_at(__builtin_addressof(__nh.__alloc_));
      __nh.__ptr_ = __node_pointer();
    }
  }
  constexpr __node_handle& operator=(__node_handle&& __nh) noexcept {
    if (this == __builtin_addressof(__nh)) {
      // [container.node.cons]/3 applied to one object: the element is destroyed, and the
      // handle ends up empty.
      if (__ptr_ != nullptr) {
        __destroy_node();
        reset();
      }
      return *this;
    }
    if (__ptr_ != nullptr) {
      ::__ycxx::__detail::__precondition(__ator_traits::propagate_on_container_move_assignment::value ||
                                       __nh.__ptr_ == nullptr || __alloc_ == __nh.__alloc_,
                                   "node handle move assignment: unequal allocators that do not propagate");
      __destroy_node();
    }
    if (__nh.__ptr_ == nullptr) {
      reset();
      return *this;
    }
    if (__ptr_ == nullptr)
      std::construct_at(__builtin_addressof(__alloc_), static_cast<_Alloc&&>(__nh.__alloc_));
    else if constexpr (__ator_traits::propagate_on_container_move_assignment::value)
      __alloc_ = static_cast<_Alloc&&>(__nh.__alloc_);
    __ptr_ = __nh.__ptr_;
    __nh.reset();
    return *this;
  }

  // ---- [container.node.dtor] ----
  constexpr ~__node_handle() {
    if (__ptr_ != nullptr) {
      __destroy_node();
      std::destroy_at(__builtin_addressof(__alloc_));
    }
  }

  // ---- [container.node.observers] ----
  constexpr _Vp& value() const noexcept
    requires(!_IsMap)
  {
    ::__ycxx::__detail::__precondition(__ptr_ != nullptr, "node handle: empty");
    return __raw()->value;
  }
  // Not constexpr ([container.node.overview]): the key of a map element is a const object.
  auto& __key() const noexcept
    requires _IsMap
  {
    ::__ycxx::__detail::__precondition(__ptr_ != nullptr, "node handle: empty");
    return const_cast<std::remove_const_t<typename _Vp::first_type>&>(__raw()->value.first);
  }
  constexpr auto& __mapped() const noexcept
    requires _IsMap
  {
    ::__ycxx::__detail::__precondition(__ptr_ != nullptr, "node handle: empty");
    return __raw()->value.second;
  }
  constexpr allocator_type get_allocator() const {
    ::__ycxx::__detail::__precondition(__ptr_ != nullptr, "node handle: empty");
    return __alloc_;
  }
  constexpr explicit operator bool() const noexcept { return __ptr_ != nullptr; }
  [[nodiscard]] constexpr bool empty() const noexcept { return __ptr_ == nullptr; }

  // ---- [container.node.modifiers] ----
  constexpr void swap(__node_handle& __nh) noexcept(__ator_traits::propagate_on_container_swap::value ||
                                                __ator_traits::is_always_equal::value) {
    if (this == __builtin_addressof(__nh))
      return;
    const bool a = __ptr_ != nullptr, b = __nh.__ptr_ != nullptr;
    if (a && b) {
      if constexpr (__ator_traits::propagate_on_container_swap::value)
        ::__ycxx::__detail::__swap_adl::__do_swap(__alloc_, __nh.__alloc_);
      else
        ::__ycxx::__detail::__precondition(__ator_traits::is_always_equal::value || __alloc_ == __nh.__alloc_,
                                     "node handle swap: unequal allocators that do not propagate");
    } else if (a) {
      std::construct_at(__builtin_addressof(__nh.__alloc_), static_cast<_Alloc&&>(__alloc_));
      std::destroy_at(__builtin_addressof(__alloc_));
    } else if (b) {
      std::construct_at(__builtin_addressof(__alloc_), static_cast<_Alloc&&>(__nh.__alloc_));
      std::destroy_at(__builtin_addressof(__nh.__alloc_));
    }
    const __node_pointer t = __ptr_;
    __ptr_ = __nh.__ptr_;
    __nh.__ptr_ = t;
  }
  friend constexpr void swap(__node_handle& __x, __node_handle& y) noexcept(noexcept(__x.swap(y))) { __x.swap(y); }
};

// [container.insert.return]
template <class _Iterator, class _NodeType>
struct insert_return_type {
  _Iterator position;
  bool __inserted;
  _NodeType __node;
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// How the containers make, inspect and empty node handles.
struct __node_handle_access {
  template <class _NH, class _Node, class _Alloc>
  static constexpr _NH __make(_Node* n, const _Alloc& a) noexcept {
    return _NH(n, a);
  }
  // The node owned by nh (null if empty).
  template <class _NH>
  static constexpr auto* peek(const _NH& __nh) noexcept {
    return __nh.__raw();
  }
  // Takes the node out of nh, which becomes empty.
  template <class _NH>
  static constexpr auto* take(_NH& __nh) noexcept {
    auto* n = __nh.__raw();
    __nh.reset();
    return n;
  }
};

}} // namespace __ycxx::__detail
