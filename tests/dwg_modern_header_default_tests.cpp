// SPDX-License-Identifier: GPL-2.0-or-later
#include "drw_header.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>

class HeaderProbe final : public DRW_Header {
public:
    using DRW_Header::encodeDwg;
};

int main() {
    for (const auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                               DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        HeaderProbe header;
        header.addInt("$DIMASO", 1, 70);
        header.addInt("$DIMSHO", 0, 70);
        header.addDouble("$TDCREATE", std::numeric_limits<double>::quiet_NaN(), 40);
        dwgBufferW body, handles, strings;
        body.putRawChar8(0xA5u); handles.putRawChar8(0x5Au); strings.putRawChar8(0x3Cu);
        body.putBit(1u); handles.putBit(0u); strings.putBit(1u);
        const auto oldBody=body.data(), oldHandles=handles.data(), oldStrings=strings.data();
        const auto oldBodyPos=body.bitPos(), oldHandlesPos=handles.bitPos(), oldStringsPos=strings.bitPos();
        if (header.encodeDwg(version,&body,&handles,&strings)
            || body.data()!=oldBody || handles.data()!=oldHandles || strings.data()!=oldStrings
            || body.bitPos()!=oldBodyPos || handles.bitPos()!=oldHandlesPos || strings.bitPos()!=oldStringsPos) {
            std::cerr << "invalid-time rollback failed " << static_cast<int>(version) << '\n';return 1;
        }
        header.addDouble("$TDCREATE", 0.0, 40);
        body.reset(); handles.reset(); strings.reset();
        if (!header.encodeDwg(version,&body,&handles,&strings)) {
            std::cerr << "encode/retry failed " << static_cast<int>(version) << '\n';return 1;
        }
        dwgBuffer in(body.data().data(),body.data().size());
        if (version >= DRW::AC1027 && in.getBitLongLong()!=0u) return 1;
        const bool modern=version>=DRW::AC1027;
        if (in.getBitDouble()!=(modern?412148564080.0:0.0)
            || in.getBitDouble()!=(modern?1.0:0.0)
            || in.getBitDouble()!=(modern?1.0:0.0)
            || in.getBitDouble()!=(modern?1.0:0.0)) {
            std::cerr << "opening defaults failed " << static_cast<int>(version) << '\n';return 1;
        }
        if (version < DRW::AC1021)
            for (unsigned i=0;i<4;++i) if (!in.getCP8Text().empty()) return 1;
        if (in.getBitLong()!=24 || in.getBitLong()!=0 || in.getBit()!=1u || in.getBit()!=0u || !in.isGood()) {
            std::cerr << "following fields failed " << static_cast<int>(version) << '\n';return 1;
        }
    }
    return 0;
}
