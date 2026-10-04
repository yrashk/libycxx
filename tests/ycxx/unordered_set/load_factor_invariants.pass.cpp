// [unord.req.general]/231: b.max_load_factor() "Returns: A positive number that the container
// attempts to keep the load factor less than or equal to. The container automatically
// increases the number of buckets as necessary to keep the load factor below this number."
// (/9: "The number of buckets is automatically increased as elements are added".) Checked after
// every kind of insertion: insert, insert_range, insert(first, last), insert(il), emplace,
// operator[], try_emplace, insert_or_assign, merge and the from_range constructor: size() <=
// bucket_count() * max_load_factor(). /237 a.rehash(n): "Postconditions: a.bucket_count() >=
// a.size() / a.max_load_factor() and a.bucket_count() >= n." /239 a.reserve(n): "Equivalent to
// a.rehash(ceil(n / a.max_load_factor()))". /63, /66: copy construction and copy assignment
// copy the maximum load factor; /234 a.max_load_factor(z) then a later insertion honours the
// new value. /242 (insert "shall not affect the validity of references to container elements")
// and /9 (rehashing does not invalidate references): references survive any rehash.
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "check.hpp"

template <class C>
bool lf_ok(const C& c) {
  return static_cast<double>(c.bucket_count()) * c.max_load_factor() >= static_cast<double>(c.size());
}

int main() {
  std::vector<int> v(500);
  for (int i = 0; i < 500; ++i) v[i] = i * 13 + 1;
  for (float mlf : {0.25f, 0.5f, 1.0f, 3.0f, 7.5f}) {
    std::unordered_set<int> s;
    s.max_load_factor(mlf);
    for (int i = 0; i < 200; ++i) {
      s.insert(i * 7);
      CHECK(lf_ok(s));
      s.emplace(-i - 1);
      CHECK(lf_ok(s));
    }
    s.insert_range(v);
    CHECK(lf_ok(s));
    s.insert(v.begin(), v.end());
    CHECK(lf_ok(s));
    s.insert({100001, 100002, 100003});
    CHECK(lf_ok(s));
    std::unordered_set<int> c(s);
    CHECK(c.max_load_factor() == s.max_load_factor() && lf_ok(c));
    std::unordered_set<int> d;
    d = s;
    CHECK(d.max_load_factor() == s.max_load_factor() && lf_ok(d));
    s.rehash(0);
    CHECK(lf_ok(s));
    s.rehash(5000);
    CHECK(s.bucket_count() >= 5000 && lf_ok(s));
    s.reserve(10000);
    CHECK(s.bucket_count() >= std::ceil(10000 / s.max_load_factor()));
    s.max_load_factor(mlf / 4);
    s.insert(-100000);
    CHECK(lf_ok(s));
    s.rehash(1);
    CHECK(lf_ok(s));

    std::unordered_multiset<int> ms;
    ms.max_load_factor(mlf);
    for (int i = 0; i < 300; ++i) {
      ms.insert(i % 17);
      CHECK(lf_ok(ms));
    }
    ms.insert_range(v);
    CHECK(lf_ok(ms));

    std::unordered_map<int, int> um;
    um.max_load_factor(mlf);
    for (int i = 0; i < 300; ++i) {
      um[i] = i;
      CHECK(lf_ok(um));
      um.try_emplace(i + 1000, 1);
      CHECK(lf_ok(um));
      um.insert_or_assign(i + 5000, 1);
      CHECK(lf_ok(um));
    }
    std::unordered_map<int, int> um2;
    for (int i = 0; i < 100; ++i) um2.emplace(i + 20000, i);
    um.merge(um2);
    CHECK(lf_ok(um) && um2.empty());
    std::unordered_set<int> fr(std::from_range, v);
    CHECK(lf_ok(fr) && fr.size() == 500);
  }
  {
    std::unordered_set<int> s;
    std::vector<const int*> ptrs;
    for (int i = 0; i < 100; ++i) ptrs.push_back(&*s.insert(i).first);
    s.rehash(10);
    s.rehash(100000);
    s.max_load_factor(0.01f);
    s.insert(1000);
    for (int i = 0; i < 100; ++i) CHECK(*ptrs[i] == i && &*s.find(i) == ptrs[i]);
  }
  return 0;
}
