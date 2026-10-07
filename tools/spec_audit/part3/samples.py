"""Sample template arguments for the part-3 probes (tools/spec_audit/part3/gen.py).

A probe instantiates each class template and function template with samples: by the class's
name (CLASS), else by the template parameter's name in the subclause's area (BY_AREA, matched as
a prefix of the stable name), else by the name alone (BY_NAME). A parameter without a sample
makes the declaration presence-only.
"""
import re
import decls as D

# Whole sample argument lists for class templates whose parameters depend on each other.
CLASS = {
    "mersenne_twister_engine": ["uint_fast32_t", "32", "624", "397", "31", "0x9908b0df", "11", "0xffffffff", "7",
                                "0x9d2c5680", "15", "0xefc60000", "18", "1812433253"],
    "linear_congruential_engine": ["uint_fast32_t", "48271", "0", "2147483647"],
    "subtract_with_carry_engine": ["uint_fast32_t", "24", "10", "24"],
    "discard_block_engine": ["ranlux24_base", "223", "23"],
    "independent_bits_engine": ["mt19937", "16", "uint_fast32_t"],
    "shuffle_order_engine": ["minstd_rand0", "256"],
    "philox_engine": ["uint_fast32_t", "32", "4", "10", "0xD2511F53", "0x9E3779B9", "0xCD9E8D57", "0xBB67AE85"],
    "codecvt": ["wchar_t", "char", "mbstate_t"],
    "codecvt_byname": ["wchar_t", "char", "mbstate_t"],
    "counting_semaphore": ["4"],
    "shared_lock": ["shared_mutex"],
    "basic_mask": ["4", "std::simd::vec<float>::abi_type"],
    "basic_vec": ["float", "std::simd::vec<float>::abi_type"],
    "stop_callback": ["spec_probe::callback"],
    "inplace_stop_callback": ["spec_probe::callback"],
    "barrier": ["spec_probe::completion"],
    "time_point": ["chrono::system_clock", "chrono::seconds"],
    "zoned_time": ["chrono::seconds", "const chrono::time_zone*"],
    "hh_mm_ss": ["chrono::seconds"],
    "clock_time_conversion": ["chrono::system_clock", "chrono::utc_clock"],
    "scoped_lock": ["mutex"],
    "packaged_task": ["int(int)"],
    "rcu_obj_base": ["spec_probe::rcu_node", "default_delete<spec_probe::rcu_node>"],
    "hazard_pointer_obj_base": ["spec_probe::hp_node", "default_delete<spec_probe::hp_node>"],
    "scaled_accessor": ["double", "default_accessor<double>"],
    "conjugated_accessor": ["default_accessor<double>"],
    "layout_transpose": ["layout_right"],
    "layout_blas_packed": ["upper_triangle_t", "column_major_t"],
    "basic_format_context": ["char*", "char"],
    "format_to_n_result": ["char*"],
    "range_formatter": ["int", "char"],
    "basic_format_arg": ["format_context"],
    "basic_format_args": ["format_context"],
    "basic_format_string": ["char", "int"],
    "basic_format_parse_context": ["char"],
    "regex_iterator": ["const char*"],
    "regex_token_iterator": ["const char*"],
    "sub_match": ["const char*"],
    "match_results": ["const char*"],
    "sender_adaptor_closure": ["spec_probe::closure"],
    "time_get": ["char"], "time_put": ["char"], "num_get": ["char"], "num_put": ["char"],
    "money_get": ["char"], "money_put": ["char"],
    "moneypunct": ["char", "false"], "moneypunct_byname": ["char", "false"],
    "basic_istream_view": None,
    "with_awaitable_senders": ["spec_probe::promise"],
    "env": ["prop<get_allocator_t, allocator<int>>"],
    "prop": ["get_allocator_t", "allocator<int>"],
    "task": ["int", "spec_probe::task_env"],
    "fpos": ["mbstate_t"],
    "valarray": ["double"], "slice_array": ["double"], "gslice_array": ["double"], "mask_array": ["double"],
    "indirect_array": ["double"],
    "complex": ["double"],
    "atomic_ref": ["int"],
    "duration": ["long long", "ratio<1>"],
    "duration_values": ["long long"],
    "treat_as_floating_point": ["double"],
    "is_clock": ["chrono::system_clock"],
    "uniform_int_distribution": ["int"], "binomial_distribution": ["int"], "geometric_distribution": ["int"],
    "negative_binomial_distribution": ["int"], "poisson_distribution": ["int"], "discrete_distribution": ["int"],
    "simple_counting_scope": None,
    "basic_spanbuf": ["char", "char_traits<char>"],
    "basic_regex": ["char", "regex_traits<char>"],
}

