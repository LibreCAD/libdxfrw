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

// Generated-only AC1027 custom OBJECT used to exercise opaque readback.
// The common header, string stream, detached handle stream and outer frame
// follow the existing local round-trip control and ODA §§10.2/20.1 layout.
inline DRW_UnsupportedObject makeRawCustomObject(
    DRW::Version version, std::uint16_t classNumber, std::uint32_t handle,
    const std::string& recordName, const std::string& className,
    const std::string& appName, std::uint32_t encodedHandle = 0) {
    if (encodedHandle == 0)
        encodedHandle = handle;

    dwgBufferW body;
    dwgBufferW handles;
    body.putObjType(version, classNumber);
    body.putHandle(rawObjectHandle(4, encodedHandle));
    body.putBitShort(0); // EED size
    body.putBitLong(0);  // reactor count
    body.putBit(0);      // no extension dictionary
    body.putBit(0);      // no DataStorage payload
    body.putRawLong32(0x12345678u);
    body.putRawShort16(0xABCDu);
    body.putRawChar8(0x5Au);
    body.alignToByte();
    for (int bit = 0; bit < 7; ++bit)
        body.putBit(0); // empty R2007+ string stream
    body.putRawShort16(0);
    body.putBit(0);
    body.alignToByte();

    handles.putHandle(rawObjectHandle(4, 0)); // owner
    handles.putHandle(rawObjectHandle(0, 0)); // xdictionary
    handles.alignToByte();

    DRW_UnsupportedObject object;
    object.m_version = version;
    object.m_objectType = classNumber;
    object.m_handle = handle;
    object.m_parentHandle = DRW::NoHandle;
    object.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::ValidatedAbsent);
    object.m_bodyBitSize = static_cast<std::uint32_t>(handles.size() * 8u);
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
