// [indirect.general]/6: "The template parameter T of indirect may be an incomplete type."
// (The destructor Mandates T is complete, so a recursive type works when the destructor is
// instantiated after T is complete.)
#include <memory>
#include <type_traits>

struct Tree;
struct Holder {
  std::indirect<Tree> child;  // Tree incomplete here
  Holder();
  ~Holder();
};
struct Tree {
  int v = 0;
};
Holder::Holder() = default;
Holder::~Holder() = default;

static_assert(std::is_same_v<std::indirect<Tree>::value_type, Tree>);
