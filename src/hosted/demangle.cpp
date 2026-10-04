// libycxx hosted runtime: an Itanium C++ ABI demangler (Itanium C++ ABI 5.1, "External Names
// (a.k.a. Mangling)"), written from that specification.
//
// The parser builds a small tree of nodes (names, types, function encodings) and prints it in
// the style of the toolchains' c++filt. Types print in two parts (left and right of the
// declarator), so that "pointer to function" comes out as "void (*)(int)". The substitution
// table and the template-parameter table follow 5.1.9-5.1.10. Expressions in template
// arguments are supported for the common forms (literals, template and function parameters,
// operators, sizeof/alignof, casts, calls, member access); anything else makes demangle()
// fail, and the caller shows the mangled name.
#include "demangle.hpp"

#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

namespace {

using std::string;
using std::string_view;

// ---- nodes ---------------------------------------------------------------------------------------

struct node {
  enum class kind { other, name, pointer, function, array, qualified, special_function_name };
  kind k = kind::other;
  explicit node(kind kk = kind::other) : k(kk) {}
  virtual ~node() = default;
  virtual void left(string& o) const = 0;
  virtual void right(string&) const {}
  // A declarator part to the right of the name: function parameters or array bounds.
  virtual bool has_right() const { return false; }
  virtual void print(string& o) const {
    left(o);
    right(o);
  }
};
using node_list = std::vector<const node*>;

void print_list(string& o, const node_list& l);

struct text_node final : node {
  string text;
  explicit text_node(string t, kind kk = kind::name) : node(kk), text(static_cast<string&&>(t)) {}
  void left(string& o) const override { o += text; }
};

// A standard abbreviation (Sa, Sb, Ss, Si, So, Sd): its short form, the full form a constructor
// or destructor is qualified with, and the class name such a member is named after.
struct abbrev_node final : node {
  const char* short_form;
  const char* full_form;
  const char* class_name;
  abbrev_node(const char* s, const char* f, const char* c) : node(kind::name), short_form(s), full_form(f), class_name(c) {}
  void left(string& o) const override { o += short_form; }
};

// A pack of template arguments (J ... E), or an expanded parameter pack: prints its elements.
struct list_node final : node {
  node_list elems;
  explicit list_node(node_list e) : elems(static_cast<node_list&&>(e)) {}
  void left(string& o) const override { print_list(o, elems); }
};

struct nested_node final : node {
  const node* qual;
  const node* name;
  nested_node(const node* q, const node* n) : node(kind::name), qual(q), name(n) {}
  void left(string& o) const override {
    qual->print(o);
    o += "::";
    name->print(o);
  }
};

struct template_node final : node {
  const node* name;
  node_list args;
  template_node(const node* n, node_list a) : node(kind::name), name(n), args(static_cast<node_list&&>(a)) {}
  void left(string& o) const override {
    name->print(o);
    if (!o.empty() && o.back() == '<')
      o += ' ';
    o += '<';
    print_list(o, args);
    o += '>';
  }
};

struct qual_node final : node {
  const node* inner;
  string cv; // " const", " volatile", " restrict", in that order
  qual_node(const node* i, string q) : node(kind::qualified), inner(i), cv(static_cast<string&&>(q)) {}
  void left(string& o) const override {
    inner->left(o);
    o += cv;
  }
  void right(string& o) const override { inner->right(o); }
  bool has_right() const override { return inner->has_right(); }
};

struct function_node final : node {
  const node* ret; // null for a function encoding without a return type
  node_list params;
  string quals; // cv- and ref-qualifiers and exception specification, after the parameters
  function_node(const node* r, node_list p, string q)
      : node(kind::function), ret(r), params(static_cast<node_list&&>(p)), quals(static_cast<string&&>(q)) {}
  void left(string& o) const override {
    if (ret)
      ret->left(o);
  }
  void right(string& o) const override {
    o += '(';
    print_list(o, params);
    o += ')';
    o += quals;
    if (ret)
      ret->right(o);
  }
  bool has_right() const override { return true; }
  void print(string& o) const override {
    left(o);
    if (ret)
      o += ' ';
    right(o);
  }
};

const node* resolve(const node* n);

struct pointer_node final : node {
  const node* pointee;
  const char* sym; // "*", "&", "&&"
  pointer_node(const node* p, const char* s) : node(kind::pointer), pointee(p), sym(s) {}
  // A reference to a reference collapses ([dcl.ref]/7): & wins.
  const pointer_node* collapsed(pointer_node& tmp) const {
    if (sym[0] != '&')
      return nullptr;
    auto inner = dynamic_cast<const pointer_node*>(resolve(pointee));
    if (inner == nullptr || inner->sym[0] != '&')
      return nullptr;
    tmp.pointee = inner->pointee;
    tmp.sym = sym[1] == '&' && inner->sym[1] == '&' ? "&&" : "&";
    return &tmp;
  }
  void left(string& o) const override {
    pointer_node tmp(nullptr, "");
    if (const pointer_node* c = collapsed(tmp))
      return c->left(o);
    pointee->left(o);
    if (pointee->has_right()) {
      o += " (";
      o += sym;
    } else {
      o += sym;
    }
  }
  void right(string& o) const override {
    pointer_node tmp(nullptr, "");
    if (const pointer_node* c = collapsed(tmp))
      return c->right(o);
    if (pointee->has_right()) {
      o += ')';
      pointee->right(o);
    }
  }
};

struct array_node final : node {
  const node* elem;
  string dim;
  array_node(const node* e, string d) : node(kind::array), elem(e), dim(static_cast<string&&>(d)) {}
  void left(string& o) const override { elem->left(o); }
  void right(string& o) const override {
    o += " [";
    o += dim;
    o += ']';
    elem->right(o);
  }
  bool has_right() const override { return true; }
};

struct ptrmem_node final : node {
  const node* cls;
  const node* member;
  ptrmem_node(const node* c, const node* m) : node(kind::pointer), cls(c), member(m) {}
  void left(string& o) const override {
    member->left(o);
    o += member->has_right() ? " (" : " ";
    cls->print(o);
    o += "::*";
  }
  void right(string& o) const override {
    if (member->has_right()) {
      o += ')';
      member->right(o);
    }
  }
};

// A function encoding: [return type] name(parameters) qualifiers.
struct encoding_node final : node {
  const node* ret;
  const node* name;
  node_list params;
  string quals;
  encoding_node(const node* r, const node* n, node_list p, string q)
      : ret(r), name(n), params(static_cast<node_list&&>(p)), quals(static_cast<string&&>(q)) {}
  void left(string& o) const override {
    if (ret) {
      ret->left(o);
      if (!ret->has_right() || ret->k != kind::pointer)
        o += ' ';
    }
    name->print(o);
    o += '(';
    print_list(o, params);
    o += ')';
    o += quals;
    if (ret)
      ret->right(o);
  }
};

struct prefix_node final : node {
  const char* prefix;
  const node* inner;
  string suffix;
  prefix_node(const char* p, const node* i, string s = string())
      : prefix(p), inner(i), suffix(static_cast<string&&>(s)) {}
  void left(string& o) const override {
    o += prefix;
    inner->print(o);
    o += suffix;
  }
};

// Printing a pack expansion prints its pattern once per element of the parameter pack it
// expands; a param_node whose argument is a pack prints the element of the current
// iteration. Outside an expansion a pack prints all its elements.
struct pack_state {
  int index = -1; // the element being printed; -1: all of them; -2: only measuring
  int size = -1;  // measured: the size of the pack the pattern names
};
thread_local pack_state pack;

// The template arguments a <template-param> refers to: those of the function encoding being
// parsed. They are bound when its name's template arguments have been read, which can be after
// the reference (in a requires-clause, or a substitution of it), so a reference resolves when
// it is printed.
struct param_scope {
  node_list args;
};

struct param_node final : node {
  const param_scope* scope;
  std::size_t idx;
  param_node(const param_scope* s, std::size_t i) : scope(s), idx(i) {}
  const node* target() const { return scope && idx < scope->args.size() ? scope->args[idx] : nullptr; }
  // The node printed: the argument, or for a pack the element of the current expansion
  // iteration; null when there is none (unbound, or a pack outside an iteration).
  const node* current() const {
    const node* t = target();
    auto l = dynamic_cast<const list_node*>(t);
    if (l == nullptr)
      return t;
    if (pack.index == -2) {
      pack.size = static_cast<int>(l->elems.size());
      return nullptr;
    }
    if (pack.index >= 0 && static_cast<std::size_t>(pack.index) < l->elems.size())
      return l->elems[static_cast<std::size_t>(pack.index)];
    return nullptr;
  }
  void unbound(string& o) const { o += idx == 0 ? string("auto") : "auto:" + std::to_string(idx + 1); }
  void left(string& o) const override {
    if (const node* c = current())
      c->left(o);
    else if (target() == nullptr)
      unbound(o);
    else if (pack.index == -1)
      target()->left(o);
  }
  void right(string& o) const override {
    if (const node* c = current())
      c->right(o);
  }
  bool has_right() const override {
    const node* c = current();
    return c && c->has_right();
  }
  void print(string& o) const override {
    if (const node* c = current())
      c->print(o);
    else if (target() == nullptr)
      unbound(o);
    else if (pack.index == -1)
      target()->print(o);
  }
};

// The node a param_node stands for (in the current expansion iteration); else n itself.
const node* resolve(const node* n) {
  while (auto p = dynamic_cast<const param_node*>(n)) {
    const node* c = p->current();
    if (c == nullptr)
      break;
    n = c;
  }
  return n;
}

struct pack_expansion_node final : node {
  const node* inner;
  explicit pack_expansion_node(const node* i) : inner(i) {}
  void left(string& o) const override {
    const pack_state saved = pack;
    pack = {-2, -1};
    string scratch;
    inner->print(scratch);
    const int n = pack.size;
    if (n < 0) { // no pack to expand: show the pattern
      pack = saved;
      inner->print(o);
      o += "...";
      return;
    }
    for (int i = 0; i < n; ++i) {
      if (i != 0)
        o += ", ";
      pack = {i, -1};
      inner->print(o);
    }
    pack = saved;
  }
};

void print_list(string& o, const node_list& l) {
  bool first = true;
  for (const node* n : l) {
    const string::size_type before = o.size();
    if (!first)
      o += ", ";
    const string::size_type mark = o.size();
    n->print(o);
    if (o.size() == mark) // an empty pack: no separator either
      o.resize(before);
    else
      first = false;
  }
}

// ---- operators -------------------------------------------------------------------------------------

struct op_info {
  char code[3];
  const char* name;
  int arity; // operands in an expression; 0: only an operator name
};
constexpr op_info operators[] = {
    {"nw", "new", 0},      {"na", "new[]", 0},    {"dl", "delete", 1},  {"da", "delete[]", 1}, {"ps", "+", 1},
    {"ng", "-", 1},        {"ad", "&", 1},        {"de", "*", 1},       {"co", "~", 1},        {"pl", "+", 2},
    {"mi", "-", 2},        {"ml", "*", 2},        {"dv", "/", 2},       {"rm", "%", 2},        {"an", "&", 2},
    {"or", "|", 2},        {"eo", "^", 2},        {"aS", "=", 2},       {"pL", "+=", 2},       {"mI", "-=", 2},
    {"mL", "*=", 2},       {"dV", "/=", 2},       {"rM", "%=", 2},      {"aN", "&=", 2},       {"oR", "|=", 2},
    {"eO", "^=", 2},       {"ls", "<<", 2},       {"rs", ">>", 2},      {"lS", "<<=", 2},      {"rS", ">>=", 2},
    {"eq", "==", 2},       {"ne", "!=", 2},       {"lt", "<", 2},       {"gt", ">", 2},        {"le", "<=", 2},
    {"ge", ">=", 2},       {"ss", "<=>", 2},      {"nt", "!", 1},       {"aa", "&&", 2},       {"oo", "||", 2},
    {"pp", "++", 1},       {"mm", "--", 1},       {"cm", ",", 2},       {"pm", "->*", 2},      {"pt", "->", 0},
    {"cl", "()", 0},       {"ix", "[]", 2},       {"qu", "?", 3},       {"aw", "co_await", 1},
};

const op_info* find_operator(const char* p) {
  for (const op_info& op : operators)
    if (op.code[0] == p[0] && op.code[1] == p[1])
      return &op;
  return nullptr;
}

// ---- parser ------------------------------------------------------------------------------------------

class parser {
public:
  parser(const char* first, const char* last) : p_(first), end_(last) {}

