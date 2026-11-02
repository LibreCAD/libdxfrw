// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFVALUE_H
#define DXFVALUE_H
#include "dxfparserlimits.h"
#include "drw_reserve.h"
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

// Shared bounded binary primitive only; no FIELD/TABLE schema substitution.
namespace dxfValue {
inline int hexDigit(char ch) {
    if(ch>='0'&&ch<='9')return ch-'0';
    if(ch>='a'&&ch<='f')return ch-'a'+10;
    if(ch>='A'&&ch<='F')return ch-'A'+10;
    return -1;
}
inline bool validHexChunk(std::string_view text,std::size_t size,
                          std::size_t limit=DRW::kMaxDxfBinaryPayloadBytes) {
    if(size>limit||(text.size()&1u)||text.size()/2u>limit-size)return false;
    for(char ch:text)if(hexDigit(ch)<0)return false;
    return true;
}
inline bool appendHexChunk(std::string_view text,std::vector<std::uint8_t>& target,
                           std::size_t limit=DRW::kMaxDxfBinaryPayloadBytes) {
    if(!validHexChunk(text,target.size(),limit))return false;
    const auto size=target.size()+text.size()/2u;
    if(size>static_cast<std::size_t>(std::numeric_limits<int>::max())
        ||!DRW::reserve(target,static_cast<int>(size)))return false;
    for(std::size_t i=0u;i<text.size();i+=2u)
        target.push_back(static_cast<std::uint8_t>((hexDigit(text[i])<<4)|hexDigit(text[i+1u])));
    return true;
}
}
#endif
