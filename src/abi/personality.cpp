// libycxx ABI runtime: the C++ personality routine ([ABI-EH] 1.6, 2.5.2) and the language-
// specific data area (LSDA, `.gcc_except_table`) both compilers emit:
//
//   header:     lpstart encoding (u8) [lpstart, encoded]
//               ttype encoding (u8)   [uleb128 offset from here to the end of the type table]
//               call-site encoding (u8), uleb128 call-site table length
//   call sites: start, length, landing pad (call-site encoding; start/length relative to the
//               function start, the landing pad to lpstart), uleb128 action (0: none, else 1 +
//               offset into the action table)
//   actions:    sleb128 filter, sleb128 offset (from that field) to the next record, 0 = last;
//               filter > 0 selects type table entry -filter (counted back from its end; a null
//               entry is catch(...)), filter < 0 an exception specification at byte -filter-1
//               after the type table's end (uleb128 type indices, 0-terminated), 0 a cleanup.
//
// Pointer encodings are DWARF's DW_EH_PE values (LSB, "Exception Frames").
// An instruction address with no call-site entry may not throw: std::terminate (GCC encodes
// noexcept this way).
#include "eh.hpp"
#include "internal.hpp"

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __abi {
namespace {

enum : unsigned char {
  pe_absptr = 0x00,
  pe_uleb128 = 0x01,
  pe_udata2 = 0x02,
  pe_udata4 = 0x03,
  pe_udata8 = 0x04,
  pe_sleb128 = 0x09,
  pe_sdata2 = 0x0A,
  pe_sdata4 = 0x0B,
  pe_sdata8 = 0x0C,
  pe_pcrel = 0x10,
  pe_textrel = 0x20,
  pe_datarel = 0x30,
  pe_funcrel = 0x40,
  pe_aligned = 0x50,
  pe_indirect = 0x80,
  pe_omit = 0xFF,
};

std::uintptr_t read_uleb128(const unsigned char*& p) noexcept {
  std::uintptr_t result = 0;
  unsigned shift = 0;
  unsigned char byte;
  do {
    byte = *p++;
    result |= static_cast<std::uintptr_t>(byte & 0x7F) << shift;
    shift += 7;
  } while (byte & 0x80);
  return result;
}
std::intptr_t read_sleb128(const unsigned char*& p) noexcept {
  std::uintptr_t result = 0;
  unsigned shift = 0;
  unsigned char byte;
  do {
    byte = *p++;
    result |= static_cast<std::uintptr_t>(byte & 0x7F) << shift;
    shift += 7;
  } while (byte & 0x80);
  if (shift < 8 * sizeof(result) && (byte & 0x40))
    result |= ~std::uintptr_t(0) << shift;
  return static_cast<std::intptr_t>(result);
}

template <class _Tp>
_Tp read_raw(const unsigned char*& p) noexcept {
  _Tp __v;
  __builtin_memcpy(&__v, p, sizeof(_Tp));
  p += sizeof(_Tp);
  return __v;
}

std::size_t encoded_size(unsigned char __enc) noexcept {
  if (__enc == pe_omit)
    return 0;
  switch (__enc & 0x0F) {
  case pe_absptr:
    return sizeof(void*);
  case pe_udata2:
  case pe_sdata2:
    return 2;
  case pe_udata4:
  case pe_sdata4:
    return 4;
  case pe_udata8:
  case pe_sdata8:
    return 8;
  default:
    std::terminate(); // a variable-length encoding cannot index the type table
  }
}

struct __y_bases {
  _Unwind_Context* __ctx = nullptr;
  std::uintptr_t __func = 0;
};

// The bases of DW_EH_PE_textrel and DW_EH_PE_datarel, asked of the unwinder only when an encoding
// needs them. Darwin's unwinder has none (Apple's <unwind.h> marks _Unwind_GetTextRelBase and
// _Unwind_GetDataRelBase unavailable, and its libunwind aborts in them), and neither compiler
// emits those encodings there (nor on the ELF targets libycxx supports). Templates, so that the
// call is dependent and is not even looked at where the branch is discarded.
template <class _Context>
std::uintptr_t text_rel_base(_Context* __ctx) noexcept {
  if constexpr (!__ycxx::__detail::__cfg::__darwin) {
    return _Unwind_GetTextRelBase(__ctx);
  } else {
    (void)__ctx;
    std::terminate();
  }
}
template <class _Context>
std::uintptr_t data_rel_base(_Context* __ctx) noexcept {
  if constexpr (!__ycxx::__detail::__cfg::__darwin) {
    return _Unwind_GetDataRelBase(__ctx);
  } else {
    (void)__ctx;
    std::terminate();
  }
}

std::uintptr_t read_encoded(const unsigned char*& p, unsigned char __enc, const __y_bases& b) noexcept {
  if (__enc == pe_omit)
    return 0;
  const unsigned char* start = p;
  std::uintptr_t __v;
  switch (__enc & 0x0F) {
  case pe_absptr:
    __v = read_raw<std::uintptr_t>(p);
    break;
  case pe_uleb128:
    __v = read_uleb128(p);
    break;
  case pe_sleb128:
    __v = static_cast<std::uintptr_t>(read_sleb128(p));
    break;
  case pe_udata2:
    __v = read_raw<std::uint16_t>(p);
    break;
  case pe_udata4:
    __v = read_raw<std::uint32_t>(p);
    break;
  case pe_udata8:
    __v = static_cast<std::uintptr_t>(read_raw<std::uint64_t>(p));
    break;
  case pe_sdata2:
    __v = static_cast<std::uintptr_t>(read_raw<std::int16_t>(p));
    break;
  case pe_sdata4:
    __v = static_cast<std::uintptr_t>(read_raw<std::int32_t>(p));
    break;
  case pe_sdata8:
    __v = static_cast<std::uintptr_t>(read_raw<std::int64_t>(p));
    break;
  default:
    std::terminate();
  }
  if (__v != 0) {
    switch (__enc & 0x70) {
    case pe_absptr:
      break;
    case pe_pcrel:
      __v += reinterpret_cast<std::uintptr_t>(start);
      break;
    case pe_textrel:
      __v += __ycxx::__abi::text_rel_base(b.__ctx);
      break;
    case pe_datarel:
      __v += __ycxx::__abi::data_rel_base(b.__ctx);
      break;
    case pe_funcrel:
      __v += b.__func;
      break;
    default:
      std::terminate();
    }
    if (__enc & pe_indirect)
      __v = *reinterpret_cast<const std::uintptr_t*>(__v);
  }
  return __v;
}

struct lsda_header {
  std::uintptr_t lpstart;
  unsigned char ttype_enc;
  const unsigned char* ttype_end; // end of the type table (entries are indexed backwards)
  unsigned char cs_enc;
  const unsigned char* cs_begin;
  const unsigned char* cs_end; // also the start of the action table
};

const unsigned char* parse_header(const unsigned char* p, const __y_bases& b, lsda_header& h) noexcept {
  const unsigned char lp_enc = *p++;
  h.lpstart = lp_enc == pe_omit ? b.__func : read_encoded(p, lp_enc, b);
  h.ttype_enc = *p++;
  h.ttype_end = nullptr;
  if (h.ttype_enc != pe_omit) {
    const std::uintptr_t __off = read_uleb128(p);
    h.ttype_end = p + __off;
  }
  h.cs_enc = *p++;
  const std::uintptr_t __len = read_uleb128(p);
  h.cs_begin = p;
  h.cs_end = p + __len;
  return p;
}

const std::type_info* type_entry(const lsda_header& h, std::intptr_t index, const __y_bases& b) noexcept {
  const unsigned char* p = h.ttype_end - index * static_cast<std::intptr_t>(encoded_size(h.ttype_enc));
  return reinterpret_cast<const std::type_info*>(read_encoded(p, h.ttype_enc, b));
}

// An exception specification (filter < 0) is violated when no listed type matches.
bool spec_violated(const lsda_header& h, std::intptr_t filter, const std::type_info* __thrown, void* __obj,
                   const __y_bases& b) noexcept {
  const unsigned char* p = h.ttype_end + (-filter - 1);
  while (const std::uintptr_t index = read_uleb128(p)) {
    if (!__thrown)
      continue; // a foreign exception matches no listed type
    void* adjusted = __obj;
    if (__catch_matches(type_entry(h, static_cast<std::intptr_t>(index), b), __thrown, &adjusted))
      return false;
  }
  return true;
}

enum class found { nothing, cleanup, __handler, terminate };

struct scan_result {
  found kind = found::nothing;
  std::uintptr_t landing_pad = 0;
  std::intptr_t switch_value = 0;
  const unsigned char* __action_record = nullptr;
  void* adjusted = nullptr;
};

scan_result scan(_Unwind_Action actions, bool native, _Unwind_Exception* __ue, _Unwind_Context* __ctx) noexcept {
  scan_result r;
  const unsigned char* __lsda = __ycxx::__abi::__lsda_of(__ctx);
  if (!__lsda)
    return r;
  __y_bases b;
  b.__ctx = __ctx;
  b.__func = _Unwind_GetRegionStart(__ctx);
  int before = 0;
  std::uintptr_t __ip = _Unwind_GetIPInfo(__ctx, &before);
  if (!before)
    --__ip; // the return address may be the first instruction of the next region

  lsda_header h;
  parse_header(__lsda, b, h);

  const std::type_info* __thrown = nullptr;
  void* __obj = nullptr;
  if (native) {
    __exception_header* eh = __header_of_unwind(__ue);
    __thrown = eh->__exception_type;
    __obj = object_of(eh);
  }

  const unsigned char* p = h.cs_begin;
  while (p < h.cs_end) {
    const std::uintptr_t start = read_encoded(p, h.cs_enc, b);
    const std::uintptr_t length = read_encoded(p, h.cs_enc, b);
    const std::uintptr_t __pad = read_encoded(p, h.cs_enc, b);
    const std::uintptr_t action = read_uleb128(p);
    if (__ip < b.__func + start)
      break; // the table is sorted: no entry covers ip
    if (__ip >= b.__func + start + length)
      continue;
    if (__pad == 0)
      return r; // no landing pad: nothing to do in this frame
    r.landing_pad = h.lpstart + __pad;
    if (action == 0) {
      r.kind = found::cleanup;
      return r;
    }
    const unsigned char* rec = h.cs_end + (action - 1);
    bool has_cleanup = false;
    for (;;) {
      const unsigned char* this_rec = rec;
      const std::intptr_t filter = read_sleb128(rec);
      const unsigned char* next_field = rec;
      const std::intptr_t next = read_sleb128(rec);
      if (filter > 0) {
        const std::type_info* t = type_entry(h, filter, b);
        void* adjusted = __obj;
        // A forced unwind (thread cancellation, pthread_exit) runs cleanups but no handler of
        // its own choosing. Clang folds a frame's cleanups into its catch(...) landing pad and
        // records no separate cleanup action, so catch(...) counts as a cleanup: the landing
        // pad runs (entering catch(...), which must rethrow, as with GCC's code).
        if (t == nullptr && (actions & _UA_FORCE_UNWIND))
          has_cleanup = true;
        if (!(actions & _UA_FORCE_UNWIND) &&
            (t == nullptr || (native && __catch_matches(t, __thrown, &adjusted)))) {
          r.kind = found::__handler;
          r.switch_value = filter;
          r.__action_record = this_rec;
          r.adjusted = t == nullptr ? __obj : adjusted;
          return r;
        }
      } else if (filter < 0) {
        if (!(actions & _UA_FORCE_UNWIND) && spec_violated(h, filter, __thrown, __obj, b)) {
          r.kind = found::__handler;
          r.switch_value = filter;
          r.__action_record = this_rec;
          r.adjusted = __obj;
          return r;
        }
      } else {
        has_cleanup = true;
      }
      if (next == 0)
        break;
      rec = next_field + next;
    }
    r.kind = has_cleanup ? found::cleanup : found::nothing;
    return r;
  }
  // No entry covers ip: the region may not throw.
  r.kind = found::terminate;
  return r;
}

_Unwind_Reason_Code install(_Unwind_Context* __ctx, _Unwind_Exception* __ue, std::intptr_t switch_value,
                            std::uintptr_t landing_pad) noexcept {
  _Unwind_SetGR(__ctx, __builtin_eh_return_data_regno(0), reinterpret_cast<std::uintptr_t>(__ue));
  _Unwind_SetGR(__ctx, __builtin_eh_return_data_regno(1), static_cast<std::uintptr_t>(switch_value));
  _Unwind_SetIP(__ctx, landing_pad);
  return _URC_INSTALL_CONTEXT;
}

} // namespace
}} // namespace __ycxx::__abi

