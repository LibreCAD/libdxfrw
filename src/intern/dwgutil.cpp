/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2025 librecad.org                                          **
**  Copyright (C) 2025 Dongxu Li (github.com/dxli)                           **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/
#include "dwgutil.h"
#include "drw_dbg.h"

duint32 dwgCompressor::copyBytes(duint32 size, duint8 *src, duint8 *dst){
    duint32 pos = 0;
    while (pos < size) {
        *dst++ = *src++;
        pos++;
    }
    return pos;
}

bool dwgCompressor::decompress18(duint8 *cbuf, duint8 *dbuf, duint32 csize, duint32 dsize){
    duint32 rpos = 0;
    duint32 wpos = 0;
    DRW_DBG("\ndwgCompressor::decompress18 start\n");
    //get compressed size
    duint32 compsize = (cbuf[0]<<8)|cbuf[1];
    //check header (not encrypted)
    if (cbuf[2] != 0x1F || cbuf[3] != 0x8B) {
        DRW_DBG("\ndwgCompressor::decompress18 warning not compressed data\n");
    }
    cbuf += 16;
    csize -= 16;
    DRW_DBG("\ndwgCompressor::decompress18 after get header\n");
    while (rpos < compsize) {
        duint8 op = cbuf[rpos++];
        DRW_DBGH(op);
        DRW_DBG("\ndwgCompressor::decompress18 op: "); DRW_DBGH(op);
        duint8 type = op >> 4;
        duint8 len = op & 0x0F;
        if (rpos >= compsize || wpos >= dsize) {
            return false;
        }
        switch (type) {
        case 0x01:
            //0b0001 L0 L1 literal len followed by 2 literals
            len = (len<<2) | ((cbuf[rpos]>>6)&0x03);
            len +=3;
            duint8 len2 = (cbuf[rpos++]&0x3F);
            len += len2;
            DRW_DBG(" literal len: "); DRW_DBG(len);
            if (rpos + len > compsize || wpos + len > dsize) {
                return false;
            }
            wpos += copyBytes(len, &cbuf[rpos], &dbuf[wpos]);
            rpos += len;
            DRW_DBG("\n literals: "); 
            for (duint32 i=0; i<len;i++) DRW_DBGH(dbuf[wpos-len+i]); DRW_DBG("\n");
            break;
        case 0x00:
            //0b0000 L literal len followed by literal
            len +=3;
            DRW_DBG(" literal len: "); DRW_DBG(len);
            if (rpos + len > compsize || wpos + len > dsize) {
                return false;
            }
            wpos += copyBytes(len, &cbuf[rpos], &dbuf[wpos]);
            rpos += len;
            DRW_DBG("\n literals: "); 
            for (duint32 i=0; i<len;i++) DRW_DBGH(dbuf[wpos-len+i]); DRW_DBG("\n");
            break;
        case 0x02:
            //0b0010 LL O literal len followed by offset
            len = (len>>2) & 0x03;
            len +=3;
            duint32 off = (op & 0x03) <<8;
            off += cbuf[rpos++];
            DRW_DBG(" literal len: "); DRW_DBG(len); DRW_DBG(" offset: "); DRW_DBG(off);
            if (wpos < off || wpos + len > dsize) {
                return false;
            }
            wpos += copyBytes(len, &dbuf[wpos-off], &dbuf[wpos]);
            DRW_DBG("\n copied bytes: "); 
            for (duint32 i=0; i<len;i++) DRW_DBGH(dbuf[wpos-len+i]); DRW_DBG("\n");
            break;
        case 0x03:
            //0b0011 O literal len =1 followed by offset
            duint32 off = (op & 0x07) <<8;
            off += cbuf[rpos++];
            DRW_DBG(" literal len: 1"); DRW_DBG(" offset: "); DRW_DBG(off);
            if (wpos < off || wpos + 1 > dsize) {
                return false;
            }
            wpos += copyBytes(1, &dbuf[wpos-off], &dbuf[wpos]);
            DRW_DBG("\n copied bytes: "); DRW_DBGH(dbuf[wpos-1]); DRW_DBG("\n");
            break;
        case 0x04:
            //0b0100 LL OOOO len followed by offset
            len = (op & 0x03) <<4;
            len += cbuf[rpos++] >>4;
            len +=9;
            duint32 off = (cbuf[rpos-1] & 0x0F) <<8;
            off += cbuf[rpos++];
            DRW_DBG(" literal len: "); DRW_DBG(len); DRW_DBG(" offset: "); DRW_DBG(off);
            if (wpos < off || wpos + len > dsize) {
                return false;
            }
            wpos += copyBytes(len, &dbuf[wpos-off], &dbuf[wpos]);
            DRW_DBG("\n copied bytes: "); 
            for (duint32 i=0; i<len;i++) DRW_DBGH(dbuf[wpos-len+i]); DRW_DBG("\n");
            break;
        default:
            //0b0101 and above reserved
            DRW_DBG("warning op "); DRW_DBGH(type);
            return false;
        }
    }
    DRW_DBG("\ndwgCompressor::decompress18 end\n");

    return true;
}

void dwgCompressor::decrypt18Hdr(duint8 *buf, duint32 size, duint32 offset){
    // Original code...
}
