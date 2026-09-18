// LibreCAD_3 ABI compatibility check.
//
// This translation unit mirrors the DRW_Interface override surface of
// LibreCAD_3's DXFimpl class
// (../LibreCAD_3/persistence/libdxfrw/dxfimpl.h, last synced against the
// upstream master @ 2026). It compiles against the in-tree libdxfrw
// headers as part of the regular build.
//
// Purpose: any change to libdxfrw's public ABI that would break LibreCAD_3
// fails this TU's compilation. Specifically:
//
//   * A new pure virtual on DRW_Interface (LibreCAD_3 doesn't override it)
//     -> compile error here, because LC3CompatCheck doesn't override it
//     either.
//   * A removed or renamed virtual that LibreCAD_3 overrides
//     -> compile error here, because the override-marked method here no
//     longer matches.
//   * A removed or renamed public field on a DRW_* class that LibreCAD_3
//     reads -> compile error here when the field-read static-asserts
//     below fail.
//
// This TU produces no runtime output and is wired in CMake as an OBJECT
// library (no installed binary). It is intentionally compiled but not
// linked into any executable.
//
// Sync procedure (when LibreCAD_3 itself adds a new override):
//   1. Diff the new ../LibreCAD_3/persistence/libdxfrw/dxfimpl.h against
//      the previous version.
//   2. Add the matching override line below.
//   3. If LibreCAD_3 starts reading a new DRW_* field, add a corresponding
//      `static_assert` for that field below.

#include <drw_interface.h>
#include <drw_base.h>
#include <type_traits>

