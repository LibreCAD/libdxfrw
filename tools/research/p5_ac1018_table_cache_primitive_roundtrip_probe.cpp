#include "dx_iface.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

static bool near(double a, double b) {
    return std::isfinite(a) && std::isfinite(b)
        && std::abs(a-b) <= 1e-10 * std::max({1.0, std::abs(a), std::abs(b)});
}
static bool point(const DRW_Coord& a, const DRW_Coord& b) {
    return near(a.x,b.x) && near(a.y,b.y) && near(a.z,b.z);
}
static bool common(const DRW_Entity& a, const DRW_Entity& b) {
    return a.eType==b.eType && a.layer==b.layer && a.color==b.color
        && a.color24==b.color24 && a.lineType==b.lineType
        && a.lWeight==b.lWeight && a.visible==b.visible
        && near(a.ltypeScale,b.ltypeScale) && a.transparency==b.transparency;
}
static bool equalEntity(const DRW_Entity& a, const DRW_Entity& b) {
    if(!common(a,b)) return false;
    if(a.eType==DRW::LINE) {
        const auto* x=dynamic_cast<const DRW_Line*>(&a);
        const auto* y=dynamic_cast<const DRW_Line*>(&b);
        return x&&y&&point(x->basePoint,y->basePoint)&&point(x->secPoint,y->secPoint)
            &&point(x->extPoint,y->extPoint)&&near(x->thickness,y->thickness);
    }
    if(a.eType==DRW::MTEXT) {
        const auto* x=dynamic_cast<const DRW_MText*>(&a);
        const auto* y=dynamic_cast<const DRW_MText*>(&b);
        return x&&y&&x->text==y->text&&x->style==y->style
            &&point(x->basePoint,y->basePoint)&&point(x->secPoint,y->secPoint)
            &&point(x->extPoint,y->extPoint)&&near(x->height,y->height)
            &&near(x->angle,y->angle)&&near(x->widthscale,y->widthscale)
            &&near(x->oblique,y->oblique)&&near(x->thickness,y->thickness)
            &&x->textgen==y->textgen&&x->alignH==y->alignH&&x->alignV==y->alignV
            &&near(x->interlin,y->interlin)&&x->linespacingStyle==y->linespacingStyle
            &&near(x->m_definedHeight,y->m_definedHeight)
            &&x->m_definedHeightPresent==y->m_definedHeightPresent
            &&x->m_backgroundFlags==y->m_backgroundFlags
            &&near(x->m_backgroundScale,y->m_backgroundScale)
            &&x->m_backgroundColor==y->m_backgroundColor
            &&x->m_backgroundColorTrue==y->m_backgroundColorTrue
            &&x->m_backgroundColorName==y->m_backgroundColorName
            &&x->m_backgroundTransparency==y->m_backgroundTransparency;
    }
    return false;
}
int main(int argc,char**argv){
 if(argc!=3)return 2;
 dx_iface reader;dx_data src;if(!reader.fileImport(argv[1],&src,false))return 3;
 const dx_ifaceBlock* sb=nullptr;
 for(auto*b:src.blocks)if(b&&b->name=="*T1"){if(sb)return 4;sb=b;}
 const DRW_Block_Record* sr=nullptr;
 for(const auto&r:src.blockRecords)if(r.name=="*T1"){if(sr)return 5;sr=&r;}
 bool layer=false,style=false;
 for(const auto&l:src.layers)if(l.name=="0")layer=true;
 for(const auto&s:src.textStyles)if(s.name=="Standard")style=true;
 if(!sb||!sr||!layer||!style)return 6;
 dx_data out;DRW_Layer outLayer;outLayer.name="0";out.layers.push_back(outLayer);
 DRW_Textstyle outStyle;outStyle.name="Standard";outStyle.font="arial.ttf";out.textStyles.push_back(outStyle);
 auto*b=new dx_ifaceBlock(static_cast<const DRW_Block&>(*sb));
 b->endBlockData=sb->endBlockData;b->hasEndBlockData=sb->hasEndBlockData;
 std::vector<const DRW_Entity*> expected;
 for(auto*e:sb->ent){
   if(auto*m=dynamic_cast<DRW_MText*>(e)){b->ent.push_back(new DRW_MText(*m));expected.push_back(e);}
   else if(auto*l=dynamic_cast<DRW_Line*>(e)){b->ent.push_back(new DRW_Line(*l));expected.push_back(e);}
   else return 7;
 }
 out.blocks.push_back(b);
 const char*path=argv[2];
 const bool ok=reader.fileExport(path,DRW::AC1018,false,&out,false,true,dx_dxfSourceTextPolicy::Compatible,DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::AllowPartial);
 std::cout<<"export="<<ok<<" source_members="<<expected.size()<<"\n";if(!ok)return 8;
 dx_iface backReader;dx_data back;if(!backReader.fileImport(path,&back,false))return 9;
 for(auto*x:back.blocks)if(x&&x->name=="*T1"){
   if(x->ent.size()!=expected.size()){std::cout<<"readback_count="<<x->ent.size()<<" expected="<<expected.size()<<"\n";return 10;}
   std::vector<DRW_Entity*> actual(x->ent.begin(),x->ent.end());
   for(std::size_t i=0;i<expected.size();++i){
     if(!actual[i]||!equalEntity(*expected[i],*actual[i])){
       std::cout<<"semantic mismatch index="<<i<<" expected_type="<<(expected[i]?expected[i]->eType:-1)
        <<" actual_type="<<(actual[i]?actual[i]->eType:-1)<<"\n";
       if(auto*m=dynamic_cast<const DRW_MText*>(expected[i]))std::cout<<" source MTEXT: text='"<<m->text<<"' style='"<<m->style<<"' at="<<m->basePoint.x<<","<<m->basePoint.y<<","<<m->basePoint.z<<" h="<<m->height<<" a="<<m->angle<<"\n";
       if(auto*m=dynamic_cast<DRW_MText*>(actual[i]))std::cout<<" output MTEXT: text='"<<m->text<<"' style='"<<m->style<<"' at="<<m->basePoint.x<<","<<m->basePoint.y<<","<<m->basePoint.z<<" h="<<m->height<<" a="<<m->angle<<"\n";
       return 11;
     }
   }
   std::cout<<"same-library DXF readback preserved modeled fields for "<<x->ent.size()<<" ordered members (7 MTEXT, 8 LINE)\n";
   return 0;
 }
 return 12;
}
