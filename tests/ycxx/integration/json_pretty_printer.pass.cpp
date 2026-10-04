// Whole-program integration: a JSON-like document model, parser and pretty printer.
//   Model: a recursive variant ([variant]) whose alternatives include vector<Value> (vector of
//   an incomplete type: [vector.overview]/4) and map<string, indirect<Value>> (indirect<T>
//   with T incomplete: [indirect.general]/2), compared with defaulted == ([variant.relops],
//   [associative.reqmts] via [container.reqmts] ==, [indirect.relops]).
//   Parser: string_view ([string.view]), from_chars for numbers ([charconv.from.chars]),
//   escape handling. Printer: visit with an overload set ([variant.visit]); a program-defined
//   formatter<Value> ([formatter.requirements]); strings with the debug format "{:?}"
//   ([format.string.escaped]: quote, Table 114 escapes \t \n \" \\, other characters of
//   General_Category C as \u{hex}); doubles with "{}" ([format.string.std] Table 105: none =
//   to_chars shortest: 26.0 -> "26", 3.25 -> "3.25", 1e+100 -> "1e+100"); nested ranges
//   ([format.range.formatter], [format.range.fmtmap]).
//   Round trip: parse(pretty(parse(s))) == parse(s).
#include <charconv>
#include <cstddef>
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "check.hpp"

struct Value;
using Array = std::vector<Value>;
using Object = std::map<std::string, std::indirect<Value>, std::less<>>;
struct Value {
  std::variant<std::nullptr_t, bool, double, std::string, Array, Object> v;
  friend bool operator==(const Value&, const Value&) = default;
};

template <class... F>
struct overloaded : F... {
  using F::operator()...;
};

class Parser {
public:
  explicit Parser(std::string_view s) : s_(s) {}
  std::optional<Value> document() {
    auto v = value();
    ws();
    if (!v || !s_.empty()) return std::nullopt;
    return v;
  }

private:
  std::string_view s_;
  void ws() {
    while (!s_.empty() && (s_[0] == ' ' || s_[0] == '\n' || s_[0] == '\t' || s_[0] == '\r')) s_.remove_prefix(1);
  }
  bool eat(char c) {
    ws();
    if (s_.starts_with(c)) {
      s_.remove_prefix(1);
      return true;
    }
    return false;
  }
  std::optional<std::string> string() {
    if (!eat('"')) return std::nullopt;
    std::string r;
    while (!s_.empty() && s_[0] != '"') {
      char c = s_[0];
      s_.remove_prefix(1);
      if (c != '\\') {
        r += c;
        continue;
      }
      if (s_.empty()) return std::nullopt;
      char e = s_[0];
      s_.remove_prefix(1);
      switch (e) {
        case 'n': r += '\n'; break;
        case 't': r += '\t'; break;
        case '"': r += '"'; break;
        case '\\': r += '\\'; break;
        case 'u': {  // \uXXXX (JSON) or \u{h...} (format's debug output); ASCII only here
          bool brace = s_.starts_with('{');
          if (brace) s_.remove_prefix(1);
          unsigned cp = 0;
          auto [p, ec] = std::from_chars(s_.data(), s_.data() + (brace ? s_.size() : std::min<std::size_t>(4, s_.size())), cp, 16);
          if (ec != std::errc() || cp > 0x7f) return std::nullopt;
          s_.remove_prefix(static_cast<std::size_t>(p - s_.data()));
          if (brace && !eat('}')) return std::nullopt;
          r += static_cast<char>(cp);
          break;
        }
        default: return std::nullopt;
      }
    }
    if (!eat('"')) return std::nullopt;
    return r;
  }
  std::optional<Value> value() {
    ws();
    if (s_.starts_with("null")) return s_.remove_prefix(4), Value{nullptr};
    if (s_.starts_with("true")) return s_.remove_prefix(4), Value{true};
    if (s_.starts_with("false")) return s_.remove_prefix(5), Value{false};
    if (s_.starts_with('"')) {
      auto s = string();
      if (!s) return std::nullopt;
      return Value{std::move(*s)};
    }
    if (eat('[')) {
      Array a;
      if (eat(']')) return Value{std::move(a)};
      do {
        auto e = value();
        if (!e) return std::nullopt;
        a.push_back(std::move(*e));
      } while (eat(','));
      if (!eat(']')) return std::nullopt;
      return Value{std::move(a)};
    }
    if (eat('{')) {
      Object o;
      if (eat('}')) return Value{std::move(o)};
      do {
        auto k = string();
        if (!k || !eat(':')) return std::nullopt;
        auto e = value();
        if (!e) return std::nullopt;
        o.insert_or_assign(std::move(*k), std::indirect<Value>(std::move(*e)));
      } while (eat(','));
      if (!eat('}')) return std::nullopt;
      return Value{std::move(o)};
    }
    double d = 0;
    auto [p, ec] = std::from_chars(s_.data(), s_.data() + s_.size(), d);
    if (ec != std::errc()) return std::nullopt;
    s_.remove_prefix(static_cast<std::size_t>(p - s_.data()));
    return Value{d};
  }
};

