#include <string>
#include <emscripten/bind.h>
#include "intern/drw_dbg.h"
#include "intern/dxfwriter.h"
#include "drw_base.h"
#include "drw_data.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "libdxfrw.h"
#include "libdwgr.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(dxf_writer) {
  class_<dxfWriter>("DxfWriter")
    .function("writeUtf8String", &dxfWriter::writeUtf8String)
    .function("writeUtf8Caps", &dxfWriter::writeUtf8Caps)
    .function("fromUtf8String", &dxfWriter::fromUtf8String)
    .function("setVersion", &dxfWriter::setVersion)
    .function("setCodePage", &dxfWriter::setCodePage)
    .function("getCodePage", &dxfWriter::getCodePage)
    .function("writeString", &dxfWriter::writeString)
    .function("writeInt16", &dxfWriter::writeInt16)
    .function("writeInt32", &dxfWriter::writeInt32)
    .function("writeInt64", &dxfWriter::writeInt64)
    .function("writeDouble", &dxfWriter::writeDouble)
    .function("writeBool", &dxfWriter::writeBool);
}

EMSCRIPTEN_BINDINGS(DRW_base) {
  enum_<DRW::Version>("DRW_Version")
    .value("UNKNOWNV", DRW::UNKNOWNV)
    .value("MC00", DRW::MC00)
    .value("AC12", DRW::AC12)
    .value("AC14", DRW::AC14)
    .value("AC150", DRW::AC150)
    .value("AC210", DRW::AC210)
    .value("AC1002", DRW::AC1002)
    .value("AC1003", DRW::AC1003)
    .value("AC1004", DRW::AC1004)
    .value("AC1006", DRW::AC1006)
    .value("AC1009", DRW::AC1009)
    .value("AC1012", DRW::AC1012)
    .value("AC1014", DRW::AC1014)
    .value("AC1015", DRW::AC1015)
    .value("AC1018", DRW::AC1018)
    .value("AC1021", DRW::AC1021)
    .value("AC1024", DRW::AC1024)
    .value("AC1027", DRW::AC1027)
    .value("AC1032", DRW::AC1032);

  enum_<DRW::error>("DWR_Error")
    .value("BAD_NONE", DRW::BAD_NONE)
    .value("BAD_UNKNOWN", DRW::BAD_UNKNOWN)
    .value("BAD_OPEN", DRW::BAD_OPEN)
    .value("BAD_VERSION", DRW::BAD_VERSION)
    .value("BAD_READ_METADATA", DRW::BAD_READ_METADATA)
    .value("BAD_READ_FILE_HEADER", DRW::BAD_READ_FILE_HEADER)
    .value("BAD_READ_HEADER", DRW::BAD_READ_HEADER)
    .value("BAD_READ_HANDLES", DRW::BAD_READ_HANDLES)
    .value("BAD_READ_CLASSES", DRW::BAD_READ_CLASSES)
    .value("BAD_READ_TABLES", DRW::BAD_READ_TABLES)
    .value("BAD_READ_BLOCKS", DRW::BAD_READ_BLOCKS)
    .value("BAD_READ_ENTITIES", DRW::BAD_READ_ENTITIES)
    .value("BAD_READ_OBJECTS", DRW::BAD_READ_OBJECTS)
    .value("BAD_READ_SECTION", DRW::BAD_READ_SECTION)
    .value("BAD_CODE_PARSED", DRW::BAD_CODE_PARSED);

  enum_<DRW::DebugLevel>("DRW_DebugLevel")
    .value("None", DRW::DebugLevel::None)
    .value("Debug", DRW::DebugLevel::Debug);

  class_<DRW::DebugPrinter>("DebugPrinter")
    .constructor<>()
    .function("printS", &DRW::DebugPrinter::printS)
    .function("printI", &DRW::DebugPrinter::printI)
    .function("printUI", &DRW::DebugPrinter::printUI)
    .function("printD", &DRW::DebugPrinter::printD)
    .function("printH", &DRW::DebugPrinter::printH)
    .function("printB", &DRW::DebugPrinter::printB)
    .function("printHL", &DRW::DebugPrinter::printHL)
    .function("printPT", &DRW::DebugPrinter::printPT);

  enum_<DRW::ColorCodes>("DRW_ColorCodes")
    .value("ColorByLayer", DRW::ColorByLayer)
    .value("ColorByBlock", DRW::ColorByBlock);

  enum_<DRW::Space>("DRW_Space")
    .value("ModelSpace", DRW::ModelSpace)
    .value("PaperSpace", DRW::PaperSpace);

  enum_<DRW::HandleCodes>("DRW_HandleCodes")
    .value("NoHandle", DRW::NoHandle);

  enum_<DRW::ShadowMode>("DRW_ShadowMode")
    .value("CastAndReceieveShadows", DRW::CastAndReceieveShadows)
    .value("CastShadows", DRW::CastShadows)
    .value("ReceiveShadows", DRW::ReceiveShadows)
    .value("IgnoreShadows", DRW::IgnoreShadows);

  enum_<DRW::MaterialCodes>("DRW_MaterialCodes")
    .value("MaterialByLayer", DRW::MaterialByLayer);

  enum_<DRW::PlotStyleCodes>("DRW_PlotStyleCodes")
    .value("DefaultPlotStyle", DRW::DefaultPlotStyle);

  enum_<DRW::TransparencyCodes>("DRW_TransparencyCodes")
    .value("Opaque", DRW::Opaque)
    .value("Transparent", DRW::Transparent);

  class_<DRW_Coord>("DRW_Coord")
    .constructor<>()
    .constructor<double, double, double>()
    .function("unitize", &DRW_Coord::unitize)
    .property("x", &DRW_Coord::x)
    .property("y", &DRW_Coord::y)
    .property("z", &DRW_Coord::z);

  class_<DRW_Vertex2D>("DRW_Vertex2D")
    .constructor<>()
    .constructor<double, double, double>()
    .property("x", &DRW_Vertex2D::x)
    .property("y", &DRW_Vertex2D::y)
    .property("startWidth", &DRW_Vertex2D::stawidth)
    .property("endWidth", &DRW_Vertex2D::endwidth)
    .property("bulge", &DRW_Vertex2D::bulge);

  enum_<DRW_Variant::TYPE>("DRW_VariantType")
    .value("STRING", DRW_Variant::STRING)
    .value("INTEGER", DRW_Variant::INTEGER)
    .value("DOUBLE", DRW_Variant::DOUBLE)
    .value("COORD", DRW_Variant::COORD)
    .value("INVALID", DRW_Variant::INVALID);

  class_<DRW_Variant>("DRW_Variant")
    .constructor<>()
    .function("addString", &DRW_Variant::addString)
    .function("addInt", &DRW_Variant::addInt)
    .function("addDouble", &DRW_Variant::addDouble)
    .function("addCoord", &DRW_Variant::addCoord)
    .function("setCoordX", &DRW_Variant::setCoordX)
    .function("setCoordY", &DRW_Variant::setCoordY)
    .function("setCoordZ", &DRW_Variant::setCoordZ)
    .function("getString", &DRW_Variant::getString)
    .function("getInt", &DRW_Variant::getInt)
    .function("getDouble", &DRW_Variant::getDouble)
    .function("getCoord", &DRW_Variant::getCoord, allow_raw_pointer<DRW_Coord*>())
    .function("type", &DRW_Variant::type)
    .function("code", &DRW_Variant::code);

  class_<dwgHandle>("DRW_Handle")
    .constructor<>()
    .property("code", &dwgHandle::code)
    .property("size", &dwgHandle::size)
    .property("ref", &dwgHandle::ref);

  enum_<DRW_LW_Conv::lineWidth>("DRW_LineWidth")
    .value("width00", DRW_LW_Conv::width00)
    .value("width01", DRW_LW_Conv::width01)
    .value("width02", DRW_LW_Conv::width02)
    .value("width03", DRW_LW_Conv::width03)
    .value("width04", DRW_LW_Conv::width04)
    .value("width05", DRW_LW_Conv::width05)
    .value("width06", DRW_LW_Conv::width06)
    .value("width07", DRW_LW_Conv::width07)
    .value("width08", DRW_LW_Conv::width08)
    .value("width09", DRW_LW_Conv::width09)
    .value("width10", DRW_LW_Conv::width10)
    .value("width11", DRW_LW_Conv::width11)
    .value("width12", DRW_LW_Conv::width12)
    .value("width13", DRW_LW_Conv::width13)
    .value("width14", DRW_LW_Conv::width14)
    .value("width15", DRW_LW_Conv::width15)
    .value("width16", DRW_LW_Conv::width16)
    .value("width17", DRW_LW_Conv::width17)
    .value("width18", DRW_LW_Conv::width18)
    .value("width19", DRW_LW_Conv::width19)
    .value("width20", DRW_LW_Conv::width20)
    .value("width21", DRW_LW_Conv::width21)
    .value("width22", DRW_LW_Conv::width22)
    .value("width23", DRW_LW_Conv::width23)
    .value("widthByLayer", DRW_LW_Conv::widthByLayer)
    .value("widthByBlock", DRW_LW_Conv::widthByBlock)
    .value("widthDefault", DRW_LW_Conv::widthDefault);

  class_<DRW_LW_Conv>("DRW_LW_Conv")
    .class_function("lineWidth2dxfInt", &DRW_LW_Conv::lineWidth2dxfInt)
    .class_function("lineWidth2dwgInt", &DRW_LW_Conv::lineWidth2dwgInt)
    .class_function("dxfInt2lineWidth", &DRW_LW_Conv::dxfInt2lineWidth)
    .class_function("dwgInt2lineWidth", &DRW_LW_Conv::dwgInt2lineWidth);
}

