// The sequence containers' emplace functions construct the element in place from the forwarded
// arguments: no temporary T, no copy or move of the new element.
//   [sequence.reqmts] a.emplace(p, args): "Inserts an object of type T constructed with
//     std::forward<Args>(args)... before p"; a.emplace_front(args) / a.emplace_back(args):
//     "Prepends/Appends an object of type T constructed with std::forward<Args>(args)...";
//     Preconditions: T is Cpp17EmplaceConstructible into X from args (for vector also
//     Cpp17MoveInsertable, and for emplace(p) Cpp17MoveInsertable and Cpp17MoveAssignable for
//     vector, inplace_vector and deque).
//   [list.modifiers], [forward.list.modifiers] emplace_after / emplace_front: the same; list and
//     forward_list need only Cpp17EmplaceConstructible, so a type that can be neither copied nor
//     moved works, as it does with deque's emplace_front/emplace_back.
//   [inplace.vector.modifiers]/8-10: try_emplace_back appends "an object of type T
//     direct-non-list-initialized with vals..." (Preconditions: only
//     Cpp17EmplaceConstructible); unchecked_emplace_back: "return *try_emplace_back(...)".
//   [stack.mod], [queue.mod]: emplace is c.emplace_back(std::forward<Args>(args)...);
//     [priqueue.members]: c.emplace_back(std::forward<Args>(args)...); push_heap(...).
// Constructor arguments keep their value category (an lvalue Arg stays an lvalue, an rvalue
// stays an rvalue, const is kept). Appending to a vector whose capacity suffices, or with
// emplace(end()), constructs the element where it ends up; when it reallocates, only the
// existing elements are moved (their move constructor is noexcept). hive: hive/emplace_in_place.
#include <deque>
#include <forward_list>
#include <inplace_vector>
#include <list>
#include <queue>
#include <stack>
#include <utility>
#include <vector>
#include "inplace_probe.hpp"
#include "check.hpp"

using probe::Arg;
using probe::counts;
using probe::Pinned;
using probe::Probe;

// emplace_back with each argument category on a container of Probe; no copies or moves.
template <class C>
void back_categories(C& c) {
  Arg a{7};
  const Arg ca{8};
  probe::reset();
  CHECK(c.emplace_back(1, a).cat == probe::lref);
  CHECK(c.emplace_back(2, ca).cat == probe::clref);
  CHECK(c.emplace_back(3, std::move(ca)).cat == probe::crref);
  CHECK(c.emplace_back(4, Arg{9}).cat == probe::rref);
  CHECK(c.emplace_back(5, std::move(a)).cat == probe::rref);
  CHECK(a.moved_from);
  CHECK(counts.made == 5);
  CHECK(counts.extra() == 0);
  CHECK(counts.destroyed == 0);
  CHECK(c.back().key == 5 && c.back().extra == 7);
}

template <class C>
void front_categories(C& c) {
  Arg a{7};
  probe::reset();
  CHECK(c.emplace_front(1, a).cat == probe::lref);
  CHECK(c.emplace_front(2, std::move(a)).cat == probe::rref);
  CHECK(counts.made == 2 && counts.extra() == 0 && counts.destroyed == 0);
  CHECK(c.front().key == 2);
}

void vector_cases() {
  std::vector<Probe> v;
  v.reserve(16);
  back_categories(v);
  // emplace(end()) and emplace(position) at the end.
  probe::reset();
  auto it = v.emplace(v.cend(), 6, Arg{1});
  CHECK(it->key == 6 && it->cat == probe::rref);
  CHECK(counts.made == 1 && counts.extra() == 0 && counts.destroyed == 0);
  // Reallocation: the new element is constructed once, the old ones are moved once each.
  v.shrink_to_fit();
  std::size_t n = v.size();
  CHECK(v.capacity() == n);
  probe::reset();
  v.emplace_back(7, 3);
  CHECK(counts.made == 1);
  CHECK(counts.copies == 0);
  CHECK(counts.moves == static_cast<int>(n));
  CHECK(counts.copy_assigns + counts.move_assigns == 0);
  CHECK(counts.destroyed == static_cast<int>(n));
  CHECK(v.back().key == 7 && v.back().extra == 3);
}

