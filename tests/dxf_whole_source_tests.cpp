#include "intern/dxfwholesource.h"
#include "intern/dxfwriter.h"
#include "dx_iface.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <type_traits>
using namespace WholeSourceV2;
// Evidence minting belongs exclusively to dxfRW, never an adapter callback.
static_assert(!std::is_constructible<Completion,Handler*,const void*>::value,"completion must not be externally constructible");
static_assert(!std::is_constructible<Capture,dxfReader&>::value,"capture must not be externally constructible");
static_assert(!std::is_constructible<FinishToken,const void*,bool>::value,"finish proof must not be externally constructible");
static_assert(!std::is_constructible<Ticket,const Source*,std::shared_ptr<Pool>,const void*>::value,"source ticket must not be externally constructible");
static_assert(!std::is_copy_constructible<Ticket>::value&&!std::is_copy_assignable<Ticket>::value
    &&!std::is_move_constructible<Ticket>::value&&!std::is_move_assignable<Ticket>::value,"source ticket must not be transferable");
static_assert(!std::is_copy_constructible<FinishToken>::value&&!std::is_copy_assignable<FinishToken>::value
    &&!std::is_move_constructible<FinishToken>::value&&!std::is_move_assignable<FinishToken>::value,"finish proof must not be transferable");
template<class T,class=void>struct ExposesFinish:std::false_type{};
template<class T>struct ExposesFinish<T,std::void_t<decltype(std::declval<T&>().finish(true))>>:std::true_type{};
template<class T,class=void>struct ExposesCaptureFinish:std::false_type{};
template<class T>struct ExposesCaptureFinish<T,std::void_t<decltype(std::declval<T&>().finish(std::declval<Handler&>(),nullptr))>>:std::true_type{};
static_assert(!ExposesFinish<Completion>::value,"completion minting must be private");
static_assert(!ExposesCaptureFinish<Capture>::value,"capture minting must be private");
template<class T,class=void>struct ExposesCaptureStart:std::false_type{};
template<class T>struct ExposesCaptureStart<T,std::void_t<decltype(std::declval<T&>().start(Limits{}))>>:std::true_type{};
static_assert(!ExposesCaptureStart<Capture>::value,"capture start must be private");
class DxWholeSourceTestAccess {
public:
    static void limits(dx_iface& a,Limits value){a.m_wholeLimits=value;}
    static std::shared_ptr<const Source> get(const dx_iface& a,const dx_data& d){
        const auto& s=d.m_tableDxfSource;const auto& p=s.wholeInput;
        if(!s.closed||!s.successful||s.wholeFailed||!s.wholeFinished||!p
            ||p.get()!=s.wholeInputOwner.get()||p->container!=&d||p->generation!=d.importGeneration
            ||p->containerId!=d.m_entitiesContainerIdentity.value||!p->source
            ||p->source.get()!=s.wholeSourceOwner.get()||p->source->version()!=d.sourceVersion
            ||d.sourceFormat!=dx_source_format::DXF)return {};
        (void)a;return p->source;
    }
};
struct ConsumerAccess {static std::shared_ptr<const Source> get(const dx_iface& c,const dx_data& d){return DxWholeSourceTestAccess::get(c,d);}};
class Consumer:public dx_iface {
public: bool import(const std::string& path,dx_data& data){return fileImport(path,&data,false);}
};
class Adversary;
static bool substituteForeign(Adversary&,const void*,const std::string&,bool&,bool) noexcept;
class Adversary:public Consumer {
public:
    enum class Mode{None,Before,After,CopyField,Skip,Duplicate,Cross,CrossTicket,CrossFinish,WrongData,Generation,SkipFinish,DuplicateFinish,QuotaPoison,ThrowRecord,Reentry};Mode mode=Mode::None;std::string reentryPath;bool reentryHit=false,reentryAccepted=false,foreignHit=false,foreignRead=false;
    void addLayer(const DRW_Layer& layer) override {
        if(mode==Mode::ThrowRecord)throw std::runtime_error("scratch callback fault");
        if(mode==Mode::Reentry&&!reentryHit) {reentryHit=true;auto nested=std::make_unique<dx_data>();reentryAccepted=import(reentryPath,*nested);}
        dx_iface::addLayer(layer);
    }
    void wholeSourcePending(const void* id,const Source& value,const Ticket& t) noexcept override {
        auto& delivery=const_cast<Source&>(value);
        if(mode==Mode::Skip)return;
        if(mode==Mode::CrossTicket||mode==Mode::CrossFinish){
            const bool finishAttack=mode==Mode::CrossFinish;
            if(finishAttack)Consumer::wholeSourcePending(id,value,t);
            foreignRead=substituteForeign(*this,id,reentryPath,foreignHit,finishAttack);return;
        }
        if(mode==Mode::Before){const auto f=delivery.field(0);const_cast<char*>(delivery.value(f).data())[0]^=1;}
        if(mode==Mode::CopyField){auto copy=delivery.field(0);++copy.code;if(copy.code==delivery.field(0).code)return;}
        if(mode==Mode::QuotaPoison){(void)t.verifiedCopy(value,id);(void)t.verifiedCopy(value,id);}
        if(mode==Mode::Cross)Consumer::wholeSourcePending(nullptr,value,t);
        auto* originalData=cData;const auto originalGeneration=originalData?originalData->importGeneration:0;
        if(mode==Mode::WrongData)cData=nullptr;
        if(mode==Mode::Generation&&originalData)++originalData->importGeneration;
        Consumer::wholeSourcePending(id,value,t);
        if(mode==Mode::WrongData)cData=originalData;
        if(mode==Mode::Generation&&originalData)originalData->importGeneration=originalGeneration;
        if(mode==Mode::Duplicate)Consumer::wholeSourcePending(id,value,t);
        if(mode==Mode::After){const auto f=delivery.field(0);const_cast<char*>(delivery.value(f).data())[0]^=1;}
    }
    void wholeSourceFinished(const void* id,const FinishToken& token)noexcept override {
        if(mode==Mode::SkipFinish)return;
        Consumer::wholeSourceFinished(id,token);
        if(mode==Mode::DuplicateFinish)Consumer::wholeSourceFinished(id,token);
    }
};
class Bare:public DRW_Interface,public Handler {
    const void* active=nullptr;std::shared_ptr<const Source> pending,finished;bool invalid=false;unsigned calls=0;
public:
    bool enabled=true;
    bool wantsWholeSource()const noexcept override{return enabled;}
    void wholeSourceBegin(const void* id)noexcept override {active=id;pending.reset();finished.reset();invalid=false;calls=0;}
    bool wholeSourceSelected(const void* id)const noexcept override{return active==id&&id;}
    void wholeSourcePending(const void* id,const Source& source,const Ticket& ticket)noexcept override {
        if(id!=active||++calls!=1u){invalid=true;return;}pending=ticket.verifiedCopy(source,id);if(!pending)invalid=true;
    }
    void wholeSourceFinished(const void* id,const FinishToken& proof)noexcept override {
        if(id==active&&!invalid&&calls==1u&&proof.validFor(id)&&pending&&pending->state()==State::ThroughEof)finished=pending;
        else finished.reset();active=nullptr;
    }
    bool hasAuthority()const noexcept{return static_cast<bool>(finished);}
    void addHeader(const DRW_Header *) override {}
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
    void writeObjects() override {}
    void writeAppId() override {}
};
static bool substituteForeign(Adversary& target,const void* outer,const std::string& path,bool& hit,bool finishAttack) noexcept {
    class Forwarder:public Bare {
        Adversary& target;const void* outer;bool& hit;bool finishAttack;
    public:
        Forwarder(Adversary& t,const void* o,bool& h,bool f):target(t),outer(o),hit(h),finishAttack(f){}
        void wholeSourcePending(const void* id,const Source& foreign,const Ticket& ticket) noexcept override {
            if(finishAttack)Bare::wholeSourcePending(id,foreign,ticket);
            else {hit=true;target.Consumer::wholeSourcePending(outer,foreign,ticket);}
        }
        void wholeSourceFinished(const void* id,const FinishToken& token) noexcept override {
            Bare::wholeSourceFinished(id,token);
            if(finishAttack){hit=true;target.Consumer::wholeSourceFinished(outer,token);}
        }
    };
    try {Forwarder f(target,outer,hit,finishAttack);dxfRW reader(path.c_str());return reader.read(&f,false);}
    catch(...) {return false;}
}
bool hidden(const Source& s) {
    bool layer=false,application=false;
    for(std::size_t i=0;i<s.fieldCount();++i) {const auto& f=s.field(i);if(!s.bounds(f))return false;
        if(f.code==0&&f.kind==DxfReadRecordView::Text){layer=s.value(f)=="LAYER";application=false;}
        if(layer&&f.code==102&&s.value(f)=="{TEST_HIDDEN")application=true;
        if(layer&&application&&f.code==340&&s.value(f)=="D")return true;
    }return false;
}
// Locally authored at test runtime; no DWG/DXF fixture is installed.
static std::string sourceText() {
    const std::pair<int,const char*> tags[] = {
        {  0, "SECTION"}, {  2, "HEADER"}, {  9, "$ACADVER"}, {  1, "AC1027"},
        {  9, "$DWGCODEPAGE"}, {  3, "ANSI_1252"}, {  0, "ENDSEC"}, {  0, "SECTION"},
        {  2, "TABLES"}, {  0, "TABLE"}, {  2, "LTYPE"}, {  5, "100"},
        {100, "AcDbSymbolTable"}, { 70, "    2"}, {  0, "LTYPE"}, {  5, "101"},
        {330, "100"}, {100, "AcDbSymbolTableRecord"}, {100, "AcDbLinetypeTableRecord"}, {  2, "ByLayer"},
        { 70, "    0"}, {  3, ""}, { 72, "   65"}, { 73, "    0"},
        { 40, "0"}, {  0, "LTYPE"}, {  5, "102"}, {330, "100"},
        {100, "AcDbSymbolTableRecord"}, {100, "AcDbLinetypeTableRecord"}, {  2, "Continuous"}, { 70, "    0"},
        {  3, ""}, { 72, "   65"}, { 73, "    0"}, { 40, "0"},
        {  0, "ENDTAB"}, {  0, "TABLE"}, {  2, "LAYER"}, {  5, "1E0"},
        {100, "AcDbSymbolTable"}, { 70, "    1"}, {  0, "LAYER"}, {  5, "1E1"},
        {330, "1E0"}, {100, "AcDbSymbolTableRecord"}, {100, "AcDbLayerTableRecord"}, {  2, "0"},
        { 70, "    0"}, { 62, "    7"}, {  6, "Continuous"}, {  0, "ENDTAB"},
        {  0, "TABLE"}, {  2, "BLOCK_RECORD"}, {  5, "200"}, {100, "AcDbSymbolTable"},
        { 70, "    3"}, {  0, "BLOCK_RECORD"}, {  5, "201"}, {330, "200"},
        {100, "AcDbSymbolTableRecord"}, {100, "AcDbBlockTableRecord"}, {  2, "*Model_Space"}, { 70, "    0"},
        {  0, "BLOCK_RECORD"}, {  5, "202"}, {330, "200"}, {100, "AcDbSymbolTableRecord"},
        {100, "AcDbBlockTableRecord"}, {  2, "*Paper_Space"}, { 70, "    0"}, {  0, "BLOCK_RECORD"},
        {  5, "203"}, {330, "200"}, {100, "AcDbSymbolTableRecord"}, {100, "AcDbBlockTableRecord"},
        {  2, "AuthorityBlock"}, { 70, "    0"}, {  0, "ENDTAB"}, {  0, "ENDSEC"},
        {  0, "SECTION"}, {  2, "BLOCKS"}, {  0, "BLOCK"}, {  5, "310"},
        {330, "201"}, {100, "AcDbEntity"}, {  8, "0"}, {100, "AcDbBlockBegin"},
        {  2, "*Model_Space"}, { 70, "    0"}, { 10, "0"}, { 20, "0"},
        { 30, "0"}, {  3, "*Model_Space"}, {  1, ""}, {  0, "ENDBLK"},
        {  5, "311"}, {330, "201"}, {100, "AcDbEntity"}, {  8, "0"},
        {100, "AcDbBlockEnd"}, {  0, "BLOCK"}, {  5, "312"}, {330, "202"},
        {100, "AcDbEntity"}, {  8, "0"}, {100, "AcDbBlockBegin"}, {  2, "*Paper_Space"},
        { 70, "    0"}, { 10, "0"}, { 20, "0"}, { 30, "0"},
        {  3, "*Paper_Space"}, {  1, ""}, {  0, "ENDBLK"}, {  5, "313"},
        {330, "202"}, {100, "AcDbEntity"}, {  8, "0"}, {100, "AcDbBlockEnd"},
        {  0, "BLOCK"}, {  5, "300"}, {330, "203"}, {100, "AcDbEntity"},
        {  8, "0"}, {100, "AcDbBlockBegin"}, {  2, "AuthorityBlock"}, { 70, "    0"},
        { 10, "0"}, { 20, "0"}, { 30, "0"}, {  3, "AuthorityBlock"},
        {  1, ""}, {  0, "MLINE"}, {  5, "D101"}, {330, "203"},
        {100, "AcDbEntity"}, { 62, "    2"}, { 48, "1.25"}, {  8, "0"},
        {100, "AcDbMline"}, {  2, "OLD_ENTITY_NAME"}, {340, "D"}, { 40, "1"},
        { 70, "    1"}, { 71, "    1"}, { 72, "    2"}, { 73, "    2"},
        { 10, "4"}, { 20, "2"}, { 30, "3"}, {210, "0"},
        {220, "0"}, {230, "1"}, { 11, "4"}, { 21, "2"},
        { 31, "3"}, { 12, "1"}, { 22, "0"}, { 32, "0"},
        { 13, "0"}, { 23, "1"}, { 33, "0"}, { 74, "    2"},
        { 41, "0.5"}, { 41, "0"}, { 75, "    0"}, { 74, "    2"},
        { 41, "-0.25"}, { 41, "0"}, { 75, "    0"}, { 11, "14"},
        { 21, "2"}, { 31, "3"}, { 12, "1"}, { 22, "0"},
        { 32, "0"}, { 13, "0"}, { 23, "1"}, { 33, "0"},
        { 74, "    2"}, { 41, "0.5"}, { 41, "0"}, { 75, "    0"},
        { 74, "    2"}, { 41, "-0.25"}, { 41, "0"}, { 75, "    0"},
        {  0, "ENDBLK"}, {  5, "301"}, {330, "203"}, {100, "AcDbEntity"},
        {  8, "0"}, {100, "AcDbBlockEnd"}, {  0, "ENDSEC"}, {  0, "SECTION"},
        {  2, "ENTITIES"}, {  0, "MLINE"}, {  5, "D100"}, {330, "201"},
        {100, "AcDbEntity"}, { 62, "    2"}, { 48, "1.25"}, {  8, "0"},
        {100, "AcDbMline"}, {  2, "OLD_ENTITY_NAME"}, {340, "D"}, { 40, "1"},
        { 70, "    1"}, { 71, "    1"}, { 72, "    2"}, { 73, "    2"},
        { 10, "1"}, { 20, "2"}, { 30, "3"}, {210, "0"},
        {220, "0"}, {230, "1"}, { 11, "1"}, { 21, "2"},
        { 31, "3"}, { 12, "1"}, { 22, "0"}, { 32, "0"},
        { 13, "0"}, { 23, "1"}, { 33, "0"}, { 74, "    2"},
        { 41, "0.5"}, { 41, "0"}, { 75, "    0"}, { 74, "    2"},
        { 41, "-0.25"}, { 41, "0"}, { 75, "    0"}, { 11, "11"},
        { 21, "2"}, { 31, "3"}, { 12, "1"}, { 22, "0"},
        { 32, "0"}, { 13, "0"}, { 23, "1"}, { 33, "0"},
        { 74, "    2"}, { 41, "0.5"}, { 41, "0"}, { 75, "    0"},
        { 74, "    2"}, { 41, "-0.25"}, { 41, "0"}, { 75, "    0"},
        {  0, "ENDSEC"}, {  0, "SECTION"}, {  2, "OBJECTS"}, {  0, "DICTIONARY"},
        {  5, "C"}, {330, "0"}, {100, "AcDbDictionary"}, {280, "    0"},
        {281, "    1"}, {  3, "ACAD_MLINESTYLE"}, {350, "A"}, {  0, "DICTIONARY"},
        {  5, "A"}, {330, "C"}, {100, "AcDbDictionary"}, {280, "    0"},
        {281, "    1"}, {  3, "EFFECTIVE_STYLE"}, {350, "D"}, {  3, "RETARGET_STYLE"},
        {350, "D2"}, {  0, "MLINESTYLE"}, {  5, "D"}, {102, "{ACAD_REACTORS"},
        {330, "A"}, {102, "}"}, {330, "A"}, {100, "AcDbMlineStyle"},
        {  2, "OLD_BODY_NAME"}, { 70, "    2"}, {  3, "source description caf\u00e9^ A\\U+005CU+1234"}, { 62, "  256"},
        { 51, "90"}, { 52, "90"}, { 71, "    2"}, { 49, "0.5"},
        { 62, "    1"}, {  6, "ByLayer"}, { 49, "-0.25"}, { 62, "    3"},
        {  6, "Continuous"}, {  0, "MLINESTYLE"}, {  5, "D2"}, {102, "{ACAD_REACTORS"},
        {330, "A"}, {102, "}"}, {330, "A"}, {100, "AcDbMlineStyle"},
        {  2, "RETARGET_BODY"}, { 70, "    0"}, {  3, "retarget description"}, { 62, "    6"},
        { 51, "90"}, { 52, "90"}, { 71, "    2"}, { 49, "0.5"},
        { 62, "    5"}, {  6, "ByLayer"}, { 49, "-0.25"}, { 62, "    7"},
        {  6, "Continuous"}, {  0, "ENDSEC"}, {  0, "EOF"},
    };
    std::ostringstream out;for(const auto& tag:tags)out<<tag.first<<'\n'<<tag.second<<'\n';return out.str();
}
static bool writeFile(const std::filesystem::path& path,const std::string& bytes){
    std::ofstream out(path,std::ios::binary);out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));return static_cast<bool>(out);
}
static bool encodeBinary(const std::string& text,std::string& result){
    std::ostringstream bytes(std::ios::binary);bytes.write("AutoCAD Binary DXF\r\n\x1a\0",22);
    const bool r12=text.find("AC1009")!=std::string::npos;
    std::unique_ptr<dxfWriter> writer=r12?std::unique_ptr<dxfWriter>(new dxfWriterBinaryR12(&bytes))
        :std::unique_ptr<dxfWriter>(new dxfWriterBinary(&bytes));
    std::istringstream input(text);std::string code,value;
    while(std::getline(input,code)&&std::getline(input,value)) {
        const auto n=std::stoi(code);bool ok=false;
        // The production binary writer deliberately rejects 999. This local
        // physical decoder probe emits the two-byte code directly; it does not
        // establish native validity of binary 999 comments.
        if(n==999&&!r12){const char marker[]={static_cast<char>(0xe7),static_cast<char>(0x03)};
            bytes.write(marker,2);bytes.write(value.c_str(),static_cast<std::streamsize>(value.size()+1u));continue;}
        switch(dxfValueKindForCode(n)) {
        case DxfValueKind::Str:ok=writer->writeString(n,value);break;
        case DxfValueKind::Dbl:ok=writer->writeDouble(n,std::stod(value));break;
        case DxfValueKind::I16:ok=writer->writeInt16(n,std::stoi(value));break;
        case DxfValueKind::I32:ok=writer->writeInt32(n,std::stoi(value));break;
        case DxfValueKind::I64:ok=writer->writeInt64(n,std::stoll(value));break;
        case DxfValueKind::Bln:ok=writer->writeBool(n,std::stoi(value)!=0);break;
        default:return false;
        }
        if(!ok)return false;
    }
    result=bytes.str();return static_cast<bool>(bytes);
}
static bool generateInputs(const std::filesystem::path& directory) {
    const std::string ascii=sourceText();auto hidden=ascii;
    const std::string marker="5\n1E1\n";const auto at=hidden.find(marker);
    if(at==std::string::npos||hidden.find(marker,at+marker.size())!=std::string::npos)return false;
    hidden.insert(at+marker.size(),"102\n{TEST_HIDDEN\n340\nD\n102\n}\n");
    auto comments=ascii;comments.insert(at+marker.size(),"999\nignored inside LAYER\n");
    std::string cleanBinary,hiddenBinary,commentBinary;
    if(!encodeBinary(ascii,cleanBinary)||!encodeBinary(hidden,hiddenBinary)||!encodeBinary(comments,commentBinary))return false;
    for(const auto& entry:std::vector<std::pair<std::string,std::string>>{{"ascii",ascii},{"binary",cleanBinary}}) {
        const auto& kind=entry.first;const auto& data=entry.second;
        if(data.size()<7u||!writeFile(directory/("clean-"+kind+".dxf"),data)
            ||!writeFile(directory/("hidden-"+kind+".dxf"),kind=="ascii"?hidden:hiddenBinary)
            ||!writeFile(directory/("comments-"+kind+".dxf"),kind=="ascii"?comments:commentBinary)
            ||!writeFile(directory/("trailing-"+kind+".dxf"),data+"TRAILING")
            ||!writeFile(directory/("truncated-"+kind+".dxf"),data.substr(0,data.size()-7u)))return false;
    }
    return true;
}
int main() {
    const auto directory=std::filesystem::temp_directory_path()/
        ("dxf-whole-source-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);struct Clean {std::filesystem::path p;~Clean(){std::error_code ec;std::filesystem::remove_all(p,ec);}} cleanup{directory};
    const std::string root=directory.string()+"/";
    if(!generateInputs(directory))return 27;
    std::size_t arithmetic=0;if(sum((std::numeric_limits<std::size_t>::max)(),1u,arithmetic)
       ||product((std::numeric_limits<std::size_t>::max)(),2u,arithmetic))return 1;
    unsigned versions=0u;
    for(const char* version:{"AC1009","AC1012","AC1014","AC1015","AC1018","AC1021","AC1024","AC1027","AC1032"}) {
        const std::string text=std::string("0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\n")+version+"\n0\nENDSEC\n0\nEOF\n";
        std::string binary;if(!encodeBinary(text,binary))return 28;
        for(bool isBinary:{false,true}) {
            const auto path=directory/(std::string("version-")+version+(isBinary?"-binary.dxf":"-ascii.dxf"));
            if(!writeFile(path,isBinary?binary:text))return 29;
            auto adapter=std::make_unique<Consumer>();adapter->enableWholeDxfSourceObservation();
            auto data=std::make_unique<dx_data>();if(!adapter->import(path.string(),*data)
                ||!ConsumerAccess::get(*adapter,*data)||ConsumerAccess::get(*adapter,*data)->binary()!=isBinary)return 30;
            ++versions;
        }
    }
    std::cout<<"version-header-captures="<<versions<<"\n";
    for(const char* kind:{"ascii","binary"}) {
        const auto clean=root+"clean-"+kind+".dxf",unsafe=root+"hidden-"+kind+".dxf";
        auto c=std::make_unique<Adversary>();auto data=std::make_unique<dx_data>();
        c->enableWholeDxfSourceObservation(false);if(!c->import(clean,*data)||ConsumerAccess::get(*c,*data))return 2;
        c->enableWholeDxfSourceObservation(true);data=std::make_unique<dx_data>();if(!c->import(clean,*data)||!ConsumerAccess::get(*c,*data))return 3;
        const auto first=ConsumerAccess::get(*c,*data);if(first->fieldCount()!=315u||hidden(*first))return 4;
        auto other=std::make_unique<dx_data>();if(ConsumerAccess::get(*c,*other))return 22;
        const auto originalGeneration=data->importGeneration;const auto originalVersion=data->sourceVersion;
        ++data->importGeneration;if(ConsumerAccess::get(*c,*data))return 23;data->importGeneration=originalGeneration;
        data->sourceVersion=DRW::UNKNOWNV;if(ConsumerAccess::get(*c,*data))return 24;data->sourceVersion=originalVersion;
        auto peak=first->peakBytes();auto work=first->workUsed();auto fields=first->fieldCount();
        const auto baseline=first->field(0).code;
        data=std::make_unique<dx_data>();if(!c->import(unsafe,*data)||!ConsumerAccess::get(*c,*data)||!hidden(*ConsumerAccess::get(*c,*data))||first==ConsumerAccess::get(*c,*data))return 5;
        data=std::make_unique<dx_data>();if(!c->import(root+"comments-"+kind+".dxf",*data)||!ConsumerAccess::get(*c,*data))return 38;
        bool ignored=false;const auto commentSource=ConsumerAccess::get(*c,*data);
        for(std::size_t i=0;i<commentSource->fieldCount();++i){const auto f=commentSource->field(i);
            if(f.code==999&&f.ignored&&commentSource->bounds(f)&&commentSource->value(f)=="ignored inside LAYER")ignored=true;}
        if(!ignored)return 39;
        for(auto mode:{Adversary::Mode::Before,Adversary::Mode::Skip,Adversary::Mode::Duplicate,Adversary::Mode::Cross,
                Adversary::Mode::WrongData,Adversary::Mode::Generation,Adversary::Mode::SkipFinish,Adversary::Mode::DuplicateFinish,Adversary::Mode::QuotaPoison}) {
            c->mode=mode;if(mode==Adversary::Mode::QuotaPoison)DxWholeSourceTestAccess::limits(*c,Limits{1000000u,64u*1024u*1024u,work});
            data=std::make_unique<dx_data>();if(!c->import(clean,*data)||ConsumerAccess::get(*c,*data))return 6;
            DxWholeSourceTestAccess::limits(*c,{});
        }
        c->mode=Adversary::Mode::CrossTicket;c->reentryPath=unsafe;c->foreignHit=false;c->foreignRead=false;
        data=std::make_unique<dx_data>();if(!c->import(clean,*data)||!c->foreignHit||!c->foreignRead
            ||ConsumerAccess::get(*c,*data))return 40;
        c->mode=Adversary::Mode::CrossFinish;c->reentryPath=unsafe;c->foreignHit=false;c->foreignRead=false;
        data=std::make_unique<dx_data>();if(!c->import(clean,*data)||!c->foreignHit||!c->foreignRead
            ||ConsumerAccess::get(*c,*data))return 42;
        c->mode=Adversary::Mode::CopyField;data=std::make_unique<dx_data>();
        if(!c->import(clean,*data)||!ConsumerAccess::get(*c,*data))return 14;
        c->mode=Adversary::Mode::After;data=std::make_unique<dx_data>();if(!c->import(clean,*data)||!ConsumerAccess::get(*c,*data)
            ||ConsumerAccess::get(*c,*data)->field(0).code!=baseline||first->field(0).code!=baseline
            ||ConsumerAccess::get(*c,*data)->value(ConsumerAccess::get(*c,*data)->field(0))!="SECTION")return 7;
        c->mode=Adversary::Mode::ThrowRecord;data=std::make_unique<dx_data>();bool caught=false;
        try{(void)c->import(clean,*data);}catch(const std::runtime_error&){caught=true;}
        if(!caught||ConsumerAccess::get(*c,*data))return 25;
        c->mode=Adversary::Mode::None;data=std::make_unique<dx_data>();
        if(!c->import(clean,*data)||!ConsumerAccess::get(*c,*data))return 26;
        data=std::make_unique<dx_data>();if(!c->import(root+"trailing-"+kind+".dxf",*data)||ConsumerAccess::get(*c,*data))return 8;
        data=std::make_unique<dx_data>();if(c->import(root+"truncated-"+kind+".dxf",*data)||ConsumerAccess::get(*c,*data))return 9;
        DxWholeSourceTestAccess::limits(*c,Limits{fields,peak,work});data=std::make_unique<dx_data>();if(!c->import(clean,*data)||!ConsumerAccess::get(*c,*data))return 10;
        for(int i=0;i<3;++i) {
            auto limited=Limits{fields,peak,work};if(i==0)--limited.fields;if(i==1)--limited.bytes;if(i==2)--limited.work;
            DxWholeSourceTestAccess::limits(*c,limited);
            data=std::make_unique<dx_data>();if(!c->import(clean,*data)||ConsumerAccess::get(*c,*data))return 11;
            DxWholeSourceTestAccess::limits(*c,Limits{fields,peak,work});data=std::make_unique<dx_data>();if(!c->import(clean,*data)||!ConsumerAccess::get(*c,*data))return 12;
        }
        DxWholeSourceTestAccess::limits(*c,{});c->mode=Adversary::Mode::None;data=std::make_unique<dx_data>();
        if(!c->import(clean,*data)||!ConsumerAccess::get(*c,*data))return 31;
        if(c->import(root+"missing-file.dxf",*data)||ConsumerAccess::get(*c,*data))return 32;
        data=std::make_unique<dx_data>();if(!c->import(clean,*data)||!ConsumerAccess::get(*c,*data))return 33;
        c->mode=Adversary::Mode::ThrowRecord;caught=false;
        try{(void)c->import(clean,*data);}catch(const std::runtime_error&){caught=true;}
        if(!caught||ConsumerAccess::get(*c,*data))return 34;
        c->mode=Adversary::Mode::None;data=std::make_unique<dx_data>();
        if(!c->import(clean,*data)||!ConsumerAccess::get(*c,*data))return 35;
        c->mode=Adversary::Mode::Reentry;c->reentryPath=clean;c->reentryHit=false;data=std::make_unique<dx_data>();
        (void)c->import(clean,*data);if(!c->reentryHit||c->reentryAccepted||ConsumerAccess::get(*c,*data))return 36;
        c->mode=Adversary::Mode::None;data=std::make_unique<dx_data>();
        if(!c->import(clean,*data)||!ConsumerAccess::get(*c,*data))return 37;
        for(std::size_t i=0;i<first->fieldCount();++i) {const auto& f=first->field(i);if(!first->bounds(f))return 13;
            if(first->binary()&&f.kind!=DxfReadRecordView::Text&&f.kind!=DxfReadRecordView::Binary
            &&(!first->value(f).empty()||!first->raw(f).empty()))return 13;}
        auto bad=first->field(0);bad.rawOffset=(std::numeric_limits<std::size_t>::max)();
        if(first->bounds(bad)||!first->raw(bad).empty()||!first->value(bad).empty())return 15;
        { Bare bare;dxfRW direct(clean.c_str());direct.setDxfReadRetainedByteBudget(50000u);bare.enabled=false;
          if(!direct.read(&bare,false)||bare.hasAuthority())return 16;
          bare.enabled=true;if(direct.read(&bare,false)||bare.hasAuthority())return 17;
          direct.setDxfReadRetainedByteBudget(4u*1024u*1024u);
          if(!direct.read(&bare,false)||!bare.hasAuthority())return 18;
          if(!direct.read(&bare,true)||!bare.hasAuthority())return 41;
        }
        if(std::string(kind)=="ascii") {
            std::ifstream file(clean,std::ios::binary);std::string content((std::istreambuf_iterator<char>(file)),{});
            Bare bare;dxfRW direct("");if(!direct.readAscii(&bare,false,content)||!bare.hasAuthority())return 19;
            auto noNewline=content;if(!noNewline.empty()&&noNewline.back()=='\n')noNewline.pop_back();
            if(!direct.readAscii(&bare,false,noNewline)||!bare.hasAuthority())return 20;
            content+="TRAILING";if(!direct.readAscii(&bare,false,content)||bare.hasAuthority())return 21;
        }
        std::cout<<kind<<" off=pass hidden=observed ignored-comment=observed cross-reader-tickets=refused minting-api=private hostile-callbacks=9 refused context/throw-retry=pass metadata-copy-isolation=pass after-base-byte-isolation=pass trailing=refused truncated=refused exact=pass 3-under/retry=pass shared-cap-fail/retry=pass stale/reentry-retry=pass fields="<<fields<<" peak="<<peak<<" work="<<work<<"\n";
    }
}