  // <mangled-name> ::= _Z <encoding> [. <vendor-specific suffix>]
  bool parse(string& out) {
    if (!consume("_Z"))
      return false;
    const node* e = encoding();
    if (!e)
      return false;
    string suffix;
    while (p_ != end_ && *p_ == '.') { // clones: .cold, .isra.0, .constprop.1, .part.0, ...
      const char* s = p_++;
      while (p_ != end_ && *p_ != '.')
        ++p_;
      while (p_ != end_ && *p_ == '.' && p_ + 1 != end_ && p_[1] >= '0' && p_[1] <= '9') {
        ++p_;
        while (p_ != end_ && *p_ >= '0' && *p_ <= '9')
          ++p_;
      }
      suffix += " [clone ";
      suffix.append(s, p_);
      suffix += ']';
    }
    if (p_ != end_)
      return false;
    e->print(out);
    out += suffix;
    return true;
  }

private:
  const char* p_;
  const char* end_;
  std::vector<std::unique_ptr<node>> arena_;
  node_list subs_;
  param_scope* scope_ = nullptr; // the template parameters T_ refers to
  int depth_ = 0;
  // Set while parsing the name of an encoding: its last template arguments become the
  // template parameters of the function's signature.
  bool encoding_name_ = false;
  bool ended_with_template_args_ = false;
  bool ctor_dtor_conv_ = false;

