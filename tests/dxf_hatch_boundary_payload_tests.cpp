// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>

struct EdgeWriter : LC3CompatCheck {
    EdgeWriter(dxfRW& target,bool isPolygon):writer(target),polygon(isPolygon) {
        edge=std::make_shared<DRW_Ellipse>();
        edge->basePoint={3.,4.,-0.};edge->secPoint={2.,0.,-0.};
        edge->ratio=.5;edge->staparam=.25;edge->endparam=1.25;
        auto loop=std::make_shared<DRW_HatchLoop>(0);loop->numedges=1;loop->objlist.push_back(edge);
        DRW_Hatch& parent=polygon?static_cast<DRW_Hatch&>(mpolygon):hatch;
        parent.name="SOLID";parent.basePoint.z=3.25;parent.extPoint={0.,1.,0.};
        parent.looplist.push_back(loop);
    }
    void writeEntities() override {
        accepted=polygon?writer.writeMPolygon(&mpolygon):writer.writeHatch(&hatch);
    }
    dxfRW& writer;bool polygon,accepted=false;DRW_Hatch hatch;DRW_MPolygon mpolygon;
    std::shared_ptr<DRW_Ellipse> edge;
};
struct EdgeReader : LC3CompatCheck {
    void addHatch(const DRW_Hatch* value) override {save(value);}
    void addMPolygon(const DRW_MPolygon* value) override {save(value);}
    void save(const DRW_Hatch* value) {
        ++count;
        if(!value)return;
        elevation=value->basePoint.z;normal=value->extPoint;
        if(value->looplist.size()!=1u||!value->looplist[0]||value->looplist[0]->objlist.size()!=1u)return;
        const auto* e=dynamic_cast<const DRW_Ellipse*>(value->looplist[0]->objlist[0].get());
        if(e){center=e->basePoint;axis=e->secPoint;found=true;}
        const auto* spline=dynamic_cast<const DRW_Spline*>(value->looplist[0]->objlist[0].get());
        if(spline)weights=spline->weightlist;
    }
    int count=0;bool found=false;double elevation=0.;DRW_Coord normal,center,axis;std::vector<double> weights;
};
struct GenericWriter : LC3CompatCheck {
    GenericWriter(dxfRW& target,bool isPolygon,int mode):writer(target),polygon(isPolygon),which(mode) {
        auto loop=std::make_shared<DRW_HatchLoop>(mode>=9?2:0);loop->numedges=1;
        if(mode<=1){line=std::make_shared<DRW_Line>();line->secPoint.x=1.;loop->objlist.push_back(line);}
        else if(mode==2){arc=std::make_shared<DRW_Arc>();arc->radious=1.;loop->objlist.push_back(arc);}
        else if(mode<=8){
            spline=std::make_shared<DRW_Spline>();spline->degree=2;
            spline->knotslist={0.,0.,0.,1.,1.,1.};
            spline->controllist={std::make_shared<DRW_Coord>(0.,0.,0.),std::make_shared<DRW_Coord>(1.,1.,0.),std::make_shared<DRW_Coord>(2.,0.,0.)};
            spline->fitlist={std::make_shared<DRW_Coord>(1.,.5,0.)};
            if(mode>=7){spline->flags=4;spline->weightlist={1.,1.+1e-13,1.};}
            loop->objlist.push_back(spline);
        } else {
            poly=std::make_shared<DRW_LWPolyline>();poly->flags=1;
            poly->addVertex(DRW_Vertex2D(0.,0.,0.));poly->addVertex(DRW_Vertex2D(1.,0.,.5));
            loop->objlist.push_back(poly);
        }
        DRW_Hatch& parent=polygon?static_cast<DRW_Hatch&>(mpolygon):hatch;
        parent.name="SOLID";parent.basePoint.z=3.25;parent.extPoint={0.,1.,0.};parent.looplist.push_back(loop);
    }
    void change(bool invalid) {
        const double z=invalid?7.:-0.;
        switch(which) {
        case 0:line->basePoint.z=z;break;case 1:line->secPoint.z=z;break;case 2:arc->basePoint.z=z;break;
        case 3:spline->controllist[0]->z=z;break;case 4:spline->fitlist[0]->z=z;break;
        case 5:spline->tgStart.z=z;break;case 6:spline->tgEnd.z=z;break;
        case 7:spline->controllist[1]->z=invalid?7.:1.+1e-13;break;
        case 8:spline->weightlist.clear();spline->controllist[1]->z=invalid?2.:1.;break;
        case 9:poly->width=z;break;case 10:poly->vertlist[0]->stawidth=z;break;
        case 11:poly->vertlist[0]->endwidth=z;break;case 12:poly->vertlist[0]->identifier=invalid?3:0;break;
        case 13:poly->flags=invalid?129:1;break;
        }
    }
    void writeEntities() override {accepted=polygon?writer.writeMPolygon(&mpolygon):writer.writeHatch(&hatch);}
    dxfRW& writer;bool polygon,accepted=false;int which;DRW_Hatch hatch;DRW_MPolygon mpolygon;
    std::shared_ptr<DRW_Line> line;std::shared_ptr<DRW_Arc> arc;std::shared_ptr<DRW_Spline> spline;
    std::shared_ptr<DRW_LWPolyline> poly;
};
bool preserved(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f),{})=="sentinel";
}
int main() {
    const auto path=std::filesystem::temp_directory_path()/
        ("libdxfrw-hatch-ellipse-edge-z-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}}cleanup{path};
    for(bool binary:{false,true})for(bool polygon:{false,true}) {
        dxfRW writer(path.string().c_str());EdgeWriter adapter(writer,polygon);
        for(int mode:{0,1,2}) {
            {std::ofstream f(path,std::ios::binary|std::ios::trunc);f<<"sentinel";}
            adapter.edge->basePoint.z=mode==0?7.:-0.;
            adapter.edge->secPoint.z=mode==1?7.:-0.;
            // The nonzero axis is still perpendicular to its edge normal; the
            // test targets unrepresentable Z, not generic ellipse validity.
            adapter.edge->extPoint=mode==1?DRW_Coord{0.,1.,0.}
                :mode==2?DRW_Coord{0.,.6,.8}:DRW_Coord{0.,0.,1.};
            adapter.edge->ratio=mode==2?2.:.5;
            if(writer.write(&adapter,DRW::AC1027,binary)||adapter.accepted||!preserved(path)) {
                std::cerr<<"nonzero ellipse-edge Z was accepted or replaced the destination\n";return 1;
            }
            if((mode==0&&adapter.edge->basePoint.z!=7.)||(mode==1&&adapter.edge->secPoint.z!=7.))return 1;
            adapter.edge->basePoint.z=-0.;adapter.edge->secPoint.z=-0.;
            adapter.edge->extPoint={0.,0.,1.};adapter.edge->ratio=.5;
            if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.accepted)return 1;
            dxfRW reader(path.string().c_str());EdgeReader result;
            if(!reader.read(&result,false)||result.count!=1||!result.found||result.elevation!=3.25
                ||result.normal.x!=0.||result.normal.y!=1.||result.normal.z!=0.
                ||result.center.x!=3.||result.center.y!=4.||result.center.z!=0.
                ||result.axis.x!=2.||result.axis.y!=0.||result.axis.z!=0.) {
                std::cerr<<"signed-zero retry or parent OCS elevation/extrusion readback failed\n";return 1;
            }
        }
    }
    for(bool binary:{false,true})for(bool polygon:{false,true})for(int mode=0;mode<=13;++mode) {
        dxfRW writer(path.string().c_str());GenericWriter adapter(writer,polygon,mode);
        {std::ofstream f(path,std::ios::binary|std::ios::trunc);f<<"sentinel";}
        adapter.change(true);
        if(writer.write(&adapter,DRW::AC1027,binary)||adapter.accepted||!preserved(path)) {
            std::cerr<<"edge loss accepted mode="<<mode<<" binary="<<binary<<" polygon="<<polygon<<'\n';return 1;
        }
        adapter.change(false);
        if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.accepted) {
            std::cerr<<"edge corrected retry failed mode="<<mode<<'\n';return 1;
        }
        dxfRW reader(path.string().c_str());EdgeReader result;
        if(!reader.read(&result,false)||result.count!=1||result.elevation!=3.25
            ||result.normal.x!=0.||result.normal.y!=1.||result.normal.z!=0.
            ||(mode==7&&(result.weights.size()!=3u||result.weights[1]!=1.+1e-13))
            ||(mode==8&&!result.weights.empty()))return 1;
        if(mode==8) {
            // No weight array plus Z=0 has two possible source meanings:
            // retain today's DXF implicit-unit behavior and record the DWG
            // legacy-zero ambiguity as unresolved rather than guess a change.
            adapter.spline->controllist[1]->z=0.;
            if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.accepted)return 1;
            dxfRW zeroReader(path.string().c_str());EdgeReader zero;
            if(!zeroReader.read(&zero,false)||zero.count!=1||!zero.weights.empty())return 1;
        }
    }
    std::cout<<"HATCH/MPOLYGON boundary loss refusal and retry PASS\n";return 0;
}