EMSCRIPTEN_BINDINGS(DRW_dbg) {
  enum_<DRW_dbg::Level>("DRW_Dbg_Level")
    .value("None", DRW_dbg::Level::None)
    .value("Debug", DRW_dbg::Level::Debug);

  class_<DRW_dbg>("DRW_Dbg")
    .class_function("getInstance", &DRW_dbg::getInstance, allow_raw_pointer<DRW_dbg*>())
    .function("setLevel", &DRW_dbg::setLevel)
    .function("getLevel", &DRW_dbg::getLevel)
    .function("printString", select_overload<void(const std::string&)>(&DRW_dbg::print))
    .function("printInt", select_overload<void(int)>(&DRW_dbg::print))
    .function("printUnsignedInt", select_overload<void(unsigned int)>(&DRW_dbg::print))
    .function("printLongLongInt", select_overload<void(long long int)>(&DRW_dbg::print))
    .function("printLongUnsignedInt", select_overload<void(long unsigned int)>(&DRW_dbg::print))
    .function("printLongLongUnsignedInt", select_overload<void(long long unsigned int)>(&DRW_dbg::print))
    .function("printDouble", select_overload<void(double)>(&DRW_dbg::print))
    .function("printH", &DRW_dbg::printH)
    .function("printB", &DRW_dbg::printB)
    .function("printHL", &DRW_dbg::printHL)
    .function("printPT", &DRW_dbg::printPT);
}

EMSCRIPTEN_BINDINGS(DRW_Header) {
  register_vector<std::string>("DRW_StringList");
  // register_map<std::string, DRW_Variant*>("DRW_VarMap");
  // register_pointer<std::unique_ptr<dxfWriter>>("DRW_UniquePtr_dxfWriter");

  class_<DRW_Header>("DRW_Header")
    .constructor<>()
    .function("addDouble", &DRW_Header::addDouble)
    .function("addInt", &DRW_Header::addInt)
    .function("addStr", &DRW_Header::addStr)
    .function("addCoord", &DRW_Header::addCoord)
    .function("getComments", &DRW_Header::getComments)
    // .function("write", &DRW_Header::write)
    .function("addComment", &DRW_Header::addComment)
    .function("getVarNames", &DRW_Header::getVarNames)
    .function("getVar", &DRW_Header::getVar, allow_raw_pointer<DRW_Variant*>())
    .function("clearVars", &DRW_Header::clearVars);
    //.property("vars", &DRW_Header::vars);
}