# Samples by a template parameter's type-constraint (exposition-only concepts without italics).
CONSTRAINED = {
    "contiguous_iterator": "const float*", "sized_sentinel_for<I>": "const float*",
    "simd-integral": "std::simd::vec<unsigned>", "simd-complex": "std::simd::vec<complex<float>>",
    "math-floating-point": "std::simd::vec<float>", "simd-vec-type": "std::simd::vec<float>",
    "simd-mask-type": "std::simd::mask<float>", "simd-floating-point": "std::simd::vec<float>",
    "ranges::contiguous_range": "std::span<float, 4>", "same_as<bitset<size()>>": "bitset<4>", "signed_integral": "int", "unsigned_integral": "unsigned",
}

# Functions whose Constraints the area's samples do not meet: (subclause, name) -> samples.
FUNC = {}
for _f in ("byteswap", "bit_ceil", "bit_floor", "has_single_bit", "shl", "shr", "rotl", "rotr", "bit_width",
           "countl_zero", "countl_one", "countr_zero", "countr_one", "popcount", "bit_compress", "bit_expand",
           "bit_reverse", "bit_repeat"):
    FUNC[("simd.syn", _f)] = {"V": "std::simd::vec<unsigned>", "V0": "std::simd::vec<unsigned>",
                              "V1": "std::simd::vec<unsigned>", "S": "std::simd::vec<unsigned>"}
FUNC[("exec.domain.default", "apply_sender")] = {"Tag": "this_thread::sync_wait_t", "Args": "\x06"}
FUNC[("execution.syn", "apply_sender")] = {"Tag": "this_thread::sync_wait_t", "Args": "\x06"}
# constexpr functions whose sample arguments violate a precondition: (subclause, name).
NO_CONSTEXPR_PROBE = {
    # the probe's Tag (sync_wait_t) has a non-constexpr apply_sender: default_domain::apply_sender
    # is constexpr, the call it makes is not
    ("exec.domain.default", "apply_sender"), ("execution.syn", "apply_sender"),
}

# Declarations probed for presence only: the signature needs more than a sample can give.
PRESENCE_ONLY = {("simd.syn", "chunk"), ("simd.syn", "cat"), ("simd.mask.overview", "to_bitset")}

# A member whose Constraints the class's sample does not meet: the class arguments to use.
MEMBER_CLASS = {}
for _op in ("operator~", "operator%", "operator&", "operator|", "operator^", "operator<<", "operator>>",
            "operator%=", "operator&=", "operator|=", "operator^=", "operator<<=", "operator>>="):
    MEMBER_CLASS[("simd.overview", _op)] = ["int", "std::simd::vec<int>::abi_type"]
for _m in ("real", "imag"):
    MEMBER_CLASS[("simd.overview", _m)] = ["complex<float>", "std::simd::vec<complex<float>>::abi_type"]

