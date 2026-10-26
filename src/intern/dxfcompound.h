// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFCOMPOUND_H
#define DXFCOMPOUND_H
#include "dxfattdef.h"
#include <optional>

// Shared production/replay field admission and exact modeled equality, including
// selected POINT/LINE/ATTDEF primary roots used beside compound chains. This
// does not inventory unknown physical scalars or qualify dependency/output roles.
namespace dxfCompound {
using SourceState=dxfAttdef::SourceState;
class Decoder {
    enum class Kind {Polyline,Vertex,Insert,Attribute,SequenceEnd,Point,Line,Definition};
    DRW_Entity* value;Kind kind;unsigned fields{0u};
    std::optional<dxfAttdef::Decoder> definition;
    enum : unsigned {Text=1u,Tag=2u,X=4u,Y=8u,Height=16u,EndX=32u,EndY=64u};
public:
    explicit Decoder(DRW_Polyline& v):value(&v),kind(Kind::Polyline){}
    explicit Decoder(DRW_Vertex& v):value(&v),kind(Kind::Vertex){}
    explicit Decoder(DRW_Insert& v):value(&v),kind(Kind::Insert){}
    explicit Decoder(DRW_Attrib& v):value(&v),kind(Kind::Attribute){}
    explicit Decoder(DRW_SeqEnd& v):value(&v),kind(Kind::SequenceEnd){}
    explicit Decoder(DRW_Point& v):value(&v),kind(Kind::Point){}
    explicit Decoder(DRW_Line& v):value(&v),kind(Kind::Line){}
    explicit Decoder(DRW_Attdef& v):value(&v),kind(Kind::Definition),definition(std::in_place,v){}
    void reset(DRW_Vertex& v) noexcept {value=&v;kind=Kind::Vertex;fields=0u;definition.reset();}
    void reset(DRW_Attrib& v) noexcept {value=&v;kind=Kind::Attribute;fields=0u;definition.reset();}
    static bool parseEntity(DRW_Entity& value,int code,const std::unique_ptr<dxfReader>& reader) {return value.parseCode(code,reader);}
    bool parseCode(int code,const std::unique_ptr<dxfReader>& reader) {
        if(kind==Kind::Definition)return code==102?definition->parseApplication(reader):definition->parseCode(code,reader);
        if(code==1)fields|=Text;if(code==2)fields|=Tag;if(code==10)fields|=X;
        if(code==20)fields|=Y;if(code==40)fields|=Height;
        if(code==11)fields|=EndX;if(code==21)fields|=EndY;
        return parseEntity(*value,code,reader);
    }
    bool finish(const DRW_Polyline* parent=nullptr) {
        switch(kind) {
        case Kind::Polyline:return static_cast<const DRW_Polyline*>(value)->validatePolyfaceDxf();
        case Kind::Vertex: {
            const auto& vertex=*static_cast<const DRW_Vertex*>(value);
            const bool ignored=parent&&(parent->flags&64)!=0
                &&DRW_Polyline::polyfaceVertexRole(vertex)==DRW_Polyline::PolyfaceVertexRole::Face;
            return (ignored||(fields&(X|Y))==(X|Y))&&vertex.validatePayloadFields();
        }
        case Kind::Insert:return (fields&(Tag|X|Y))==(Tag|X|Y)&&!static_cast<const DRW_Insert*>(value)->name.empty();
        case Kind::Attribute: {
            auto& attribute=*static_cast<DRW_Attrib*>(value);
            return (fields&(Text|Tag|X|Y|Height))==(Text|Tag|X|Y|Height)&&!attribute.tag.empty()
                &&dxfAttdef::Decoder::finishAttribute(attribute);
        }
        case Kind::SequenceEnd:return true;
        case Kind::Point:return (fields&(X|Y))==(X|Y);
        case Kind::Line:return (fields&(X|Y|EndX|EndY))==(X|Y|EndX|EndY);
        case Kind::Definition:return definition->finish(false);
        }
        return false;
    }
    static SourceState observe(const DRW_Entity& v) noexcept {
        if(const auto* attribute=dynamic_cast<const DRW_Attrib*>(&v))return dxfAttdef::Decoder::observeAttribute(*attribute);
        SourceState out;out.host=dxfAttdef::Decoder::entityCursor(v);return out;
    }
    template<class Equal>
    static bool sameModel(const DRW_Entity& a,const DRW_Entity& b,Equal equal) noexcept {
        if(a.eType!=b.eType)return false;
        switch(a.eType) {
        case DRW::ATTDEF: {
            const auto* x=dynamic_cast<const DRW_Attdef*>(&a);const auto* y=dynamic_cast<const DRW_Attdef*>(&b);
            return x&&y&&dxfAttdef::Decoder::sameModel(*x,*y,equal);
        }
        case DRW::POINT: {
            const auto* x=dynamic_cast<const DRW_Point*>(&a);const auto* y=dynamic_cast<const DRW_Point*>(&b);
            return x&&y&&samePoint(*x,*y,equal);
        }
        case DRW::LINE: {
            const auto* x=dynamic_cast<const DRW_Line*>(&a);const auto* y=dynamic_cast<const DRW_Line*>(&b);
            return x&&y&&samePoint(*x,*y,equal)&&point(x->secPoint,y->secPoint);
        }
        case DRW::POLYLINE: {
            const auto* x=dynamic_cast<const DRW_Polyline*>(&a);const auto* y=dynamic_cast<const DRW_Polyline*>(&b);
            if(!x||!y||!samePoint(*x,*y,equal)||x->hadlesList!=y->hadlesList||x->firstEH!=y->firstEH||x->lastEH!=y->lastEH
                ||!handle(x->seqEndH,y->seqEndH)||!real(x->defstawidth,y->defstawidth)||!real(x->defendwidth,y->defendwidth)
                ||std::tie(x->flags,x->vertexcount,x->facecount,x->smoothM,x->smoothN,x->curvetype)
                    !=std::tie(y->flags,y->vertexcount,y->facecount,y->smoothM,y->smoothN,y->curvetype)
                ||x->vertlist.size()!=y->vertlist.size()||!child(x->seqEndEntity,y->seqEndEntity,equal))return false;
            for(std::size_t i=0;i<x->vertlist.size();++i)if(!child(x->vertlist[i],y->vertlist[i],equal))return false;
            return true;
        }
        case DRW::INSERT: {
            const auto* x=dynamic_cast<const DRW_Insert*>(&a);const auto* y=dynamic_cast<const DRW_Insert*>(&b);
            if(!x||!y||!samePoint(*x,*y,equal)||x->name!=y->name||x->colcount!=y->colcount||x->rowcount!=y->rowcount
                ||!real(x->xscale,y->xscale)||!real(x->yscale,y->yscale)||!real(x->zscale,y->zscale)||!real(x->angle,y->angle)
                ||!real(x->colspace,y->colspace)||!real(x->rowspace,y->rowspace)||!handle(x->blockRecH,y->blockRecH)
                ||!handle(x->seqendH,y->seqendH)||x->attribHandles.size()!=y->attribHandles.size()
                ||x->attlist.size()!=y->attlist.size()||!child(x->seqEndEntity,y->seqEndEntity,equal))return false;
            for(std::size_t i=0;i<x->attribHandles.size();++i)if(!handle(x->attribHandles[i],y->attribHandles[i]))return false;
            for(std::size_t i=0;i<x->attlist.size();++i)if(!child(x->attlist[i],y->attlist[i],equal))return false;
            return true;
        }
        case DRW::VERTEX: {
            const auto* x=dynamic_cast<const DRW_Vertex*>(&a);const auto* y=dynamic_cast<const DRW_Vertex*>(&b);
            return x&&y&&samePoint(*x,*y,equal)&&real(x->stawidth,y->stawidth)&&real(x->endwidth,y->endwidth)
                &&real(x->bulge,y->bulge)&&real(x->tgdir,y->tgdir)&&x->dwgSubtype()==y->dwgSubtype()
                &&std::tie(x->flags,x->vindex1,x->vindex2,x->vindex3,x->vindex4,x->identifier)
                    ==std::tie(y->flags,y->vindex1,y->vindex2,y->vindex3,y->vindex4,y->identifier);
        }
        case DRW::ATTRIB: {
            const auto* x=dynamic_cast<const DRW_Attrib*>(&a);const auto* y=dynamic_cast<const DRW_Attrib*>(&b);
            return x&&y&&dxfAttdef::Decoder::sameAttributeModel(*x,*y,equal);
        }
        case DRW::SEQEND:return dynamic_cast<const DRW_SeqEnd*>(&a)&&dynamic_cast<const DRW_SeqEnd*>(&b)
            &&dxfAttdef::Decoder::sameEntityModel(a,b,equal);
        default:return false;
        }
    }
private:
    static bool real(double a,double b) noexcept {return std::memcmp(&a,&b,sizeof(a))==0;}
    static bool point(const DRW_Coord& a,const DRW_Coord& b) noexcept {return real(a.x,b.x)&&real(a.y,b.y)&&real(a.z,b.z);}
    static bool handle(const dwgHandle& a,const dwgHandle& b) noexcept {
        return std::tie(a.code,a.size,a.ref,a.ref64)==std::tie(b.code,b.size,b.ref,b.ref64);
    }
    template<class Equal> static bool samePoint(const DRW_Point& a,const DRW_Point& b,Equal equal) noexcept {
        return dxfAttdef::Decoder::sameEntityModel(a,b,equal)&&point(a.basePoint,b.basePoint)&&point(a.extPoint,b.extPoint)
            &&real(a.thickness,b.thickness)&&real(a.xAxisAngle,b.xAxisAngle);
    }
    template<class T,class Equal> static bool child(const std::shared_ptr<T>& a,const std::shared_ptr<T>& b,Equal equal) noexcept {
        return static_cast<bool>(a)==static_cast<bool>(b)&&(!a||sameModel(*a,*b,equal));
    }
};
} // namespace dxfCompound
#endif