EMSCRIPTEN_BINDINGS(DRW_Objects) {
  register_vector<DRW_Variant*>("DRW_VariantList");
  register_vector<double>("DRW_DoubleList");
  // register_map<std::string, std::string>("map<string, string>");

  enum_<DRW::TTYPE>("TTYPE")
    .value("UNKNOWNT", DRW::UNKNOWNT)
    .value("LTYPE", DRW::LTYPE)
    .value("LAYER", DRW::LAYER)
    .value("STYLE", DRW::STYLE)
    .value("DIMSTYLE", DRW::DIMSTYLE)
    .value("VPORT", DRW::VPORT)
    .value("BLOCK_RECORD", DRW::BLOCK_RECORD)
    .value("APPID", DRW::APPID)
    .value("IMAGEDEF", DRW::IMAGEDEF);

  class_<DRW_TableEntry>("DRW_TableEntry")
    .property("tType", &DRW_TableEntry::tType)
    .property("handle", &DRW_TableEntry::handle)
    .property("parentHandle", &DRW_TableEntry::parentHandle)
    .property("name", &DRW_TableEntry::name)
    .property("flags", &DRW_TableEntry::flags)
    .property("extData", &DRW_TableEntry::extData, return_value_policy::reference());

  class_<DRW_Dimstyle, base<DRW_TableEntry>>("DRW_Dimstyle")
    .constructor<>()
    .function("reset", &DRW_Dimstyle::reset)
    .property("dimpost", &DRW_Dimstyle::dimpost)
    .property("dimapost", &DRW_Dimstyle::dimapost)
    .property("dimblk", &DRW_Dimstyle::dimblk)
    .property("dimblk1", &DRW_Dimstyle::dimblk1)
    .property("dimblk2", &DRW_Dimstyle::dimblk2)
    .property("dimscale", &DRW_Dimstyle::dimscale)
    .property("dimasz", &DRW_Dimstyle::dimasz)
    .property("dimexo", &DRW_Dimstyle::dimexo)
    .property("dimdli", &DRW_Dimstyle::dimdli)
    .property("dimexe", &DRW_Dimstyle::dimexe)
    .property("dimrnd", &DRW_Dimstyle::dimrnd)
    .property("dimdle", &DRW_Dimstyle::dimdle)
    .property("dimtp", &DRW_Dimstyle::dimtp)
    .property("dimtm", &DRW_Dimstyle::dimtm)
    .property("dimfxl", &DRW_Dimstyle::dimfxl)
    .property("dimtxt", &DRW_Dimstyle::dimtxt)
    .property("dimcen", &DRW_Dimstyle::dimcen)
    .property("dimtsz", &DRW_Dimstyle::dimtsz)
    .property("dimaltf", &DRW_Dimstyle::dimaltf)
    .property("dimlfac", &DRW_Dimstyle::dimlfac)
    .property("dimtvp", &DRW_Dimstyle::dimtvp)
    .property("dimtfac", &DRW_Dimstyle::dimtfac)
    .property("dimgap", &DRW_Dimstyle::dimgap)
    .property("dimaltrnd", &DRW_Dimstyle::dimaltrnd)
    .property("dimtol", &DRW_Dimstyle::dimtol)
    .property("dimlim", &DRW_Dimstyle::dimlim)
    .property("dimtih", &DRW_Dimstyle::dimtih)
    .property("dimtoh", &DRW_Dimstyle::dimtoh)
    .property("dimse1", &DRW_Dimstyle::dimse1)
    .property("dimse2", &DRW_Dimstyle::dimse2)
    .property("dimtad", &DRW_Dimstyle::dimtad)
    .property("dimzin", &DRW_Dimstyle::dimzin)
    .property("dimazin", &DRW_Dimstyle::dimazin)
    .property("dimalt", &DRW_Dimstyle::dimalt)
    .property("dimaltd", &DRW_Dimstyle::dimaltd)
    .property("dimtofl", &DRW_Dimstyle::dimtofl)
    .property("dimsah", &DRW_Dimstyle::dimsah)
    .property("dimtix", &DRW_Dimstyle::dimtix)
    .property("dimsoxd", &DRW_Dimstyle::dimsoxd)
    .property("dimclrd", &DRW_Dimstyle::dimclrd)
    .property("dimclre", &DRW_Dimstyle::dimclre)
    .property("dimclrt", &DRW_Dimstyle::dimclrt)
    .property("dimadec", &DRW_Dimstyle::dimadec)
    .property("dimunit", &DRW_Dimstyle::dimunit)
    .property("dimdec", &DRW_Dimstyle::dimdec)
    .property("dimtdec", &DRW_Dimstyle::dimtdec)
    .property("dimaltu", &DRW_Dimstyle::dimaltu)
    .property("dimalttd", &DRW_Dimstyle::dimalttd)
    .property("dimaunit", &DRW_Dimstyle::dimaunit)
    .property("dimfrac", &DRW_Dimstyle::dimfrac)
    .property("dimlunit", &DRW_Dimstyle::dimlunit)
    .property("dimdsep", &DRW_Dimstyle::dimdsep)
    .property("dimtmove", &DRW_Dimstyle::dimtmove)
    .property("dimjust", &DRW_Dimstyle::dimjust)
    .property("dimsd1", &DRW_Dimstyle::dimsd1)
    .property("dimsd2", &DRW_Dimstyle::dimsd2)
    .property("dimtolj", &DRW_Dimstyle::dimtolj)
    .property("dimtzin", &DRW_Dimstyle::dimtzin)
    .property("dimaltz", &DRW_Dimstyle::dimaltz)
    .property("dimaltttz", &DRW_Dimstyle::dimaltttz)
    .property("dimfit", &DRW_Dimstyle::dimfit)
    .property("dimupt", &DRW_Dimstyle::dimupt)
    .property("dimatfit", &DRW_Dimstyle::dimatfit)
    .property("dimfxlon", &DRW_Dimstyle::dimfxlon)
    .property("dimtxsty", &DRW_Dimstyle::dimtxsty)
    .property("dimldrblk", &DRW_Dimstyle::dimldrblk)
    .property("dimlwd", &DRW_Dimstyle::dimlwd)
    .property("dimlwe", &DRW_Dimstyle::dimlwe);

  class_<DRW_LType, base<DRW_TableEntry>>("DRW_LType")
    .constructor<>()
    .function("reset", &DRW_LType::reset)
    .property("desc", &DRW_LType::desc)
    .property("size", &DRW_LType::size)
    .property("length", &DRW_LType::length)
    .property("path", &DRW_LType::path, return_value_policy::reference());

  class_<DRW_Layer, base<DRW_TableEntry>>("DRW_Layer")
    .constructor<>()
    .function("reset", &DRW_Layer::reset)
    .property("lineType", &DRW_Layer::lineType)
    .property("color", &DRW_Layer::color)
    .property("color24", &DRW_Layer::color24)
    .property("plotF", &DRW_Layer::plotF)
    .property("lWeight", &DRW_Layer::lWeight)
    .property("handlePlotS", &DRW_Layer::handlePlotS, return_value_policy::reference())
    .property("handleMaterialS", &DRW_Layer::handleMaterialS, return_value_policy::reference())
    .property("lTypeH", &DRW_Layer::lTypeH, return_value_policy::reference());

  class_<DRW_Block_Record, base<DRW_TableEntry>>("DRW_Block_Record")
    .constructor<>()
    .function("reset", &DRW_Block_Record::reset)
    .property("insUnits", &DRW_Block_Record::insUnits)
    .property("basePoint", &DRW_Block_Record::basePoint, return_value_policy::reference());

  class_<DRW_Textstyle, base<DRW_TableEntry>>("DRW_Textstyle")  
    .constructor()
    .function("reset", &DRW_Textstyle::reset)
    .property("height", &DRW_Textstyle::height)
    .property("width", &DRW_Textstyle::width)
    .property("oblique", &DRW_Textstyle::oblique)
    .property("genFlag", &DRW_Textstyle::genFlag)
    .property("lastHeight", &DRW_Textstyle::lastHeight)
    .property("font", &DRW_Textstyle::font)
    .property("bigFont", &DRW_Textstyle::bigFont)
    .property("fontFamily", &DRW_Textstyle::fontFamily);

  class_<DRW_Vport, base<DRW_TableEntry>>("DRW_Vport")
    .constructor<>()
    .function("reset", &DRW_Vport::reset)
    .property("lowerLeft", &DRW_Vport::lowerLeft, return_value_policy::reference())
    .property("upperRight", &DRW_Vport::UpperRight, return_value_policy::reference())
    .property("center", &DRW_Vport::center, return_value_policy::reference())
    .property("snapBase", &DRW_Vport::snapBase, return_value_policy::reference())
    .property("snapSpacing", &DRW_Vport::snapSpacing, return_value_policy::reference())
    .property("gridSpacing", &DRW_Vport::gridSpacing, return_value_policy::reference())
    .property("viewDir", &DRW_Vport::viewDir, return_value_policy::reference())
    .property("viewTarget", &DRW_Vport::viewTarget, return_value_policy::reference())
    .property("height", &DRW_Vport::height)
    .property("ratio", &DRW_Vport::ratio)
    .property("lensHeight", &DRW_Vport::lensHeight)
    .property("frontClip", &DRW_Vport::frontClip)
    .property("backClip", &DRW_Vport::backClip)
    .property("snapAngle", &DRW_Vport::snapAngle)
    .property("twistAngle", &DRW_Vport::twistAngle)
    .property("viewMode", &DRW_Vport::viewMode)
    .property("circleZoom", &DRW_Vport::circleZoom)
    .property("fastZoom", &DRW_Vport::fastZoom)
    .property("ucsIcon", &DRW_Vport::ucsIcon)
    .property("snap", &DRW_Vport::snap)
    .property("grid", &DRW_Vport::grid)
    .property("snapStyle", &DRW_Vport::snapStyle)
    .property("snapIsopair", &DRW_Vport::snapIsopair)
    .property("gridBehavior", &DRW_Vport::gridBehavior);

  class_<DRW_ImageDef, base<DRW_TableEntry>>("DRW_ImageDef")
    .constructor<>()
    .function("reset", &DRW_ImageDef::reset)
    .property("name", &DRW_ImageDef::name)
    .property("imgVersion", &DRW_ImageDef::imgVersion)
    .property("u", &DRW_ImageDef::u)
    .property("v", &DRW_ImageDef::v)
    .property("up", &DRW_ImageDef::up)
    .property("vp", &DRW_ImageDef::vp)
    .property("loaded", &DRW_ImageDef::loaded)
    .property("resolution", &DRW_ImageDef::resolution);
    //.property("reactors", &DRW_ImageDef::reactors, return_value_policy::reference());

  class_<DRW_AppId, base<DRW_TableEntry>>("DRW_AppId")
    .constructor<>()
    .function("reset", &DRW_AppId::reset);
}

