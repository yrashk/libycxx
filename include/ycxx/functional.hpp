// libycxx extensions: inspect owning wrappers' inline storage and select a fixed-capacity,
// copyable callable wrapper whose target storage never allocates. See docs/FUNCTION_STORAGE.md.
#pragma once

#include <cstddef>
#include <ycxx/core/function_wrappers.hpp>

/** @brief Opt-in libycxx extensions, available through their `ycxx/` headers. */
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] ycxx { inline namespace __y1 {

/** @brief Copyable owning function wrapper with configurable inline storage and no heap fallback.
 *
 * Include `<ycxx/functional.hpp>`; this is a libycxx extension, not a member of `std` and not
 * exported by `import std;`. Only function signatures of the form `R(Args...)` are supported.
 *
 * Targets must be copy constructible, nothrow move constructible, callable as `T&`, and fit
 * Capacity bytes at Alignment alignment. Unsuitable targets are rejected by constraints.
 * Capacity must be positive; Alignment must be a supported positive power-of-two alignment.
 * Defaults are four pointer widths and `alignof(std::max_align_t)` (32 bytes and 16-byte
 * alignment on typical 64-bit targets). Capacity excludes the wrapper's metadata and padding.
 *
 * The wrapper never allocates target storage. A target's construction, copy, invocation or
 * destruction can allocate internally. Copies own independent target copies; moves empty the
 * source. Calling an empty wrapper throws `std::bad_function_call`, following libycxx's error
 * handler policy when exceptions are disabled. The const call operator invokes a mutable target
 * just as `std::function` does. References returned by the target preserve their lifetime
 * requirements; dangling reference conversions are rejected.
 *
 * @code
 * #include <ycxx/functional.hpp>
 * #include <array>
 *
 * int main() {
 *     std::array<int, 12> values{};
 *     ycxx::inplace_function<int(), 64> callback = [values] { return values[0]; };
 *     auto copy = callback; // independent target, still no wrapper allocation
 * }
 * @endcode
 *
 * Unlike `std::function`, this extension has no `target()`/`target_type()` interface, allocator
 * parameter, or qualified/noexcept signatures. A different capacity specialization is treated
 * like any other callable: the entire source wrapper must fit; no unwrapping is performed.
 *
 * @tparam _Signature Function signature `R(Args...)`.
 * @tparam _Capacity Maximum target size in bytes, excluding metadata and padding.
 * @tparam _Alignment Alignment of the target buffer, in bytes.
 * @see uses_inline_storage
 */
template <class _Signature, std::size_t _Capacity = 4 * sizeof(void*), std::size_t _Alignment = alignof(std::max_align_t)>
class inplace_function;

// A storage-policy query, not a call-signature or constructor-validity query. For an existing
// erased wrapper its runtime target cannot be discovered from the wrapper's type.
/** @brief Reports whether a decayed target type meets a wrapper's inline-storage policy.
 *
 * A `std::bool_constant` trait for libycxx's `std::function`, `std::move_only_function`,
 * `std::copyable_function`, and `ycxx::inplace_function`. Its value is true when `decay_t<Target>`
 * fits the wrapper's target buffer, meets its alignment limit, and is nothrow move constructible.
 * Unknown wrappers, void and incomplete target types yield false. The wrapper type must be
 * unqualified. Targets are decayed, so a function type is tested as a function pointer.
 *
 * This checks storage properties only. It does not check call-signature compatibility,
 * copyability, or construction from a particular argument. It does not inspect an erased
 * wrapper's current runtime target, or describe transfers that adopt another wrapper's target.
 * True means no wrapper allocation for an otherwise valid target construction; target code
 * itself can still allocate. Policy values may change between libycxx ABI revisions.
 *
 * @code
 * auto task = [a = 1, b = 2] { return a + b; };
 * static_assert(ycxx::uses_inline_storage_v<std::function<int()>, decltype(task)>);
 * @endcode
 *
 * @tparam _Wrapper Unqualified owning wrapper type.
 * @tparam _Target Target type, decayed before checking storage properties.
 * @see uses_inline_storage_v
 */
template <class _Wrapper, class _Target>
struct uses_inline_storage : std::false_type {};

template <class _Signature, class _Target>
struct uses_inline_storage<std::function<_Signature>, _Target>
    : std::bool_constant<::__ycxx::__detail::__fw::__fits_inline<_Target, ::__ycxx::__detail::__fw::__small_size,
                                                             alignof(::__ycxx::__detail::__fw::__storage)>> {};
template <class _Target, class... _Signatures>
struct uses_inline_storage<std::move_only_function<_Signatures...>, _Target>
    : uses_inline_storage<std::function<void()>, _Target> {};
template <class _Target, class... _Signatures>
struct uses_inline_storage<std::copyable_function<_Signatures...>, _Target>
    : uses_inline_storage<std::function<void()>, _Target> {};
template <class _Signature, std::size_t _Capacity, std::size_t _Alignment, class _Target>
struct uses_inline_storage<inplace_function<_Signature, _Capacity, _Alignment>, _Target>
    : std::bool_constant<::__ycxx::__detail::__fw::__fits_inline<_Target, _Capacity, _Alignment>> {};

/** @brief Value of uses_inline_storage<Wrapper, Target>.
 * @see uses_inline_storage
 */
template <class _Wrapper, class _Target>
inline constexpr bool uses_inline_storage_v = uses_inline_storage<_Wrapper, _Target>::value;

/** @brief Fixed-capacity wrapper for a function signature `R(Args...)`.
 * The primary template documents the storage guarantees and capacity requirements.
 */
template <class _Rp, class... _Ap, std::size_t _Capacity, std::size_t _Alignment>
class inplace_function<_Rp(_Ap...), _Capacity, _Alignment> {
  static_assert(_Capacity > 0, "ycxx::inplace_function: capacity must be positive");
  static_assert(_Alignment > 0 && (_Alignment & (_Alignment - 1)) == 0,
                "ycxx::inplace_function: alignment must be a positive power of two");

  using __thunk_t = _Rp (*)(void*, ::__ycxx::__detail::__fw::__param_t<_Ap>...);
  struct __ops {
    void (*__relocate)(void*, void*) noexcept;
    void (*copy)(void*, const void*);
    void (*destroy)(void*) noexcept;
  };

  template <class _VT>
  static _VT* __target(void* __p) noexcept {
    return std::launder(static_cast<_VT*>(__p));
  }
  template <class _VT>
  static void __relocate(void* __dst, void* __src) noexcept {
    _VT* __p = __target<_VT>(__src);
    ::new (__dst) _VT(static_cast<_VT&&>(*__p));
    __p->~_VT();
  }
  template <class _VT>
  static void __copy(void* __dst, const void* __src) {
    ::new (__dst) _VT(*std::launder(static_cast<const _VT*>(__src)));
  }
  template <class _VT>
  static void __destroy(void* __p) noexcept {
    __target<_VT>(__p)->~_VT();
  }
  template <class _VT>
  static inline constexpr __ops __ops_for = {&__relocate<_VT>, &__copy<_VT>, &__destroy<_VT>};

  template <class _VT>
  static _Rp __invoke(void* __p, ::__ycxx::__detail::__fw::__param_t<_Ap>... __args) {
    return ::__ycxx::__detail::invoke_r<_Rp>(*__target<_VT>(__p),
                                          static_cast<::__ycxx::__detail::__fw::__param_t<_Ap>&&>(__args)...);
  }
  [[noreturn]] static _Rp __empty(void*, ::__ycxx::__detail::__fw::__param_t<_Ap>...) {
    ::__ycxx::__detail::__throw_bad_function_call();
  }

  template <class _Fp>
  static consteval bool __accepts() {
    using _VT = std::decay_t<_Fp>;
    if constexpr (std::is_same_v<std::remove_cvref_t<_Fp>, inplace_function>)
      return false;
    else if constexpr (!::__ycxx::__detail::__fw::__fits_inline<_VT, _Capacity, _Alignment>)
      return false;
    else
      return std::is_copy_constructible_v<_VT> && std::is_constructible_v<_VT, _Fp> &&
             std::is_invocable_r_v<_Rp, _VT&, _Ap...>;
  }

  void __reset() noexcept {
    if (__ops_) {
      __ops_->destroy(__buf_);
      __ops_ = nullptr;
      __call_ = &__empty;
    }
  }
  // __take requires an empty destination; a move always empties the source.
  void __take(inplace_function& __other) noexcept {
    if (__other.__ops_) {
      __other.__ops_->__relocate(__buf_, __other.__buf_);
      __ops_ = __other.__ops_;
      __call_ = __other.__call_;
      __other.__ops_ = nullptr;
      __other.__call_ = &__empty;
    }
  }

  __thunk_t __call_ = &__empty;
  const __ops* __ops_ = nullptr;
  // Keep the buffer after metadata so an empty target cannot share an enclosing empty base's
  // address. A nonzero capacity is enforced above; keep invalid instantiations diagnosable.
  alignas(_Alignment) mutable unsigned char __buf_[_Capacity > 0 ? _Capacity : 1];

public:
  using result_type = _Rp;
  /// Maximum target size in bytes; excludes metadata and padding.
  static constexpr std::size_t capacity = _Capacity;
  /// Alignment of the target buffer, in bytes.
  static constexpr std::size_t alignment = _Alignment;

  /// Constructs an empty wrapper without allocating.
  inplace_function() noexcept {}
  /// Constructs an empty wrapper without allocating.
  inplace_function(std::nullptr_t) noexcept {}
  /** @brief Copies the source's target into independent inline storage.
   * Propagates any exception thrown by the target's copy constructor.
   */
  inplace_function(const inplace_function& __other) {
    if (__other.__ops_) {
      __other.__ops_->copy(__buf_, __other.__buf_);
      __ops_ = __other.__ops_;
      __call_ = __other.__call_;
    }
  }
  /// Moves the target into this wrapper and leaves the source empty.
  inplace_function(inplace_function&& __other) noexcept { __take(__other); }

  /** @brief Constructs an eligible target directly in inline storage.
   *
   * Requires a copy-constructible, nothrow-movable target that fits the buffer and can be
   * constructed from the argument and invoked as an lvalue with the specified result type.
   * Null function pointers and null member pointers produce an empty wrapper.
   * Propagates target-construction exceptions. Nonthrowing when that construction is nonthrowing.
   */
  template <class _Fp>
    requires(__accepts<_Fp>())
  inplace_function(_Fp&& __f) noexcept(std::is_nothrow_constructible_v<std::decay_t<_Fp>, _Fp>) {
    using _VT = std::decay_t<_Fp>;
    if constexpr (::__ycxx::__detail::__fw::__is_nullable_pointer<_VT>) {
      if (__f == nullptr)
        return;
    }
    ::new (static_cast<void*>(__buf_)) _VT(static_cast<_Fp&&>(__f));
    __ops_ = &__ops_for<_VT>;
    __call_ = &__invoke<_VT>;
  }

  /// Destroys the target, if present.
  ~inplace_function() { __reset(); }

  /** @brief Replaces the target with an independent copy of the source's target.
   * Propagates target-copy exceptions; this wrapper stays unchanged if copying throws.
   */
  inplace_function& operator=(const inplace_function& __other) {
    inplace_function __tmp(__other);
    __reset();
    __take(__tmp);
    return *this;
  }
  /// Replaces the target by moving from the source, which becomes empty; self-move does nothing.
  inplace_function& operator=(inplace_function&& __other) noexcept {
    if (this != __builtin_addressof(__other)) {
      // The source can be owned indirectly by this target. Take it before destroying the owner.
      inplace_function __tmp(static_cast<inplace_function&&>(__other));
      __reset();
      __take(__tmp);
    }
    return *this;
  }
  /// Destroys the target and makes the wrapper empty.
  inplace_function& operator=(std::nullptr_t) noexcept {
    __reset();
    return *this;
  }
  /** @brief Replaces the target by constructing an eligible callable.
   * Propagates target-construction exceptions; the old target stays unchanged if construction throws.
   */
  template <class _Fp>
    requires(__accepts<_Fp>())
  inplace_function& operator=(_Fp&& __f) noexcept(std::is_nothrow_constructible_v<std::decay_t<_Fp>, _Fp>) {
    inplace_function __tmp(static_cast<_Fp&&>(__f));
    __reset();
    __take(__tmp);
    return *this;
  }

  /// Returns true exactly when the wrapper contains a target.
  explicit operator bool() const noexcept { return __ops_ != nullptr; }
  /** @brief Invokes the stored target as a mutable lvalue, forwarding the arguments.
   * @throws std::bad_function_call if empty; otherwise any exception thrown by the target.
   */
  _Rp operator()(_Ap... __args) const {
    return __call_(__buf_, static_cast<_Ap&&>(__args)...);
  }
  /// Exchanges targets without allocation or exceptions; self-swap does nothing.
  void swap(inplace_function& __other) noexcept {
    if (this == __builtin_addressof(__other))
      return;
    inplace_function __tmp(static_cast<inplace_function&&>(*this));
    __take(__other);
    __other.__take(__tmp);
  }
  /// Exchanges targets without allocation or exceptions.
  friend void swap(inplace_function& __a, inplace_function& __b) noexcept { __a.swap(__b); }
  /// Returns true exactly when the wrapper is empty; reversed comparisons are supported.
  friend bool operator==(const inplace_function& __f, std::nullptr_t) noexcept { return !__f; }
};

}} // namespace ycxx::__y1
