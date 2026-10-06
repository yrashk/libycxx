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
  virtual void left(string& __o) const = 0;
  virtual void right(string&) const {}
  // A declarator part to the right of the name: function parameters or array bounds.
  virtual bool has_right() const { return false; }
  virtual void print(string& __o) const {
    left(__o);
    right(__o);
  }
};
using node_list = std::vector<const node*>;

void print_list(string& __o, const node_list& __l);

// Substitutions and template parameters share nodes, so a crafted name can describe an output
// exponentially longer than itself, or (through a template parameter that refers to its own
// arguments) an endless one: printing stops at this size and demangle() then fails.
constexpr std::size_t max_output = 1 << 16;
bool over(const string& __o) { return __o.size() > max_output; }

struct text_node final : node {
  string __text;
  explicit text_node(string t, kind kk = kind::name) : node(kk), __text(static_cast<string&&>(t)) {}
  void left(string& __o) const override { __o += __text; }
};

// A standard abbreviation (Sa, Sb, Ss, Si, So, Sd): its short form, the full form a constructor
// or destructor is qualified with, and the class name such a member is named after.
struct abbrev_node final : node {
  const char* short_form;
  const char* full_form;
  const char* class_name;
  abbrev_node(const char* s, const char* __f, const char* c) : node(kind::name), short_form(s), full_form(__f), class_name(c) {}
  void left(string& __o) const override { __o += short_form; }
};

// A pack of template arguments (J ... E), or an expanded parameter pack: prints its elements.
struct __list_node final : node {
  node_list __y_elems;
  explicit __list_node(node_list e) : __y_elems(static_cast<node_list&&>(e)) {}
  void left(string& __o) const override { print_list(__o, __y_elems); }
};

struct nested_node final : node {
  const node* qual;
  const node* name;
  nested_node(const node* __q, const node* n) : node(kind::name), qual(__q), name(n) {}
  void left(string& __o) const override {
    if (over(__o))
      return;
    qual->print(__o);
    __o += "::";
    name->print(__o);
  }
};

struct template_node final : node {
  const node* name;
  node_list __args;
  template_node(const node* n, node_list a) : node(kind::name), name(n), __args(static_cast<node_list&&>(a)) {}
  void left(string& __o) const override {
    if (over(__o))
      return;
    name->print(__o);
    if (!__o.empty() && __o.back() == '<')
      __o += ' ';
    __o += '<';
    print_list(__o, __args);
    __o += '>';
  }
};

struct qual_node final : node {
  const node* __inner;
  string __cv; // " const", " volatile", " restrict", in that order
  qual_node(const node* i, string __q) : node(kind::qualified), __inner(i), __cv(static_cast<string&&>(__q)) {}
  void left(string& __o) const override {
    __inner->left(__o);
    __o += __cv;
  }
  void right(string& __o) const override { __inner->right(__o); }
  bool has_right() const override { return __inner->has_right(); }
};

struct function_node final : node {
  const node* __ret; // null for a function encoding without a return type
  node_list __params;
  string __quals; // cv- and ref-qualifiers and exception specification, after the parameters
  function_node(const node* r, node_list p, string __q)
      : node(kind::function), __ret(r), __params(static_cast<node_list&&>(p)), __quals(static_cast<string&&>(__q)) {}
  void left(string& __o) const override {
    if (__ret)
      __ret->left(__o);
  }
  void right(string& __o) const override {
    __o += '(';
    print_list(__o, __params);
    __o += ')';
    __o += __quals;
    if (__ret)
      __ret->right(__o);
  }
  bool has_right() const override { return true; }
  void print(string& __o) const override {
    left(__o);
    if (__ret)
      __o += ' ';
    right(__o);
  }
};

const node* __resolve(const node* n);

struct pointer_node final : node {
  const node* __pointee;
  const char* __sym; // "*", "&", "&&"
  pointer_node(const node* p, const char* s) : node(kind::pointer), __pointee(p), __sym(s) {}
  // A reference to a reference collapses ([dcl.ref]/7): & wins.
  const pointer_node* collapsed(pointer_node& __tmp) const {
    if (__sym[0] != '&')
      return nullptr;
    auto __inner = dynamic_cast<const pointer_node*>(__resolve(__pointee));
    if (__inner == nullptr || __inner->__sym[0] != '&')
      return nullptr;
    __tmp.__pointee = __inner->__pointee;
    __tmp.__sym = __sym[1] == '&' && __inner->__sym[1] == '&' ? "&&" : "&";
    return &__tmp;
  }
  void left(string& __o) const override {
    pointer_node __tmp(nullptr, "");
    if (const pointer_node* c = collapsed(__tmp))
      return c->left(__o);
    __pointee->left(__o);
    if (__pointee->has_right()) {
      __o += " (";
      __o += __sym;
    } else {
      __o += __sym;
    }
  }
  void right(string& __o) const override {
    pointer_node __tmp(nullptr, "");
    if (const pointer_node* c = collapsed(__tmp))
      return c->right(__o);
    if (__pointee->has_right()) {
      __o += ')';
      __pointee->right(__o);
    }
  }
};

