#ifndef LIBDXFRW_TESTS_GENERATED_RAW_DWG_OBJECT_H
#define LIBDXFRW_TESTS_GENERATED_RAW_DWG_OBJECT_H

#include <cstdint>
#include <string>
#include <vector>

#include "drw_objects.h"
#include "intern/dwgbufferw.h"

namespace libdxfrw_test {

inline dwgHandle rawObjectHandle(std::uint8_t code, std::uint32_t reference) {
    dwgHandle handle;
    handle.code = reference == 0 ? 0 : code;
    handle.size = 0;
    handle.ref = reference;
    handle.ref64 = reference;
    return handle;
}

// Generated-only custom OBJECT profiles exercise opaque readback across the
// older interleaved and modern detached streams. The common header and
// versioned string/handle layout follow ODA §§10.2/20.1.
inline DRW_UnsupportedObject makeRawCustomObject(
    DRW::Version version, std::uint16_t classNumber, std::uint32_t handle,
    const std::string& recordName, const std::string& className,
    const std::string& appName, std::uint32_t encodedHandle = 0,
    const std::string& singleVariableTextBody = {}) {
    if (encodedHandle == 0)
        encodedHandle = handle;

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    std::uint64_t objectSizeBit = 0;
    const bool hasExplicitObjectSize =
        version > DRW::AC1014 && version < DRW::AC1024;
    body.putObjType(version, classNumber);
    if (hasExplicitObjectSize) {
        objectSizeBit = body.bitCount();
        body.putRawLong32(0); // pre-R2010 object-data end bit
    }
    body.putHandle(rawObjectHandle(4, encodedHandle));
    body.putBitShort(0); // EED size
    body.putBitLong(0);  // reactor count
    if (version > DRW::AC1015)
        body.putBit(0); // R2004+: XDictionary handle follows
    if (version > DRW::AC1024)
        body.putBit(0); // R2013+: no DataStorage payload
    if (!singleVariableTextBody.empty()) {
        body.putBitLong(1);
        if (version > DRW::AC1018)
            strings.putVariableText(version, singleVariableTextBody);
        else
            body.putVariableText(version, singleVariableTextBody);
    } else {
        body.putRawLong32(0x12345678u);
        body.putRawShort16(0xABCDu);
        body.putRawChar8(0x5Au);
    }

    // R2007+ strings live at the end of the object data and are indexed by
    // the bounded string-stream footer.
    if (version > DRW::AC1018) {
        body.alignToByte();
        if (singleVariableTextBody.empty()) {
            // An empty, seven-bit stream keeps the string-presence encoding
            // valid for generated profiles with no variable text.
            for (int bit = 0; bit < 7; ++bit)
                body.putBit(0);
            body.putRawShort16(7);
        } else {
            const std::uint64_t stringBits = strings.bitCount();
            for (std::uint64_t bit = 0; bit < stringBits; ++bit) {
                body.putBit((strings.data()[static_cast<std::size_t>(bit / 8u)]
                             >> (7u - bit % 8u)) & 1u);
            }
            // The footer flag must occupy the last bit of the object's final
            // byte. Pad the detached stream to seven modulo eight bits, then
            // include that padding in its encoded length so the 16-bit size
            // field and final flag end exactly at the byte boundary.
            std::uint64_t encodedStringBits = stringBits;
            while ((encodedStringBits & 7u) != 7u) {
                body.putBit(0);
                ++encodedStringBits;
            }
            body.putRawShort16(
                static_cast<std::uint16_t>(encodedStringBits));
        }
        body.putBit(1);
    }
    body.alignToByte();
    if (hasExplicitObjectSize)
        body.patchRawLong32AtBit(
            objectSizeBit, static_cast<std::uint32_t>(body.bitCount()));

    handles.putHandle(rawObjectHandle(4, 0)); // parent
    handles.putHandle(rawObjectHandle(0, 0)); // XDictionary
    handles.alignToByte();

    DRW_UnsupportedObject object;
    object.m_version = version;
    object.m_objectType = classNumber;
    object.m_handle = handle;
    object.m_parentHandle = DRW::NoHandle;
    object.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::ValidatedAbsent);
    object.m_bodyBitSize = static_cast<std::uint32_t>(handles.bitCount());
    object.m_isEntity = false;
    object.m_isCustomClass = true;
    object.m_recordName = recordName;
    object.m_className = className;
    object.m_hasClassDefinition = true;
    object.m_classProxyFlag = 0x401;
    object.m_classAppName = appName;
    object.m_classEntityFlagRaw = 0x1F3;
    object.m_rawBytes = body.data();
    object.m_rawBytes.insert(object.m_rawBytes.end(), handles.data().begin(),
                             handles.data().end());
    object.m_objectSize = static_cast<std::uint32_t>(object.m_rawBytes.size());
    return object;
}

} // namespace libdxfrw_test

#endif // LIBDXFRW_TESTS_GENERATED_RAW_DWG_OBJECT_H