# Samples by template-parameter name, per area (prefix of the stable name).
BY_AREA = [
    ("exec.env", {"Envs": "spec_probe::query_env"}),
    ("istream.syn", {"T": "spec_probe::streamable_ref", "Istream": "istream"}),
    ("ostream.syn", {"T": "spec_probe::streamable", "Ostream": "ostream"}),
    ("re", {"Allocator": "allocator<sub_match<const char*>>", "traits": "regex_traits<char>"}),
    ("thread.stoptoken.syn", {"T": "stop_token"}),
    ("depr.vector.bool", {"Allocator": "allocator<bool>"}),
    ("rand.util.seedseq", {"T": "int"}),
    ("saferecl.hp", {"T": "spec_probe::hp_node", "D": "default_delete<spec_probe::hp_node>"}),
    ("hazard", {"T": "spec_probe::hp_node", "D": "default_delete<spec_probe::hp_node>"}),
    ("ifstream", {"T": "filesystem::path"}), ("ofstream", {"T": "filesystem::path"}),
    ("fstream", {"T": "filesystem::path"}), ("filebuf", {"T": "filesystem::path"}),
    ("stringbuf", {"T": "std::string_view"}), ("istringstream", {"T": "std::string_view"}),
    ("ostringstream", {"T": "std::string_view"}), ("stringstream", {"T": "std::string_view"}),
    ("time.duration", {"Period2": "ratio<60>"}),
    ("time.point", {"Duration2": "chrono::minutes"}),
    ("time.zone.zonedtime", {"Duration2": "chrono::minutes"}),
    ("locale.ctype.general", {"charT": "wchar_t"}),
    ("thread.thread", {"T": "char"}),
    ("thread.jthread", {"T": "char"}),
    ("format.fmt.string", {"T": "std::string_view"}),
    ("depr.tuple", {"T": "tuple<int>"}),
    ("depr.variant", {"T": "variant<int>"}),
    ("depr.meta", {"Types": "int"}),
    ("atomics.ref", {"U": "=integral-type|floating-point-type|T"}),
    ("complex", {"T": "double", "X": "double"}),
    ("cmplx", {"T": "double", "X": "double"}),
    ("atomics", {"T": "int", "U": "int"}),
    ("depr.atomics", {"T": "int"}),
    ("stdatomic", {"T": "int"}),
    ("valarray", {"T": "double"}),
    ("numarray", {"T": "double"}),
    ("template.", {"T": "double"}),
    ("class.slice", {"T": "double"}),
    ("class.gslice", {"T": "double"}),
    ("gslice", {"T": "double"}),
    ("slice", {"T": "double"}),
    ("numbers", {"T": "double"}),
    ("future", {"R": "int", "Allocator": "allocator<int>", "F": "spec_probe::fn_ii", "T": "int"}),
    ("futures", {"R": "int", "Allocator": "allocator<int>", "F": "spec_probe::fn_ii", "T": "int"}),
    ("shared.lock", {"Mutex": "shared_mutex"}),
    ("thread.lock.shared", {"Mutex": "shared_mutex"}),
    ("thread.sharedtimedmutex", {"Mutex": "shared_mutex"}),
    ("rand", {"InputIterator": "const double*", "T": "double"}),
    ("re", {"InputIterator": "const char*"}),
    ("format", {"T": "int", "Out": "char*", "R": "std::span<int>"}),
    ("simd.mask", {"G": "spec_probe::mask_generator", "T": "unsigned"}),
    ("simd", {"T": "float", "V": "std::simd::vec<float>", "U": "float", "I": "std::simd::vec<int>", "R": "std::span<float, 4>",
              "S": "const float*", "M": "std::simd::mask<float>", "Abi": "std::simd::vec<float>::abi_type",
              "Bytes": "4", "N": "4", "UAbi": "std::simd::rebind_t<double, std::simd::mask<float>>::abi_type", "VX": "std::simd::vec<int>",
              "VS": "std::simd::vec<int>", "V0": "std::simd::vec<float>", "V1": "std::simd::vec<float>",
              "UBytes": "8",
              "IdxMap": "spec_probe::idxmap", "G": "spec_probe::generator", "BinaryOperation": "plus<>"}),
    ("linalg", {"InMat": "spec_probe::mat", "InMat1": "spec_probe::mat", "InMat2": "spec_probe::mat",
                "InMat3": "spec_probe::mat", "OutMat": "spec_probe::mat", "InOutMat": "spec_probe::mat",
                "InVec": "spec_probe::vec", "InVec1": "spec_probe::vec", "InVec2": "spec_probe::vec",
                "InVec3": "spec_probe::vec", "OutVec": "spec_probe::vec", "InOutVec": "spec_probe::vec",
                "InOutVec1": "spec_probe::vec", "InOutVec2": "spec_probe::vec",
                "InObj": "spec_probe::vec", "InObj1": "spec_probe::vec", "InObj2": "spec_probe::vec",
                "OutObj": "spec_probe::vec", "InOutObj": "spec_probe::vec", "InOutObj1": "spec_probe::vec",
                "InOutObj2": "spec_probe::vec", "Scalar": "double", "Real": "double", "ScalingFactor": "double",
                "Triangle": "linalg::upper_triangle_t", "DiagonalStorage": "linalg::explicit_diagonal_t",
                "ExecutionPolicy": "execution::sequenced_policy", "Extents": "dextents<size_t, 2>",
                "Layout": "layout_right", "NestedAccessor": "default_accessor<double>",
                "OtherNestedAccessor": "default_accessor<double>", "StorageOrder": "linalg::column_major_t",
                "ElementType": "double", "Accessor": "default_accessor<double>", "T": "double",
                "OtherExtents": "dextents<size_t, 2>", "BinaryDivideOp": "divides<>", "Index0": "size_t",
                "Index1": "size_t"}),
    ("exec", {"Sndr": "spec_probe::sndr", "Sender": "spec_probe::sndr", "Rcvr": "spec_probe::rcvr",
              "Env": "execution::env<>", "Sch": "execution::inline_scheduler", "Tag": "execution::set_value_t",
              "Token": "spec_probe::scope_token", "T": "int", "Promise": "spec_probe::promise",
              "QueryTag": "get_allocator_t", "E": "int", "Scope": "execution::counting_scope",
              "Data": "int", "Alloc": "allocator<int>", "Environment": "spec_probe::task_env", "V": "int"}),
    ("execution", {"Sndr": "spec_probe::sndr", "Sender": "spec_probe::sndr", "Rcvr": "spec_probe::rcvr",
                   "Env": "execution::env<>", "Sch": "execution::inline_scheduler", "Tag": "execution::set_value_t",
                   "Token": "spec_probe::scope_token", "T": "int", "Promise": "spec_probe::promise",
                   "QueryTag": "get_allocator_t", "E": "int", "CPO": "execution::set_value_t",
                   "ValueType": "int", "Domain": "execution::default_domain", "D": "spec_probe::derived_env"}),
    ("task", {"T": "int", "Environment": "spec_probe::task_env", "Alloc": "allocator<int>", "E": "int",
              "Sndr": "spec_probe::sndr", "Sender": "spec_probe::sndr", "Rcvr": "spec_probe::rcvr",
              "Env": "execution::env<>", "V": "int"}),
    ("saferecl", {"T": "spec_probe::rcu_node", "D": "default_delete<spec_probe::rcu_node>"}),
    ("rcu", {"T": "spec_probe::rcu_node", "D": "default_delete<spec_probe::rcu_node>"}),
    ("hazard", {"T": "spec_probe::hp_node", "D": "default_delete<spec_probe::hp_node>"}),
    ("thread.stoptoken", {"Token": "stop_token"}),
    ("stoptoken", {"Token": "stop_token"}),
    ("stopcallback", {"Token": "stop_token"}),
    ("time", {"T": "chrono::seconds", "TimeZonePtr": "const chrono::time_zone*",
              "TimeZonePtr2": "const chrono::time_zone*", "TimeZonePtrOrName": "const chrono::time_zone*"}),
    ("depr.iterator", {"Category": "input_iterator_tag", "T": "int", "Distance": "ptrdiff_t", "Pointer": "int*",
                       "Reference": "int&"}),
    ("depr.relops", {"T": "int"}),
    ("depr", {"T": "int", "Len": "8", "Align": "8", "Types": "int", "Category": "input_iterator_tag",
              "Distance": "ptrdiff_t", "Pointer": "int*", "Reference": "int&", "Iterator": "int*"}),
    ("fs", {"Source": "std::string", "InputIterator": "const char*", "EcharT": "char", "T": "int"}),
    ("locale", {"InputIterator": "istreambuf_iterator<char>", "OutputIterator": "ostreambuf_iterator<char>",
                "Facet": "ctype<char>", "internT": "wchar_t", "externT": "char", "stateT": "mbstate_t",
                "International": "false", "Intl": "false"}),
    ("category", {"InputIterator": "istreambuf_iterator<char>", "OutputIterator": "ostreambuf_iterator<char>",
                  "internT": "wchar_t", "externT": "char", "stateT": "mbstate_t"}),
    ("facet", {"InputIterator": "istreambuf_iterator<char>", "OutputIterator": "ostreambuf_iterator<char>",
               "internT": "wchar_t", "externT": "char", "stateT": "mbstate_t", "International": "false",
               "Intl": "false"}),
    ("iomanip", {"moneyT": "long double", "T": "int"}),
    ("ext.manip", {"moneyT": "long double", "T": "int"}),
]

