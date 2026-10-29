// SPDX-License-Identifier: GPL-2.0-or-later
// Synthetic write controls only; no drawing fixture is committed.
#include "libdxfrw.h"
#include "test_scratch.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
class Bare : public DRW_Interface {
public:
  dxfRW *codec = nullptr;
  int mode = 0;
  int classesRead = 0;
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
  void addDxfClass(const DRW_Class &) override { ++classesRead; }
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
        || readback.classesRead != 1) {
      std::cerr << "staged output readback failed for binary=" << binary
                << " classes=" << readback.classesRead
                << "\n";
      return 11;
    }
  }
  std::cout << "raw-section guard checks=" << checks << " ASCII/binary pass\n";
}