  struct depth_guard {
    parser& ps;
    bool ok;
    explicit depth_guard(parser& p) : ps(p), ok(++p.depth_ < 256) {}
    ~depth_guard() { --ps.depth_; }
  };

  template <class N, class... Args>
  const N* make(Args&&... args) {
    std::unique_ptr<node> n(new N(static_cast<Args&&>(args)...));
    const N* r = static_cast<const N*>(n.get());
    arena_.push_back(static_cast<std::unique_ptr<node>&&>(n));
    return r;
  }
  std::vector<std::unique_ptr<param_scope>> scopes_;
  param_scope* new_scope() {
    std::unique_ptr<param_scope> sc(new param_scope);
    param_scope* r = sc.get();
    scopes_.push_back(static_cast<std::unique_ptr<param_scope>&&>(sc));
    return r;
  }
  const node* text(string s, node::kind k = node::kind::name) { return make<text_node>(static_cast<string&&>(s), k); }

  char peek(std::size_t i = 0) const { return static_cast<std::size_t>(end_ - p_) > i ? p_[i] : '\0'; }
  bool consume(char c) {
    if (peek() != c)
      return false;
    ++p_;
    return true;
  }
  bool consume(const char* s) {
    const std::size_t n = std::strlen(s);
    if (static_cast<std::size_t>(end_ - p_) < n || std::memcmp(p_, s, n) != 0)
      return false;
    p_ += n;
    return true;
  }
  bool number(std::size_t& v) {
    if (peek() < '0' || peek() > '9')
      return false;
    v = 0;
    while (peek() >= '0' && peek() <= '9') {
      v = v * 10 + static_cast<std::size_t>(*p_++ - '0');
      if (v > (std::size_t(1) << 31))
        return false;
    }
    return true;
  }
  // <seq-id> in base 36 (digits and upper-case letters).
  bool seq_id(std::size_t& v) {
    v = 0;
    bool any = false;
    for (;;) {
      const char c = peek();
      std::size_t d;
      if (c >= '0' && c <= '9')
        d = static_cast<std::size_t>(c - '0');
      else if (c >= 'A' && c <= 'Z')
        d = static_cast<std::size_t>(c - 'A' + 10);
      else
        break;
      v = v * 36 + d;
      ++p_;
      any = true;
      if (v > (std::size_t(1) << 31))
        return false;
    }
    return any;
  }

  // <encoding> ::= <name> <bare-function-type> | <name> | <special-name>
  const node* encoding() {
    depth_guard g(*this);
    if (!g.ok)
      return nullptr;
    if (peek() == 'T' || (peek() == 'G' && (peek(1) == 'V' || peek(1) == 'R' || peek(1) == 'T')))
      return special_name();
    const bool saved_enc = encoding_name_;
    param_scope* const saved_scope = scope_;
    scope_ = new_scope();
    encoding_name_ = true;
    ended_with_template_args_ = false;
    ctor_dtor_conv_ = false;
    string quals;
    const node* n = name(&quals);
    encoding_name_ = saved_enc;
    if (!n)
      return nullptr;
    const bool is_template = ended_with_template_args_, no_return = ctor_dtor_conv_;
    if (p_ == end_ || peek() == 'E' || peek() == '.') { // a data object
      scope_ = saved_scope;
      return n;
    }
    const node* ret = nullptr;
    if (is_template && !no_return) {
      ret = type();
      if (!ret)
        return nullptr;
    }
    node_list params;
    if (!bare_function_type(params))
      return nullptr;
    scope_ = saved_scope;
    return make<encoding_node>(ret, n, static_cast<node_list&&>(params), static_cast<string&&>(quals));
  }

  bool bare_function_type(node_list& params) {
    if (peek() == 'v' && (p_ + 1 == end_ || p_[1] == 'E' || p_[1] == '.')) {
      ++p_;
      return true;
    }
    while (p_ != end_ && peek() != 'E' && peek() != '.') {
      const node* t = type();
      if (!t)
        return false;
      params.push_back(t);
    }
    return !params.empty();
  }

  // <call-offset> ::= h <nv-offset> _ | v <v-offset> _
  bool call_offset() {
    if (consume('h')) {
      consume('n');
      std::size_t v;
      return number(v) && consume('_');
    }
    if (consume('v')) {
      consume('n');
      std::size_t v;
      if (!number(v) || !consume('_'))
        return false;
      consume('n');
      return number(v) && consume('_');
    }
    return false;
  }