struct array_node final : node {
  const node* __elem;
  string dim;
  array_node(const node* e, string d) : node(kind::array), __elem(e), dim(static_cast<string&&>(d)) {}
  void left(string& __o) const override { __elem->left(__o); }
  void right(string& __o) const override {
    __o += " [";
    __o += dim;
    __o += ']';
    __elem->right(__o);
  }
  bool has_right() const override { return true; }
};

struct ptrmem_node final : node {
  const node* __cls;
  const node* __member;
  ptrmem_node(const node* c, const node* m) : node(kind::pointer), __cls(c), __member(m) {}
  void left(string& __o) const override {
    __member->left(__o);
    __o += __member->has_right() ? " (" : " ";
    __cls->print(__o);
    __o += "::*";
  }
  void right(string& __o) const override {
    if (__member->has_right()) {
      __o += ')';
      __member->right(__o);
    }
  }
};

// A function encoding: [return type] name(parameters) qualifiers.
struct encoding_node final : node {
  const node* __ret;
  const node* name;
  node_list __params;
  string __quals;
  encoding_node(const node* r, const node* n, node_list p, string __q)
      : __ret(r), name(n), __params(static_cast<node_list&&>(p)), __quals(static_cast<string&&>(__q)) {}
  void left(string& __o) const override {
    if (__ret) {
      __ret->left(__o);
      if (!__ret->has_right() || __ret->k != kind::pointer)
        __o += ' ';
    }
    name->print(__o);
    __o += '(';
    print_list(__o, __params);
    __o += ')';
    __o += __quals;
    if (__ret)
      __ret->right(__o);
  }
};

