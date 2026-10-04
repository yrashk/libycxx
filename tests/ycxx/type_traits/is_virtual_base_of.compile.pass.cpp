// [meta.rel] is_virtual_base_of<Base, Derived>: "Base is a virtual base class of Derived
// without regard to cv-qualifiers." Note 2: private, protected or ambiguous virtual bases
// are nonetheless virtual bases. Note 3: a class is never a virtual base class of itself.
#include <type_traits>

struct B {};
struct VD : virtual B {};
struct VD2 : virtual B {};
struct Indirect : VD {};                   // B is an (indirect) virtual base
struct Diamond : VD, VD2 {};               // single shared virtual B
struct NV : B {};                          // non-virtual base
struct PrivV : private virtual B {};
struct ProtV : protected virtual B {};
struct Mixed : NV, virtual B {};           // B is both a direct non-virtual (via NV) and virtual base
struct ThroughNV : NV {};                  // only non-virtual path
struct Other {};
union U {};

static_assert(std::is_base_of_v<std::true_type, std::is_virtual_base_of<B, VD>>);
static_assert(std::is_base_of_v<std::false_type, std::is_virtual_base_of<B, NV>>);

static_assert(std::is_virtual_base_of_v<B, VD>);
static_assert(std::is_virtual_base_of_v<B, Indirect>);
static_assert(std::is_virtual_base_of_v<B, Diamond>);
static_assert(std::is_virtual_base_of_v<B, PrivV>);
static_assert(std::is_virtual_base_of_v<B, ProtV>);
static_assert(std::is_virtual_base_of_v<B, Mixed>);
static_assert(!std::is_virtual_base_of_v<B, NV>);
static_assert(!std::is_virtual_base_of_v<B, ThroughNV>);
static_assert(!std::is_virtual_base_of_v<NV, Mixed>);    // NV is a non-virtual base of Mixed
static_assert(!std::is_virtual_base_of_v<VD, Indirect>); // VD is a non-virtual base of Indirect
static_assert(!std::is_virtual_base_of_v<VD, Diamond>);

// cv-qualifiers are ignored.
static_assert(std::is_virtual_base_of_v<const B, VD>);
static_assert(std::is_virtual_base_of_v<B, volatile VD>);
static_assert(std::is_virtual_base_of_v<const volatile B, const Indirect>);

// Never a virtual base of itself; not the reverse direction; unrelated classes.
static_assert(!std::is_virtual_base_of_v<B, B>);
static_assert(!std::is_virtual_base_of_v<VD, VD>);
static_assert(!std::is_virtual_base_of_v<const VD, VD>);
static_assert(!std::is_virtual_base_of_v<VD, B>);
static_assert(!std::is_virtual_base_of_v<Other, VD>);

// Non-class types.
static_assert(!std::is_virtual_base_of_v<int, int>);
static_assert(!std::is_virtual_base_of_v<B&, VD&>);
static_assert(!std::is_virtual_base_of_v<B*, VD*>);
static_assert(!std::is_virtual_base_of_v<B, VD[1]>);
static_assert(!std::is_virtual_base_of_v<U, U>);
static_assert(!std::is_virtual_base_of_v<void, void>);

// Base may be incomplete when Derived is not a class type... and the trait is usable
// with an incomplete Base when Derived is complete.
struct Incomplete;
static_assert(!std::is_virtual_base_of_v<Incomplete, VD>);
