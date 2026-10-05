// Every enumerator of text_encoding::id (other than other and unknown), from [text.encoding.id].
// [text.encoding.members]/3-4: text_encoding(i) has mib() == i and a name contained in
// aliases(); /7-8: for a known registered encoding aliases() is non-empty, front() is the primary
// name, there are no duplicates (by strcmp), and each element is a non-empty ntbs of basic
// characters. [text.encoding.general]/4: no two aliases or primary names of distinct registered
// encodings are equivalent under comp-name, and /6: e.mib() == text_encoding(e.name()).mib().
// /1-2: text_encoding(enc) finds the encoding whose primary name or alias compares equal to enc
// under comp-name (which ignores case and non-alphanumeric characters), and name() is enc as
// given. So every alias, also with its letters' case flipped and punctuation added around it,
// maps back to its own enumerator. [text.encoding.aliases]: aliases_view is a copyable,
// borrowed, random-access view of const char*. [text.encoding.cmp]: == between text_encoding
// and id compares mib(). All of this is constexpr.
#include <text_encoding>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include "check.hpp"

using id = std::text_encoding::id;
using av = std::text_encoding::aliases_view;
static_assert(std::ranges::view<av> && std::ranges::random_access_range<av> && std::ranges::borrowed_range<av>);
static_assert(std::copyable<av>);
static_assert(std::is_same_v<std::ranges::range_value_t<av>, const char*>);
static_assert(std::is_same_v<std::ranges::range_reference_t<av>, const char*>);

constexpr id all_ids[] = {
    id::ASCII, id::ISOLatin1, id::ISOLatin2, id::ISOLatin3, id::ISOLatin4, id::ISOLatinCyrillic,
    id::ISOLatinArabic, id::ISOLatinGreek, id::ISOLatinHebrew, id::ISOLatin5, id::ISOLatin6,
    id::ISOTextComm, id::HalfWidthKatakana, id::JISEncoding, id::ShiftJIS, id::EUCPkdFmtJapanese,
    id::EUCFixWidJapanese, id::ISO4UnitedKingdom, id::ISO11SwedishForNames, id::ISO15Italian,
    id::ISO17Spanish, id::ISO21German, id::ISO60DanishNorwegian, id::ISO69French, id::ISO10646UTF1,
    id::ISO646basic1983, id::INVARIANT, id::ISO2IntlRefVersion, id::NATSSEFI, id::NATSSEFIADD,
    id::ISO10Swedish, id::KSC56011987, id::ISO2022KR, id::EUCKR, id::ISO2022JP, id::ISO2022JP2,
    id::ISO13JISC6220jp, id::ISO14JISC6220ro, id::ISO16Portuguese, id::ISO18Greek7Old,
    id::ISO19LatinGreek, id::ISO25French, id::ISO27LatinGreek1, id::ISO5427Cyrillic,
    id::ISO42JISC62261978, id::ISO47BSViewdata, id::ISO49INIS, id::ISO50INIS8,
    id::ISO51INISCyrillic, id::ISO54271981, id::ISO5428Greek, id::ISO57GB1988, id::ISO58GB231280,
    id::ISO61Norwegian2, id::ISO70VideotexSupp1, id::ISO84Portuguese2, id::ISO85Spanish2,
    id::ISO86Hungarian, id::ISO87JISX0208, id::ISO88Greek7, id::ISO89ASMO449, id::ISO90,
    id::ISO91JISC62291984a, id::ISO92JISC62991984b, id::ISO93JIS62291984badd,
    id::ISO94JIS62291984hand, id::ISO95JIS62291984handadd, id::ISO96JISC62291984kana, id::ISO2033,
    id::ISO99NAPLPS, id::ISO102T617bit, id::ISO103T618bit, id::ISO111ECMACyrillic,
    id::ISO121Canadian1, id::ISO122Canadian2, id::ISO123CSAZ24341985gr, id::ISO88596E,
    id::ISO88596I, id::ISO128T101G2, id::ISO88598E, id::ISO88598I, id::ISO139CSN369103,
    id::ISO141JUSIB1002, id::ISO143IECP271, id::ISO146Serbian, id::ISO147Macedonian, id::ISO150,
    id::ISO151Cuba, id::ISO6937Add, id::ISO153GOST1976874, id::ISO8859Supp, id::ISO10367Box,
    id::ISO158Lap, id::ISO159JISX02121990, id::ISO646Danish, id::USDK, id::DKUS, id::KSC5636,
    id::Unicode11UTF7, id::ISO2022CN, id::ISO2022CNEXT, id::UTF8, id::ISO885913, id::ISO885914,
    id::ISO885915, id::ISO885916, id::GBK, id::GB18030, id::OSDEBCDICDF0415, id::OSDEBCDICDF03IRV,
    id::OSDEBCDICDF041, id::ISO115481, id::KZ1048, id::UCS2, id::UCS4, id::UnicodeASCII,
    id::UnicodeLatin1, id::UnicodeJapanese, id::UnicodeIBM1261, id::UnicodeIBM1268,
    id::UnicodeIBM1276, id::UnicodeIBM1264, id::UnicodeIBM1265, id::Unicode11, id::SCSU, id::UTF7,
    id::UTF16BE, id::UTF16LE, id::UTF16, id::CESU8, id::UTF32, id::UTF32BE, id::UTF32LE, id::BOCU1,
    id::UTF7IMAP, id::Windows30Latin1, id::Windows31Latin1, id::Windows31Latin2,
    id::Windows31Latin5, id::HPRoman8, id::AdobeStandardEncoding, id::VenturaUS,
    id::VenturaInternational, id::DECMCS, id::PC850Multilingual, id::PCp852, id::PC8CodePage437,
    id::PC8DanishNorwegian, id::PC862LatinHebrew, id::PC8Turkish, id::IBMSymbols, id::IBMThai,
    id::HPLegal, id::HPPiFont, id::HPMath8, id::HPPSMath, id::HPDesktop, id::VenturaMath,
    id::MicrosoftPublishing, id::Windows31J, id::GB2312, id::Big5, id::Macintosh, id::IBM037,
    id::IBM038, id::IBM273, id::IBM274, id::IBM275, id::IBM277, id::IBM278, id::IBM280, id::IBM281,
    id::IBM284, id::IBM285, id::IBM290, id::IBM297, id::IBM420, id::IBM423, id::IBM424, id::IBM500,
    id::IBM851, id::IBM855, id::IBM857, id::IBM860, id::IBM861, id::IBM863, id::IBM864, id::IBM865,
    id::IBM868, id::IBM869, id::IBM870, id::IBM871, id::IBM880, id::IBM891, id::IBM903, id::IBM904,
    id::IBM905, id::IBM918, id::IBM1026, id::IBMEBCDICATDE, id::EBCDICATDEA, id::EBCDICCAFR,
    id::EBCDICDKNO, id::EBCDICDKNOA, id::EBCDICFISE, id::EBCDICFISEA, id::EBCDICFR, id::EBCDICIT,
    id::EBCDICPT, id::EBCDICES, id::EBCDICESA, id::EBCDICESS, id::EBCDICUK, id::EBCDICUS,
    id::Unknown8BiT, id::Mnemonic, id::Mnem, id::VISCII, id::VIQR, id::KOI8R, id::HZGB2312,
    id::IBM866, id::PC775Baltic, id::KOI8U, id::IBM00858, id::IBM00924, id::IBM01140, id::IBM01141,
    id::IBM01142, id::IBM01143, id::IBM01144, id::IBM01145, id::IBM01146, id::IBM01147,
    id::IBM01148, id::IBM01149, id::Big5HKSCS, id::IBM1047, id::PTCP154, id::Amiga1251,
    id::KOI7switched, id::BRF, id::TSCII, id::CP51932, id::windows874, id::windows1250,
    id::windows1251, id::windows1252, id::windows1253, id::windows1254, id::windows1255,
    id::windows1256, id::windows1257, id::windows1258, id::TIS620, id::CP50220,};
