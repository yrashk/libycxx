// [unique.ptr.create]/5: "template<class T, class... Args> unspecified make_unique(Args&&...)
// = delete; Constraints: T is an array of known bound."
#include <memory>

int main() {
  auto p = std::make_unique<int[4]>();
}
