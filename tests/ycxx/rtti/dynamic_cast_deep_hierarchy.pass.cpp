// [expr.dynamic.cast]/9 over a deep hierarchy with a shared virtual base and one extra
// polymorphic base per level: Ch<N> derives from Ch<N-1>, Tag<N> and virtually from V. For a
// complete Ch<N> object and every k: a cast from the V subobject to Ch<k>* is a downcast (/9.1,
// k <= N) and otherwise fails; a cast to Tag<k>* is a cross cast from V or from any other
// Tag<j> (/9.2, unique public base when k <= N); dynamic_cast<void*> yields the complete object
// (/8). Many most derived types, many target types, one remembered-result table: every
// (dynamic type, source, target) triple must be answered separately.
#include <utility>
#include "check.hpp"

struct V {
  virtual ~V() = default;
  int v = 7;
};
template <int N>
struct Tag {
  virtual ~Tag() = default;
  int t = N;
};
template <int N>
struct Ch;
template <>
struct Ch<0> : virtual V, Tag<0> {
  long pad0 = 0;
};
template <int N>
struct Ch : Ch<N - 1>, Tag<N>, virtual V {
  char pad[N % 3 * 8 + 1] = {};
};

constexpr int depth = 14;

// casts from the V subobject and from the Tag<j> subobjects of a Ch<N>
struct Obj {
  V* vp;
  void* whole;
  void* ch[depth + 1];    // expected Ch<k>* (as void*) or null
  void* tag[depth + 1];   // expected Tag<k>* (as void*) or null
  void* tagsrc[depth + 1];  // the object's Tag<j> subobjects (null when j > N)
};

template <int K>
[[gnu::noinline]] static void* v_to_ch(V* p) {
  return dynamic_cast<Ch<K>*>(p);
}
template <int K>
[[gnu::noinline]] static void* v_to_tag(V* p) {
  return dynamic_cast<Tag<K>*>(p);
}
template <int J, int K>
[[gnu::noinline]] static void* tag_to_tag(void* p) {
  return dynamic_cast<Tag<K>*>(static_cast<Tag<J>*>(p));
}

template <int N>
static Obj make() {
  auto* o = new Ch<N>;
  Obj r{};
  r.vp = o;
  r.whole = o;
  [&]<int... K>(std::integer_sequence<int, K...>) {
    ((r.ch[K] = K <= N ? static_cast<void*>(static_cast<Ch<(K <= N ? K : N)>*>(o)) : nullptr), ...);
    ((r.tag[K] = K <= N ? static_cast<void*>(static_cast<Tag<(K <= N ? K : N)>*>(o)) : nullptr), ...);
    ((r.tagsrc[K] = r.tag[K]), ...);
  }(std::make_integer_sequence<int, depth + 1>());
  return r;
}

static void expect(bool ok, int what) {
  if (!ok) dprintf(2, "failed cast %d\n", what);
  CHECK(ok);
}

template <int... K>
static void check_all(const Obj& o, std::integer_sequence<int, K...> seq) {
  CHECK(dynamic_cast<void*>(o.vp) == o.whole);
  (expect(v_to_ch<K>(o.vp) == o.ch[K], 1000 + K), ...);
  (expect(v_to_tag<K>(o.vp) == o.tag[K], 2000 + K), ...);
  // from every existing Tag<J> to every Tag<K>
  auto from = [&]<int J>(std::integral_constant<int, J>) {
    if (o.tagsrc[J]) (expect(tag_to_tag<J, K>(o.tagsrc[J]) == o.tag[K], 3000 + J * 100 + K), ...);
  };
  (from(std::integral_constant<int, K>{}), ...);
  (void)seq;
}

int main() {
  Obj objs[depth + 1];
  [&]<int... N>(std::integer_sequence<int, N...>) {
    ((objs[N] = make<N>()), ...);
  }(std::make_integer_sequence<int, depth + 1>());

  auto seq = std::make_integer_sequence<int, depth + 1>();
  for (int i = 0; i <= depth; ++i) check_all(objs[i], seq);
  for (int i = depth; i >= 0; --i) check_all(objs[i], seq);
  unsigned x = 4242;
  for (int k = 0; k < 300; ++k) {
    x = x * 1103515245u + 12345u;
    check_all(objs[(x >> 16) % (depth + 1)], seq);
  }
  for (auto& o : objs) delete o.vp;  // V has a virtual destructor
  return 0;
}