EMSCRIPTEN_BINDINGS(DRW_entities) {
  value_object<std::shared_ptr<DRW_Variant>>("DRW_VariantPtr");
  value_object<std::shared_ptr<DRW_Vertex>>("DRW_VertexPtr");
  value_object<std::shared_ptr<DRW_Vertex2D>>("DRW_Vertex2DPtr");
  value_object<std::shared_ptr<DRW_HatchLoop>>("DRW_HatchLoopPtr");
  register_vector<DRW_Vertex*>("DRW_VertexList");
  register_vector<DRW_Vertex2D*>("DRW_Vertex2DList");
  register_vector<DRW_Coord*>("DRW_CoordList");
  register_vector<DRW_HatchLoop*>("DRW_HatchLoopList");
  register_vector<DRW_HatchPattenLine>("DRW_HatchPattenLineList");

  enum_<DRW::ETYPE>("DRW_ETYPE")
    .value("E3DFACE", DRW::E3DFACE)
    .value("ARC", DRW::ARC)
    .value("BLOCK", DRW::BLOCK)
    .value("CIRCLE", DRW::CIRCLE)
    .value("DIMENSION", DRW::DIMENSION)
    .value("DIMALIGNED", DRW::DIMALIGNED)
    .value("DIMLINEAR", DRW::DIMLINEAR)
    .value("DIMRADIAL", DRW::DIMRADIAL)
    .value("DIMDIAMETRIC", DRW::DIMDIAMETRIC)
    .value("DIMANGULAR", DRW::DIMANGULAR)
    .value("DIMANGULAR3P", DRW::DIMANGULAR3P)
    .value("DIMORDINATE", DRW::DIMORDINATE)
    .value("ELLIPSE", DRW::ELLIPSE)
    .value("HATCH", DRW::HATCH)
    .value("IMAGE", DRW::IMAGE)
    .value("INSERT", DRW::INSERT)
    .value("LEADER", DRW::LEADER)
    .value("LINE", DRW::LINE)
    .value("LWPOLYLINE", DRW::LWPOLYLINE)
    .value("MTEXT", DRW::MTEXT)
    .value("POINT", DRW::POINT)
    .value("POLYLINE", DRW::POLYLINE)
    .value("RAY", DRW::RAY)
    .value("SOLID", DRW::SOLID)
    .value("SPLINE", DRW::SPLINE)
    .value("TEXT", DRW::TEXT)
    .value("TRACE", DRW::TRACE)
    .value("UNDERLAY", DRW::UNDERLAY)
    .value("VERTEX", DRW::VERTEX)
    .value("VIEWPORT", DRW::VIEWPORT)
    .value("XLINE", DRW::XLINE)
    .value("UNKNOWN", DRW::UNKNOWN);

  class_<DRW_Entity>("DRW_Entity")
    .function("reset", &DRW_Entity::reset)
    .function("applyExtrusion", &DRW_Entity::applyExtrusion)
    .property("eType", &DRW_Entity::eType)
    .property("handle", &DRW_Entity::handle)
    .property("parentHandle", &DRW_Entity::parentHandle)
    .property("space", &DRW_Entity::space)
    .property("layer", &DRW_Entity::layer)
    .property("lineType", &DRW_Entity::lineType)
    .property("material", &DRW_Entity::material)
    .property("color", &DRW_Entity::color)
    .property("lWeight", &DRW_Entity::lWeight)
    .property("ltypeScale", &DRW_Entity::ltypeScale)
    .property("visible", &DRW_Entity::visible)
    .property("numProxyGraph", &DRW_Entity::numProxyGraph)
    .property("proxyGraphics", &DRW_Entity::proxyGraphics, return_value_policy::reference())
    .property("color24", &DRW_Entity::color24)
    .property("colorName", &DRW_Entity::colorName, return_value_policy::reference())
    .property("transparency", &DRW_Entity::transparency)
    .property("plotStyle", &DRW_Entity::plotStyle)
    .property("shadow", &DRW_Entity::shadow)
    .property("haveExtrusion", &DRW_Entity::haveExtrusion);
    // .property("extData", &DRW_Entity::extData);

  class_<DRW_Point, emscripten::base<DRW_Entity>>("DRW_Point")
    .constructor<>()
    .function("applyExtrusion", &DRW_Point::applyExtrusion)
    .property("basePoint", &DRW_Point::basePoint, return_value_policy::reference())
    .property("thickness", &DRW_Point::thickness)
    .property("extPoint", &DRW_Point::extPoint, return_value_policy::reference());

  class_<DRW_Line, base<DRW_Point>>("DRW_Line")
    .constructor<>()
    .function("applyExtrusion", &DRW_Line::applyExtrusion)
    .property("secPoint", &DRW_Line::secPoint, return_value_policy::reference());

  class_<DRW_Ray, base<DRW_Line>>("DRW_Ray")
    .constructor<>();

  class_<DRW_Xline, base<DRW_Ray>>("DRW_Xline")
    .constructor<>();

  class_<DRW_Circle, base<DRW_Point>>("DRW_Circle")
    .constructor<>()
    .function("applyExtrusion", &DRW_Circle::applyExtrusion)
    .property("radius", &DRW_Circle::radius);

  class_<DRW_Arc, base<DRW_Circle>>("DRW_Arc")
    .constructor<>()
    .function("applyExtrusion", &DRW_Arc::applyExtrusion)
    .function("center", &DRW_Arc::center)
    .function("thick", &DRW_Arc::thick)
    .function("extrusion", &DRW_Arc::extrusion)
    .property("startAngle", &DRW_Arc::staangle)
    .property("endAngle", &DRW_Arc::endangle)
    .property("isccw", &DRW_Arc::isccw);

  class_<DRW_Ellipse, base<DRW_Line>>("DRW_Ellipse")
    .constructor<>()
    .function("toPolyline", &DRW_Ellipse::toPolyline, allow_raw_pointer<DRW_Polyline*>())
    .function("applyExtrusion", &DRW_Ellipse::applyExtrusion)
    .property("ratio", &DRW_Ellipse::ratio)
    .property("startAngle", &DRW_Ellipse::staparam)
    .property("endAngle", &DRW_Ellipse::endparam)
    .property("isCounterClockwise", &DRW_Ellipse::isccw);

  class_<DRW_Trace, base<DRW_Line>>("DRW_Trace")
    .constructor<>()
    .function("applyExtrusion", &DRW_Trace::applyExtrusion)
    .property("thirdPoint", &DRW_Trace::thirdPoint, return_value_policy::reference())
    .property("fourPoint", &DRW_Trace::fourPoint, return_value_policy::reference());

  class_<DRW_Solid, base<DRW_Trace>>("DRW_Solid")
    .constructor<>()
    .function("firstCorner", &DRW_Solid::firstCorner)
    .function("secondCorner", &DRW_Solid::secondCorner)
    .function("thirdCorner", &DRW_Solid::thirdCorner)
    .function("fourthCorner", &DRW_Solid::fourthCorner)
    .function("thick", &DRW_Solid::thick)
    .function("elevation", &DRW_Solid::elevation)
    .function("extrusion", &DRW_Solid::extrusion);

  enum_<DRW_3Dface::InvisibleEdgeFlags>("InvisibleEdgeFlags")
    .value("NoEdge", DRW_3Dface::NoEdge)
    .value("FirstEdge", DRW_3Dface::FirstEdge)
    .value("SecondEdge", DRW_3Dface::SecodEdge)
    .value("ThirdEdge", DRW_3Dface::ThirdEdge)
    .value("FourthEdge", DRW_3Dface::FourthEdge)
    .value("AllEdges", DRW_3Dface::AllEdges);

  class_<DRW_3Dface, base<DRW_Trace>>("DRW_3Dface")
    .constructor<>()
    .function("firstCorner", &DRW_3Dface::firstCorner)
    .function("secondCorner", &DRW_3Dface::secondCorner)
    .function("thirdCorner", &DRW_3Dface::thirdCorner)
    .function("fourthCorner", &DRW_3Dface::fourthCorner)
    .function("edgeFlags", &DRW_3Dface::edgeFlags)
    .function("applyExtrusion", &DRW_3Dface::applyExtrusion)
    .property("invisibleflag", &DRW_3Dface::invisibleflag);

  class_<DRW_Block, base<DRW_Point>>("DRW_Block")
    .constructor<>()
    .function("applyExtrusion", &DRW_Block::applyExtrusion)
    .property("name", &DRW_Block::name)
    .property("flags", &DRW_Block::flags);

  class_<DRW_Insert, base<DRW_Point>>("DRW_Insert")
    .constructor<>()
    .function("applyExtrusion", &DRW_Insert::applyExtrusion)
    .property("name", &DRW_Insert::name)
    .property("xScale", &DRW_Insert::xscale)
    .property("yScale", &DRW_Insert::yscale)
    .property("zScale", &DRW_Insert::zscale)
    .property("angle", &DRW_Insert::angle)
    .property("colCount", &DRW_Insert::colcount)
    .property("rowCount", &DRW_Insert::rowcount)
    .property("colSpace", &DRW_Insert::colspace)
    .property("rowSpace", &DRW_Insert::rowspace)
    .property("blockRecH", &DRW_Insert::blockRecH)
    .property("seqendH", &DRW_Insert::seqendH);

  class_<DRW_LWPolyline, base<DRW_Entity>>("DRW_LWPolyline")
    .constructor<>()
    .function("applyExtrusion", &DRW_LWPolyline::applyExtrusion)
    .function("addVertex", &DRW_LWPolyline::addVertex)
    .function("appendVertex", &DRW_LWPolyline::appendVertex)
    .function("getVertexList", &DRW_LWPolyline::getVertexList)
    .property("vertexNum", &DRW_LWPolyline::vertexnum)
    .property("flags", &DRW_LWPolyline::flags)
    .property("width", &DRW_LWPolyline::width)
    .property("elevation", &DRW_LWPolyline::elevation)
    .property("thickness", &DRW_LWPolyline::thickness)
    .property("extPoint", &DRW_LWPolyline::extPoint, return_value_policy::reference())
    .property("vertex", &DRW_LWPolyline::vertex);
    // .property("vertlist", &DRW_LWPolyline::vertlist);

  enum_<DRW_Text::VAlign>("VAlign")
    .value("VBaseLine", DRW_Text::VBaseLine)
    .value("VBottom", DRW_Text::VBottom)
    .value("VMiddle", DRW_Text::VMiddle)
    .value("VTop", DRW_Text::VTop);

  enum_<DRW_Text::HAlign>("HAlign")
    .value("HLeft", DRW_Text::HLeft)
    .value("HCenter", DRW_Text::HCenter)
    .value("HRight", DRW_Text::HRight)
    .value("HAligned", DRW_Text::HAligned)
    .value("HMiddle", DRW_Text::HMiddle)
    .value("HFit", DRW_Text::HFit);

  class_<DRW_Text, base<DRW_Line>>("DRW_Text")
    .constructor<>()
    .property("height", &DRW_Text::height)
    .property("text", &DRW_Text::text)
    .property("angle", &DRW_Text::angle)
    .property("widthScale", &DRW_Text::widthscale)
    .property("oblique", &DRW_Text::oblique)
    .property("style", &DRW_Text::style)
    .property("textGen", &DRW_Text::textgen)
    .property("alignH", &DRW_Text::alignH)
    .property("alignV", &DRW_Text::alignV)
    .property("styleH", &DRW_Text::styleH, return_value_policy::reference())
    .function("applyExtrusion", &DRW_Text::applyExtrusion);

  enum_<DRW_MText::Attach>("Attach")
    .value("TopLeft", DRW_MText::TopLeft)
    .value("TopCenter", DRW_MText::TopCenter)
    .value("TopRight", DRW_MText::TopRight)
    .value("MiddleLeft", DRW_MText::MiddleLeft)
    .value("MiddleCenter", DRW_MText::MiddleCenter)
    .value("MiddleRight", DRW_MText::MiddleRight)
    .value("BottomLeft", DRW_MText::BottomLeft)
    .value("BottomCenter", DRW_MText::BottomCenter)
    .value("BottomRight", DRW_MText::BottomRight);

  class_<DRW_MText, base<DRW_Text>>("DRW_MText")
    .constructor<>()
    .property("interlin", &DRW_MText::interlin);

  class_<DRW_Vertex, base<DRW_Point>>("DRW_Vertex")
    .constructor<>()
    .constructor<double, double, double, double>()
    .property("stawidth", &DRW_Vertex::stawidth)
    .property("endwidth", &DRW_Vertex::endwidth)
    .property("bulge", &DRW_Vertex::bulge)
    .property("flags", &DRW_Vertex::flags)
    .property("tgdir", &DRW_Vertex::tgdir)
    .property("vindex1", &DRW_Vertex::vindex1)
    .property("vindex2", &DRW_Vertex::vindex2)
    .property("vindex3", &DRW_Vertex::vindex3)
    .property("vindex4", &DRW_Vertex::vindex4)
    .property("identifier", &DRW_Vertex::identifier);

  class_<DRW_Polyline, base<DRW_Point>>("DRW_Polyline")
    .constructor<>()
    .function("addVertex", &DRW_Polyline::addVertex)
    .function("appendVertex", &DRW_Polyline::appendVertex)
    .function("getVertexList", &DRW_Polyline::getVertexList)
    .property("defstawidth", &DRW_Polyline::defstawidth)
    .property("defendwidth", &DRW_Polyline::defendwidth)
    .property("flags", &DRW_Polyline::flags)
    .property("vertexcount", &DRW_Polyline::vertexcount)
    .property("facecount", &DRW_Polyline::facecount)
    .property("smoothM", &DRW_Polyline::smoothM)
    .property("smoothN", &DRW_Polyline::smoothN)
    .property("curvetype", &DRW_Polyline::curvetype)
    .function("addVertex", &DRW_Polyline::addVertex)
    .function("appendVertex", &DRW_Polyline::appendVertex);
    // .property("vertlist", &DRW_Polyline::vertlist);

  class_<DRW_Spline, base<DRW_Entity>>("DRW_Spline")
    .constructor<>()
    .function("applyExtrusion", &DRW_Spline::applyExtrusion)
    .function("getControlList", &DRW_Spline::getControlList)
    .function("getFitList", &DRW_Spline::getFitList)
    .property("normalVec", &DRW_Spline::normalVec, return_value_policy::reference())
    .property("tgStart", &DRW_Spline::tgStart, return_value_policy::reference())
    .property("tgEnd", &DRW_Spline::tgEnd, return_value_policy::reference())
    .property("flags", &DRW_Spline::flags)
    .property("degree", &DRW_Spline::degree)
    .property("numberOfKnots", &DRW_Spline::nknots)
    .property("numberOfControls", &DRW_Spline::ncontrol)
    .property("numberOfFits", &DRW_Spline::nfit)
    .property("tolKnot", &DRW_Spline::tolknot)
    .property("tolControl", &DRW_Spline::tolcontrol)
    .property("tolFit", &DRW_Spline::tolfit)
    .property("knots", &DRW_Spline::knotslist, return_value_policy::reference())
    .property("weights", &DRW_Spline::weightlist, return_value_policy::reference());
    // .property("controllist", &DRW_Spline::controllist)
    // .property("fitlist", &DRW_Spline::fitlist);

  class_<DRW_HatchLoop>("DRW_HatchLoop")
    .constructor<int>()
    .function("update", &DRW_HatchLoop::update)
    .function("getObjList", &DRW_HatchLoop::getObjList)
    .property("type", &DRW_HatchLoop::type)
    .property("numberOfEdges", &DRW_HatchLoop::numedges);
    // .property("objlist", &DRW_HatchLoop::objlist);

  value_object<DRW_HatchPattenLine>("DRW_HatchPattenLine")
    .field("angle", &DRW_HatchPattenLine::angle)
    .field("base", &DRW_HatchPattenLine::base)
    .field("offset", &DRW_HatchPattenLine::offset)
    .field("dashPattern", &DRW_HatchPattenLine::dashPattern);

  class_<DRW_Hatch, base<DRW_Point>>("DRW_Hatch")
    .constructor<>()
    .function("appendLoop", &DRW_Hatch::appendLoop)
    .function("getLoopList", &DRW_Hatch::getLoopList)
    .property("name", &DRW_Hatch::name)
    .property("solid", &DRW_Hatch::solid)
    .property("associative", &DRW_Hatch::associative)
    .property("hatchStyle", &DRW_Hatch::hstyle)
    .property("patternType", &DRW_Hatch::hpattern)
    .property("doubleFlag", &DRW_Hatch::doubleflag)
    .property("numberOfLoops", &DRW_Hatch::loopsnum)
    .property("angle", &DRW_Hatch::angle)
    .property("scale", &DRW_Hatch::scale)
    .property("numberOfDefinitionLines", &DRW_Hatch::deflines)
    .property("definitionLines", &DRW_Hatch::deflinelist, return_value_policy::reference());
    // .property("looplist", &DRW_Hatch::looplist);

  class_<DRW_Image, base<DRW_Line>>("DRW_Image")
    .constructor<>()
    .property("ref", &DRW_Image::ref)
    .property("vVector", &DRW_Image::vVector, return_value_policy::reference())
    .property("sizeu", &DRW_Image::sizeu)
    .property("sizev", &DRW_Image::sizev)
    .property("dz", &DRW_Image::dz)
    .property("clip", &DRW_Image::clip)
    .property("brightness", &DRW_Image::brightness)
    .property("contrast", &DRW_Image::contrast)
    .property("fade", &DRW_Image::fade);

  class_<DRW_Dimension, base<DRW_Entity>>("DRW_Dimension")
    .constructor<>()
    .function("getDefPoint", &DRW_Dimension::getDefPoint)
    .function("setDefPoint", &DRW_Dimension::setDefPoint)
    .function("getTextPoint", &DRW_Dimension::getTextPoint)
    .function("setTextPoint", &DRW_Dimension::setTextPoint)
    .function("getStyle", &DRW_Dimension::getStyle)
    .function("setStyle", &DRW_Dimension::setStyle)
    .function("getAlign", &DRW_Dimension::getAlign)
    .function("setAlign", &DRW_Dimension::setAlign)
    .function("getTextLineStyle", &DRW_Dimension::getTextLineStyle)
    .function("setTextLineStyle", &DRW_Dimension::setTextLineStyle)
    .function("getText", &DRW_Dimension::getText)
    .function("setText", &DRW_Dimension::setText)
    .function("getTextLineFactor", &DRW_Dimension::getTextLineFactor)
    .function("setTextLineFactor", &DRW_Dimension::setTextLineFactor)
    .function("getDir", &DRW_Dimension::getDir)
    .function("setDir", &DRW_Dimension::setDir)
    .function("getExtrusion", &DRW_Dimension::getExtrusion)
    .function("setExtrusion", &DRW_Dimension::setExtrusion)
    .function("getName", &DRW_Dimension::getName)
    .function("setName", &DRW_Dimension::setName);

  class_<DRW_DimAligned, base<DRW_Dimension>>("DRW_DimAligned")
    .constructor<>()
    .constructor<const DRW_Dimension&>()
    .function("getClonePoint", &DRW_DimAligned::getClonepoint)
    .function("setClonePoint", &DRW_DimAligned::setClonePoint)
    .function("getDimPoint", &DRW_DimAligned::getDimPoint)
    .function("setDimPoint", &DRW_DimAligned::setDimPoint)
    .function("getDef1Point", &DRW_DimAligned::getDef1Point)
    .function("setDef1Point", &DRW_DimAligned::setDef1Point)
    .function("getDef2Point", &DRW_DimAligned::getDef2Point)
    .function("setDef2Point", &DRW_DimAligned::setDef2Point);

  class_<DRW_DimLinear, base<DRW_DimAligned>>("DRW_DimLinear")
    .constructor<>()
    .constructor<const DRW_Dimension&>()
    .function("getAngle", &DRW_DimLinear::getAngle)
    .function("setAngle", &DRW_DimLinear::setAngle)
    .function("getOblique", &DRW_DimLinear::getOblique)
    .function("setOblique", &DRW_DimLinear::setOblique);

  class_<DRW_DimRadial, base<DRW_Dimension>>("DRW_DimRadial")
    .constructor<>()
    .constructor<const DRW_Dimension&>()
    .function("getCenterPoint", &DRW_DimRadial::getCenterPoint)
    .function("setCenterPoint", &DRW_DimRadial::setCenterPoint)
    .function("getDiameterPoint", &DRW_DimRadial::getDiameterPoint)
    .function("setDiameterPoint", &DRW_DimRadial::setDiameterPoint)
    .function("getLeaderLength", &DRW_DimRadial::getLeaderLength)
    .function("setLeaderLength", &DRW_DimRadial::setLeaderLength);

  class_<DRW_DimDiametric, base<DRW_Dimension>>("DRW_DimDiametric")
    .constructor<>()
    .constructor<const DRW_Dimension&>()
    .function("getDiameter1Point", &DRW_DimDiametric::getDiameter1Point)
    .function("setDiameter1Point", &DRW_DimDiametric::setDiameter1Point)
    .function("getDiameter2Point", &DRW_DimDiametric::getDiameter2Point)
    .function("setDiameter2Point", &DRW_DimDiametric::setDiameter2Point)
    .function("getLeaderLength", &DRW_DimDiametric::getLeaderLength)
    .function("setLeaderLength", &DRW_DimDiametric::setLeaderLength);

  class_<DRW_DimAngular, base<DRW_Dimension>>("DRW_DimAngular")
    .constructor<>()
    .constructor<const DRW_Dimension&>()
    .function("getFirstLine1", &DRW_DimAngular::getFirstLine1)
    .function("setFirstLine1", &DRW_DimAngular::setFirstLine1)
    .function("getFirstLine2", &DRW_DimAngular::getFirstLine2)
    .function("setFirstLine2", &DRW_DimAngular::setFirstLine2)
    .function("getSecondLine1", &DRW_DimAngular::getSecondLine1)
    .function("setSecondLine1", &DRW_DimAngular::setSecondLine1)
    .function("getSecondLine2", &DRW_DimAngular::getSecondLine2)
    .function("setSecondLine2", &DRW_DimAngular::setSecondLine2)
    .function("getDimPoint", &DRW_DimAngular::getDimPoint)
    .function("setDimPoint", &DRW_DimAngular::setDimPoint);

  class_<DRW_DimAngular3p, base<DRW_Dimension>>("DRW_DimAngular3p")
    .constructor<>()
    .constructor<const DRW_Dimension&>()
    .function("getFirstLine", &DRW_DimAngular3p::getFirstLine)
    .function("setFirstLine", &DRW_DimAngular3p::setFirstLine)
    .function("getSecondLine", &DRW_DimAngular3p::getSecondLine)
    .function("setSecondLine", &DRW_DimAngular3p::setSecondLine)
    .function("getVertexPoint", &DRW_DimAngular3p::getVertexPoint)
    .function("SetVertexPoint", &DRW_DimAngular3p::SetVertexPoint)
    .function("getDimPoint", &DRW_DimAngular3p::getDimPoint)
    .function("setDimPoint", &DRW_DimAngular3p::setDimPoint);

  class_<DRW_DimOrdinate, base<DRW_Dimension>>("DRW_DimOrdinate")
    .constructor<>()
    .constructor<const DRW_Dimension&>()
    .function("getOriginPoint", &DRW_DimOrdinate::getOriginPoint)
    .function("setOriginPoint", &DRW_DimOrdinate::setOriginPoint)
    .function("getFirstLine", &DRW_DimOrdinate::getFirstLine)
    .function("setFirstLine", &DRW_DimOrdinate::setFirstLine)
    .function("getSecondLine", &DRW_DimOrdinate::getSecondLine)
    .function("setSecondLine", &DRW_DimOrdinate::setSecondLine);

  class_<DRW_Leader, base<DRW_Entity>>("DRW_Leader")
    .constructor<>()
    .function("applyExtrusion", &DRW_Leader::applyExtrusion)
    .property("style", &DRW_Leader::style)
    .property("arrow", &DRW_Leader::arrow)
    .property("leadertype", &DRW_Leader::leadertype)
    .property("flag", &DRW_Leader::flag)
    .property("hookline", &DRW_Leader::hookline)
    .property("hookflag", &DRW_Leader::hookflag)
    .property("textheight", &DRW_Leader::textheight)
    .property("textwidth", &DRW_Leader::textwidth)
    .property("vertnum", &DRW_Leader::vertnum)
    .property("coloruse", &DRW_Leader::coloruse)
    .property("annotHandle", &DRW_Leader::annotHandle)
    .property("extrusionPoint", &DRW_Leader::extrusionPoint, return_value_policy::reference())
    .property("horizdir", &DRW_Leader::horizdir, return_value_policy::reference())
    .property("offsetblock", &DRW_Leader::offsetblock, return_value_policy::reference())
    .property("offsettext", &DRW_Leader::offsettext, return_value_policy::reference());
    // .property("vertexList", &DRW_Leader::vertexlist);

  class_<DRW_Viewport, base<DRW_Point>>("DRW_Viewport")
    .constructor<>()
    .function("applyExtrusion", &DRW_Viewport::applyExtrusion)
    .property("pswidth", &DRW_Viewport::pswidth)
    .property("psheight", &DRW_Viewport::psheight)
    .property("vpstatus", &DRW_Viewport::vpstatus)
    .property("vpID", &DRW_Viewport::vpID)
    .property("centerPX", &DRW_Viewport::centerPX)
    .property("centerPY", &DRW_Viewport::centerPY)
    .property("snapPX", &DRW_Viewport::snapPX)
    .property("snapPY", &DRW_Viewport::snapPY)
    .property("snapSpPX", &DRW_Viewport::snapSpPX)
    .property("snapSpPY", &DRW_Viewport::snapSpPY)
    .property("viewDir", &DRW_Viewport::viewDir, return_value_policy::reference())
    .property("viewTarget", &DRW_Viewport::viewTarget, return_value_policy::reference())
    .property("viewLength", &DRW_Viewport::viewLength)
    .property("frontClip", &DRW_Viewport::frontClip)
    .property("backClip", &DRW_Viewport::backClip)
    .property("viewHeight", &DRW_Viewport::viewHeight)
    .property("snapAngle", &DRW_Viewport::snapAngle)
    .property("twistAngle", &DRW_Viewport::twistAngle);
}

