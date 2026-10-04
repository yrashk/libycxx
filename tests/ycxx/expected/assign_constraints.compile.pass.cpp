// [expected.object.assign]/11: operator=(U&&) constraints, incl. (11.5) is_nothrow_constructible
// _v<T,U> || nothrow-move T || nothrow-move E; /15: operator=(unexpected<G>) constraints incl.
// (15.3). Noexcept of move assignment (/9).
#include <expected>
#include <type_traits>

struct TT {  // nothing is nothrow
  TT(int) noexcept(false) {}
  TT(const TT&) noexcept(false) {}
  TT(TT&&) noexcept(false) {}
  TT& operator=(const TT&) = default;
  TT& operator=(TT&&) = default;
  TT& operator=(int) { return *this; }
};
struct NoAssignFromInt {
  NoAssignFromInt(int) {}
  NoAssignFromInt& operator=(int) = delete;
};

// T=TT, E=TT: neither nothrow-movable; U&& path needs nothrow construction -> excluded,
// and the copy/move assignment fallback is deleted as well.
static_assert(!std::is_assignable_v<std::expected<TT, TT>&, int>);
static_assert(!std::is_assignable_v<std::expected<TT, TT>&, std::unexpected<int>>);
static_assert(std::is_assignable_v<std::expected<TT, int>&, int>);
static_assert(std::is_assignable_v<std::expected<int, TT>&, std::unexpected<int>>);
static_assert(!std::is_assignable_v<std::expected<int, long>&, int*>);
static_assert(!std::is_assignable_v<std::expected<int, int*>&, std::unexpected<long>>);
static_assert(std::is_nothrow_move_assignable_v<std::expected<int, long>>);
static_assert(!std::is_nothrow_move_assignable_v<std::expected<int, TT>>);
