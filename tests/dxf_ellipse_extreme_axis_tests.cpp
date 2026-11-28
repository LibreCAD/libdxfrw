// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

struct EllipseWriter : LC3CompatCheck {
    explicit EllipseWriter(dxfRW& value):writer(value) {}
    void writeEntities() override {accepted=writer.writeEllipse(&ellipse);}
    dxfRW& writer;DRW_Ellipse ellipse;bool accepted=false;
};
struct EllipseReader : LC3CompatCheck {
    void addEllipse(const DRW_Ellipse& value) override {ellipse=value;++count;}
    DRW_Ellipse ellipse;int count=0;
};
struct PolylineReader : LC3CompatCheck {
    void addPolyline(const DRW_Polyline& value) override {polyline=value;++count;}
    DRW_Polyline polyline;int count=0;
};
struct BoundaryWriter : LC3CompatCheck {
    BoundaryWriter(dxfRW& value,bool polygon):writer(value),mpolygon(polygon) {
        edge=std::make_shared<DRW_Ellipse>();edge->secPoint={1e200,0.,0.};edge->ratio=2.;
        edge->staparam=.25;edge->endparam=1.25;
        auto loop=std::make_shared<DRW_HatchLoop>(0);loop->numedges=1;loop->objlist.push_back(edge);
        if(mpolygon){polygonData.name="SOLID";polygonData.looplist.push_back(loop);}
        else {hatch.name="SOLID";hatch.looplist.push_back(loop);}
    }
    void writeEntities() override {accepted=mpolygon?writer.writeMPolygon(&polygonData):writer.writeHatch(&hatch);}
    dxfRW& writer;bool mpolygon,accepted=false;DRW_Hatch hatch;DRW_MPolygon polygonData;
    std::shared_ptr<DRW_Ellipse> edge;
};
struct BoundaryReader : LC3CompatCheck {
    void addHatch(const DRW_Hatch* value) override {capture(value);}
    void addMPolygon(const DRW_MPolygon* value) override {capture(value);}
    void capture(const DRW_Hatch* value) {
        ++count;
        if(value&&value->looplist.size()==1u&&value->looplist[0]
            &&value->looplist[0]->objlist.size()==1u) {
            const auto* ellipse=dynamic_cast<const DRW_Ellipse*>(value->looplist[0]->objlist[0].get());
            if(ellipse){axis=ellipse->secPoint;ratio=ellipse->ratio;}
        }
    }
    int count=0;DRW_Coord axis;double ratio=0.;
};
bool close(double a,double b) {return std::abs(a-b)<=1e-12*std::max(1.,std::abs(b));}
bool cancelledOverflowingMinorTerm() {
    DRW_Ellipse ellipse;
    ellipse.basePoint={0.,-1.5e308,0.};
    ellipse.secPoint={1.5e308,0.,0.};
    ellipse.extPoint={0.,0.,1.};
    ellipse.ratio=1.5;
    ellipse.staparam=1.4;
    ellipse.endparam=1.7;
    if(!std::isinf((ellipse.secPoint.x*std::sin(ellipse.staparam))
            *ellipse.ratio)) return false;
    DRW_Polyline polyline;
    ellipse.toPolyline(&polyline,4);
    if(polyline.vertlist.size()!=2u || !polyline.vertlist[0]
        || !polyline.vertlist[1]) return false;
    for(const auto& vertex:polyline.vertlist)
        if(!std::isfinite(vertex->basePoint.x)
            || !std::isfinite(vertex->basePoint.y)
            || !std::isfinite(vertex->basePoint.z)) return false;
    const double expectedY=1.5e308*(1.5*std::sin(1.4)-1.0);
    if(!close(polyline.vertlist.front()->basePoint.y/1.5e308,
              expectedY/1.5e308)) return false;

    // This was the previously documented cancellation gap: the normalized
    // minor direction points along Z, and its sampled term overflows before
    // being combined with the finite negative center.
    ellipse.basePoint={0.,0.,-5e307};
    ellipse.secPoint={1.5e308,1.5e308,0.};
    ellipse.extPoint={1.,-1.,0.};
    ellipse.ratio=1.;
    ellipse.staparam=1.2;
    ellipse.endparam=1.3;
    if(!std::isinf((1.5e308*std::sqrt(2.))*std::sin(1.2))) return false;
    polyline.vertlist.clear();
    ellipse.toPolyline(&polyline,4);
    if(polyline.vertlist.size()!=2u || !polyline.vertlist.front()
        || !std::isfinite(polyline.vertlist.front()->basePoint.z)) return false;
    const double expectedZ=1.5e308*(std::sqrt(2.)*std::sin(1.2)-1.0/3.0);
    return close(polyline.vertlist.front()->basePoint.z/1.5e308,
                 expectedZ/1.5e308);
}
bool ratioAboveOneOverflowingNorm(const std::filesystem::path& path) {
    for(const bool binary:{false,true}) {
        DRW_Ellipse expectedEllipse;
        expectedEllipse.secPoint={1.3e308,1.3e308,0.};
        expectedEllipse.extPoint={1./std::sqrt(6.),-1./std::sqrt(6.),
                                  2./std::sqrt(6.)};
        expectedEllipse.ratio=1.1;
        if(!std::isinf(std::hypot(expectedEllipse.secPoint.x,
                                  expectedEllipse.secPoint.y))) return false;
        expectedEllipse.staparam=.3;
        expectedEllipse.endparam=.9;
        DRW_Polyline expected;
        expectedEllipse.toPolyline(&expected,8);
        if(expected.vertlist.size()!=2u || (expected.flags&8)==0
            || !std::isfinite(expected.vertlist.front()->basePoint.x)
            || !std::isfinite(expected.vertlist.front()->basePoint.y)
            || !std::isfinite(expected.vertlist.front()->basePoint.z)) return false;

        dxfRW writer(path.string().c_str());
        writer.setEllipseParts(8);
        EllipseWriter adapter(writer);
        adapter.ellipse=expectedEllipse;
        const bool wrote=writer.write(&adapter,DRW::AC1009,binary);
        if(!wrote||!adapter.accepted
            || adapter.ellipse.secPoint.x!=1.3e308
            || adapter.ellipse.ratio!=1.1) {
            std::cerr<<"R12 writer rejected overflow-norm ratio normalization binary="
                     <<binary<<" result="<<wrote<<" accepted="
                     <<adapter.accepted<<"\n";
            return false;
        }
        dxfRW reader(path.string().c_str());
        PolylineReader actual;
        if(!reader.read(&actual,false)||actual.count!=1
            ||actual.polyline.vertlist.size()!=expected.vertlist.size()
            ||(actual.polyline.flags&8)==0) {
            std::cerr<<"R12 ratio-normalized polyline readback shape mismatch binary="
                     <<binary<<" count="<<actual.count<<" vertices="
                     <<actual.polyline.vertlist.size()<<"\n";
            return false;
        }
        for(std::size_t i=0;i<expected.vertlist.size();++i) {
            const DRW_Coord& a=actual.polyline.vertlist[i]->basePoint;
            const DRW_Coord& b=expected.vertlist[i]->basePoint;
            if(std::abs(a.x/1.3e308-b.x/1.3e308)>1.e-12
                ||std::abs(a.y/1.3e308-b.y/1.3e308)>1.e-12
                ||std::abs(a.z/1.3e308-b.z/1.3e308)>1.e-12) {
                std::cerr<<"R12 ratio-normalized geometry mismatch binary="
                         <<binary<<" vertex="<<i<<" actual="<<a.x/1.3e308
                         <<","<<a.y/1.3e308<<","<<a.z/1.3e308
                         <<" expected="<<b.x/1.3e308<<","<<b.y/1.3e308
                         <<","<<b.z/1.3e308<<"\n";
                return false;
            }
        }
    }
    return true;
}
int main() {
    const auto path=std::filesystem::temp_directory_path()/
        ("libdxfrw-extreme-ellipse-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
    struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}}cleanup{path};
    for(bool binary:{false,true})for(int mode:{0,1,2,3}) {
        dxfRW writer(path.string().c_str());EllipseWriter adapter(writer);
        const double scale=mode==0?1e200:mode==3?1e-200:1.;
        const double normal=mode==1?1e200:mode==2?1e-200:1.;
        adapter.ellipse.secPoint={scale,0.,0.};adapter.ellipse.extPoint={0.,0.,normal};
        adapter.ellipse.ratio=2.;adapter.ellipse.staparam=.25;adapter.ellipse.endparam=1.25;
        if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.accepted) return 1;
        if(adapter.ellipse.secPoint.x!=scale||adapter.ellipse.secPoint.y!=0.||adapter.ellipse.ratio!=2.)return 1;
        dxfRW reader(path.string().c_str());EllipseReader captured;
        if(!reader.read(&captured,false)||captured.count!=1
            ||!close(captured.ellipse.secPoint.x,0.)||!close(captured.ellipse.secPoint.y/scale,2.)
            ||captured.ellipse.ratio!=.5
            ||!close(captured.ellipse.endparam-captured.ellipse.staparam,1.)) {
            std::cerr<<"large finite ellipse axis normalization failed mode="<<mode<<'\n';return 1;
        }
        const double oldX=scale*std::cos(.25),oldY=2.*scale*std::sin(.25);
        const double newX=-scale*std::sin(captured.ellipse.staparam);
        const double newY=2.*scale*std::cos(captured.ellipse.staparam);
        if(!close(newX/scale,oldX/scale)||!close(newY/scale,oldY/scale))return 1;
        {std::ofstream sentinel(path,std::ios::binary|std::ios::trunc);sentinel<<"unchanged";}
        adapter.ellipse.secPoint={1e308,0.,0.};adapter.ellipse.extPoint={0.,0.,1.};adapter.ellipse.ratio=4.;
        if(writer.write(&adapter,DRW::AC1027,binary)||adapter.accepted)return 1;
        {std::ifstream sentinel(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(sentinel),{})!="unchanged")return 1;}
        if(adapter.ellipse.secPoint.x!=1e308||adapter.ellipse.ratio!=4.)return 1;
        adapter.ellipse.secPoint={1.,0.,0.};
        if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.accepted)return 1;
        {std::ofstream sentinel(path,std::ios::binary|std::ios::trunc);sentinel<<"unchanged";}
        adapter.ellipse.staparam=-1e308;adapter.ellipse.endparam=1e308;
        if(writer.write(&adapter,DRW::AC1027,binary)||adapter.accepted)return 1;
        {std::ifstream sentinel(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(sentinel),{})!="unchanged")return 1;}
        if(adapter.ellipse.staparam!=-1e308||adapter.ellipse.endparam!=1e308)return 1;
        adapter.ellipse.staparam=.25;adapter.ellipse.endparam=1.25;
        if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.accepted)return 1;
    }
    for(bool binary:{false,true})for(bool polygon:{false,true}) {
        dxfRW writer(path.string().c_str());BoundaryWriter adapter(writer,polygon);
        if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.accepted)return 1;
        if(adapter.edge->secPoint.x!=1e200||adapter.edge->ratio!=2.)return 1;
        dxfRW reader(path.string().c_str());BoundaryReader capture;
        if(!reader.read(&capture,false)||capture.count!=1||!close(capture.axis.x,0.)
            ||!close(capture.axis.y/1e200,2.)||capture.ratio!=.5)return 1;
        {std::ofstream sentinel(path,std::ios::binary|std::ios::trunc);sentinel<<"unchanged";}
        adapter.edge->secPoint={1e308,0.,0.};adapter.edge->ratio=4.;
        if(writer.write(&adapter,DRW::AC1027,binary)||adapter.accepted)return 1;
        {std::ifstream sentinel(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(sentinel),{})!="unchanged")return 1;}
        if(adapter.edge->secPoint.x!=1e308||adapter.edge->ratio!=4.)return 1;
        adapter.edge->secPoint={1.,0.,0.};
        if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.accepted)return 1;
        {std::ofstream sentinel(path,std::ios::binary|std::ios::trunc);sentinel<<"unchanged";}
        adapter.edge->staparam=0.;adapter.edge->endparam=1e308;
        if(writer.write(&adapter,DRW::AC1027,binary)||adapter.accepted)return 1;
        {std::ifstream sentinel(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(sentinel),{})!="unchanged")return 1;}
        if(adapter.edge->endparam!=1e308)return 1;
        adapter.edge->staparam=.25;adapter.edge->endparam=1.25;
        if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.accepted)return 1;
    }
    if(!cancelledOverflowingMinorTerm()) {
        std::cerr<<"finite ELLIPSE coordinate lost to overflowing minor term\n";
        return 1;
    }
    if(!ratioAboveOneOverflowingNorm(path)) {
        std::cerr<<"ratio-above-one ELLIPSE normalization lost finite geometry\n";
        return 1;
    }
    std::cout<<"extreme ELLIPSE axis normalization and retry PASS\n";return 0;
}