// Compact form through a formatter specialization: "{}".
template <>
struct std::formatter<Value> {
  constexpr auto parse(std::format_parse_context& pc) { return pc.begin(); }
  // The return type is spelled out: formattable<Value> ([format.formattable]) is needed while
  // formatting a vector<Value>, inside this very function.
  template <class FC>
  typename FC::iterator format(const Value& val, FC& fc) const {
    return std::visit(overloaded{
                          [&](std::nullptr_t) { return std::format_to(fc.out(), "null"); },
                          [&](bool b) { return std::format_to(fc.out(), "{}", b); },
                          [&](double d) { return std::format_to(fc.out(), "{}", d); },
                          [&](const std::string& s) { return std::format_to(fc.out(), "{:?}", s); },
                          [&](const Array& a) { return std::format_to(fc.out(), "{}", a); },
                          [&](const Object& o) {
                            auto out = std::format_to(fc.out(), "{{");
                            bool first = true;
                            for (const auto& [k, e] : o) {
                              out = std::format_to(out, "{}{:?}:{}", first ? "" : ",", k, *e);
                              first = false;
                            }
                            return std::format_to(out, "}}");
                          },
                      },
                      val.v);
  }
};

static void pretty(std::string& out, const Value& val, int indent) {
  std::visit(overloaded{
                 [&](const Array& a) {
                   if (a.empty()) return void(out += "[]");
                   out += "[\n";
                   for (std::size_t i = 0; i < a.size(); ++i) {
                     out.append(static_cast<std::size_t>(indent + 2), ' ');
                     pretty(out, a[i], indent + 2);
                     out += i + 1 < a.size() ? ",\n" : "\n";
                   }
                   std::format_to(std::back_inserter(out), "{}]", std::string(static_cast<std::size_t>(indent), ' '));
                 },
                 [&](const Object& o) {
                   if (o.empty()) return void(out += "{}");
                   out += "{\n";
                   std::size_t i = 0;
                   for (const auto& [k, e] : o) {
                     std::format_to(std::back_inserter(out), "{}{:?}: ", std::string(static_cast<std::size_t>(indent + 2), ' '), k);
                     pretty(out, *e, indent + 2);
                     out += ++i < o.size() ? ",\n" : "\n";
                   }
                   std::format_to(std::back_inserter(out), "{}}}", std::string(static_cast<std::size_t>(indent), ' '));
                 },
                 [&](const auto&) { std::format_to(std::back_inserter(out), "{}", val); },
             },
             val.v);
}

static int depth(const Value& v) {
  return std::visit(overloaded{
                        [](const Array& a) {
                          int d = 0;
                          for (auto& e : a) d = std::max(d, depth(e));
                          return d + 1;
                        },
                        [](const Object& o) {
                          int d = 0;
                          for (auto& [k, e] : o) d = std::max(d, depth(*e));
                          return d + 1;
                        },
                        [](const auto&) { return 0; },
                    },
                    v.v);
}

int main() {
  constexpr std::string_view input = R"({"name": "ycxx", "tags": ["c++", "std", [1, 2.5, -0.125]],
     "version": 26, "big": 1e100, "ok": true, "no": false, "none": null,
     "text": "say \"hi\"\n\tand \\ \u0001",
     "nested": {"a": [], "b": {}, "c": [{"x": null}]}, "version": 2026})";
  auto doc = Parser(input).document();
  CHECK(doc.has_value());
  CHECK(std::holds_alternative<Object>(doc->v));
  const Object& root = std::get<Object>(doc->v);
  CHECK(root.size() == 9);
  CHECK(std::get<double>(root.find("version")->second->v) == 2026);  // insert_or_assign kept the last
  CHECK(depth(*doc) == 4);

  std::string compact = std::format("{}", *doc);
  CHECK(compact ==
        R"({"big":1e+100,"name":"ycxx","nested":{"a":[],"b":{},"c":[{"x":null}]},"no":false,"none":null,"ok":true,)"
        R"("tags":["c++", "std", [1, 2.5, -0.125]],"text":"say \"hi\"\n\tand \\ \u{1}","version":2026})");

  std::string p;
  pretty(p, *doc, 0);
  constexpr std::string_view want = R"({
  "big": 1e+100,
  "name": "ycxx",
  "nested": {
    "a": [],
    "b": {},
    "c": [
      {
        "x": null
      }
    ]
  },
  "no": false,
  "none": null,
  "ok": true,
  "tags": [
    "c++",
    "std",
    [
      1,
      2.5,
      -0.125
    ]
  ],
  "text": "say \"hi\"\n\tand \\ \u{1}",
  "version": 2026
})";
  CHECK(p == want);

  auto again = Parser(p).document();
  CHECK(again.has_value() && *again == *doc);
  auto again2 = Parser(compact).document();
  CHECK(again2.has_value() && *again2 == *doc);

  // A deep copy (indirect copies its owned object: [indirect.ctor]) is equal and independent.
  Value copy = *doc;
  CHECK(copy == *doc);
  std::get<Object>(copy.v).at("nested")->v = Value{std::string("changed")}.v;
  CHECK(copy != *doc);
  CHECK(std::format("{}", *std::get<Object>(doc->v).at("nested")) == R"({"a":[],"b":{},"c":[{"x":null}]})");

  CHECK(!Parser("[1, 2").document());
  CHECK(!Parser("{\"a\" 1}").document());
  CHECK(!Parser("[1] x").document());
  return 0;
}
