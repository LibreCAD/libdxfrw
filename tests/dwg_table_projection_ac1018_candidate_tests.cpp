#include "dx_iface.h"
#include "intern/dwg_table_projection_ac1018_candidate.h"
#include <cstdlib>
#include <new>
namespace fault {long remaining=-1;std::size_t count=0;bool active=false;}
void* operator new(std::size_t n){if(fault::active)++fault::count;if(fault::remaining==0){fault::remaining=-1;throw std::bad_alloc();}if(fault::remaining>0)--fault::remaining;if(void* p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace {
DRW_Table source(){DRW_Table t;t.handle=0x9cu;t.m_hasSemanticContent=true;t.m_semanticContentComplete=true;
    t.numProxyGraph=2104;t.proxyGraphics=std::string(2104,'p');t.m_content.m_columns.resize(3);t.m_content.m_rows.resize(3);
    for(auto& row:t.m_content.m_rows){row.m_cells.resize(3);for(auto& cell:row.m_cells){cell.m_contents.resize(1);cell.m_contents[0].m_value.m_value=DRW_Variant(1,UTF8STRING("x"));}}
    return t;}
DRW_Insert projected(const DRW_Table& t){DRW_Insert result(t);result.numProxyGraph=0;result.proxyGraphics.clear();result.revokeProxyGraphicsReplayAuthorization();return result;}
class InsertOnly final:public dx_iface {public:int calls=0;std::uint32_t handle=0;bool sawTable=false;
    void addInsert(const DRW_Insert& value) override{++calls;handle=value.handle;sawTable=dynamic_cast<const DRW_Table*>(&value)!=nullptr;}};
class DerivedInsert final:public DRW_Insert {public:explicit DerivedInsert(const DRW_Insert& v):DRW_Insert(v){}};
}
int main(){
    auto table=source();auto insert=projected(table);using DwgTableProjectionAc1018Candidate::matches;
    if(!matches(table,insert))return 1;
    auto wrong=insert;wrong.basePoint.x=1;if(matches(table,wrong))return 2;
    wrong=insert;wrong.xDictHandle=1;if(matches(table,wrong))return 3;
    wrong=insert;wrong.layer="other";if(matches(table,wrong))return 4;
    wrong=insert;wrong.numProxyGraph=1;wrong.proxyGraphics="x";if(matches(table,wrong))return 5;
    wrong=insert;wrong.attlist.push_back(std::make_shared<DRW_Attrib>());if(matches(table,wrong))return 6;
    wrong=insert;wrong.extData.push_back(std::make_shared<DRW_Variant>(1,UTF8STRING("x")));if(matches(table,wrong))return 7;
    wrong=insert;wrong.name=std::string(257,'x');if(matches(table,wrong))return 8;
    wrong=insert;wrong.eType=DRW::TABLE;if(matches(table,wrong))return 9;
    auto broken=table;broken.m_content.m_rows[0].m_cells.clear();if(matches(broken,insert))return 10;
    fault::count=0;fault::active=true;const bool good=matches(table,insert);fault::active=false;const auto count=fault::count;
    if(!good||!count)return 11;for(std::size_t i=0;i<count;++i){fault::remaining=static_cast<long>(i);if(matches(table,insert))return 12;
        fault::remaining=-1;if(!matches(table,insert))return 13;}
    InsertOnly legacy;legacy.DRW_Interface::addTable(table);if(legacy.calls!=1||legacy.handle!=table.handle||!legacy.sawTable)return 14;
    DerivedInsert derived(insert);if(matches(table,derived))return 15;
    return 0;
}