  const node* special_name() {
    struct simple {
      const char* code;
      const char* prefix;
    };
    static constexpr simple type_specials[] = {
        {"TV", "vtable for "}, {"TT", "VTT for "}, {"TI", "typeinfo for "}, {"TS", "typeinfo name for "}};
    for (const simple& s : type_specials)
      if (consume(s.code)) {
        const node* t = type();
        return t ? make<prefix_node>(s.prefix, t) : nullptr;
      }
    if (consume("Tc")) {
      if (!call_offset() || !call_offset())
        return nullptr;
      const node* e = encoding();
      return e ? make<prefix_node>("covariant return thunk to ", e) : nullptr;
    }
    if (peek() == 'T' && (peek(1) == 'h' || peek(1) == 'v')) {
      const bool is_virtual = peek(1) == 'v';
      ++p_; // call_offset reads the h or v
      if (!call_offset())
        return nullptr;
      const node* e = encoding();
      return e ? make<prefix_node>(is_virtual ? "virtual thunk to " : "non-virtual thunk to ", e) : nullptr;
    }
    if (consume("TH") || consume("TW")) {
      const bool init = p_[-1] == 'H';
      const node* n = name(nullptr);
      return n ? make<prefix_node>(init ? "TLS init function for " : "TLS wrapper function for ", n) : nullptr;
    }
    if (consume("GV")) {
      const node* n = name(nullptr);
      return n ? make<prefix_node>("guard variable for ", n) : nullptr;
    }
    if (consume("GR")) {
      const node* n = name(nullptr);
      if (!n)
        return nullptr;
      std::size_t v;
      seq_id(v);
      consume('_');
      return make<prefix_node>("reference temporary for ", n);
    }
    if (consume("GTt") || consume("GTn")) {
      const node* e = encoding();
      return e ? make<prefix_node>("transaction clone for ", e) : nullptr;
    }
    return nullptr;
  }

  // <name>. quals receives the cv- and ref-qualifiers of a nested name (member functions).
  const node* name(string* quals) {
    depth_guard g(*this);
    if (!g.ok)
      return nullptr;
    if (peek() == 'N')
      return nested_name(quals);
    if (peek() == 'Z')
      return local_name(quals);
    const node* n;
    if (peek() == 'S' && peek(1) != 't') {
      n = substitution();
      if (!n || peek() != 'I')
        return nullptr; // an <unscoped-template-name> substitution needs its arguments
    } else {
      const bool is_std = consume("St");
      n = unqualified_name(nullptr);
      if (!n)
        return nullptr;
      if (is_std)
        n = make<nested_node>(text("std"), n);
      if (peek() != 'I')
        return n;
      subs_.push_back(n);
    }
    return template_args_of(n);
  }

  const node* template_args_of(const node* n) {
    node_list args;
    if (!template_args(args))
      return nullptr;
    if (encoding_name_) {
      if (scope_)
        scope_->args = args;
      ended_with_template_args_ = true;
    }
    return make<template_node>(n, static_cast<node_list&&>(args));
  }

  string cv_qualifiers() {
    string q;
    bool r = consume('r'), v = consume('V'), c = consume('K');
    if (c)
      q += " const";
    if (v)
      q += " volatile";
    if (r)
      q += " restrict";
    return q;
  }

  // <nested-name> ::= N [<CV-qualifiers>] [<ref-qualifier>] <prefix> <unqualified-name> E
  //               ::= N [<CV-qualifiers>] [<ref-qualifier>] <template-prefix> <template-args> E
  const node* nested_name(string* quals) {
    if (!consume('N'))
      return nullptr;
    string q = cv_qualifiers();
    if (consume('R'))
      q += " &";
    else if (consume('O'))
      q += " &&";
    if (quals)
      *quals = q;
    const node* current = nullptr;
    string last_name; // for constructors and destructors
    bool is_std = false;
    while (!consume('E')) {
      if (p_ == end_)
        return nullptr;
      if (encoding_name_)
        ended_with_template_args_ = false;
      const node* comp = nullptr;
      if (peek() == 'S' && peek(1) == 't') {
        p_ += 2;
        is_std = true;
        continue;
      }
      if (peek() == 'S') {
        if (current)
          return nullptr;
        current = substitution();
        if (!current)
          return nullptr;
        last_name = base_name(current);
        continue;
      }
      if (peek() == 'I') {
        if (!current)
          return nullptr;
        current = template_args_of(current);
        if (!current)
          return nullptr;
        if (peek() != 'E')
          subs_.push_back(current);
        continue;
      }
      if (peek() == 'T') {
        if (current)
          return nullptr;
        current = template_param();
        if (!current)
          return nullptr;
        subs_.push_back(current);
        continue;
      }
      if (peek() == 'D' && (peek(1) == 't' || peek(1) == 'T')) {
        if (current)
          return nullptr;
        current = decltype_node();
        if (!current)
          return nullptr;
        subs_.push_back(current);
        continue;
      }
      if (peek() == 'M') { // <data-member-prefix>: a closure in a member initializer
        ++p_;
        continue;
      }
      // A constructor or destructor of a standard abbreviation names the class in full.
      if (auto a = dynamic_cast<const abbrev_node*>(current);
          a && ((peek() == 'C' && peek(1) != 'p') || (peek() == 'D' && peek(1) >= '0' && peek(1) <= '5')))
        current = text(a->full_form);
      comp = unqualified_name(&last_name);
      if (!comp)
        return nullptr;
      if (is_std) {
        comp = make<nested_node>(text("std"), comp);
        is_std = false;
      }
      current = current ? make<nested_node>(current, comp) : comp;
      if (peek() != 'E')
        subs_.push_back(current);
    }
    return current;
  }

  // The unqualified name a constructor or destructor of the class n is named after.
  static string base_name(const node* n) {
    for (;;) {
      if (auto t = dynamic_cast<const template_node*>(n))
        n = t->name;
      else if (auto s = dynamic_cast<const nested_node*>(n))
        n = s->name;
      else
        break;
    }
    if (auto a = dynamic_cast<const abbrev_node*>(n))
      return a->class_name;
    string o;
    n->print(o);
    return o;
  }