struct prefix_node final : node {
  const char* prefix;
  const node* __inner;
  string suffix;
  prefix_node(const char* p, const node* i, string s = string())
      : prefix(p), __inner(i), suffix(static_cast<string&&>(s)) {}
  void left(string& __o) const override {
    __o += prefix;
    __inner->print(__o);
    __o += suffix;
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
  node_list __args;
};

struct param_node final : node {
  const param_scope* scope;
  std::size_t __idx;
  param_node(const param_scope* s, std::size_t i) : scope(s), __idx(i) {}
  const node* target() const { return scope && __idx < scope->__args.size() ? scope->__args[__idx] : nullptr; }
  // The node printed: the argument, or for a pack the element of the current expansion
  // iteration; null when there is none (unbound, or a pack outside an iteration).
  const node* current() const {
    const node* t = target();
    auto __l = dynamic_cast<const __list_node*>(t);
    if (__l == nullptr)
      return t;
    if (pack.index == -2) {
      pack.size = static_cast<int>(__l->__y_elems.size());
      return nullptr;
    }
    if (pack.index >= 0 && static_cast<std::size_t>(pack.index) < __l->__y_elems.size())
      return __l->__y_elems[static_cast<std::size_t>(pack.index)];
    return nullptr;
  }
  void unbound(string& __o) const { __o += __idx == 0 ? string("auto") : "auto:" + std::to_string(__idx + 1); }
  // A reference printed inside its own argument (T_ within the arguments it names) would never
  // end; it is printed as unbound instead.
  struct active_guard {
    const param_node* __self;
    bool cycle;
    explicit active_guard(const param_node* p) : __self(p), cycle(false) {
      for (const param_node* a : __active())
        cycle = cycle || a == p;
      if (!cycle)
        __active().push_back(p);
    }
    ~active_guard() {
      if (!cycle)
        __active().pop_back();
    }
  };
  static std::vector<const param_node*>& __active() {
    thread_local std::vector<const param_node*> __v;
    return __v;
  }
  void left(string& __o) const override {
    const active_guard __g(this);
    if (__g.cycle || over(__o))
      return unbound(__o);
    if (const node* c = current())
      c->left(__o);
    else if (target() == nullptr)
      unbound(__o);
    else if (pack.index == -1)
      target()->left(__o);
  }
  void right(string& __o) const override {
    const active_guard __g(this);
    if (__g.cycle || over(__o))
      return;
    if (const node* c = current())
      c->right(__o);
  }
  bool has_right() const override {
    const active_guard __g(this);
    if (__g.cycle)
      return false;
    const node* c = current();
    return c && c->has_right();
  }
  void print(string& __o) const override {
    const active_guard __g(this);
    if (__g.cycle || over(__o))
      return unbound(__o);
    if (const node* c = current())
      c->print(__o);
    else if (target() == nullptr)
      unbound(__o);
    else if (pack.index == -1)
      target()->print(__o);
  }
};

// The node a param_node stands for (in the current expansion iteration); else n itself.
const node* __resolve(const node* n) {
  for (int hops = 0; hops < 16; ++hops) { // a parameter can name another (or, crafted, itself)
    auto p = dynamic_cast<const param_node*>(n);
    if (p == nullptr)
      break;
    const node* c = p->current();
    if (c == nullptr)
      break;
    n = c;
  }
  return n;
}

struct pack_expansion_node final : node {
  const node* __inner;
  explicit pack_expansion_node(const node* i) : __inner(i) {}
  void left(string& __o) const override {
    const pack_state __saved = pack;
    pack = {-2, -1};
    string scratch;
    __inner->print(scratch);
    const int n = pack.size;
    if (n < 0) { // no pack to expand: show the pattern
      pack = __saved;
      __inner->print(__o);
      __o += "...";
      return;
    }
    for (int i = 0; i < n; ++i) {
      if (i != 0)
        __o += ", ";
      pack = {i, -1};
      __inner->print(__o);
    }
    pack = __saved;
  }
};

void print_list(string& __o, const node_list& __l) {
  bool first = true;
  for (const node* n : __l) {
    if (over(__o))
      return;
    const string::size_type before = __o.size();
    if (!first)
      __o += ", ";
    const string::size_type __mark = __o.size();
    n->print(__o);
    if (__o.size() == __mark) // an empty pack: no separator either
      __o.resize(before);
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
  parser(const char* first, const char* last) : __p_(first), __end_(last) {}

  // <mangled-name> ::= _Z <encoding> [. <vendor-specific suffix>]
  bool parse(string& out) {
    if (!consume("_Z"))
      return false;
    const node* e = encoding();
    if (!e)
      return false;
    string suffix;
    while (__p_ != __end_ && *__p_ == '.') { // clones: .cold, .isra.0, .constprop.1, .part.0, ...
      const char* s = __p_++;
      while (__p_ != __end_ && *__p_ != '.')
        ++__p_;
      while (__p_ != __end_ && *__p_ == '.' && __p_ + 1 != __end_ && __p_[1] >= '0' && __p_[1] <= '9') {
        ++__p_;
        while (__p_ != __end_ && *__p_ >= '0' && *__p_ <= '9')
          ++__p_;
      }
      suffix += " [clone ";
      suffix.append(s, __p_);
      suffix += ']';
    }
    if (__p_ != __end_)
      return false;
    e->print(out);
    out += suffix;
    return true;
  }

private:
  const char* __p_;
  const char* __end_;
  std::vector<std::unique_ptr<node>> arena_;
  node_list __subs_;
  param_scope* __scope_ = nullptr; // the template parameters T_ refers to
  int __depth_ = 0;
  // Set while parsing the name of an encoding: its last template arguments become the
  // template parameters of the function's signature.
  bool encoding_name_ = false;
  bool ended_with_template_args_ = false;
  bool ctor_dtor_conv_ = false;

  struct depth_guard {
    parser& __ps;
    bool ok;
    explicit depth_guard(parser& p) : __ps(p), ok(++p.__depth_ < 256) {}
    ~depth_guard() { --__ps.__depth_; }
  };

  template <class _Np, class... _Args>
  const _Np* __make(_Args&&... __args) {
    std::unique_ptr<node> n(new _Np(static_cast<_Args&&>(__args)...));
    const _Np* r = static_cast<const _Np*>(n.get());
    arena_.push_back(static_cast<std::unique_ptr<node>&&>(n));
    return r;
  }
  std::vector<std::unique_ptr<param_scope>> scopes_;
  param_scope* new_scope() {
    std::unique_ptr<param_scope> __sc(new param_scope);
    param_scope* r = __sc.get();
    scopes_.push_back(static_cast<std::unique_ptr<param_scope>&&>(__sc));
    return r;
  }
  const node* __text(string s, node::kind k = node::kind::name) { return __make<text_node>(static_cast<string&&>(s), k); }

  char peek(std::size_t i = 0) const { return static_cast<std::size_t>(__end_ - __p_) > i ? __p_[i] : '\0'; }
  bool consume(char c) {
    if (peek() != c)
      return false;
    ++__p_;
    return true;
  }
  bool consume(const char* s) {
    const std::size_t n = std::strlen(s);
    if (static_cast<std::size_t>(__end_ - __p_) < n || std::memcmp(__p_, s, n) != 0)
      return false;
    __p_ += n;
    return true;
  }
  bool __number(std::size_t& __v) {
    if (peek() < '0' || peek() > '9')
      return false;
    __v = 0;
    while (peek() >= '0' && peek() <= '9') {
      __v = __v * 10 + static_cast<std::size_t>(*__p_++ - '0');
      if (__v > (std::size_t(1) << 31))
        return false;
    }
    return true;
  }
  // <seq-id> in base 36 (digits and upper-case letters).
  bool seq_id(std::size_t& __v) {
    __v = 0;
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
      __v = __v * 36 + d;
      ++__p_;
      any = true;
      if (__v > (std::size_t(1) << 31))
        return false;
    }
    return any;
  }

  // <encoding> ::= <name> <bare-function-type> | <name> | <special-name>
  const node* encoding() {
    depth_guard __g(*this);
    if (!__g.ok)
      return nullptr;
    if (peek() == 'T' || (peek() == 'G' && (peek(1) == 'V' || peek(1) == 'R' || peek(1) == 'T')))
      return special_name();
    const bool saved_enc = encoding_name_;
    param_scope* const saved_scope = __scope_;
    __scope_ = new_scope();
    encoding_name_ = true;
    ended_with_template_args_ = false;
    ctor_dtor_conv_ = false;
    string __quals;
    const node* n = name(&__quals);
    encoding_name_ = saved_enc;
    if (!n)
      return nullptr;
    const bool is_template = ended_with_template_args_, no_return = ctor_dtor_conv_;
    if (__p_ == __end_ || peek() == 'E' || peek() == '.') { // a data object
      __scope_ = saved_scope;
      return n;
    }
    const node* __ret = nullptr;
    if (is_template && !no_return) {
      __ret = type();
      if (!__ret)
        return nullptr;
    }
    node_list __params;
    if (!bare_function_type(__params))
      return nullptr;
    __scope_ = saved_scope;
    return __make<encoding_node>(__ret, n, static_cast<node_list&&>(__params), static_cast<string&&>(__quals));
  }

  bool bare_function_type(node_list& __params) {
    if (peek() == 'v' && (__p_ + 1 == __end_ || __p_[1] == 'E' || __p_[1] == '.')) {
      ++__p_;
      return true;
    }
    while (__p_ != __end_ && peek() != 'E' && peek() != '.') {
      const node* t = type();
      if (!t)
        return false;
      __params.push_back(t);
    }
    return !__params.empty();
  }

  // <call-offset> ::= h <nv-offset> _ | v <v-offset> _
  bool call_offset() {
    if (consume('h')) {
      consume('n');
      std::size_t __v;
      return __number(__v) && consume('_');
    }
    if (consume('v')) {
      consume('n');
      std::size_t __v;
      if (!__number(__v) || !consume('_'))
        return false;
      consume('n');
      return __number(__v) && consume('_');
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
        return t ? __make<prefix_node>(s.prefix, t) : nullptr;
      }
    if (consume("Tc")) {
      if (!call_offset() || !call_offset())
        return nullptr;
      const node* e = encoding();
      return e ? __make<prefix_node>("covariant return thunk to ", e) : nullptr;
    }
    if (peek() == 'T' && (peek(1) == 'h' || peek(1) == 'v')) {
      const bool is_virtual = peek(1) == 'v';
      ++__p_; // call_offset reads the h or v
      if (!call_offset())
        return nullptr;
      const node* e = encoding();
      return e ? __make<prefix_node>(is_virtual ? "virtual thunk to " : "non-virtual thunk to ", e) : nullptr;
    }
    if (consume("TH") || consume("TW")) {
      const bool init = __p_[-1] == 'H';
      const node* n = name(nullptr);
      return n ? __make<prefix_node>(init ? "TLS init function for " : "TLS wrapper function for ", n) : nullptr;
    }
    if (consume("GV")) {
      const node* n = name(nullptr);
      return n ? __make<prefix_node>("guard variable for ", n) : nullptr;
    }
    if (consume("GR")) {
      const node* n = name(nullptr);
      if (!n)
        return nullptr;
      std::size_t __v;
      seq_id(__v);
      consume('_');
      return __make<prefix_node>("reference temporary for ", n);
    }
    if (consume("GTt") || consume("GTn")) {
      const node* e = encoding();
      return e ? __make<prefix_node>("transaction clone for ", e) : nullptr;
    }
    return nullptr;
  }

  // <name>. quals receives the cv- and ref-qualifiers of a nested name (member functions).
  const node* name(string* __quals) {
    depth_guard __g(*this);
    if (!__g.ok)
      return nullptr;
    if (peek() == 'N')
      return nested_name(__quals);
    if (peek() == 'Z')
      return local_name(__quals);
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
        n = __make<nested_node>(__text("std"), n);
      if (peek() != 'I')
        return n;
      __subs_.push_back(n);
    }
    return template_args_of(n);
  }

