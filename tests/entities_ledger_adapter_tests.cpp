#include "dx_data.h"
#include "dx_iface.h"
#include "test_scratch.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
class DxIfaceTableSourceTestAccess {
public:
  static bool bad(const dx_data &d) {
    return d.m_tableDxfSource.entitiesCaptureFailed ||
           d.m_tableDxfSource.entitiesDeliveryCaptureFailed;
  }
  static auto counts(const dx_data &d) {
    return std::make_pair(d.m_tableDxfSource.entitiesSources.size(),
                          d.m_tableDxfSource.entitiesDeliveries.size());
  }
};
struct H : dx_iface {
  int mode = 0;
  bool source = true, delivery = true;
  int n = 0;
  bool wantsDxfEntitiesSource() const override { return source; }
  bool wantsDxfEntitiesDeliverySource() const override { return delivery; }
  void addDxfEntitiesSource(const DRW_DxfEntitiesSource &c) override {
    ++n;
    if (mode == 1)
      return;
    if (mode == 2) {
      auto x = c;
      dx_iface::addDxfEntitiesSource(x);
      return;
    }
    if (mode == 3) {
      dx_iface::addDxfEntitiesSource(c);
      dx_iface::addDxfEntitiesSource(c);
      return;
    }
    if (mode == 4) {
      auto &x = const_cast<DRW_DxfEntitiesSource &>(c);
      x.records[0].rawValues[0] += '0';
      dx_iface::addDxfEntitiesSource(c);
      x.records[0].rawValues[0].pop_back();
      return;
    }
    if (mode == 5)
      throw std::runtime_error("source");
    dx_iface::addDxfEntitiesSource(c);
  }
  void addDxfEntitiesDeliverySource(
      const DRW_DxfEntitiesDeliverySource &c) override {
    if (mode == 6)
      return;
    if (mode == 7) {
      auto x = c;
      dx_iface::addDxfEntitiesDeliverySource(x);
      return;
    }
    if (mode == 8) {
      dx_iface::addDxfEntitiesDeliverySource(c);
      dx_iface::addDxfEntitiesDeliverySource(c);
      return;
    }
    if (mode == 9) {
      auto &x = const_cast<DRW_DxfEntitiesDeliverySource &>(c);
      x.events[0].handle++;
      dx_iface::addDxfEntitiesDeliverySource(c);
      x.events[0].handle--;
      return;
    }
    if (mode == 10)
      throw std::runtime_error("manifest");
    dx_iface::addDxfEntitiesDeliverySource(c);
  }
};
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      std::cerr << "fail line " << __LINE__ << " " << #x << "\n";              \
      return 1;                                                                \
    }                                                                          \
  } while (0)