BY_NAME = {
    "BiIter": "const char*", "FST": "char_traits<char>", "FSA": "allocator<char>",
    "Parsable": "chrono::sys_seconds", "Alloc": "allocator<char>",
    "charT": "char", "CharT": "char", "traits": "char_traits<char>", "Allocator": "allocator<char>",
    "SAlloc": "allocator<char>", "ST": "char_traits<char>", "SA": "allocator<char>",
    "T": "int", "U": "int", "R": "int",
    "RealType": "double", "IntType": "int", "UIntType": "uint_fast32_t",
    "URBG": "mt19937", "Engine": "mt19937", "Sseq": "seed_seq", "UnaryOperation": "spec_probe::fn_dd",
    "InputIteratorB": "const double*", "InputIteratorW": "const double*", "RandomAccessIterator": "unsigned*",
    "BidirectionalIterator": "const char*", "ForwardIterator": "const char*", "OutputIter": "char*",
    "OutputIterator": "char*", "InputIterator": "const char*",
    "Duration": "chrono::seconds", "Duration1": "chrono::seconds", "Duration2": "chrono::milliseconds",
    "ToDuration": "chrono::milliseconds", "Rep": "long long", "Rep1": "long long", "Rep2": "int",
    "Period": "ratio<1>", "Period1": "ratio<1>", "Period2": "milli", "Clock": "chrono::system_clock",
    "Clock1": "chrono::system_clock", "Clock2": "chrono::system_clock", "DestClock": "chrono::utc_clock",
    "SourceClock": "chrono::system_clock", "TimeZonePtr": "const chrono::time_zone*",
    "Mutex": "mutex", "Lock": "unique_lock<mutex>", "Predicate": "spec_probe::pred", "L1": "mutex", "L2": "mutex",
    "Callable": "spec_probe::fn", "Callback": "spec_probe::fn", "CallbackFn": "spec_probe::callback",
    "Initializer": "spec_probe::callback", "CompletionFunction": "spec_probe::completion",
    "Context": "format_context", "FormatContext": "format_context", "ParseContext": "format_parse_context",
    "Out": "char*", "Facet": "ctype<char>", "stateT": "mbstate_t", "state": "mbstate_t",
    "Istream": "istream", "Ostream": "ostream", "ROS": "std::string_view", "F": "spec_probe::fn",
    "moneyT": "long double", "Visitor": "spec_probe::visitor",
}
# Value parameters by name.
VALUES = {"I": "0", "N": "4", "w": "32", "International": "false", "Intl": "false", "least_max_value": "4",
          "Bytes": "4", "Len": "8", "Align": "8", "cnt": "4"}
