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

#ifndef DWGUTIL_H
#define DWGUTIL_H

#include "../drw_base.h"

namespace DRW {
    std::string toHexStr(int n);
}

namespace dwgRSCodec {
    void decode239I(duint8 *in, duint8 *out, duint32 blk);
    void decode251I(duint8 *in, duint8 *out, duint32 blk);
};

class dwgCompressor {
public:
    dwgCompressor()=default;

    bool decompress18(duint8 *cbuf, duint8 *dbuf, duint64 csize, duint64 dsize);
    static void decrypt18Hdr(duint8 *buf, duint64 size, duint64 offset);
//    static void decrypt18Data(duint8 *buf, duint32 size, duint32 offset);
    static void decompress21(duint8 *cbuf, duint8 *dbuf, duint64 csize, duint64 dsize);

private:
    duint32 litLength18();
    static duint32 litLength21(duint8 *cbuf, duint8 oc, duint32 *si);
    static void copyCompBytes21(duint8 *cbuf, duint8 *dbuf, duint32 l, duint32 si, duint32 di);
    static void readInstructions21(duint8 *cbuf, duint32 *si, duint8 *oc, duint32 *so, duint32 *l);

    duint32 longCompressionOffset();
    duint32 long20CompressionOffset();
    duint32 twoByteOffset(duint32 *ll);

    duint32 compressedByte(void);
    duint32 decompByte(const duint32 index);
    void decompSet(const duint8 value);
    bool buffersGood(void);

    duint8 *compressedBuffer {nullptr};
    duint32 compressedSize {0};
    duint32 compressedPos {0};
    bool    compressedGood {true};
    duint8 *decompBuffer {nullptr};
    duint32 decompSize {0};
    duint32 decompPos {0};
    bool    decompGood {true};
};

namespace secEnum {
    enum DWGSection {
        UNKNOWNS,      /*!< UNKNOWN section. */
        FILEHEADER,    /*!< File Header (in R3-R15*/
        HEADER,        /*!< AcDb:Header */
        CLASSES,       /*!< AcDb:Classes */
        SUMARYINFO,    /*!< AcDb:SummaryInfo */
        PREVIEW,       /*!< AcDb:Preview */
        VBAPROY,       /*!< AcDb:VBAProject */
        APPINFO,       /*!< AcDb:AppInfo */
        FILEDEP,       /*!< AcDb:FileDepList */
        REVHISTORY,    /*!< AcDb:RevHistory */
        SECURITY,      /*!< AcDb:Security */
        OBJECTS,       /*!< AcDb:AcDbObjects */
        OBJFREESPACE,  /*!< AcDb:ObjFreeSpace */
        TEMPLATE,      /*!< AcDb:Template */
        HANDLES,       /*!< AcDb:Handles */
        PROTOTYPE,     /*!< AcDb:AcDsPrototype_1b */
        AUXHEADER,     /*!< AcDb:AuxHeader, in (R13-R15) second file header */
        SIGNATURE,     /*!< AcDb:Signature */
        APPINFOHISTORY,     /*!< AcDb:AppInfoHistory (in ac1021 may be a renamed section?*/
        EXTEDATA,      /*!< Extended Entity Data */
        PROXYGRAPHICS /*!< PROXY ENTITY GRAPHICS */
    };

    DWGSection getEnum(const std::string &nameSec);
};

#endif // DWGUTIL_H
