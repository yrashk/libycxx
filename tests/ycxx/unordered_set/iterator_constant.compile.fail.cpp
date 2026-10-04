// [unord.req.general]/8: "For unordered containers where the value type is the same as the
// key type, both iterator and const_iterator are constant iterators." Assigning through an
// unordered_set::iterator is therefore ill-formed.
#include <unordered_set>

int main() {
  std::unordered_set<int> s{1, 2, 3};
  std::unordered_set<int>::iterator it = s.begin();
  *it = 5;
  return 0;
}
