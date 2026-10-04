// [forward.list.modifiers]/1: the modifiers "do not affect the validity of iterators and
// references when inserting elements, and when erasing elements invalidate iterators and
// references to the erased elements only"; /38: clear() "Does not invalidate past-the-end
// iterators". [forward.list.ops]: remove / remove_if / unique "Invalidate only the iterators
// and references to the erased elements" (/15, /20); merge: iterators to the moved elements
// of x "continue to refer to their elements, but they now behave as iterators into *this"
// (/26); sort and reverse "Does not affect the validity of iterators and references" (/29,
// /32). Saved iterators (including before_begin() and end()) and references are checked
// to still refer to the same elements and to be usable for traversal afterwards.
#include <forward_list>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <vector>
#include "check.hpp"

using FL = std::forward_list<int>;

struct Saved {
  std::vector<FL::iterator> its;
  std::vector<int*> ptrs;
  std::vector<int> vals;
};

static Saved save(FL& f) {
  Saved s;
  for (auto it = f.begin(); it != f.end(); ++it) {
    s.its.push_back(it);
    s.ptrs.push_back(std::addressof(*it));
    s.vals.push_back(*it);
  }
  return s;
}

// Every saved element (except the indices in `gone`) is still referred to by its iterator and
// pointer, and is reachable by iterating the list.
static bool still_valid(FL& f, const Saved& s, std::initializer_list<int> gone = {}) {
  for (std::size_t i = 0; i < s.its.size(); ++i) {
    bool erased = false;
    for (int g : gone) erased |= static_cast<std::size_t>(g) == i;
    if (erased) continue;
    if (std::addressof(*s.its[i]) != s.ptrs[i] || *s.its[i] != s.vals[i]) return false;
    bool found = false;
    for (auto it = f.begin(); it != f.end(); ++it) found |= it == s.its[i];
    if (!found) return false;
  }
  return true;
}

int main() {
  {  // insertion
    FL f{1, 2, 3, 4, 5};
    Saved s = save(f);
    const auto bb = f.before_begin();
    const auto e = f.end();
    int arr[3] = {7, 8, 9};
    f.push_front(0);
    f.emplace_front(-1);
    f.insert_after(s.its[1], 20);
    f.insert_after(s.its[2], 2, 30);
    f.insert_after(s.its[4], arr, arr + 3);
    f.insert_after(s.its[0], {10, 11});
    f.emplace_after(s.its[3], 40);
    f.insert_range_after(s.its[2], arr);
    f.prepend_range(arr);
    f.resize(40);
    CHECK(still_valid(f, s));
    CHECK(bb == f.before_begin() && std::next(bb) == f.begin());
    CHECK(e == f.end());
    CHECK(std::next(s.its[4], 1) != f.end());
  }
  {  // erasure of single elements and ranges
    FL f{1, 2, 3, 4, 5, 6, 7, 8};
    Saved s = save(f);
    const auto e = f.end();
    f.erase_after(s.its[1]);               // erases 3 (index 2)
    f.erase_after(s.its[3], s.its[6]);     // erases 5, 6 (indices 4, 5)
    f.pop_front();                         // erases 1 (index 0)
    CHECK(still_valid(f, s, {0, 2, 4, 5}));
    CHECK(std::next(s.its[1]) == s.its[3] && std::next(s.its[3]) == s.its[6]);
    f.resize(3);                           // erases 8 (index 7)
    CHECK(still_valid(f, s, {0, 2, 4, 5, 7}));
    CHECK(e == f.end() && std::next(s.its[6]) == f.end());
  }
  {  // remove, remove_if, unique
    FL f{1, 1, 2, 3, 3, 3, 4, 5, 6};
    Saved s = save(f);
    CHECK(f.remove(4) == 1);                                  // index 6
    CHECK(f.remove_if([](int x) { return x == 6; }) == 1);    // index 8
    CHECK(f.unique() == 3);                                   // indices 1, 4, 5
    CHECK(still_valid(f, s, {1, 4, 5, 6, 8}));
    CHECK(std::next(s.its[0]) == s.its[2] && std::next(s.its[3]) == s.its[7]);
  }
  {  // sort, reverse: same elements, new order
    FL f{5, 3, 1, 4, 2};
    Saved s = save(f);
    f.sort();
    CHECK(still_valid(f, s));
    CHECK(f.begin() == s.its[2] && std::next(s.its[2]) == s.its[4]);  // 1 then 2
    f.reverse();
    CHECK(still_valid(f, s));
    CHECK(f.begin() == s.its[0] && std::next(s.its[0]) == s.its[3]);  // 5 then 4
    f.sort([](int a, int b) { return a > b; });
    CHECK(still_valid(f, s));
  }
  {  // merge: x's iterators now traverse *this
    FL f{1, 3, 5};
    FL x{2, 4, 6};
    Saved sf = save(f), sx = save(x);
    f.merge(x);
    CHECK(x.empty());
    CHECK(still_valid(f, sf) && still_valid(f, sx));
    CHECK(std::next(sx.its[0]) == sf.its[1]);  // 2 -> 3
    CHECK(std::next(sx.its[2]) == f.end());     // 6 is last, its successor is f.end()
  }
  {  // clear keeps end() valid
    FL f{1, 2, 3};
    const auto e = f.end();
    f.clear();
    CHECK(f.begin() == e && e == f.end());
  }
  return 0;
}