  // <local-name> ::= Z <encoding> E <entity name> [<discriminator>]
  //              ::= Z <encoding> E s [<discriminator>]
  const node* local_name(string* quals) {
    if (!consume('Z'))
      return nullptr;
    const bool saved_enc = encoding_name_;
    const bool saved_ended = ended_with_template_args_, saved_cdc = ctor_dtor_conv_;
    encoding_name_ = false;
    const node* fn = encoding();
    encoding_name_ = saved_enc;
    ended_with_template_args_ = saved_ended;
    ctor_dtor_conv_ = saved_cdc;
    if (!fn || !consume('E'))
      return nullptr;
    const node* entity;
    if (consume('s')) {
      entity = text("string literal");
    } else {
      if (consume('d')) { // a default argument's entity: Ed [<number>] _
        std::size_t v;
        number(v);
        if (!consume('_'))
          return nullptr;
      }
      entity = name(quals);
      if (!entity)
        return nullptr;
    }
    discriminator();
    return make<nested_node>(fn, entity);
  }

  void discriminator() {
    if (peek() != '_')
      return;
    std::size_t v;
    if (peek(1) == '_') {
      p_ += 2;
      number(v);
      consume('_');
    } else if (peek(1) >= '0' && peek(1) <= '9') {
      p_ += 2;
    }
  }

  // <source-name> ::= <positive length number> <identifier>
  const node* source_name() {
    std::size_t n;
    if (!number(n) || n == 0 || static_cast<std::size_t>(end_ - p_) < n)
      return nullptr;
    string id(p_, n);
    p_ += n;
    if (id.rfind("_GLOBAL__N", 0) == 0)
      id = "(anonymous namespace)";
    return text(static_cast<string&&>(id));
  }

  // <unqualified-name> [<abi-tags>]. last_name is the previous component's name (for
  // constructors and destructors) and receives this one's.
  const node* unqualified_name(string* last_name) {
    consume('L'); // internal linkage
    const node* n = nullptr;
    const char c = peek();
    if (c >= '1' && c <= '9') {
      n = source_name();
      if (n && last_name)
        n->print(*last_name = string());
    } else if (c == 'U') {
      n = unnamed_type_name();
    } else if (c == 'C' && (peek(1) == '1' || peek(1) == '2' || peek(1) == '3' || peek(1) == '4' || peek(1) == '5' ||
                            peek(1) == 'I')) {
      if (!last_name || last_name->empty())
        return nullptr;
      p_ += 1;
      if (consume('I')) { // inheriting constructor: CI1 <type>
        ++p_;
        if (!type())
          return nullptr;
      } else {
        ++p_;
      }
      n = text(*last_name);
      if (encoding_name_)
        ctor_dtor_conv_ = true;
    } else if (c == 'D' && (peek(1) == '0' || peek(1) == '1' || peek(1) == '2' || peek(1) == '4' || peek(1) == '5')) {
      if (!last_name || last_name->empty())
        return nullptr;
      p_ += 2;
      n = text("~" + *last_name);
      if (encoding_name_)
        ctor_dtor_conv_ = true;
    } else if (c == 'D' && peek(1) == 'C') { // structured binding: DC <source-name>+ E
      p_ += 2;
      string s = "[";
      bool first = true;
      while (!consume('E')) {
        const node* id = source_name();
        if (!id)
          return nullptr;
        if (!first)
          s += ", ";
        id->print(s);
        first = false;
      }
      n = text(s + "]");
    } else if (c >= 'a' && c <= 'z') {
      n = operator_name();
    }
    if (!n)
      return nullptr;
    while (consume('B')) { // <abi-tag>
      const node* tag = source_name();
      if (!tag)
        return nullptr;
      string s;
      n->print(s);
      s += "[abi:";
      tag->print(s);
      s += ']';
      n = text(static_cast<string&&>(s));
    }
    return n;
  }

  // <unnamed-type-name> ::= Ut [<number>] _ | Ul <lambda-sig> E [<number>] _
  const node* unnamed_type_name() {
    if (consume("Ut")) {
      std::size_t v = 0;
      const bool has = number(v);
      if (!consume('_'))
        return nullptr;
      return text("{unnamed type#" + std::to_string(has ? v + 2 : 1) + "}");
    }
    if (consume("Ul")) {
      // Template parameter declarations of a generic lambda: Ty, Tn <type>, Tt ... E, Tp ...
      if (peek() == 'T' && (peek(1) == 'y' || peek(1) == 'n' || peek(1) == 't' || peek(1) == 'p'))
        return nullptr;
      node_list params;
      param_scope* const saved_scope = scope_;
      scope_ = nullptr; // auto parameters: the lambda's own template parameters
      const bool ok = bare_function_type(params);
      scope_ = saved_scope;
      if (!ok || !consume('E'))
        return nullptr;
      std::size_t v = 0;
      const bool has = number(v);
      if (!consume('_'))
        return nullptr;
      string s = "{lambda(";
      print_list(s, params);
      s += ")#" + std::to_string(has ? v + 2 : 1) + "}";
      return text(static_cast<string&&>(s));
    }
    return nullptr;
  }

  const node* operator_name() {
    if (consume("cv")) { // conversion operator
      const bool saved = encoding_name_;
      encoding_name_ = false;
      const node* t = type();
      encoding_name_ = saved;
      if (!t)
        return nullptr;
      if (encoding_name_)
        ctor_dtor_conv_ = true;
      string s = "operator ";
      t->print(s);
      return text(static_cast<string&&>(s));
    }
    if (consume("li")) { // literal operator
      const node* id = source_name();
      if (!id)
        return nullptr;
      string s = "operator\"\" ";
      id->print(s);
      return text(static_cast<string&&>(s));
    }
    if (peek() == 'v' && peek(1) >= '0' && peek(1) <= '9') { // vendor extended operator
      p_ += 2;
      const node* id = source_name();
      if (!id)
        return nullptr;
      string s = "operator ";
      id->print(s);
      return text(static_cast<string&&>(s));
    }
    if (static_cast<std::size_t>(end_ - p_) < 2)
      return nullptr;
    const op_info* op = find_operator(p_);
    if (!op)
      return nullptr;
    p_ += 2;
    string s = "operator";
    if (op->name[0] >= 'a' && op->name[0] <= 'z')
      s += ' ';
    s += op->name;
    return text(static_cast<string&&>(s));
  }

