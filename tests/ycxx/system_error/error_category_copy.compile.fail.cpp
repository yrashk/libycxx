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
