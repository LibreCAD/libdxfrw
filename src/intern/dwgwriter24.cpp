/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**  Copyright (C) 2026 Dongxu Li (github.com/dxli)                            **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include "dwgwriter24.h"
#include "dwgutil.h"
#include "dwgbuffer.h"
#include "dwg_fixed_handles.h"
#include "dwg_default_material.h"
#include "../drw_entities.h"
#include <cctype>
#include <limits>

namespace {
static std::string toUpperCase(const std::string& s) {
    std::string r = s;
    for (auto& c : r)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return r;
}
} // namespace

bool dwgWriter24::appendR2007StringStream(dwgBufferW& data,
                                          const dwgBufferW& strings,
                                          bool writeAbsentFooter) {
    const std::uint64_t stringBits = strings.bitCount();
    if (stringBits > std::numeric_limits<std::uint32_t>::max()
        || stringBits > std::numeric_limits<std::uint64_t>::max()
                            - data.bitCount())
        return false;

    if (stringBits == 0 && !writeAbsentFooter)
        return true;

    if (stringBits == 0) {
        for (int bit = 0; bit < 7; ++bit)
            data.putBit(0);
        data.putRawShort16(0);
        data.putBit(0);
        return true;
    }

    // Keep the string start at the exact data bit boundary and copy only the
    // logical string bits. The backing vector's last byte may contain up to
    // seven buffer padding bits, which are not part of the string stream.
    // Pad the combined data/string stream before its size/presence trailer.
    const unsigned int paddingBits =
        static_cast<unsigned int>((7u - ((data.bitCount() + stringBits) & 7u)) & 7u);
    const std::uint64_t stringBitSize =
        stringBits + paddingBits;
    if (stringBitSize > 0x7FFFFFFFULL)
        return false;

    for (std::uint64_t bit = 0; bit < stringBits; ++bit)
        data.putBit(static_cast<std::uint8_t>(
            (strings.data()[static_cast<std::size_t>(bit / 8u)]
             >> (7u - bit % 8u)) & 1u));
    for (unsigned int bit = 0; bit < paddingBits; ++bit)
        data.putBit(0);
    if (stringBitSize <= 0x7FFFU) {
        data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    } else {
        data.putRawShort16(static_cast<std::uint16_t>(stringBitSize >> 15));
        data.putRawShort16(static_cast<std::uint16_t>(
            0x8000U | (stringBitSize & 0x7FFFU)));
    }
    data.putBit(1);
    return data.isGood();
}

// --- dwgWriter24::writeDwgHeader --------------------------------------------
// AC1024 HEADER section differs from AC1018 in that the handle stream is
// separated from the data stream.  Layout:
//   [begin sentinel 16B][RL size][RL bitSize][data bytes][handle bytes][RS CRC16][end sentinel 16B]
//
// bitSize = 32 + dataBytes * 8, so that the reader computes:
//   hBbuf->setPosition((160 + bitSize) >> 3) = 24 + dataBytes
// which is exactly the first byte after the data section.