# Packs: their sample (text of the arguments; "" for an empty pack).
PACKS = {"Args": "int", "ArgTypes": "int", "MutexTypes": "mutex", "L3": "mutex", "Flags": "", "Ts": "int",
         "Env": "", "Envs": "", "Domains": "", "Child": "", "T": "int", "Types": "int", "Abis": "",
         "Other": "", "consts": "", "Fns": ""}

PLACEHOLDERS = {
    "integer-type": ["char", "signed char", "unsigned char", "short", "unsigned short", "int", "unsigned int",
                     "long", "unsigned long", "long long", "unsigned long long"],
    "integral-type": ["char", "signed char", "unsigned char", "short", "unsigned short", "int", "unsigned int",
                      "long", "unsigned long", "long long", "unsigned long long", "char8_t", "char16_t",
                      "char32_t", "wchar_t"],
    "floating-point-type": ["float", "double", "long double"],
    "extended-floating-point-type": [],
    "pointer-type": ["int*"],
}


# Declarations the draft makes optional or implementation-defined: (subclause, name) -> why.
# Item declarations that no synopsis of these clauses repeats (the headers' synopses are in
# other clauses, or the declaration is only in its subclause): (subclause, header, namespace,
# entity, check, code). Found by comparing the item declarations with inventory.tsv.
_C = "template<class Z> concept c = requires {{ {} }}; static_assert(c<void>);"
_dv = "spec_probe::dv<spec_probe::dep<Z, {}>>()"
EXTRA = [
    ("c.mb.wcs", "cstdlib", "std", "std::mblen", "call ret",
     _C.format("{ std::mblen(%s, %s) } -> spec_probe::same<int>;" % (_dv.format("const char*"), _dv.format("size_t")))),
    ("c.mb.wcs", "cstdlib", "std", "std::mbtowc", "call ret",
     _C.format("{ std::mbtowc(%s, %s, %s) } -> spec_probe::same<int>;" % (_dv.format("wchar_t*"), _dv.format("const char*"), _dv.format("size_t")))),
    ("c.mb.wcs", "cstdlib", "std", "std::wctomb", "call ret",
     _C.format("{ std::wctomb(%s, %s) } -> spec_probe::same<int>;" % (_dv.format("char*"), _dv.format("wchar_t")))),
    ("c.mb.wcs", "cstdlib", "std", "std::mbstowcs", "call ret",
     _C.format("{ std::mbstowcs(%s, %s, %s) } -> spec_probe::same<size_t>;" % (_dv.format("wchar_t*"), _dv.format("const char*"), _dv.format("size_t")))),
    ("c.mb.wcs", "cstdlib", "std", "std::wcstombs", "call ret",
     _C.format("{ std::wcstombs(%s, %s, %s) } -> spec_probe::same<size_t>;" % (_dv.format("char*"), _dv.format("const wchar_t*"), _dv.format("size_t")))),
    ("c.math.rand", "cstdlib", "std", "std::rand", "call ret", _C.format("{ std::rand() } -> spec_probe::same<int>;")),
    ("c.math.rand", "cstdlib", "std", "std::srand", "call ret",
     _C.format("{ std::srand(%s) } -> spec_probe::same<void>;" % _dv.format("unsigned"))),
    ("numerics.c.ckdint", "stdckdint.h", "", "ckd_add", "call ret",
     _C.format("{ ckd_add(%s, %s, %s) } -> spec_probe::same<bool>;" % (_dv.format("int*"), _dv.format("long"), _dv.format("unsigned char")))),
    ("numerics.c.ckdint", "stdckdint.h", "", "ckd_sub", "call ret",
     _C.format("{ ckd_sub(%s, %s, %s) } -> spec_probe::same<bool>;" % (_dv.format("long long*"), _dv.format("int"), _dv.format("int")))),
    ("numerics.c.ckdint", "stdckdint.h", "", "ckd_mul", "call ret",
     _C.format("{ ckd_mul(%s, %s, %s) } -> spec_probe::same<bool>;" % (_dv.format("unsigned*"), _dv.format("int"), _dv.format("short")))),
    ("numerics.c.ckdint", "stdckdint.h", "", "ckd_add", "constexpr",
     "static_assert([] { int r = 0; return !ckd_add(&r, 2, 3) && r == 5 && ckd_add(&r, 2147483647, 1); }());"),
    ("time.clock.utc.nonmembers", "chrono", "std::chrono", "std::chrono::leap_second_info::is_leap_second", "var",
     "static_assert(spec_probe::same<decltype(std::chrono::leap_second_info::is_leap_second), bool>);"),
    ("time.clock.utc.nonmembers", "chrono", "std::chrono", "std::chrono::leap_second_info::elapsed", "var",
     "static_assert(spec_probe::same<decltype(std::chrono::leap_second_info::elapsed), std::chrono::seconds>);"),
    ("futures.task.members", "future", "std", "std::packaged_task::make_ready_at_thread_exit", "call ret",
     _C.format("{ %s.make_ready_at_thread_exit(%s) } -> spec_probe::same<void>;" % (_dv.format("std::packaged_task<int(int)>&"), _dv.format("int")))),
    ("depr.fs.path.factory", "filesystem", "std::filesystem", "std::filesystem::u8path", "presence",
     "using std::filesystem::u8path;"),
]

