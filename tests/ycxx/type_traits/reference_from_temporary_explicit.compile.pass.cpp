// [meta.unary.prop] reference_constructs_from_temporary uses direct-initialization
// "T t(VAL<U>);", reference_converts_from_temporary uses copy-initialization "T t = VAL<U>;".
// [over.match.ref]/1.1: "For direct-initialization, the permissible types for explicit
// conversion functions are the members of R having the form 'cv T2' ... where T2 can be
// converted to type T with a (possibly trivial) qualification conversion". So an explicit
// conversion to int is used by direct-initialization of const int& (binding to the
// materialized temporary) but not by copy-initialization.
#include <type_traits>

struct ExplicitToInt { explicit operator int() const; };

// Compiler gap: GCC 16 rejects the core-language initialization itself
// ('const int& r(ExplicitToInt{});' is diagnosed as invalid), so its built-in trait cannot
// agree with the draft. Checked on other compilers only.
#if defined(__clang__) || !defined(__GNUC__)

static_assert(std::reference_constructs_from_temporary_v<const int&, ExplicitToInt>);
static_assert(std::reference_constructs_from_temporary_v<int&&, ExplicitToInt>);
static_assert(!std::reference_converts_from_temporary_v<const int&, ExplicitToInt>);
static_assert(!std::reference_converts_from_temporary_v<int&&, ExplicitToInt>);
#endif
