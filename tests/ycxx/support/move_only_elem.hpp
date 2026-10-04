// A move-only, constexpr-friendly element type for libycxx's own suite. It is
// Cpp17MoveConstructible and Cpp17MoveAssignable but not copyable, so a container operation
// that needs only Cpp17MoveInsertable / Cpp17EmplaceConstructible elements
// ([container.alloc.reqmts]/2) must compile with it. A moved-from object has value -1.
// Independent of every other test suite.
#pragma once
#include <compare>
#include <initializer_list>

struct MOElem {
  int* heap = nullptr;  // owns one int; nullptr once moved from
  constexpr MOElem() : heap(new int(0)) {}
  constexpr MOElem(int v) : heap(new int(v)) {}
  MOElem(const MOElem&) = delete;
  MOElem& operator=(const MOElem&) = delete;
  constexpr MOElem(MOElem&& o) noexcept : heap(o.heap) { o.heap = nullptr; }
  constexpr MOElem& operator=(MOElem&& o) noexcept {
    if (this != &o) {
      delete heap;
      heap = o.heap;
      o.heap = nullptr;
    }
    return *this;
  }
  constexpr ~MOElem() { delete heap; }
  constexpr int value() const { return heap ? *heap : -1; }
  friend constexpr bool operator==(const MOElem& a, const MOElem& b) { return a.value() == b.value(); }
  friend constexpr std::strong_ordering operator<=>(const MOElem& a, const MOElem& b) {
    return a.value() <=> b.value();
  }
};

// Observers for containers of any element type with value() (Elem, MOElem) or of int.
template <class E>
constexpr int value_of(const E& e) {
  if constexpr (requires { e.value(); })
    return e.value();
  else
    return static_cast<int>(e);
}

// values_are(c, {v0, v1, ...}): the elements of c, in iteration order, have values v0, v1, ...
template <class C>
constexpr bool values_are(const C& c, std::initializer_list<int> vs) {
  auto it = c.begin();
  for (int v : vs) {
    if (it == c.end() || value_of(*it) != v) return false;
    ++it;
  }
  return it == c.end();
}

// append_to(c, args...): emplaces a new last element, also for forward_list (emplace_after
// on the last element).
template <class C, class... Args>
constexpr void append_to(C& c, Args&&... args) {
  if constexpr (requires { c.emplace_back(static_cast<Args&&>(args)...); }) {
    c.emplace_back(static_cast<Args&&>(args)...);
  } else {
    auto last = c.before_begin();
    for (auto it = c.begin(); it != c.end(); ++it) last = it;
    c.emplace_after(last, static_cast<Args&&>(args)...);
  }
}