# Declarations whose sample cannot be right: (subclause, text in the declaration), why.
SKIP_DECLS = {
    ("re.syn", "match_results<typename basic_string<charT, ST, SA>::const_iterator"):
        "the string overloads of regex_match/regex_search take match_results<string::const_iterator, "
        "Allocator>; the area's Allocator sample is allocator<sub_match<const char*>>",
}

SKIP = {
    ("exec.snd.concepts", "catch"): "not a declaration (code of a consteval function body)",
    ("task.promise", "return_void"): "declared only when T is void ([task.promise]/1); the sample is task<int>",
    ("task.promise", "yield_value"): "the with_error argument needs an error type of the environment's error_types",
    # [thread.req.native]/1: the presence of native_handle_type and native_handle is
    # implementation-defined (STATUS: no native_handle for mutexes and condition variables)
    ("thread.mutex.class", "native_handle_type"): "", ("thread.mutex.class", "native_handle"): "",
    ("thread.mutex.recursive", "native_handle_type"): "", ("thread.mutex.recursive", "native_handle"): "",
    ("thread.timedmutex.class", "native_handle_type"): "", ("thread.timedmutex.class", "native_handle"): "",
    ("thread.timedmutex.recursive", "native_handle_type"): "", ("thread.timedmutex.recursive", "native_handle"): "",
    ("thread.sharedmutex.class", "native_handle_type"): "", ("thread.sharedmutex.class", "native_handle"): "",
    ("thread.sharedtimedmutex.class", "native_handle_type"): "", ("thread.sharedtimedmutex.class", "native_handle"): "",
    ("thread.condition.condvar", "native_handle_type"): "", ("thread.condition.condvar", "native_handle"): "",
    ("cmath.syn", "FP_FAST_FMA"): "C23 7.12/7: optionally defined (fma is fast)",
    ("cmath.syn", "FP_FAST_FMAF"): "C23 7.12/7: optionally defined",
    ("cmath.syn", "FP_FAST_FMAL"): "C23 7.12/7: optionally defined",
    ("cinttypes.syn", "abs"): "[cinttypes.syn]/2: declared only if intmax_t is an extended integer type",
    ("cinttypes.syn", "div"): "[cinttypes.syn]/2: declared only if intmax_t is an extended integer type",
}