  // <substitution> ::= S_ | S <seq-id> _ | Sa | Sb | Ss | Si | So | Sd
  const node* substitution() {
    if (!consume('S'))
      return nullptr;
    struct abbrev {
      char c;
      const char* short_form;
      const char* full_form;
      const char* class_name;
    };
    static constexpr abbrev abbrevs[] = {
        {'a', "std::allocator", "std::allocator", "allocator"},
        {'b', "std::basic_string", "std::basic_string", "basic_string"},
        {'s', "std::string", "std::basic_string<char, std::char_traits<char>, std::allocator<char>>", "basic_string"},
        {'i', "std::istream", "std::basic_istream<char, std::char_traits<char>>", "basic_istream"},
        {'o', "std::ostream", "std::basic_ostream<char, std::char_traits<char>>", "basic_ostream"},
        {'d', "std::iostream", "std::basic_iostream<char, std::char_traits<char>>", "basic_iostream"},
    };
    for (const abbrev& a : abbrevs)
      if (consume(a.c))
        return make<abbrev_node>(a.short_form, a.full_form, a.class_name);
    std::size_t idx = 0;
    if (!consume('_')) {
      if (!seq_id(idx) || !consume('_'))
        return nullptr;
      ++idx;
    }
    return idx < subs_.size() ? subs_[idx] : nullptr;
  }

  // <template-param> ::= T_ | T <number> _
  const node* template_param() {
    if (!consume('T'))
      return nullptr;
    std::size_t idx = 0;
    if (!consume('_')) {
      if (!number(idx) || !consume('_'))
        return nullptr;
      ++idx;
    }
    return make<param_node>(scope_, idx);
  }

  // <template-args> ::= I <template-arg>+ E
  bool template_args(node_list& args) {
    if (!consume('I'))
      return false;
    const bool saved = encoding_name_;
    encoding_name_ = false;
    while (!consume('E')) {
      if (p_ == end_)
        return false;
      if (consume('Q')) { // a requires-clause: not shown
        if (!expression())
          return false;
        continue;
      }
      const node* a = template_arg();
      if (!a)
        return false;
      args.push_back(a);
    }
    encoding_name_ = saved;
    return true;
  }

  const node* template_arg() {
    depth_guard g(*this);
    if (!g.ok)
      return nullptr;
    if (peek() == 'L')
      return expr_primary();
    if (consume('X')) {
      const node* e = expression();
      return e && consume('E') ? e : nullptr;
    }
    if (consume('J')) {
      node_list pack;
      while (!consume('E')) {
        if (p_ == end_)
          return nullptr;
        const node* a = template_arg();
        if (!a)
          return nullptr;
        pack.push_back(a);
      }
      return make<list_node>(static_cast<node_list&&>(pack));
    }
    return type();
  }

  const node* decltype_node() {
    if (!consume("Dt") && !consume("DT"))
      return nullptr;
    const node* e = expression();
    if (!e || !consume('E'))
      return nullptr;
    string s = "decltype(";
    e->print(s);
    s += ')';
    return text(static_cast<string&&>(s));
  }

  // <type>
  const node* type() {
    depth_guard g(*this);
    if (!g.ok)
      return nullptr;
    const bool saved = encoding_name_;
    encoding_name_ = false;
    const node* t = type_inner();
    encoding_name_ = saved;
    return t;
  }

  const node* builtin(char c) {
    switch (c) {
    case 'v': return text("void");
    case 'w': return text("wchar_t");
    case 'b': return text("bool");
    case 'c': return text("char");
    case 'a': return text("signed char");
    case 'h': return text("unsigned char");
    case 's': return text("short");
    case 't': return text("unsigned short");
    case 'i': return text("int");
    case 'j': return text("unsigned int");
    case 'l': return text("long");
    case 'm': return text("unsigned long");
    case 'x': return text("long long");
    case 'y': return text("unsigned long long");
    case 'n': return text("__int128");
    case 'o': return text("unsigned __int128");
    case 'f': return text("float");
    case 'd': return text("double");
    case 'e': return text("long double");
    case 'g': return text("__float128");
    case 'z': return text("...");
    default: return nullptr;
    }
  }

