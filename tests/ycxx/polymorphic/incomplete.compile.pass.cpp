// [polymorphic.general]/6: "The template parameter T of polymorphic may be an incomplete type."
#include <memory>
#include <type_traits>

struct Expr;
struct Ast {
  std::polymorphic<Expr> root;  // Expr incomplete here
  Ast();
  ~Ast();
};
struct Expr {
  virtual ~Expr() = default;
};
Ast::Ast() = default;
Ast::~Ast() = default;

static_assert(std::is_same_v<std::polymorphic<Expr>::value_type, Expr>);
