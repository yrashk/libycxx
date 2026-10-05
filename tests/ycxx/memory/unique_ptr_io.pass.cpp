// [unique.ptr.io]: template<class E, class T, class Y, class D>
//   basic_ostream<E, T>& operator<<(basic_ostream<E, T>& os, const unique_ptr<Y, D>& p);
// Constraints: "os << p.get() is a valid expression"; Effects: "Equivalent to: os << p.get();";
// Returns: os. So the output is exactly what inserting the stored pointer gives, for any
// character type, for the array form, and for a deleter whose pointer type has its own
// inserter; and a unique_ptr whose pointer cannot be inserted is not insertable.
#include <concepts>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include "check.hpp"

template <class S, class P>
concept insertable = requires(S& os, const P& p) {
  { os << p } -> std::same_as<S&>;
};

// A pointer-like handle with its own inserter (a NullablePointer).
struct Handle {
  int id = 0;
  Handle() = default;
  Handle(std::nullptr_t) {}
  explicit Handle(int i) : id(i) {}
  friend bool operator==(Handle, Handle) = default;
  friend std::ostream& operator<<(std::ostream& os, Handle h) { return os << "handle#" << h.id; }
};
struct HandleDeleter {
  using pointer = Handle;
  void operator()(Handle) const {}
};

// A pointer type without any inserter.
struct Opaque {
  int id = 0;
  Opaque() = default;
  Opaque(std::nullptr_t) {}
  friend bool operator==(Opaque, Opaque) = default;
};
struct OpaqueDeleter {
  using pointer = Opaque;
  void operator()(Opaque) const {}
};

static_assert(insertable<std::ostream, std::unique_ptr<int>>);
static_assert(insertable<std::wostream, std::unique_ptr<int>>);
static_assert(insertable<std::ostream, std::unique_ptr<int[]>>);
static_assert(insertable<std::ostream, std::unique_ptr<Handle, HandleDeleter>>);
static_assert(!insertable<std::wostream, std::unique_ptr<Handle, HandleDeleter>>);
static_assert(!insertable<std::ostream, std::unique_ptr<Opaque, OpaqueDeleter>>);

template <class C, class P>
std::basic_string<C> show(const P& p) {
  std::basic_ostringstream<C> os;
  auto& r = (os << p);
  CHECK(&r == &os);
  return os.str();
}
template <class C, class Ptr>
std::basic_string<C> show_raw(Ptr p) {
  std::basic_ostringstream<C> os;
  os << p;
  return os.str();
}

int main() {
  auto a = std::make_unique<int>(5);
  CHECK(show<char>(a) == show_raw<char>(a.get()));
  CHECK(show<wchar_t>(a) == show_raw<wchar_t>(a.get()));
  std::unique_ptr<int> null;
  CHECK(show<char>(null) == show_raw<char>(static_cast<int*>(nullptr)));
  auto arr = std::make_unique<int[]>(3);
  CHECK(show<char>(arr) == show_raw<char>(arr.get()));
  // os << p.get() with a char* inserts the characters, so unique_ptr<char[]> prints the string.
  auto text = std::make_unique<char[]>(4);
  text[0] = 'h';
  text[1] = 'i';
  CHECK(show<char>(text) == "hi");
  std::unique_ptr<Handle, HandleDeleter> h(Handle(7));
  CHECK(show<char>(h) == "handle#7");
  // Formatting state applies as for the pointer itself.
  std::ostringstream os;
  os.width(30);
  os << std::left << a;
  std::ostringstream ref;
  ref.width(30);
  ref << std::left << a.get();
  CHECK(os.str() == ref.str() && os.str().size() == 30);
  return 0;
}
