// [variant.visit]/3: for visit<R>, e(m) is "INVOKE<R>(std::forward<Visitor>(vis),
// GET<m>(std::forward<V>(vars))...)"; /5: "Mandates: For each valid pack m, e(m) is a valid
// expression." [func.require]/2: "If reference_converts_from_temporary_v<R,
// decltype(INVOKE(f, t1, t2, ..., tN))> is true, INVOKE<R>(f, t1, t2, ..., tN) is ill-formed."
// A visitor returning int prvalues cannot be visited with R = const long& (control: R = long).
#include <variant>

void f() {
  std::variant<int, short> v;
  (void)std::visit<const long&>([](auto x) -> int { return x; }, v);
}