int main() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "libdxfrw-entities-auth", ec);
  if (ec)
    return 1;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{dir};
  const auto path = (dir / "control.dxf").string();
  std::ofstream out(path);
  out << "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1018\n9\n$"
         "DWGCODEPAGE\n3\nANSI_"
         "1252\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nPOINT\n5\n101\n8\n0\n10"
         "\n1."
         "0\n20\n2\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nPOINT\n5\n102\n8\n0"
         "\n10\n3\n20\n4\n0\nENDSEC\n0\nEOF\n";
  out.close();
  H h;
  CHECK(h.enableLegacyDxfIdentityObservation());
  dx_data ok;
  CHECK(h.fileImport(path, &ok, false));
  CHECK(!DxIfaceTableSourceTestAccess::bad(ok));
  CHECK(DxIfaceTableSourceTestAccess::counts(ok) ==
        std::make_pair(std::size_t(2), std::size_t(2)));
  for (int mode = 1; mode <= 10; ++mode) {
    h.mode = mode;
    dx_data bad;
    bool threw = false;
    try {
      CHECK(h.fileImport(path, &bad, false));
    } catch (const std::runtime_error &) {
      threw = true;
    }
    if (mode == 5 || mode == 10)
      CHECK(threw);
    else
      CHECK(!threw && DxIfaceTableSourceTestAccess::bad(bad));
    h.mode = 0;
    dx_data retry;
    CHECK(h.fileImport(path, &retry, false));
    CHECK(!DxIfaceTableSourceTestAccess::bad(retry));
    CHECK(DxIfaceTableSourceTestAccess::counts(retry) ==
          std::make_pair(std::size_t(2), std::size_t(2)));
  }
  // Locally generated binary DXF from the same scalar tags.
  std::ifstream text(path);
  std::vector<std::pair<int, std::string>> tags;
  std::string code, value;
  while (std::getline(text, code) && std::getline(text, value))
    tags.emplace_back(std::stoi(code), value);
  text.close();
  std::ofstream binary(path, std::ios::binary | std::ios::trunc);
  binary.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
  const auto word = [&](std::uint64_t value, unsigned n) {
    for (unsigned i = 0; i < n; ++i)
      binary.put(static_cast<char>(value >> (i * 8u)));
  };
  for (const auto &tag : tags) {
    word(static_cast<unsigned>(tag.first), 2u);
    if (tag.first >= 10 && tag.first <= 59) {
      double d = std::stod(tag.second);
      std::uint64_t bits = 0;
      std::memcpy(&bits, &d, sizeof(d));
      word(bits, 8u);
    } else {
      binary << tag.second;
      binary.put('\0');
    }
  }
  binary.close();
  for (int mode = 0; mode <= 10; ++mode) {
    h.mode = mode;
    dx_data sample;
    bool threw = false;
    try {
      CHECK(h.fileImport(path, &sample, false));
    } catch (const std::runtime_error &) {
      threw = true;
    }
    if (mode == 5 || mode == 10)
      CHECK(threw);
    else
      CHECK(!threw &&
            (DxIfaceTableSourceTestAccess::bad(sample) == (mode != 0)));
    h.mode = 0;
    dx_data retry;
    CHECK(h.fileImport(path, &retry, false));
    CHECK(!DxIfaceTableSourceTestAccess::bad(retry));
  }
  h.mode = 4;
  dx_data reused;
  CHECK(h.fileImport(path, &reused, false));
  CHECK(DxIfaceTableSourceTestAccess::bad(reused));
  h.mode = 0;
  CHECK(h.fileImport(path, &reused, false));
  CHECK(DxIfaceTableSourceTestAccess::bad(reused));
  H sourceonly;
  sourceonly.delivery = false;
  CHECK(sourceonly.enableLegacyDxfIdentityObservation());
  dx_data part;
  CHECK(sourceonly.fileImport(path, &part, false));
  CHECK(!DxIfaceTableSourceTestAccess::bad(part));
  CHECK(DxIfaceTableSourceTestAccess::counts(part) ==
        std::make_pair(std::size_t(2), std::size_t(0)));
  // Legacy consumers that selected only the physical facet before this
  // ledger existed retain their original capture behavior without opting in.
  H oldPhysical;
  oldPhysical.delivery = false;
  dx_data oldData;
  CHECK(oldPhysical.fileImport(path, &oldData, false));
  CHECK(!DxIfaceTableSourceTestAccess::bad(oldData));
  CHECK(DxIfaceTableSourceTestAccess::counts(oldData) ==
        std::make_pair(std::size_t(2), std::size_t(0)));
  H normal;
  normal.source = false;
  dx_data plain;
  CHECK(normal.fileImport(path, &plain, false));
  CHECK(!DxIfaceTableSourceTestAccess::bad(plain));
  CHECK(DxIfaceTableSourceTestAccess::counts(plain) ==
        std::make_pair(std::size_t(0), std::size_t(0)));
  std::cout << "adapter_bad_callbacks=20 retries=21 same_data_refusal=1 "
               "binary=1 source_only=1 old_facet=1 default=1 sections=2\n";
}
