// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdxfrw.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

namespace {
enum class Entry { Point, Line, Circle, Ray, Trace, Text, Attrib, Hatch, Dimension };
class Writer : public LC3CompatCheck {
public:
    explicit Writer(dxfRW& writer) : out(writer) {}
    void writeEntities() override {
        switch (entry) {
        case Entry::Point: ok = out.writePoint(dynamic_cast<DRW_Point*>(entity)); break;
        case Entry::Line: ok = out.writeLine(dynamic_cast<DRW_Line*>(entity)); break;
        case Entry::Circle: ok = out.writeCircle(dynamic_cast<DRW_Circle*>(entity)); break;
        case Entry::Ray: ok = out.writeRay(dynamic_cast<DRW_Ray*>(entity)); break;
        case Entry::Trace: ok = out.writeTrace(dynamic_cast<DRW_Trace*>(entity)); break;
        case Entry::Text: ok = out.writeText(dynamic_cast<DRW_Text*>(entity)); break;
        case Entry::Attrib: ok = out.writeAttrib(dynamic_cast<DRW_Attrib*>(entity)); break;
        case Entry::Hatch: ok = out.writeHatch(dynamic_cast<DRW_Hatch*>(entity)); break;
        case Entry::Dimension: ok = out.writeDimension(dynamic_cast<DRW_Dimension*>(entity)); break;
        }
    }
    bool finalizeDxfWrite() override { return ok; }
    dxfRW& out;
    DRW_Entity* entity = nullptr;
    Entry entry = Entry::Line;
    bool ok = false;
};
class Reader : public LC3CompatCheck {
public:
    void addPoint(const DRW_Point& value) override { name = "POINT"; valid = value.basePoint.x == 2.; }
    void addDimLinear(const DRW_DimLinear* value) override { name = "DIMLINEAR"; valid = value && value->getDef1Point().x == 3.; }
    void addDimAlign(const DRW_DimAligned* value) override { name = "DIMALIGNED"; valid = value != nullptr; }
    void addDimRadial(const DRW_DimRadial* value) override {
        if (const auto* large = dynamic_cast<const DRW_DimLargeRadial*>(value)) {
            name = "DIMLARGERADIAL"; valid = large->jogAngle == .25;
        } else { name = "DIMRADIAL"; valid = value != nullptr; }
    }
    void addDimDiametric(const DRW_DimDiametric* value) override { name = "DIMDIAMETRIC"; valid = value != nullptr; }
    void addDimAngular(const DRW_DimAngular* value) override { name = "DIMANGULAR"; valid = value != nullptr; }
    void addDimAngular3P(const DRW_DimAngular3p* value) override { name = "DIMANGULAR3P"; valid = value != nullptr; }
    void addDimOrdinate(const DRW_DimOrdinate* value) override { name = "DIMORDINATE"; valid = value != nullptr; }
    void addDimArc(const DRW_DimArc* value) override { name = "DIMARC"; valid = value != nullptr; }
    void addViewport(const DRW_Viewport& value) override { name = "VIEWPORT"; valid = value.pswidth == 9.; }
    void addPolyline(const DRW_Polyline& value) override { name = "POLYLINE"; valid = value.vertlist.size() == 1; }
    void addInsert(const DRW_Insert& value) override { name = "INSERT"; valid = value.name == "TEST_BLOCK"; }
    void addHatch(const DRW_Hatch* value) override { name = "HATCH"; valid = value && value->looplist.size() == 1; }
    void addMPolygon(const DRW_MPolygon* value) override { name = "MPOLYGON"; valid = value && value->looplist.size() == 1; }
    void addLine(const DRW_Line& value) override { name = "LINE"; valid = value.secPoint.x == 3.; }
    void add3DLine(const DRW_3DLine& value) override { name = "3DLINE"; valid = value.secPoint.z == 4.; }
    void addRay(const DRW_Ray&) override { name = "RAY"; valid = true; }
    void addXline(const DRW_Xline&) override { name = "XLINE"; valid = true; }
    void addCircle(const DRW_Circle&) override { name = "CIRCLE"; valid = true; }
    void addArc(const DRW_Arc& value) override {
        name = "ARC"; valid = value.radious == 5. && std::fabs(value.endangle - 1.5707963267948966) < 1e-10;
    }
    void addEllipse(const DRW_Ellipse& value) override { name = "ELLIPSE"; valid = value.ratio == 0.5; }
    void addTrace(const DRW_Trace& value) override { name = "TRACE"; valid = value.thirdPoint.x == 2.; }
    void addSolid(const DRW_Solid& value) override { name = "SOLID"; valid = value.thirdPoint.x == 2.; }
    void add3dFace(const DRW_3Dface& value) override { name = "3DFACE"; valid = value.invisibleflag == 3; }
    void addMText(const DRW_MText& value) override { name = "MTEXT"; valid = value.text == "text"; }
    void addText(const DRW_Text& value) override {
        if (const auto* rtext = dynamic_cast<const DRW_RText*>(&value)) {
            name = "RTEXT"; valid = rtext->m_rTextFlags == 1 && value.text == "text";
        } else if (const auto* arc = dynamic_cast<const DRW_ArcAlignedText*>(&value)) {
            name = "ARCALIGNEDTEXT"; valid = arc->m_radius == 5. && value.text == "text";
        } else { name = "TEXT"; valid = value.text == "text"; }
    }
    void addAttDef(const DRW_Attdef& value) override { name = "ATTDEF"; valid = value.tag == "TAG" && value.prompt == "prompt"; }
    void addWipeout(const DRW_Wipeout* value) override { name = "WIPEOUT"; valid = value && value->clipPath.size() == 3; }
    std::string name;
    bool valid = false;
};
std::string contents(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
struct CustomLine : DRW_Line {};
struct CustomText : DRW_Text {};
struct CustomPoint : DRW_Point {};
struct CustomRText : DRW_RText {};
struct CustomArcAlignedText : DRW_ArcAlignedText {};
struct CustomMPolygon : DRW_MPolygon {};
struct CustomDimension : DRW_Dimension {};
struct CustomLinearDimension : DRW_DimLinear {};
struct CustomAttdef : DRW_Attdef {};
bool exercise(const std::filesystem::path& path, bool binary) {
    dxfRW output(path.string().c_str());
    Writer writer(output);
    const auto check = [&](DRW_Entity& value, Entry entry, const char* expected) {
        writer.entity = &value; writer.entry = entry; writer.ok = false;
        if (!output.write(&writer, DRW::AC1027, binary) || !writer.ok) {
            std::cerr << "write failed for " << expected << '\n'; return false;
        }
        dxfRW input(path.string().c_str()); Reader reader;
        if (!input.read(&reader, false) || reader.name != expected || !reader.valid) {
            std::cerr << "read failed for " << expected << ", got " << reader.name << '\n'; return false;
        }
        return true;
    };
    DRW_Point point; point.basePoint.x = 2.;
    CustomPoint customPoint; customPoint.basePoint.x = 2.;
    if (!check(point, Entry::Point, "POINT") || !check(customPoint, Entry::Point, "POINT")) return false;
    DRW_Line line; line.secPoint = {3., 0., 0.};
    CustomLine customLine; customLine.secPoint = line.secPoint;
    if (!check(line, Entry::Line, "LINE") || !check(line, Entry::Point, "LINE")
        || !check(customLine, Entry::Line, "LINE")) return false;
    DRW_3DLine line3d; line3d.secPoint = {3., 0., 4.};
    if (!check(line3d, Entry::Line, "3DLINE")) return false;
    DRW_Ray ray; ray.secPoint = {1., 0., 0.};
    DRW_Xline xline; xline.secPoint = ray.secPoint;
    if (!check(ray, Entry::Line, "RAY") || !check(xline, Entry::Line, "XLINE")
        || !check(xline, Entry::Ray, "XLINE")) return false;
    DRW_Arc arc; arc.radious = 5.; arc.endangle = 1.5707963267948966;
    if (!check(arc, Entry::Circle, "ARC") || !check(arc, Entry::Point, "ARC")) return false;
    DRW_Ellipse ellipse; ellipse.secPoint = {2., 0., 0.}; ellipse.ratio = 0.5;
    ellipse.staparam = 0.; ellipse.endparam = 1.;
    if (!check(ellipse, Entry::Line, "ELLIPSE")) return false;
    DRW_Trace trace; trace.thirdPoint = {2., 2., 0.};
    DRW_Solid solid; solid.thirdPoint = trace.thirdPoint;
    DRW_3Dface face; face.thirdPoint = trace.thirdPoint; face.invisibleflag = 3;
    if (!check(trace, Entry::Line, "TRACE") || !check(solid, Entry::Line, "SOLID")
        || !check(face, Entry::Line, "3DFACE") || !check(solid, Entry::Trace, "SOLID")
        || !check(face, Entry::Trace, "3DFACE")) return false;
    DRW_Text text; text.text = "text"; text.height = 1.;
    CustomText customText; customText.text = text.text; customText.height = text.height;
    DRW_MText mtext; mtext.text = "text"; mtext.height = 1.;
    DRW_RText rtext; rtext.text = "text"; rtext.height = 1.; rtext.m_rTextFlags = 1;
    DRW_ArcAlignedText arctext; arctext.text = "text"; arctext.height = 1.; arctext.m_radius = 5.;
    DRW_Attdef attdef; attdef.text = "text"; attdef.height = 1.; attdef.tag = "TAG"; attdef.prompt = "prompt";
    CustomAttdef customAttdef; customAttdef.text = "text"; customAttdef.height = 1.;
    customAttdef.tag = "TAG"; customAttdef.prompt = "prompt";
    if (!check(text, Entry::Line, "TEXT") || !check(customText, Entry::Text, "TEXT")
        || !check(mtext, Entry::Line, "MTEXT") || !check(rtext, Entry::Text, "RTEXT")
        || !check(rtext, Entry::Point, "RTEXT")
        || !check(arctext, Entry::Text, "ARCALIGNEDTEXT") || !check(attdef, Entry::Text, "ATTDEF")
        || !check(customAttdef, Entry::Attrib, "ATTDEF")) return false;
    DRW_Wipeout wipeout; wipeout.clipPath = {{0., 0., 0.}, {4., 0., 0.}, {0., 3., 0.}};
    if (!check(wipeout, Entry::Line, "WIPEOUT")) return false;
    DRW_Viewport viewport; viewport.pswidth = 9.; viewport.psheight = 10.;
    if (!check(viewport, Entry::Point, "VIEWPORT")) return false;
    DRW_Polyline polyline; polyline.vertlist.push_back(std::make_shared<DRW_Vertex>());
    if (!check(polyline, Entry::Point, "POLYLINE")) return false;
    DRW_Insert insert; insert.name = "TEST_BLOCK";
    if (!check(insert, Entry::Point, "INSERT")) return false;
    auto edge = std::make_shared<DRW_Line>(); edge->secPoint.x = 1.;
    auto loop = std::make_shared<DRW_HatchLoop>(0); loop->numedges = 1; loop->objlist.push_back(edge);
    DRW_Hatch hatch; hatch.name = "SOLID"; hatch.looplist.push_back(loop);
    DRW_MPolygon polygon; polygon.name = "SOLID"; polygon.looplist.push_back(loop);
    if (!check(hatch, Entry::Point, "HATCH") || !check(polygon, Entry::Point, "MPOLYGON")
        || !check(polygon, Entry::Hatch, "MPOLYGON")) return false;
    DRW_DimLinear linearDimension;
    linearDimension.setDef1Point(DRW_Coord(3., 4., 0.));
    if (!check(linearDimension, Entry::Dimension, "DIMLINEAR")) return false;
    CustomLinearDimension customLinear;
    customLinear.setDef1Point(DRW_Coord(3., 4., 0.));
    if (!check(customLinear, Entry::Dimension, "DIMLINEAR")) return false;
    DRW_DimAligned alignedDimension;
    DRW_DimRadial radialDimension;
    DRW_DimLargeRadial largeRadialDimension; largeRadialDimension.jogAngle = .25;
    DRW_DimDiametric diametricDimension;
    DRW_DimAngular angularDimension;
    DRW_DimAngular3p angular3pDimension;
    DRW_DimOrdinate ordinateDimension;
    DRW_DimArc arcDimension;
    if (!check(alignedDimension, Entry::Dimension, "DIMALIGNED")
        || !check(radialDimension, Entry::Dimension, "DIMRADIAL")
        || !check(largeRadialDimension, Entry::Dimension, "DIMLARGERADIAL")
        || !check(diametricDimension, Entry::Dimension, "DIMDIAMETRIC")
        || !check(angularDimension, Entry::Dimension, "DIMANGULAR")
        || !check(angular3pDimension, Entry::Dimension, "DIMANGULAR3P")
        || !check(ordinateDimension, Entry::Dimension, "DIMORDINATE")
        || !check(arcDimension, Entry::Dimension, "DIMARC")) return false;
    // A plain generic DIMENSION is used by existing consumers; it remains
    // admitted while the public subtype tag is checked before any downcast.
    DRW_Dimension genericDimension;
    writer.entity = &genericDimension; writer.entry = Entry::Dimension;
    if (!output.write(&writer, DRW::AC1027, binary) || !writer.ok) return false;
    CustomDimension customDimension;
    writer.entity = &customDimension;
    if (!output.write(&writer, DRW::AC1027, binary) || !writer.ok) return false;
    for (const auto tag : {DRW::DIMALIGNED, DRW::DIMLINEAR, DRW::DIMRADIAL,
                           DRW::DIMDIAMETRIC, DRW::DIMANGULAR, DRW::DIMANGULAR3P,
                           DRW::DIMORDINATE, DRW::DIMARC}) {
        { std::ofstream existing(path, std::ios::binary | std::ios::trunc); existing << "sentinel"; }
        genericDimension.eType = tag;
        writer.entity = &genericDimension; writer.entry = Entry::Dimension;
        if (output.write(&writer, DRW::AC1027, binary) || writer.ok || contents(path) != "sentinel") return false;
        genericDimension.eType = DRW::DIMENSION;
        if (!output.write(&writer, DRW::AC1027, binary) || !writer.ok) return false;
    }
    { std::ofstream existing(path, std::ios::binary | std::ios::trunc); existing << "sentinel"; }
    linearDimension.eType = DRW::DIMALIGNED;
    writer.entity = &linearDimension; writer.entry = Entry::Dimension;
    if (output.write(&writer, DRW::AC1027, binary) || writer.ok || contents(path) != "sentinel") return false;
    linearDimension.eType = DRW::DIMLINEAR;
    if (!check(linearDimension, Entry::Dimension, "DIMLINEAR")) return false;
    { std::ofstream existing(path, std::ios::binary | std::ios::trunc); existing << "sentinel"; }
    largeRadialDimension.eType = DRW::DIMARC;
    writer.entity = &largeRadialDimension; writer.entry = Entry::Dimension;
    if (output.write(&writer, DRW::AC1027, binary) || writer.ok || contents(path) != "sentinel") return false;
    largeRadialDimension.eType = DRW::DIMRADIAL;
    if (!check(largeRadialDimension, Entry::Dimension, "DIMLARGERADIAL")) return false;

    DRW_Image image;
    DRW_Block block;
    DRW_Vertex vertex;
    DRW_Table incompleteTable;
    DRW_Viewport wrongViewport; wrongViewport.eType = DRW::POINT;
    CustomRText customRtext; customRtext.text = "text"; customRtext.m_rTextFlags = 1;
    CustomArcAlignedText customArcText; customArcText.text = "text"; customArcText.m_radius = 5.;
    CustomMPolygon customPolygon;
    DRW_MPolygon wrongPolygon; wrongPolygon.eType = DRW::HATCH;
    DRW_Attrib attrib; attrib.text = "text"; attrib.tag = "TAG";
    DRW_Arc wrongTag; wrongTag.eType = DRW::CIRCLE;
    DRW_Xline wrongXline; wrongXline.eType = DRW::RAY;
    DRW_Solid wrongSolid; wrongSolid.eType = DRW::DXF_TRACE;
    DRW_MText wrongMtext; wrongMtext.eType = DRW::TEXT;
    for (DRW_Entity* invalid : {static_cast<DRW_Entity*>(&image),
                                static_cast<DRW_Entity*>(&attrib),
                                static_cast<DRW_Entity*>(&block),
                                static_cast<DRW_Entity*>(&vertex),
                                static_cast<DRW_Entity*>(&incompleteTable),
                                static_cast<DRW_Entity*>(&wrongViewport),
                                static_cast<DRW_Entity*>(&customRtext),
                                static_cast<DRW_Entity*>(&customArcText),
                                static_cast<DRW_Entity*>(&customPolygon),
                                static_cast<DRW_Entity*>(&wrongPolygon),
                                static_cast<DRW_Entity*>(&wrongTag),
                                static_cast<DRW_Entity*>(&wrongXline),
                                static_cast<DRW_Entity*>(&wrongSolid),
                                static_cast<DRW_Entity*>(&wrongMtext)}) {
        { std::ofstream existing(path, std::ios::binary | std::ios::trunc); existing << "sentinel"; }
        writer.entity = invalid;
        writer.entry = invalid == &wrongTag ? Entry::Circle
            : invalid == &block || invalid == &vertex || invalid == &incompleteTable
                || invalid == &wrongViewport ? Entry::Point
            : invalid == &customRtext || invalid == &customArcText ? Entry::Text
            : invalid == &customPolygon || invalid == &wrongPolygon ? Entry::Hatch
            : Entry::Line;
        if (output.write(&writer, DRW::AC1027, binary) || writer.ok || contents(path) != "sentinel")
            return false;
        if (!check(line, Entry::Line, "LINE")) return false;
    }
    { std::ofstream existing(path, std::ios::binary | std::ios::trunc); existing << "sentinel"; }
    writer.entity = &rtext; writer.entry = Entry::Text;
    if (output.write(&writer, DRW::AC1009, binary) || writer.ok || contents(path) != "sentinel") return false;
    if (!check(line, Entry::Line, "LINE")) return false;
    { std::ofstream existing(path, std::ios::binary | std::ios::trunc); existing << "sentinel"; }
    writer.entity = &line3d; writer.entry = Entry::Line;
    if (output.write(&writer, DRW::AC1014, binary) || writer.ok || contents(path) != "sentinel") return false;
    return check(line, Entry::Line, "LINE");
}
}
int main() {
    const auto path = std::filesystem::temp_directory_path()
        / ("dxf-primitive-dispatch-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code ignored; std::filesystem::remove(path, ignored); } } cleanup{path};
    for (bool binary : {false, true}) if (!exercise(path, binary)) return 1;
    return 0;
}