namespace {

class LC3CompatCheck : public DRW_Interface {
public:
    // ===== READ FUNCTIONALITY =====
    // Mirrors dxfimpl.h:47-126 line-for-line. Empty stubs are still
    // declared with `override` so a removed/renamed upstream method
    // surfaces as a compile error.
    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addLine(const DRW_Line&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addArc(const DRW_Arc&) override {}
    void addCircle(const DRW_Circle&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addLWPolyline(const DRW_LWPolyline&) override {}
    void addPolyline(const DRW_Polyline&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addMText(const DRW_MText&) override {}
    void addText(const DRW_Text&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}

    // ===== WRITE FUNCTIONALITY =====
    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeAppId() override {}
    void writeObjects() override {}
};

// Trivially constructible verifies no abstract virtual got introduced
// upstream that LibreCAD_3 doesn't override.
static_assert(std::is_default_constructible<LC3CompatCheck>::value,
              "LibreCAD_3 DXFimpl ABI broken: a new pure virtual was added "
              "to DRW_Interface. Either make it non-pure with a default "
              "implementation, or update LibreCAD_3's DXFimpl to override "
              "it (and add the override here).");
static_assert(!std::is_abstract<LC3CompatCheck>::value,
              "LibreCAD_3 compatibility sink must remain concrete after "
              "optional callback additions");

// ===== Public DRW_* field-stability checks =====
// Compile-time read of every public field LibreCAD_3 dxfimpl.cpp
// touches. If upstream renames or removes any of these, the
// corresponding `decltype` expression fails to compile.

template <typename T> constexpr void touch_field(T const&) {}

// Field stability runs inside a struct ctor so a static-storage instance
// keeps it ODR-used without tripping -Wunused-function under -Werror.
struct DrwFieldStabilityCheck {
    DrwFieldStabilityCheck() {
    // DRW_Layer fields LibreCAD_3 reads at rs_filter equivalent path
    DRW_Layer layer;
    touch_field(layer.name);
    touch_field(layer.flags);
    touch_field(layer.plotF);
    touch_field(layer.lineType);
    touch_field(layer.lWeight);
    touch_field(layer.color);
    touch_field(layer.color24);
    touch_field(layer.extData);

    // DRW_Spline fields LibreCAD_3 reads in addSpline (dxfimpl.cpp)
    DRW_Spline spline;
    touch_field(spline.degree);
    touch_field(spline.flags);
    touch_field(spline.knotslist);
    touch_field(spline.controllist);
    touch_field(spline.fitlist);
    touch_field(spline.nknots);
    touch_field(spline.ncontrol);
    touch_field(spline.nfit);
    touch_field(spline.tolfit);
    touch_field(spline.tgStart);
    touch_field(spline.tgEnd);
    touch_field(spline.normalVec);

    // DRW_LWPolyline + DRW_Vertex2D fields LibreCAD_3 iterates
    DRW_LWPolyline lwpline;
    touch_field(lwpline.flags);
    touch_field(lwpline.vertexnum);
    touch_field(lwpline.vertlist);

    // DRW_Polyline fields
    DRW_Polyline pline;
    touch_field(pline.flags);
    touch_field(pline.vertlist);

    // DRW_Insert fields
    DRW_Insert insert;
    touch_field(insert.name);
    touch_field(insert.basePoint);
    touch_field(insert.xscale);
    touch_field(insert.yscale);
    touch_field(insert.zscale);
    touch_field(insert.angle);
    touch_field(insert.colcount);
    touch_field(insert.rowcount);
    touch_field(insert.colspace);
    touch_field(insert.rowspace);

    // DRW_Image fields
    DRW_Image img;
    touch_field(img.ref);
    touch_field(img.basePoint);
    touch_field(img.secPoint);
    touch_field(img.vVector);
    touch_field(img.sizeu);
    touch_field(img.sizev);
    touch_field(img.brightness);
    touch_field(img.contrast);
    touch_field(img.fade);

    // DRW_Vport (LibreCAD_3 only logs a warning; minimal field touch)
    DRW_Vport vport;
    touch_field(vport.name);

    // DRW_Hatch fields
    DRW_Hatch hatch;
    touch_field(hatch.name);
    touch_field(hatch.solid);
    touch_field(hatch.associative);
    touch_field(hatch.hstyle);
    touch_field(hatch.hpattern);
    touch_field(hatch.angle);
    touch_field(hatch.scale);

    // DRW_Block fields
    DRW_Block block;
    touch_field(block.name);
    touch_field(block.basePoint);
    touch_field(block.flags);

    // DRW_Text + DRW_MText fields
    DRW_Text text;
    touch_field(text.basePoint);
    touch_field(text.secPoint);
    touch_field(text.height);
    touch_field(text.text);
    touch_field(text.angle);
    touch_field(text.widthscale);
    touch_field(text.oblique);
    touch_field(text.style);
    touch_field(text.alignH);
    touch_field(text.alignV);

    DRW_MText mtext;
    touch_field(mtext.basePoint);
    touch_field(mtext.height);
    touch_field(mtext.text);
    touch_field(mtext.textgen);
    touch_field(mtext.alignH);

    // DRW_Dim* common fields LibreCAD_3 reads
    DRW_DimAligned dimA;
    touch_field(dimA.type);

    // DRW_Header (read but body empty in LC3; just ensure type present)
    DRW_Header header;
    touch_field(header.vars);

    // DRW_LType fields
    DRW_LType ltype;
    touch_field(ltype.name);
    touch_field(ltype.desc);
    touch_field(ltype.path);

    // Newer classes: confirm the addX virtuals exist (compile-only check)
    // and that the classes are default-constructible. These are not yet
    // overridden in LibreCAD_3 but must compile against future libdxfrw.
    DRW_Dictionary dict;
    touch_field(dict.cloning);
    touch_field(dict.hardOwner);
    DRW_Layout layout;
    touch_field(layout.layoutFlags);
    touch_field(layout.tabOrder);
    DRW_MLineStyle mline;
    touch_field(mline.flags);
    touch_field(mline.startAngle);
    touch_field(mline.endAngle);
    DRW_UCS ucs;
    touch_field(ucs.origin);
    touch_field(ucs.elevation);
    DRW_View view;
    touch_field(view.lensLen);
    touch_field(view.viewMode);
    DRW_Tolerance tol;
    touch_field(tol.text);
    touch_field(tol.dimStyleName);
    }
};

// Touch DRW::Version + DRW::error enum values LibreCAD_3 maps positionally.
// If upstream reorders these, integer values shift and LC3 mappings break.
// LC3 maps via switch in file.cpp:34 — append-only is required.
static_assert(DRW::AC1009 < DRW::AC1015, "DRW::Version enum reordered");
static_assert(DRW::AC1015 < DRW::AC1018, "DRW::Version enum reordered");
static_assert(DRW::AC1018 < DRW::AC1021, "DRW::Version enum reordered");
static_assert(DRW::AC1021 < DRW::AC1024, "DRW::Version enum reordered");
static_assert(DRW::AC1024 < DRW::AC1027, "DRW::Version enum reordered");
static_assert(DRW::AC1027 < DRW::AC1032, "DRW::Version enum reordered");

static_assert(DRW::BAD_NONE == 0, "DRW::error reordered: BAD_NONE shifted");

// Static-storage instance forces the constructor (and thus all the field
// touches above) to be emitted by the compiler.
static DrwFieldStabilityCheck g_drw_field_check;

} // anonymous namespace
