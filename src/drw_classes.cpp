/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include "drw_classes.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/dwgbuffer.h"
#include "intern/drw_dbg.h"
#include <limits>
#include <string>

namespace {

bool isSafeDxfClassString(const std::string& value) {
    return value.find_first_of("\r\n") == std::string::npos
        && value.find('\0') == std::string::npos;
}

bool isValidDxfClass(const DRW_Class& cls) {
    // recName may be empty for proxy-only classes, but the C++ class name is
    // required by the DXF CLASS record and all emitted strings must be one
    // physical DXF value.
    if (cls.className.empty() || !isSafeDxfClassString(cls.recName)
        || !isSafeDxfClassString(cls.className)
        || !isSafeDxfClassString(cls.appName)) {
        return false;
    }
    if (cls.proxyFlag < 0
        || cls.proxyFlag > std::numeric_limits<std::uint16_t>::max()
        || cls.instanceCount < 0
        || cls.wasaProxyFlag < 0 || cls.wasaProxyFlag > 1
        || cls.entityFlag < 0 || cls.entityFlag > 1) {
        return false;
    }
    return true;
}

} // namespace


bool DRW_Class::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf){
    DRW_DBG("\n***************************** parsing Class *********************************************\n");

    if (buf == nullptr || strBuf == nullptr || !buf->isGood()
        || !strBuf->isGood()) {
        if (buf != nullptr)
            buf->invalidate();
        if (strBuf != nullptr && strBuf != buf)
            strBuf->invalidate();
        return false;
    }

    // A class is consumed from one or two bit streams depending on the DWG
    // version. Decode into independent cursors so a malformed record cannot
    // publish partial metadata or advance the caller past the failed field.
    DRW_Class parsed;
    dwgBuffer dataProbe = buf->forkIndependent();
    dwgBuffer stringProbe = strBuf == buf
        ? dataProbe : strBuf->forkIndependent();
    dwgBuffer *const classStrings = strBuf == buf
        ? &dataProbe : &stringProbe;

    parsed.classNum = dataProbe.getBitShort();
    DRW_DBG("Class number: "); DRW_DBG(parsed.classNum);
    parsed.proxyFlag = dataProbe.getBitShort(); //in dwg specs says "version"

    parsed.appName = classStrings->getVariableText(version, false);
    parsed.className = classStrings->getVariableText(version, false);
    parsed.recName = classStrings->getVariableText(version, false);

    DRW_DBG("\napp name: "); DRW_DBG(parsed.appName.c_str());
    DRW_DBG("\nclass name: "); DRW_DBG(parsed.className.c_str());
    DRW_DBG("\ndxf rec name: "); DRW_DBG(parsed.recName.c_str());
    parsed.wasaProxyFlag = dataProbe.getBit(); //in dwg says wasazombie
    parsed.entityFlagRaw = dataProbe.getBitShort();
    parsed.entityFlag = parsed.entityFlagRaw == 0x1F2 ? 1 : 0;

    DRW_DBG("\nProxy capabilities flag: "); DRW_DBG(parsed.proxyFlag);
    DRW_DBG(", proxy flag (280): "); DRW_DBG(parsed.wasaProxyFlag);
    DRW_DBG(", entity flag: "); DRW_DBGH(parsed.entityFlag);

    if (version > DRW::AC1015) {//2004+
        // Retain the observed BL producer-compatibility path. ODA5.8 documents
        // BL version/maintenance fields for R2007, while ODA10.2 says BS in its
        // general R2004+ description. Small values can encode alike; values
        // >=256 distinguish widths. Writer agreement alone cannot establish a
        // universal schema. Independent section/profile evidence is required
        // before changing this compatibility choice or promoting support.
        parsed.instanceCount = dataProbe.getBitLong();
        DRW_DBG("\nInstance Count: "); DRW_DBG(parsed.instanceCount);
        if (parsed.instanceCount < 0) {
            buf->invalidate();
            if (strBuf != buf)
                strBuf->invalidate();
            return false;
        }
        parsed.dwgVersion = dataProbe.getBitLong();
        DRW_DBG("\nDWG version: "); DRW_DBG(parsed.dwgVersion);
        parsed.maintenanceVersion = dataProbe.getBitLong();
        parsed.unknown1 = dataProbe.getBitLong();
        parsed.unknown2 = dataProbe.getBitLong();
        DRW_DBG("\nmaintenance version: "); DRW_DBG(parsed.maintenanceVersion);
        DRW_DBG("\nunknown 1: "); DRW_DBG(parsed.unknown1);
        DRW_DBG("\nunknown 2: "); DRW_DBG(parsed.unknown2);
    }

    if (!dataProbe.isGood() || !classStrings->isGood()) {
        buf->invalidate();
        if (strBuf != buf)
            strBuf->invalidate();
        return false;
    }

    parsed.toDwgType();
    *this = parsed;
    *buf = dataProbe;
    if (strBuf != buf)
        *strBuf = stringProbe;
    DRW_DBG("\n");
    return true;
}

bool DRW_Class::write(dxfWriter *writer, DRW::Version ver) const {
    if (writer == nullptr)
        return false;
    if (ver <= DRW::AC1009)
        return true;
    if (!isValidDxfClass(*this))
        return false;

    if (!writer->writeString(0, "CLASS") ||
        !writer->writeUtf8String(1, recName) ||
        !writer->writeUtf8String(2, className) ||
        !writer->writeUtf8String(3, appName) ||
        !writer->writeInt32(90, proxyFlag)) {
        return false;
    }
    if (ver > DRW::AC1015 &&
        !writer->writeInt32(91, instanceCount)) { // 2004+
        return false;
    }
    return writer->writeInt16(280, wasaProxyFlag) &&
           writer->writeInt16(281, entityFlag);
}

void DRW_Class::toDwgType(){
    if (recName == "LWPOLYLINE")
        dwgType = 77;
    else if (recName == "HATCH")
        dwgType = 78;
    else if (recName == "GROUP")
        dwgType = 72;
/*    else if (recName == "GROUP")
        dwgType = 72;*/
    else if (recName == "LAYOUT")
        dwgType = 82;
    else if (recName == "IMAGE")
        dwgType = 101;
    else if (recName == "IMAGEDEF")
        dwgType = 102;
    else if (recName == "NAVISWORKSMODEL")
        dwgType = 1150;
    else if (recName == "_3DLINE" || recName == "3DLINE")
        dwgType = 1162;
    else if (recName == "GEOPOSITIONMARKER" || recName == "POSITIONMARKER")
        dwgType = 1164;
    else if (recName == "ACDBPLACEHOLDER"
             && className == "AcDbPlaceHolder")
        // ODA §20.4.1 assigns non-fixed object frames through their class
        // entry. This built-in class is the same logical object as fixed
        // DWG type 80 (ACDBPLACEHOLDER); its source class ordinal remains
        // file-local and is never hardcoded here.
        dwgType = 80;
    else
        dwgType = 0;
    // NOTE: ARC_DIMENSION must NOT be added here. It has no fixed DWG type < 500.
    // Its classNum (>= 500) is assigned by the DWG class table at read time and by
    // writeDwgClasses() at write time. Dispatch goes via classesmap recName lookup
    // in dwgreader.cpp default: block. Adding dwgType=27 (or any < 500) would silently
    // redirect ARC_DIMENSION entities to the wrong parser (POINT is case 27).
}
