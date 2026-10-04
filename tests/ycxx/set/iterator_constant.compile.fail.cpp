// [associative.reqmts.general]/6: "For associative containers where the value type is the
// same as the key type, both iterator and const_iterator are constant iterators." Assigning
// through a set::iterator is therefore ill-formed.
#include <set>

int main() {
  std::set<int> s{1, 2, 3};
  std::set<int>::iterator it = s.begin();
  *it = 5;
  return 0;
}