bool dwgWriter24::writeDwgHeader() {
    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::HEADER] = static_cast<std::uint32_t>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::HEADER_BEGIN);

    // R2013 files emitted by this writer use application maintenance 6, so
    // the R2010/R2013 high-size field is required. R2018 always carries it.
    if (m_version == DRW::AC1027 || m_version >= DRW::AC1032)
        m_buf.putRawLong32(0);

    // AC1024+: RL bitSize placeholder.
    size_t bitSizeOffset = m_buf.size();
    m_buf.putRawLong32(0);

    initHeaderControlHandles();

    if (m_header != nullptr) {
        dwgBufferW dataBuf;
        dwgBufferW handleBuf;
        dwgBufferW strBuf;
        std::array<std::uint64_t, 4> currentReferenceBits{};
        std::uint32_t currentMaterial = 0;
        if (defaultDwgMaterialGraphActive()) {
            bool supplied = false;
            if (!m_header->getDxfMaterialReferenceHandle(currentMaterial, supplied)) return false;
            if (!supplied) currentMaterial = m_defaultMaterialHandles[2];
            else if (std::find(m_defaultMaterialHandles.begin()+1, m_defaultMaterialHandles.end(),
                               currentMaterial) == m_defaultMaterialHandles.end()) return false;
        }
        // Preserve the legacy writer entry point and bind the two witnessed
        // material header references only for the explicit opt-in graph.
        if (!m_header->encodeDwgForWriterMaterial(
                m_version, &dataBuf, &handleBuf, &strBuf,
                DRW::DwgNamedObjectsDictionaryHandle,
                m_plotSettingsDictionaryHandle, &currentReferenceBits,
                DRW::DwgAcadGroupDictionaryHandle,
                m_defaultPlotStyleDictionaryHandle,
                m_defaultMaterialHandles[0], currentMaterial))
            return false;
        dataBuf.alignToByte();
        strBuf.alignToByte();
        handleBuf.alignToByte();

        if (m_version > DRW::AC1018) {
            if (!appendR2007StringStream(dataBuf, strBuf, true))
                return false;
        }

        if (dataBuf.size() >
            (std::numeric_limits<std::uint32_t>::max() - 32u) / 8u)
            return false;
        std::uint32_t bitSize = 32u + static_cast<std::uint32_t>(dataBuf.size()) * 8u;
        m_buf.patchRawLong32(bitSizeOffset, bitSize);

        const std::uint64_t dataStartBit = m_buf.bitCount();
        m_buf.putBytes(dataBuf.data().data(), dataBuf.size());
        if (!recordHeaderHandseedOffset(dataStartBit))
            return false;
        if (!recordHeaderCurrentReferenceOffsets(m_buf.bitCount(),
                                                 currentReferenceBits))
            return false;
        if (handleBuf.size() > 0)
            m_buf.putBytes(handleBuf.data().data(), handleBuf.size());
    }

    if (m_version >= DRW::AC1027) {
        // In the observed R2013/R2018 HEADER profile, the low size excludes
        // the optional high-size word.  The CRC still covers both words and
        // the complete payload, beginning immediately after the sentinel.
        m_buf.alignToByte();
        if (m_buf.size() < sizeOffset + 8u
            || m_buf.size() - (sizeOffset + 8u)
                   > std::numeric_limits<std::uint32_t>::max())
            return false;
        m_buf.patchRawLong32(sizeOffset, static_cast<std::uint32_t>(
            m_buf.size() - (sizeOffset + 8u)));
        const auto crc = m_buf.crc16(0xC0C1, sectionStart + 16u, m_buf.size());
        m_buf.putRawShort16(crc);
        m_buf.putBytes(dwgSentinels::HEADER_END, 16);
    } else {
        endSentinelSection(sectionStart, sizeOffset, dwgSentinels::HEADER_END);
    }

    m_sectionSizes[recno::HEADER] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