def area_table(sec):
    out = {}
    for prefix, tab in BY_AREA:
        if sec == prefix or sec.startswith(prefix + ".") or sec.startswith(prefix):
            for k, v in tab.items():
                out.setdefault(k, v)
    return out


def sample_for(p, sec):
    if p.pack:
        tab = area_table(sec)
        if p.name in tab:
            return "\x06" + tab[p.name]
        if p.name in PACKS:
            return "\x06" + PACKS[p.name]
        return None
    if p.kind == "value":
        tab = area_table(sec)
        if p.name in tab:
            return tab[p.name]
        if p.name in VALUES:
            return VALUES[p.name]
        t = D.render(p.type) if p.type else ""
        if t in ("size_t", "int", "ptrdiff_t", "unsigned"):
            return "4"
        if t == "bool":
            return "false"
        if p.default:
            return None
        return None
    if p.kind == "template":
        return None
    if p.constraint:
        c = D.render(p.constraint).replace("⟨", "").replace("⟩", "")
        if c in CONSTRAINED:
            return CONSTRAINED[c]
    tab = area_table(sec)
    if p.name in tab:
        return tab[p.name]   # "=a|b" names other samples (resolved in params_env)
    if p.name in BY_NAME:
        return BY_NAME[p.name]
    return None


def params_env(params, decl, env):
    """[(env, label)] for one template head."""
    env = dict(env)
    cls_name = None
    # class-specific samples: the class whose own head this is
    if decl.kind in ("classdef",) or getattr(decl, "kind", None) == "class":
        cls_name = decl.name
    if cls_name and cls_name in CLASS:
        vals = CLASS[cls_name]
        if vals is None:
            return []
        vals = list(vals)
        for k, p in enumerate(params):
            v = vals[k] if k < len(vals) else None
            if p.pack:
                v = "\x06" + ", ".join(vals[k:])
            if p.name is None:
                continue
            if v is None:
                if p.default is not None:
                    try:
                        from gen import subst, Ctx
                        v = subst(p.default, Ctx(env))
                    except Exception:
                        return []
                elif p.pack:
                    v = ""
                else:
                    return []
            env[p.name] = v
        return [(env, "")]
    for p in params:
        if p.name is None:
            continue
        v = FUNC.get((decl.sec, decl.name), {}).get(p.name) or sample_for(p, decl.sec)
        if v is not None and v.startswith("="):
            v = next((env[k] for k in v[1:].split("|") if k in env), None)
        if v is None:
            if p.default is not None and not p.pack:
                try:
                    from gen import subst, Ctx
                    v = subst(p.default, Ctx(env))
                except Exception:
                    return []
            else:
                return []
        env[p.name] = v
    return [(env, "")]


def heads_env(heads, decl):
    return params_env(heads[-1], decl, {})


def arg_text(p, env):
    return env.get(p.name, "").replace("\x06", "")


def placeholder_envs(toks, env):
    names = sorted({D.italic_text(t) for t in toks if D.is_italic(t) and D.italic_text(t) in PLACEHOLDERS and D.italic_text(t) not in env})
    if not names:
        return [(env, "")]
    out = [(dict(env), "")]
    for nm in names:
        nout = []
        for e, lab in out:
            for v in PLACEHOLDERS[nm]:
                e2 = dict(e)
                e2[nm] = v
                nout.append((e2, (lab + " " + v).strip()))
        out = nout
    return out