static_assert(std::size(all_ids) == 256);

bool basic_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
         std::strchr(" _{}[]#()<>%:;.?*+-/^&|~!=,\\\"'", c) != nullptr;
}

std::string flip_and_decorate(std::string_view a) {
  std::string s = "-";
  for (char c : a) {
    if (c >= 'a' && c <= 'z') s.push_back(static_cast<char>(c - 'a' + 'A'));
    else if (c >= 'A' && c <= 'Z') s.push_back(static_cast<char>(c - 'A' + 'a'));
    else s.push_back(c);
  }
  s += "._";
  return s;
}

constexpr bool constexpr_checks() {
  std::text_encoding e(id::UTF8);
  if (e.mib() != id::UTF8 || !(e == id::UTF8)) return false;
  std::text_encoding f("utf8");
  if (f.mib() != id::UTF8 || std::string_view(f.name()) != "utf8") return false;
  auto r = e.aliases();
  if (r.empty()) return false;
  std::text_encoding g(r.front());
  return g.mib() == id::UTF8;
}
static_assert(constexpr_checks());

const auto as_sv = [](const char* p) { return std::string_view(p); };

int main() {
  int total_aliases = 0;
  for (id i : all_ids) {
    std::text_encoding e(i);
    CHECK(e.mib() == i && e == i);
    const char* name = e.name();
    CHECK(name != nullptr && *name != '\0');
    CHECK(std::text_encoding(name).mib() == i);
    av r = e.aliases();
    CHECK(!r.empty());
    CHECK(std::ranges::contains(r, std::string_view(name), as_sv));
    const auto n = std::ranges::distance(r);
    auto it = r.begin();
    for (std::ptrdiff_t k = 0; k < n; ++k) {
      const char* a = r[k];
      CHECK(a == *(it + k) && a == *std::ranges::next(r.begin(), k));
      CHECK(a != nullptr && *a != '\0');
      for (const char* p = a; *p; ++p) CHECK(basic_char(*p));
      for (std::ptrdiff_t j = 0; j < k; ++j) CHECK(std::strcmp(a, r[j]) != 0);
      CHECK(std::strlen(a) <= std::text_encoding::max_name_length);
      std::text_encoding back(a);
      CHECK(back.mib() == i);
      CHECK(std::string_view(back.name()) == a);  // the name is kept as given
      std::string d = flip_and_decorate(a);
      if (d.size() <= std::text_encoding::max_name_length) {
        std::text_encoding fd(d);
        CHECK(fd.mib() == i);
        CHECK(std::string_view(fd.name()) == d);
      }
      ++total_aliases;
    }
    // copyable: a copy walks the same elements
    av copy = r;
    CHECK(std::ranges::equal(copy, r));
  }
  CHECK(total_aliases >= 256);
  // other / unknown: empty name and aliases
  for (id i : {id::other, id::unknown}) {
    std::text_encoding e(i);
    CHECK(e.mib() == i && *e.name() == '\0' && e.aliases().empty());
  }
  // US-ASCII includes "ASCII" ([text.encoding.general]/4)
  CHECK(std::ranges::contains(std::text_encoding(id::ASCII).aliases(), std::string_view("ASCII"), as_sv));
}
