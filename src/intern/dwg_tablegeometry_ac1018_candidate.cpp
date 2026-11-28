/* Bounded opt-in geometry observer; see header for scope and limitations. */
#include "dwg_tablegeometry_ac1018_candidate.h"
#include "dwgbuffer.h"

#include <cmath>
#include <limits>

namespace DwgTableGeometryAc1018Candidate {
namespace {
constexpr std::size_t maxBodyBytes = 4096u;

std::uint64_t bit(const dwgBuffer& b) noexcept {
    return b.getPosition() * 8u + b.getBitPos();
}
bool longValue(dwgBuffer& b, std::uint64_t end, std::int32_t& value) {
    const auto tag = b.get2Bits();
    if (tag == 0u) value = static_cast<std::int32_t>(b.getRawLong32());
    else if (tag == 1u) value = b.getRawChar8();
    else if (tag == 2u) value = 0;
    else return false; // ODA BL selector 3 is reserved.
    return b.isGood() && bit(b) <= end;
}
bool shortValue(dwgBuffer& b, std::uint64_t end, std::uint16_t& value) {
    value = b.getBitShort();
    return b.isGood() && bit(b) <= end;
}
bool nonnullHandle(dwgBuffer& b, std::uint8_t role, std::uint32_t& value) {
    const dwgHandle h = b.getHandle();
    if (!b.isGood() || h.rawCode != role || h.size == 0u || h.ref64 == 0u
        || h.ref64 > std::numeric_limits<std::uint32_t>::max()) return false;
    value = static_cast<std::uint32_t>(h.ref64);
    return true;
}
bool nullSoftPointer(dwgBuffer& b) {
    const dwgHandle h = b.getHandle();
    return b.isGood() && h.rawCode == 4u && h.size == 0u && h.ref64 == 0u;
}
}

bool inspect(const DRW_UnsupportedObject& raw, Result& result) noexcept {
    result = Result{};
    if (raw.m_version != DRW::AC1018 || raw.m_isEntity || !raw.m_isCustomClass
        || !raw.m_hasClassDefinition || raw.m_hasDataStorage
        || raw.m_recordName != "TABLEGEOMETRY" || raw.m_className != "AcDbTableGeometry"
        || raw.m_objectType < 500 || raw.m_objectType > std::numeric_limits<std::uint16_t>::max()
        || raw.m_handle == 0u || raw.m_handle == DRW::NoHandle || raw.m_rawBytes.empty()
        || raw.m_rawBytes.size() > maxBodyBytes || raw.m_objectSize != raw.m_rawBytes.size()
        || raw.m_bodyBitSize != 0u) return false;
    try {
        Result candidate;
        dwgBuffer b(raw.m_rawBytes.data(), raw.m_rawBytes.size(), nullptr);
        const auto total = static_cast<std::uint64_t>(raw.m_rawBytes.size()) * 8u;
        candidate.classNumber = b.getObjType(DRW::AC1018);
        candidate.dataEndBit = b.getRawLong32();
        if (!b.isGood() || candidate.classNumber != raw.m_objectType
            || candidate.dataEndBit > total || candidate.dataEndBit < bit(b)) return false;
        if (!nonnullHandle(b, 0u, candidate.handle) || candidate.handle != raw.m_handle
            || bit(b) > candidate.dataEndBit) return false;
        std::uint16_t eed = 0u;
        std::int32_t reactors = 0;
        if (!shortValue(b, candidate.dataEndBit, eed) || eed != 0u
            || !longValue(b, candidate.dataEndBit, reactors) || reactors != 0
            || b.getBit() != 1u || !b.isGood() || bit(b) > candidate.dataEndBit) return false;
        std::int32_t rows = 0, columns = 0, count = 0;
        if (!longValue(b, candidate.dataEndBit, rows) || rows != 3
            || !longValue(b, candidate.dataEndBit, columns) || columns != 3
            || !longValue(b, candidate.dataEndBit, count) || count != 9) return false;
        for (auto& cell : candidate.cells) {
            cell.startBit = bit(b);
            std::int32_t contents = 0;
            if (!longValue(b, candidate.dataEndBit, cell.flags) || cell.flags != 0) return false;
            cell.widthWithGap = b.getBitDouble();
            if (!b.isGood() || bit(b) > candidate.dataEndBit
                || !std::isfinite(cell.widthWithGap) || cell.widthWithGap != 0.0) return false;
            cell.heightWithGap = b.getBitDouble();
            if (!b.isGood() || bit(b) > candidate.dataEndBit
                || !std::isfinite(cell.heightWithGap) || cell.heightWithGap != 0.0
                || !longValue(b, candidate.dataEndBit, contents) || contents != 0) return false;
            cell.endBit = bit(b);
        }
        candidate.parentHandleStartBit = bit(b);
        if (candidate.parentHandleStartBit != candidate.dataEndBit
            || !nonnullHandle(b, 4u, candidate.parentHandle)
            || candidate.parentHandle == candidate.handle) return false;
        candidate.parentHandleEndBit = bit(b);
        const bool commonValidated = drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence);
        if (raw.m_commonHandleDataValidated != commonValidated) return false;
        if (commonValidated && (raw.m_commonLinkEvidence != DRW_DwgCommonLinkEvidence::Validated
            || raw.m_parentHandle != candidate.parentHandle || raw.m_numReactors != 0
            || !raw.m_reactorHandles.empty() || raw.m_xDictFlag != 1u
            || raw.m_xDictHandle != DRW::NoHandle)) return false;
        for (auto& cell : candidate.cells) {
            cell.handleStartBit = bit(b);
            if (!nullSoftPointer(b)) return false;
            cell.handleEndBit = bit(b);
            cell.unknownHandle = 0u;
        }
        candidate.handleEndBit = bit(b);
        if (candidate.handleEndBit > total || total - candidate.handleEndBit >= 8u) return false;
        while (bit(b) < total) if (b.getBit() != 0u || !b.isGood()) return false;
        result = candidate;
        return true;
    } catch (...) { result = Result{}; return false; }
}
}
