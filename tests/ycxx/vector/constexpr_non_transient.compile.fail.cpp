// [expr.const]/10 (core constant expression): storage allocated with operator new during
// constant evaluation must be deallocated within the same evaluation ("transient"
// allocation). A constexpr vector variable whose elements live in allocated storage would
// keep that storage past the evaluation, so `constexpr std::vector<int> v{1, 2, 3};` is
// ill-formed even though every vector member is constexpr ([vector.overview]).
// (constexpr_vector.pass.cpp checks that transient use in a constant expression works.)
#include <vector>

constexpr std::vector<int> v{1, 2, 3};
