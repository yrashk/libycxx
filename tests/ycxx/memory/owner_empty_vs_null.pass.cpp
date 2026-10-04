// "Empty" (no ownership) and "null" (get() == nullptr) are different properties:
// [util.smartptr.shared.const]/10: shared_ptr(nullptr_t p, D d) "Postconditions:
//   use_count() == 1 && get() == p" -- it owns (a null pointer); /18 the aliasing constructor
//   on an empty r gives an empty shared_ptr whose get() is non-null ("Postconditions: get() ==
//   p && use_count() == r.use_count()").
// [util.smartptr.shared.obs]/19.2: "!owner_before(b) && !b.owner_before(*this) is true if
//   and only if owner_equal(b) is true"; /23 owner_equal: "true if and only if *this and b
//   share ownership or are both empty"; owner_hash agrees (/21).
// [util.smartptr.weak.obs]: an expired weak_ptr still belongs to its ownership group, so it
//   is not equivalent to an empty one; weak_ptr from an empty shared_ptr is empty.
// [util.smartptr.ownerless]: owner_less<> compares by owner_before.
#include <memory>
#include <set>
#include "check.hpp"

int main() {
  std::owner_less<> ol;
  std::shared_ptr<int> empty;
  std::shared_ptr<int> empty2;
  std::shared_ptr<int> null_owner(nullptr, [](int*) {});
  CHECK(null_owner.use_count() == 1 && null_owner.get() == nullptr && !null_owner);
  CHECK(!empty.owner_equal(null_owner) && !null_owner.owner_equal(empty));
  CHECK(ol(empty, null_owner) != ol(null_owner, empty));  // not equivalent
  CHECK(!ol(empty, empty2) && !ol(empty2, empty) && empty.owner_equal(empty2));

  int x = 0;
  std::shared_ptr<int> alias_of_empty(empty, &x);
  CHECK(alias_of_empty.get() == &x && alias_of_empty.use_count() == 0);
  CHECK(alias_of_empty.owner_equal(empty) && !ol(alias_of_empty, empty) && !ol(empty, alias_of_empty));
  CHECK(alias_of_empty.owner_hash() == empty.owner_hash());
  std::weak_ptr<int> w_alias = alias_of_empty;
  CHECK(w_alias.expired() && w_alias.owner_equal(empty));

  std::shared_ptr<int> alias_of_null_owner(null_owner, &x);
  CHECK(alias_of_null_owner.owner_equal(null_owner) && alias_of_null_owner.use_count() == 2);
  CHECK(alias_of_null_owner.owner_hash() == null_owner.owner_hash());

  std::set<std::shared_ptr<int>, std::owner_less<std::shared_ptr<int>>> s{empty, null_owner, alias_of_empty,
                                                                          alias_of_null_owner};
  CHECK(s.size() == 2);

  std::weak_ptr<int> w_null = null_owner;
  alias_of_null_owner.reset();
  s.clear();
  null_owner.reset();
  CHECK(w_null.expired());
  CHECK(!w_null.owner_equal(empty) && (ol(w_null, empty) || ol(empty, w_null)));
  std::weak_ptr<int> w_empty;
  CHECK(w_empty.owner_equal(empty) && !ol(w_empty, empty) && !ol(empty, w_empty));
  return 0;
}