using namespace __ycxx::__abi;

// (__gxx_personality_v0 itself is src/abi/entry/cxa_personality.cpp's forwarder, DECISIONS §20.6.)
extern "C" _Unwind_Reason_Code __ycxx_abi_personality(int version, _Unwind_Action actions, std::uint64_t __cls,
                                                      _Unwind_Exception* __ue, _Unwind_Context* __ctx) {
  if (version != 1 || !__ue || !__ctx)
    return _URC_FATAL_PHASE1_ERROR;
  const bool native = __is_native(__cls);

  // Phase 2 in the frame phase 1 chose: use what phase 1 cached ([ABI-EH] 2.2.1).
  if (actions == (_UA_CLEANUP_PHASE | _UA_HANDLER_FRAME) && native) {
    __exception_header* h = __header_of_unwind(__ue);
    return install(__ctx, __ue, h->__handler_switch_value, reinterpret_cast<std::uintptr_t>(h->__catch_temp));
  }

  const scan_result r = scan(actions, native, __ue, __ctx);
  if (actions & _UA_SEARCH_PHASE) {
    switch (r.kind) {
    case found::nothing:
    case found::cleanup:
      return _URC_CONTINUE_UNWIND;
    case found::terminate:
      __terminate_for(__ue);
    case found::__handler:
      if (native) {
        __exception_header* h = __header_of_unwind(__ue);
        h->__handler_switch_value = static_cast<int>(r.switch_value);
        h->__action_record = r.__action_record;
        h->__lsda = __ycxx::__abi::__lsda_of(__ctx);
        h->__catch_temp = reinterpret_cast<void*>(r.landing_pad);
        h->__adjusted_ptr = r.adjusted;
      }
      return _URC_HANDLER_FOUND;
    }
  }

  // Phase 2.
  switch (r.kind) {
  case found::nothing:
    return _URC_CONTINUE_UNWIND;
  case found::terminate:
    __terminate_for(__ue);
  case found::cleanup:
    return install(__ctx, __ue, 0, r.landing_pad);
  case found::__handler:
    if (actions & _UA_HANDLER_FRAME)
      return install(__ctx, __ue, r.switch_value, r.landing_pad); // a foreign exception's handler
    // A catch here that phase 1 did not choose: run the frame's cleanups only. Selector 0
    // matches no catch clause, so the landing pad resumes unwinding after them.
    return install(__ctx, __ue, 0, r.landing_pad);
  }
  return _URC_FATAL_PHASE2_ERROR;
}