  const node* template_args_of(const node* n) {
    node_list __args;
    if (!template_args(__args))
      return nullptr;
    if (encoding_name_) {
      if (__scope_)
        __scope_->__args = __args;
      ended_with_template_args_ = true;
    }
    return __make<template_node>(n, static_cast<node_list&&>(__args));
  }

  string cv_qualifiers() {
    string __q;
    bool r = consume('r'), __v = consume('V'), c = consume('K');
    if (c)
      __q += " const";
    if (__v)
      __q += " volatile";
    if (r)
      __q += " restrict";
    return __q;
  }

  // <nested-name> ::= N [<CV-qualifiers>] [<ref-qualifier>] <prefix> <unqualified-name> E
  //               ::= N [<CV-qualifiers>] [<ref-qualifier>] <template-prefix> <template-args> E
  const node* nested_name(string* __quals) {
    if (!consume('N'))
      return nullptr;
    string __q = cv_qualifiers();
    if (consume('R'))
      __q += " &";
    else if (consume('O'))
      __q += " &&";
    if (__quals)
      *__quals = __q;
    const node* current = nullptr;
    string last_name; // for constructors and destructors
    bool is_std = false;
    while (!consume('E')) {
      if (__p_ == __end_)
        return nullptr;
      if (encoding_name_)
        ended_with_template_args_ = false;
      const node* comp = nullptr;
      if (peek() == 'S' && peek(1) == 't') {
        __p_ += 2;
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
          __subs_.push_back(current);
        continue;
      }
      if (peek() == 'T') {
        if (current)
          return nullptr;
        current = template_param();
        if (!current)
          return nullptr;
        __subs_.push_back(current);
        continue;
      }
      if (peek() == 'D' && (peek(1) == 't' || peek(1) == 'T')) {
        if (current)
          return nullptr;
        current = decltype_node();
        if (!current)
          return nullptr;
        __subs_.push_back(current);
        continue;
      }
      if (peek() == 'M') { // <data-member-prefix>: a closure in a member initializer
        ++__p_;
        continue;
      }
      // A constructor or destructor of a standard abbreviation names the class in full.
      if (auto a = dynamic_cast<const abbrev_node*>(current);
          a && ((peek() == 'C' && peek(1) != 'p') || (peek() == 'D' && peek(1) >= '0' && peek(1) <= '5')))
        current = __text(a->full_form);
      comp = unqualified_name(&last_name);
      if (!comp)
        return nullptr;
      if (is_std) {
        comp = __make<nested_node>(__text("std"), comp);
        is_std = false;
      }
      current = current ? __make<nested_node>(current, comp) : comp;
      if (peek() != 'E')
        __subs_.push_back(current);
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
    string __o;
    n->print(__o);
    return __o;
  }

  // <local-name> ::= Z <encoding> E <entity name> [<discriminator>]
  //              ::= Z <encoding> E s [<discriminator>]
  const node* local_name(string* __quals) {
    if (!consume('Z'))
      return nullptr;
    const bool saved_enc = encoding_name_;
    const bool saved_ended = ended_with_template_args_, saved_cdc = ctor_dtor_conv_;
    encoding_name_ = false;
    const node* __fn = encoding();
    encoding_name_ = saved_enc;
    ended_with_template_args_ = saved_ended;
    ctor_dtor_conv_ = saved_cdc;
    if (!__fn || !consume('E'))
      return nullptr;
    const node* entity;
    if (consume('s')) {
      entity = __text("string literal");
    } else {
      if (consume('d')) { // a default argument's entity: Ed [<number>] _
        std::size_t __v;
        __number(__v);
        if (!consume('_'))
          return nullptr;
      }
      entity = name(__quals);
      if (!entity)
        return nullptr;
    }
    discriminator();
    return __make<nested_node>(__fn, entity);
  }

  void discriminator() {
    if (peek() != '_')
      return;
    std::size_t __v;
    if (peek(1) == '_') {
      __p_ += 2;
      __number(__v);
      consume('_');
    } else if (peek(1) >= '0' && peek(1) <= '9') {
      __p_ += 2;
    }
  }

  // <source-name> ::= <positive length number> <identifier>
  const node* source_name() {
    std::size_t n;
    if (!__number(n) || n == 0 || static_cast<std::size_t>(__end_ - __p_) < n)
      return nullptr;
    string id(__p_, n);
    __p_ += n;
    if (id.rfind("_GLOBAL__N", 0) == 0)
      id = "(anonymous namespace)";
    return __text(static_cast<string&&>(id));
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
      __p_ += 1;
      if (consume('I')) { // inheriting constructor: CI1 <type>
        ++__p_;
        if (!type())
          return nullptr;
      } else {
        ++__p_;
      }
      n = __text(*last_name);
      if (encoding_name_)
        ctor_dtor_conv_ = true;
    } else if (c == 'D' && (peek(1) == '0' || peek(1) == '1' || peek(1) == '2' || peek(1) == '4' || peek(1) == '5')) {
      if (!last_name || last_name->empty())
        return nullptr;
      __p_ += 2;
      n = __text("~" + *last_name);
      if (encoding_name_)
        ctor_dtor_conv_ = true;
    } else if (c == 'D' && peek(1) == 'C') { // structured binding: DC <source-name>+ E
      __p_ += 2;
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
      n = __text(s + "]");
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
      n = __text(static_cast<string&&>(s));
    }
    return n;
  }