// --- dwgWriter24::writeDwgClasses -------------------------------------------
// AC1024 CLASSES section differs from AC1018 in two ways:
//   1. An RL bitSize field is inserted after RL size.
//   2. Eight ODA-defined unknown zero bytes are written before the end
//      sentinel.
//
bool dwgWriter24::writeDwgClasses() {
    if (hasDwgClassConflict() || !validateDwgClassManifest())
        return false;

    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::CLASSES] = static_cast<std::uint32_t>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::CLASSES_BEGIN);

    const bool hasHSize = m_version == DRW::AC1027 || m_version >= DRW::AC1032;
    if (hasHSize)
        m_buf.putRawLong32(0);

    // RL bitSize placeholder — back-patched after class data is written.
    size_t bitSizeOffset = m_buf.size();
    m_buf.putRawLong32(0);

    // Class numeric data and class strings are split for R2007+ in the same
    // way object bodies split data/strings/handles. The reader finds the
    // string stream by using the bitSize value plus the fixed CLASSES header
    // prefix (159 bits, or 191 bits when AC1032 hSize is present).
    m_buf.putBitShort(maxDwgClassNumber());
    m_buf.putRawChar8(0);   // rc1
    m_buf.putRawChar8(0);   // rc2
    m_buf.putBit(1);        // ODA 10.2: CLASSES header flag is true

    dwgBufferW classStrings;
    if (!emitDwgClassDefinitions(
            [this, &classStrings](const DwgClassDefinition& definition) {
                return writeDwgClassDefinition(definition, &m_buf,
                                               &classStrings);
            }))
        return false;

    m_buf.alignToByte();
    // The footer describes encoded text bits, not the backing byte capacity.
    // Rounding this stream up inserts trailing non-text bits that an ordinary
    // count-driven reader skips but a complete stream observer must refuse.
    const std::uint64_t stringBitSize = classStrings.bitCount();
    if (stringBitSize > 0x7FFFFFFFu)
        return false;
    const auto wholeBytes = static_cast<std::size_t>(stringBitSize / 8u);
    if (wholeBytes != 0u)
        m_buf.putBytes(classStrings.data().data(), wholeBytes);
    for (unsigned bit = 0; bit < stringBitSize % 8u; ++bit)
        m_buf.putBit((classStrings.data()[wholeBytes] >> (7u - bit)) & 1u);
    if (stringBitSize > 0x7FFFu) {
        // The extended footer is written high word first because the reader
        // walks backwards from the low word at the end of the stream.
        m_buf.putRawShort16(static_cast<std::uint16_t>(stringBitSize >> 15));
        m_buf.putRawShort16(static_cast<std::uint16_t>(
            0x8000u | (stringBitSize & 0x7FFFu)));
    } else if (stringBitSize != 0u) {
        m_buf.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    }
    // The class string stream terminates with its presence bit. bitSize includes
    // its own 32-bit field and everything through that marker; the reader's
    // 159/191-bit base then lands on the marker's first bit.
    m_buf.putBit(stringBitSize != 0u ? 1u : 0u);
    const std::uint64_t endBit =
        m_buf.bitCount()
        - static_cast<std::uint64_t>(sectionStart) * 8u;
    const std::uint64_t dataStartBit =
        (static_cast<std::uint64_t>(bitSizeOffset) + 4u) * 8u
        - static_cast<std::uint64_t>(sectionStart) * 8u;
    const std::uint64_t bitSize64 = 32u + endBit - dataStartBit;
    if (bitSize64 > std::numeric_limits<std::uint32_t>::max())
        return false;
    const std::uint32_t bitSize = static_cast<std::uint32_t>(bitSize64);
    m_buf.patchRawLong32(bitSizeOffset, bitSize);

    m_buf.alignToByte();

    // The low RL counts the class data through the byte before the CRC.
    // R2007+ stores an optional high-size RL immediately after it, so exclude
    // that word from the low count (ODA 10.2; the reader locates the CRC from
    // the end of the complete size prefix).
    const std::size_t lowSizePrefixBytes = sizeof(std::uint32_t)
        + (hasHSize ? sizeof(std::uint32_t) : 0u);
    if (sizeOffset > m_buf.size()
        || m_buf.size() - sizeOffset < lowSizePrefixBytes)
        return false;
    const std::size_t payloadSize = m_buf.size()
        - sizeOffset - lowSizePrefixBytes;
    if (payloadSize > std::numeric_limits<std::uint32_t>::max())
        return false;
    m_buf.patchRawLong32(sizeOffset,
                         static_cast<std::uint32_t>(payloadSize));

    const std::uint16_t crc = m_buf.crc16(0xC0C1, sectionStart + 16, m_buf.size());
    m_buf.putRawShort16(crc);
    for (std::size_t i = 0; i < dwgSpec::kClassesUnknownTailBytes; ++i)
        m_buf.putRawChar8(0);

    m_buf.putBytes(dwgSentinels::CLASSES_END, 16);

    m_sectionSizes[recno::CLASSES] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

// --- dwgWriter24::encodeEntity ----------------------------------------------
// Encodes a single entity using the AC1024 three-stream model:
//   m_objectBody   — numeric DATA fields
//   m_objectStrings — variable-text (TV/TU) string fields
//   m_objectHandles — handle fields
// finishObject() assembles these into the AC1024 wire format.

