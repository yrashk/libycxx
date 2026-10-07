// [meta.reflection]: the queries the own suite (tests/ycxx/meta) does not exercise: linkage and
// storage duration ([meta.reflection.queries]), the member-function and template predicates,
// parameters_of/variable_of/return_type_of, object_of/constant_of, reflect_object/function,
// subobjects_of, annotations_of ([meta.reflection.annotation]), is_applicable_type & co., and
// meta::exception's members ([meta.reflection.exception]).
// REQUIRES: gcc
// FLAGS: -freflection
// GAP: clang P1-09 Clang 23 has no reflection
#include <meta>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace m = std::meta;
consteval m::access_context ctx() { return m::access_context::unchecked(); }

int ext_var;
static int int_var;
thread_local int tl_var;
extern "C" void c_fn();
namespace NS { int x; }
namespace NA = NS;
template <class T> void ftmpl(T);
template <class T> using atmpl = T;
template <class T> constexpr int vtmpl = 0;
template <class T> concept Conc = true;
template <class T> struct ctmpl {};
int operator""_lit(unsigned long long);

struct S {
  S();
  S(const S&);
  S(S&&) = default;
  S& operator=(const S&) = delete;
  S& operator=(S&&);
  ~S();
  explicit operator bool() const;
  template <class T> operator T*() const;
  template <class T> S(T, T);
  virtual void v();
  void cref() const&;
  void rref() &&;
  void nx() noexcept;
  void self(this S&);
  void vararg(int, ...);
  void defarg(int = 1);
  mutable int mm;
  [[=1, =2.0]] int annotated;
  int f(int p);
  template <class T> void mtmpl(T);
  bool operator==(const S&) const;
};
struct Final final : S { void v() override; };
struct Abs { virtual void pv() = 0; };

consteval m::info member(m::info cls, std::string_view name) {
  for (m::info r : m::members_of(cls, ctx()))
    if (m::has_identifier(r) && m::identifier_of(r) == name)
      return r;
  throw "no such member";
}
consteval m::info special(bool (*pred)(m::info)) {
  for (m::info r : m::members_of(^^S, ctx()))
    if (pred(r))
      return r;
  throw "none";
}

// linkage and storage duration
static_assert(m::has_external_linkage(^^ext_var) && m::has_internal_linkage(^^int_var) && m::has_linkage(^^ext_var));
static_assert(!m::has_module_linkage(^^ext_var) && m::has_c_language_linkage(^^c_fn));
static_assert(m::has_static_storage_duration(^^ext_var) && m::has_thread_storage_duration(^^tl_var));
static_assert(!m::has_automatic_storage_duration(^^ext_var));
// templates, aliases, concepts
static_assert(m::is_function_template(^^ftmpl) && m::is_alias_template(^^atmpl) && m::is_variable_template(^^vtmpl));
static_assert(m::is_class_template(^^ctmpl) && m::is_concept(^^Conc) && m::is_template(^^Conc));
static_assert(m::is_namespace_alias(^^NA) && m::is_namespace_member(^^NS::x) && m::is_literal_operator(^^operator""_lit));
static_assert(m::has_template_arguments(^^ctmpl<int>) && m::template_of(^^ctmpl<int>) == ^^ctmpl);
static_assert(m::is_class_member(^^S::f) && m::is_value(m::reflect_constant(1)) && m::is_object(m::reflect_object(ext_var)));
static_assert(m::object_of(^^ext_var) == m::reflect_object(ext_var));
static_assert(m::constant_of(^^vtmpl<int>) == m::reflect_constant(0));
static_assert(m::reflect_function(c_fn) == ^^c_fn);
// special members and member-function predicates
static_assert(m::is_default_constructor(special(m::is_default_constructor)) && m::is_user_provided(special(m::is_copy_constructor)));
static_assert(m::is_defaulted(special(m::is_move_constructor)) && m::is_deleted(special(m::is_copy_assignment)));
static_assert(m::is_move_assignment(special(m::is_move_assignment)) && m::is_destructor(special(m::is_destructor)));
static_assert(m::is_special_member_function(special(m::is_destructor)) && m::is_user_declared(special(m::is_move_constructor)));
static_assert(m::is_assignment(special(m::is_copy_assignment)) && m::is_conversion_function(special(m::is_conversion_function)));
static_assert(m::is_explicit(special(m::is_conversion_function)));
static_assert(m::is_conversion_function_template(special(m::is_conversion_function_template)));
static_assert(m::is_constructor_template(special(m::is_constructor_template)));
static_assert(m::is_operator_function(^^S::operator==) && m::is_function_template(^^S::mtmpl));
static_assert(m::is_virtual(^^S::v) && m::is_override(^^Final::v) && m::is_final(^^Final) && m::is_pure_virtual(^^Abs::pv));
static_assert(m::is_lvalue_reference_qualified(^^S::cref) && m::is_rvalue_reference_qualified(^^S::rref));
static_assert(m::is_const(^^S::cref) && !m::is_volatile(^^S::cref) && m::is_noexcept(^^S::nx));
static_assert(m::is_vararg_function(^^S::vararg) && m::is_mutable_member(^^S::mm) && m::is_protected(^^S::f) == false);
// parameters
static_assert(m::parameters_of(^^S::f).size() == 1 && m::is_function_parameter(m::parameters_of(^^S::f)[0]));
static_assert(m::has_default_argument(m::parameters_of(^^S::defarg)[0]));
static_assert(m::is_explicit_object_parameter(m::parameters_of(^^S::self)[0]));
static_assert(m::return_type_of(^^S::f) == (^^int) && m::type_of(m::parameters_of(^^S::f)[0]) == (^^int));
// subobjects, annotations
static_assert(m::subobjects_of(^^Final, ctx()).size() == 1 + m::nonstatic_data_members_of(^^Final, ctx()).size());
static_assert(m::annotations_of(^^S::annotated).size() == 2 && m::is_annotation(m::annotations_of(^^S::annotated)[0]));
static_assert(m::annotations_of_with_type(^^S::annotated, ^^int).size() == 1);
static_assert(m::extract<int>(m::constant_of(m::annotations_of(^^S::annotated)[0])) == 1);
// type queries
static_assert(m::is_enumerable_type(^^S) && !m::is_structured_binding(^^ext_var) && m::has_parent(^^S::f));
static_assert(m::is_applicable_type(^^int (*)(int, long), ^^std::tuple<int, long>) &&
              !m::is_nothrow_applicable_type(^^int (*)(int, long), ^^std::tuple<int, long>));
static_assert(m::apply_result(^^long (*)(int), ^^std::tuple<int>) == ^^long);
// [meta.reflection.exception]
consteval bool exception_members() {
  try {
    (void)m::identifier_of(^^int*);
  } catch (const m::exception& e) {
    return e.from() == ^^m::identifier_of && std::string_view(e.what()).size() > 0 && e.u8what().size() > 0 &&
           e.where().line() > 0;
  }
  return false;
}
static_assert(exception_members());
static_assert(std::is_base_of_v<std::exception, m::exception>);
static_assert(std::is_same_v<std::remove_cvref_t<decltype(m::exception(std::string_view("x"), ^^int))>, m::exception>);