  // <unnamed-type-name> ::= Ut [<number>] _ | Ul <lambda-sig> E [<number>] _
  const node* unnamed_type_name() {
    if (consume("Ut")) {
      std::size_t __v = 0;
      const bool has = __number(__v);
      if (!consume('_'))
        return nullptr;
      return __text("{unnamed type#" + std::to_string(has ? __v + 2 : 1) + "}");
    }
    if (consume("Ul")) {
      // Template parameter declarations of a generic lambda: Ty, Tn <type>, Tt ... E, Tp ...
      if (peek() == 'T' && (peek(1) == 'y' || peek(1) == 'n' || peek(1) == 't' || peek(1) == 'p'))
        return nullptr;
      node_list __params;
      param_scope* const saved_scope = __scope_;
      __scope_ = nullptr; // auto parameters: the lambda's own template parameters
      const bool ok = bare_function_type(__params);
      __scope_ = saved_scope;
      if (!ok || !consume('E'))
        return nullptr;
      std::size_t __v = 0;
      const bool has = __number(__v);
      if (!consume('_'))
        return nullptr;
      string s = "{lambda(";
      print_list(s, __params);
      s += ")#" + std::to_string(has ? __v + 2 : 1) + "}";
      return __text(static_cast<string&&>(s));
    }
    return nullptr;
  }

