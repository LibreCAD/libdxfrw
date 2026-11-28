// Scratch capture and selected typed-source observations; no output authority.
#include "dx_iface.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>

class Capture final : public dx_iface {
public:
    explicit Capture(std::filesystem::path path) : path_(std::move(path)) {}
    void addDictionary(const DRW_Dictionary& record) override {
        dx_iface::addDictionary(record);
        if (record.handle!=0x6au && record.handle!=0xdbu) return;
        const bool shape=record.handle==0x6au
            ? record.parentHandle==0xcu && record.hardOwner==0u && record.m_entries.size()==1u
                && record.m_entries[0].m_name=="Standard" && record.m_entries[0].m_handle==0x6bu
            : record.parentHandle==0x6bu && record.hardOwner==1u && record.m_entries.size()==2u
                && record.m_entries[0].m_name=="ACAD_XREC_ROUNDTRIP" && record.m_entries[0].m_handle==0xdcu
                && record.m_entries[1].m_name=="ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP" && record.m_entries[1].m_handle==0xddu;
        if (!shape || !typed_.insert(record.handle).second) good_=false;
    }
    void addTableStyle(const DRW_TableStyle& record) override {
        dx_iface::addTableStyle(record);
        if (record.handle!=0x6bu) return;
        const std::array<double,3> heights{{4.5,6.0,4.5}};
        const std::array<int,3> alignments{{2,5,5}};
        bool valid=record.parentHandle==0x6au && record.xDictHandle==0xdbu
            && record.reactorHandles==std::vector<std::uint32_t>({0x6au,0x9cu})
            && record.m_name=="Standard" && record.m_rowStyles.size()==3u;
        if (valid) for (std::size_t i=0;i<3u;++i) {
            const auto& row=record.m_rowStyles[i];
            valid=valid && row.m_textStyleHandle==0x11u && row.m_textHeight==heights[i]
                && row.m_textAlignment==alignments[i] && row.m_borders.size()==6u;
        }
        if (!valid || !typed_.insert(record.handle).second) good_=false;
    }
    void addCellStyleMap(const DRW_CellStyleMap& record) override {
        dx_iface::addCellStyleMap(record);
        if (record.handle!=0xddu) return;
        const std::array<int,3> ids{{3,1,2}}, classes{{2,1,1}}, alignments{{2,5,5}};
        const std::array<double,3> heights{{4.5,6.0,4.5}};
        const std::array<const char*,3> names{{"_DATA","_TITLE","_HEADER"}};
        const std::array<int,6> edges{{1,2,4,8,16,32}};
        bool valid=record.parentHandle==0xdbu && record.reactorHandles==std::vector<std::uint32_t>({0xdbu})
            && record.m_cellStyles.size()==3u && !record.validateDwgPayload(DRW::AC1018);
        if (valid) for (std::size_t i=0;i<3u;++i) {
            const auto& s=record.m_cellStyles[i];const auto& f=s.m_contentFormat;
            valid=valid && s.m_id==ids[i] && s.m_styleClass==classes[i] && s.m_name==names[i]
                && s.m_type==5 && s.m_hasData && s.m_propertyOverrideFlags==0u
                && s.m_mergeFlags==(ids[i]==1 ? 32768u : 0u)
                && s.m_backgroundColorCmc.m_method==0xc8u
                && s.m_backgroundColorCmc.hasRawDwgPackedValue()
                && s.m_backgroundColorCmc.rawDwgPackedValue()==0xc8000000u
                && s.m_contentLayout==1u && f.m_propertyOverrideFlags==0u && f.m_propertyFlags==0u
                && f.m_valueDataType==4 && f.m_valueUnitType==0 && f.m_valueFormatString.empty()
                && f.m_rotation==0.0 && f.m_blockScale==1.0 && f.m_cellAlignment==alignments[i]
                && f.m_contentColorCmc.m_method==0xc1u && f.m_textStyleHandle==0x11u
                && f.m_textHeight==heights[i] && s.m_marginOverrideFlags==1
                && s.m_verticalMargin==1.5 && s.m_horizontalMargin==1.5 && s.m_bottomMargin==1.5
                && s.m_rightMargin==1.5 && s.m_marginHorizontalSpacing==1.5 && s.m_marginVerticalSpacing==1.5
                && s.m_borders.size()==6u;
            if (s.m_borders.size()==6u) for (std::size_t j=0;j<6u;++j) {
                const auto& b=s.m_borders[j];
                valid=valid && b.m_edgeFlags==edges[j] && b.m_propertyOverrideFlags==0
                    && b.m_borderType==1 && b.m_colorCmc.m_method==0xc1u && b.m_lineWeight==-2
                    && b.m_lineTypeHandle==0x14u && b.m_visible==0 && b.m_doubleLineSpacing==0.045;
            }
        }
        if (!valid || !typed_.insert(record.handle).second) good_=false;
    }
    void addUnsupportedObject(const DRW_UnsupportedObject& record) override {
        dx_iface::addUnsupportedObject(record);
        const std::set<std::uint32_t> selected{0x6au,0x6bu,0xdbu,0xdcu,0xddu};
        if (!selected.count(record.m_handle)) return;
        if (!seen_.insert(record.m_handle).second) { good_=false; return; }
        const auto outputPath=path_/("body-"+std::to_string(record.m_handle)+".bin");
        std::ofstream output(outputPath,std::ios::binary|std::ios::trunc);
        output.write(reinterpret_cast<const char*>(record.m_rawBytes.data()),
                     static_cast<std::streamsize>(record.m_rawBytes.size()));
        if (!output) good_=false;
        std::cout<<"raw="<<std::hex<<record.m_handle<<std::dec
                 <<" objectBufferOffset="<<record.m_objectOffset<<" bytes="<<record.m_rawBytes.size()<<'\n';
    }
    bool good() const { return good_ && seen_.size()==5u && typed_.size()==4u; }
private:
    std::filesystem::path path_;
    std::set<std::uint32_t> seen_,typed_;
    bool good_{true};
};
int main(int argc,char** argv) {
    if (argc!=3) return 2;
    std::error_code error;
    if (!std::filesystem::create_directory(argv[2],error)) {
        std::cerr<<"output directory must be new: "<<error.message()<<'\n';return 2;
    }
    auto data=std::make_unique<dx_data>();auto capture=std::make_unique<Capture>(argv[2]);
    const bool read=capture->fileImport(argv[1],data.get(),false);
    std::cout<<"read="<<read<<" selectedTypedAndRaw="<<capture->good()<<'\n';
    return read && capture->good() ? 0 : 1;
}