EMSCRIPTEN_BINDINGS(DRW_Interface) {
  emscripten::class_<DRW_Interface>("DRW_Interface")
    .function("addHeader", &DRW_Interface::addHeader, allow_raw_pointer<DRW_Header*>())
    .function("addLType", &DRW_Interface::addLType)
    .function("addLayer", &DRW_Interface::addLayer)
    .function("addDimStyle", &DRW_Interface::addDimStyle)
    .function("addVport", &DRW_Interface::addVport)
    .function("addTextStyle", &DRW_Interface::addTextStyle)
    .function("addAppId", &DRW_Interface::addAppId)
    .function("addBlock", &DRW_Interface::addBlock)
    .function("setBlock", &DRW_Interface::setBlock)
    .function("endBlock", &DRW_Interface::endBlock)
    .function("addPoint", &DRW_Interface::addPoint)
    .function("addLine", &DRW_Interface::addLine)
    .function("addRay", &DRW_Interface::addRay)
    .function("addXline", &DRW_Interface::addXline)
    .function("addArc", &DRW_Interface::addArc)
    .function("addCircle", &DRW_Interface::addCircle)
    .function("addEllipse", &DRW_Interface::addEllipse)
    .function("addLWPolyline", &DRW_Interface::addLWPolyline)
    .function("addPolyline", &DRW_Interface::addPolyline)
    .function("addSpline", &DRW_Interface::addSpline, allow_raw_pointer<DRW_Spline*>())
    .function("addKnot", &DRW_Interface::addKnot)
    .function("addInsert", &DRW_Interface::addInsert)
    .function("addTrace", &DRW_Interface::addTrace)
    .function("add3dFace", &DRW_Interface::add3dFace)
    .function("addSolid", &DRW_Interface::addSolid)
    .function("addMText", &DRW_Interface::addMText)
    .function("addText", &DRW_Interface::addText)
    .function("addDimAlign", &DRW_Interface::addDimAlign, allow_raw_pointer<DRW_DimAligned*>())
    .function("addDimLinear", &DRW_Interface::addDimLinear, allow_raw_pointer<DRW_DimLinear*>())
    .function("addDimRadial", &DRW_Interface::addDimRadial, allow_raw_pointer<DRW_DimRadial*>())
    .function("addDimDiametric", &DRW_Interface::addDimDiametric, allow_raw_pointer<DRW_DimDiametric*>())
    .function("addDimAngular", &DRW_Interface::addDimAngular, allow_raw_pointer<DRW_DimAngular*>())
    .function("addDimAngular3P", &DRW_Interface::addDimAngular3P, allow_raw_pointer<DRW_DimAngular3p*>())
    .function("addDimOrdinate", &DRW_Interface::addDimOrdinate, allow_raw_pointer<DRW_DimOrdinate*>())
    .function("addLeader", &DRW_Interface::addLeader, allow_raw_pointer<DRW_Leader*>())
    .function("addHatch", &DRW_Interface::addHatch, allow_raw_pointer<DRW_Hatch*>())
    .function("addViewport", &DRW_Interface::addViewport)
    .function("addImage", &DRW_Interface::addImage, allow_raw_pointer<DRW_Image*>())
    .function("linkImage", &DRW_Interface::linkImage, allow_raw_pointer<DRW_ImageDef*>())
    // .function("addComment", &DRW_Interface::addComment, allow_raw_pointer<const char*>())
    .function("writeHeader", &DRW_Interface::writeHeader)
    .function("writeBlocks", &DRW_Interface::writeBlocks)
    .function("writeBlockRecords", &DRW_Interface::writeBlockRecords)
    .function("writeEntities", &DRW_Interface::writeEntities)
    .function("writeLTypes", &DRW_Interface::writeLTypes)
    .function("writeLayers", &DRW_Interface::writeLayers)
    .function("writeTextstyles", &DRW_Interface::writeTextstyles)
    .function("writeVports", &DRW_Interface::writeVports)
    .function("writeDimstyles", &DRW_Interface::writeDimstyles)
    .function("writeAppId", &DRW_Interface::writeAppId);
}

