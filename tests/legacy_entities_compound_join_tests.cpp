#include "dx_data.h"
#include "dx_iface.h"
#include "dx_legacy_attribute_compound_join.h"
#include "test_scratch.h"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
static long failAfter = -1;
void *operator new(std::size_t n) {
  if (failAfter == 0)
    throw std::bad_alloc();
  if (failAfter > 0)
    --failAfter;
  if (auto p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
struct Tag {
  int code;
  std::string value;
};
using Tags = std::vector<Tag>;
class DxIfaceTableSourceTestAccess {
public:
  static std::pair<std::size_t, std::size_t> usage(const dx_data &d) {
    return {d.m_tableDxfSource.legacyEntitiesCompoundBytes,
            d.m_tableDxfSource.legacyEntitiesCompoundWork};
  }
  static void limit(dx_data &d, unsigned i, std::size_t n) {
    if (i == 0)
      d.m_tableDxfSource.maxLegacyEntitiesCompoundBytes = n;
    else
      d.m_tableDxfSource.maxLegacyEntitiesCompoundWork = n;
  }
  static bool failed(const dx_data &d) {
    return d.m_tableDxfSource.legacyEntitiesCompoundFailed;
  }
  static std::size_t count(const dx_data &d) {
    return d.m_tableDxfSource.legacyEntitiesCompoundJoins.size();
  }
  static bool allocations(dx_iface &r, dx_data &d, unsigned &faults) {
    auto &s = d.m_tableDxfSource;
    bool calibrated = false;
    for (long n = 0; n < 100 && !calibrated; ++n) {
      s.legacyEntitiesCompoundJoins.clear();
      s.legacyEntitiesCompoundFailed = false;
      failAfter = n;
      r.finishLegacyEntitiesCompoundJoins(d);
      failAfter = -1;
      if (s.legacyEntitiesCompoundFailed) {
        ++faults;
        if (!s.legacyEntitiesCompoundJoins.empty() ||
            s.legacyEntitiesCompoundBytes || s.legacyEntitiesCompoundWork)
          return false;
        s.legacyEntitiesCompoundFailed = false;
        r.finishLegacyEntitiesCompoundJoins(d);
        if (s.legacyEntitiesCompoundFailed ||
            s.legacyEntitiesCompoundJoins.size() != 1u)
          return false;
      } else
        calibrated = s.legacyEntitiesCompoundJoins.size() == 1u;
    }
    return calibrated;
  }
  static bool alteredReplay(dx_iface &r, dx_data &d) {
    auto &s = d.m_tableDxfSource;
    auto &audit =
        const_cast<dx_data::TableDxfSourceCapture::EntitiesCompoundAudit &>(
            *s.entitiesCompoundAudits[0]);
    auto &proof = audit.proofs[0];
    auto &observed = const_cast<
        std::vector<dx_data::TableDxfSourceCapture::EntitiesObservedValue> &>(
        *audit.observed)[proof.event];
    for (int fault = 0; fault < 4; ++fault) {
      const auto event = proof.event;
      const auto modeled = proof.modeledMatched;
      const auto state = proof.replayStates[0];
      const auto unavailable = observed.modelUnavailable;
      if (fault == 0)
        ++proof.event;
      if (fault == 1)
        proof.modeledMatched = false;
      if (fault == 2)
        proof.replayStates[0].host.present = true;
      if (fault == 3)
        observed.modelUnavailable = true;
      s.legacyEntitiesCompoundJoins.clear();
      r.finishLegacyEntitiesCompoundJoins(d);
      const bool rejected = s.legacyEntitiesCompoundJoins.empty();
      proof.event = event;
      proof.modeledMatched = modeled;
      proof.replayStates[0] = state;
      observed.modelUnavailable = unavailable;
      if (!rejected)
        return false;
      s.legacyEntitiesCompoundFailed = false;
      r.finishLegacyEntitiesCompoundJoins(d);
      if (s.legacyEntitiesCompoundFailed ||
          s.legacyEntitiesCompoundJoins.size() != 1u)
        return false;
    }
    return true;
  }
  static bool crossRead(dx_iface &r, dx_data &a, const dx_data &b) {
    auto &s = a.m_tableDxfSource;
    const auto &t = b.m_tableDxfSource;
    auto input = s.legacyIdentityObservation;
    auto owner = s.legacyIdentityObservationOwner;
    auto source = s.legacyIdentitySourceOwner;
    auto context = s.legacyIdentityContextSnapshot;
    s.legacyIdentityObservation = t.legacyIdentityObservation;
    s.legacyIdentityObservationOwner = t.legacyIdentityObservationOwner;
    s.legacyIdentitySourceOwner = t.legacyIdentitySourceOwner;
    s.legacyIdentityContextSnapshot = t.legacyIdentityContextSnapshot;
    s.legacyEntitiesCompoundJoins.clear();
    r.finishLegacyEntitiesCompoundJoins(a);
    const bool rejected =
        s.legacyEntitiesCompoundFailed && s.legacyEntitiesCompoundJoins.empty();
    s.legacyIdentityObservation = input;
    s.legacyIdentityObservationOwner = owner;
    s.legacyIdentitySourceOwner = source;
    s.legacyIdentityContextSnapshot = context;
    s.legacyEntitiesCompoundFailed = false;
    r.finishLegacyEntitiesCompoundJoins(a);
    return rejected && !s.legacyEntitiesCompoundFailed &&
           s.legacyEntitiesCompoundJoins.size() == 1u;
  }
  static bool badContext(dx_iface &r, dx_data &a) {
    auto &s = a.m_tableDxfSource;
    auto &context =
        const_cast<dx_data::TableDxfSourceCapture::EntitiesReadContext &>(
            *s.legacyIdentityObservation->context);
    const auto old = context.binary;
    context.binary = !old;
    s.legacyEntitiesCompoundJoins.clear();
    r.finishLegacyEntitiesCompoundJoins(a);
    bool rejected =
        s.legacyEntitiesCompoundFailed && s.legacyEntitiesCompoundJoins.empty();
    context.binary = old;
    s.legacyEntitiesCompoundFailed = false;
    r.finishLegacyEntitiesCompoundJoins(a);
    return rejected && !s.legacyEntitiesCompoundFailed &&
           s.legacyEntitiesCompoundJoins.size() == 1u;
  }
  static bool multi(const dx_data &data) {
    const auto &s = data.m_tableDxfSource;
    if (s.legacyEntitiesCompoundFailed ||
        s.legacyEntitiesCompoundJoins.size() != 2u ||
        s.entitiesCompoundAudits.size() != 2u)
      return false;
    const auto &a = *s.entitiesCompoundAudits[0];
    const auto &b = *s.entitiesCompoundAudits[1];
    const auto &census = *s.legacyIdentityObservation->source;
    dx_legacy_identity_join::Index idx;
    if (!dx_legacy_identity_join::buildIndex(census, idx) || a.proofs.empty() ||
        b.proofs.empty())
      return false;
    dx_legacy_attribute_compound_join::Result result;
    return !dx_legacy_attribute_compound_join::join(
               *a.physical, *b.delivery, census, idx, a.proofs[0].first,
               a.proofs[0].dispatch, result) &&
           !dx_legacy_attribute_compound_join::join(
               *b.physical, *a.delivery, census, idx, b.proofs[0].first,
               b.proofs[0].dispatch, result);
  }
  static bool test(const dx_data &data, unsigned &negative) {
    const auto &s = data.m_tableDxfSource;
    if (!s.legacyIdentityObservation || !s.legacyIdentityObservation->source ||
        s.entitiesCompoundAudits.size() != 1u || s.entitiesCaptureFailed ||
        s.entitiesDeliveryCaptureFailed || s.legacyEntitiesCompoundFailed ||
        s.legacyEntitiesCompoundJoins.size() != 1u)
      return false;
    const auto &census = *s.legacyIdentityObservation->source;
    const auto &audit = *s.entitiesCompoundAudits[0];
    dx_legacy_identity_join::Index index;
    if (!dx_legacy_identity_join::buildIndex(census, index))
      return false;
    const auto it = std::find_if(
        audit.proofs.begin(), audit.proofs.end(), [&](const auto &p) {
          return p.compound && p.first < audit.physical->records.size() &&
                 audit.physical->records[p.first].name == "INSERT";
        });
    if (it == audit.proofs.end() ||
        it->state != dx_data::TableDxfSourceCapture::EntitiesCompoundProof::
                         State::ProfileMismatch ||
        !it->modeledMatched)
      return false;
    dx_legacy_attribute_compound_join::Result out;
    const auto run = [&](const auto &physical, const auto &delivery,
                         const auto &identity, const auto &ix) {
      return dx_legacy_attribute_compound_join::join(
          physical, delivery, identity, ix, it->first, it->dispatch, out);
    };
    if (!run(*audit.physical, *audit.delivery, census, index))
      return false;
    if (out.attribute.owner != out.root.self ||
        out.footer.owner != out.root.self || out.root.self == out.root.owner ||
        out.root.block.empty() || out.attribute.appId != "AcDbAttr")
      return false;
    const auto stored = s.legacyEntitiesCompoundJoins.find(out.root.self);
    if (stored == s.legacyEntitiesCompoundJoins.end() || !stored->second ||
        stored->second->root != out.root.self ||
        stored->second->attribute != out.attribute.self ||
        stored->second->footer != out.footer.self ||
        stored->second->ownerRecord != out.ownerRecord)
      return false;
    const auto p = it->first;
    const auto d = it->dispatch;
    const auto rootId = out.root.self, attributeId = out.attribute.self;
    for (unsigned fault = 0; fault < 35; ++fault) {
      auto phys = *audit.physical;
      auto manifest = *audit.delivery;
      auto identity = census;
      auto &a = phys.records[p];
      auto &b = phys.records[p + 1];
      auto &c = phys.records[p + 2];
      switch (fault) {
      case 0:
        a.groups[0].addString(5, "0");
        break;
      case 1:
        a.groups[1].addString(330, "FFFFFFFFF");
        break;
      case 2:
        a.groups[2].addString(100, "AcDbText");
        break;
      case 3:
        a.groups[3].addString(8, "");
        break;
      case 4:
        a.groups[4].addString(100, "AcDbBlockBegin");
        break;
      case 5:
        a.groups[5].addInt(66, 0);
        break;
      case 6:
        a.groups[6].addString(2, "Na\xC3\xAFve");
        break;
      case 7:
        a.groups[7].addDouble(10, std::numeric_limits<double>::quiet_NaN());
        break;
      case 8:
        b.groups[1].addString(330, "FF");
        break;
      case 9:
        b.groups[4].addString(100, "AcDbMText");
        break;
      case 10:
        b.groups[8].addDouble(40, 0.0);
        break;
      case 11:
        b.groups[9].addString(1, "");
        break;
      case 12:
        b.groups[10].addString(100, "AcDbAttributeDefinition");
        break;
      case 13:
        b.groups[11].addString(2, "");
        break;
      case 14:
        b.groups[12].addInt(70, 1);
        break;
      case 15:
        b.groups[13].addString(1001, "OTHER");
        break;
      case 16:
        b.groups[14].addInt(1070, 32768);
        break;
      case 17:
        b.groups[15].addInt(1070, -32769);
        break;
      case 18:
        c.groups[1].addString(330, "FF");
        break;
      case 19:
        c.groups[2].addString(100, "AcDbBlockEnd");
        break;
      case 20:
        a.groups.pop_back();
        break;
      case 21:
        b.spans[0].ignoredComment = true;
        break;
      case 22:
        c.name = "LINE";
        break;
      case 23:
        ++a.endOffset;
        break;
      case 24:
        ++b.marker.ordinal;
        break;
      case 25:
        ++c.spans[0].beginOffset;
        break;
      case 26:
        manifest.dispatches[d].events = 0;
        break;
      case 27:
        ++manifest.dispatches[d].end.beginOffset;
        break;
      case 28:
        manifest.events[manifest.dispatches[d].firstEvent].owner++;
        break;
      case 29:
        ++manifest.scopeId;
        break;
      case 30:
        phys.version = DRW::AC1021;
        break;
      case 31:
        identity.version = DRW::AC1021;
        break;
      case 32: {
        auto &candidate = identity.candidates[index.ids.at(rootId).candidate];
        candidate.repeatedAdmissions = 1u;
        break;
      }
      case 33: {
        auto &candidate =
            identity.candidates[index.ids.at(attributeId).candidate];
        candidate.admission = DRW_DxfIdentitySource::Admission::Duplicate;
        break;
      }
      case 34: {
        auto alias = identity.candidates[index.ids.at(rootId).candidate];
        alias.lexeme = "000000000000000E0";
        alias.rawValue = identity.binary ? "" : alias.lexeme;
        alias.record = DRW_DxfIdentitySource::NoIndex;
        alias.scope = DRW_DxfIdentitySource::Scope::Unframed;
        identity.candidates.push_back(alias);
        break;
      }
      }
      dx_legacy_identity_join::Index changed;
      if (!dx_legacy_identity_join::buildIndex(identity, changed)) {
        ++negative;
        continue;
      }
      if (run(phys, manifest, identity, changed)) {
        std::cerr << "accepted fault " << fault << "\n";
        return false;
      }
      ++negative;
    }
    return true;
  }
};
struct Reader : dx_iface {
  bool mutate = false;
  bool wantsDxfEntitiesSource() const override { return true; }
  bool wantsDxfEntitiesDeliverySource() const override { return true; }
  void addDxfEntitiesSource(const DRW_DxfEntitiesSource &source) override {
    if (!mutate) {
      dx_iface::addDxfEntitiesSource(source);
      return;
    }
    auto &changed = const_cast<DRW_DxfEntitiesSource &>(source);
    changed.records[0].rawValues[0] += '0';
    dx_iface::addDxfEntitiesSource(source);
    changed.records[0].rawValues[0].pop_back();
  }
};
static bool write(const std::filesystem::path &path, const Tags &tags,
                  bool bin) {
  std::ofstream out(path, std::ios::binary);
  const auto word = [&](std::uint64_t n, unsigned k) {
    for (unsigned i = 0; i < k; ++i)
      out.put(static_cast<char>(n >> (i * 8u)));
  };
  if (bin)
    out.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
  for (const auto &t : tags)
    if (!bin)
      out << t.code << '\n' << t.value << '\n';
    else {
      word(static_cast<unsigned>(t.code), 2);
      if (t.code >= 10 && t.code <= 59) {
        double v = std::stod(t.value);
        std::uint64_t bits = 0;
        std::memcpy(&bits, &v, 8);
        word(bits, 8);
      } else if ((t.code >= 60 && t.code <= 79) || t.code == 1070)
        word(static_cast<std::uint16_t>(std::stoi(t.value)), 2);
      else {
        out << t.value;
        out.put('\0');
      }
    }
  return out.good();
}
int main() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "legacy-compound", ec);
  if (ec)
    return 1;
  struct Cleanup {
    std::filesystem::path p;
    ~Cleanup() {
      std::error_code e;
      std::filesystem::remove_all(p, e);
    }
  } cleanup{dir};
  Tags doc = {{0, "SECTION"},
              {2, "HEADER"},
              {9, "$ACADVER"},
              {1, "AC1018"},
              {9, "$DWGCODEPAGE"},
              {3, "ANSI_1252"},
              {0, "ENDSEC"},
              {0, "SECTION"},
              {2, "TABLES"},
              {0, "TABLE"},
              {2, "BLOCK_RECORD"},
              {5, "B9"},
              {100, "AcDbSymbolTable"},
              {70, "1"},
              {0, "BLOCK_RECORD"},
              {5, "B0"},
              {330, "B9"},
              {100, "AcDbSymbolTableRecord"},
              {100, "AcDbBlockTableRecord"},
              {2, "TEST_BLOCK"},
              {0, "ENDTAB"},
              {0, "ENDSEC"},
              {0, "SECTION"},
              {2, "ENTITIES"},
              {0, "INSERT"},
              {5, "E0"},
              {330, "B0"},
              {100, "AcDbEntity"},
              {8, "LayerA"},
              {100, "AcDbBlockReference"},
              {66, "1"},
              {2, "TEST_BLOCK"},
              {10, "2"},
              {20, "3"},
              {30, "0"},
              {41, "1"},
              {42, "1"},
              {43, "1"},
              {0, "ATTRIB"},
              {5, "E1"},
              {330, "E0"},
              {100, "AcDbEntity"},
              {8, "0"},
              {100, "AcDbText"},
              {10, "2"},
              {20, "3"},
              {30, "0"},
              {40, "2"},
              {1, "value"},
              {100, "AcDbAttribute"},
              {2, "TEST_TAG"},
              {70, "0"},
              {1001, "AcDbAttr"},
              {1070, "0"},
              {1070, "1"},
              {0, "SEQEND"},
              {5, "E2"},
              {330, "E0"},
              {100, "AcDbEntity"},
              {8, "LayerA"},
              {0, "ENDSEC"},
              {0, "EOF"}};
  unsigned negative = 0, quotas = 0, retries = 0, faults = 0;
  for (bool binary : {false, true}) {
    const auto path = dir / (binary ? "binary.dxf" : "ascii.dxf");
    if (!write(path, doc, binary))
      return 2;
    Reader r;
    dx_data d;
    if (!r.enableLegacyDxfIdentityObservation() ||
        !r.fileImport(path.string(), &d, false) ||
        !DxIfaceTableSourceTestAccess::test(d, negative))
      return 3;
    if (!DxIfaceTableSourceTestAccess::allocations(r, d, faults) ||
        !DxIfaceTableSourceTestAccess::alteredReplay(r, d))
      return 20;
    const auto usage = DxIfaceTableSourceTestAccess::usage(d);
    for (unsigned i = 0; i < 2; ++i)
      for (bool exact : {false, true}) {
        Reader bounded;
        bounded.enableLegacyDxfIdentityObservation();
        dx_data target;
        const auto v = i ? usage.second : usage.first;
        if (!v)
          return 5;
        DxIfaceTableSourceTestAccess::limit(target, i, v - (exact ? 0u : 1u));
        if (!bounded.fileImport(path.string(), &target, false) ||
            DxIfaceTableSourceTestAccess::failed(target) == exact ||
            DxIfaceTableSourceTestAccess::count(target) !=
                static_cast<std::size_t>(exact))
          return 6;
        ++quotas;
        if (!exact) {
          dx_data retry;
          if (!bounded.fileImport(path.string(), &retry, false) ||
              DxIfaceTableSourceTestAccess::count(retry) != 1u)
            return 7;
          ++retries;
        }
      }
    dx_data other;
    if (!r.fileImport(path.string(), &other, false) ||
        !DxIfaceTableSourceTestAccess::crossRead(r, d, other) ||
        !DxIfaceTableSourceTestAccess::badContext(r, d))
      return 8;
    Reader tamper;
    tamper.enableLegacyDxfIdentityObservation();
    tamper.mutate = true;
    dx_data rejected;
    if (!tamper.fileImport(path.string(), &rejected, false) ||
        DxIfaceTableSourceTestAccess::count(rejected) != 0u)
      return 11;
    tamper.mutate = false;
    dx_data corrected;
    if (!tamper.fileImport(path.string(), &corrected, false) ||
        DxIfaceTableSourceTestAccess::count(corrected) != 1u)
      return 12;
    auto renamed = doc;
    for (std::size_t i = 0; i + 1 < renamed.size(); ++i)
      if (renamed[i].code == 100 &&
          renamed[i].value == "AcDbBlockTableRecord" &&
          renamed[i + 1].code == 2) {
        renamed[i + 1].value = "OTHER_BLOCK";
        break;
      }
    if (!write(path, renamed, binary))
      return 13;
    dx_data targetNotResolved;
    if (!r.fileImport(path.string(), &targetNotResolved, false) ||
        DxIfaceTableSourceTestAccess::count(targetNotResolved) != 1u)
      return 14;
    auto multi = doc;
    auto first = std::find_if(multi.begin(), multi.end(), [](const Tag &t) {
      return t.code == 2 && t.value == "ENTITIES";
    });
    if (first == multi.end())
      return 15;
    auto last = std::find_if(first, multi.end(), [](const Tag &t) {
      return t.code == 0 && t.value == "ENDSEC";
    });
    if (last == multi.end())
      return 16;
    Tags extra(first - 1, last + 1);
    for (auto &tag : extra)
      if (tag.value == "E0")
        tag.value = "E3";
      else if (tag.value == "E1")
        tag.value = "E4";
      else if (tag.value == "E2")
        tag.value = "E5";
    multi.insert(multi.end() - 1, extra.begin(), extra.end());
    if (!write(path, multi, binary))
      return 17;
    dx_data two;
    if (!r.fileImport(path.string(), &two, false) ||
        !DxIfaceTableSourceTestAccess::multi(two))
      return 18;
    if (!write(path, doc, binary))
      return 19;
    if (!r.fileImport(path.string(), &d, false) ||
        DxIfaceTableSourceTestAccess::count(d) != 0u)
      return 9;
    dx_data retry;
    if (!r.fileImport(path.string(), &retry, false) ||
        DxIfaceTableSourceTestAccess::count(retry) != 1u)
      return 10;
    ++retries;
  }
  std::cout
      << "source-join profiles=2 negative=" << negative
      << " ascii-binary=2 quota=" << quotas << " retries=" << retries
      << " allocation-failures=" << faults
      << " cross-read=2 context=2 replay=8 cross-section=4 tamper-retries=2 "
         "renamed-target=unresolved generic=ProfileMismatch\n";
  return negative == 70 ? 0 : 4;
}