  const node* type_inner() {
    const char c = peek();
    if (const node* b = (c >= 'a' && c <= 'z' && c != 'u') ? builtin(c) : nullptr) {
      ++p_;
      return b;
    }
    const node* t = nullptr;
    switch (c) {
    case 'u': { // vendor extended type
      ++p_;
      t = source_name();
      if (t && peek() == 'I')
        t = template_args_of(t);
      break;
    }
    case 'D': {
      const char d = peek(1);
      const char* simple = nullptr;
      switch (d) {
      case 'd': simple = "decimal64"; break;
      case 'e': simple = "decimal128"; break;
      case 'f': simple = "decimal32"; break;
      case 'h': simple = "half"; break;
      case 'i': simple = "char32_t"; break;
      case 's': simple = "char16_t"; break;
      case 'u': simple = "char8_t"; break;
      case 'a': simple = "auto"; break;
      case 'c': simple = "decltype(auto)"; break;
      case 'n': simple = "std::nullptr_t"; break;
      default: break;
      }
      if (simple) {
        p_ += 2;
        return text(simple);
      }
      if (d == 'F') { // DF <number> _ (_FloatN), DF <number> x (_FloatNx), DF16b (bfloat16)
        p_ += 2;
        if (consume("16b"))
          return text("std::bfloat16_t");
        std::size_t n;
        if (!number(n))
          return nullptr;
        if (consume('_'))
          return text("_Float" + std::to_string(n));
        if (consume('x'))
          return text("_Float" + std::to_string(n) + "x");
        return nullptr;
      }
      if (d == 'B' || d == 'U') { // _BitInt(N) / unsigned _BitInt(N)
        p_ += 2;
        std::size_t n;
        if (!number(n) || !consume('_'))
          return nullptr;
        return text(string(d == 'U' ? "unsigned " : "") + "_BitInt(" + std::to_string(n) + ")");
      }
      if (d == 'p') { // pack expansion
        p_ += 2;
        const node* inner = type();
        if (!inner)
          return nullptr;
        t = make<pack_expansion_node>(inner);
        break;
      }
      if (d == 't' || d == 'T') {
        t = decltype_node();
        break;
      }
      if (d == 'v') { // Dv <number> _ <type>: vector type
        p_ += 2;
        std::size_t n;
        if (!number(n) || !consume('_'))
          return nullptr;
        const node* e = type();
        if (!e)
          return nullptr;
        string s;
        e->print(s);
        s += " __vector(" + std::to_string(n) + ")";
        t = text(static_cast<string&&>(s));
        break;
      }
      if (d == 'o' || d == 'O' || d == 'w' || d == 'x') { // exception specification, then F
        t = function_type(string());
        break;
      }
      return nullptr;
    }
    case 'r':
    case 'V':
    case 'K': {
      string q = cv_qualifiers();
      if (peek() == 'F' || (peek() == 'D' && (peek(1) == 'o' || peek(1) == 'O' || peek(1) == 'w' || peek(1) == 'x'))) {
        t = function_type(static_cast<string&&>(q));
        break;
      }
      const node* inner = type();
      if (!inner)
        return nullptr;
      t = make<qual_node>(inner, static_cast<string&&>(q));
      break;
    }
    case 'P':
    case 'R':
    case 'O': {
      ++p_;
      const node* inner = type();
      if (!inner)
        return nullptr;
      t = make<pointer_node>(inner, c == 'P' ? "*" : c == 'R' ? "&" : "&&");
      break;
    }
    case 'C':
    case 'G': {
      ++p_;
      const node* inner = type();
      if (!inner)
        return nullptr;
      string s;
      inner->print(s);
      s += c == 'C' ? " _Complex" : " _Imaginary";
      t = text(static_cast<string&&>(s));
      break;
    }
    case 'F':
      t = function_type(string());
      break;
    case 'A': {
      ++p_;
      string dim;
      if (peek() >= '0' && peek() <= '9') {
        std::size_t n;
        if (!number(n))
          return nullptr;
        dim = std::to_string(n);
      } else if (peek() != '_') {
        const node* e = expression();
        if (!e)
          return nullptr;
        e->print(dim);
      }
      if (!consume('_'))
        return nullptr;
      const node* elem = type();
      if (!elem)
        return nullptr;
      t = make<array_node>(elem, static_cast<string&&>(dim));
      break;
    }
    case 'M': {
      ++p_;
      const node* cls = type();
      if (!cls)
        return nullptr;
      const node* mem = type();
      if (!mem)
        return nullptr;
      t = make<ptrmem_node>(cls, mem);
      break;
    }
    case 'T': {
      t = template_param();
      if (!t)
        return nullptr;
      subs_.push_back(t);
      if (peek() == 'I') { // <template-template-param> <template-args>
        node_list args;
        if (!template_args(args))
          return nullptr;
        t = make<template_node>(t, static_cast<node_list&&>(args));
        break;
      }
      return t;
    }
    case 'S': {
      if (peek(1) == 't') { // std:: class
        t = name(nullptr);
        break;
      }
      const node* s = substitution();
      if (!s)
        return nullptr;
      if (peek() != 'I')
        return s; // a substitution is not added again
      node_list args;
      if (!template_args(args))
        return nullptr;
      t = make<template_node>(s, static_cast<node_list&&>(args));
      break;
    }
    case 'N':
    case 'Z':
      t = name(nullptr);
      break;
    default:
      if (c >= '1' && c <= '9') {
        t = name(nullptr);
        break;
      }
      return nullptr;
    }
    if (t)
      subs_.push_back(t);
    return t;
  }

  // <function-type> ::= [<CV-qualifiers>] [<exception-spec>] [Dx] F [Y] <bare-function-type>
  //                     [<ref-qualifier>] E
  const node* function_type(string quals) {
    if (consume("Do")) {
      quals += " noexcept";
    } else if (consume("DO")) {
      const node* e = expression();
      if (!e || !consume('E'))
        return nullptr;
      quals += " noexcept(";
      e->print(quals);
      quals += ')';
    } else if (consume("Dw")) {
      quals += " throw(";
      bool first = true;
      while (!consume('E')) {
        const node* t = type();
        if (!t)
          return nullptr;
        if (!first)
          quals += ", ";
        t->print(quals);
        first = false;
      }
      quals += ')';
    }
    consume("Dx");
    if (!consume('F'))
      return nullptr;
    consume('Y');
    const node* ret = type();
    if (!ret)
      return nullptr;
    node_list params;
    if (consume('v')) {
      // no parameters
    }
    while (peek() != 'E' && !(peek() == 'R' && peek(1) == 'E') && !(peek() == 'O' && peek(1) == 'E')) {
      if (p_ == end_)
        return nullptr;
      const node* t = type();
      if (!t)
        return nullptr;
      params.push_back(t);
    }
    string ref;
    if (consume('R'))
      ref = " &";
    else if (consume('O'))
      ref = " &&";
    if (!consume('E'))
      return nullptr;
    return make<function_node>(ret, static_cast<node_list&&>(params), quals + ref);
  }

  // <expr-primary> ::= L <type> <value number> E | L <type> <value float> E | L <mangled-name> E
  //                ::= L _Z <encoding> E | LDnE | LDn0E
  const node* expr_primary() {
    if (!consume('L'))
      return nullptr;
    if (consume("_Z")) {
      const bool saved = encoding_name_;
      encoding_name_ = false;
      const node* e = encoding();
      encoding_name_ = saved;
      return e && consume('E') ? e : nullptr;
    }
    if (consume("DnE") || consume("Dn0E"))
      return text("nullptr");
    const char tc = peek();
    const node* t = type();
    if (!t)
      return nullptr;
    string v;
    if (consume('n'))
      v = "-";
    const char* s = p_;
    while (p_ != end_ && *p_ != 'E')
      ++p_;
    if (!consume('E'))
      return nullptr;
    v.append(s, p_ - 1);
    switch (tc) {
    case 'b':
      return text(v == "0" ? "false" : v == "1" ? "true" : "(bool)" + v);
    case 'i': return text(v);
    case 'j': return text(v + "u");
    case 'l': return text(v + "l");
    case 'm': return text(v + "ul");
    case 'x': return text(v + "ll");
    case 'y': return text(v + "ull");
    default: {
      string o = "(";
      t->print(o);
      o += ')';
      o += v;
      return text(static_cast<string&&>(o));
    }
    }
  }