bool dwgWriter24::encodeEntity(DRW_Entity *ent) {

    const bool polylineChild = (m_version == DRW::AC1021 || m_version == DRW::AC1024) && ent != nullptr
        && m_ac1021PolylineChildPermit == ent
        && (dynamic_cast<DRW_Vertex*>(ent) != nullptr
            || dynamic_cast<DRW_SeqEnd*>(ent) != nullptr);
    if (polylineChild)
        m_ac1021PolylineChildPermit = nullptr;
    if (ent == nullptr || objectWriteFailed() || blockControlEmitted())
        return false;

    // The case-folding allocation must happen before writer or caller state
    // changes so an allocation failure permits an unchanged retry.
    DRW_Text* resolvedText = nullptr;
    std::uint32_t resolvedTextStyle = 0u;
    if (ent != nullptr && ent->eType == DRW::TEXT
        && dynamic_cast<DRW_RText*>(ent) == nullptr
        && dynamic_cast<DRW_ArcAlignedText*>(ent) == nullptr
        && dynamic_cast<DRW_Attrib*>(ent) == nullptr
        && dynamic_cast<DRW_MText*>(ent) == nullptr) {
        auto* text = dynamic_cast<DRW_Text*>(ent);
        if (text != nullptr && text->styleH.ref == 0u && text->styleH.ref64 == 0u
            && !text->style.empty()) {
            const auto styleIt = m_writingCtx.styleMap.find(toUpperCase(text->style));
            if (styleIt != m_writingCtx.styleMap.end()) {
                resolvedText = text;
                resolvedTextStyle = styleIt->second;
            }
        }
    }

    m_lastEntityWriteCheckpointValid = false;
    m_lastEntityWriteUsesCompoundCheckpoint = false;
    m_lastEntityCompoundCheckpoint = {};
    EntityFrameCheckpoint checkpoint = checkpointEntityFrame();
    CompoundWriteCheckpoint classCheckpoint;
    bool hasClassCheckpoint = false;
    bool frameStarted = false;
    const auto fail = [this, &checkpoint, &classCheckpoint,
                       &hasClassCheckpoint, &frameStarted]() {
        if (hasClassCheckpoint)
            rollbackCompoundWrite(classCheckpoint);
        else
            rollbackEntityFrame(checkpoint, frameStarted);
        return false;
    };

    if (!prepareEntityEed(*ent)) {
        m_entityEedWriteFailure = !ent->extData.empty();
        return fail();
    }
    if (ent->handle == 0) {
        ent->handle = m_handles.next();
    } else {
        m_handles.reserve(ent->handle);
    }
    if (m_dwgClassInstanceHandleOwners.find(ent->handle)
        != m_dwgClassInstanceHandleOwners.end()) {
        classCheckpoint = checkpointCompoundWrite();
        hasClassCheckpoint = true;
    }
    // Layer name → handle resolution (same logic as dwgWriter15::encodeEntity).
    if (ent->layerH.ref == 0) {
        auto layerUp = toUpperCase(ent->layer);
        auto it = m_writingCtx.layerMap.find(layerUp);
        ent->layerH.ref = (it != m_writingCtx.layerMap.end())
            ? it->second
            : static_cast<std::uint32_t>(0x12);
    }
    if (ent->lTypeH.ref == 0 && !ent->lineType.empty()) {
        if (auto it = m_writingCtx.ltypeMap.find(toUpperCase(ent->lineType));
            it != m_writingCtx.ltypeMap.end())
            ent->lTypeH.ref = it->second;
    }
    if (ent->eType == DRW::MLINE) {
        auto *mline = dynamic_cast<DRW_MLine *>(ent);
        if (mline != nullptr && mline->styleHandle == 0 && !mline->styleName.empty()) {
            if (auto styleIt = m_writingCtx.mlineStyleMap.find(toUpperCase(mline->styleName));
                styleIt != m_writingCtx.mlineStyleMap.end())
                mline->styleHandle = styleIt->second;
        }
    }
    // Resolve a TEXT style name only when the caller did not supply a
    // handle. The handle is drawing-local, so restore caller state on every
    // return, including exceptions and failed staged writes.
    struct ScopedTextStyleHandle {
        DRW_Text* text{nullptr};
        dwgHandle original{};
        ~ScopedTextStyleHandle() { if (text != nullptr) text->styleH = original; }
    } textStyleScope;
    if (resolvedText != nullptr) {
        textStyleScope.text = resolvedText;
        textStyleScope.original = resolvedText->styleH;
        resolvedText->styleH.ref = resolvedTextStyle;
    }
    prepareBlockOwnedEntity(*ent);
    if (!canRecordBlockOwnedEntity(*ent))
        return fail();

    // The outer R2018 multiline-attribute pointer is drawing-local. Resolve
    // it through the APPID identity collected before the APPID table was
    // emitted, and restore the source-model handle immediately after codec
    // execution so callers never observe an output handle in their input
    // object. This belongs in the R2007+ override: AC1032 inherits this
    // three-stream encoder rather than dwgWriter15::encodeEntity.
    DRW_Attrib* outerAnnotativeAttribute = dynamic_cast<DRW_Attrib*>(ent);
    struct OuterAnnotativeAppHandleRestore {
        DRW_Attrib* attribute {nullptr};
        std::uint32_t sourceHandle {0u};

        ~OuterAnnotativeAppHandleRestore() {
            if (attribute != nullptr)
                attribute->m_r2018OuterAnnotativeAppHandle = sourceHandle;
        }
    } restoreOuterAppHandle;
    if (m_version >= DRW::AC1032 && outerAnnotativeAttribute != nullptr
        && !outerAnnotativeAttribute->m_r2018OuterAnnotativeData.empty()) {
        std::string appIdName =
            outerAnnotativeAttribute->m_r2018OuterAnnotativeAppIdName;
        if (appIdName.empty()
            && (outerAnnotativeAttribute->m_r2018OuterAnnotativeAppHandle == 0
                || outerAnnotativeAttribute->m_r2018OuterAnnotativeAppHandle
                    == DRW::DwgAcadAppIdHandle))
            appIdName = "ACAD";
        const std::uint32_t outputAppHandle = findAppIdHandle(appIdName);
        if (outputAppHandle == 0) {
            // A callback-level false is normally a skippable entity. Here it
            // would silently turn an unsupported APPID identity into a
            // successful but semantically corrupt drawing, so poison the
            // writer and make the outer output transaction abort.
            m_writeError = true;
            return fail();
        }
        restoreOuterAppHandle.attribute = outerAnnotativeAttribute;
        restoreOuterAppHandle.sourceHandle =
            outerAnnotativeAttribute->m_r2018OuterAnnotativeAppHandle;
        outerAnnotativeAttribute->m_r2018OuterAnnotativeAppHandle =
            outputAppHandle;
    }

    frameStarted = true;
    beginObject(ent->handle);
    bool ok = false;
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
        m_objectHandles.reset();
        ok = ent->encodeDwg(m_version, &m_objectBody, 0,
                             &m_objectStrings, &m_objectHandles);
    } else {
        ok = ent->encodeDwg(m_version, &m_objectBody, 0);
    }
    if (!ok) {
        if (!ent->extData.empty())
            m_entityEedWriteFailure = true;
        return fail();
    }
    finishObject();
    if (objectWriteFailed())
        return fail();
    if (!polylineChild && !recordBlockOwnedEntity(*ent, &checkpoint.blockOwnerHandle))
        return fail();
    checkpoint.entityHandle = ent->handle;
    checkpoint.blockOwnedEntityRecorded = !polylineChild;
    if (!recordBlockInsertReference(*ent))
        return fail();
    if (const auto* insert = dynamic_cast<const DRW_Insert*>(ent)) {
        checkpoint.insertOwnerHandle = insert->blockRecH.ref;
        checkpoint.blockInsertReferenceRecorded = m_version > DRW::AC1014;
    }

    m_lastEntityFrameCheckpoint = checkpoint;
    if (hasClassCheckpoint) {
        m_lastEntityCompoundCheckpoint = std::move(classCheckpoint);
        m_lastEntityWriteUsesCompoundCheckpoint = true;
    }
    m_lastEntityWriteCheckpointValid = true;
    return true;
}

