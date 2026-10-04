// [concept.invocable]: invocable<F, Args...> = requires(F&& f, Args&&... args) {
// invoke(std::forward<F>(f), std::forward<Args>(args)...); }. [concept.regularinvocable],
// [concept.predicate]: predicate = regular_invocable<F, Args...> &&
// boolean-testable<invoke_result_t<F, Args...>>. [concept.relation], [concept.equiv],
// [concept.strictweakorder].
#include <concepts>

struct S {
  int m;
  int f(int);
};
struct NotBool {};
struct Pred {
  bool operator()(int) const;
};
struct NotPred {
  NotBool operator()(int) const;
};
struct OneWay {
  bool operator()(int, long) const;
  bool operator()(long, int) const = delete;
};
struct Rel {
  bool operator()(int, int) const;
};
struct LValueOnly {
  bool operator()(int) &;
};

static_assert(std::invocable<int (*)(int), int>);
static_assert(std::invocable<int (*)(int), short>);
static_assert(!std::invocable<int (*)(int)>);
static_assert(std::invocable<int S::*, S&>);
static_assert(std::invocable<int (S::*)(int), S*, int>);
static_assert(!std::invocable<int (S::*)(int), const S&, int>);
static_assert(!std::invocable<int, int>);
static_assert(std::invocable<LValueOnly&, int>);
static_assert(!std::invocable<LValueOnly, int>);
static_assert(std::regular_invocable<Pred, int>);
static_assert(std::predicate<Pred, int>);
static_assert(std::predicate<int* (*)(int), int>);  // pointers are boolean-testable
static_assert(!std::predicate<NotPred, int>);
static_assert(!std::predicate<void (*)(int), int>);
static_assert(std::relation<Rel, int, int>);
static_assert(std::relation<Rel, int, short>);
static_assert(!std::relation<OneWay, int, long>);  // needs all four argument orders
static_assert(std::equivalence_relation<Rel, int, int>);
static_assert(std::strict_weak_order<Rel, int, int>);
static_assert(!std::relation<Pred, int, int>);
