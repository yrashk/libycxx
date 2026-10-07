// [stoptoken.general]/1 and [stopsource.general]/1 declare operator== as a defaulted member
// (`bool operator==(const stop_token& rhs) noexcept = default;`), so it can be named as a member;
// [stoptoken.mem]/[stopsource.mem]: equal when both are empty or share an ownership of the same
// stop state. (The draft omits `const`, which [class.compare.default]/1 requires: a draft
// defect; the member is const here.)
#include <stop_token>

int main() {
  std::stop_source a, b(std::nostopstate), c(std::nostopstate);
  std::stop_source a2 = a;
  static_assert(noexcept(a.operator==(a2)));
  static_assert(noexcept(a.get_token().operator==(a2.get_token())));
  if (!a.operator==(a2) || a.operator==(b) || !b.operator==(c)) return 1;
  std::stop_token t = a.get_token(), u = a2.get_token(), v;
  if (!t.operator==(u) || t.operator==(v) || !(v == std::stop_token{})) return 2;
  const std::stop_token& ct = t;
  if (!(ct == u) || ct != u) return 3;
  return 0;
}
