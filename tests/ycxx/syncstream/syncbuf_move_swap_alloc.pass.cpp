// [syncstream.syncbuf.cons]/1-2: basic_syncbuf(obuf, allocator) has get_wrapped() == obuf and
// get_allocator() == allocator; /5-6: the move constructor takes over the wrapped buffer and the
// stored output; afterwards other.pbase() == other.pptr() and other.get_wrapped() == nullptr,
// so destroying other produces no output; /7: the destructor calls emit().
// [syncstream.syncbuf.assign]/1-4: move assignment "Calls emit() then move assigns from rhs";
// rhs.get_wrapped() == nullptr; the allocator propagates iff propagate_on_container_move_
// assignment, otherwise it is unchanged. /6: swap exchanges the state; [syncstream.syncbuf.
// special]: swap(a, b) is a.swap(b). [syncstream.syncbuf.members]/4: emit() returns false when
// wrapped is null; [syncstream.syncbuf.virtuals]/1-2: with emit-on-sync, sync() calls emit()
// and returns -1 if that returned false. Also with wchar_t.
// [syncstream.osyncstream.cons]/4-5: the osyncstream move constructor moves sb and calls
// set_rdbuf(addressof(sb)); the source's get_wrapped() is null afterwards; rdbuf() is the
// address of the stream's own sb; move assignment ([syncstream.osyncstream.overview]) emits
// the target's pending output first (it move-assigns sb).
#include <syncstream>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include "check.hpp"

template <class T, bool Pocma>
struct Alloc {
  using value_type = T;
  using propagate_on_container_move_assignment = std::bool_constant<Pocma>;
  using propagate_on_container_swap = std::true_type;
  int id = 0;
  Alloc() = default;
  explicit Alloc(int i) : id(i) {}
  template <class U>
  Alloc(const Alloc<U, Pocma>& o) : id(o.id) {}
  template <class U>
  struct rebind {
    using other = Alloc<U, Pocma>;
  };
  T* allocate(std::size_t n) { return std::allocator<T>().allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>().deallocate(p, n); }
  friend bool operator==(const Alloc&, const Alloc&) = default;
};

// Exposes the put area pointers.
template <class Base>
struct Probe : Base {
  using Base::Base;
  Probe(Probe&&) = default;
  Probe& operator=(Probe&&) = default;
  bool put_area_empty() const { return this->pbase() == this->pptr(); }
};

template <class SB>
void move_and_swap() {
  using charT = typename SB::char_type;
  using Str = std::basic_string<charT>;
  auto S = [](const char* s) {
    Str r;
    for (; *s; ++s) r.push_back(static_cast<charT>(*s));
    return r;
  };
  std::basic_stringbuf<charT> out1, out2;
  using A = typename SB::allocator_type;
  {
    Probe<SB> a(&out1, A(1));
    CHECK(a.get_wrapped() == &out1 && a.get_allocator() == A(1));
    a.sputn(S("abc").data(), 3);
    Probe<SB> b(std::move(a));
    CHECK(b.get_wrapped() == &out1);
    CHECK(a.get_wrapped() == nullptr && a.put_area_empty());
    CHECK(!a.emit());  // wrapped == nullptr
    CHECK(out1.str().empty());
    b.sputn(S("de").data(), 2);
    CHECK(b.emit());
    CHECK(out1.str() == S("abcde"));
    b.sputn(S("f").data(), 1);
  }  // b's destructor emits "f"; a's emits nothing
  CHECK(out1.str() == S("abcdef"));

  // move assignment: emit the target's output first, then take over the source
  out1.str(Str());
  {
    Probe<SB> t(&out2, A(2)), s(&out1, A(3));
    t.sputn(S("xyz").data(), 3);
    s.sputn(S("123").data(), 3);
    t = std::move(s);
    CHECK(out2.str() == S("xyz"));
    CHECK(out1.str().empty());
    CHECK(t.get_wrapped() == &out1 && s.get_wrapped() == nullptr);
    if constexpr (std::allocator_traits<A>::propagate_on_container_move_assignment::value)
      CHECK(t.get_allocator() == A(3));
    else
      CHECK(t.get_allocator() == A(2));
    CHECK(t.emit() && out1.str() == S("123"));
  }
  CHECK(out1.str() == S("123") && out2.str() == S("xyz"));

  // swap (member and non-member)
  out1.str(Str());
  out2.str(Str());
  {
    SB p(&out1, A(4)), q(&out2, A(4));
    p.sputn(S("p").data(), 1);
    q.sputn(S("qq").data(), 2);
    p.swap(q);
    CHECK(p.get_wrapped() == &out2 && q.get_wrapped() == &out1);
    CHECK(p.emit() && out2.str() == S("qq"));
    CHECK(q.emit() && out1.str() == S("p"));
    p.sputn(S("1").data(), 1);
    using std::swap;
    swap(p, q);
    CHECK(p.get_wrapped() == &out1 && q.get_wrapped() == &out2);
    CHECK(q.emit() && out2.str() == S("qq1"));
  }

  // emit-on-sync with no wrapped buffer: sync() returns -1
  {
    SB n;
    CHECK(n.get_wrapped() == nullptr);
    n.set_emit_on_sync(true);
    CHECK(n.pubsync() == -1);
    n.set_emit_on_sync(false);
    CHECK(n.pubsync() == 0);
  }
}

void osyncstream_moves() {
  std::ostringstream sink1, sink2;
  {
    std::osyncstream a(sink1);
    a << "hello";
    std::osyncstream b(std::move(a));
    CHECK(a.get_wrapped() == nullptr);
    CHECK(b.get_wrapped() == sink1.rdbuf());
    CHECK(b.rdbuf() != a.rdbuf());
    CHECK(static_cast<std::ostream&>(b).rdbuf() == b.rdbuf());
    CHECK(b.rdbuf()->get_wrapped() == sink1.rdbuf());
    b << " world";
    CHECK(sink1.str().empty());
    b.emit();
    CHECK(b.good() && sink1.str() == "hello world");
    b << '!';
    std::osyncstream c(sink2);
    c << "pending";
    c = std::move(b);  // c's "pending" goes to sink2 first
    CHECK(sink2.str() == "pending");
    CHECK(c.get_wrapped() == sink1.rdbuf() && b.get_wrapped() == nullptr);
    CHECK(static_cast<std::ostream&>(c).rdbuf() == c.rdbuf());
    c << '?';
  }
  CHECK(sink1.str() == "hello world!?");
  CHECK(sink2.str() == "pending");
}

int main() {
  move_and_swap<std::basic_syncbuf<char, std::char_traits<char>, Alloc<char, true>>>();
  move_and_swap<std::basic_syncbuf<char, std::char_traits<char>, Alloc<char, false>>>();
  move_and_swap<std::basic_syncbuf<wchar_t, std::char_traits<wchar_t>, Alloc<wchar_t, true>>>();
  osyncstream_moves();
}