  const node* operator_name() {
    if (consume("cv")) { // conversion operator
      const bool __saved = encoding_name_;
      encoding_name_ = false;
      const node* t = type();
      encoding_name_ = __saved;
      if (!t)
        return nullptr;
      if (encoding_name_)
        ctor_dtor_conv_ = true;
      string s = "operator ";
      t->print(s);
      return __text(static_cast<string&&>(s));
    }
    if (consume("li")) { // literal operator
      const node* id = source_name();
      if (!id)
        return nullptr;
      string s = "operator\"\" ";
      id->print(s);
      return __text(static_cast<string&&>(s));
    }
    if (peek() == 'v' && peek(1) >= '0' && peek(1) <= '9') { // vendor extended operator
      __p_ += 2;
      const node* id = source_name();
      if (!id)
        return nullptr;
      string s = "operator ";
      id->print(s);
      return __text(static_cast<string&&>(s));
    }
    if (static_cast<std::size_t>(__end_ - __p_) < 2)
      return nullptr;
    const op_info* op = find_operator(__p_);
    if (!op)
      return nullptr;
    __p_ += 2;
    string s = "operator";
    if (op->name[0] >= 'a' && op->name[0] <= 'z')
      s += ' ';
    s += op->name;
    return __text(static_cast<string&&>(s));
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
        return __make<abbrev_node>(a.short_form, a.full_form, a.class_name);
    std::size_t __idx = 0;
    if (!consume('_')) {
      if (!seq_id(__idx) || !consume('_'))
        return nullptr;
      ++__idx;
    }
    return __idx < __subs_.size() ? __subs_[__idx] : nullptr;
  }

  // <template-param> ::= T_ | T <number> _
  const node* template_param() {
    if (!consume('T'))
      return nullptr;
    std::size_t __idx = 0;
    if (!consume('_')) {
      if (!__number(__idx) || !consume('_'))
        return nullptr;
      ++__idx;
    }
    return __make<param_node>(__scope_, __idx);
  }

  // <template-args> ::= I <template-arg>+ E
  bool template_args(node_list& __args) {
    if (!consume('I'))
      return false;
    const bool __saved = encoding_name_;
    encoding_name_ = false;
    while (!consume('E')) {
      if (__p_ == __end_)
        return false;
      if (consume('Q')) { // a requires-clause: not shown
        if (!expression())
          return false;
        continue;
      }
      const node* a = template_arg();
      if (!a)
        return false;
      __args.push_back(a);
    }
    encoding_name_ = __saved;
    return true;
  }

