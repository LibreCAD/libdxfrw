// Locally authored source, normalized with frame/page checksums for a reader-only
// graph test; the production writer's populated TABLE output gate remains closed.
#define main p5_prior_candidate_main
#include "dwg_table_candidate_binding_tests.cpp"
#undef main
#include "intern/dwg_table_entity_xdictionary_ac1018_candidate.h"
#include "intern/dwg_fixed_handles.h"
class DrwEntityEncodeTestAccess {public:static void markParsedProxyGraphics(DRW_Entity& e,DRW::Version v){e.m_proxyGraphicsProvenance={DRW_ProxyGraphicsOrigin::ParsedDwg,DRW_ProxyGraphicsEncoding::DwgBitStream,v,"",0,static_cast<std::uint64_t>(e.proxyGraphics.size()),false,false};}};
class TableStyleNodWholeFileAdapter:public dx_iface {
public:
 TableStyleNodWholeFileAdapter(dwgRW* w,dx_data* d,bool,bool,bool):m_writer(w){cData=d;
  dictionary.handle=0x6au;dictionary.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;dictionary.cloning=1;dictionary.hardOwner=1;dictionary.m_entries.push_back({"Standard",0x6bu});
  style.handle=0x6bu;style.parentHandle=0x6au;style.m_name="Standard";
  for(int i=0;i<3;++i){DRW_TableStyleRowStyle r;r.m_textHeight=1.+i;r.m_textAlignment=5;r.m_textStyleHandle=DRW::DwgStandardTextStyleHandle;for(int j=0;j<6;++j){DRW_TableStyleBorder b;b.m_edgeFlags=1<<j;r.m_borders.push_back(b);}style.m_rowStyles.push_back(r);}
  table.handle=0x9cu;table.parentHandle=DRW::NoHandle;table.name="*T1";table.basePoint=DRW_Coord(105.,145.,0.);table.extPoint=DRW_Coord(0.,0.,1.);table.m_horizontalDirection=DRW_Coord(0.,1.,0.);table.colcount=3;table.rowcount=3;table.m_hasSemanticContent=true;table.m_semanticContentComplete=true;table.m_tableStyleHandle=0x6bu;table.m_content.m_tableStyleHandle=0x6bu;
  for(int i=0;i<3;++i){DRW_TableColumn c;c.m_width=152.;table.m_content.m_columns.push_back(c);}
  const char* values[3][3]={{"Title","",""},{"1","2","3"},{"4","5","6"}};for(int i=0;i<3;++i){DRW_TableRow r;r.m_height=i?9.:11.;for(int j=0;j<3;++j){DRW_TableCell c;c.m_type=1;c.m_mergedWidth=i==0&&j==0?3:1;c.m_mergedHeight=1;c.m_isMerged=i==0&&j!=0;DRW_TableCellContent v;v.m_type=1;v.m_text=values[i][j];c.m_contents.push_back(v);r.m_cells.push_back(c);}table.m_content.m_rows.push_back(r);}
 }
 void setExtensionMode(int){table.xDictHandle=0xbbu;}void useParsedBlockRecordMetadata(bool){}void useParsedProxyCache(bool){}
 void writeHeader(DRW_Header& h) override {h.vars.clear();}
 void collectDwgAppIds() override {}void writeBlockRecords() override {}void writeLTypes() override {}void writeLayers() override {}void writeTextstyles() override {}void writeVports() override {}void writeViews() override {}void writeUCSs() override {}void writeDimstyles() override {}void writeAppId() override {}
 void writeDwgClasses() override {ready=m_writer->registerDwgNamedObjectDictionaryEntry("ACAD_TABLESTYLE",0x6au)&&m_writer->registerTableEntityClass(&table)&&m_writer->registerTableStyleObjectClass(&style);}
 void writeBlocks() override {const auto h=m_writer->defineBlock("*T1",DRW_Coord(0,0,0));if(!h || !m_writer->beginBlockContent(h)){ready=false;return;}table.blockRecH.ref=h;table.proxyGraphics=std::string(2104u,'X');table.numProxyGraph=2104;DrwEntityEncodeTestAccess::markParsedProxyGraphics(table,DRW::AC1018);if(!table.authorizeParsedDwgProxyGraphicsReplay()){ready=false;return;}DRW_Line line;line.basePoint=DRW_Coord(0,0,0);line.secPoint=DRW_Coord(1,0,0);if(!m_writer->writeLine(&line)||!m_writer->endBlockContent())ready=false;}
 void writeEntities() override {if(!m_writer->writeTable(&table))ready=false;}
 bool finalizeDwgWrite() override{return ready;}bool finalizeDwgWriteStructure() override{return true;}
 dwgRW* m_writer;DRW_Dictionary dictionary;DRW_TableStyle style;DRW_Table table;bool ready=false;
};
class CompleteWriter final:public TableStyleNodWholeFileAdapter {
public:
 CompleteWriter(dwgRW* w,dx_data* d,const char* contentPath,const char* dataPath,const char* geometryPath,const char* cellStyleMapPath):TableStyleNodWholeFileAdapter(w,d,true,false,false) {
  setExtensionMode(6);useParsedBlockRecordMetadata(true);useParsedProxyCache(true);
  style.m_name="Standar";dictionary.m_entries[0].m_name="Standar";style.xDictHandle=0xdbu;style.reactorHandles={0x6au,0x9cu};style.setDwgCommonObjectState(2,0u,false);
  style.m_horizontalCellMargin=style.m_verticalCellMargin=1.5;
  const double heights[]={4.5,6.0,4.5};const int align[]={2,5,5};
  for(std::size_t i=0;i<3u;++i){auto& r=style.m_rowStyles[i];r.m_textHeight=heights[i];r.m_textAlignment=align[i];r.m_fillColor=256;r.m_textColorCmc.m_method=0xc1u;r.m_fillColorCmc.m_method=0xc8u;r.m_fillColorCmc.setRawDwgPackedValue(0xc8000000u);for(auto& b:r.m_borders){b.m_lineWeight=-2;b.m_visible=1;b.m_colorCmc.m_method=0xc1u;}}
  a=raw(contentPath,514,0xb9u,"TABLECONTENT","AcDbTableContent");
  b=raw(dataPath,517,0xbdu,"DATATABLE","AcDbDataTable");
  g=raw(geometryPath,515,0xbau,"TABLEGEOMETRY","AcDbTableGeometry");
  cellStyleMap=raw(cellStyleMapPath,508,0xddu,"CELLSTYLEMAP","AcDbCellStyleMap");
 }
 DRW_UnsupportedObject raw(const char* path,int type,std::uint32_t h,const char* name,const char* cls){DRW_UnsupportedObject r;std::ifstream f(path,std::ios::binary);r.m_rawBytes.assign(std::istreambuf_iterator<char>(f),{});r.m_version=DRW::AC1018;r.m_objectType=type;r.m_handle=h;r.m_objectSize=r.m_rawBytes.size();r.m_isCustomClass=r.m_hasClassDefinition=true;r.m_recordName=name;r.m_className=cls;r.m_classAppName="LOCALLY_AUTHORED";r.m_classProxyFlag=0x401u;r.m_classEntityFlagRaw=0x1f3u;return r;}
 void writeDwgClasses() override {TableStyleNodWholeFileAdapter::writeDwgClasses();ready=ready&&m_writer->registerRawDwgObjectClass(&cellStyleMap)&&m_writer->registerRawDwgObjectClass(&a)&&m_writer->registerRawDwgObjectClass(&b)&&m_writer->registerRawDwgObjectClass(&g);std::cerr<<"registered="<<ready<<" table class="<<table.getDwgClassNum()<<'\n';}
 void writeObjects() override {
  if(!ready || !m_writer->writeDictionary(&dictionary) || !m_writer->writeTableStyle(&style)){ready=false;std::cerr<<"style failed\n";return;}
  DRW_Dictionary root;root.handle=0xbbu;root.parentHandle=0x9cu;root.hardOwner=1;root.cloning=1;root.setDwgCommonObjectState(0,1u,false);DRW_Dictionary populated=root;populated.handle=0xe0u;populated.m_entries.push_back({"ACAD_XREC_ROUNDTRI",0xbcu});
  DRW_Dictionary sty;sty.handle=0xdbu;sty.parentHandle=0x6bu;sty.hardOwner=1;sty.cloning=1;sty.setDwgCommonObjectState(0,1u,false);sty.m_entries.push_back({"ACAD_XREC_ROUNDTR",0xdcu});sty.m_entries.push_back({"ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP",0xddu});
  ready=m_writer->writeDictionary(&root) && m_writer->writeDictionary(&populated) && m_writer->writeDictionary(&sty);std::cerr<<"dicts="<<ready<<'\n';
  for(unsigned h=0xbcu;h<=0xc6u;++h){if(h==0xbdu)continue;DRW_XRecord x;x.handle=h;x.parentHandle=h==0xbcu?0xe0u:0xbdu;
   if(h==0xbcu){x.reactorHandles={0xe0u};x.setDwgCommonObjectState(1,1u,false);x.m_cloning=1;auto&e=x.m_dataEntries;e.emplace_back(102,UTF8STRING("ACAD_ROUNDTRIP_2008_TABLE_ENTITY"));e.emplace_back(360,std::uint64_t{0xb9u});e.emplace_back(70,std::int32_t{2});e.emplace_back(90,std::int32_t{1});e.emplace_back(10,DRW_Coord(0,0,0));e.emplace_back(90,std::int32_t{0});e.emplace_back(90,std::int32_t{2});e.emplace_back(361,std::uint64_t{0xbau});e.emplace_back(102,UTF8STRING("ACAD_ROUNDTRIP_PRE2007_TABLE"));e.emplace_back(90,std::int32_t{3});e.emplace_back(91,std::int32_t{3});e.emplace_back(102,UTF8STRING("ACAD_ROUNDTRIP_PRE2007_TABLECELL"));e.emplace_back(360,std::uint64_t{0xbdu});}
   else {ChildFixture::Options o;o.handle=h;o.empty=(h==0xbfu || h==0xc0u);if(h>=0xc1u)o.value=std::to_string(h-0xc0u);x=ChildFixture::build(o).typed;}
   if(!m_writer->writeXRecord(&x)){std::cerr<<"x fail "<<std::hex<<h<<'\n';ready=false;return;}}
  DRW_XRecord x;x.handle=0xdcu;x.parentHandle=0xdbu;x.reactorHandles={0xdbu};x.setDwgCommonObjectState(1,1u,false);x.m_cloning=1;auto&e=x.m_dataEntries;e.emplace_back(102,UTF8STRING("ACAD_ROUNDTRIP_PRE2007_TABLESTYLE"));e.emplace_back(90,std::int32_t{4});e.emplace_back(91,std::int32_t{0});e.emplace_back(1,UTF8STRING(""));e.emplace_back(92,std::int32_t{4});e.emplace_back(93,std::int32_t{0});e.emplace_back(2,UTF8STRING(""));e.emplace_back(94,std::int32_t{4});e.emplace_back(95,std::int32_t{0});e.emplace_back(3,UTF8STRING(""));
  ready=ready&&m_writer->writeXRecord(&x)&&m_writer->writeRawDwgObject(&cellStyleMap)&&m_writer->writeRawDwgObject(&a)&&m_writer->writeRawDwgObject(&b)&&m_writer->writeRawDwgObject(&g);std::cerr<<"objects="<<ready<<'\n';
 }
 DRW_UnsupportedObject a,b,g,cellStyleMap;
};
namespace {
void localHandle(dwgBufferW& b,unsigned role,std::uint32_t h){dwgHandle x;x.code=role;x.ref=h;b.putHandle(x);}
std::vector<std::uint8_t> styleXBody(int value=4){
 dwgBufferW b;b.putObjType(DRW::AC1018,79u);const auto end=b.bitCount();b.putRawLong32(0u);localHandle(b,0u,0xdcu);b.putBitShort(0u);b.putBitLong(1);b.putBit(1u);b.putBitLong(89);
 const auto text=[&](unsigned code,const std::string& s){b.putRawShort16(code);b.putRawShort16(static_cast<std::uint16_t>(s.size()));b.putRawChar8(45u);for(char c:s)b.putRawChar8(static_cast<std::uint8_t>(c));};
 const auto integer=[&](unsigned code,int v){b.putRawShort16(code);b.putRawLong32(static_cast<std::uint32_t>(v));};
 text(102,"ACAD_ROUNDTRIP_PRE2007_TABLESTYLE");integer(90,value);integer(91,0);text(1,"");integer(92,4);integer(93,0);text(2,"");integer(94,4);integer(95,0);text(3,"");b.putBitShort(1u);b.patchRawLong32AtBit(end,static_cast<std::uint32_t>(b.bitCount()));localHandle(b,4u,0xdbu);localHandle(b,4u,0xdbu);b.alignToByte();return b.data();
}
std::vector<std::uint8_t> fileBytes(const char* path){std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
bool enabled(dx_iface& a){return a.enableVerifiedTableCellStyleMapSource(fullXrecords,0xb9u,0xbdu,0xbau,0x9cu,0xe0u,0x6bu,0xdbu,0xdcu,0xddu);}
bool verified(const dx_data& d){DwgTableStyleRoundtripXRecordAc1018Candidate::Result x;DwgCellStyleMapAc1018Candidate::Result map;
 const bool xOk=dx_iface::readVerifiedTableStyleXRecordSource(d,fullXrecords,0xb9u,0xbdu,0xbau,0x9cu,0xe0u,0x6bu,0xdbu,0xdcu,0xddu,x);
 const bool mapOk=dx_iface::readVerifiedTableCellStyleMapSource(d,fullXrecords,0xb9u,0xbdu,0xbau,0x9cu,0xe0u,0x6bu,0xdbu,0xdcu,0xddu,map);
 const bool ok=xOk&&x.handle==0xdcu&&x.owner==0xdbu&&x.reactor==0xdbu&&mapOk&&map.handle==0xddu&&map.owner==0xdbu&&map.styles.size()==3u&&map.styles[0].id==3&&map.styles[1].id==1&&map.styles[2].id==2;
 if(!ok)std::cerr<<"verified bundle x="<<xOk<<" xHandle="<<x.handle<<" xOwner="<<x.owner<<" xReactor="<<x.reactor<<" map="<<mapOk<<" mapHandle="<<map.handle<<" mapOwner="<<map.owner<<" styles="<<map.styles.size()<<'\n';return ok;}
bool patchTable(const std::filesystem::path& path){
 std::ifstream input(path,std::ios::binary);std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
 const auto found=std::search_n(bytes.cbegin(),bytes.cend(),2000u,0xc2u);if(found==bytes.cend())return false;const auto at=static_cast<std::size_t>(found-bytes.cbegin());if(at<100u)return false;
 for(std::size_t i=at-100u;i<at;++i){dwgBuffer buf(bytes.data(),bytes.size(),nullptr);DwgObjectFrame frame;if(!frame.readAt(buf,DRW::AC1018,i)||frame.body().size()<2104u)continue;
  DRW_UnsupportedObject entity;entity.m_rawBytes=frame.body();dwgBuffer head(entity.m_rawBytes.data(),entity.m_rawBytes.size(),nullptr);entity.m_objectType=head.getObjType(DRW::AC1018);entity.m_handle=0x9cu;entity.m_version=DRW::AC1018;entity.m_isEntity=entity.m_isCustomClass=entity.m_hasClassDefinition=true;entity.m_recordName="ACAD_TABLE";entity.m_className="AcDbTable";entity.m_objectSize=static_cast<std::uint32_t>(entity.m_rawBytes.size());
  DwgTableEntityXDictionaryAc1018Candidate::Result r;if(!DwgTableEntityXDictionaryAc1018Candidate::inspect(entity,r))continue;if(r.dictionary!=0xbbu)return false;
  auto changed=entity.m_rawBytes;patchRawBits(changed,r.dictionaryStartBit+8u,0xe0u,1u);return replaceSelectedBody(path,entity,changed);
 }return false;
}
bool execute(const char* content,const char* data,const char* geometry,const char* rootPath,const char* stylePath,const char* styleDictPath,const char* cellStyleMapPath,const std::filesystem::path& path){
#define CHECK_F(x) do{if(!(x)){std::cerr<<"full bundle failed at line "<<__LINE__<<'\n';return false;}}while(false)
 dx_data writerData;dwgRW writer(path.string().c_str());for(unsigned h=0xb9u;h<=0xddu;++h)writer.reserveHandle(h);writer.reserveHandle(0xe0u);writer.reserveHandle(0x9cu);writer.reserveHandle(0x6au);writer.reserveHandle(0x6bu);
 CompleteWriter authored(&writer,&writerData,content,data,geometry,cellStyleMapPath);CHECK_F(writer.write(&authored,DRW::AC1018,true)&&authored.ready);
 dx_iface initialReader;dx_data initial;CHECK_F(initialReader.fileImport(path.string(),&initial,false));
 auto root=fileBytes(rootPath);patchRawBits(root,1675u,0xe0u,1u);patchRawBits(root,1691u,0xe0u,1u);CHECK_F(replaceSelectedBody(path,DxIfaceStandardDwgCaptureTestAccess::standardRaw(initial,0xbcu),root));
 for(unsigned h=0xbeu;h<=0xc6u;++h){ChildFixture::Options o;o.handle=h;o.empty=(h==0xbfu||h==0xc0u);if(h>=0xc1u)o.value=std::to_string(h-0xc0u);CHECK_F(replaceSelectedBody(path,DxIfaceStandardDwgCaptureTestAccess::standardRaw(initial,h),ChildFixture::build(o).raw.m_rawBytes));}
 auto dict=DxIfaceStandardDwgCaptureTestAccess::standardRaw(initial,0xe0u);auto dictBody=dict.m_rawBytes;patchRawBits(dictBody,245u,'P',1u);CHECK_F(replaceSelectedBody(path,dict,dictBody));
 auto style=DxIfaceStandardDwgCaptureTestAccess::raw(initial,0x6bu);auto styleBody=fileBytes(stylePath);patchRawBits(styleBody,2u,static_cast<unsigned>(style.m_objectType),2u);CHECK_F(replaceSelectedBody(path,style,styleBody));
 CHECK_F(replaceSelectedBody(path,DxIfaceStandardDwgCaptureTestAccess::standardRaw(initial,0xdbu),fileBytes(styleDictPath)));
 CHECK_F(replaceSelectedBody(path,DxIfaceStandardDwgCaptureTestAccess::standardRaw(initial,0xdcu),styleXBody()));CHECK_F(patchTable(path));
 dx_iface reader;CHECK_F(enabled(reader));dx_data baseline;CHECK_F(reader.fileImport(path.string(),&baseline,false)&&verified(baseline));
 const auto output=path.string()+".dxf";CHECK_F(!std::filesystem::exists(output));{std::ofstream f(output,std::ios::binary|std::ios::trunc);f<<"FULL_BUNDLE_SENTINEL";CHECK_F(static_cast<bool>(f));}
 CHECK_F(!reader.fileExport(output,DRW::AC1018,false,&baseline,false,true,dx_dxfSourceTextPolicy::Compatible,DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete));
 {std::ifstream f(output,std::ios::binary);const std::string s((std::istreambuf_iterator<char>(f)),{});CHECK_F(s=="FULL_BUNDLE_SENTINEL");std::error_code e;std::filesystem::remove(output,e);}
 auto x=DxIfaceStandardDwgCaptureTestAccess::standardRaw(baseline,0xdcu);auto changed=styleXBody(5);CHECK_F(replaceSelectedBody(path,x,changed));
 dx_iface previous;CHECK_F(previous.enableVerifiedTableStyleDictionarySource(fullXrecords,0xb9u,0xbdu,0xbau,0x9cu,0xe0u,0x6bu,0xdbu,0xdcu,0xddu));dx_data prior;CHECK_F(previous.fileImport(path.string(),&prior,false));DwgTableStyleExtensionDictionaryAc1018Candidate::Result priorResult;CHECK_F(dx_iface::readVerifiedTableStyleDictionarySource(prior,fullXrecords,0xb9u,0xbdu,0xbau,0x9cu,0xe0u,0x6bu,0xdbu,0xdcu,0xddu,priorResult));
 dx_data altered;CHECK_F(reader.fileImport(path.string(),&altered,false)&&!verified(altered)&&!dx_iface::hasUnchangedVerifiedXRecordSource(altered,0xbcu));
 auto alteredRaw=x;alteredRaw.m_rawBytes=changed;CHECK_F(replaceSelectedBody(path,alteredRaw,styleXBody()));dx_data restored;CHECK_F(reader.fileImport(path.string(),&restored,false)&&verified(restored));
 std::vector<std::uint8_t> before;CHECK_F(corruptSelectedCrc(path,x,before));dx_data broken;(void)reader.fileImport(path.string(),&broken,false);CHECK_F(!verified(broken)&&!dx_iface::hasUnchangedVerifiedXRecordSource(broken,0xbcu));
 const auto& integrity=broken.dwgReadOutcome.observations.integrity;CHECK_F(!integrity.extractionOmitted&&!integrity.reportedUpstreamDropped&&!integrity.aggregateSaturated);bool target=false;for(std::size_t i=0;i<integrity.retained;++i){const auto& row=integrity.rows[i];CHECK_F(row.kind!=DwgIntegrityCheckKind::DataPageChecksum&&row.kind!=DwgIntegrityCheckKind::DataPageCrc);if(row.kind==DwgIntegrityCheckKind::ObjectFrameBounds&&row.hasLogicalHandle&&row.logicalHandle==0xdcu)target=true;}CHECK_F(target);
 {std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(before.data()),before.size());CHECK_F(static_cast<bool>(out));}dx_data retry;CHECK_F(reader.fileImport(path.string(),&retry,false)&&verified(retry));
 std::cout<<"locally authored full style XRECORD/CELLSTYLEMAP bundle, XRECORD CRC failure and same-reader retry passed\n";return true;
#undef CHECK_F
}
}
int main(int argc,char** argv){if(argc!=9)return 2;const std::filesystem::path path(argv[8]);if(std::filesystem::exists(path))return 3;const bool ok=execute(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],argv[7],path);std::error_code e;std::filesystem::remove(path,e);return ok?0:1;}
