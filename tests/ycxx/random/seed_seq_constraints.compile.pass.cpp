// [rand.util.seedseq]/2: the initializer_list<T> constructor has "Constraints: T is an integer
// type." [rand.req.seedseq]: seed_seq meets the seed sequence requirements (S(), S(ib, ie),
// S(il), q.generate(rb, re), r.size(), r.param(ob)).
#include <random>
#include <cstdint>
#include <initializer_list>
#include <type_traits>

static_assert(std::is_constructible_v<std::seed_seq, std::initializer_list<int>>);
static_assert(std::is_constructible_v<std::seed_seq, std::initializer_list<unsigned long long>>);
static_assert(std::is_constructible_v<std::seed_seq, std::initializer_list<char>>);
static_assert(!std::is_constructible_v<std::seed_seq, std::initializer_list<double>>);
static_assert(!std::is_constructible_v<std::seed_seq, std::initializer_list<float>>);
static_assert(!std::is_constructible_v<std::seed_seq, std::initializer_list<int*>>);

template <class S>
concept seed_sequence = requires(S q, const S r, std::uint32_t* rb, std::uint_least32_t* ob,
                                 const unsigned* ib, std::initializer_list<typename S::result_type> il) {
  S();
  S(ib, ib);
  S(il);
  { q.generate(rb, rb) } -> std::same_as<void>;
  { r.size() } -> std::same_as<std::size_t>;
  { r.param(ob) } -> std::same_as<void>;
};
static_assert(seed_sequence<std::seed_seq>);
static_assert(std::is_unsigned_v<std::seed_seq::result_type> && sizeof(std::seed_seq::result_type) * 8 >= 32);

int main() {}