// --- dwgWriter24::finishObject ----------------------------------------------
// AC1024 object wire format:
//   [MS totalBodyBytes][UMC handleBits][data section bytes][handle section bytes][RS CRC16]
//
// data section = m_objectBody bytes + string section tail (RS strBitCount + B flag) + pad
// handle section = m_objectHandles bytes (byte-aligned)
//
// objSize (used by reader to locate handle section) = dataSectionBytes * 8.
// handleBits (= bs) = handleSectionBytes * 8.

void dwgWriter24::finishObject() {
    if (rejectDefaultDwgMaterialFrameConflicts()) { m_writeError = true; return; }
    // Keep the R2010+ three-stream writer consistent with the legacy frame
    // assembler: reject duplicate handles before appending a second frame.
    if (hasEmittedDwgObjectHandle(m_currentHandle)) {
        m_writeError = true;
        return;
    }

    // --- Assemble the DATA section -----------------------------------------
    if (!m_buf.isGood() || !m_objectBody.isGood() || !m_objectStrings.isGood()
        || !m_objectHandles.isGood()) {
        m_writeError = true;
        return;
    }
    // Keep strings contiguous with the scalar data. The footer accounts for
    // trailing alignment; its low size word carries 0x8000 when the preceding
    // word supplies the upper 16 bits. Preserve the no-strings framing.
    // These source-backed empty R2013 controls already end with a codec-owned
    // absent-string bit. Their handles follow it without intervening padding;
    // the UMC handle size includes the terminal padding bits.
    const bool profileView = m_currentHandle == DRW::DwgViewControlHandle;
    const bool profileUcs = m_currentHandle == DRW::DwgUcsControlHandle;
    const bool profileBlock = m_currentHandle == DRW::DwgBlockControlHandle;
    const bool exactEmptyControl = m_version == DRW::AC1027
        && (profileBlock || profileView || profileUcs)
        && m_objectStrings.data().empty()
        && m_objectBody.bitCount() == 35u
        && m_objectHandles.bitCount() == (profileBlock ? 48u : 16u);
    // Exact-bit trailers are witnessed only for the opt-in AC1027 default
    // material graph and its internally synthesized layer 0.
    const bool exactLayer = m_version == DRW::AC1027
        && defaultDwgMaterialGraphActive() && !m_haveLayer0
        && m_currentHandle == DRW::DwgLayer0Handle;
    const bool exactMaterial = m_version == DRW::AC1027
        && m_writingDefaultMaterialGraph
        && std::find(m_defaultMaterialHandles.begin() + 1,
                     m_defaultMaterialHandles.end(), m_currentHandle)
               != m_defaultMaterialHandles.end();
    const bool exactStrings = exactLayer || exactMaterial;
    const bool mergedFrame = exactEmptyControl || exactStrings;
    const bool protectedFrame = exactEmptyControl || exactLayer;
    const auto exactFrameStart = m_buf.size();
    const auto exactObjectMapSize = m_objectMap.size();
    const auto exactGeneration = m_nextDwgObjectFrameGeneration;
    const auto failExactEmptyControl = [&]() noexcept {
        m_buf.truncate(exactFrameStart);
        while (m_objectMap.size() > exactObjectMapSize) {
            m_emittedDwgObjectHandles.erase(m_objectMap.back().first);
            m_objectMap.pop_back();
        }
        m_nextDwgObjectFrameGeneration = exactGeneration;
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        m_currentDwgObjectFrameProvenance = {};
        m_currentHandle = 0;
        m_objectBody.reset();
        m_objectStrings.reset();
        m_objectHandles.reset();
        m_writeError = true;
    };
    try {
    if (exactEmptyControl) {
            dwgBuffer data(m_objectBody.data().data(), m_objectBody.size());
            dwgBuffer handles(m_objectHandles.data().data(), m_objectHandles.size());
            const auto type = data.getObjType(m_version);
            const auto own = data.getHandle();
            const auto eed = data.getBitShort();
            const auto reactors = data.getBitLong();
            const auto missingXdict = data.getBit();
            const auto hasDataStorage = data.getBit();
            const auto entries = data.getBitLong();
            const auto stringsPresent = data.getBit();
            const auto owner = handles.getHandle();
            const auto xdict = handles.getHandle();
            dwgHandle model, paper;
            if (profileBlock) {
                model = handles.getHandle();
                paper = handles.getHandle();
            }
            if (!data.isGood() || !handles.isGood()
                || m_dwgClassInstanceHandleOwners.find(m_currentHandle)
                       != m_dwgClassInstanceHandleOwners.end()
                || type != (profileBlock ? DRW::DwgBlockControlObjectType
                           : profileView ? DRW::DwgViewControlObjectType
                                            : DRW::DwgUcsControlObjectType)
                || own.code != 0u || own.ref != m_currentHandle
                || eed != 0 || reactors != 0 || missingXdict != 0
                || hasDataStorage != 0 || entries != 0 || stringsPresent != 0
                || data.getPosition() * 8u + data.getBitPos() != 35u
                || owner.code != DRW::DwgSoftPointer || owner.ref != 0u
                || xdict.code != DRW::DwgHardOwnership || xdict.ref != 0u
                || (profileBlock && (model.code != DRW::DwgHardOwnership
                || model.ref != DRW::DwgModelSpaceBlockRecordHandle
                || paper.code != DRW::DwgHardOwnership
                || paper.ref != DRW::DwgPaperSpaceBlockRecordHandle))) {
                failExactEmptyControl();
                return;
            }
    }
    if (exactStrings) {
        if (m_objectBody.data().empty() || m_objectStrings.data().empty()
            || m_objectHandles.data().empty()) {
            if (protectedFrame) failExactEmptyControl();
            else m_frameWriteError = true;
            return;
        }
        dwgBuffer data(m_objectBody.data().data(), m_objectBody.size());
        dwgBuffer handles(m_objectHandles.data().data(), m_objectHandles.size());
        const auto type = data.getObjType(m_version);
        const auto own = data.getHandle();
        const auto eed = data.getBitShort();
        const auto reactors = data.getBitLong();
        const auto missingXdict = data.getBit();
        const auto dataStorage = data.getBit();
        const auto owner = handles.getHandle();
        bool valid = data.isGood() && handles.isGood() && own.code == 0u
            && own.ref == m_currentHandle && eed == 0 && missingXdict == 1
            && dataStorage == 0 && owner.code == DRW::DwgSoftPointer;
        if (exactLayer) {
            const auto xref = handles.getHandle();
            const auto plot = handles.getHandle();
            const auto material = handles.getHandle();
            const auto ltype = handles.getHandle();
            const auto visual = handles.getHandle();
            dwgBufferW expectedName;
            expectedName.putVariableText(m_version, "0");
            valid = valid && expectedName.isGood()
                && expectedName.bitCount() == m_objectStrings.bitCount()
                && expectedName.data() == m_objectStrings.data();
            valid = valid && type == DRW::DwgLayerObjectType && reactors == 0
                && m_dwgClassInstanceHandleOwners.find(m_currentHandle)
                      == m_dwgClassInstanceHandleOwners.end()
                && owner.ref == DRW::DwgLayerControlHandle
                && xref.code == DRW::DwgHardPointer && xref.ref == 0u
                && plot.code == DRW::DwgHardPointer
                && plot.ref == m_defaultPlotStylePlaceholderHandle
                && material.code == DRW::DwgHardPointer
                && material.ref == m_defaultMaterialHandles[3]
                && ltype.code == DRW::DwgHardPointer && ltype.ref != 0u
                && visual.code == DRW::DwgHardPointer && visual.ref == 0u
                && m_objectBody.bitCount() == 96u
                && m_objectStrings.bitCount() == 42u;
        } else {
            const auto reactor = handles.getHandle();
            const bool global = m_currentHandle == m_defaultMaterialHandles[3];
            const auto kind = global ? drwDwgDefaultMaterial::Kind::Global
                : m_currentHandle == m_defaultMaterialHandles[1]
                    ? drwDwgDefaultMaterial::Kind::ByBlock
                    : drwDwgDefaultMaterial::Kind::ByLayer;
            dwgBufferW expectedBody, expectedStrings;
            const auto bodyStart = data.getPosition() * 8u + data.getBitPos();
            valid = valid && drwDwgDefaultMaterial::build(
                m_version, kind, expectedBody, expectedStrings)
                && bodyStart + expectedBody.bitCount() == m_objectBody.bitCount()
                && expectedStrings.bitCount() == m_objectStrings.bitCount()
                && expectedStrings.data() == m_objectStrings.data();
            for (std::uint64_t i = 0; valid && i < expectedBody.bitCount(); ++i) {
                const auto actual = (m_objectBody.data()[(bodyStart + i) / 8u]
                    >> (7u - (bodyStart + i) % 8u)) & 1u;
                const auto expected = (expectedBody.data()[i / 8u]
                    >> (7u - i % 8u)) & 1u;
                valid = actual == expected;
            }
            valid = valid && type == typedDwgClassNum("AcDbMaterial", "MATERIAL", 0)
                && reactors == 1 && owner.ref == m_defaultMaterialHandles[0]
                && reactor.code == DRW::DwgSoftPointer
                && reactor.ref == m_defaultMaterialHandles[0]
                && m_objectStrings.bitCount() == expectedStrings.bitCount();
        }
        valid = valid && handles.isGood()
            && handles.getPosition() * 8u + handles.getBitPos() == m_objectHandles.bitCount();
        if (!valid) {
            if (protectedFrame) failExactEmptyControl();
            else m_frameWriteError = true;
            return;
        }
    }
    if (m_objectStrings.data().empty() && !mergedFrame)
        m_objectBody.alignToByte();
    const std::uint64_t mergedStringBaseBit = m_objectBody.bitCount();
    std::uint64_t mergedHandleBaseBit = 0;
    if (exactStrings) {
        const auto stringBits = m_objectStrings.bitCount();
        for (std::uint64_t i = 0; i < stringBits; ++i)
            m_objectBody.putBit(static_cast<std::uint8_t>(
                (m_objectStrings.data()[i / 8u] >> (7u - i % 8u)) & 1u));
        m_objectBody.putRawShort16(static_cast<std::uint16_t>(stringBits));
        m_objectBody.putBit(1u);
        mergedHandleBaseBit = m_objectBody.bitCount();
        m_objectBody.putBytes(m_objectHandles.data().data(), m_objectHandles.size());
        m_objectBody.alignToByte();
    } else if (exactEmptyControl) {
        mergedHandleBaseBit = m_objectBody.bitCount();
        m_objectBody.putBytes(m_objectHandles.data().data(), m_objectHandles.size());
        m_objectBody.alignToByte();
        if (!m_objectBody.isGood()) {
            failExactEmptyControl();
            return;
        }
    } else if (!appendR2007StringStream(m_objectBody, m_objectStrings, true)) {
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    // The assembled data/string/footer stream ends on a byte boundary.

    // --- Byte-align the handle section -------------------------------------
    m_objectHandles.alignToByte();

    if (m_objectBody.size() > std::numeric_limits<std::uint32_t>::max()
        || m_objectHandles.size() > std::numeric_limits<std::uint32_t>::max()
        || m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        if (protectedFrame) { failExactEmptyControl(); return; }
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    const auto dataBytes = static_cast<std::uint32_t>(m_objectBody.size());
    const auto handleBytes = mergedFrame ? 0u : static_cast<std::uint32_t>(m_objectHandles.size());
    const std::uint64_t totalBytes64 =
        static_cast<std::uint64_t>(dataBytes) + handleBytes;
    if (handleBytes > std::numeric_limits<std::uint32_t>::max() / 8u
        || totalBytes64 > std::numeric_limits<std::int32_t>::max()) {
        if (protectedFrame) { failExactEmptyControl(); return; }
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    const auto totalBytes = static_cast<std::uint32_t>(totalBytes64);
    const auto bs = mergedFrame
        ? static_cast<std::uint32_t>(std::uint64_t(dataBytes) * 8u - mergedHandleBaseBit)
        : handleBytes * 8u;  // includes terminal padding

    std::uint32_t frameStart = static_cast<std::uint32_t>(m_buf.size());

    // MS totalBodyBytes + UMC bs + body bytes.
    m_buf.putModularShort(static_cast<std::int32_t>(totalBytes));
    if (!m_buf.isGood()) {
        if (protectedFrame) { failExactEmptyControl(); return; }
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    m_buf.putUModularChar(bs);
    if (!m_buf.isGood()) {
        if (protectedFrame) { failExactEmptyControl(); return; }
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    size_t bodyStart = m_buf.size();
    m_buf.putBytes(m_objectBody.data().data(), dataBytes);
    if (handleBytes > 0)
        m_buf.putBytes(m_objectHandles.data().data(), handleBytes);

    // CRC covers MS prefix + UMC bs + body bytes.
    std::uint16_t crc = m_buf.crc16(0xC0C1, frameStart, bodyStart + totalBytes);
    m_buf.putRawShort16(crc);
    if (protectedFrame && !m_buf.isGood()) {
        failExactEmptyControl();
        return;
    }

    if (!captureLastDwgObjectHandleOccurrences(mergedStringBaseBit, mergedHandleBaseBit, true,
                                               mergedFrame)) {
        if (protectedFrame) { failExactEmptyControl(); return; }
        m_buf.truncate(frameStart);
        m_writeError = true;
        m_currentHandle = 0;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    if (!publishDwgObjectMapEntry(m_currentHandle, frameStart)) {
        if (protectedFrame) { failExactEmptyControl(); return; }
        m_buf.truncate(frameStart);
        m_writeError = true;
        m_currentHandle = 0;
        m_objectStrings.reset();
        m_objectHandles.reset();
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        m_currentDwgObjectFrameProvenance = {};
        return;
    }
    markDwgClassInstanceEmitted(m_currentHandle);
    m_currentHandle = 0;
    } catch (...) {
        if (!protectedFrame)
            throw;
        failExactEmptyControl();
    }
}

bool dwgWriter24::finalize() {
    return !m_writeError && dwgWriter18::finalize();
}