void deque_cases() {
  std::deque<Probe> d;
  back_categories(d);
  front_categories(d);
  std::deque<Pinned> p;
  probe::reset();
  p.emplace_back(1, Arg{});
  p.emplace_front(0);
  for (int i = 2; i < 100; ++i) p.emplace_back(i);  // several blocks
  CHECK(p.size() == 100 && p.front().key == 0 && p[1].cat == probe::rref && p.back().key == 99);
  CHECK(counts.made == 100);
}

void list_cases() {
  std::list<Probe> l;
  back_categories(l);
  front_categories(l);
  Arg a{4};
  probe::reset();
  auto it = l.emplace(std::next(l.cbegin()), 50, a);
  CHECK(it->cat == probe::lref && std::prev(it) == l.begin());
  CHECK(counts.made == 1 && counts.extra() == 0);

  std::list<Pinned> p;
  p.emplace_back(1);
  p.emplace_front(0, a);
  p.emplace(std::next(p.cbegin()), 5, std::move(a));
  CHECK(p.size() == 3 && p.front().cat == probe::lref);
  CHECK(std::next(p.begin())->key == 5 && std::next(p.begin())->cat == probe::rref);
}

void forward_list_cases() {
  std::forward_list<Probe> f;
  front_categories(f);
  Arg a{4};
  probe::reset();
  auto it = f.emplace_after(f.cbefore_begin(), 9, std::as_const(a));
  CHECK(it == f.begin() && it->cat == probe::clref);
  CHECK(counts.made == 1 && counts.extra() == 0);

  std::forward_list<Pinned> p;
  p.emplace_front(1, a);
  p.emplace_after(p.cbegin(), 2, Arg{});
  CHECK(p.front().cat == probe::lref && std::next(p.begin())->cat == probe::rref);
}

void inplace_vector_cases() {
  std::inplace_vector<Probe, 16> v;
  back_categories(v);
  probe::reset();
  auto r = v.try_emplace_back(8, Arg{});
  CHECK(r.has_value() && r->cat == probe::rref);
  v.unchecked_emplace_back(9);
  CHECK(counts.made == 2 && counts.extra() == 0);
  probe::reset();
  v.emplace(v.cend(), 10, Arg{});
  CHECK(v.back().cat == probe::rref && counts.made == 1 && counts.extra() == 0);

  std::inplace_vector<Pinned, 4> p;
  Arg a{};
  p.emplace_back(1, a);
  auto pr = p.try_emplace_back(2, std::move(a));
  p.unchecked_emplace_back(3);
  CHECK(pr && pr->cat == probe::rref && p.size() == 3 && p[0].cat == probe::lref);
}

void adaptor_cases() {
  std::stack<Pinned> s;  // deque
  std::queue<Pinned, std::list<Pinned>> q;
  Arg a{};
  CHECK(s.emplace(1, a).cat == probe::lref);
  CHECK(s.emplace(2, std::move(a)).cat == probe::rref);
  CHECK(q.emplace(3, std::as_const(a)).cat == probe::clref);
  CHECK(s.top().key == 2 && q.back().key == 3);

  std::queue<Probe> qp;
  probe::reset();
  qp.emplace(1, Arg{});
  qp.emplace(2, a);
  CHECK(counts.made == 2 && counts.extra() == 0);
  CHECK(qp.front().cat == probe::rref && qp.back().cat == probe::lref);

  // priority_queue may move elements while restoring the heap, but never copies them, and the
  // new element is made from the arguments once.
  std::priority_queue<Probe> pq;
  for (int i = 0; i < 20; ++i) {
    probe::reset();
    pq.emplace((i * 7) % 20, Arg{});
    CHECK(counts.made == 1 && counts.copies == 0 && counts.copy_assigns == 0);
  }
  CHECK(pq.top().key == 19 && pq.top().cat == probe::rref);
}

int main() {
  vector_cases();
  deque_cases();
  list_cases();
  forward_list_cases();
  inplace_vector_cases();
  adaptor_cases();
  return 0;
}
