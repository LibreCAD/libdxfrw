// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INDEPENDENT_DXF_SCALAR_WALKER_H
#define INDEPENDENT_DXF_SCALAR_WALKER_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

// Included inside the owning probe's anonymous namespace. This independent
// original-file observer uses neither dxfReader nor the production audit.
struct AttdefWireTag {
    int code=0;
    std::size_t begin=0,valueBegin=0,end=0;
    std::string text;
    std::int64_t integer=0;
    double real=0;
    enum Kind {String,Integer,Real,Binary} kind=String;
};
std::vector<AttdefWireTag> attdefWireTags(const std::filesystem::path& path,bool binary) {
    std::ifstream input(path,std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(input)),{});
    std::vector<AttdefWireTag> out;std::size_t at=binary?22u:0u;
    const auto word=[&](unsigned count) {
        if(at>bytes.size()||count>bytes.size()-at)throw std::runtime_error("independent wire observer: truncated number");
        std::uint64_t result=0u;
        for(unsigned n=0;n<count;++n)result|=static_cast<std::uint64_t>(static_cast<unsigned char>(bytes[at++]))<<(8u*n);
        return result;
    };
    const auto line=[&]() {
        const auto end=bytes.find('\n',at);
        if(end==std::string::npos)throw std::runtime_error("independent wire observer: truncated line");
        auto result=bytes.substr(at,end-at);at=end+1u;
        if(!result.empty()&&result.back()=='\r')result.pop_back();
        return result;
    };
    while(at<bytes.size()) {
        AttdefWireTag tag;tag.begin=at;
        tag.code=binary?static_cast<int>(word(2u)):std::stoi(line());tag.valueBegin=at;
        unsigned width=0u;
        if((tag.code>=60&&tag.code<=79)||(tag.code>=170&&tag.code<=179)
            ||(tag.code>=270&&tag.code<=289)||(tag.code>=370&&tag.code<=389)
            ||(tag.code>=400&&tag.code<=409)||tag.code==1070)width=2u;
        else if((tag.code>=90&&tag.code<=99)||(tag.code>=420&&tag.code<=429)
            ||(tag.code>=440&&tag.code<=459)||tag.code==1071)width=4u;
        else if(tag.code>=160&&tag.code<=169)width=8u;
        else if(tag.code>=290&&tag.code<=299)width=1u;
        if(width) {
            tag.kind=AttdefWireTag::Integer;
            if(!binary)tag.integer=std::stoll(line());
            else {
                const auto value=word(width);
                tag.integer=width==1u?static_cast<std::int64_t>(value):width==2u?static_cast<std::int16_t>(value)
                    :width==4u?static_cast<std::int32_t>(value):static_cast<std::int64_t>(value);
            }
        } else if((tag.code>=10&&tag.code<=59)||(tag.code>=110&&tag.code<=149)||(tag.code>=210&&tag.code<=239)
            ||(tag.code>=460&&tag.code<=469)
            ||(tag.code>=1010&&tag.code<=1059)) {
            tag.kind=AttdefWireTag::Real;
            if(!binary)tag.real=std::stod(line());
            else {const auto bits=word(8u);std::memcpy(&tag.real,&bits,sizeof(bits));}
        } else if(binary&&((tag.code>=310&&tag.code<=319)||tag.code==1004)) {
            tag.kind=AttdefWireTag::Binary;const auto count=word(1u);
            constexpr char digits[]="0123456789ABCDEF";
            for(std::uint64_t n=0;n<count;++n) {const auto value=word(1u);tag.text+=digits[value>>4u];tag.text+=digits[value&15u];}
        } else if(!binary)tag.text=line();
        else {
            const auto end=bytes.find('\0',at);
            if(end==std::string::npos)throw std::runtime_error("independent wire observer: truncated string");
            tag.text=bytes.substr(at,end-at);at=end+1u;
        }
        tag.end=at;out.push_back(std::move(tag));
    }
    return out;
}
#endif