EMSCRIPTEN_BINDINGS(dxfRW) {
  class_<dxfRW>("DRW_DxfRW")
    .constructor<const std::string&>()
    .function("setDebug", &dxfRW::setDebug)
    .function("read", &dxfRW::read, allow_raw_pointer<DRW_Interface*>())
    .function("setBinary", &dxfRW::setBinary)
    .function("write", &dxfRW::write, allow_raw_pointer<DRW_Interface*>())
    .function("writeLineType", &dxfRW::writeLineType, allow_raw_pointer<DRW_LType*>())
    .function("writeLayer", &dxfRW::writeLayer, allow_raw_pointer<DRW_Layer*>())
    .function("writeDimstyle", &dxfRW::writeDimstyle, allow_raw_pointer<DRW_Dimstyle*>())
    .function("writeTextstyle", &dxfRW::writeTextstyle, allow_raw_pointer<DRW_Textstyle*>())
    .function("writeVport", &dxfRW::writeVport, allow_raw_pointer<DRW_Vport*>())
    .function("writeAppId", &dxfRW::writeAppId, allow_raw_pointer<DRW_AppId*>())
    .function("writePoint", &dxfRW::writePoint, allow_raw_pointer<DRW_Point*>())
    .function("writeLine", &dxfRW::writeLine, allow_raw_pointer<DRW_Line*>())
    .function("writeRay", &dxfRW::writeRay, allow_raw_pointer<DRW_Ray*>())
    .function("writeXline", &dxfRW::writeXline, allow_raw_pointer<DRW_Xline*>())
    .function("writeCircle", &dxfRW::writeCircle, allow_raw_pointer<DRW_Circle*>())
    .function("writeArc", &dxfRW::writeArc, allow_raw_pointer<DRW_Arc*>())
    .function("writeEllipse", &dxfRW::writeEllipse, allow_raw_pointer<DRW_Ellipse*>())
    .function("writeTrace", &dxfRW::writeTrace, allow_raw_pointer<DRW_Trace*>())
    .function("writeSolid", &dxfRW::writeSolid, allow_raw_pointer<DRW_Solid*>())
    .function("write3dface", &dxfRW::write3dface, allow_raw_pointer<DRW_3Dface*>())
    .function("writeLWPolyline", &dxfRW::writeLWPolyline, allow_raw_pointer<DRW_LWPolyline*>())
    .function("writePolyline", &dxfRW::writePolyline, allow_raw_pointer<DRW_Polyline*>())
    .function("writeSpline", &dxfRW::writeSpline, allow_raw_pointer<DRW_Spline*>())
    .function("writeBlockRecord", &dxfRW::writeBlockRecord)
    .function("writeBlock", &dxfRW::writeBlock, allow_raw_pointer<DRW_Block*>())
    .function("writeInsert", &dxfRW::writeInsert, allow_raw_pointer<DRW_Insert*>())
    .function("writeMText", &dxfRW::writeMText, allow_raw_pointer<DRW_MText*>())
    .function("writeText", &dxfRW::writeText, allow_raw_pointer<DRW_Text*>())
    .function("writeHatch", &dxfRW::writeHatch, allow_raw_pointer<DRW_Hatch*>())
    .function("writeViewport", &dxfRW::writeViewport, allow_raw_pointer<DRW_Viewport*>())
    .function("writeImage", &dxfRW::writeImage, allow_raw_pointer<DRW_Image*>())
    .function("writeLeader", &dxfRW::writeLeader, allow_raw_pointer<DRW_Leader*>())
    .function("writeDimension", &dxfRW::writeDimension, allow_raw_pointer<DRW_Dimension*>())
    .function("setEllipseParts", &dxfRW::setEllipseParts);
}

