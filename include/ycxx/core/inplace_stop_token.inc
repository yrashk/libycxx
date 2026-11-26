// Included inside the provider's public namespace after its source and callback declarations.
// The provider supplies __inplace_stop_state and __inplace_stop_callback_node.
#pragma once

// [stoptoken.inplace]
class inplace_stop_token {
  const inplace_stop_source* __stop_source_ = nullptr;

  constexpr explicit inplace_stop_token(const inplace_stop_source* s) noexcept : __stop_source_(s) {}
  friend class inplace_stop_source;
  template <class>
  friend class inplace_stop_callback;

public:
  template <class _CallbackFn>
  using callback_type = inplace_stop_callback<_CallbackFn>;

  inplace_stop_token() = default;
  bool operator==(const inplace_stop_token&) const = default;
  [[nodiscard]] bool stop_requested() const noexcept;
  [[nodiscard]] bool stop_possible() const noexcept { return __stop_source_ != nullptr; }
  void swap(inplace_stop_token& __rhs) noexcept {
    auto* s = __stop_source_;
    __stop_source_ = __rhs.__stop_source_;
    __rhs.__stop_source_ = s;
  }
  friend void swap(inplace_stop_token& a, inplace_stop_token& b) noexcept { a.swap(b); }
};

// [stopsource.inplace]
class inplace_stop_source {
  mutable __inplace_stop_state __state_;

  template <class>
  friend class inplace_stop_callback;

public:
  constexpr inplace_stop_source() noexcept = default;
  inplace_stop_source(inplace_stop_source&&) = delete;
  inplace_stop_source(const inplace_stop_source&) = delete;
  inplace_stop_source& operator=(inplace_stop_source&&) = delete;
  inplace_stop_source& operator=(const inplace_stop_source&) = delete;
  ~inplace_stop_source() = default;

  [[nodiscard]] constexpr inplace_stop_token get_token() const noexcept { return inplace_stop_token(this); }
  [[nodiscard]] static constexpr bool stop_possible() noexcept { return true; }
  [[nodiscard]] bool stop_requested() const noexcept { return __state_.stop_requested(); }
  bool request_stop() noexcept { return __state_.request_stop(); }
};

inline bool inplace_stop_token::stop_requested() const noexcept {
  return __stop_source_ != nullptr && __stop_source_->stop_requested();
}

// [stopcallback.inplace]
template <class _CallbackFn>
class inplace_stop_callback : __inplace_stop_callback_node {
  static_assert(::std::invocable<_CallbackFn> && ::std::destructible<_CallbackFn>,
                "inplace_stop_callback: CallbackFn must be invocable and destructible");

  _CallbackFn __callback_fn_;
  __inplace_stop_state* __state_ = nullptr;

  static void run(__inplace_stop_callback_node* n) noexcept {
    static_cast<_CallbackFn&&>(static_cast<inplace_stop_callback*>(n)->__callback_fn_)();
  }

public:
  using callback_type = _CallbackFn;

  template <class _Initializer>
    requires ::std::constructible_from<_CallbackFn, _Initializer>
  explicit inplace_stop_callback(inplace_stop_token __st, _Initializer&& init) noexcept(
      ::std::is_nothrow_constructible_v<_CallbackFn, _Initializer>)
      : __callback_fn_(static_cast<_Initializer&&>(init)) {
    if (!__st.__stop_source_)
      return;
    this->invoke = &run;
    __inplace_stop_state& s = __st.__stop_source_->__state_;
    if (s.add(this))
      __state_ = &s;
    else
      static_cast<_CallbackFn&&>(__callback_fn_)();
  }
  ~inplace_stop_callback() {
    if (__state_)
      __state_->remove(this);
  }
  inplace_stop_callback(inplace_stop_callback&&) = delete;
  inplace_stop_callback(const inplace_stop_callback&) = delete;
  inplace_stop_callback& operator=(inplace_stop_callback&&) = delete;
  inplace_stop_callback& operator=(const inplace_stop_callback&) = delete;
};
template <class _CallbackFn>
inplace_stop_callback(inplace_stop_token, _CallbackFn) -> inplace_stop_callback<_CallbackFn>;