  // <simple-id> ::= <source-name> [<template-args>] (also an operator name)
  const node* simple_id() {
    const node* n = peek() >= '1' && peek() <= '9' ? source_name() : (consume("on"), operator_name());
    if (!n)
      return nullptr;
    if (peek() == 'I') {
      node_list args;
      if (!template_args(args))
        return nullptr;
      n = make<template_node>(n, static_cast<node_list&&>(args));
    }
    return n;
  }

  // <expression>, common forms only.
  const node* expression() {
    depth_guard g(*this);
    if (!g.ok)
      return nullptr;
    const char c = peek();
    if (c == 'T')
      return template_param();
    if (c == 'L')
      return expr_primary();
    if (consume("fp") || consume("fL")) { // function parameter
      if (p_[-1] == 'L') {
        std::size_t lvl;
        if (!number(lvl) || !consume('p'))
          return nullptr;
      }
      cv_qualifiers();
      std::size_t n = 0;
      const bool has = number(n);
      if (!consume('_'))
        return nullptr;
      return text(has ? "fp" + std::to_string(n + 1) : string("fp"));
    }
    if (consume("sp")) {
      const node* e = expression();
      return e ? make<pack_expansion_node>(e) : nullptr;
    }
    if (consume("st") || consume("at")) {
      const bool size = p_[-2] == 's';
      const node* t = type();
      return t ? make<prefix_node>(size ? "sizeof (" : "alignof (", t, ")") : nullptr;
    }
    if (consume("sz") || consume("az")) {
      const bool size = p_[-2] == 's';
      const node* e = expression();
      return e ? make<prefix_node>(size ? "sizeof (" : "alignof (", e, ")") : nullptr;
    }
    if (consume("sZ")) {
      const node* t = template_param();
      return t ? make<prefix_node>("sizeof...(", t, ")") : nullptr;
    }
    if (consume("gs")) { // ::name
      const node* e = expression();
      return e ? make<prefix_node>("::", e) : nullptr;
    }
    if (consume("sr")) { // unresolved names
      const node* scope = nullptr;
      const bool levels = consume('N') || (peek() >= '1' && peek() <= '9');
      if (!(peek() >= '1' && peek() <= '9')) { // sr [N] <unresolved-type>
        scope = type();
        if (!scope)
          return nullptr;
      }
      if (levels) { // <unresolved-qualifier-level>* E
        while (!consume('E')) {
          const node* q = simple_id();
          if (!q)
            return nullptr;
          scope = scope ? make<nested_node>(scope, q) : q;
        }
      }
      const node* n = simple_id();
      if (!n)
        return nullptr;
      return scope ? make<nested_node>(scope, n) : n;
    }
    if (consume("cl")) {
      const node* callee = expression();
      if (!callee)
        return nullptr;
      node_list args;
      while (!consume('E')) {
        if (p_ == end_)
          return nullptr;
        const node* a = expression();
        if (!a)
          return nullptr;
        args.push_back(a);
      }
      string s;
      callee->print(s);
      s += '(';
      print_list(s, args);
      s += ')';
      return text(static_cast<string&&>(s));
    }
    if (consume("cv")) {
      const node* t = type();
      if (!t)
        return nullptr;
      node_list args;
      if (consume('_')) {
        while (!consume('E')) {
          if (p_ == end_)
            return nullptr;
          const node* a = expression();
          if (!a)
            return nullptr;
          args.push_back(a);
        }
      } else {
        const node* a = expression();
        if (!a)
          return nullptr;
        args.push_back(a);
      }
      string s = "(";
      t->print(s);
      s += ")(";
      print_list(s, args);
      s += ')';
      return text(static_cast<string&&>(s));
    }
    if (consume("dt") || consume("pt")) {
      const bool arrow = p_[-2] == 'p';
      const node* obj = expression();
      if (!obj)
        return nullptr;
      const node* member = peek() >= '1' && peek() <= '9' ? unqualified_name(nullptr) : expression();
      if (!member)
        return nullptr;
      string s;
      obj->print(s);
      s += arrow ? "->" : ".";
      member->print(s);
      return text(static_cast<string&&>(s));
    }
    if (static_cast<std::size_t>(end_ - p_) >= 2) {
      if (const op_info* op = find_operator(p_); op && op->arity > 0) {
        p_ += 2;
        node_list operands;
        for (int i = 0; i < op->arity; ++i) {
          const node* e = expression();
          if (!e)
            return nullptr;
          operands.push_back(e);
        }
        string s;
        if (op->arity == 1) {
          s += op->name;
          s += '(';
          operands[0]->print(s);
          s += ')';
        } else if (op->arity == 2) {
          s += '(';
          operands[0]->print(s);
          s += ')';
          if (op->code[0] == 'i' && op->code[1] == 'x') {
            s += '[';
            operands[1]->print(s);
            s += ']';
          } else {
            s += ' ';
            s += op->name;
            s += " (";
            operands[1]->print(s);
            s += ')';
          }
        } else {
          s += '(';
          operands[0]->print(s);
          s += ") ? (";
          operands[1]->print(s);
          s += ") : (";
          operands[2]->print(s);
          s += ')';
        }
        return text(static_cast<string&&>(s));
      }
    }
    if (c >= '1' && c <= '9') // an unresolved name
      return simple_id();
    return nullptr;
  }
};

} // namespace

bool ycxx::detail::demangle(const char* mangled, std::string& out) {
  if (mangled == nullptr || std::strncmp(mangled, "_Z", 2) != 0)
    return false;
  parser ps(mangled, mangled + std::strlen(mangled));
  std::string s;
  if (!ps.parse(s))
    return false;
  out = static_cast<std::string&&>(s);
  return true;
}