EMSCRIPTEN_BINDINGS(dwgR) {
  class_<dwgR>("DRW_DwgR")
    .constructor<const std::string&>()
    .function("read", &dwgR::read, allow_raw_pointer<DRW_Interface*>())
    .function("getPreview", &dwgR::getPreview)
    .function("getVersion", &dwgR::getVersion)
    .function("getError", &dwgR::getError)
    .function("testReader", &dwgR::testReader)
    .function("setDebug", &dwgR::setDebug);
}

EMSCRIPTEN_BINDINGS(DRW_reader) {
  register_vector<DRW_Entity*>("DRW_EntityList");
  register_vector<DRW_LType>("DRW_LTypeList");
  register_vector<DRW_Layer>("DRW_LayerList");
  register_vector<DRW_Dimstyle>("DRW_DimstyleList");
  register_vector<DRW_Vport>("DRW_VportList");
  register_vector<DRW_Textstyle>("DRW_TextstyleList");
  register_vector<DRW_AppId>("DRW_AppIdList");
  register_vector<dx_ifaceBlock*>("DRW_BlockList");
  register_vector<dx_ifaceImg*>("DRW_ImgList");

  class_<dx_ifaceImg, base<DRW_Image>>("dx_ifaceImg")
    .constructor<>()
    .constructor<const DRW_Image&>()
    .property("path", &dx_ifaceImg::path, return_value_policy::reference());

  class_<dx_ifaceBlock, base<DRW_Block>>("dx_ifaceBlock")
    .constructor<>()
    .constructor<const DRW_Block&>()
    .property("entities", &dx_ifaceBlock::ent, return_value_policy::reference());

  class_<dx_data>("dx_data")
    .constructor<>()
    .property("header", &dx_data::headerC, return_value_policy::reference())
    .property("lineTypes", &dx_data::lineTypes, return_value_policy::reference())
    .property("layers", &dx_data::layers, return_value_policy::reference())
    .property("dimStyles", &dx_data::dimStyles, return_value_policy::reference())
    .property("viewports", &dx_data::viewports, return_value_policy::reference())
    .property("textStyles", &dx_data::textStyles, return_value_policy::reference())
    .property("appIds", &dx_data::appIds, return_value_policy::reference())
    .property("blocks", &dx_data::blocks, return_value_policy::reference())
    .property("images", &dx_data::images, return_value_policy::reference())
    .property("mBlock", &dx_data::mBlock, return_value_policy::reference(), allow_raw_pointer<dx_ifaceBlock*>());

  class_<dx_iface, base<DRW_Interface>>("dx_iface")
    .constructor<>()
    .function("fileImport", &dx_iface::fileImport, allow_raw_pointer<dx_data*>())
    .function("fileExport", &dx_iface::fileExport, allow_raw_pointer<dx_data*>())
    .function("writeEntity", &dx_iface::writeEntity, allow_raw_pointer<DRW_Entity*>())
    .function("addHeader", &dx_iface::addHeader, allow_raw_pointer<DRW_Header*>())
    .function("addLType", &dx_iface::addLType)
    .function("addLayer", &dx_iface::addLayer)
    .function("addDimStyle", &dx_iface::addDimStyle)
    .function("addVport", &dx_iface::addVport)
    .function("addTextStyle", &dx_iface::addTextStyle)
    .function("addAppId", &dx_iface::addAppId)
    .function("addBlock", &dx_iface::addBlock)
    .function("setBlock", &dx_iface::setBlock)
    .function("endBlock", &dx_iface::endBlock)
    .function("addPoint", &dx_iface::addPoint)
    .function("addLine", &dx_iface::addLine)
    .function("addRay", &dx_iface::addRay)
    .function("addXline", &dx_iface::addXline)
    .function("addArc", &dx_iface::addArc)
    .function("addCircle", &dx_iface::addCircle)
    .function("addEllipse", &dx_iface::addEllipse)
    .function("addLWPolyline", &dx_iface::addLWPolyline)
    .function("addPolyline", &dx_iface::addPolyline)
    .function("addSpline", &dx_iface::addSpline, allow_raw_pointer<DRW_Spline*>())
    .function("addKnot", &dx_iface::addKnot)
    .function("addInsert", &dx_iface::addInsert)
    .function("addTrace", &dx_iface::addTrace)
    .function("add3dFace", &dx_iface::add3dFace)
    .function("addSolid", &dx_iface::addSolid)
    .function("addMText", &dx_iface::addMText)
    .function("addText", &dx_iface::addText)
    .function("addDimAlign", &dx_iface::addDimAlign, allow_raw_pointer<DRW_DimAligned*>())
    .function("addDimLinear", &dx_iface::addDimLinear, allow_raw_pointer<DRW_DimLinear*>())
    .function("addDimRadial", &dx_iface::addDimRadial, allow_raw_pointer<DRW_DimRadial*>())
    .function("addDimDiametric", &dx_iface::addDimDiametric, allow_raw_pointer<DRW_DimDiametric*>())
    .function("addDimAngular", &dx_iface::addDimAngular, allow_raw_pointer<DRW_DimAngular*>())
    .function("addDimAngular3P", &dx_iface::addDimAngular3P, allow_raw_pointer<DRW_DimAngular3p*>())
    .function("addDimOrdinate", &dx_iface::addDimOrdinate, allow_raw_pointer<DRW_DimOrdinate*>())
    .function("addLeader", &dx_iface::addLeader, allow_raw_pointer<DRW_Leader*>())
    .function("addHatch", &dx_iface::addHatch, allow_raw_pointer<DRW_Hatch*>())
    .function("addViewport", &dx_iface::addViewport)
    .function("addImage", &dx_iface::addImage, allow_raw_pointer<DRW_Image*>())
    .function("linkImage", &dx_iface::linkImage, allow_raw_pointer<DRW_ImageDef*>())
    // .function("addComment", &dx_iface::addComment, allow_raw_pointer<const char*>())
    .function("writeHeader", &dx_iface::writeHeader)
    .function("writeBlocks", &dx_iface::writeBlocks)
    .function("writeBlockRecords", &dx_iface::writeBlockRecords)
    .function("writeEntities", &dx_iface::writeEntities)
    .function("writeLTypes", &dx_iface::writeLTypes)
    .function("writeLayers", &dx_iface::writeLayers)
    .function("writeTextstyles", &dx_iface::writeTextstyles)
    .function("writeVports", &dx_iface::writeVports)
    .function("writeDimstyles", &dx_iface::writeDimstyles)
    .function("writeAppId", &dx_iface::writeAppId)
    .property("dxfW", &dx_iface::dxfW, allow_raw_pointer<dxfRW*>())
    .property("cData", &dx_iface::cData, allow_raw_pointer<dx_data*>())
    .property("currentBlock", &dx_iface::currentBlock, allow_raw_pointer<dx_ifaceBlock*>());
}
