// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::error_category::error_category\(const std::error_category&\)
// EXPECT-ERROR-CLANG: error: call to implicitly-deleted copy constructor of 'Cat'
// EXPECT-ERROR-CLANG: note: copy constructor of 'Cat' is implicitly deleted because base class 'std::error_category' has a deleted copy constructor
// [syserr.errcat.overview]: error_category(const error_category&) = delete; categories are
// compared by identity and cannot be copied.
#include <system_error>
#include <string>

struct Cat : std::error_category {
  const char* name() const noexcept override { return "c"; }
  std::string message(int) const override { return ""; }
};

const Cat a;
const Cat ok{};  // control: default construction
Cat b = a;

int main() { return ok == b; }