  const node* template_arg() {
    depth_guard __g(*this);
    if (!__g.ok)
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
        if (__p_ == __end_)
          return nullptr;
        const node* a = template_arg();
        if (!a)
          return nullptr;
        pack.push_back(a);
      }
      return __make<__list_node>(static_cast<node_list&&>(pack));
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
    return __text(static_cast<string&&>(s));
  }

  // <type>
  const node* type() {
    depth_guard __g(*this);
    if (!__g.ok)
      return nullptr;
    const bool __saved = encoding_name_;
    encoding_name_ = false;
    const node* t = type_inner();
    encoding_name_ = __saved;
    return t;
  }

  const node* __y_builtin(char c) {
    switch (c) {
    case 'v': return __text("void");
    case 'w': return __text("wchar_t");
    case 'b': return __text("bool");
    case 'c': return __text("char");
    case 'a': return __text("signed char");
    case 'h': return __text("unsigned char");
    case 's': return __text("short");
    case 't': return __text("unsigned short");
    case 'i': return __text("int");
    case 'j': return __text("unsigned int");
    case 'l': return __text("long");
    case 'm': return __text("unsigned long");
    case 'x': return __text("long long");
    case 'y': return __text("unsigned long long");
    case 'n': return __text("__int128");
    case 'o': return __text("unsigned __int128");
    case 'f': return __text("float");
    case 'd': return __text("double");
    case 'e': return __text("long double");
    case 'g': return __text("__float128");
    case 'z': return __text("...");
    default: return nullptr;
    }
  }

  const node* type_inner() {
    const char c = peek();
    if (const node* b = (c >= 'a' && c <= 'z' && c != 'u') ? __y_builtin(c) : nullptr) {
      ++__p_;
      return b;
    }
    const node* t = nullptr;
    switch (c) {
    case 'u': { // vendor extended type
      ++__p_;
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
        __p_ += 2;
        return __text(simple);
      }
      if (d == 'F') { // DF <number> _ (_FloatN), DF <number> x (_FloatNx), DF16b (bfloat16)
        __p_ += 2;
        if (consume("16b"))
          return __text("std::bfloat16_t");
        std::size_t n;
        if (!__number(n))
          return nullptr;
        if (consume('_'))
          return __text("_Float" + std::to_string(n));
        if (consume('x'))
          return __text("_Float" + std::to_string(n) + "x");
        return nullptr;
      }
      if (d == 'B' || d == 'U') { // _BitInt(N) / unsigned _BitInt(N)
        __p_ += 2;
        std::size_t n;
        if (!__number(n) || !consume('_'))
          return nullptr;
        return __text(string(d == 'U' ? "unsigned " : "") + "_BitInt(" + std::to_string(n) + ")");
      }
      if (d == 'p') { // pack expansion
        __p_ += 2;
        const node* __inner = type();
        if (!__inner)
          return nullptr;
        t = __make<pack_expansion_node>(__inner);
        break;
      }
      if (d == 't' || d == 'T') {
        t = decltype_node();
        break;
      }
      if (d == 'v') { // Dv <number> _ <type>: vector type
        __p_ += 2;
        std::size_t n;
        if (!__number(n) || !consume('_'))
          return nullptr;
        const node* e = type();
        if (!e)
          return nullptr;
        string s;
        e->print(s);
        s += " __vector(" + std::to_string(n) + ")";
        t = __text(static_cast<string&&>(s));
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
      string __q = cv_qualifiers();
      if (peek() == 'F' || (peek() == 'D' && (peek(1) == 'o' || peek(1) == 'O' || peek(1) == 'w' || peek(1) == 'x'))) {
        t = function_type(static_cast<string&&>(__q));
        break;
      }
      const node* __inner = type();
      if (!__inner)
        return nullptr;
      t = __make<qual_node>(__inner, static_cast<string&&>(__q));
      break;
    }
    case 'P':
    case 'R':
    case 'O': {
      ++__p_;
      const node* __inner = type();
      if (!__inner)
        return nullptr;
      t = __make<pointer_node>(__inner, c == 'P' ? "*" : c == 'R' ? "&" : "&&");
      break;
    }
    case 'C':
    case 'G': {
      ++__p_;
      const node* __inner = type();
      if (!__inner)
        return nullptr;
      string s;
      __inner->print(s);
      s += c == 'C' ? " _Complex" : " _Imaginary";
      t = __text(static_cast<string&&>(s));
      break;
    }
    case 'F':
      t = function_type(string());
      break;
    case 'A': {
      ++__p_;
      string dim;
      if (peek() >= '0' && peek() <= '9') {
        std::size_t n;
        if (!__number(n))
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
      const node* __elem = type();
      if (!__elem)
        return nullptr;
      t = __make<array_node>(__elem, static_cast<string&&>(dim));
      break;
    }
    case 'M': {
      ++__p_;
      const node* __cls = type();
      if (!__cls)
        return nullptr;
      const node* __mem = type();
      if (!__mem)
        return nullptr;
      t = __make<ptrmem_node>(__cls, __mem);
      break;
    }
    case 'T': {
      t = template_param();
      if (!t)
        return nullptr;
      __subs_.push_back(t);
      if (peek() == 'I') { // <template-template-param> <template-args>
        node_list __args;
        if (!template_args(__args))
          return nullptr;
        t = __make<template_node>(t, static_cast<node_list&&>(__args));
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
      node_list __args;
      if (!template_args(__args))
        return nullptr;
      t = __make<template_node>(s, static_cast<node_list&&>(__args));
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
      __subs_.push_back(t);
    return t;
  }

  // <function-type> ::= [<CV-qualifiers>] [<exception-spec>] [Dx] F [Y] <bare-function-type>
  //                     [<ref-qualifier>] E
  const node* function_type(string __quals) {
    if (consume("Do")) {
      __quals += " noexcept";
    } else if (consume("DO")) {
      const node* e = expression();
      if (!e || !consume('E'))
        return nullptr;
      __quals += " noexcept(";
      e->print(__quals);
      __quals += ')';
    } else if (consume("Dw")) {
      __quals += " throw(";
      bool first = true;
      while (!consume('E')) {
        const node* t = type();
        if (!t)
          return nullptr;
        if (!first)
          __quals += ", ";
        t->print(__quals);
        first = false;
      }
      __quals += ')';
    }
    consume("Dx");
    if (!consume('F'))
      return nullptr;
    consume('Y');
    const node* __ret = type();
    if (!__ret)
      return nullptr;
    node_list __params;
    if (consume('v')) {
      // no parameters
    }
    while (peek() != 'E' && !(peek() == 'R' && peek(1) == 'E') && !(peek() == 'O' && peek(1) == 'E')) {
      if (__p_ == __end_)
        return nullptr;
      const node* t = type();
      if (!t)
        return nullptr;
      __params.push_back(t);
    }
    string ref;
    if (consume('R'))
      ref = " &";
    else if (consume('O'))
      ref = " &&";
    if (!consume('E'))
      return nullptr;
    return __make<function_node>(__ret, static_cast<node_list&&>(__params), __quals + ref);
  }

  // <expr-primary> ::= L <type> <value number> E | L <type> <value float> E | L <mangled-name> E
  //                ::= L _Z <encoding> E | LDnE | LDn0E
  const node* expr_primary() {
    if (!consume('L'))
      return nullptr;
    if (consume("_Z")) {
      const bool __saved = encoding_name_;
      encoding_name_ = false;
      const node* e = encoding();
      encoding_name_ = __saved;
      return e && consume('E') ? e : nullptr;
    }
    if (consume("DnE") || consume("Dn0E"))
      return __text("nullptr");
    const char __tc = peek();
    const node* t = type();
    if (!t)
      return nullptr;
    string __v;
    if (consume('n'))
      __v = "-";
    const char* s = __p_;
    while (__p_ != __end_ && *__p_ != 'E')
      ++__p_;
    if (!consume('E'))
      return nullptr;
    __v.append(s, __p_ - 1);
    switch (__tc) {
    case 'b':
      return __text(__v == "0" ? "false" : __v == "1" ? "true" : "(bool)" + __v);
    case 'i': return __text(__v);
    case 'j': return __text(__v + "u");
    case 'l': return __text(__v + "l");
    case 'm': return __text(__v + "ul");
    case 'x': return __text(__v + "ll");
    case 'y': return __text(__v + "ull");
    default: {
      string __o = "(";
      t->print(__o);
      __o += ')';
      __o += __v;
      return __text(static_cast<string&&>(__o));
    }
    }
  }

  // <simple-id> ::= <source-name> [<template-args>] (also an operator name)
  const node* simple_id() {
    const node* n = peek() >= '1' && peek() <= '9' ? source_name() : (consume("on"), operator_name());
    if (!n)
      return nullptr;
    if (peek() == 'I') {
      node_list __args;
      if (!template_args(__args))
        return nullptr;
      n = __make<template_node>(n, static_cast<node_list&&>(__args));
    }
    return n;
  }

  // <expression>, common forms only.
  const node* expression() {
    depth_guard __g(*this);
    if (!__g.ok)
      return nullptr;
    const char c = peek();
    if (c == 'T')
      return template_param();
    if (c == 'L')
      return expr_primary();
    if (consume("fp") || consume("fL")) { // function parameter
      if (__p_[-1] == 'L') {
        std::size_t lvl;
        if (!__number(lvl) || !consume('p'))
          return nullptr;
      }
      cv_qualifiers();
      std::size_t n = 0;
      const bool has = __number(n);
      if (!consume('_'))
        return nullptr;
      return __text(has ? "fp" + std::to_string(n + 1) : string("fp"));
    }
    if (consume("sp")) {
      const node* e = expression();
      return e ? __make<pack_expansion_node>(e) : nullptr;
    }
    if (consume("st") || consume("at")) {
      const bool size = __p_[-2] == 's';
      const node* t = type();
      return t ? __make<prefix_node>(size ? "sizeof (" : "alignof (", t, ")") : nullptr;
    }
    if (consume("sz") || consume("az")) {
      const bool size = __p_[-2] == 's';
      const node* e = expression();
      return e ? __make<prefix_node>(size ? "sizeof (" : "alignof (", e, ")") : nullptr;
    }
    if (consume("sZ")) {
      const node* t = template_param();
      return t ? __make<prefix_node>("sizeof...(", t, ")") : nullptr;
    }
    if (consume("gs")) { // ::name
      const node* e = expression();
      return e ? __make<prefix_node>("::", e) : nullptr;
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
          const node* __q = simple_id();
          if (!__q)
            return nullptr;
          scope = scope ? __make<nested_node>(scope, __q) : __q;
        }
      }
      const node* n = simple_id();
      if (!n)
        return nullptr;
      return scope ? __make<nested_node>(scope, n) : n;
    }
    if (consume("cl")) {
      const node* callee = expression();
      if (!callee)
        return nullptr;
      node_list __args;
      while (!consume('E')) {
        if (__p_ == __end_)
          return nullptr;
        const node* a = expression();
        if (!a)
          return nullptr;
        __args.push_back(a);
      }
      string s;
      callee->print(s);
      s += '(';
      print_list(s, __args);
      s += ')';
      return __text(static_cast<string&&>(s));
    }
    if (consume("cv")) {
      const node* t = type();
      if (!t)
        return nullptr;
      node_list __args;
      if (consume('_')) {
        while (!consume('E')) {
          if (__p_ == __end_)
            return nullptr;
          const node* a = expression();
          if (!a)
            return nullptr;
          __args.push_back(a);
        }
      } else {
        const node* a = expression();
        if (!a)
          return nullptr;
        __args.push_back(a);
      }
      string s = "(";
      t->print(s);
      s += ")(";
      print_list(s, __args);
      s += ')';
      return __text(static_cast<string&&>(s));
    }
    if (consume("dt") || consume("pt")) {
      const bool __arrow = __p_[-2] == 'p';
      const node* __obj = expression();
      if (!__obj)
        return nullptr;
      const node* __member = peek() >= '1' && peek() <= '9' ? unqualified_name(nullptr) : expression();
      if (!__member)
        return nullptr;
      string s;
      __obj->print(s);
      s += __arrow ? "->" : ".";
      __member->print(s);
      return __text(static_cast<string&&>(s));
    }
    if (static_cast<std::size_t>(__end_ - __p_) >= 2) {
      if (const op_info* op = find_operator(__p_); op && op->arity > 0) {
        __p_ += 2;
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
        return __text(static_cast<string&&>(s));
      }
    }
    if (c >= '1' && c <= '9') // an unresolved name
      return simple_id();
    return nullptr;
  }
};

} // namespace

bool __ycxx::__detail::__demangle(const char* __mangled, std::string& out) {
  if (__mangled == nullptr || std::strncmp(__mangled, "_Z", 2) != 0)
    return false;
  parser __ps(__mangled, __mangled + std::strlen(__mangled));
  std::string s;
  if (!__ps.parse(s) || over(s))
    return false;
  out = static_cast<std::string&&>(s);
  return true;
}
