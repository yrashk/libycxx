// unique_ptr::reset whose deleter destroys the unique_ptr itself.
//   [unique.ptr.single.modifiers]/3: reset(p) "Assigns p to the stored pointer, and then, with
//     the old value of the stored pointer, old_p, evaluates if (old_p) get_deleter()(old_p);
//     [Note 1: The order of these operations is significant because the call to get_deleter()
//     might destroy *this.]" /4 Note 2: the postcondition does not hold then. So reset must not
//     touch *this after calling the deleter.
//   (operator=(nullptr_t) is "As if by reset()" but then returns *this, a reference to the
//   destroyed object, so it is not used here.)
//   [unique.ptr.runtime.modifiers]/2: the array form's reset "behaves the same as the reset
//     member of the primary template".
//   All of it is constexpr ([unique.ptr.single]), so the same sequences are also evaluated in
//   constant expressions, where any access to the destroyed unique_ptr is diagnosed.
#include <memory>
#include <optional>
#include "check.hpp"

// An object that owns itself.
struct SelfOwned {
  std::unique_ptr<SelfOwned> self;
  int* destroyed;
  constexpr explicit SelfOwned(int* d) : destroyed(d) {}
  constexpr ~SelfOwned() { ++*destroyed; }
};

// A holder whose unique_ptr's deleter destroys the holder.
struct Holder;
struct KillHolder {
  Holder* holder = nullptr;
  int* calls = nullptr;
  constexpr void operator()(int* p) const;
};
struct Holder {
  std::unique_ptr<int, KillHolder> p;
};
constexpr void KillHolder::operator()(int* p) const {
  ++*calls;
  delete p;
  Holder* h = holder;  // copy what is needed before *this (inside h->p) is destroyed
  delete h;
}

// Arrays: the deleter destroys the optional that holds the unique_ptr.
struct ResetOptional {
  std::optional<std::unique_ptr<int[], ResetOptional>>* opt = nullptr;
  int* calls = nullptr;
  constexpr void operator()(int* p) const {
    int* c = calls;
    auto* o = opt;
    delete[] p;
    o->reset();  // destroys the unique_ptr whose deleter is running (and this deleter)
    ++*c;
  }
};

constexpr bool run() {
  int destroyed = 0;
  {
    SelfOwned* s = new SelfOwned(&destroyed);
    s->self.reset(s);
    s->self.reset();
    if (destroyed != 1) return false;
  }
  {
    // reset(p) with a non-null p: p is stored, then the old object (the owner) is deleted
    // together with the unique_ptr, which destroys p as well.
    SelfOwned* s = new SelfOwned(&destroyed);
    s->self.reset(s);
    SelfOwned* other = new SelfOwned(&destroyed);
    s->self.reset(other);  // other is now owned by s->self, destroyed with s
    if (destroyed != 3) return false;
  }
  int calls = 0;
  {
    Holder* h = new Holder{std::unique_ptr<int, KillHolder>(new int(5), KillHolder{nullptr, &calls})};
    h->p.get_deleter().holder = h;
    h->p.reset();
    if (calls != 1) return false;
  }
  int acalls = 0;
  {
    std::optional<std::unique_ptr<int[], ResetOptional>> opt;
    opt.emplace(new int[3]{1, 2, 3}, ResetOptional{&opt, &acalls});
    opt->reset();
    if (acalls != 1 || opt.has_value()) return false;
  }
  return true;
}

int main() {
  CHECK(run());
  static_assert(run());
  return 0;
}
