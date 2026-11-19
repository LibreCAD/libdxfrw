// SPDX-License-Identifier: GPL-2.0-or-later
// Synthetic write controls only; no drawing fixture is committed.
#include "libdxfrw.h"
#include "intern/dxfparserlimits.h"
#include "test_scratch.h"
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
class Bare : public DRW_Interface {
public:
  dxfRW *codec = nullptr;
  int mode = 0;
  int classesRead = 0;
  bool customClassRead = false;
  bool attempted = false, receipt = false;
  static DRW_RawDxfSection raw() {
    DRW_RawDxfSection s;
    s.m_name = "TEST_PRIVATE";
    s.m_version = DRW::AC1027;
    s.m_groups = {{0, std::string("TEST_RECORD")}, {340, std::string("D")}};
    return s;
  }
  bool finalizeDxfWrite() override {
    receipt = codec->dxfWriteNoRawSectionsObserved();
    if (mode == 4)
      codec->setRawDxfSections({raw()});
    if (mode == 5) {
      std::vector<DRW_RawDxfSection> v{raw()};
      codec->returnAdoptedRawDxfSections(v);
    }
    if (mode == 6)
      return false;
    if (mode == 9)
      throw std::runtime_error("test finalizer exception");
    if (mode == 10)
      attempted = !codec->setDxfNoRawSectionsWriteGuard(false);
    return true;
  }
  void addHeader(const DRW_Header *) override {}
  void addDxfClass(const DRW_Class &data) override {
    ++classesRead;
    customClassRead = customClassRead || data.recName == "TEST_CUSTOM_RECORD";
  }
  void addLType(const DRW_LType &) override {}
  void addLayer(const DRW_Layer &) override {}
  void addDimStyle(const DRW_Dimstyle &) override {}
  void addVport(const DRW_Vport &) override {}
  void addTextStyle(const DRW_Textstyle &) override {}
  void addAppId(const DRW_AppId &) override {}
  void addBlock(const DRW_Block &) override {}
  void setBlock(int) override {}
  void endBlock() override {}
  void addPoint(const DRW_Point &) override {}
  void addLine(const DRW_Line &) override {}
  void addRay(const DRW_Ray &) override {}
  void addXline(const DRW_Xline &) override {}
  void addArc(const DRW_Arc &) override {}
  void addCircle(const DRW_Circle &) override {}
  void addEllipse(const DRW_Ellipse &) override {}
  void addLWPolyline(const DRW_LWPolyline &) override {}
  void addPolyline(const DRW_Polyline &) override {}
  void addSpline(const DRW_Spline *) override {}
  void addKnot(const DRW_Entity &) override {}
  void addInsert(const DRW_Insert &) override {}
  void addTrace(const DRW_Trace &) override {}
  void add3dFace(const DRW_3Dface &) override {}
  void addSolid(const DRW_Solid &) override {}
  void addMText(const DRW_MText &) override {}
  void addText(const DRW_Text &) override {}
  void addDimAlign(const DRW_DimAligned *) override {}
  void addDimLinear(const DRW_DimLinear *) override {}
  void addDimRadial(const DRW_DimRadial *) override {}
  void addDimDiametric(const DRW_DimDiametric *) override {}
  void addDimAngular(const DRW_DimAngular *) override {}
  void addDimAngular3P(const DRW_DimAngular3p *) override {}
  void addDimOrdinate(const DRW_DimOrdinate *) override {}
  void addLeader(const DRW_Leader *) override {}
  void addHatch(const DRW_Hatch *) override {}
  void addViewport(const DRW_Viewport &) override {}
  void addImage(const DRW_Image *) override {}
  void linkImage(const DRW_ImageDef *) override {}
  void addComment(const char *) override {}
  void writeHeader(DRW_Header &) override {}
  void writeBlocks() override {}
  void writeBlockRecords() override {}
  void writeEntities() override {}
  void writeLTypes() override {}
  void writeLayers() override {}
  void writeTextstyles() override {}
  void writeVports() override {}
  void writeDimstyles() override {}
  void writeObjects() override {
    if (mode == 2)
      codec->setRawDxfSections({raw()});
    if (mode == 3)
      attempted = !codec->setDxfNoRawSectionsWriteGuard(false);
    if (mode == 7) {
      codec->setRawDxfSections({raw()});
      codec->setRawDxfSections({});
    }
    if (mode == 8)
      attempted = !codec->write(this, DRW::AC1027, false);
  }
  void writeAppId() override {}
};
static std::string read(const std::string &p) {
  std::ifstream f(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(f), {}};
}
static void sentinel(const std::string &p) {
  std::ofstream f(p, std::ios::binary);
  f << "sentinel";
}
struct StagedFileProbe {
  bool accept = false;
  bool throwException = false;
  bool called = false;
  std::string stagedBytes;
};
static bool verifyStagedFile(const std::filesystem::path &path, void *opaque) {
  auto *probe = static_cast<StagedFileProbe *>(opaque);
  probe->called = true;
  if (probe->throwException)
    throw 1;
  probe->stagedBytes = read(path.string());
  return probe->accept && !probe->stagedBytes.empty();
}
int main() {
  std::error_code error;
  const auto directory = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "no-raw-sections", error);
  if (error || directory.empty())
    return 8;
  struct Cleanup {
    std::filesystem::path p;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(p, ignored);
    }
  } cleanup{directory};
  const std::string path = (directory / "output.dxf").string();
  const std::vector<DRW_RawDxfSection> oversizedRawSections(
      DRW::kMaxDxfRawSections + 1u);
  // Keep this count one above dxfRW's private 100,000-reference ceiling.
  const std::vector<std::uint32_t> oversizedSunReferences(100001u, 0xA100u);
  for (bool binary : {false, true}) {
    Bare boundedAdapter;
    dxfRW boundedCodec(path.c_str());
    boundedAdapter.codec = &boundedCodec;
    boundedCodec.setRawDxfSections(oversizedRawSections);
    sentinel(path);
    if (boundedCodec.write(&boundedAdapter, DRW::AC1027, binary) ||
        read(path) != "sentinel")
      return 11;
    boundedCodec.setRawDxfSections({Bare::raw()});
    if (!boundedCodec.write(&boundedAdapter, DRW::AC1027, binary) ||
        read(path).find("TEST_PRIVATE") == std::string::npos)
      return 12;
  }
  for (bool binary : {false, true}) {
    Bare boundedAdapter;
    dxfRW boundedCodec(path.c_str());
    boundedAdapter.codec = &boundedCodec;
    boundedCodec.setResolvedSunHandles(oversizedSunReferences);
    sentinel(path);
    if (boundedCodec.write(&boundedAdapter, DRW::AC1027, binary) ||
        read(path) != "sentinel")
      return 13;
    boundedCodec.setResolvedSunHandles({0xA101u});
    if (!boundedCodec.write(&boundedAdapter, DRW::AC1027, binary) ||
        read(path) == "sentinel")
      return 14;
  }
  std::map<std::uint32_t, std::uint32_t> oversizedHandleRemap;
  for (std::uint32_t index = 0u; index < 100001u; ++index)
    oversizedHandleRemap.emplace(0x10000000u + index,
                                 0x20000000u + index);
  for (bool binary : {false, true}) {
    Bare boundedAdapter;
    dxfRW boundedCodec(path.c_str());
    boundedAdapter.codec = &boundedCodec;
    boundedCodec.setHandleRemap(oversizedHandleRemap);
    sentinel(path);
    if (boundedCodec.write(&boundedAdapter, DRW::AC1027, binary) ||
        read(path) != "sentinel")
      return 15;
    boundedCodec.setHandleRemap({{0xA500u, 0xA501u}});
    if (!boundedCodec.write(&boundedAdapter, DRW::AC1027, binary) ||
        read(path) == "sentinel")
      return 16;
  }
  Bare b;
  dxfRW c(path.c_str());
  b.codec = &c;
  unsigned checks = 0;
  for (bool binary : {false, true}) {
    // Default-off compatibility: the regular raw-section writer remains
    // available.
    c.setRawDxfSections({Bare::raw()});
    b.mode = 0;
    b.receipt = true;
    if (!c.write(&b, DRW::AC1027, binary) || b.receipt ||
        read(path).find("TEST_PRIVATE") == std::string::npos)
      return 1;
    ++checks;
    c.setRawDxfSections({});
    if (!c.setDxfNoRawSectionsWriteGuard(true) ||
        c.dxfWriteNoRawSectionsObserved())
      return 2;
    for (int mode : {1, 2, 3, 4, 5, 6, 9, 10}) {
      b.mode = mode;
      b.attempted = false;
      b.receipt = false;
      c.setRawDxfSections(mode == 1
                              ? std::vector<DRW_RawDxfSection>{Bare::raw()}
                              : std::vector<DRW_RawDxfSection>{});
      sentinel(path);
      if (c.write(&b, DRW::AC1027, binary) || read(path) != "sentinel" ||
          c.dxfWriteNoRawSectionsObserved() ||
          ((mode == 3 || mode == 10) && !b.attempted) ||
          (mode >= 4 && !b.receipt)) {
        std::cerr << "failed mode " << mode << " binary " << binary
                  << " receipt " << b.receipt << "\n";
        return 3;
      }
      ++checks;
      c.setRawDxfSections({});
      b.mode = 0;
      b.receipt = false;
      if (!c.write(&b, DRW::AC1027, binary) || !b.receipt ||
          c.dxfWriteNoRawSectionsObserved() ||
          read(path).find("TEST_PRIVATE") != std::string::npos)
        return 4;
      ++checks;
    }
    b.mode = 7;
    b.receipt = false;
    if (!c.write(&b, DRW::AC1027, binary) || !b.receipt ||
        read(path).find("TEST_PRIVATE") != std::string::npos)
      return 5;
    ++checks;
    b.mode = 8;
    b.receipt = false;
    b.attempted = false;
    if (!c.write(&b, DRW::AC1027, binary) || !b.receipt || !b.attempted)
      return 6;
    ++checks;
    if (!c.setDxfNoRawSectionsWriteGuard(false))
      return 7;
  }
  DRW_Class generatedClass;
  generatedClass.recName = "TEST_CUSTOM_RECORD";
  generatedClass.className = "AcDbTestCustom";
  generatedClass.appName = "TEST_APP";
  generatedClass.instanceCount = 1;
  if (!c.reserveHandle(0x20044u)
      || !c.setDxfStagedOutputAuthentication(true))
    return 8;
  c.setDxfClasses({generatedClass});
  for (bool binary : {false, true}) {
    sentinel(path);
    if (c.write(&b, DRW::AC1009, binary)
        || read(path) != "sentinel"
        || c.getError() != DRW::BAD_VERSION
        || c.getLastDiagnostic().code != "dxf-class-target-version")
      return 13;
    ++checks;
    if (!c.write(&b, DRW::AC1027, binary)) {
      const auto diagnostic = c.getLastDiagnostic();
      std::cerr << "staged output authentication failed for binary=" << binary
                << " stage=" << diagnostic.code << " " << diagnostic.message
                << "\n";
      return 9;
    }
    const std::string emitted = read(path);
    if (emitted.find("TEST_CUSTOM_RECORD") == std::string::npos)
      return 10;
    char expectedHandseed[DRW_Header::kHandseedFieldWidth + 1];
    std::snprintf(expectedHandseed, sizeof(expectedHandseed), "%08X",
                  c.highWaterHandle());
    if (std::string(expectedHandseed) == "00020000"
        || emitted.find(expectedHandseed) == std::string::npos)
      return 12;
    dxfRW readbackCodec(path.c_str());
    Bare readback;
    if (!readbackCodec.read(&readback, false)
        || readback.classesRead != 3 || !readback.customClassRead) {
      std::cerr << "staged output readback failed for binary=" << binary
                << " classes=" << readback.classesRead
                << "\n";
      return 11;
    }
  }
  dxfRW verifiedCodec(path.c_str());
  Bare verifiedWriter;
  verifiedWriter.codec = &verifiedCodec;
  for (bool binary : {false, true}) {
    StagedFileProbe reject;
    sentinel(path);
    if (verifiedCodec.writeVerified(&verifiedWriter, DRW::AC1027, binary,
                                   &verifyStagedFile, &reject) ||
        !reject.called || reject.stagedBytes.empty() ||
        read(path) != "sentinel")
      return 14;
    ++checks;

    StagedFileProbe accept;
    accept.accept = true;
    if (!verifiedCodec.writeVerified(&verifiedWriter, DRW::AC1027, binary,
                                    &verifyStagedFile, &accept) ||
        !accept.called || accept.stagedBytes.empty() || read(path).empty())
      return 15;
    ++checks;

    sentinel(path);
    if (verifiedCodec.writeVerified(&verifiedWriter, DRW::AC1027, binary,
                                   nullptr, nullptr) ||
        read(path) != "sentinel")
      return 16;
    ++checks;
  }
  std::cout << "raw-section guard checks=" << checks << " ASCII/binary pass\n";
}
