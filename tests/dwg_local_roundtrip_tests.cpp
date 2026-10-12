#include <cstdio>
#include <cstdint>
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "drw_entities.h"
#include "drw_acis.h"
#include "drw_header.h"
#include "dwg2dxf/dx_iface.h"
#include "libdwgr.h"
#include "libdxfrw.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwg_fixed_handles.h"

namespace {

struct Ac1024ClassesIntegrityReceipt {
    bool valid {false};
    bool usedExtendedStringSize {false};
    std::uint32_t classDataSize {0};
    std::uint32_t bitSize {0};
    std::uint16_t maxClassNumber {0};
    std::uint64_t stringBitSize {0};
    std::size_t declaredCrcOffset {0};
    std::size_t footerCrcOffset {0};
    std::uint16_t storedCrc {0};
    std::uint16_t calculatedCrc {0};
};

std::uint16_t readLe16(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset])
        | static_cast<std::uint16_t>(bytes[offset + 1]) << 8;
}

std::uint32_t readLe32(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset])
        | static_cast<std::uint32_t>(bytes[offset + 1]) << 8
        | static_cast<std::uint32_t>(bytes[offset + 2]) << 16
        | static_cast<std::uint32_t>(bytes[offset + 3]) << 24;
}

class IndependentBitReader {
public:
    IndependentBitReader(const std::vector<std::uint8_t>& bytes,
                         std::uint64_t bitOffset)
        : m_bytes{bytes}, m_bitOffset{bitOffset} {}

    bool readBit(std::uint8_t& value) {
        if (m_bitOffset >= static_cast<std::uint64_t>(m_bytes.size()) * 8u)
            return false;
        value = static_cast<std::uint8_t>(
            (m_bytes[static_cast<std::size_t>(m_bitOffset >> 3)]
             >> (7u - static_cast<unsigned>(m_bitOffset & 7u))) & 1u);
        ++m_bitOffset;
        return true;
    }

    bool readRawByte(std::uint8_t& value) {
        value = 0;
        for (int i = 0; i < 8; ++i) {
            std::uint8_t bit = 0;
            if (!readBit(bit))
                return false;
            value = static_cast<std::uint8_t>((value << 1) | bit);
        }
        return true;
    }

    bool readRawShort(std::uint16_t& value) {
        std::uint8_t low = 0;
        std::uint8_t high = 0;
        if (!readRawByte(low) || !readRawByte(high))
            return false;
        value = static_cast<std::uint16_t>(low)
            | static_cast<std::uint16_t>(high) << 8;
        return true;
    }

    bool readBitShort(std::uint16_t& value) {
        std::uint8_t highCode = 0;
        std::uint8_t lowCode = 0;
        if (!readBit(highCode) || !readBit(lowCode))
            return false;
        const std::uint8_t code = static_cast<std::uint8_t>(
            (highCode << 1) | lowCode);
        if (code == 2) {
            value = 0;
            return true;
        }
        if (code == 3) {
            value = 256;
            return true;
        }
        if (code == 1) {
            std::uint8_t byte = 0;
            if (!readRawByte(byte))
                return false;
            value = byte;
            return true;
        }
        return readRawShort(value);
    }

private:
    const std::vector<std::uint8_t>& m_bytes;
    std::uint64_t m_bitOffset {0};
};

// ODA 2.14.1's reflected 0xA001 CRC, implemented bit-by-bit so this test does
// not agree with production merely because it called dwgBuffer::crc8 or
// dwgBufferW::crc16.  CLASSES starts the range immediately after its opening
// sentinel, uses seed 0xC0C1, and stops immediately before the stored RS.
std::uint16_t independentOdaCrc16(
        const std::vector<std::uint8_t>& bytes, std::size_t begin,
        std::size_t end) {
    std::uint16_t crc = 0xC0C1u;
    for (std::size_t i = begin; i < end; ++i) {
        crc = static_cast<std::uint16_t>(crc ^ bytes[i]);
        for (int bit = 0; bit < 8; ++bit) {
            crc = static_cast<std::uint16_t>(
                (crc >> 1) ^ ((crc & 1u) != 0 ? 0xA001u : 0u));
        }
    }
    return crc;
}

bool readRawShortAtBit(const std::vector<std::uint8_t>& bytes,
                       std::uint64_t bitOffset, std::uint16_t& value) {
    IndependentBitReader reader(bytes, bitOffset);
    return reader.readRawShort(value);
}

Ac1024ClassesIntegrityReceipt inspectAc1024ClassesIntegrity(
        const std::filesystem::path& path) {
    // Literal constants and the 0x7400 page size come from ODA 5.8/10.2.  The
    // local AC1024 writer emits stored R2004 pages, so reconstruct the decoded
    // section by skipping each page's independent 32-byte physical header.
    constexpr std::array<std::uint8_t, 16> beginSentinel {
        0x8D, 0xA1, 0xC4, 0xB8, 0xC4, 0xA9, 0xF8, 0xC5,
        0xC0, 0xDC, 0xF4, 0x5F, 0xE7, 0xCF, 0xB6, 0x8A};
    constexpr std::array<std::uint8_t, 16> endSentinel {
        0x72, 0x5E, 0x3B, 0x47, 0x3B, 0x56, 0x07, 0x3A,
        0x3F, 0x23, 0x0B, 0xA0, 0x18, 0x30, 0x49, 0x75};
    constexpr std::size_t pageDataSize = 0x7400u;
    constexpr std::size_t pageHeaderSize = 32u;
    constexpr std::size_t unknownTailSize = 8u;

    Ac1024ClassesIntegrityReceipt receipt;
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return receipt;
    const std::vector<std::uint8_t> file {
        std::istreambuf_iterator<char>{stream},
        std::istreambuf_iterator<char>{}};
    const auto begin = std::search(file.cbegin(), file.cend(),
                                   beginSentinel.cbegin(), beginSentinel.cend());
    if (begin == file.cend())
        return receipt;
    const std::size_t physicalStart = static_cast<std::size_t>(
        std::distance(file.cbegin(), begin));
    if (physicalStart > file.size()
        || file.size() - physicalStart < beginSentinel.size() + 4u)
        return receipt;
    receipt.classDataSize = readLe32(file, physicalStart + 16u);
    const std::uint64_t logicalSize64 =
        static_cast<std::uint64_t>(receipt.classDataSize) + 46u;
    if (logicalSize64 > (std::numeric_limits<std::size_t>::max)())
        return receipt;
    const std::size_t logicalSize = static_cast<std::size_t>(logicalSize64);
    std::vector<std::uint8_t> section;
    section.reserve(logicalSize);
    std::size_t physical = physicalStart;
    while (section.size() < logicalSize) {
        const std::size_t count = std::min(
            pageDataSize, logicalSize - section.size());
        if (physical > file.size() || count > file.size() - physical)
            return receipt;
        section.insert(section.end(), file.cbegin() + physical,
                       file.cbegin() + physical + count);
        physical += count;
        if (section.size() < logicalSize) {
            if (physical > file.size()
                || pageHeaderSize > file.size() - physical)
                return receipt;
            physical += pageHeaderSize;
        }
    }
    if (!std::equal(beginSentinel.cbegin(), beginSentinel.cend(),
                    section.cbegin())
        || section.size() < endSentinel.size()
        || !std::equal(endSentinel.cbegin(), endSentinel.cend(),
                       section.cend() - endSentinel.size()))
        return receipt;

    receipt.bitSize = readLe32(section, 20u);
    IndependentBitReader classHeader(section, 24u * 8u);
    if (!classHeader.readBitShort(receipt.maxClassNumber))
        return receipt;

    // AC1024 without the optional high-size RL uses the documented 159-bit
    // fixed prefix. `footerEndBit` addresses the one-bit string-present flag.
    const std::uint64_t footerEndBit =
        static_cast<std::uint64_t>(receipt.bitSize) + 159u;
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(section.size()) * 8u;
    if (footerEndBit >= totalBits || footerEndBit < 16u)
        return receipt;
    IndependentBitReader endBitReader(section, footerEndBit);
    std::uint8_t endBit = 0;
    if (!endBitReader.readBit(endBit) || endBit != 1u)
        return receipt;
    std::uint16_t lowSize = 0;
    if (!readRawShortAtBit(section, footerEndBit - 16u, lowSize))
        return receipt;
    receipt.usedExtendedStringSize = (lowSize & 0x8000u) != 0;
    receipt.stringBitSize = lowSize & 0x7FFFu;
    if (receipt.usedExtendedStringSize) {
        if (footerEndBit < 32u)
            return receipt;
        std::uint16_t highSize = 0;
        if (!readRawShortAtBit(section, footerEndBit - 32u, highSize))
            return receipt;
        receipt.stringBitSize |= static_cast<std::uint64_t>(highSize) << 15;
    }

    receipt.declaredCrcOffset =
        20u + static_cast<std::size_t>(receipt.classDataSize);
    receipt.footerCrcOffset = static_cast<std::size_t>(
        (footerEndBit + 1u + 7u) / 8u);
    if (receipt.declaredCrcOffset != receipt.footerCrcOffset
        || receipt.footerCrcOffset > section.size()
        || section.size() - receipt.footerCrcOffset
               < 2u + unknownTailSize + endSentinel.size())
        return receipt;
    receipt.storedCrc = readLe16(section, receipt.footerCrcOffset);
    receipt.calculatedCrc = independentOdaCrc16(
        section, beginSentinel.size(), receipt.footerCrcOffset);
    receipt.valid = true;
    return receipt;
}

class LocalDwgInterface final : public dx_iface {
public:
    explicit LocalDwgInterface(dwgRW* writer = nullptr,
                               DRW::Version expectedVersion = DRW::UNKNOWNV)
        : writer_(writer), expectedVersion_(expectedVersion) {
        cData = &data_;
        currentBlock = data_.mBlock;
    }

    void writeHeader(DRW_Header& data) override {
        // Keep the locally generated case independent of external files while
        // still exercising the production header encoder.
        data.vars.clear();
    }

    void writeDwgClasses() override {
        if (writer_ != nullptr) {
            registeredGroup_ = writer_->registerDwgNamedObjectDictionaryEntry(
                "LOCAL_GROUP", 0xA600u);
            registeredDictionary_ = writer_->registerDwgNamedObjectDictionaryEntry(
                "LOCAL_DICTIONARY", 0xA601u);
            registeredPlotSettings_ = writer_->registerPlotSettingsObjectClass(
                0xA603u);
            DRW_MLeaderStyle mleaderRegistration;
            mleaderRegistration.handle = 0xA900u;
            registeredMLeaderStyle_ = writer_->registerMLeaderStyleObjectClass(
                &mleaderRegistration);
            DRW_DictionaryVar dictionaryVarRegistration;
            dictionaryVarRegistration.handle = 0xB000u;
            registeredDictionaryVar_ = writer_->registerDictionaryVarObjectClass(
                &dictionaryVarRegistration);
            DRW_DictionaryWithDefault dictionaryWithDefaultRegistration;
            dictionaryWithDefaultRegistration.handle = 0xB100u;
            registeredDictionaryWithDefault_ =
                writer_->registerDictionaryWithDefaultObjectClass(
                    &dictionaryWithDefaultRegistration);
            DRW_SortEntsTable sortEntsRegistration;
            sortEntsRegistration.handle = 0xB200u;
            registeredSortEntsTable_ = writer_->registerSortEntsTableObjectClass(
                &sortEntsRegistration);
            DRW_FieldList fieldListRegistration;
            fieldListRegistration.handle = 0xB300u;
            registeredFieldList_ = writer_->registerFieldListObjectClass(
                &fieldListRegistration);
            DRW_Field fieldRegistration;
            fieldRegistration.handle = 0xB400u;
            registeredField_ = writer_->registerFieldObjectClass(
                &fieldRegistration);
            DRW_RasterVariables rasterRegistration;
            rasterRegistration.handle = 0xB500u;
            registeredRasterVariables_ = writer_->registerRasterVariablesObjectClass(
                &rasterRegistration);
            DRW_WipeoutVariables wipeoutRegistration;
            wipeoutRegistration.handle = 0xB600u;
            registeredWipeoutVariables_ = writer_->registerWipeoutVariablesObjectClass(
                &wipeoutRegistration);
            DRW_VisualStyle visualStyleRegistration;
            visualStyleRegistration.handle = 0xC000u;
            registeredVisualStyle_ = writer_->registerVisualStyleObjectClass(
                &visualStyleRegistration);
            DRW_RenderSettings renderSettingsRegistration;
            renderSettingsRegistration.handle = 0xC100u;
            registeredRenderSettings_ = writer_->registerRenderSettingsObjectClass(
                &renderSettingsRegistration);
            DRW_RenderSettings renderEnvironmentRegistration;
            renderEnvironmentRegistration.handle = 0xC200u;
            renderEnvironmentRegistration.m_kind = DRW_RenderSettings::Environment;
            registeredRenderEnvironment_ = writer_->registerRenderSettingsObjectClass(
                &renderEnvironmentRegistration);
            DRW_RenderSettings renderGlobalRegistration;
            renderGlobalRegistration.handle = 0xC300u;
            renderGlobalRegistration.m_kind = DRW_RenderSettings::Global;
            registeredRenderGlobal_ = writer_->registerRenderSettingsObjectClass(
                &renderGlobalRegistration);
            DRW_RenderSettings renderEntryRegistration;
            renderEntryRegistration.handle = 0xC400u;
            renderEntryRegistration.m_kind = DRW_RenderSettings::Entry;
            registeredRenderEntry_ = writer_->registerRenderSettingsObjectClass(
                &renderEntryRegistration);
            DRW_RenderSettings renderRapidRegistration;
            renderRapidRegistration.handle = 0xC600u;
            renderRapidRegistration.m_kind = DRW_RenderSettings::RapidRT;
            registeredRenderRapid_ = writer_->registerRenderSettingsObjectClass(
                &renderRapidRegistration);
            DRW_RenderSettings renderMentalRegistration;
            renderMentalRegistration.handle = 0xC700u;
            renderMentalRegistration.m_kind = DRW_RenderSettings::MentalRay;
            registeredRenderMental_ = writer_->registerRenderSettingsObjectClass(
                &renderMentalRegistration);
            DRW_Material materialRegistration;
            materialRegistration.handle = 0xC800u;
            registeredMaterial_ = writer_->registerMaterialObjectClass(
                &materialRegistration);
            if (writer_->getVersion() >= DRW::AC1018) {
                DRW_DbColor dbColorRegistration;
                dbColorRegistration.handle = 0xC900u;
                registeredDbColor_ = writer_->registerDbColorObjectClass(
                    &dbColorRegistration);
            }
            DRW_LightList lightListRegistration;
            lightListRegistration.handle = 0xCA00u;
            registeredLightList_ = writer_->registerLightListObjectClass(
                &lightListRegistration);
            DRW_Scale scaleRegistration;
            scaleRegistration.handle = 0xCB00u;
            registeredScale_ = writer_->registerScaleObjectClass(&scaleRegistration);
            DRW_IDBuffer idBufferRegistration;
            idBufferRegistration.handle = 0xCC00u;
            registeredIDBuffer_ = writer_->registerIDBufferObjectClass(
                &idBufferRegistration);
            DRW_LayerIndex layerIndexRegistration;
            layerIndexRegistration.handle = 0xCD00u;
            registeredLayerIndex_ = writer_->registerLayerIndexObjectClass(
                &layerIndexRegistration);
            DRW_SpatialIndex spatialIndexRegistration;
            spatialIndexRegistration.handle = 0xCE00u;
            registeredSpatialIndex_ = writer_->registerSpatialIndexObjectClass(
                &spatialIndexRegistration);
            if (writer_->getVersion() <= DRW::AC1021) {
                DRW_TableStyle tableStyleRegistration;
                tableStyleRegistration.handle = 0xCF00u;
                registeredTableStyle_ = writer_->registerTableStyleObjectClass(
                    &tableStyleRegistration);
            }
            DRW_SpatialFilter spatialFilterRegistration;
            spatialFilterRegistration.handle = 0xD000u;
            registeredSpatialFilter_ = writer_->registerSpatialFilterObjectClass(
                &spatialFilterRegistration);
            DRW_GeoData geoDataRegistration;
            geoDataRegistration.handle = 0xD100u;
            registeredGeoData_ = writer_->registerGeoDataObjectClass(
                &geoDataRegistration);
            DRW_GeoData geoDataV2Registration;
            geoDataV2Registration.handle = 0xD200u;
            registeredGeoDataV2_ = writer_->registerGeoDataObjectClass(
                &geoDataV2Registration);
            DRW_UnderlayDefinition pdfUnderlayRegistration;
            pdfUnderlayRegistration.handle = 0xD300u;
            pdfUnderlayRegistration.kind = DRW_UnderlayDefinition::PDF;
            registeredPdfUnderlay_ =
                writer_->registerUnderlayDefinitionObjectClass(
                    &pdfUnderlayRegistration);
            DRW_UnderlayDefinition dgnUnderlayRegistration;
            dgnUnderlayRegistration.handle = 0xD400u;
            dgnUnderlayRegistration.kind = DRW_UnderlayDefinition::DGN;
            registeredDgnUnderlay_ =
                writer_->registerUnderlayDefinitionObjectClass(
                    &dgnUnderlayRegistration);
            DRW_UnderlayDefinition dwfUnderlayRegistration;
            dwfUnderlayRegistration.handle = 0xD500u;
            dwfUnderlayRegistration.kind = DRW_UnderlayDefinition::DWF;
            registeredDwfUnderlay_ =
                writer_->registerUnderlayDefinitionObjectClass(
                    &dwfUnderlayRegistration);
            if (writer_->getVersion() >= DRW::AC1021) {
                DRW_PlaneSurface planeSurfaceRegistration;
                DRW_ExtrudedSurface extrudedSurfaceRegistration;
                DRW_RevolvedSurface revolvedSurfaceRegistration;
                DRW_SweptSurface sweptSurfaceRegistration;
                DRW_LoftedSurface loftedSurfaceRegistration;
                DRW_NurbsSurface nurbsSurfaceRegistration;
                planeSurfaceRegistration.handle = 0xFB00u;
                extrudedSurfaceRegistration.handle = 0xFB01u;
                revolvedSurfaceRegistration.handle = 0xFB02u;
                sweptSurfaceRegistration.handle = 0xFB03u;
                loftedSurfaceRegistration.handle = 0xFB04u;
                nurbsSurfaceRegistration.handle = 0xFB05u;
                registeredSurfaceClasses_ =
                    writer_->registerSurfaceEntityClass(
                        &planeSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        planeSurfaceRegistration.getDwgClassNum(),
                        planeSurfaceRegistration.handle)
                    && writer_->registerSurfaceEntityClass(
                        &extrudedSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        extrudedSurfaceRegistration.getDwgClassNum(),
                        extrudedSurfaceRegistration.handle)
                    && writer_->registerSurfaceEntityClass(
                        &revolvedSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        revolvedSurfaceRegistration.getDwgClassNum(),
                        revolvedSurfaceRegistration.handle)
                    && writer_->registerSurfaceEntityClass(
                        &sweptSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        sweptSurfaceRegistration.getDwgClassNum(),
                        sweptSurfaceRegistration.handle)
                    && writer_->registerSurfaceEntityClass(
                        &loftedSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        loftedSurfaceRegistration.getDwgClassNum(),
                        loftedSurfaceRegistration.handle)
                    && writer_->registerSurfaceEntityClass(
                        &nurbsSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        nurbsSurfaceRegistration.getDwgClassNum(),
                        nurbsSurfaceRegistration.handle);
            }
            DRW_PointCloudDef pointCloudRegistration;
            pointCloudRegistration.handle = 0xD600u;
            pointCloudRegistration.m_kind = DRW_PointCloudDef::Definition;
            registeredPointCloudDefinition_ =
                writer_->registerPointCloudDefObjectClass(
                    &pointCloudRegistration);
            DRW_PointCloudDef pointCloudExRegistration;
            pointCloudExRegistration.handle = 0xD601u;
            pointCloudExRegistration.m_kind = DRW_PointCloudDef::DefinitionEx;
            registeredPointCloudDefinitionEx_ =
                writer_->registerPointCloudDefObjectClass(
                    &pointCloudExRegistration);
            DRW_PointCloudDef pointCloudReactorRegistration;
            pointCloudReactorRegistration.handle = 0xD602u;
            pointCloudReactorRegistration.m_kind = DRW_PointCloudDef::Reactor;
            registeredPointCloudReactor_ =
                writer_->registerPointCloudDefObjectClass(
                    &pointCloudReactorRegistration);
            DRW_PointCloudDef pointCloudReactorExRegistration;
            pointCloudReactorExRegistration.handle = 0xD603u;
            pointCloudReactorExRegistration.m_kind = DRW_PointCloudDef::ReactorEx;
            registeredPointCloudReactorEx_ =
                writer_->registerPointCloudDefObjectClass(
                    &pointCloudReactorExRegistration);
            DRW_PointCloudColorMap colorMapRegistration;
            colorMapRegistration.handle = 0xD800u;
            registeredPointCloudColorMap_ =
                writer_->registerPointCloudColorMapObjectClass(
                    &colorMapRegistration);
            DRW_NavisworksModelDef navisworksRegistration;
            navisworksRegistration.handle = 0xD900u;
            registeredNavisworksModelDef_ =
                writer_->registerNavisworksModelDefObjectClass(
                    &navisworksRegistration);
            DRW_SunStudy sunStudyRegistration;
            sunStudyRegistration.handle = 0xDA00u;
            registeredSunStudy_ = writer_->registerSunStudyObjectClass(
                &sunStudyRegistration);
            DRW_MotionPath motionPathRegistration;
            motionPathRegistration.handle = 0xDB00u;
            registeredMotionPath_ = writer_->registerMotionPathObjectClass(
                &motionPathRegistration);
            DRW_CurvePath curvePathRegistration;
            curvePathRegistration.handle = 0xDC00u;
            registeredCurvePath_ = writer_->registerCurvePathObjectClass(
                &curvePathRegistration);
            DRW_PointPath pointPathRegistration;
            pointPathRegistration.handle = 0xDD00u;
            registeredPointPath_ = writer_->registerPointPathObjectClass(
                &pointPathRegistration);
            DRW_ObjectPtr objectPtrRegistration;
            objectPtrRegistration.handle = 0xDE00u;
            registeredObjectPtr_ = writer_->registerObjectPtrObjectClass(
                &objectPtrRegistration);
            DRW_PartialViewingIndex partialViewingIndexRegistration;
            partialViewingIndexRegistration.handle = 0xDF00u;
            registeredPartialViewingIndex_ =
                writer_->registerPartialViewingIndexObjectClass(
                    &partialViewingIndexRegistration);
            DRW_Background solidBackgroundRegistration;
            solidBackgroundRegistration.handle = 0xE000u;
            solidBackgroundRegistration.m_kind = DRW_Background::Solid;
            registeredSolidBackground_ = writer_->registerBackgroundObjectClass(
                &solidBackgroundRegistration);
            DRW_Background gradientBackgroundRegistration;
            gradientBackgroundRegistration.handle = 0xE100u;
            gradientBackgroundRegistration.m_kind = DRW_Background::Gradient;
            registeredGradientBackground_ =
                writer_->registerBackgroundObjectClass(
                    &gradientBackgroundRegistration);
            DRW_Background groundPlaneBackgroundRegistration;
            groundPlaneBackgroundRegistration.handle = 0xE200u;
            groundPlaneBackgroundRegistration.m_kind =
                DRW_Background::GroundPlane;
            registeredGroundPlaneBackground_ =
                writer_->registerBackgroundObjectClass(
                    &groundPlaneBackgroundRegistration);
            DRW_Background imageBackgroundRegistration;
            imageBackgroundRegistration.handle = 0xE300u;
            imageBackgroundRegistration.m_kind = DRW_Background::Image;
            registeredImageBackground_ =
                writer_->registerBackgroundObjectClass(
                    &imageBackgroundRegistration);
            DRW_Background iblBackgroundRegistration;
            iblBackgroundRegistration.handle = 0xE400u;
            iblBackgroundRegistration.m_kind = DRW_Background::Ibl;
            registeredIblBackground_ = writer_->registerBackgroundObjectClass(
                &iblBackgroundRegistration);
            DRW_Background skylightBackgroundRegistration;
            skylightBackgroundRegistration.handle = 0xE500u;
            skylightBackgroundRegistration.m_kind = DRW_Background::Skylight;
            registeredSkylightBackground_ =
                writer_->registerBackgroundObjectClass(
                    &skylightBackgroundRegistration);
            DRW_TvDeviceProperties tvDeviceRegistration;
            tvDeviceRegistration.handle = 0xEA00u;
            registeredTvDeviceProperties_ =
                writer_->registerTvDevicePropertiesObjectClass(
                    &tvDeviceRegistration);
            DRW_VxControl vxControlRegistration;
            vxControlRegistration.handle = 0xEB00u;
            registeredVxControl_ = writer_->registerVxControlObjectClass(
                &vxControlRegistration);
            DRW_VxTableRecord vxTableRecordRegistration;
            vxTableRecordRegistration.handle = 0xEC00u;
            registeredVxTableRecord_ =
                writer_->registerVxTableRecordObjectClass(
                    &vxTableRecordRegistration);
            // RTEXT and ARCALIGNEDTEXT are already part of the writer's
            // typed class manifest.  Stage their fixed entity handles with
            // the entity-instance ledger before CLASSES is serialized; raw
            // class registration here would create a second identity and
            // remap the wire class number.
            registeredRText_ = writer_->registerDwgEntityClassInstance(
                DRW_RText::kDwgClassNum, 0xED00u);
            registeredArcAlignedText_ =
                writer_->registerDwgEntityClassInstance(
                    DRW_ArcAlignedText::kDwgClassNum, 0xED01u);
            // HELIX is a typed custom entity (class 503) already present in
            // the writer's class manifest. Stage its fixed instance before
            // CLASSES so the entity object map and class identity agree.
            registeredHelix_ = writer_->registerDwgEntityClassInstance(
                DRW_Helix::kDwgClassNum, 0xEE00u);
            if (expectedVersion_ >= DRW::AC1018) {
                registeredCamera_ = writer_->registerDwgEntityClassInstance(
                    DRW_Camera::kDwgClassNum, 0xEF00u);
            }
            if (expectedVersion_ >= DRW::AC1021) {
                registeredLight_ = writer_->registerDwgEntityClassInstance(
                    DRW_Light::kDwgClassNum, 0xF600u);
            }
            if (expectedVersion_ >= DRW::AC1018) {
                registeredMesh_ = writer_->registerDwgEntityClassInstance(
                    DRW_Mesh::kDwgClassNum, 0xF700u);
            }
            if (expectedVersion_ >= DRW::AC1021) {
                DRW_DimensionAssociation dimAssocRegistration;
                dimAssocRegistration.handle = 0xF000u;
                registeredDimensionAssociation_ =
                    writer_->registerDimensionAssociationObjectClass(
                        &dimAssocRegistration);
                DRW_EvaluationGraph evaluationGraphRegistration;
                evaluationGraphRegistration.handle = 0xF100u;
                registeredEvaluationGraph_ =
                    writer_->registerEvaluationGraphObjectClass(
                        &evaluationGraphRegistration);
            }
            if (expectedVersion_ >= DRW::AC1021) {
                DRW_Section sectionManagerRegistration;
                sectionManagerRegistration.handle = 0xE700u;
                sectionManagerRegistration.m_kind = DRW_Section::Manager;
                registeredSectionManager_ = writer_->registerSectionObjectClass(
                    &sectionManagerRegistration);
                DRW_Section sectionSettingsRegistration;
                sectionSettingsRegistration.handle = 0xE800u;
                sectionSettingsRegistration.m_kind = DRW_Section::Settings;
                registeredSectionSettings_ = writer_->registerSectionObjectClass(
                    &sectionSettingsRegistration);
            }
        }
    }

    void writeBlocks() override {
        if (writer_ == nullptr)
            return;
        const std::uint32_t blockHandle = writer_->defineBlock(
            "LOCAL_BLOCK", DRW_Coord(0.0, 0.0, 0.0));
        if (blockHandle == 0 || !writer_->beginBlockContent(blockHandle))
            return;
        DRW_Line blockLine;
        blockLine.basePoint = DRW_Coord(80.0, 81.0, 0.0);
        blockLine.secPoint = DRW_Coord(82.0, 83.0, 0.0);
        wroteBlock_ = writer_->writeLine(&blockLine) && blockLine.handle != 0;
        DRW_Polyline blockPolyline;
        blockPolyline.vertexcount = 2;
        blockPolyline.addVertex(DRW_Vertex(86.0, 87.0, 0.0, 0.0));
        blockPolyline.addVertex(DRW_Vertex(88.0, 89.0, 0.0, 0.0));
        wroteBlockPolyline_ = writer_->writePolyline(&blockPolyline)
            && blockPolyline.handle != 0;
        wroteBlockContent_ = writer_->endBlockContent();
    }
    void writeBlockRecords() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {
        if (writer_ == nullptr || modelSpaceLineHandle_ == 0)
            return;

        // Exercise the full production OBJECT stream.  The custom dictionary
        // is a named-object child and owns the remaining carriers; this keeps
        // the object graph explicit instead of relying on an untracked frame.
        DRW_Dictionary dictionary;
        dictionary.handle = 0xA601u;
        dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        dictionary.cloning = 1;
        dictionary.hardOwner = 1;
        dictionary.m_entries = {
            {"LOCAL_XRECORD", 0xA602u},
            {"LOCAL_PLOTSETTINGS", 0xA603u},
            {"LOCAL_LAYOUT", 0xA700u},
            {"LOCAL_MLINESTYLE", 0xA800u},
            {"LOCAL_MLEADERSTYLE", 0xA900u},
            {"LOCAL_DICTIONARYVAR", 0xB000u},
            {"LOCAL_DICTIONARYWDFLT", 0xB100u},
            {"LOCAL_SORTENTSTABLE", 0xB200u},
            {"LOCAL_FIELDLIST", 0xB300u},
            {"LOCAL_FIELD", 0xB400u},
            {"LOCAL_RASTERVARIABLES", 0xB500u},
            {"LOCAL_WIPEOUTVARIABLES", 0xB600u},
            {"LOCAL_VISUALSTYLE", 0xC000u},
            {"LOCAL_RENDERSETTINGS", 0xC100u},
            {"LOCAL_RENDER_ENVIRONMENT", 0xC200u},
            {"LOCAL_RENDER_GLOBAL", 0xC300u},
            {"LOCAL_RENDER_ENTRY", 0xC400u},
            {"LOCAL_RENDER_RAPIDRT", 0xC600u},
            {"LOCAL_RENDER_MENTALRAY", 0xC700u},
            {"LOCAL_MATERIAL", 0xC800u},
            {"LOCAL_DBCOLOR", 0xC900u},
            {"LOCAL_LIGHTLIST", 0xCA00u},
            {"LOCAL_SCALE", 0xCB00u},
            {"LOCAL_IDBUFFER", 0xCC00u},
            {"LOCAL_LAYER_INDEX", 0xCD00u},
            {"LOCAL_SPATIAL_INDEX", 0xCE00u},
            {"LOCAL_TABLESTYLE", 0xCF00u},
            {"LOCAL_SPATIAL_FILTER", 0xD000u},
            {"LOCAL_GEODATA", 0xD100u},
            {"LOCAL_GEODATA_V2", 0xD200u},
            {"LOCAL_PDFDEFINITION", 0xD300u},
            {"LOCAL_DGNDEFINITION", 0xD400u},
            {"LOCAL_DWFDEFINITION", 0xD500u},
            {"LOCAL_POINTCLOUDDEFINITION", 0xD600u},
            {"LOCAL_POINTCLOUDDEFINITIONEX", 0xD601u},
            {"LOCAL_POINTCLOUDCOLORMAP", 0xD800u},
            {"LOCAL_NAVISWORKSMODELDEF", 0xD900u},
            {"LOCAL_SUNSTUDY", 0xDA00u},
            {"LOCAL_MOTIONPATH", 0xDB00u},
            {"LOCAL_CURVEPATH", 0xDC00u},
            {"LOCAL_POINTPATH", 0xDD00u},
            {"LOCAL_OBJECT_PTR", 0xDE00u},
            {"LOCAL_PARTIAL_VIEWING_INDEX", 0xDF00u},
            {"LOCAL_SOLID_BACKGROUND", 0xE000u},
            {"LOCAL_GRADIENT_BACKGROUND", 0xE100u},
            {"LOCAL_GROUNDPLANE_BACKGROUND", 0xE200u},
            {"LOCAL_IMAGE_BACKGROUND", 0xE300u},
            {"LOCAL_IBL_BACKGROUND", 0xE400u},
            {"LOCAL_SKYLIGHT_BACKGROUND", 0xE500u},
            {"LOCAL_TVDEVICEPROPERTIES", 0xEA00u},
            {"LOCAL_VXCONTROL", 0xEB00u},
            {"LOCAL_VXTABLERECORD", 0xEC00u},
        };
        if (expectedVersion_ >= DRW::AC1021) {
            dictionary.m_entries.push_back(
                DRW_Dictionary::Entry{"LOCAL_SECTION_MANAGER", 0xE700u});
            dictionary.m_entries.push_back(
                DRW_Dictionary::Entry{"LOCAL_SECTION_SETTINGS", 0xE800u});
        }
        dictionary.m_entries.push_back(
            DRW_Dictionary::Entry{"LOCAL_NULL_ENTRY", 0u});
        wroteDictionary_ = registeredDictionary_
            && writer_->writeDictionary(&dictionary)
            && dictionary.handle != 0;

        DRW_XRecord xrecord;
        xrecord.handle = 0xA602u;
        xrecord.parentHandle = dictionary.handle;
        xrecord.m_values.emplace_back(40, 1.25);
        xrecord.m_values.emplace_back(1, UTF8STRING("LOCAL_XRECORD"));
        xrecord.m_values.emplace_back(
            310, std::vector<std::uint8_t>{0x01, 0x02, 0x03});
        xrecord.m_handleValues.emplace_back(0, modelSpaceLineHandle_);
        wroteXRecord_ = writer_->writeXRecord(&xrecord)
            && xrecord.handle != 0;

        DRW_PlotSettings plotSettings;
        plotSettings.handle = 0xA603u;
        plotSettings.parentHandle = dictionary.handle;
        plotSettings.pageSetupName = "LOCAL_PAGE";
        plotSettings.printerConfig = "LOCAL_PRINTER";
        plotSettings.plotLayoutFlags = 1;
        plotSettings.marginLeft = 1.0;
        plotSettings.marginBottom = 2.0;
        plotSettings.marginRight = 3.0;
        plotSettings.marginTop = 4.0;
        plotSettings.paperWidth = 210.0;
        plotSettings.paperHeight = 297.0;
        plotSettings.paperSize = "A4";
        plotSettings.plotOriginX = 0.0;
        plotSettings.plotOriginY = 0.0;
        plotSettings.paperUnits = 1;
        plotSettings.plotRotation = 0;
        plotSettings.plotType = 0;
        plotSettings.windowMinX = -10.0;
        plotSettings.windowMinY = -20.0;
        plotSettings.windowMaxX = 10.0;
        plotSettings.windowMaxY = 20.0;
        plotSettings.plotViewName = "LOCAL_VIEW";
        plotSettings.realWorldUnits = 1.0;
        plotSettings.drawingUnits = 1.0;
        plotSettings.currentStyleSheet = "LOCAL_STYLE";
        plotSettings.scaleType = 1;
        plotSettings.scaleFactor = 1.0;
        plotSettings.shadePlotMode = 1;
        plotSettings.shadePlotResLevel = 2;
        plotSettings.shadePlotCustomDPI = 300;
        wrotePlotSettings_ = registeredPlotSettings_
            && writer_->writePlotSettings(&plotSettings)
            && plotSettings.handle != 0;

        if (expectedVersion_ >= DRW::AC1021) {
            DRW_DimensionAssociation dimAssoc;
            dimAssoc.handle = 0xF000u;
            dimAssoc.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
            dimAssoc.m_dimensionHandle = modelSpaceLineHandle_;
            dimAssoc.m_associativityFlags = 1;
            dimAssoc.m_isTransSpace = false;
            dimAssoc.m_rotatedDimensionType = 2;
            dimAssoc.m_osnapRefs.push_back({"AcDbLine", 0,
                                             modelSpaceLineHandle_});
            wroteDimensionAssociation_ = registeredDimensionAssociation_
                && writer_->writeDimensionAssociation(&dimAssoc)
                && dimAssoc.handle == 0xF000u;

            DRW_DimensionAssociation invalidDimAssoc = dimAssoc;
            invalidDimAssoc.handle = 0xF001u;
            invalidDimAssoc.m_associativityFlags = 2;
            invalidDimAssoc.m_osnapRefs.clear();
            rejectedMalformedDimensionAssociation_ =
                !writer_->writeDimensionAssociation(&invalidDimAssoc)
                && invalidDimAssoc.handle == 0xF001u;

            DRW_EvaluationGraph evaluationGraph;
            evaluationGraph.handle = 0xF100u;
            evaluationGraph.parentHandle =
                DRW::DwgNamedObjectsDictionaryHandle;
            evaluationGraph.m_value96 = 96;
            evaluationGraph.m_value97 = 97;
            DRW_EvaluationGraphNode node;
            node.m_index = 1;
            node.m_flags = 2;
            node.m_nextNodeIndex = 0;
            node.m_expressionHandle = modelSpaceLineHandle_;
            node.m_data1 = 11;
            node.m_data2 = 12;
            node.m_data3 = 13;
            node.m_data4 = 14;
            evaluationGraph.m_nodes.push_back(node);
            DRW_EvaluationGraphEdge edge;
            edge.m_value92 = 92;
            edge.m_value93 = 93;
            edge.m_value94 = 94;
            edge.m_value91a = 91;
            edge.m_value91b = 910;
            edge.m_value92a = 921;
            edge.m_value92b = 922;
            edge.m_value92c = 923;
            edge.m_value92d = 924;
            edge.m_value92e = 925;
            evaluationGraph.m_edges.push_back(edge);
            wroteEvaluationGraph_ = registeredEvaluationGraph_
                && writer_->writeEvaluationGraph(&evaluationGraph)
                && evaluationGraph.handle == 0xF100u;

            DRW_EvaluationGraph invalidEvaluationGraph = evaluationGraph;
            invalidEvaluationGraph.handle = 0xF101u;
            invalidEvaluationGraph.reactorHandles.resize(1000001u);
            rejectedMalformedEvaluationGraph_ =
                !writer_->writeEvaluationGraph(&invalidEvaluationGraph)
                && invalidEvaluationGraph.handle == 0xF101u;
        }

        DRW_BlockRepresentationData blockRepresentation;
        blockRepresentation.handle = 0xF200u;
        blockRepresentation.parentHandle =
            DRW::DwgNamedObjectsDictionaryHandle;
        blockRepresentation.m_flag = 7;
        blockRepresentation.m_blockHandle = modelSpaceLineHandle_;
        wroteBlockRepresentationData_ =
            writer_->writeBlockRepresentationData(&blockRepresentation)
            && blockRepresentation.handle == 0xF200u;

        DRW_BlockRepresentationData invalidBlockRepresentation =
            blockRepresentation;
        invalidBlockRepresentation.handle = 0xF201u;
        invalidBlockRepresentation.reactorHandles.resize(1000001u);
        rejectedMalformedBlockRepresentationData_ =
            !writer_->writeBlockRepresentationData(&invalidBlockRepresentation)
            && invalidBlockRepresentation.handle == 0xF201u;

        DRW_Layout layout;
        layout.handle = 0xA700u;
        layout.parentHandle = dictionary.handle;
        layout.pageSetupName = "LOCAL_LAYOUT_PAGE";
        layout.printerConfig = "LOCAL_LAYOUT_PRINTER";
        layout.paperSize = "A4";
        layout.marginLeft = 1.0;
        layout.marginBottom = 2.0;
        layout.marginRight = 3.0;
        layout.marginTop = 4.0;
        layout.paperWidth = 210.0;
        layout.paperHeight = 297.0;
        layout.name = "LOCAL_LAYOUT";
        layout.tabOrder = 1;
        layout.layoutFlags = 1;
        layout.ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
        layout.ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
        layout.extMax = DRW_Coord(100.0, 100.0, 0.0);
        layout.plotViewHandle.ref = 0xA710u;
        layout.shadePlotHandle.ref = 0xA711u;
        layout.paperSpaceBlockRecordHandle.ref = 0xA712u;
        layout.lastActiveViewportHandle.ref = 0xA713u;
        layout.baseUcsHandle.ref = 0xA714u;
        layout.namedUcsHandle.ref = 0xA715u;
        layout.viewportCount = 1;
        layout.viewportHandles = {0xA716u};
        wroteLayout_ = writer_->writeLayout(&layout)
            && layout.handle != 0;

        DRW_MLineStyle mlineStyle;
        mlineStyle.handle = 0xA800u;
        mlineStyle.parentHandle = dictionary.handle;
        mlineStyle.name = "LOCAL_MLINESTYLE";
        mlineStyle.description = "LOCAL_MLINESTYLE_DESC";
        mlineStyle.startAngle = 0.0;
        mlineStyle.endAngle = 1.5707963267948966;
        DRW_MLineElement mlineElement;
        mlineElement.offset = 0.5;
        mlineElement.color = 256;
        mlineElement.color24 = -1;
        mlineElement.linetypeIndex = 0;
        mlineStyle.elements.push_back(mlineElement);
        wroteMLineStyle_ = writer_->writeMLineStyle(&mlineStyle)
            && mlineStyle.handle != 0;

        DRW_MLeaderStyle mleaderStyle;
        mleaderStyle.handle = 0xA900u;
        mleaderStyle.parentHandle = dictionary.handle;
        mleaderStyle.name = "LOCAL_MLEADERSTYLE";
        mleaderStyle.description = "LOCAL_MLEADERSTYLE_DESC";
        mleaderStyle.contentType = 2;
        mleaderStyle.leaderType = 1;
        mleaderStyle.landingGap = 0.25;
        mleaderStyle.textDefault = "LOCAL_MLEADER_TEXT";
        mleaderStyle.textHeight = 2.5;
        mleaderStyle.scaleFactor = 1.0;
        wroteMLeaderStyle_ = registeredMLeaderStyle_
            && writer_->writeMLeaderStyle(&mleaderStyle)
            && mleaderStyle.handle != 0;

        DRW_DictionaryVar dictionaryVar;
        dictionaryVar.handle = 0xB000u;
        dictionaryVar.parentHandle = dictionary.handle;
        dictionaryVar.name = "LOCAL_DICTIONARYVAR";
        dictionaryVar.m_schema = 7;
        dictionaryVar.m_value = "LOCAL_DICTIONARYVAR_VALUE";
        wroteDictionaryVar_ = registeredDictionaryVar_
            && writer_->writeDictionaryVar(&dictionaryVar)
            && dictionaryVar.handle != 0;

        DRW_DictionaryWithDefault dictionaryWithDefault;
        dictionaryWithDefault.handle = 0xB100u;
        dictionaryWithDefault.parentHandle = dictionary.handle;
        dictionaryWithDefault.cloning = 1;
        dictionaryWithDefault.hardOwner = 1;
        dictionaryWithDefault.m_entries = {
            {"LOCAL_DEFAULT", 0xB000u}, {"LOCAL_NULL_DEFAULT_MEMBER", 0u}};
        dictionaryWithDefault.m_defaultEntryHandle = 0xB000u;
        wroteDictionaryWithDefault_ = registeredDictionaryWithDefault_
            && writer_->writeDictionaryWithDefault(&dictionaryWithDefault)
            && dictionaryWithDefault.handle != 0;

        DRW_SortEntsTable sortEnts;
        sortEnts.handle = 0xB200u;
        sortEnts.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
        sortEnts.m_blockOwnerHandle = DRW::DwgModelSpaceBlockRecordHandle;
        sortEnts.m_entityHandles = {modelSpaceLineHandle_};
        sortEnts.m_sortHandles = {modelSpaceLineHandle_};
        wroteSortEntsTable_ = registeredSortEntsTable_
            && writer_->writeSortEntsTable(&sortEnts)
            && sortEnts.handle != 0;

        DRW_FieldList fieldList;
        fieldList.handle = 0xB300u;
        fieldList.parentHandle = dictionary.handle;
        fieldList.m_unknown = 0;
        fieldList.m_fieldHandles = {0xB400u};
        wroteFieldList_ = registeredFieldList_
            && writer_->writeFieldList(&fieldList)
            && fieldList.handle != 0;

        DRW_Field field;
        field.handle = 0xB400u;
        field.parentHandle = dictionary.handle;
        field.m_evaluatorId = "ACAD";
        field.m_fieldCode = "LOCAL_FIELD_CODE";
        field.m_value.m_dataType = 1;
        field.m_value.m_value = DRW_Variant(0, 42);
        field.m_value.m_unitType = 12;
        field.m_valueString = "42";
        field.m_valueStringLength = 2;
        wroteField_ = registeredField_ && writer_->writeField(&field)
            && field.handle != 0;

        DRW_RasterVariables rasterVariables;
        rasterVariables.handle = 0xB500u;
        rasterVariables.parentHandle = dictionary.handle;
        rasterVariables.m_classVersion = 1;
        rasterVariables.m_imageFrame = 1;
        rasterVariables.m_imageQuality = 2;
        rasterVariables.m_units = 3;
        wroteRasterVariables_ = registeredRasterVariables_
            && writer_->writeRasterVariables(&rasterVariables)
            && rasterVariables.handle != 0;

        DRW_WipeoutVariables wipeoutVariables;
        wipeoutVariables.handle = 0xB600u;
        wipeoutVariables.parentHandle = dictionary.handle;
        wipeoutVariables.m_displayFrame = 1;
        wroteWipeoutVariables_ = registeredWipeoutVariables_
            && writer_->writeWipeoutVariables(&wipeoutVariables)
            && wipeoutVariables.handle != 0;

        DRW_VisualStyle visualStyle;
        visualStyle.handle = 0xC000u;
        visualStyle.parentHandle = dictionary.handle;
        visualStyle.desc = "LOCAL_VISUALSTYLE_DESC";
        visualStyle.type = 1;
        visualStyle.m_body.faceLightingModel = 2;
        visualStyle.m_body.faceOpacity = 0.75;
        visualStyle.m_body.faceSpecular = 0.25;
        visualStyle.m_body.edgeModel = 1;
        visualStyle.m_body.edgeStyle = 2;
        visualStyle.m_body.edgeOpacity = 0.5;
        visualStyle.m_body.edgeIsolines = 3;
        visualStyle.m_body.displaySettings = 4;
        visualStyle.m_body.displayBrightness = 0.8;
        visualStyle.m_body.extLightingModel = 1;
        visualStyle.m_body.hasR2013bExpansion = true;
        visualStyle.m_body.bProp1c = true;
        visualStyle.m_body.blProp25 = 5;
        visualStyle.m_body.bdProp26 = 1.25;
        visualStyle.m_body.bdProp27 = 2.5;
        visualStyle.m_body.blProp28 = 6;
        visualStyle.m_body.cProp29 = 7;
        visualStyle.m_body.bdProp34 = 3.5;
        visualStyle.m_body.bdProp38 = 4.5;
        visualStyle.m_body.bdProp39 = 5.5;
        wroteVisualStyle_ = registeredVisualStyle_
            && writer_->writeVisualStyle(&visualStyle)
            && visualStyle.handle != 0;

        DRW_RenderSettings renderSettings;
        renderSettings.handle = 0xC100u;
        renderSettings.parentHandle = dictionary.handle;
        renderSettings.m_kind = DRW_RenderSettings::Settings;
        renderSettings.m_classVersion = 1;
        renderSettings.m_name = "LOCAL_RENDERSETTINGS";
        renderSettings.m_strings = {"LOCAL_RENDERSETTINGS", "", "LOCAL_RENDER_DESC"};
        renderSettings.m_longs = {1, 2};
        renderSettings.m_bools = {true, false, true, false};
        renderSettings.m_description = "LOCAL_RENDER_DESC";
        wroteRenderSettings_ = registeredRenderSettings_
            && writer_->writeRenderSettings(&renderSettings)
            && renderSettings.handle != 0;

        DRW_RenderSettings renderEnvironment;
        renderEnvironment.handle = 0xC200u;
        renderEnvironment.parentHandle = dictionary.handle;
        renderEnvironment.m_kind = DRW_RenderSettings::Environment;
        renderEnvironment.m_classVersion = 1;
        renderEnvironment.m_name = "LOCAL_RENDER_ENVIRONMENT";
        renderEnvironment.m_strings = {"LOCAL_RENDER_ENVIRONMENT"};
        renderEnvironment.m_bools = {true, false, true};
        renderEnvironment.m_bytes = {10, 20, 30};
        renderEnvironment.m_doubles = {0.1, 0.9, 2.0, 3.0};
        wroteRenderEnvironment_ = registeredRenderEnvironment_
            && writer_->writeRenderSettings(&renderEnvironment)
            && renderEnvironment.handle != 0;

        DRW_RenderSettings renderGlobal;
        renderGlobal.handle = 0xC300u;
        renderGlobal.parentHandle = dictionary.handle;
        renderGlobal.m_kind = DRW_RenderSettings::Global;
        renderGlobal.m_classVersion = 1;
        renderGlobal.m_name = "LOCAL_RENDER_GLOBAL";
        renderGlobal.m_strings = {"LOCAL_RENDER_GLOBAL"};
        renderGlobal.m_longs = {1, 7, 8};
        wroteRenderGlobal_ = registeredRenderGlobal_
            && writer_->writeRenderSettings(&renderGlobal)
            && renderGlobal.handle != 0;

        DRW_RenderSettings renderEntry;
        renderEntry.handle = 0xC400u;
        renderEntry.parentHandle = dictionary.handle;
        renderEntry.m_kind = DRW_RenderSettings::Entry;
        renderEntry.m_classVersion = 1;
        renderEntry.m_name = "LOCAL_RENDER_ENTRY";
        renderEntry.m_strings = {"LOCAL_RENDER_ENTRY", "", ""};
        renderEntry.m_longs = {1, 11, 12, 13, 14, 15, 16, 17};
        renderEntry.m_shorts = {1, 2, 3, 4, 5, 6};
        renderEntry.m_doubles = {1.5};
        wroteRenderEntry_ = registeredRenderEntry_
            && writer_->writeRenderSettings(&renderEntry)
            && renderEntry.handle != 0;

        DRW_RenderSettings renderRapid;
        renderRapid.handle = 0xC600u;
        renderRapid.parentHandle = dictionary.handle;
        renderRapid.m_kind = DRW_RenderSettings::RapidRT;
        renderRapid.m_classVersion = 1;
        renderRapid.m_name = "LOCAL_RENDER_RAPIDRT";
        renderRapid.m_strings = {"LOCAL_RENDER_RAPIDRT", "", "LOCAL_RAPID_DESC"};
        renderRapid.m_longs = {1, 9, 2, 3, 4, 5, 6, 7};
        renderRapid.m_bools = {true, false, true, false};
        renderRapid.m_doubles = {0.25, 0.75};
        renderRapid.m_hasPredefined = true;
        wroteRenderRapid_ = registeredRenderRapid_
            && writer_->writeRenderSettings(&renderRapid)
            && renderRapid.handle != 0;

        DRW_RenderSettings renderMental;
        renderMental.handle = 0xC700u;
        renderMental.parentHandle = dictionary.handle;
        renderMental.m_kind = DRW_RenderSettings::MentalRay;
        renderMental.m_classVersion = 1;
        renderMental.m_name = "LOCAL_RENDER_MENTALRAY";
        renderMental.m_strings = {"LOCAL_RENDER_MENTALRAY", "", "LOCAL_MENTAL_DESC"};
        renderMental.m_longs = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
        renderMental.m_shorts = {1, 2, 3, 4, 5, 6, 7};
        renderMental.m_bools = {true, false, true, false, true, false, true, false};
        renderMental.m_doubles = {
            0.1, 0.2, 0.3, 0.4, 0.5, 0.6,
            0.7, 0.8, 0.9, 1.0, 1.1, 1.2};
        renderMental.m_hasPredefined = true;
        wroteRenderMental_ = registeredRenderMental_
            && writer_->writeRenderSettings(&renderMental)
            && renderMental.handle != 0;

        DRW_Material material;
        material.handle = 0xC800u;
        material.parentHandle = dictionary.handle;
        material.m_name = "LOCAL_MATERIAL";
        material.m_description = "LOCAL_MATERIAL_DESC";
        wroteMaterial_ = registeredMaterial_ && writer_->writeMaterial(&material)
            && material.handle != 0;

        DRW_DbColor dbColor;
        dbColor.handle = 0xC900u;
        dbColor.parentHandle = dictionary.handle;
        dbColor.rgb = 0x123456;
        dbColor.colorMethod = dwgColor::RGB;
        dbColor.name = "LOCAL_COLOR";
        dbColor.bookName = "LOCAL_BOOK";
        const bool dbColorWrite = writer_->writeDbColor(&dbColor);
        wroteDbColor_ = writer_->getVersion() < DRW::AC1018
            ? false : registeredDbColor_ && dbColorWrite
                && dbColor.handle != 0;
        rejectedUnsupportedDbColor_ = writer_->getVersion() < DRW::AC1018
            && !dbColorWrite;

        // A failed object write must not poison the following valid frames or
        // publish a partial object.  The writer's public transaction wrapper
        // owns the rollback boundary; this assertion keeps that contract in
        // the runtime lane without retaining a malformed drawing.
        DRW_XRecord invalidXRecord;
        invalidXRecord.handle = 0xA701u;
        invalidXRecord.parentHandle = dictionary.handle;
        invalidXRecord.m_values.emplace_back(
            40, std::numeric_limits<double>::quiet_NaN());
        rejectedMalformedObject_ = !writer_->writeXRecord(&invalidXRecord);

        DRW_MLineStyle invalidMLineStyle;
        invalidMLineStyle.handle = 0xA801u;
        invalidMLineStyle.parentHandle = dictionary.handle;
        invalidMLineStyle.name = "LOCAL_BAD_MLINESTYLE";
        DRW_MLineElement invalidMLineElement;
        invalidMLineElement.offset = std::numeric_limits<double>::quiet_NaN();
        invalidMLineStyle.elements.push_back(invalidMLineElement);
        rejectedMalformedStyle_ = !writer_->writeMLineStyle(&invalidMLineStyle);

        DRW_MLeaderStyle invalidMLeaderStyle;
        invalidMLeaderStyle.handle = 0xA901u;
        invalidMLeaderStyle.parentHandle = dictionary.handle;
        invalidMLeaderStyle.name = "LOCAL_BAD_MLEADERSTYLE";
        invalidMLeaderStyle.firstSegmentAngle =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedMLeaderStyle_ =
            !writer_->writeMLeaderStyle(&invalidMLeaderStyle);

        DRW_DictionaryVar invalidDictionaryVar;
        invalidDictionaryVar.handle = 0xB001u;
        invalidDictionaryVar.parentHandle = dictionary.handle;
        invalidDictionaryVar.name = "LOCAL_BAD_DICTIONARYVAR";
        invalidDictionaryVar.m_schema = 256;
        rejectedMalformedDictionaryVar_ =
            !writer_->writeDictionaryVar(&invalidDictionaryVar);

        DRW_DictionaryWithDefault invalidDictionaryWithDefault;
        invalidDictionaryWithDefault.handle = 0xB101u;
        invalidDictionaryWithDefault.parentHandle = dictionary.handle;
        invalidDictionaryWithDefault.cloning = 1;
        invalidDictionaryWithDefault.hardOwner = 1;
        invalidDictionaryWithDefault.m_defaultEntryHandle = 0;
        rejectedMalformedDictionaryWithDefault_ =
            !writer_->writeDictionaryWithDefault(&invalidDictionaryWithDefault);

        DRW_SortEntsTable invalidSortEnts;
        invalidSortEnts.handle = 0xB201u;
        invalidSortEnts.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
        invalidSortEnts.m_blockOwnerHandle = DRW::DwgModelSpaceBlockRecordHandle;
        invalidSortEnts.m_entityHandles = {modelSpaceLineHandle_};
        rejectedMalformedSortEntsTable_ = !writer_->writeSortEntsTable(&invalidSortEnts);

        DRW_FieldList invalidFieldList;
        invalidFieldList.handle = 0xB301u;
        invalidFieldList.parentHandle = dictionary.handle;
        invalidFieldList.m_unknown = 2;
        rejectedMalformedFieldList_ = !writer_->writeFieldList(&invalidFieldList);

        DRW_Field invalidField;
        invalidField.handle = 0xB401u;
        invalidField.parentHandle = dictionary.handle;
        invalidField.m_evaluatorId = "ACAD";
        invalidField.m_fieldCode = "LOCAL_BAD_FIELD";
        invalidField.m_value.m_dataType = 99;
        rejectedMalformedField_ = !writer_->writeField(&invalidField);

        DRW_RasterVariables invalidRasterVariables;
        invalidRasterVariables.handle = 0xB501u;
        invalidRasterVariables.parentHandle = dictionary.handle;
        invalidRasterVariables.m_classVersion = 11;
        rejectedMalformedRasterVariables_ =
            !writer_->writeRasterVariables(&invalidRasterVariables);

        DRW_WipeoutVariables invalidWipeoutVariables;
        invalidWipeoutVariables.handle = 0xB601u;
        invalidWipeoutVariables.parentHandle = dictionary.handle;
        invalidWipeoutVariables.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedWipeoutVariables_ =
            !writer_->writeWipeoutVariables(&invalidWipeoutVariables);

        DRW_VisualStyle invalidVisualStyle;
        invalidVisualStyle.handle = 0xC001u;
        invalidVisualStyle.parentHandle = dictionary.handle;
        invalidVisualStyle.desc = "LOCAL_BAD_VISUALSTYLE";
        invalidVisualStyle.m_body.faceOpacity =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedVisualStyle_ =
            !writer_->writeVisualStyle(&invalidVisualStyle);

        DRW_RenderSettings invalidRenderSettings;
        invalidRenderSettings.handle = 0xC101u;
        invalidRenderSettings.parentHandle = dictionary.handle;
        invalidRenderSettings.m_kind = DRW_RenderSettings::Settings;
        invalidRenderSettings.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedRenderSettings_ =
            !writer_->writeRenderSettings(&invalidRenderSettings);

        DRW_RenderSettings invalidRenderEnvironment;
        invalidRenderEnvironment.handle = 0xC201u;
        invalidRenderEnvironment.parentHandle = dictionary.handle;
        invalidRenderEnvironment.m_kind = DRW_RenderSettings::Environment;
        invalidRenderEnvironment.m_doubles = {
            std::numeric_limits<double>::quiet_NaN()};
        rejectedMalformedRenderEnvironment_ =
            !writer_->writeRenderSettings(&invalidRenderEnvironment);

        DRW_RenderSettings invalidRenderGlobal;
        invalidRenderGlobal.handle = 0xC301u;
        invalidRenderGlobal.parentHandle = dictionary.handle;
        invalidRenderGlobal.m_kind = DRW_RenderSettings::Global;
        invalidRenderGlobal.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedRenderGlobal_ =
            !writer_->writeRenderSettings(&invalidRenderGlobal);

        DRW_RenderSettings invalidRenderEntry;
        invalidRenderEntry.handle = 0xC401u;
        invalidRenderEntry.parentHandle = dictionary.handle;
        invalidRenderEntry.m_kind = DRW_RenderSettings::Entry;
        invalidRenderEntry.m_shorts = {70000};
        rejectedMalformedRenderEntry_ =
            !writer_->writeRenderSettings(&invalidRenderEntry);

        DRW_RenderSettings invalidRenderRapid;
        invalidRenderRapid.handle = 0xC601u;
        invalidRenderRapid.parentHandle = dictionary.handle;
        invalidRenderRapid.m_kind = DRW_RenderSettings::RapidRT;
        invalidRenderRapid.m_doubles = {
            std::numeric_limits<double>::quiet_NaN()};
        rejectedMalformedRenderRapid_ =
            !writer_->writeRenderSettings(&invalidRenderRapid);

        DRW_RenderSettings invalidRenderMental;
        invalidRenderMental.handle = 0xC701u;
        invalidRenderMental.parentHandle = dictionary.handle;
        invalidRenderMental.m_kind = DRW_RenderSettings::MentalRay;
        invalidRenderMental.m_doubles = {
            std::numeric_limits<double>::quiet_NaN()};
        rejectedMalformedRenderMental_ =
            !writer_->writeRenderSettings(&invalidRenderMental);

        DRW_Material invalidMaterial;
        invalidMaterial.handle = 0xC801u;
        invalidMaterial.parentHandle = dictionary.handle;
        invalidMaterial.m_name = "LOCAL_BAD_MATERIAL";
        invalidMaterial.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedMaterial_ = !writer_->writeMaterial(&invalidMaterial);

        DRW_DbColor invalidDbColor;
        invalidDbColor.handle = 0xC901u;
        invalidDbColor.parentHandle = dictionary.handle;
        invalidDbColor.rgb = 0x123456;
        invalidDbColor.colorMethod = dwgColor::RGB;
        invalidDbColor.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedDbColor_ = !writer_->writeDbColor(&invalidDbColor);

        DRW_LightList lightList;
        lightList.handle = 0xCA00u;
        lightList.parentHandle = dictionary.handle;
        lightList.m_classVersion = 1;
        lightList.m_lightCount = 1;
        lightList.m_lights.push_back({modelSpaceLineHandle_, "LOCAL_LIGHT"});
        wroteLightList_ = registeredLightList_ && writer_->writeLightList(&lightList)
            && lightList.handle != 0;

        DRW_LightList invalidLightList;
        invalidLightList.handle = 0xCA01u;
        invalidLightList.parentHandle = dictionary.handle;
        invalidLightList.m_lightCount = 2;
        invalidLightList.m_lights.push_back({modelSpaceLineHandle_, "LOCAL_LIGHT"});
        rejectedMalformedLightList_ = !writer_->writeLightList(&invalidLightList);

        DRW_Scale scale;
        scale.handle = 0xCB00u;
        scale.parentHandle = dictionary.handle;
        scale.flag = 0;
        scale.name = "LOCAL_SCALE";
        scale.paperUnits = 1.0;
        scale.drawingUnits = 48.0;
        scale.isUnitScale = false;
        wroteScale_ = registeredScale_ && writer_->writeScale(&scale)
            && scale.handle != 0;

        DRW_Scale invalidScale;
        invalidScale.handle = 0xCB01u;
        invalidScale.parentHandle = dictionary.handle;
        invalidScale.name = "LOCAL_BAD_SCALE";
        invalidScale.paperUnits = std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedScale_ = !writer_->writeScale(&invalidScale);

        DRW_IDBuffer idBuffer;
        idBuffer.handle = 0xCC00u;
        idBuffer.parentHandle = dictionary.handle;
        idBuffer.classVersion = 0;
        idBuffer.objIds = {modelSpaceLineHandle_};
        wroteIDBuffer_ = registeredIDBuffer_ && writer_->writeIDBuffer(&idBuffer)
            && idBuffer.handle != 0;

        DRW_IDBuffer invalidIDBuffer;
        invalidIDBuffer.handle = 0xCC01u;
        invalidIDBuffer.parentHandle = dictionary.handle;
        invalidIDBuffer.objIds.assign(DRW_IDBuffer::kMaxObjectIds + 1, 0u);
        rejectedMalformedIDBuffer_ = !writer_->writeIDBuffer(&invalidIDBuffer);

        DRW_LayerIndex layerIndex;
        layerIndex.handle = 0xCD00u;
        layerIndex.parentHandle = dictionary.handle;
        layerIndex.timestamp1 = 100;
        layerIndex.timestamp2 = 200;
        layerIndex.entries.push_back({1, "LOCAL_LAYER", idBuffer.handle});
        wroteLayerIndex_ = registeredLayerIndex_
            && writer_->writeLayerIndex(&layerIndex)
            && layerIndex.handle != 0;

        DRW_LayerIndex invalidLayerIndex;
        invalidLayerIndex.handle = 0xCD01u;
        invalidLayerIndex.parentHandle = dictionary.handle;
        invalidLayerIndex.entries.push_back({1, "LOCAL_BAD_LAYER", 0});
        invalidLayerIndex.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedLayerIndex_ = !writer_->writeLayerIndex(&invalidLayerIndex);

        DRW_SpatialIndex spatialIndex;
        spatialIndex.handle = 0xCE00u;
        spatialIndex.parentHandle = dictionary.handle;
        spatialIndex.timestamp1 = 300;
        spatialIndex.timestamp2 = 400;
        wroteSpatialIndex_ = registeredSpatialIndex_
            && writer_->writeSpatialIndex(&spatialIndex)
            && spatialIndex.handle != 0;

        DRW_SpatialIndex invalidSpatialIndex;
        invalidSpatialIndex.handle = 0xCE01u;
        invalidSpatialIndex.parentHandle = dictionary.handle;
        invalidSpatialIndex.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedSpatialIndex_ =
            !writer_->writeSpatialIndex(&invalidSpatialIndex);

        DRW_TableStyle tableStyle;
        tableStyle.handle = 0xCF00u;
        tableStyle.parentHandle = dictionary.handle;
        tableStyle.m_name = "LOCAL_TABLESTYLE";
        tableStyle.m_flowDirection = 0;
        tableStyle.m_flags = 0;
        tableStyle.m_horizontalCellMargin = 0.1;
        tableStyle.m_verticalCellMargin = 0.2;
        tableStyle.m_titleSuppressed = false;
        tableStyle.m_headerSuppressed = false;
        for (int rowIndex = 0; rowIndex < 3; ++rowIndex) {
            DRW_TableStyleRowStyle row;
            row.m_textHeight = 1.0 + rowIndex;
            row.m_textAlignment = rowIndex;
            row.m_textColor = 0;
            row.m_fillColor = 0;
            row.m_hasBackgroundColor = false;
            row.m_valueDataType = rowIndex;
            row.m_valueUnitType = rowIndex + 1;
            row.m_valueFormatString = "LOCAL_FORMAT";
            for (int borderIndex = 0; borderIndex < 6; ++borderIndex) {
                DRW_TableStyleBorder border;
                border.m_edgeFlags = 1 << borderIndex;
                border.m_lineWeight = borderIndex;
                border.m_color = 0;
                border.m_visible = 1;
                row.m_borders.push_back(border);
            }
            tableStyle.m_rowStyles.push_back(row);
        }
        const bool tableStyleSupported = writer_->getVersion() <= DRW::AC1021;
        if (tableStyleSupported) {
            wroteTableStyle_ = registeredTableStyle_
                && writer_->writeTableStyle(&tableStyle)
                && tableStyle.handle != 0;
            DRW_TableStyle invalidTableStyle = tableStyle;
            invalidTableStyle.handle = 0xCF01u;
            invalidTableStyle.m_rowStyles.pop_back();
            rejectedMalformedTableStyle_ =
                !writer_->writeTableStyle(&invalidTableStyle);
        } else {
            rejectedUnsupportedTableStyle_ =
                !writer_->writeTableStyle(&tableStyle);
        }

        DRW_SpatialFilter spatialFilter;
        spatialFilter.handle = 0xD000u;
        spatialFilter.parentHandle = dictionary.handle;
        spatialFilter.m_boundaryPoints = {
            DRW_Coord{1.0, 2.0, 0.0}, DRW_Coord{3.0, 4.0, 0.0}};
        spatialFilter.m_normal = DRW_Coord{0.0, 0.0, 1.0};
        spatialFilter.m_origin = DRW_Coord{10.0, 20.0, 30.0};
        spatialFilter.m_displayBoundary = true;
        spatialFilter.m_clipFrontPlane = true;
        spatialFilter.m_frontDistance = 5.0;
        spatialFilter.m_inverseInsertTransform.assign(12, 0.0);
        spatialFilter.m_insertTransform.assign(12, 0.0);
        spatialFilter.m_inverseInsertTransform[0] = 1.0;
        spatialFilter.m_inverseInsertTransform[5] = 1.0;
        spatialFilter.m_inverseInsertTransform[10] = 1.0;
        spatialFilter.m_insertTransform[0] = 1.0;
        spatialFilter.m_insertTransform[5] = 1.0;
        spatialFilter.m_insertTransform[10] = 1.0;
        wroteSpatialFilter_ = registeredSpatialFilter_
            && writer_->writeSpatialFilter(&spatialFilter)
            && spatialFilter.handle != 0;

        DRW_SpatialFilter invalidSpatialFilter = spatialFilter;
        invalidSpatialFilter.handle = 0xD001u;
        invalidSpatialFilter.m_boundaryPoints.assign(
            DRW_SpatialFilter::kMaxBoundaryPoints + 1,
            DRW_Coord{0.0, 0.0, 0.0});
        rejectedMalformedSpatialFilter_ =
            !writer_->writeSpatialFilter(&invalidSpatialFilter);

        DRW_GeoData geoData;
        geoData.handle = 0xD100u;
        geoData.parentHandle = 0x17u;
        geoData.xDictHandle = dictionary.handle;
        geoData.m_version = 1;
        geoData.m_hostBlockHandle = 0x17u;
        geoData.m_coordinatesType = 1;
        geoData.m_designPoint = DRW_Coord{100.0, 200.0, 300.0};
        geoData.m_referencePoint = DRW_Coord{10.0, 20.0, 30.0};
        geoData.m_upDirection = DRW_Coord{0.0, 0.0, 1.0};
        geoData.m_northDirection = DRW_Coord{0.0, 1.0, 0.0};
        geoData.m_horizontalUnitScale = 1.5;
        geoData.m_horizontalUnits = 2;
        geoData.m_coordinateSystemDefinition = "LOCAL_COORD_SYS";
        geoData.m_geoRssTag = "LOCAL_GEO_TAG";
        geoData.m_observationFromTag = "LOCAL_FROM";
        geoData.m_observationToTag = "LOCAL_TO";
        geoData.m_observationCoverageTag = "LOCAL_COVERAGE";
        wroteGeoData_ = registeredGeoData_ && writer_->writeGeoData(&geoData)
            && geoData.handle != 0;

        DRW_GeoData invalidGeoData = geoData;
        invalidGeoData.handle = 0xD101u;
        invalidGeoData.m_designPoint.x = std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedGeoData_ = !writer_->writeGeoData(&invalidGeoData);

        DRW_GeoData geoDataV2;
        geoDataV2.handle = 0xD200u;
        geoDataV2.parentHandle = 0x17u;
        geoDataV2.xDictHandle = dictionary.handle;
        geoDataV2.m_version = 2;
        geoDataV2.m_hostBlockHandle = 0x17u;
        geoDataV2.m_coordinatesType = 2;
        geoDataV2.m_designPoint = DRW_Coord{100.0, 200.0, 300.0};
        geoDataV2.m_referencePoint = DRW_Coord{10.0, 20.0, 30.0};
        geoDataV2.m_upDirection = DRW_Coord{0.0, 0.0, 1.0};
        geoDataV2.m_northDirection = DRW_Coord{0.0, 1.0, 0.0};
        geoDataV2.m_horizontalUnitScale = 1.5;
        geoDataV2.m_horizontalUnits = 2;
        geoDataV2.m_verticalUnitScale = 2.5;
        geoDataV2.m_verticalUnits = 3;
        geoDataV2.m_scaleEstimationMethod = 1;
        geoDataV2.m_userSpecifiedScaleFactor = 1.25;
        geoDataV2.m_enableSeaLevelCorrection = true;
        geoDataV2.m_seaLevelElevation = 4.5;
        geoDataV2.m_coordinateProjectionRadius = 6.5;
        geoDataV2.m_coordinateSystemDefinition = "LOCAL_COORD_SYS_V2";
        geoDataV2.m_geoRssTag = "LOCAL_GEO_TAG_V2";
        geoDataV2.m_observationFromTag = "LOCAL_FROM_V2";
        geoDataV2.m_observationToTag = "LOCAL_TO_V2";
        geoDataV2.m_observationCoverageTag = "LOCAL_COVERAGE_V2";
        wroteGeoDataV2_ = registeredGeoDataV2_
            && writer_->writeGeoData(&geoDataV2)
            && geoDataV2.handle != 0;

        DRW_UnderlayDefinition pdfDefinition;
        pdfDefinition.handle = 0xD300u;
        pdfDefinition.parentHandle = dictionary.handle;
        pdfDefinition.kind = DRW_UnderlayDefinition::PDF;
        pdfDefinition.filename = "LOCAL_PDF.pdf";
        pdfDefinition.sheetName = "LOCAL_PDF_SHEET";
        wrotePdfUnderlay_ = registeredPdfUnderlay_
            && writer_->writeUnderlayDefinition(&pdfDefinition)
            && pdfDefinition.handle != 0;

        DRW_UnderlayDefinition dgnDefinition;
        dgnDefinition.handle = 0xD400u;
        dgnDefinition.parentHandle = dictionary.handle;
        dgnDefinition.kind = DRW_UnderlayDefinition::DGN;
        dgnDefinition.filename = "LOCAL_DGN.dgn";
        dgnDefinition.sheetName = "LOCAL_DGN_SHEET";
        wroteDgnUnderlay_ = registeredDgnUnderlay_
            && writer_->writeUnderlayDefinition(&dgnDefinition)
            && dgnDefinition.handle != 0;

        DRW_UnderlayDefinition dwfDefinition;
        dwfDefinition.handle = 0xD500u;
        dwfDefinition.parentHandle = dictionary.handle;
        dwfDefinition.kind = DRW_UnderlayDefinition::DWF;
        dwfDefinition.filename = "LOCAL_DWF.dwf";
        dwfDefinition.sheetName = "LOCAL_DWF_SHEET";
        wroteDwfUnderlay_ = registeredDwfUnderlay_
            && writer_->writeUnderlayDefinition(&dwfDefinition)
            && dwfDefinition.handle != 0;

        DRW_UnderlayDefinition invalidUnderlay = pdfDefinition;
        invalidUnderlay.handle = 0xD301u;
        invalidUnderlay.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedUnderlay_ =
            !writer_->writeUnderlayDefinition(&invalidUnderlay);

        DRW_PointCloudDef pointCloudDefinition;
        pointCloudDefinition.handle = 0xD600u;
        pointCloudDefinition.parentHandle = dictionary.handle;
        pointCloudDefinition.m_kind = DRW_PointCloudDef::Definition;
        pointCloudDefinition.m_classVersion = 2;
        pointCloudDefinition.m_sourceFilename = "LOCAL_POINTCLOUD.rcs";
        pointCloudDefinition.m_isLoaded = true;
        pointCloudDefinition.m_pointCount = 1234;
        pointCloudDefinition.m_extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
        pointCloudDefinition.m_extentsMax = DRW_Coord{10.0, 20.0, 30.0};
        wrotePointCloudDefinition_ = registeredPointCloudDefinition_
            && writer_->writePointCloudDef(&pointCloudDefinition)
            && pointCloudDefinition.handle != 0;

        DRW_PointCloudDef pointCloudDefinitionEx = pointCloudDefinition;
        pointCloudDefinitionEx.handle = 0xD601u;
        pointCloudDefinitionEx.m_kind = DRW_PointCloudDef::DefinitionEx;
        pointCloudDefinitionEx.m_sourceFilename = "LOCAL_POINTCLOUD_EX.rcs";
        pointCloudDefinitionEx.m_pointCount = 5678;
        wrotePointCloudDefinitionEx_ = registeredPointCloudDefinitionEx_
            && writer_->writePointCloudDef(&pointCloudDefinitionEx)
            && pointCloudDefinitionEx.handle != 0;

        DRW_PointCloudDef pointCloudReactor;
        pointCloudReactor.handle = 0xD602u;
        pointCloudReactor.parentHandle = pointCloudDefinition.handle;
        pointCloudReactor.m_kind = DRW_PointCloudDef::Reactor;
        pointCloudReactor.m_classVersion = 2;
        wrotePointCloudReactor_ = registeredPointCloudReactor_
            && writer_->writePointCloudDef(&pointCloudReactor)
            && pointCloudReactor.handle != 0;

        DRW_PointCloudDef pointCloudReactorEx = pointCloudReactor;
        pointCloudReactorEx.handle = 0xD603u;
        pointCloudReactorEx.parentHandle = pointCloudDefinitionEx.handle;
        pointCloudReactorEx.m_kind = DRW_PointCloudDef::ReactorEx;
        wrotePointCloudReactorEx_ = registeredPointCloudReactorEx_
            && writer_->writePointCloudDef(&pointCloudReactorEx)
            && pointCloudReactorEx.handle != 0;

        DRW_PointCloudDef invalidPointCloud = pointCloudDefinition;
        invalidPointCloud.handle = 0xD604u;
        invalidPointCloud.m_extentsMin.x =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedPointCloud_ =
            !writer_->writePointCloudDef(&invalidPointCloud);

        DRW_PointCloudColorMap colorMap;
        colorMap.handle = 0xD800u;
        colorMap.parentHandle = dictionary.handle;
        colorMap.m_classVersion = 1;
        colorMap.m_defaultIntensityColorScheme = "LOCAL_INTENSITY";
        colorMap.m_defaultElevationColorScheme = "LOCAL_ELEVATION";
        colorMap.m_defaultClassificationColorScheme = "LOCAL_CLASSIFICATION";
        colorMap.m_colorRampCount = 1;
        DRW_PointCloudColorMapRamp colorRamp;
        colorRamp.m_classVersion = 2;
        colorRamp.m_rampCount = 2;
        colorRamp.m_colorSchemes = {"LOCAL_RAMP_LOW", "LOCAL_RAMP_HIGH"};
        colorMap.m_colorRamps.push_back(colorRamp);
        colorMap.m_classificationColorRampCount = 1;
        DRW_PointCloudColorMapRamp classificationRamp;
        classificationRamp.m_classVersion = 3;
        classificationRamp.m_rampCount = 1;
        classificationRamp.m_colorSchemes = {"LOCAL_CLASS_A"};
        colorMap.m_classificationColorRamps.push_back(classificationRamp);
        wrotePointCloudColorMap_ = registeredPointCloudColorMap_
            && writer_->writePointCloudColorMap(&colorMap)
            && colorMap.handle != 0;

        DRW_PointCloudColorMap invalidColorMap = colorMap;
        invalidColorMap.handle = 0xD801u;
        invalidColorMap.m_colorRampCount = 2;
        rejectedMalformedPointCloudColorMap_ =
            !writer_->writePointCloudColorMap(&invalidColorMap);

        DRW_NavisworksModelDef navisworksDefinition;
        navisworksDefinition.handle = 0xD900u;
        navisworksDefinition.parentHandle = dictionary.handle;
        navisworksDefinition.m_flags = 3;
        navisworksDefinition.m_path = "LOCAL_NAVISWORKS.nwd";
        navisworksDefinition.m_status = true;
        navisworksDefinition.m_minExtent = DRW_Coord{-4.0, -5.0, -6.0};
        navisworksDefinition.m_maxExtent = DRW_Coord{40.0, 50.0, 60.0};
        navisworksDefinition.m_hostDrawingVisibility = true;
        wroteNavisworksModelDef_ = registeredNavisworksModelDef_
            && writer_->writeNavisworksModelDef(&navisworksDefinition)
            && navisworksDefinition.handle != 0;

        DRW_NavisworksModelDef invalidNavisworks = navisworksDefinition;
        invalidNavisworks.handle = 0xD901u;
        invalidNavisworks.m_maxExtent.x =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedNavisworksModelDef_ =
            !writer_->writeNavisworksModelDef(&invalidNavisworks);

        DRW_SunStudy sunStudy;
        sunStudy.handle = 0xDA00u;
        sunStudy.parentHandle = dictionary.handle;
        sunStudy.m_classVersion = 1;
        sunStudy.m_setupName = "LOCAL_SUNSTUDY";
        sunStudy.m_description = "LOCAL_SUN_DESC";
        sunStudy.m_sheetSetName = "LOCAL_SHEETSET";
        sunStudy.m_sheetSubsetName = "LOCAL_SUBSET";
        sunStudy.m_outputType = 0;
        sunStudy.m_useSubset = true;
        sunStudy.m_selectDatesFromCalendar = true;
        sunStudy.m_selectRangeOfDates = true;
        sunStudy.m_lockViewports = true;
        sunStudy.m_labelViewports = false;
        sunStudy.m_startTime = 10;
        sunStudy.m_endTime = 20;
        sunStudy.m_interval = 5;
        sunStudy.m_shadePlotType = 2;
        sunStudy.m_viewportCount = 3;
        sunStudy.m_rowCount = 4;
        sunStudy.m_columnCount = 5;
        sunStudy.m_spacing = 1.5;
        sunStudy.m_dates = {{2451545, 3600000}};
        sunStudy.m_hours = {true, false, true};
        sunStudy.m_pageSetupWizardHandle = 0xA603u;
        sunStudy.m_viewHandle = 0xA700u;
        sunStudy.m_visualStyleHandle = 0xC000u;
        sunStudy.m_textStyleHandle = 0xA800u;
        wroteSunStudy_ = registeredSunStudy_
            && writer_->writeSunStudy(&sunStudy)
            && sunStudy.handle != 0;

        DRW_SunStudy invalidSunStudy = sunStudy;
        invalidSunStudy.handle = 0xDA01u;
        invalidSunStudy.m_spacing = std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedSunStudy_ = !writer_->writeSunStudy(&invalidSunStudy);

        DRW_MotionPath motionPath;
        motionPath.handle = 0xDB00u;
        motionPath.parentHandle = dictionary.handle;
        motionPath.m_classVersion = 2;
        motionPath.m_cameraPathHandle = 0xD925u;
        motionPath.m_targetPathHandle = 0xD926u;
        motionPath.m_viewTableHandle = 0xA700u;
        motionPath.m_frames = 120;
        motionPath.m_frameRate = 30;
        motionPath.m_cornerDeceleration = true;
        wroteMotionPath_ = registeredMotionPath_
            && writer_->writeMotionPath(&motionPath)
            && motionPath.handle != 0;

        DRW_MotionPath invalidMotionPath = motionPath;
        invalidMotionPath.handle = 0xDB01u;
        invalidMotionPath.m_frames = std::numeric_limits<std::int32_t>::max();
        rejectedMalformedMotionPath_ =
            !writer_->writeMotionPath(&invalidMotionPath);

        DRW_CurvePath curvePath;
        curvePath.handle = 0xDC00u;
        curvePath.parentHandle = dictionary.handle;
        curvePath.m_classVersion = 2;
        curvePath.m_entityHandle = modelSpaceLineHandle_;
        wroteCurvePath_ = registeredCurvePath_
            && writer_->writeCurvePath(&curvePath)
            && curvePath.handle != 0;

        DRW_CurvePath invalidCurvePath = curvePath;
        invalidCurvePath.handle = 0xDC01u;
        invalidCurvePath.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedCurvePath_ = !writer_->writeCurvePath(&invalidCurvePath);

        DRW_PointPath pointPath;
        pointPath.handle = 0xDD00u;
        pointPath.parentHandle = dictionary.handle;
        pointPath.m_classVersion = 3;
        pointPath.m_point = DRW_Coord{7.0, 8.0, 9.0};
        wrotePointPath_ = registeredPointPath_
            && writer_->writePointPath(&pointPath)
            && pointPath.handle != 0;

        DRW_PointPath invalidPointPath = pointPath;
        invalidPointPath.handle = 0xDD01u;
        invalidPointPath.m_point.x = std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedPointPath_ = !writer_->writePointPath(&invalidPointPath);

        DRW_ObjectPtr objectPtr;
        objectPtr.handle = 0xDE00u;
        objectPtr.parentHandle = dictionary.handle;
        wroteObjectPtr_ = registeredObjectPtr_
            && writer_->writeObjectPtr(&objectPtr)
            && objectPtr.handle != 0;

        DRW_ObjectPtr invalidObjectPtr = objectPtr;
        invalidObjectPtr.handle = 0xDE01u;
        invalidObjectPtr.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedObjectPtr_ = !writer_->writeObjectPtr(&invalidObjectPtr);

        DRW_PartialViewingIndex partialViewingIndex;
        partialViewingIndex.handle = 0xDF00u;
        partialViewingIndex.parentHandle = dictionary.handle;
        partialViewingIndex.m_entries = {
            {DRW_Coord{-1.0, -2.0, -3.0}, DRW_Coord{10.0, 20.0, 30.0},
             modelSpaceLineHandle_},
            {DRW_Coord{40.0, 50.0, 60.0}, DRW_Coord{70.0, 80.0, 90.0},
             0xD925u},
        };
        wrotePartialViewingIndex_ = registeredPartialViewingIndex_
            && writer_->writePartialViewingIndex(&partialViewingIndex)
            && partialViewingIndex.handle != 0;

        DRW_PartialViewingIndex invalidPartialViewingIndex = partialViewingIndex;
        invalidPartialViewingIndex.handle = 0xDF01u;
        invalidPartialViewingIndex.m_entries.front().extentsMax.x =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedPartialViewingIndex_ =
            !writer_->writePartialViewingIndex(&invalidPartialViewingIndex);

        DRW_Background solidBackground;
        solidBackground.handle = 0xE000u;
        solidBackground.parentHandle = dictionary.handle;
        solidBackground.m_kind = DRW_Background::Solid;
        solidBackground.m_classVersion = 1;
        solidBackground.m_solidColor = 0x010203;
        wroteSolidBackground_ = registeredSolidBackground_
            && writer_->writeBackground(&solidBackground)
            && solidBackground.handle != 0;

        DRW_Background gradientBackground;
        gradientBackground.handle = 0xE100u;
        gradientBackground.parentHandle = dictionary.handle;
        gradientBackground.m_kind = DRW_Background::Gradient;
        gradientBackground.m_classVersion = 1;
        gradientBackground.m_colorTop = 0x101112;
        gradientBackground.m_colorMiddle = 0x202122;
        gradientBackground.m_colorBottom = 0x303132;
        gradientBackground.m_horizon = 0.1;
        gradientBackground.m_height = 0.2;
        gradientBackground.m_rotation = 0.3;
        wroteGradientBackground_ = registeredGradientBackground_
            && writer_->writeBackground(&gradientBackground)
            && gradientBackground.handle != 0;

        DRW_Background groundPlaneBackground;
        groundPlaneBackground.handle = 0xE200u;
        groundPlaneBackground.parentHandle = dictionary.handle;
        groundPlaneBackground.m_kind = DRW_Background::GroundPlane;
        groundPlaneBackground.m_classVersion = 1;
        groundPlaneBackground.m_colorSkyZenith = 0x404142;
        groundPlaneBackground.m_colorSkyHorizon = 0x505152;
        groundPlaneBackground.m_colorUndergroundHorizon = 0x606162;
        groundPlaneBackground.m_colorUndergroundAzimuth = 0x707172;
        groundPlaneBackground.m_colorNear = 0x808182;
        groundPlaneBackground.m_colorFar = 0x909192;
        wroteGroundPlaneBackground_ = registeredGroundPlaneBackground_
            && writer_->writeBackground(&groundPlaneBackground)
            && groundPlaneBackground.handle != 0;

        DRW_Background imageBackground;
        imageBackground.handle = 0xE300u;
        imageBackground.parentHandle = dictionary.handle;
        imageBackground.m_kind = DRW_Background::Image;
        imageBackground.m_classVersion = 1;
        imageBackground.m_fileName = "LOCAL_BACKGROUND_IMAGE.png";
        imageBackground.m_fitToScreen = true;
        imageBackground.m_maintainAspect = true;
        imageBackground.m_useTiling = false;
        imageBackground.m_offset = DRW_Coord{1.0, 2.0, 0.0};
        imageBackground.m_scale = DRW_Coord{3.0, 4.0, 0.0};
        wroteImageBackground_ = registeredImageBackground_
            && writer_->writeBackground(&imageBackground)
            && imageBackground.handle != 0;

        DRW_Background iblBackground;
        iblBackground.handle = 0xE400u;
        iblBackground.parentHandle = dictionary.handle;
        iblBackground.m_kind = DRW_Background::Ibl;
        iblBackground.m_classVersion = 1;
        iblBackground.m_iblName = "LOCAL_IBL";
        iblBackground.m_enabled = true;
        iblBackground.m_displayImage = true;
        iblBackground.m_rotation = 0.4;
        iblBackground.m_secondaryBackgroundHandle = imageBackground.handle;
        wroteIblBackground_ = registeredIblBackground_
            && writer_->writeBackground(&iblBackground)
            && iblBackground.handle != 0;

        DRW_Background skylightBackground;
        skylightBackground.handle = 0xE500u;
        skylightBackground.parentHandle = dictionary.handle;
        skylightBackground.m_kind = DRW_Background::Skylight;
        skylightBackground.m_classVersion = 1;
        skylightBackground.m_sunHandle = 0xDA00u;
        wroteSkylightBackground_ = registeredSkylightBackground_
            && writer_->writeBackground(&skylightBackground)
            && skylightBackground.handle != 0;

        DRW_Background invalidGradientBackground = gradientBackground;
        invalidGradientBackground.handle = 0xE600u;
        invalidGradientBackground.m_rotation =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedBackground_ =
            !writer_->writeBackground(&invalidGradientBackground);

        if (expectedVersion_ >= DRW::AC1021) {
            DRW_Section sectionManager;
            sectionManager.handle = 0xE700u;
            sectionManager.parentHandle = dictionary.handle;
            sectionManager.m_kind = DRW_Section::Manager;
            sectionManager.m_isLive = true;
            sectionManager.m_sectionHandles = {0xE800u};
            wroteSectionManager_ = registeredSectionManager_
                && writer_->writeSection(&sectionManager)
                && sectionManager.handle != 0;

            DRW_Section sectionSettings;
            sectionSettings.handle = 0xE800u;
            sectionSettings.parentHandle = dictionary.handle;
            sectionSettings.m_kind = DRW_Section::Settings;
            sectionSettings.m_currentType = 1;
            DRW_SectionTypeSettings sectionType;
            sectionType.m_type = 2;
            sectionType.m_generation = 3;
            sectionType.m_sourceHandles = {modelSpaceLineHandle_};
            sectionType.m_destinationBlockHandle = 0x17u;
            sectionType.m_destinationFile = "LOCAL_SECTION.dwg";
            DRW_SectionGeometrySettings sectionGeometry;
            sectionGeometry.m_numGeometries = 1;
            sectionGeometry.m_hexIndex = 4;
            sectionGeometry.m_flags = 5;
            sectionGeometry.m_color = 6;
            sectionGeometry.m_layer = "LOCAL_LAYER";
            sectionGeometry.m_lineType = "CONTINUOUS";
            sectionGeometry.m_lineTypeScale = 0.5;
            sectionGeometry.m_plotStyle = "LOCAL_STYLE";
            sectionGeometry.m_lineWeight = 7;
            sectionGeometry.m_faceTransparency = 8;
            sectionGeometry.m_edgeTransparency = 9;
            sectionGeometry.m_hatchType = 1;
            sectionGeometry.m_hatchPattern = "SOLID";
            sectionGeometry.m_hatchAngle = 0.25;
            sectionGeometry.m_hatchSpacing = 0.75;
            sectionGeometry.m_hatchScale = 1.25;
            sectionType.m_geometry.push_back(sectionGeometry);
            sectionSettings.m_types.push_back(sectionType);
            wroteSectionSettings_ = registeredSectionSettings_
                && writer_->writeSection(&sectionSettings)
                && sectionSettings.handle != 0;

            DRW_Section invalidSection = sectionSettings;
            invalidSection.handle = 0xE900u;
            invalidSection.m_types.resize(
                static_cast<std::size_t>(DRW_Section::kMaxSectionTypeCount) + 1u);
            rejectedMalformedSection_ = !writer_->writeSection(&invalidSection);
        } else {
            DRW_Section unsupportedManager;
            unsupportedManager.handle = 0xE700u;
            unsupportedManager.m_kind = DRW_Section::Manager;
            DRW_Section unsupportedSettings;
            unsupportedSettings.handle = 0xE800u;
            unsupportedSettings.m_kind = DRW_Section::Settings;
            rejectedUnsupportedSection_ = !writer_->writeSection(&unsupportedManager)
                && !writer_->writeSection(&unsupportedSettings);
        }

        DRW_TvDeviceProperties tvDevice;
        tvDevice.handle = 0xEA00u;
        tvDevice.parentHandle = dictionary.handle;
        tvDevice.flags = 1;
        tvDevice.maxRegenThreads = 2;
        tvDevice.useLutPalette = 3;
        tvDevice.alternateHighlight = 4;
        tvDevice.alternateHighlightColor = 5;
        tvDevice.geometryShaderUsage = 6;
        tvDevice.blendingMode = 7;
        tvDevice.antialiasingLevel = 0.25;
        tvDevice.valueBd2 = 0.75;
        wroteTvDeviceProperties_ = registeredTvDeviceProperties_
            && writer_->writeTvDeviceProperties(&tvDevice)
            && tvDevice.handle != 0;
        DRW_TvDeviceProperties invalidTvDevice = tvDevice;
        invalidTvDevice.handle = 0xEA01u;
        invalidTvDevice.antialiasingLevel =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedTvDeviceProperties_ =
            !writer_->writeTvDeviceProperties(&invalidTvDevice);

        DRW_VxControl vxControl;
        vxControl.handle = 0xEB00u;
        vxControl.parentHandle = dictionary.handle;
        vxControl.classVersion = 8;
        vxControl.flags = 9;
        vxControl.recordHandles = expectedVersion_ > DRW::AC1018
            ? std::vector<std::uint32_t>{modelSpaceLineHandle_}
            : std::vector<std::uint32_t>{};
        wroteVxControl_ = registeredVxControl_
            && writer_->writeVxControl(&vxControl)
            && vxControl.handle != 0;
        DRW_VxControl invalidVxControl = vxControl;
        invalidVxControl.handle = 0xEB01u;
        invalidVxControl.recordHandles.resize(1000001u);
        rejectedMalformedVxControl_ =
            !writer_->writeVxControl(&invalidVxControl);

        DRW_VxTableRecord vxTableRecord;
        vxTableRecord.handle = 0xEC00u;
        vxTableRecord.parentHandle = dictionary.handle;
        vxTableRecord.classVersion = 10;
        vxTableRecord.flags = 11;
        vxTableRecord.name = "LOCAL_VX_RECORD";
        wroteVxTableRecord_ = registeredVxTableRecord_
            && writer_->writeVxTableRecord(&vxTableRecord)
            && vxTableRecord.handle != 0;
        DRW_VxTableRecord invalidVxTableRecord = vxTableRecord;
        invalidVxTableRecord.handle = 0xEC01u;
        invalidVxTableRecord.reactorHandles.resize(1000001u);
        rejectedMalformedVxTableRecord_ =
            !writer_->writeVxTableRecord(&invalidVxTableRecord);

        DRW_Group group;
        group.handle = 0xA600u;
        group.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        group.m_description = "LOCAL_GROUP";
        group.m_entityHandles = {modelSpaceLineHandle_};
        wroteGroup_ = registeredGroup_ && writer_->writeGroup(&group)
            && group.handle != 0;
    }
    void writeAppId() override {}

    void writeEntities() override {
        if (writer_ == nullptr)
            return;
        DRW_Line line;
        line.basePoint = DRW_Coord(1.0, 2.0, 3.0);
        line.secPoint = DRW_Coord(4.0, 5.0, 6.0);
        wroteLine_ = writer_->writeLine(&line) && line.handle != 0;
        modelSpaceLineHandle_ = line.handle;

        DRW_Image image;
        image.basePoint = DRW_Coord(50.0, 60.0, 0.0);
        image.secPoint = DRW_Coord(114.0, 60.0, 0.0);
        image.vVector = DRW_Coord(0.0, 1.0, 0.0);
        image.sizeu = 64.0;
        image.sizev = 48.0;
        image.clip = 1;
        image.brightness = 60;
        image.contrast = 70;
        image.fade = 10;
        image.m_classVersion = 2;
        const std::string imageFileName = "LOCAL_IMAGE.png";
        DRW_ImageDef imageDefinition;
        imageDefinition.handle = 0xD700u;
        imageDefinition.imgVersion = 2;
        imageDefinition.u = 64.0;
        imageDefinition.v = 48.0;
        imageDefinition.up = 1.0;
        imageDefinition.vp = 1.0;
        imageDefinition.loaded = 1;
        imageDefinition.resolution = 0;
        DRW_ImageDefinitionReactor imageReactor;
        imageReactor.handle = 0xD701u;
        imageReactor.m_classVersion = 2;
        if (writer_->getVersion() < DRW::AC1018) {
            wroteImage_ = false;
            rejectedMalformedImage_ = true;
        } else {
            wroteImage_ = writer_->writeImage(
                    &image, &imageFileName, &imageDefinition, &imageReactor)
                && image.handle != 0
                && image.ref != 0
                && image.m_imageDefReactorHandle != 0;
            DRW_Image invalidImage = image;
            invalidImage.handle = 0xD710u;
            invalidImage.sizeu = std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedImage_ =
                !writer_->writeImage(&invalidImage, &imageFileName);
        }

        DRW_Point point;
        point.basePoint = DRW_Coord(7.0, 8.0, 9.0);
        wrotePoint_ = writer_->writePoint(&point) && point.handle != 0;

        DRW_Circle circle;
        circle.basePoint = DRW_Coord(10.0, 11.0, 12.0);
        circle.radious = 2.5;
        wroteCircle_ = writer_->writeCircle(&circle) && circle.handle != 0;

        DRW_Arc arc;
        arc.basePoint = DRW_Coord(13.0, 14.0, 15.0);
        arc.radious = 3.5;
        arc.staangle = 0.25;
        arc.endangle = 1.25;
        wroteArc_ = writer_->writeArc(&arc) && arc.handle != 0;

        DRW_LWPolyline polyline;
        polyline.vertexnum = 2;
        polyline.addVertex(DRW_Vertex2D(16.0, 17.0, 0.0));
        polyline.addVertex(DRW_Vertex2D(18.0, 19.0, 0.0));
        wrotePolyline_ = writer_->writeLWPolyline(&polyline)
            && polyline.handle != 0;

        DRW_Text text;
        text.basePoint = DRW_Coord(20.0, 21.0, 22.0);
        text.height = 1.5;
        text.text = "TEXT";
        wroteText_ = writer_->writeText(&text) && text.handle != 0;

        DRW_MText mtext;
        mtext.basePoint = DRW_Coord(23.0, 24.0, 25.0);
        mtext.height = 1.5;
        mtext.text = "MTEXT";
        wroteMText_ = writer_->writeMText(&mtext) && mtext.handle != 0;

        DRW_Ellipse ellipse;
        ellipse.basePoint = DRW_Coord(26.0, 27.0, 28.0);
        ellipse.secPoint = DRW_Coord(2.0, 0.0, 0.0);
        ellipse.ratio = 0.5;
        wroteEllipse_ = writer_->writeEllipse(&ellipse) && ellipse.handle != 0;

        DRW_Trace trace;
        trace.basePoint = DRW_Coord(29.0, 30.0, 0.0);
        trace.secPoint = DRW_Coord(31.0, 30.0, 0.0);
        trace.thirdPoint = DRW_Coord(31.0, 32.0, 0.0);
        trace.fourPoint = DRW_Coord(29.0, 32.0, 0.0);
        wroteTrace_ = writer_->writeTrace(&trace) && trace.handle != 0;

        DRW_Solid solid;
        solid.basePoint = DRW_Coord(33.0, 34.0, 0.0);
        solid.secPoint = DRW_Coord(35.0, 34.0, 0.0);
        solid.thirdPoint = DRW_Coord(35.0, 36.0, 0.0);
        solid.fourPoint = DRW_Coord(33.0, 36.0, 0.0);
        wroteSolid_ = writer_->writeSolid(&solid) && solid.handle != 0;

        DRW_3Dface face;
        face.basePoint = DRW_Coord(37.0, 38.0, 0.0);
        face.secPoint = DRW_Coord(39.0, 38.0, 0.0);
        face.thirdPoint = DRW_Coord(39.0, 40.0, 0.0);
        face.fourPoint = DRW_Coord(37.0, 40.0, 0.0);
        wrote3dFace_ = writer_->write3dface(&face) && face.handle != 0;

        DRW_Ray ray;
        ray.basePoint = DRW_Coord(41.0, 42.0, 0.0);
        ray.secPoint = DRW_Coord(43.0, 44.0, 0.0);
        wroteRay_ = writer_->writeRay(&ray) && ray.handle != 0;

        DRW_Xline xline;
        xline.basePoint = DRW_Coord(45.0, 46.0, 0.0);
        xline.secPoint = DRW_Coord(47.0, 48.0, 0.0);
        wroteXline_ = writer_->writeXline(&xline) && xline.handle != 0;

        DRW_3DLine line3d;
        line3d.basePoint = DRW_Coord(49.0, 50.0, 51.0);
        line3d.secPoint = DRW_Coord(52.0, 53.0, 54.0);
        wrote3dLine_ = writer_->write3DLine(&line3d) && line3d.handle != 0;

        DRW_Polyline oldPolyline;
        oldPolyline.vertexcount = 2;
        oldPolyline.addVertex(DRW_Vertex(55.0, 56.0, 0.0, 0.0));
        oldPolyline.addVertex(DRW_Vertex(57.0, 58.0, 0.0, 0.0));
        wroteOldPolyline_ = writer_->writePolyline(&oldPolyline)
            && oldPolyline.handle != 0;

        DRW_Spline spline;
        spline.m_scenario = 1;
        spline.degree = 2;
        spline.ncontrol = 3;
        spline.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        spline.normalVec = DRW_Coord(0.0, 0.0, 1.0);
        spline.controllist.push_back(
            std::make_shared<DRW_Coord>(59.0, 60.0, 0.0));
        spline.controllist.push_back(
            std::make_shared<DRW_Coord>(61.0, 62.0, 0.0));
        spline.controllist.push_back(
            std::make_shared<DRW_Coord>(63.0, 64.0, 0.0));
        wroteSpline_ = writer_->writeSpline(&spline) && spline.handle != 0;

        DRW_Helix helix;
        helix.handle = 0xEE00u;
        helix.m_scenario = 1;
        helix.degree = 2;
        helix.ncontrol = 3;
        helix.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        helix.normalVec = DRW_Coord(0.0, 0.0, 1.0);
        helix.controllist.push_back(
            std::make_shared<DRW_Coord>(73.0, 74.0, 0.0));
        helix.controllist.push_back(
            std::make_shared<DRW_Coord>(75.0, 76.0, 0.0));
        helix.controllist.push_back(
            std::make_shared<DRW_Coord>(77.0, 78.0, 0.0));
        helix.m_majorVersion = 1;
        helix.m_maintVersion = 2;
        helix.axisBasePt = DRW_Coord(70.0, 71.0, 0.0);
        helix.startPt = DRW_Coord(72.0, 73.0, 0.0);
        helix.axisVector = DRW_Coord(0.0, 0.0, 1.0);
        helix.radius = 4.5;
        helix.turns = 3.25;
        helix.turnHeight = 2.75;
        helix.handedness = true;
        helix.constraintType = 2;
        wroteHelix_ = registeredHelix_ && writer_->writeHelix(&helix)
            && helix.handle == 0xEE00u;
        DRW_Helix invalidHelix = helix;
        invalidHelix.handle = 0xEE01u;
        invalidHelix.radius = std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedHelix_ = !writer_->writeHelix(&invalidHelix)
            && invalidHelix.handle == 0xEE01u;

        if (expectedVersion_ < DRW::AC1018) {
            wroteCamera_ = false;
            rejectedMalformedCamera_ = true;
        } else {
            DRW_Camera camera;
            camera.handle = 0xEF00u;
            camera.m_viewHandle = 0;
            wroteCamera_ = registeredCamera_ && writer_->writeCamera(&camera)
                && camera.handle == 0xEF00u;
            DRW_Camera invalidCamera = camera;
            invalidCamera.handle = 0xEF01u;
            invalidCamera.reactorHandles.resize(1000001u);
            rejectedMalformedCamera_ = !writer_->writeCamera(&invalidCamera)
                && invalidCamera.handle == 0xEF01u;
        }

        if (expectedVersion_ < DRW::AC1027) {
            wroteGeoPositionMarker_ = false;
            rejectedMalformedGeoPositionMarker_ = true;
        } else {
            DRW_GeoPositionMarker marker;
            marker.handle = 0xF300u;
            marker.m_classVersion = 1;
            marker.m_position = DRW_Coord(81.0, 82.0, 83.0);
            marker.m_radius = 3.5;
            marker.m_notes = "LOCAL_GEO_MARKER";
            marker.m_landingGap = 0.75;
            marker.m_mtextVisible = true;
            marker.m_textAlignment = 2;
            marker.m_enableFrameText = false;
            wroteGeoPositionMarker_ = writer_->writeGeoPositionMarker(&marker)
                && marker.handle == 0xF300u;
            DRW_GeoPositionMarker invalidMarker = marker;
            invalidMarker.handle = 0xF301u;
            invalidMarker.m_radius = std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedGeoPositionMarker_ =
                !writer_->writeGeoPositionMarker(&invalidMarker)
                && invalidMarker.handle == 0xF301u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteShape_ = false;
            rejectedMalformedShape_ = true;
        } else {
            DRW_Shape shape;
            shape.handle = 0xF400u;
            shape.m_insertionPoint = DRW_Coord(91.0, 92.0, 93.0);
            shape.m_scale = 2.5;
            shape.m_rotation = 0.25;
            shape.m_widthFactor = 0.8;
            shape.m_oblique = 0.1;
            shape.m_thickness = 0.2;
            shape.m_shapeIndex = 7;
            shape.m_extrusion = DRW_Coord(0.0, 0.0, 1.0);
            shape.m_shapeFileHandle = DRW::DwgStandardTextStyleHandle;
            shape.m_styleName = "STANDARD";
            wroteShape_ = writer_->writeShape(&shape)
                && shape.handle == 0xF400u;
            DRW_Shape invalidShape = shape;
            invalidShape.handle = 0xF401u;
            invalidShape.m_shapeFileHandle = 0;
            rejectedMalformedShape_ = !writer_->writeShape(&invalidShape)
                && invalidShape.handle == 0xF401u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteMLine_ = false;
            rejectedMalformedMLine_ = true;
        } else {
            DRW_MLine mline;
            mline.handle = 0xF500u;
            mline.scale = 1.5;
            mline.justification = 1;
            mline.basePoint = DRW_Coord(101.0, 102.0, 103.0);
            mline.extPoint = DRW_Coord(0.0, 0.0, 1.0);
            mline.openClosed = 1;
            mline.numLines = 1;
            mline.numVerts = 2;
            mline.styleHandle = 0xA800u;
            mline.styleName = "LOCAL_MLINESTYLE";
            DRW_MLineVertex first;
            first.position = DRW_Coord(104.0, 105.0, 106.0);
            first.vertexDir = DRW_Coord(1.0, 0.0, 0.0);
            first.miterDir = DRW_Coord(0.0, 1.0, 0.0);
            first.segParms = {{0.5}};
            first.areaFillParms = {{0.25}};
            mline.vertlist.push_back(first);
            DRW_MLineVertex second = first;
            second.position = DRW_Coord(107.0, 108.0, 109.0);
            mline.vertlist.push_back(second);
            wroteMLine_ = writer_->writeMLine(&mline)
                && mline.handle == 0xF500u;
            DRW_MLine invalidMLine = mline;
            invalidMLine.handle = 0xF501u;
            invalidMLine.scale = std::numeric_limits<double>::quiet_NaN();
            const bool rejectedNonFinite = !writer_->writeMLine(&invalidMLine)
                && invalidMLine.handle == 0xF501u;
            DRW_MLine countMismatchMLine = mline;
            countMismatchMLine.handle = 0xF502u;
            countMismatchMLine.numVerts = 1;
            const bool rejectedCountMismatch =
                !writer_->writeMLine(&countMismatchMLine)
                && countMismatchMLine.handle == 0xF502u;
            rejectedMalformedMLine_ = rejectedNonFinite && rejectedCountMismatch;
        }

        if (expectedVersion_ < DRW::AC1021) {
            wroteLight_ = false;
            rejectedMalformedLight_ = true;
        } else {
            DRW_Light light;
            light.handle = 0xF600u;
            light.m_classVersion = 1;
            light.m_name = "LOCAL_LIGHT";
            light.m_type = 1;
            light.m_status = true;
            light.m_color = 3;
            light.m_plotGlyph = false;
            light.m_intensity = 2.5;
            light.m_position = DRW_Coord(111.0, 112.0, 113.0);
            light.m_target = DRW_Coord(114.0, 115.0, 116.0);
            light.m_attenuationType = 1;
            light.m_useAttenuationLimits = true;
            light.m_attenuationStartLimit = 0.5;
            light.m_attenuationEndLimit = 12.5;
            light.m_hotspotAngle = 0.25;
            light.m_falloffAngle = 0.75;
            light.m_castShadows = true;
            light.m_shadowType = 1;
            light.m_shadowMapSize = 1024;
            light.m_shadowMapSoftness = 3;
            light.m_hasPhotometricData = true;
            light.m_hasWebFile = true;
            light.m_webFile = "LOCAL_LIGHT.IES";
            light.m_physicalIntensityMethod = 2;
            light.m_physicalIntensity = 4.5;
            light.m_illuminanceDistance = 6.5;
            light.m_lampColorType = 1;
            light.m_lampColorTemperature = 3500.0;
            light.m_lampColorPreset = 2;
            light.m_webRotation = DRW_Coord(0.0, 1.0, 0.0);
            light.m_extendedLightShape = 3;
            light.m_extendedLightLength = 1.25;
            light.m_extendedLightWidth = 2.25;
            light.m_extendedLightRadius = 3.25;
            wroteLight_ = registeredLight_ && writer_->writeLight(&light)
                && light.handle == 0xF600u;
            DRW_Light invalidLight = light;
            invalidLight.handle = 0xF601u;
            invalidLight.m_intensity =
                std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedLight_ = !writer_->writeLight(&invalidLight)
                && invalidLight.handle == 0xF601u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteMesh_ = false;
            rejectedMalformedMesh_ = true;
        } else {
            DRW_Mesh mesh;
            mesh.handle = 0xF700u;
            mesh.version = 2;
            mesh.blendCrease = true;
            mesh.subdivisionLevel = 1;
            mesh.vertices = {
                DRW_Coord(121.0, 122.0, 123.0),
                DRW_Coord(124.0, 122.0, 123.0),
                DRW_Coord(124.0, 125.0, 123.0),
                DRW_Coord(121.0, 125.0, 123.0),
            };
            mesh.faces = {{0, 1, 2, 3}};
            mesh.edges = {{0, 1}};
            mesh.creases = {0.5};
            mesh.unknown = 0;
            wroteMesh_ = registeredMesh_ && writer_->writeMesh(&mesh)
                && mesh.handle == 0xF700u;
            DRW_Mesh invalidMesh = mesh;
            invalidMesh.handle = 0xF701u;
            invalidMesh.vertices[0].x =
                std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedMesh_ = !writer_->writeMesh(&invalidMesh)
                && invalidMesh.handle == 0xF701u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteWipeout_ = false;
            rejectedMalformedWipeout_ = true;
        } else {
        DRW_Wipeout wipeout;
        wipeout.handle = 0xF800u;
        wipeout.m_classVersion = 0;
        wipeout.basePoint = DRW_Coord(131.0, 132.0, 133.0);
        wipeout.secPoint = DRW_Coord(0.0, 0.0, 1.0);
        wipeout.vVector = DRW_Coord(0.0, 1.0, 0.0);
        wipeout.sizeu = 64.0;
        wipeout.sizev = 48.0;
        wipeout.clip = 1;
        wipeout.brightness = 60;
        wipeout.contrast = 70;
        wipeout.fade = 10;
        wipeout.m_clipBoundaryType = 2;
        wipeout.clipPath = {
            DRW_Coord(0.0, 0.0, 0.0), DRW_Coord(64.0, 0.0, 0.0),
            DRW_Coord(64.0, 48.0, 0.0), DRW_Coord(0.0, 48.0, 0.0),
        };
        wipeout.clipMode = expectedVersion_ > DRW::AC1021;
        wroteWipeout_ = writer_->writeWipeout(&wipeout)
            && wipeout.handle == 0xF800u;
        DRW_Wipeout invalidWipeout = wipeout;
        invalidWipeout.handle = 0xF801u;
        invalidWipeout.m_clipBoundaryType = 1;
        rejectedMalformedWipeout_ = !writer_->writeWipeout(&invalidWipeout)
            && invalidWipeout.handle == 0xF801u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteNavisworksModel_ = false;
            rejectedMalformedNavisworksModel_ = true;
        } else {
        DRW_NavisworksModel navisworks;
        navisworks.handle = 0xF900u;
        navisworks.flags = 7;
        navisworks.definitionHandle = 0xD900u;
        navisworks.transform = {
            1.0, 0.0, 0.0, 10.0,
            0.0, 1.0, 0.0, 20.0,
            0.0, 0.0, 1.0, 30.0,
            0.0, 0.0, 0.0, 1.0,
        };
        navisworks.unitFactor = 2.5;
        wroteNavisworksModel_ = writer_->writeNavisworksModel(&navisworks)
            && navisworks.handle == 0xF900u;
        DRW_NavisworksModel invalidNavisworks = navisworks;
        invalidNavisworks.handle = 0xF901u;
        invalidNavisworks.unitFactor =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedNavisworksModel_ =
            !writer_->writeNavisworksModel(&invalidNavisworks)
            && invalidNavisworks.handle == 0xF901u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteUnderlay_ = false;
            rejectedMalformedUnderlay_ = true;
        } else {
            DRW_Underlay underlay;
            underlay.handle = 0xFA00u;
            underlay.kind = DRW_Underlay::PDF;
            underlay.position = DRW_Coord(141.0, 142.0, 143.0);
            underlay.scale = DRW_Coord(2.0, 3.0, 1.0);
            underlay.rotation = 0.25;
            underlay.extPoint = DRW_Coord(0.0, 0.0, 1.0);
            underlay.flags = 2;
            underlay.contrast = 80;
            underlay.fade = 5;
            underlay.definitionHandle = 0xD300u;
            underlay.clipBoundary = {
                DRW_Coord(0.0, 0.0, 0.0), DRW_Coord(64.0, 0.0, 0.0),
                DRW_Coord(64.0, 48.0, 0.0), DRW_Coord(0.0, 48.0, 0.0),
            };
            wroteUnderlay_ = writer_->writeUnderlay(&underlay)
                && underlay.handle == 0xFA00u;
            DRW_Underlay invalidUnderlay = underlay;
            invalidUnderlay.handle = 0xFA01u;
            invalidUnderlay.scale.x =
                std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedUnderlay_ = !writer_->writeUnderlay(&invalidUnderlay)
                && invalidUnderlay.handle == 0xFA01u;

            DRW_Underlay dgnUnderlay = underlay;
            dgnUnderlay.handle = 0xFA02u;
            dgnUnderlay.kind = DRW_Underlay::DGN;
            dgnUnderlay.definitionHandle = 0xD400u;
            wroteDgnUnderlayEntity_ = writer_->writeUnderlay(&dgnUnderlay)
                && dgnUnderlay.handle == 0xFA02u;

            DRW_Underlay dwfUnderlay = underlay;
            dwfUnderlay.handle = 0xFA04u;
            dwfUnderlay.kind = DRW_Underlay::DWF;
            dwfUnderlay.definitionHandle = 0xD500u;
            wroteDwfUnderlayEntity_ = writer_->writeUnderlay(&dwfUnderlay)
                && dwfUnderlay.handle == 0xFA04u;

            DRW_Underlay invalidDgnUnderlay = dgnUnderlay;
            invalidDgnUnderlay.handle = 0xFA03u;
            invalidDgnUnderlay.scale.x =
                std::numeric_limits<double>::quiet_NaN();
            const bool rejectedInvalidClip =
                !writer_->writeUnderlay(&invalidDgnUnderlay)
                && invalidDgnUnderlay.handle == 0xFA03u;
            rejectedMalformedUnderlay_ =
                rejectedMalformedUnderlay_ && rejectedInvalidClip;
        }

        // SURFACE DWG bodies are available from AC1021 onward.  Keep this
        // probe metadata-only (the writer emits an empty modeler body) so the
        // test does not depend on external ACIS/SAB fixtures while still
        // exercising every target variant's class registration, framing, and
        // callback route.
        if (expectedVersion_ < DRW::AC1021) {
            wroteSurfaceSet_ = false;
            rejectedMalformedSurface_ = true;
        } else {
            const auto registerSurfaceForWrite =
                [this](DRW_Surface& surface) {
                    return writer_->registerSurfaceEntityClass(&surface);
                };
            DRW_PlaneSurface plane;
            plane.handle = 0xFB00u;
            plane.uIsolines = 2;
            plane.vIsolines = 3;
            plane.modelerFormatVersion = 1;
            wrotePlaneSurface_ = registerSurfaceForWrite(plane)
                && writer_->writeSurface(&plane)
                && plane.handle == 0xFB00u;

            DRW_ExtrudedSurface extruded;
            extruded.handle = 0xFB01u;
            extruded.uIsolines = 4;
            extruded.vIsolines = 5;
            extruded.modelerFormatVersion = 1;
            extruded.sweepVector = DRW_Coord(0.0, 0.0, 1.0);
            extruded.draftAngle = 0.1;
            extruded.scaleFactor = 1.25;
            wroteExtrudedSurface_ = registerSurfaceForWrite(extruded)
                && writer_->writeSurface(&extruded)
                && extruded.handle == 0xFB01u;

            DRW_RevolvedSurface revolved;
            revolved.handle = 0xFB02u;
            revolved.uIsolines = 6;
            revolved.vIsolines = 7;
            revolved.classId = 2;
            revolved.id = 3;
            revolved.axisPoint = DRW_Coord(1.0, 2.0, 3.0);
            revolved.axisVector = DRW_Coord(0.0, 0.0, 1.0);
            revolved.revolveAngle = 1.5;
            revolved.startAngle = 0.25;
            wroteRevolvedSurface_ = registerSurfaceForWrite(revolved)
                && writer_->writeSurface(&revolved)
                && revolved.handle == 0xFB02u;

            DRW_SweptSurface swept;
            swept.handle = 0xFB03u;
            swept.uIsolines = 8;
            swept.vIsolines = 9;
            swept.classVersion = 2;
            swept.sweepEntityId = 4;
            swept.pathEntityId = 5;
            swept.referenceVector = DRW_Coord(1.0, 0.0, 0.0);
            swept.scaleFactor = 1.1;
            wroteSweptSurface_ = registerSurfaceForWrite(swept)
                && writer_->writeSurface(&swept)
                && swept.handle == 0xFB03u;

            DRW_LoftedSurface lofted;
            lofted.handle = 0xFB04u;
            lofted.uIsolines = 10;
            lofted.vIsolines = 11;
            lofted.modelerFormatVersion = 1;
            lofted.planeNormalLoftingType = 1;
            lofted.startDraftAngle = 0.2;
            lofted.endDraftAngle = 0.3;
            wroteLoftedSurface_ = registerSurfaceForWrite(lofted)
                && writer_->writeSurface(&lofted)
                && lofted.handle == 0xFB04u;

            DRW_NurbsSurface nurbs;
            nurbs.handle = 0xFB05u;
            nurbs.uIsolines = 12;
            nurbs.vIsolines = 13;
            nurbs.short170 = 14;
            nurbs.cvHullDisplay = true;
            nurbs.uvec1 = DRW_Coord(1.0, 0.0, 0.0);
            nurbs.vvec1 = DRW_Coord(0.0, 1.0, 0.0);
            nurbs.uvec2 = DRW_Coord(2.0, 0.0, 0.0);
            nurbs.vvec2 = DRW_Coord(0.0, 2.0, 0.0);
            wroteNurbsSurface_ = registerSurfaceForWrite(nurbs)
                && writer_->writeSurface(&nurbs)
                && nurbs.handle == 0xFB05u;

            DRW_PlaneSurface invalidSurface;
            invalidSurface.handle = 0xFB06u;
            invalidSurface.uIsolines = -1;
            const bool rejectedInvalidSurface =
                registerSurfaceForWrite(invalidSurface)
                && !writer_->writeSurface(&invalidSurface)
                && invalidSurface.handle == 0xFB06u;
            wroteSurfaceSet_ = wrotePlaneSurface_ && wroteExtrudedSurface_
                && wroteRevolvedSurface_ && wroteSweptSurface_
                && wroteLoftedSurface_ && wroteNurbsSurface_;
            rejectedMalformedSurface_ = rejectedInvalidSurface;
        }

        DRW_Hatch hatch;
        hatch.name = "SOLID";
        hatch.solid = 1;
        hatch.associative = 0;
        auto hatchLoop = std::make_shared<DRW_HatchLoop>(2);
        auto hatchBoundary = std::make_shared<DRW_LWPolyline>();
        hatchBoundary->flags = 1;
        hatchBoundary->addVertex(DRW_Vertex2D(65.0, 66.0, 0.0));
        hatchBoundary->addVertex(DRW_Vertex2D(67.0, 66.0, 0.0));
        hatchBoundary->addVertex(DRW_Vertex2D(67.0, 68.0, 0.0));
        hatchBoundary->addVertex(DRW_Vertex2D(65.0, 68.0, 0.0));
        hatchLoop->objlist.push_back(hatchBoundary);
        hatchLoop->update();
        hatch.appendLoop(hatchLoop);
        wroteHatch_ = writer_->writeHatch(&hatch) && hatch.handle != 0;

        DRW_Leader leader;
        leader.style = "STANDARD";
        leader.leadertype = 0;
        leader.flag = 3;
        leader.hookline = 1;
        leader.arrow = 1;
        leader.vertnum = 2;
        leader.vertexlist.push_back(
            std::make_shared<DRW_Coord>(69.0, 70.0, 0.0));
        leader.vertexlist.push_back(
            std::make_shared<DRW_Coord>(71.0, 72.0, 0.0));
        wroteLeader_ = writer_->writeLeader(&leader) && leader.handle != 0;

        DRW_Tolerance tolerance;
        tolerance.text = "LOCAL_TOLERANCE";
        tolerance.dimStyleName = "STANDARD";
        tolerance.insertionPoint = DRW_Coord(73.0, 74.0, 0.0);
        tolerance.xAxisDirectionVector = DRW_Coord(1.0, 0.0, 0.0);
        tolerance.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        wroteTolerance_ = writer_->writeTolerance(&tolerance)
            && tolerance.handle != 0;
        DRW_Tolerance invalidTolerance = tolerance;
        invalidTolerance.handle = 0xD920u;
        invalidTolerance.reactorHandles.resize(1000001u);
        rejectedMalformedTolerance_ =
            !writer_->writeTolerance(&invalidTolerance)
            && invalidTolerance.handle == 0xD920u;

        DRW_RText rtext;
        rtext.handle = 0xED00u;
        rtext.text = "LOCAL_RTEXT";
        rtext.basePoint = DRW_Coord(90.0, 91.0, 0.0);
        rtext.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        rtext.angle = 15.0;
        rtext.height = 2.0;
        rtext.m_rTextFlags = 1;
        wroteRText_ = registeredRText_ && writer_->writeRText(&rtext)
            && rtext.handle == 0xED00u;
        DRW_RText invalidRText = rtext;
        invalidRText.handle = 0xED02u;
        invalidRText.reactorHandles.resize(1000001u);
        rejectedMalformedRText_ =
            !writer_->writeRText(&invalidRText)
            && invalidRText.handle == 0xED02u;

        DRW_ArcAlignedText arcAlignedText;
        arcAlignedText.handle = 0xED01u;
        arcAlignedText.text = "LOCAL_ARC_TEXT";
        arcAlignedText.style = "STANDARD";
        arcAlignedText.m_center = DRW_Coord(100.0, 100.0, 0.0);
        arcAlignedText.m_radius = 10.0;
        arcAlignedText.m_startAngle = 0.25;
        arcAlignedText.m_endAngle = 1.25;
        arcAlignedText.m_textSize = "2";
        arcAlignedText.m_xScale = "1";
        arcAlignedText.m_charSpacing = "1";
        arcAlignedText.m_offsetFromArc = "0";
        arcAlignedText.m_rightOffset = "0";
        arcAlignedText.m_leftOffset = "0";
        arcAlignedText.m_fontName = "TXT";
        arcAlignedText.m_bigFontName = "";
        arcAlignedText.m_rawColor = 256;
        arcAlignedText.m_characterSet = 0;
        arcAlignedText.m_pitchAndFamily = 0;
        arcAlignedText.m_isShx = 0;
        arcAlignedText.m_isBold = 0;
        arcAlignedText.m_isItalic = 0;
        arcAlignedText.m_isUnderlined = 0;
        arcAlignedText.m_alignment = 0;
        arcAlignedText.m_isReverse = 0;
        arcAlignedText.m_wizardFlag = 0;
        arcAlignedText.m_textPosition = 0;
        arcAlignedText.m_textDirection = 0;
        wroteArcAlignedText_ = registeredArcAlignedText_
            && writer_->writeArcAlignedText(&arcAlignedText)
            && arcAlignedText.handle == 0xED01u;
        DRW_ArcAlignedText invalidArcAlignedText = arcAlignedText;
        invalidArcAlignedText.handle = 0xED03u;
        invalidArcAlignedText.reactorHandles.resize(1000001u);
        rejectedMalformedArcAlignedText_ =
            !writer_->writeArcAlignedText(&invalidArcAlignedText)
            && invalidArcAlignedText.handle == 0xED03u;

        DRW_Insert insert;
        insert.name = "LOCAL_BLOCK";
        insert.basePoint = DRW_Coord(84.0, 85.0, 0.0);
        auto attribute = std::make_shared<DRW_Attrib>();
        attribute->tag = "LOCAL_TAG";
        attribute->text = "LOCAL_VALUE";
        attribute->basePoint = DRW_Coord(84.0, 85.0, 0.0);
        attribute->height = 1.0;
        insert.attlist.push_back(attribute);
        wroteInsert_ = writer_->writeInsert(&insert) && insert.handle != 0;
        wroteAttrib_ = wroteInsert_ && insert.attlist.size() == 1
            && insert.attlist.front()->handle != 0
            && insert.seqendH.ref != DRW::NoHandle;

        DRW_PointCloud pointCloud;
        pointCloud.handle = 0xD925u;
        pointCloud.classVersion = 2;
        pointCloud.origin = DRW_Coord{1.0, 2.0, 3.0};
        pointCloud.savedFilename = "LOCAL_POINTCLOUD.rcs";
        pointCloud.sourceFileCount = 0;
        pointCloud.extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
        pointCloud.extentsMax = DRW_Coord{10.0, 20.0, 30.0};
        pointCloud.pointCount = 1234;
        pointCloud.ucsName = "LOCAL_POINTCLOUD_UCS";
        pointCloud.ucsOrigin = DRW_Coord{4.0, 5.0, 6.0};
        pointCloud.definitionHandle = 0xD600u;
        pointCloud.reactorHandle = 0xD602u;
        pointCloud.showIntensity = false;
        pointCloud.showClipping = false;
        if (writer_->getVersion() <= DRW::AC1018) {
            wrotePointCloud_ = false;
            rejectedMalformedPointCloudEntity_ = true;
        } else {
            wrotePointCloud_ = writer_->writePointCloud(&pointCloud)
                && pointCloud.handle != 0;
            DRW_PointCloud invalidPointCloudEntity = pointCloud;
            invalidPointCloudEntity.origin.x =
                std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedPointCloudEntity_ =
                !writer_->writePointCloud(&invalidPointCloudEntity);
        }

        DRW_PointCloudEx pointCloudEx;
        pointCloudEx.handle = 0xD926u;
        pointCloudEx.classVersion = 3;
        pointCloudEx.extentsMin = DRW_Coord{-10.0, -20.0, -30.0};
        pointCloudEx.extentsMax = DRW_Coord{100.0, 200.0, 300.0};
        pointCloudEx.ucsOrigin = DRW_Coord{0.0, 0.0, 0.0};
        pointCloudEx.name = "LOCAL_POINTCLOUD_EX";
        pointCloudEx.definitionHandle = 0xD601u;
        pointCloudEx.reactorHandle = 0xD603u;
        pointCloudEx.stylizationType = 1;
        pointCloudEx.intensityColorScheme = "LOCAL_INTENSITY";
        pointCloudEx.currentColorScheme = "LOCAL_CURRENT";
        pointCloudEx.classificationColorScheme = "LOCAL_CLASSIFICATION";
        pointCloudEx.elevationMin = -5.0;
        pointCloudEx.elevationMax = 50.0;
        pointCloudEx.intensityMin = 1.0;
        pointCloudEx.intensityMax = 255.0;
        pointCloudEx.intensityOutOfRangeBehavior = 2;
        pointCloudEx.elevationOutOfRangeBehavior = 3;
        if (writer_->getVersion() <= DRW::AC1024) {
            wrotePointCloudEx_ = false;
            rejectedMalformedPointCloudEx_ = true;
        } else {
            wrotePointCloudEx_ = writer_->writePointCloudEx(&pointCloudEx)
                && pointCloudEx.handle != 0;
            DRW_PointCloudEx invalidPointCloudEx = pointCloudEx;
            invalidPointCloudEx.extentsMax.z =
                std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedPointCloudEx_ =
                !writer_->writePointCloudEx(&invalidPointCloudEx);
        }
    }

    void addLine(const DRW_Line& data) override {
        readLineSeen_ = true;
        // The user block is read before model space, so retain the canonical
        // model-space line used by the existing geometry assertion.
        if (data.basePoint.x == 1.0 && data.basePoint.y == 2.0
            && data.basePoint.z == 3.0) {
            readLine_ = data;
            modelSpaceLineHandle_ = data.handle;
        }
    }
    void addPoint(const DRW_Point&) override { readPointSeen_ = true; }
    void addCircle(const DRW_Circle&) override { readCircleSeen_ = true; }
    void addArc(const DRW_Arc&) override { readArcSeen_ = true; }
    void addLWPolyline(const DRW_LWPolyline&) override {
        readPolylineSeen_ = true;
    }
    void addText(const DRW_Text& data) override {
        readTextSeen_ = true;
        if (data.text == "LOCAL_RTEXT") {
            const auto* rtext = dynamic_cast<const DRW_RText*>(&data);
            readRTextSeen_ = rtext != nullptr
                && rtext->m_rTextFlags == 1
                && data.basePoint.x == 90.0
                && data.basePoint.y == 91.0
                && data.height == 2.0;
        }
        if (data.text == "LOCAL_ARC_TEXT") {
            const auto* arcAligned =
                dynamic_cast<const DRW_ArcAlignedText*>(&data);
            readArcAlignedTextSeen_ = arcAligned != nullptr
                && arcAligned->m_center.x == 100.0
                && arcAligned->m_center.y == 100.0
                && arcAligned->m_radius == 10.0
                && arcAligned->m_textSize == "2"
                && data.height == 2.0;
        }
    }
    void addMText(const DRW_MText&) override { readMTextSeen_ = true; }
    void addEllipse(const DRW_Ellipse&) override { readEllipseSeen_ = true; }
    void addTrace(const DRW_Trace&) override { readTraceSeen_ = true; }
    void addSolid(const DRW_Solid&) override { readSolidSeen_ = true; }
    void add3dFace(const DRW_3Dface&) override { read3dFaceSeen_ = true; }
    void addRay(const DRW_Ray&) override { readRaySeen_ = true; }
    void addXline(const DRW_Xline&) override { readXlineSeen_ = true; }
    void add3DLine(const DRW_3DLine&) override { read3dLineSeen_ = true; }
    void addPointCloud(const DRW_PointCloud* data) override {
        if (data != nullptr)
            readPointCloudSeen_ = data->classVersion == 2
                && data->handle == 0xD925u
                && data->origin.x == 1.0
                && data->origin.z == 3.0
                && data->savedFilename == "LOCAL_POINTCLOUD.rcs"
                && data->sourceFileCount == 0
                && data->pointCount == 1234
                && data->extentsMin.x == -1.0
                && data->extentsMax.z == 30.0
                && data->ucsName == "LOCAL_POINTCLOUD_UCS"
                && data->ucsOrigin.x == 4.0
                && (expectedVersion_ <= DRW::AC1024
                    ? (data->definitionHandle == 0
                       && data->reactorHandle == 0)
                    : (data->definitionHandle == 0xD600u
                       && data->reactorHandle == 0xD602u))
                && !data->showIntensity && !data->showClipping;
    }
    void addPointCloudEx(const DRW_PointCloudEx* data) override {
        if (data != nullptr)
            readPointCloudExSeen_ = data->classVersion == 3
                && data->handle == 0xD926u
                && data->name == "LOCAL_POINTCLOUD_EX"
                && data->extentsMin.x == -10.0
                && data->extentsMax.z == 300.0
                && data->ucsOrigin.x == 0.0
                && data->definitionHandle == 0xD601u
                && data->reactorHandle == 0xD603u
                && data->stylizationType == 1
                && data->intensityColorScheme == "LOCAL_INTENSITY"
                && data->currentColorScheme == "LOCAL_CURRENT"
                && data->classificationColorScheme == "LOCAL_CLASSIFICATION"
                && data->elevationMin == -5.0
                && data->elevationMax == 50.0
                && data->intensityMin == 1.0
                && data->intensityMax == 255.0
                && data->intensityOutOfRangeBehavior == 2
                && data->elevationOutOfRangeBehavior == 3;
    }
    void addImage(const DRW_Image* data) override {
        if (data != nullptr && data->sizeu == 64.0 && data->sizev == 48.0)
            readImageSeen_ = data->ref != 0
                && data->m_imageDefReactorHandle != 0
                && data->sizeu == 64.0 && data->sizev == 48.0
                && data->brightness == 60 && data->contrast == 70
                && data->fade == 10;
        if (readImageSeen_ && data != nullptr)
            readImageHandle_ = data->handle;
    }
    void linkImage(const DRW_ImageDef* data) override {
        if (data != nullptr && data->name == "LOCAL_IMAGE.png")
            readImageDefSeen_ = data->u == 64.0 && data->v == 48.0
                && data->up == 1.0 && data->vp == 1.0
                && data->loaded == 1 && data->resolution == 0;
    }
    void addImageDefinitionReactor(
        const DRW_ImageDefinitionReactor& data) override {
        if (data.parentHandle == readImageHandle_ && readImageHandle_ != 0)
            readImageReactorSeen_ = data.m_classVersion == 2;
    }
    void addPolyline(const DRW_Polyline&) override {
        readOldPolylineSeen_ = true;
    }
    void addSpline(const DRW_Spline* data) override {
        readSplineSeen_ = data != nullptr;
        if (readSplineSeen_ && expectedVersion_ > DRW::AC1024)
            readSplineR2013Fields_ = data->m_splineFlags1 == 0
                && data->m_knotParam == 15;
    }
    void addHelix(const DRW_Helix* data) override {
        if (data == nullptr || data->handle != 0xEE00u)
            return;
        readHelixSeen_ = data->m_majorVersion == 1
            && data->m_maintVersion == 2
            && data->axisBasePt.x == 70.0
            && data->startPt.y == 73.0
            && data->axisVector.z == 1.0
            && data->radius == 4.5
            && data->turns == 3.25
            && data->turnHeight == 2.75
            && data->handedness
            && data->constraintType == 2
            && data->controllist.size() == 3;
    }
    void addCamera(const DRW_Camera& data) override {
        if (data.handle == 0xEF00u && data.m_viewHandle == 0)
            readCameraSeen_ = true;
    }
    void addGeoPositionMarker(const DRW_GeoPositionMarker& data) override {
        if (data.handle == 0xF300u)
            readGeoPositionMarkerSeen_ =
                expectedVersion_ >= DRW::AC1027
                && data.m_classVersion == 1
                && data.m_position.x == 81.0
                && data.m_position.y == 82.0
                && data.m_position.z == 83.0
                && data.m_radius == 3.5
                && data.m_notes == "LOCAL_GEO_MARKER"
                && data.m_landingGap == 0.75
                && data.m_mtextVisible
                && data.m_textAlignment == 2
                && !data.m_enableFrameText
                && data.mtext == nullptr;
    }
    void addShape(const DRW_Shape& data) override {
        if (data.handle == 0xF400u)
            readShapeSeen_ = data.m_insertionPoint.x == 91.0
                && data.m_insertionPoint.y == 92.0
                && data.m_insertionPoint.z == 93.0
                && data.m_scale == 2.5
                && data.m_rotation == 0.25
                && data.m_widthFactor == 0.8
                && data.m_oblique == 0.1
                && data.m_thickness == 0.2
                && data.m_shapeIndex == 7
                && data.m_extrusion.z == 1.0
                && data.m_shapeFileHandle == DRW::DwgStandardTextStyleHandle
                && data.m_styleName == "STANDARD";
    }
    void addMLine(const DRW_MLine* data) override {
        if (data == nullptr || data->handle != 0xF500u)
            return;
        readMLineSeen_ = expectedVersion_ >= DRW::AC1018
            && data->scale == 1.5
            && data->justification == 1
            && data->basePoint.x == 101.0
            && data->basePoint.y == 102.0
            && data->basePoint.z == 103.0
            && data->numLines == 1
            && data->numVerts == 2
            && data->styleHandle == 0xA800u
            // Entities are published before OBJECTS, so the optional style
            // name lookup is unavailable during the normal DWG read pass.
            // Accept the unresolved name while requiring the stable handle.
            && (data->styleName.empty()
                || data->styleName == "LOCAL_MLINESTYLE")
            && data->vertlist.size() == 2
            && data->vertlist[0].position.x == 104.0
            && data->vertlist[1].position.z == 109.0
            && data->vertlist[0].segParms.size() == 1
            && data->vertlist[0].segParms[0].size() == 1
            && data->vertlist[0].segParms[0][0] == 0.5
            && data->vertlist[0].areaFillParms[0][0] == 0.25;
    }
    void addLight(const DRW_Light& data) override {
        if (data.handle != 0xF600u)
            return;
        readLightSeen_ = expectedVersion_ >= DRW::AC1021
            && data.m_classVersion == 1
            && data.m_name == "LOCAL_LIGHT"
            && data.m_type == 1
            && data.m_status
            && data.m_color == 3
            && !data.m_plotGlyph
            && data.m_intensity == 2.5
            && data.m_position.x == 111.0
            && data.m_position.y == 112.0
            && data.m_position.z == 113.0
            && data.m_target.x == 114.0
            && data.m_target.y == 115.0
            && data.m_target.z == 116.0
            && data.m_attenuationType == 1
            && data.m_useAttenuationLimits
            && data.m_attenuationStartLimit == 0.5
            && data.m_attenuationEndLimit == 12.5
            && data.m_hotspotAngle == 0.25
            && data.m_falloffAngle == 0.75
            && data.m_castShadows
            && data.m_shadowType == 1
            && data.m_shadowMapSize == 1024
            && data.m_shadowMapSoftness == 3
            && data.m_hasPhotometricData
            && data.m_hasWebFile
            && data.m_webFile == "LOCAL_LIGHT.IES"
            && data.m_physicalIntensityMethod == 2
            && data.m_physicalIntensity == 4.5
            && data.m_illuminanceDistance == 6.5
            && data.m_lampColorType == 1
            && data.m_lampColorTemperature == 3500.0
            && data.m_lampColorPreset == 2
            && data.m_webRotation.y == 1.0
            && data.m_extendedLightShape == 3
            && data.m_extendedLightLength == 1.25
            && data.m_extendedLightWidth == 2.25
            && data.m_extendedLightRadius == 3.25;
    }
    void addMesh(const DRW_Mesh& data) override {
        if (data.handle != 0xF700u)
            return;
        readMeshSeen_ = expectedVersion_ >= DRW::AC1018
            && data.version == 2
            && data.blendCrease
            && data.subdivisionLevel == 1
            && data.vertices.size() == 4
            && data.vertices[0].x == 121.0
            && data.vertices[3].y == 125.0
            && data.faces.size() == 1
            && data.faces.front() == std::vector<std::int32_t>({0, 1, 2, 3})
            && data.edges.size() == 1
            && data.edges.front() == std::make_pair<std::int32_t, std::int32_t>(0, 1)
            && data.creases.size() == 1
            && data.creases.front() == 0.5
            && data.unknown == 0;
    }
    void addWipeout(const DRW_Wipeout* data) override {
        if (data == nullptr || data->handle != 0xF800u)
            return;
        readWipeoutSeen_ = expectedVersion_ >= DRW::AC1018
            && data->m_classVersion == 0
            && data->basePoint.x == 131.0
            && data->basePoint.y == 132.0
            && data->basePoint.z == 133.0
            && data->secPoint.z == 1.0
            && data->vVector.y == 1.0
            && data->sizeu == 64.0
            && data->sizev == 48.0
            && data->clip == 1
            && data->brightness == 60
            && data->contrast == 70
            && data->fade == 10
            && data->m_clipBoundaryType == 2
            && data->clipPath.size() == 4
            && data->clipPath[2].x == 64.0
            && data->clipPath[2].y == 48.0
            && data->clipMode == (expectedVersion_ > DRW::AC1021);
    }
    void addNavisworksModel(const DRW_NavisworksModel* data) override {
        if (data == nullptr || data->handle != 0xF900u)
            return;
        readNavisworksModelSeen_ = expectedVersion_ >= DRW::AC1018
            && data->flags == 7
            && data->definitionHandle == 0xD900u
            && data->transform[0] == 1.0
            && data->transform[3] == 10.0
            && data->transform[7] == 20.0
            && data->transform[11] == 30.0
            && data->transform[15] == 1.0
            && data->unitFactor == 2.5;
    }
    void addUnderlay(const DRW_Underlay* data) override {
        if (data == nullptr)
            return;
        if (data->handle == 0xFA00u)
            readUnderlaySeen_ = expectedVersion_ >= DRW::AC1018
            && data->kind == DRW_Underlay::PDF
            && data->position.x == 141.0
            && data->position.y == 142.0
            && data->position.z == 143.0
            && data->scale.x == 2.0
            && data->scale.y == 3.0
            && data->scale.z == 1.0
            && data->rotation == 0.25
            && data->extPoint.z == 1.0
            && data->flags == 2
            && data->contrast == 80
            && data->fade == 5
            && data->definitionHandle == 0xD300u
            && data->clipBoundary.size() == 4
            && data->clipBoundary[2].x == 64.0
            && data->clipBoundary[2].y == 48.0;
        if (data->handle == 0xFA02u)
            readDgnUnderlayEntitySeen_ = data->kind == DRW_Underlay::DGN
                && data->definitionHandle == 0xD400u
                && data->clipBoundary.size() == 4;
        if (data->handle == 0xFA04u)
            readDwfUnderlayEntitySeen_ = data->kind == DRW_Underlay::DWF
                && data->definitionHandle == 0xD500u
                && data->clipBoundary.size() == 4;
    }
    void addSurface(const DRW_Surface* data) override {
        if (data == nullptr)
            return;
        if (data->handle == 0xFB00u) {
            const auto* plane = dynamic_cast<const DRW_PlaneSurface*>(data);
            readPlaneSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && plane != nullptr && data->eType == DRW::PLANESURFACE
                && data->dwgClassNum != 0
                && data->uIsolines == 2 && data->vIsolines == 3;
        }
        if (data->handle == 0xFB01u) {
            const auto* extruded =
                dynamic_cast<const DRW_ExtrudedSurface*>(data);
            readExtrudedSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && extruded != nullptr
                && data->eType == DRW::EXTRUDEDSURFACE
                && data->dwgClassNum != 0
                && data->uIsolines == 4 && data->vIsolines == 5
                && extruded->sweepVector.z == 1.0
                && extruded->draftAngle == 0.1;
        }
        if (data->handle == 0xFB02u) {
            const auto* revolved =
                dynamic_cast<const DRW_RevolvedSurface*>(data);
            readRevolvedSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && revolved != nullptr
                && data->eType == DRW::REVOLVEDSURFACE
                && data->dwgClassNum != 0
                && revolved->classId == 2 && revolved->id == 3
                && revolved->axisPoint.x == 1.0
                && revolved->axisVector.z == 1.0
                && revolved->revolveAngle == 1.5;
        }
        if (data->handle == 0xFB03u) {
            const auto* swept = dynamic_cast<const DRW_SweptSurface*>(data);
            readSweptSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && swept != nullptr && data->eType == DRW::SWEPTSURFACE
                && data->dwgClassNum != 0
                && data->uIsolines == 8 && data->vIsolines == 9
                && swept->classVersion == 2
                && swept->sweepEntityId == 4
                && swept->pathEntityId == 5
                && swept->scaleFactor == 1.1;
        }
        if (data->handle == 0xFB04u) {
            const auto* lofted = dynamic_cast<const DRW_LoftedSurface*>(data);
            readLoftedSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && lofted != nullptr && data->eType == DRW::LOFTEDSURFACE
                && data->dwgClassNum != 0
                && data->uIsolines == 10 && data->vIsolines == 11
                && lofted->modelerFormatVersion == 1
                && lofted->planeNormalLoftingType == 1
                && lofted->startDraftAngle == 0.2;
        }
        if (data->handle == 0xFB05u) {
            const auto* nurbs = dynamic_cast<const DRW_NurbsSurface*>(data);
            readNurbsSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && nurbs != nullptr && data->eType == DRW::NURBSURFACE
                && data->dwgClassNum != 0
                && data->uIsolines == 12 && data->vIsolines == 13
                && (expectedVersion_ < DRW::AC1027
                    ? (nurbs->short170 == 0 && !nurbs->cvHullDisplay)
                    : (nurbs->short170 == 14 && nurbs->cvHullDisplay
                       && nurbs->uvec1.x == 1.0 && nurbs->vvec1.y == 1.0
                       && nurbs->uvec2.x == 2.0 && nurbs->vvec2.y == 2.0));
        }
    }
    void addHatch(const DRW_Hatch*) override { readHatchSeen_ = true; }
    void addLeader(const DRW_Leader*) override { readLeaderSeen_ = true; }
    void addTolerance(const DRW_Tolerance& data) override {
        if (data.text == "LOCAL_TOLERANCE")
            readToleranceSeen_ = data.dimStyleH.ref != 0
                && data.insertionPoint.x == 73.0
                && data.insertionPoint.y == 74.0
                && data.extPoint.z == 1.0
                && data.xAxisDirectionVector.x == 1.0;
    }
    void addDimensionAssociation(
        const DRW_DimensionAssociation& data) override {
        if (data.handle == 0xF000u)
            readDimensionAssociationSeen_ =
                expectedVersion_ >= DRW::AC1021
                && data.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
                && data.m_dimensionHandle == modelSpaceLineHandle_
                && data.m_associativityFlags == 1
                && data.m_osnapRefs.size() == 1
                && data.m_osnapRefs.front().m_className == "AcDbLine"
                && data.m_osnapRefs.front().m_objectHandle
                    == modelSpaceLineHandle_
                && !data.m_hasUnrepresentableDetail;
    }
    void addEvaluationGraph(const DRW_EvaluationGraph& data) override {
        if (data.handle == 0xF100u)
            readEvaluationGraphSeen_ =
                expectedVersion_ >= DRW::AC1021
                && data.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
                && data.m_value96 == 96 && data.m_value97 == 97
                && data.m_nodes.size() == 1
                && data.m_nodes.front().m_index == 1
                && data.m_nodes.front().m_expressionHandle
                    == modelSpaceLineHandle_
                && data.m_edges.size() == 1
                && data.m_edges.front().m_value92 == 92
                && data.m_edges.front().m_value92e == 925;
    }
    void addBlockRepresentationData(
        const DRW_BlockRepresentationData& data) override {
        if (data.handle == 0xF200u)
            readBlockRepresentationDataSeen_ =
                data.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
                && data.m_flag == 7
                && data.m_blockHandle == modelSpaceLineHandle_;
    }
    void addGroup(const DRW_Group& data) override {
        readGroupSeen_ = data.m_entityHandles.size() == 1
            && data.m_description == "LOCAL_GROUP";
    }
    void addUnsupportedObject(const DRW_UnsupportedObject& data) override {
        if (data.m_isEntity)
            return;
        readRawObjectInvariant_ = readRawObjectInvariant_
            && data.m_version == expectedVersion_
            && data.m_objectSize == data.m_rawBytes.size()
            && static_cast<std::uint64_t>(data.m_bodyBitSize)
                <= static_cast<std::uint64_t>(data.m_rawBytes.size()) * 8u;
        if (!isLocalObjectHandle(data.m_handle))
            return;
        if (std::find(readLocalRawObjectHandles_.begin(),
                      readLocalRawObjectHandles_.end(), data.m_handle)
            != readLocalRawObjectHandles_.end()) {
            readRawObjectInvariant_ = false;
            return;
        }
        readLocalRawObjectHandles_.push_back(data.m_handle);
    }
    void addDictionary(const DRW_Dictionary& data) override {
        if (data.handle == 0xA601u) {
            readDictionarySeen_ = data.parentHandle
                    == DRW::DwgNamedObjectsDictionaryHandle
                && data.m_entries.size()
                    == (expectedVersion_ >= DRW::AC1021 ? 55u : 53u)
                && data.m_entries[0].m_name == "LOCAL_XRECORD"
                && data.m_entries[0].m_handle == 0xA602u
                && data.m_entries[1].m_name == "LOCAL_PLOTSETTINGS"
                && data.m_entries[1].m_handle == 0xA603u
                && data.m_entries[2].m_name == "LOCAL_LAYOUT"
                && data.m_entries[2].m_handle == 0xA700u
                && data.m_entries[3].m_name == "LOCAL_MLINESTYLE"
                && data.m_entries[3].m_handle == 0xA800u
                && data.m_entries[4].m_name == "LOCAL_MLEADERSTYLE"
                && data.m_entries[4].m_handle == 0xA900u
                && data.m_entries[5].m_name == "LOCAL_DICTIONARYVAR"
                && data.m_entries[5].m_handle == 0xB000u
                && data.m_entries[6].m_name == "LOCAL_DICTIONARYWDFLT"
                && data.m_entries[6].m_handle == 0xB100u
                && data.m_entries[7].m_name == "LOCAL_SORTENTSTABLE"
                && data.m_entries[7].m_handle == 0xB200u
                && data.m_entries[8].m_name == "LOCAL_FIELDLIST"
                && data.m_entries[8].m_handle == 0xB300u
                && data.m_entries[9].m_name == "LOCAL_FIELD"
                && data.m_entries[9].m_handle == 0xB400u
                && data.m_entries[10].m_name == "LOCAL_RASTERVARIABLES"
                && data.m_entries[10].m_handle == 0xB500u
                && data.m_entries[11].m_name == "LOCAL_WIPEOUTVARIABLES"
                && data.m_entries[11].m_handle == 0xB600u
                && data.m_entries[12].m_name == "LOCAL_VISUALSTYLE"
                && data.m_entries[12].m_handle == 0xC000u
                && data.m_entries[13].m_name == "LOCAL_RENDERSETTINGS"
                && data.m_entries[13].m_handle == 0xC100u
                && data.m_entries[14].m_name == "LOCAL_RENDER_ENVIRONMENT"
                && data.m_entries[14].m_handle == 0xC200u
                && data.m_entries[15].m_name == "LOCAL_RENDER_GLOBAL"
                && data.m_entries[15].m_handle == 0xC300u
                && data.m_entries[16].m_name == "LOCAL_RENDER_ENTRY"
                && data.m_entries[16].m_handle == 0xC400u
                && data.m_entries[17].m_name == "LOCAL_RENDER_RAPIDRT"
                && data.m_entries[17].m_handle == 0xC600u
                && data.m_entries[18].m_name == "LOCAL_RENDER_MENTALRAY"
                && data.m_entries[18].m_handle == 0xC700u
                && data.m_entries[19].m_name == "LOCAL_MATERIAL"
                && data.m_entries[19].m_handle == 0xC800u
                && data.m_entries[20].m_name == "LOCAL_DBCOLOR"
                && data.m_entries[20].m_handle == 0xC900u
                && data.m_entries[21].m_name == "LOCAL_LIGHTLIST"
                && data.m_entries[21].m_handle == 0xCA00u
                && data.m_entries[22].m_name == "LOCAL_SCALE"
                && data.m_entries[22].m_handle == 0xCB00u
                && data.m_entries[23].m_name == "LOCAL_IDBUFFER"
                && data.m_entries[23].m_handle == 0xCC00u
                && data.m_entries[24].m_name == "LOCAL_LAYER_INDEX"
                && data.m_entries[24].m_handle == 0xCD00u
                && data.m_entries[25].m_name == "LOCAL_SPATIAL_INDEX"
                && data.m_entries[25].m_handle == 0xCE00u
                && data.m_entries[26].m_name == "LOCAL_TABLESTYLE"
                && data.m_entries[26].m_handle == 0xCF00u
                && data.m_entries[27].m_name == "LOCAL_SPATIAL_FILTER"
                && data.m_entries[27].m_handle == 0xD000u
                && data.m_entries[28].m_name == "LOCAL_GEODATA"
                && data.m_entries[28].m_handle == 0xD100u
                && data.m_entries[29].m_name == "LOCAL_GEODATA_V2"
                && data.m_entries[29].m_handle == 0xD200u
                && data.m_entries[30].m_name == "LOCAL_PDFDEFINITION"
                && data.m_entries[30].m_handle == 0xD300u
                && data.m_entries[31].m_name == "LOCAL_DGNDEFINITION"
                && data.m_entries[31].m_handle == 0xD400u
                && data.m_entries[32].m_name == "LOCAL_DWFDEFINITION"
                && data.m_entries[32].m_handle == 0xD500u
                && data.m_entries[33].m_name == "LOCAL_POINTCLOUDDEFINITION"
                && data.m_entries[33].m_handle == 0xD600u
                && data.m_entries[34].m_name == "LOCAL_POINTCLOUDDEFINITIONEX"
                && data.m_entries[34].m_handle == 0xD601u
                && data.m_entries[35].m_name == "LOCAL_POINTCLOUDCOLORMAP"
                && data.m_entries[35].m_handle == 0xD800u
                && data.m_entries[36].m_name == "LOCAL_NAVISWORKSMODELDEF"
                && data.m_entries[36].m_handle == 0xD900u
                && data.m_entries[37].m_name == "LOCAL_SUNSTUDY"
                && data.m_entries[37].m_handle == 0xDA00u
                && data.m_entries[38].m_name == "LOCAL_MOTIONPATH"
                && data.m_entries[38].m_handle == 0xDB00u
                && data.m_entries[39].m_name == "LOCAL_CURVEPATH"
                && data.m_entries[39].m_handle == 0xDC00u
                && data.m_entries[40].m_name == "LOCAL_POINTPATH"
                && data.m_entries[40].m_handle == 0xDD00u
                && data.m_entries[41].m_name == "LOCAL_OBJECT_PTR"
                && data.m_entries[41].m_handle == 0xDE00u
                && data.m_entries[42].m_name == "LOCAL_PARTIAL_VIEWING_INDEX"
                && data.m_entries[42].m_handle == 0xDF00u
                && data.m_entries[43].m_name == "LOCAL_SOLID_BACKGROUND"
                && data.m_entries[43].m_handle == 0xE000u
                && data.m_entries[44].m_name == "LOCAL_GRADIENT_BACKGROUND"
                && data.m_entries[44].m_handle == 0xE100u
                && data.m_entries[45].m_name == "LOCAL_GROUNDPLANE_BACKGROUND"
                && data.m_entries[45].m_handle == 0xE200u
                && data.m_entries[46].m_name == "LOCAL_IMAGE_BACKGROUND"
                && data.m_entries[46].m_handle == 0xE300u
                && data.m_entries[47].m_name == "LOCAL_IBL_BACKGROUND"
                && data.m_entries[47].m_handle == 0xE400u
                && data.m_entries[48].m_name == "LOCAL_SKYLIGHT_BACKGROUND"
                && data.m_entries[48].m_handle == 0xE500u
                && data.m_entries[49].m_name == "LOCAL_TVDEVICEPROPERTIES"
                && data.m_entries[49].m_handle == 0xEA00u
                && data.m_entries[50].m_name == "LOCAL_VXCONTROL"
                && data.m_entries[50].m_handle == 0xEB00u
                && data.m_entries[51].m_name == "LOCAL_VXTABLERECORD"
                && data.m_entries[51].m_handle == 0xEC00u
                && (expectedVersion_ < DRW::AC1021
                    || (data.m_entries[52].m_name == "LOCAL_SECTION_MANAGER"
                        && data.m_entries[52].m_handle == 0xE700u
                        && data.m_entries[53].m_name == "LOCAL_SECTION_SETTINGS"
                        && data.m_entries[53].m_handle == 0xE800u))
                && data.m_entries.back().m_name == "LOCAL_NULL_ENTRY"
                && data.m_entries.back().m_handle == 0u;
        }
    }
    void addXRecord(const DRW_XRecord& data) override {
        if (data.handle == 0xA602u)
            readXRecordSeen_ = !data.m_values.empty()
                && data.parentHandle == 0xA601u;
        if (data.handle == 0xA701u)
            readMalformedObjectSeen_ = true;
    }
    void addPlotSettings(const DRW_PlotSettings* data) override {
        if (data != nullptr && data->handle == 0xA603u)
            readPlotSettingsSeen_ = data->parentHandle == 0xA601u
                && data->pageSetupName == "LOCAL_PAGE"
                && data->printerConfig == "LOCAL_PRINTER"
                && data->plotLayoutFlags == 1
                && data->marginLeft == 1.0
                && data->marginBottom == 2.0
                && data->marginRight == 3.0
                && data->marginTop == 4.0
                && data->paperWidth == 210.0
                && data->paperHeight == 297.0
                && data->paperSize == "A4"
                && data->plotOriginX == 0.0
                && data->plotOriginY == 0.0
                && data->paperUnits == 1
                && data->plotRotation == 0
                && data->plotType == 0
                && data->windowMinX == -10.0
                && data->windowMinY == -20.0
                && data->windowMaxX == 10.0
                && data->windowMaxY == 20.0
                && (expectedVersion_ >= DRW::AC1012
                        && expectedVersion_ <= DRW::AC1015
                    ? data->plotViewName == "LOCAL_VIEW"
                    : data->plotViewName.empty())
                && data->realWorldUnits == 1.0
                && data->drawingUnits == 1.0
                && data->currentStyleSheet == "LOCAL_STYLE"
                && data->scaleType == 1
                && data->scaleFactor == 1.0
                && data->paperImageOriginX == 0.0
                && data->paperImageOriginY == 0.0
                && data->shadePlotMode == (expectedVersion_ >= DRW::AC1018
                    ? 1 : 0)
                && data->shadePlotResLevel == (expectedVersion_ >= DRW::AC1018
                    ? 2 : 0)
                && data->shadePlotCustomDPI == (expectedVersion_ >= DRW::AC1018
                    ? 300 : 0);
    }
    void addLayout(const DRW_Layout& data) override {
        if (data.handle == 0xA700u)
            readLayoutSeen_ = data.name == "LOCAL_LAYOUT"
                && data.parentHandle == 0xA601u
                && data.pageSetupName == "LOCAL_LAYOUT_PAGE"
                && data.printerConfig == "LOCAL_LAYOUT_PRINTER"
                && data.plotLayoutFlags == 0
                && data.marginLeft == 1.0
                && data.marginBottom == 2.0
                && data.marginRight == 3.0
                && data.marginTop == 4.0
                && data.paperWidth == 210.0
                && data.paperHeight == 297.0
                && data.paperSize == "A4"
                && data.plotOriginX == 0.0
                && data.plotOriginY == 0.0
                && data.paperUnits == 0
                && data.plotRotation == 0
                && data.plotType == 0
                && data.windowMinX == 0.0
                && data.windowMinY == 0.0
                && data.windowMaxX == 0.0
                && data.windowMaxY == 0.0
                // The local LAYOUT writer leaves the legacy plot-view name
                // unset; newer versions omit it from the body altogether.
                && data.plotViewName.empty()
                && data.realWorldUnits == 1.0
                && data.drawingUnits == 1.0
                && data.currentStyleSheet.empty()
                && data.scaleType == 0
                && data.scaleFactor == 1.0
                && data.paperImageOriginX == 0.0
                && data.paperImageOriginY == 0.0
                && data.shadePlotMode == 0
                && data.shadePlotResLevel == 0
                && data.shadePlotCustomDPI == 0
                && data.tabOrder == 1
                && data.layoutFlags == 1
                && data.ucsOrigin.x == 0.0 && data.ucsOrigin.y == 0.0
                && data.ucsOrigin.z == 0.0
                && data.limMinX == 0.0 && data.limMinY == 0.0
                && data.limMaxX == 0.0 && data.limMaxY == 0.0
                && data.insPoint.x == 0.0 && data.insPoint.y == 0.0
                && data.insPoint.z == 0.0
                && data.ucsXAxis.x == 1.0 && data.ucsXAxis.y == 0.0
                && data.ucsXAxis.z == 0.0
                && data.ucsYAxis.x == 0.0 && data.ucsYAxis.y == 1.0
                && data.ucsYAxis.z == 0.0
                && data.elevation == 0.0
                && data.orthoViewType == 0
                && data.extMin.x == 0.0 && data.extMin.y == 0.0
                && data.extMin.z == 0.0
                && data.extMax.x == 100.0 && data.extMax.y == 100.0
                && data.extMax.z == 0.0
                && data.viewportCount == (expectedVersion_ >= DRW::AC1018 ? 1 : 0)
                && data.plotViewHandle.ref == (expectedVersion_ >= DRW::AC1018
                    ? 0xA710u : 0u)
                && data.shadePlotHandle.ref == (expectedVersion_ > DRW::AC1018
                    ? 0xA711u : 0u)
                && data.paperSpaceBlockRecordHandle.ref == 0xA712u
                && data.lastActiveViewportHandle.ref == 0xA713u
                && data.baseUcsHandle.ref == 0xA714u
                && data.namedUcsHandle.ref == 0xA715u
                && (expectedVersion_ >= DRW::AC1018
                    ? data.viewportHandles.size() == 1
                        && data.viewportHandles.front() == 0xA716u
                    : data.viewportHandles.empty());
    }
    void addMLineStyle(const DRW_MLineStyle& data) override {
        if (data.handle == 0xA800u)
            readMLineStyleSeen_ = data.name == "LOCAL_MLINESTYLE"
                && data.parentHandle == 0xA601u
                && data.elements.size() == 1
                && data.elements.front().offset == 0.5;
        if (data.handle == 0xA801u)
            readMalformedStyleSeen_ = true;
    }
    void addMLeaderStyle(const DRW_MLeaderStyle* data) override {
        if (data != nullptr && data->handle == 0xA900u)
            readMLeaderStyleSeen_ = data->parentHandle == 0xA601u
                && data->description == "LOCAL_MLEADERSTYLE_DESC"
                && data->contentType == 2
                && data->landingGap == 0.25
                && data->textDefault == "LOCAL_MLEADER_TEXT"
                && data->textHeight == 2.5;
        if (data != nullptr && data->handle == 0xA901u)
            readMalformedMLeaderStyleSeen_ = true;
    }
    void addDictionaryVar(const DRW_DictionaryVar& data) override {
        if (data.handle == 0xB000u)
            readDictionaryVarSeen_ = data.parentHandle == 0xA601u
                && data.m_schema == 7
                && data.m_value == "LOCAL_DICTIONARYVAR_VALUE";
        if (data.handle == 0xB001u)
            readMalformedDictionaryVarSeen_ = true;
    }
    void addDictionaryWithDefault(
        const DRW_DictionaryWithDefault& data) override {
        if (data.handle == 0xB100u)
            readDictionaryWithDefaultSeen_ = data.parentHandle == 0xA601u
                && data.m_entries.size() == 2
                && data.m_entries.front().m_name == "LOCAL_DEFAULT"
                && data.m_entries.front().m_handle == 0xB000u
                && data.m_entries.back().m_name == "LOCAL_NULL_DEFAULT_MEMBER"
                && data.m_entries.back().m_handle == 0u
                && data.m_defaultEntryHandle == 0xB000u;
        if (data.handle == 0xB101u)
            readMalformedDictionaryWithDefaultSeen_ = true;
    }
    void addSortEntsTable(const DRW_SortEntsTable& data) override {
        if (data.handle == 0xB200u)
            readSortEntsTableSeen_ = data.parentHandle
                    == DRW::DwgModelSpaceBlockRecordHandle
                && data.m_blockOwnerHandle
                    == DRW::DwgModelSpaceBlockRecordHandle
                && data.m_entityHandles.size() == 1
                && data.m_sortHandles.size() == 1
                && data.m_entityHandles.front() != DRW::NoHandle
                && data.m_entityHandles.front() == data.m_sortHandles.front();
        if (data.handle == 0xB201u)
            readMalformedSortEntsTableSeen_ = true;
    }
    void addFieldList(const DRW_FieldList& data) override {
        if (data.handle == 0xB300u)
            readFieldListSeen_ = data.parentHandle == 0xA601u
                && data.m_unknown == 0 && data.m_fieldHandles.size() == 1
                && data.m_fieldHandles.front() == 0xB400u;
        if (data.handle == 0xB301u)
            readMalformedFieldListSeen_ = true;
    }
    void addField(const DRW_Field& data) override {
        if (data.handle == 0xB400u)
            readFieldSeen_ = data.parentHandle == 0xA601u
                && data.m_evaluatorId == "ACAD"
                && data.m_fieldCode == "LOCAL_FIELD_CODE"
                && data.m_value.m_dataType == 1
                && data.m_value.m_value.type() == DRW_Variant::INTEGER
                && data.m_value.m_value.i_val() == 42;
        if (data.handle == 0xB401u)
            readMalformedFieldSeen_ = true;
    }
    void addRasterVariables(const DRW_RasterVariables& data) override {
        if (data.handle == 0xB500u)
            readRasterVariablesSeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 1
                && data.m_imageFrame == 1
                && data.m_imageQuality == 2
                && data.m_units == 3;
        if (data.handle == 0xB501u)
            readMalformedRasterVariablesSeen_ = true;
    }
    void addWipeoutVariables(const DRW_WipeoutVariables& data) override {
        if (data.handle == 0xB600u)
            readWipeoutVariablesSeen_ = data.parentHandle == 0xA601u
                && data.m_displayFrame == 1;
        if (data.handle == 0xB601u)
            readMalformedWipeoutVariablesSeen_ = true;
    }
    void addVisualStyle(const DRW_VisualStyle& data) override {
        if (data.handle == 0xC000u)
            readVisualStyleSeen_ = data.parentHandle == 0xA601u
                && data.desc == "LOCAL_VISUALSTYLE_DESC"
                && data.type == 1
                && data.m_body.faceLightingModel == 2
                && data.m_body.faceOpacity == 0.75
                && data.m_body.edgeModel == 1
                && data.m_body.edgeIsolines == 3
                && data.m_body.displaySettings == 4
                && data.m_bodyDecoded;
        if (data.handle == 0xC001u)
            readMalformedVisualStyleSeen_ = true;
    }
    void addRenderSettings(const DRW_RenderSettings& data) override {
        if (data.handle == 0xC100u)
            readRenderSettingsSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::Settings
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDERSETTINGS"
                && data.m_description == "LOCAL_RENDER_DESC";
        if (data.handle == 0xC101u)
            readMalformedRenderSettingsSeen_ = true;
        if (data.handle == 0xC200u)
            readRenderEnvironmentSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::Environment
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDER_ENVIRONMENT"
                && data.m_fogEnabled
                && !data.m_fogBackgroundEnabled
                && data.m_environmentImageEnabled
                && data.m_fogColorR == 10
                && data.m_fogColorG == 20
                && data.m_fogColorB == 30
                && data.m_fogDensityNear == 0.1
                && data.m_fogDensityFar == 0.9
                && data.m_fogDistanceNear == 2.0
                && data.m_fogDistanceFar == 3.0;
        if (data.handle == 0xC201u)
            readMalformedRenderEnvironmentSeen_ = true;
        if (data.handle == 0xC300u)
            readRenderGlobalSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::Global
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDER_GLOBAL"
                && data.m_procedure == 7
                && data.m_destination == 8;
        if (data.handle == 0xC301u)
            readMalformedRenderGlobalSeen_ = true;
        if (data.handle == 0xC400u)
            readRenderEntrySeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::Entry
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDER_ENTRY"
                && data.m_longs.size() >= 8
                && data.m_longs[1] == 11
                && data.m_longs[2] == 12
                && data.m_shorts.size() >= 6
                && data.m_shorts[0] == 1
                && data.m_shorts[5] == 6
                && data.m_doubles.size() == 1
                && data.m_doubles.front() == 1.5;
        if (data.handle == 0xC401u)
            readMalformedRenderEntrySeen_ = true;
        if (data.handle == 0xC600u)
            readRenderRapidSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::RapidRT
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDER_RAPIDRT"
                && data.m_longs.size() >= 8
                && data.m_longs[1] == 9
                && data.m_longs[7] == 7
                && data.m_doubles.size() == 2
                && data.m_doubles[0] == 0.25
                && data.m_doubles[1] == 0.75;
        if (data.handle == 0xC601u)
            readMalformedRenderRapidSeen_ = true;
        if (data.handle == 0xC700u)
            readRenderMentalSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::MentalRay
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDER_MENTALRAY"
                && data.m_shorts.size() >= 7
                && data.m_shorts[0] == 1
                && data.m_shorts[6] == 7
                && data.m_doubles.size() == 12
                && data.m_doubles[0] == 0.1
                && data.m_doubles[11] == 1.2;
        if (data.handle == 0xC701u)
            readMalformedRenderMentalSeen_ = true;
    }
    void addMaterial(const DRW_Material& data) override {
        if (data.handle == 0xC800u)
            readMaterialSeen_ = data.parentHandle == 0xA601u
                && data.m_name == "LOCAL_MATERIAL"
                && data.m_description == "LOCAL_MATERIAL_DESC";
        if (data.handle == 0xC801u)
            readMalformedMaterialSeen_ = true;
    }
    void addDbColor(const DRW_DbColor& data) override {
        if (data.handle == 0xC900u)
            readDbColorSeen_ = data.parentHandle == 0xA601u
                && data.rgb == 0x123456
                && data.colorMethod == dwgColor::RGB
                && data.name == "LOCAL_COLOR"
                && data.bookName == "LOCAL_BOOK";
        if (data.handle == 0xC901u)
            readMalformedDbColorSeen_ = true;
    }
    void addLightList(const DRW_LightList& data) override {
        if (data.handle == 0xCA00u)
            readLightListSeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 1
                && data.m_lightCount == 1
                && data.m_lights.size() == 1
                && data.m_lights.front().m_handle != DRW::NoHandle
                && data.m_lights.front().m_name == "LOCAL_LIGHT";
        if (data.handle == 0xCA01u)
            readMalformedLightListSeen_ = true;
    }
    void addScale(const DRW_Scale& data) override {
        if (data.handle == 0xCB00u)
            readScaleSeen_ = data.parentHandle == 0xA601u
                && data.flag == 0
                && data.name == "LOCAL_SCALE"
                && data.paperUnits == 1.0
                && data.drawingUnits == 48.0
                && !data.isUnitScale;
        if (data.handle == 0xCB01u)
            readMalformedScaleSeen_ = true;
    }
    void addIDBuffer(const DRW_IDBuffer& data) override {
        if (data.handle == 0xCC00u)
            readIDBufferSeen_ = data.parentHandle == 0xA601u
                && data.classVersion == 0
                && data.objIds.size() == 1
                && data.objIds.front() != DRW::NoHandle;
        if (data.handle == 0xCC01u)
            readMalformedIDBufferSeen_ = true;
    }
    void addLayerIndex(const DRW_LayerIndex& data) override {
        if (data.handle == 0xCD00u)
            readLayerIndexSeen_ = data.parentHandle == 0xA601u
                && data.timestamp1 == 100
                && data.timestamp2 == 200
                && data.entries.size() == 1
                && data.entries.front().indexLong == 1
                && data.entries.front().name == "LOCAL_LAYER"
                && data.entries.front().entryHandle != DRW::NoHandle;
        if (data.handle == 0xCD01u)
            readMalformedLayerIndexSeen_ = true;
    }
    void addSpatialIndex(const DRW_SpatialIndex& data) override {
        if (data.handle == 0xCE00u)
            readSpatialIndexSeen_ = data.parentHandle == 0xA601u
                && data.timestamp1 == 300
                && data.timestamp2 == 400;
        if (data.handle == 0xCE01u)
            readMalformedSpatialIndexSeen_ = true;
    }
    void addTableStyle(const DRW_TableStyle& data) override {
        if (data.handle == 0xCF00u)
            readTableStyleSeen_ = data.parentHandle == 0xA601u
                && data.m_name == "LOCAL_TABLESTYLE"
                && data.m_rowStyles.size() == 3
                && data.m_rowStyles.front().m_borders.size() == 6
                && data.m_rowStyles.back().m_borders.size() == 6;
        if (data.handle == 0xCF01u)
            readMalformedTableStyleSeen_ = true;
    }
    void addSpatialFilter(const DRW_SpatialFilter& data) override {
        if (data.handle == 0xD000u)
            readSpatialFilterSeen_ = data.parentHandle == 0xA601u
                && data.m_boundaryPoints.size() == 2
                && data.m_boundaryPoints[0].x == 1.0
                && data.m_boundaryPoints[1].y == 4.0
                && data.m_normal.z == 1.0
                && data.m_origin.x == 10.0
                && data.m_displayBoundary
                && data.m_clipFrontPlane
                && data.m_frontDistance == 5.0;
        if (data.handle == 0xD001u)
            readMalformedSpatialFilterSeen_ = true;
    }
    void addGeoData(const DRW_GeoData& data) override {
        if (data.handle == 0xD100u)
            readGeoDataSeen_ = data.parentHandle == 0x17u
                && data.xDictHandle == 0xA601u
                && data.m_version == 1
                && data.m_hostBlockHandle == 0x17u
                && data.m_coordinatesType == 1
                && data.m_designPoint.x == 100.0
                && data.m_referencePoint.y == 20.0
                && data.m_horizontalUnitScale == 1.5
                && data.m_horizontalUnits == 2
                && data.m_coordinateSystemDefinition == "LOCAL_COORD_SYS"
                && data.m_geoRssTag == "LOCAL_GEO_TAG"
                && data.m_observationFromTag == "LOCAL_FROM"
                && data.m_observationToTag == "LOCAL_TO"
                && data.m_observationCoverageTag == "LOCAL_COVERAGE"
                && data.m_points.empty() && data.m_faces.empty();
        if (data.handle == 0xD101u)
            readMalformedGeoDataSeen_ = true;
        if (data.handle == 0xD200u)
            readGeoDataV2Seen_ = data.parentHandle == 0x17u
                && data.xDictHandle == 0xA601u
                && data.m_version == 2
                && data.m_coordinatesType == 2
                && data.m_designPoint.x == 100.0
                && data.m_referencePoint.y == 20.0
                && data.m_horizontalUnitScale == 1.5
                && data.m_verticalUnitScale == 2.5
                && data.m_horizontalUnits == 2
                && data.m_verticalUnits == 3
                && data.m_scaleEstimationMethod == 1
                && data.m_userSpecifiedScaleFactor == 1.25
                && data.m_enableSeaLevelCorrection
                && data.m_seaLevelElevation == 4.5
                && data.m_coordinateProjectionRadius == 6.5
                && data.m_coordinateSystemDefinition == "LOCAL_COORD_SYS_V2"
                && data.m_geoRssTag == "LOCAL_GEO_TAG_V2"
                && data.m_observationFromTag == "LOCAL_FROM_V2"
                && data.m_observationToTag == "LOCAL_TO_V2"
                && data.m_observationCoverageTag == "LOCAL_COVERAGE_V2"
                && data.m_points.empty() && data.m_faces.empty();
    }
    void linkUnderlay(const DRW_UnderlayDefinition* data) override {
        if (data == nullptr)
            return;
        if (data->handle == 0xD300u)
            readPdfUnderlaySeen_ = data->kind == DRW_UnderlayDefinition::PDF
                && data->parentHandle == 0xA601u
                && data->filename == "LOCAL_PDF.pdf"
                && data->sheetName == "LOCAL_PDF_SHEET";
        if (data->handle == 0xD400u)
            readDgnUnderlaySeen_ = data->kind == DRW_UnderlayDefinition::DGN
                && data->parentHandle == 0xA601u
                && data->filename == "LOCAL_DGN.dgn"
                && data->sheetName == "LOCAL_DGN_SHEET";
        if (data->handle == 0xD500u)
            readDwfUnderlaySeen_ = data->kind == DRW_UnderlayDefinition::DWF
                && data->parentHandle == 0xA601u
                && data->filename == "LOCAL_DWF.dwf"
                && data->sheetName == "LOCAL_DWF_SHEET";
    }
    void addPointCloudDef(const DRW_PointCloudDef& data) override {
        if (data.handle == 0xD600u)
            readPointCloudDefinitionSeen_ =
                data.m_kind == DRW_PointCloudDef::Definition
                && data.parentHandle == 0xA601u
                && data.m_classVersion == 2
                && data.m_sourceFilename == "LOCAL_POINTCLOUD.rcs"
                && data.m_isLoaded
                && data.m_pointCount == 1234
                && data.m_extentsMin.x == -1.0
                && data.m_extentsMax.z == 30.0;
        if (data.handle == 0xD601u)
            readPointCloudDefinitionExSeen_ =
                data.m_kind == DRW_PointCloudDef::DefinitionEx
                && data.parentHandle == 0xA601u
                && data.m_classVersion == 2
                && data.m_sourceFilename == "LOCAL_POINTCLOUD_EX.rcs"
                && data.m_pointCount == 5678;
        if (data.handle == 0xD602u)
            readPointCloudReactorSeen_ =
                data.m_kind == DRW_PointCloudDef::Reactor
                && data.parentHandle == 0xD600u
                && data.m_classVersion == 2;
        if (data.handle == 0xD603u)
            readPointCloudReactorExSeen_ =
                data.m_kind == DRW_PointCloudDef::ReactorEx
                && data.parentHandle == 0xD601u
                && data.m_classVersion == 2;
        if (data.handle == 0xD604u)
            readMalformedPointCloudSeen_ = true;
    }
    void addPointCloudColorMap(const DRW_PointCloudColorMap& data) override {
        if (data.handle == 0xD800u)
            readPointCloudColorMapSeen_ =
                data.parentHandle == 0xA601u
                && data.m_classVersion == 1
                && data.m_defaultIntensityColorScheme == "LOCAL_INTENSITY"
                && data.m_defaultElevationColorScheme == "LOCAL_ELEVATION"
                && data.m_defaultClassificationColorScheme
                    == "LOCAL_CLASSIFICATION"
                && data.m_colorRampCount == 1
                && data.m_colorRamps.size() == 1
                && data.m_colorRamps.front().m_classVersion == 2
                && data.m_colorRamps.front().m_rampCount == 2
                && data.m_colorRamps.front().m_colorSchemes.size() == 2
                && data.m_colorRamps.front().m_colorSchemes[0]
                    == "LOCAL_RAMP_LOW"
                && data.m_classificationColorRampCount == 1
                && data.m_classificationColorRamps.size() == 1
                && data.m_classificationColorRamps.front().m_rampCount == 1
                && data.m_classificationColorRamps.front().m_colorSchemes[0]
                    == "LOCAL_CLASS_A";
        if (data.handle == 0xD801u)
            readMalformedPointCloudColorMapSeen_ = true;
    }
    void addNavisworksModelDef(const DRW_NavisworksModelDef& data) override {
        if (data.handle == 0xD900u)
            readNavisworksModelDefSeen_ =
                data.parentHandle == 0xA601u
                && data.m_flags == 3
                && data.m_path == "LOCAL_NAVISWORKS.nwd"
                && data.m_status
                && data.m_minExtent.x == -4.0
                && data.m_maxExtent.z == 60.0
                && data.m_hostDrawingVisibility;
        if (data.handle == 0xD901u)
            readMalformedNavisworksModelDefSeen_ = true;
    }
    void addSunStudy(const DRW_SunStudy& data) override {
        if (data.handle == 0xDA00u)
            readSunStudySeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 1
                && data.m_setupName == "LOCAL_SUNSTUDY"
                && data.m_description == "LOCAL_SUN_DESC"
                && data.m_sheetSetName == "LOCAL_SHEETSET"
                && data.m_sheetSubsetName == "LOCAL_SUBSET"
                && data.m_outputType == 0
                && data.m_useSubset
                && data.m_selectDatesFromCalendar
                && data.m_selectRangeOfDates
                && data.m_startTime == 10
                && data.m_endTime == 20
                && data.m_interval == 5
                && data.m_shadePlotType == 2
                && data.m_viewportCount == 3
                && data.m_rowCount == 4
                && data.m_columnCount == 5
                && data.m_spacing == 1.5
                && data.m_dates.size() == 1
                && data.m_dates.front().m_julianDay == 2451545
                && data.m_dates.front().m_milliseconds == 3600000
                && data.m_hours.size() == 3
                && data.m_hours[0] && !data.m_hours[1] && data.m_hours[2]
                && data.m_pageSetupWizardHandle == 0xA603u
                && data.m_viewHandle == 0xA700u
                && data.m_visualStyleHandle == 0xC000u
                && data.m_textStyleHandle == 0xA800u;
        if (data.handle == 0xDA01u)
            readMalformedSunStudySeen_ = true;
    }
    void addMotionPath(const DRW_MotionPath& data) override {
        if (data.handle == 0xDB00u)
            readMotionPathSeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 2
                && data.m_cameraPathHandle == 0xD925u
                && data.m_targetPathHandle == 0xD926u
                && data.m_viewTableHandle == 0xA700u
                && data.m_frames == 120
                && data.m_frameRate == 30
                && data.m_cornerDeceleration;
        if (data.handle == 0xDB01u)
            readMalformedMotionPathSeen_ = true;
    }
    void addCurvePath(const DRW_CurvePath& data) override {
        if (data.handle == 0xDC00u)
            readCurvePathSeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 2
                && data.m_entityHandle != 0;
        if (data.handle == 0xDC01u)
            readMalformedCurvePathSeen_ = true;
    }
    void addPointPath(const DRW_PointPath& data) override {
        if (data.handle == 0xDD00u)
            readPointPathSeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 3
                && data.m_point.x == 7.0
                && data.m_point.y == 8.0
                && data.m_point.z == 9.0;
        if (data.handle == 0xDD01u)
            readMalformedPointPathSeen_ = true;
    }
    void addObjectPtr(const DRW_ObjectPtr& data) override {
        if (data.handle == 0xDE00u)
            readObjectPtrSeen_ = data.parentHandle == 0xA601u;
        if (data.handle == 0xDE01u)
            readMalformedObjectPtrSeen_ = true;
    }
    void addPartialViewingIndex(const DRW_PartialViewingIndex& data) override {
        if (data.handle == 0xDF00u)
            readPartialViewingIndexSeen_ = data.parentHandle == 0xA601u
                && data.m_entryCount == 2
                && data.m_hasEntries
                && data.m_entries.size() == 2
                && data.m_entries[0].extentsMin.x == -1.0
                && data.m_entries[0].extentsMax.z == 30.0
                && data.m_entries[0].objectHandle != 0
                && data.m_entries[1].extentsMin.x == 40.0
                && data.m_entries[1].extentsMax.z == 90.0
                && data.m_entries[1].objectHandle == 0xD925u;
        if (data.handle == 0xDF01u)
            readMalformedPartialViewingIndexSeen_ = true;
    }
    void addBackground(const DRW_Background& data) override {
        if (data.handle == 0xE000u)
            readSolidBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::Solid
                && data.m_classVersion == 1
                && data.m_solidColor == 0x010203;
        if (data.handle == 0xE100u)
            readGradientBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::Gradient
                && data.m_classVersion == 1
                && data.m_colorTop == 0x101112
                && data.m_colorMiddle == 0x202122
                && data.m_colorBottom == 0x303132
                && data.m_horizon == 0.1
                && data.m_height == 0.2
                && data.m_rotation == 0.3;
        if (data.handle == 0xE200u)
            readGroundPlaneBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::GroundPlane
                && data.m_classVersion == 1
                && data.m_colorSkyZenith == 0x404142
                && data.m_colorFar == 0x909192;
        if (data.handle == 0xE300u)
            readImageBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::Image
                && data.m_classVersion == 1
                && data.m_fileName == "LOCAL_BACKGROUND_IMAGE.png"
                && data.m_fitToScreen && data.m_maintainAspect
                && !data.m_useTiling
                && data.m_offset.x == 1.0 && data.m_offset.y == 2.0
                && data.m_scale.x == 3.0 && data.m_scale.y == 4.0;
        if (data.handle == 0xE400u)
            readIblBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::Ibl
                && data.m_classVersion == 1
                && data.m_iblName == "LOCAL_IBL"
                && data.m_enabled && data.m_displayImage
                && data.m_rotation == 0.4
                && data.m_secondaryBackgroundHandle == 0xE300u;
        if (data.handle == 0xE500u)
            readSkylightBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::Skylight
                && data.m_classVersion == 1
                && data.m_sunHandle == 0xDA00u;
        if (data.handle == 0xE600u)
            readMalformedBackgroundSeen_ = true;
    }
    void addSection(const DRW_Section& data) override {
        if (data.handle == 0xE700u)
            readSectionManagerSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Section::Manager
                && data.m_isLive
                && data.m_sectionHandles.size() == 1
                && data.m_sectionHandles.front() == 0xE800u;
        if (data.handle == 0xE800u)
            readSectionSettingsSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Section::Settings
                && data.m_currentType == 1
                && data.m_types.size() == 1
                && data.m_types.front().m_type == 2
                && data.m_types.front().m_generation == 3
                && data.m_types.front().m_sourceHandles.size() == 1
                && data.m_types.front().m_sourceHandles.front() != 0
                && data.m_types.front().m_destinationBlockHandle == 0x17u
                && data.m_types.front().m_destinationFile == "LOCAL_SECTION.dwg"
                && data.m_types.front().m_geometry.size() == 1
                && data.m_types.front().m_geometry.front().m_numGeometries == 1
                && data.m_types.front().m_geometry.front().m_color == 6
                && data.m_types.front().m_geometry.front().m_layer == "LOCAL_LAYER"
                && data.m_types.front().m_geometry.front().m_hatchScale == 1.25;
        if (data.handle == 0xE900u)
            readMalformedSectionSeen_ = true;
    }
    void addTvDeviceProperties(const DRW_TvDeviceProperties& data) override {
        if (data.handle == 0xEA00u)
            readTvDevicePropertiesSeen_ = data.parentHandle == 0xA601u
                && data.flags == 1
                && data.maxRegenThreads == 2
                && data.useLutPalette == 3
                && data.alternateHighlight == 4
                && data.alternateHighlightColor == 5
                && data.geometryShaderUsage == 6
                && data.blendingMode == 7
                && data.antialiasingLevel == 0.25
                && data.valueBd2 == 0.75;
        if (data.handle == 0xEA01u)
            readMalformedTvDevicePropertiesSeen_ = true;
    }
    void addVxControl(const DRW_VxControl& data) override {
        if (data.handle == 0xEB00u)
            readVxControlSeen_ = data.parentHandle == 0xA601u
                && (expectedVersion_ <= DRW::AC1018
                    ? (data.classVersion == 0 && data.flags == 0
                       && data.recordHandles.empty())
                    : (data.classVersion == 8 && data.flags == 9
                       && data.recordHandles.size() == 1
                       && data.recordHandles.front() != 0));
        if (data.handle == 0xEB01u)
            readMalformedVxControlSeen_ = true;
    }
    void addVxTableRecord(const DRW_VxTableRecord& data) override {
        if (data.handle == 0xEC00u)
            readVxTableRecordSeen_ = data.parentHandle == 0xA601u
                && (expectedVersion_ <= DRW::AC1018
                    ? (data.classVersion == 0 && data.flags == 0
                       && data.name.empty())
                    : (data.classVersion == 10 && data.flags == 11
                       && data.name == "LOCAL_VX_RECORD"));
        if (data.handle == 0xEC01u)
            readMalformedVxTableRecordSeen_ = true;
    }
    void addInsert(const DRW_Insert& data) override {
        readInsertSeen_ = true;
        readAttribSeen_ = data.attlist.size() == 1
            && data.attlist.front() != nullptr
            && data.attlist.front()->tag == "LOCAL_TAG"
            && data.attlist.front()->text == "LOCAL_VALUE";
        dx_iface::addInsert(data);
    }

    bool wroteLine() const { return wroteLine_; }
    bool wroteSimpleEntities() const {
        return wrotePoint_ && wroteCircle_ && wroteArc_ && wrotePolyline_
            && wroteText_ && wroteMText_ && wroteEllipse_ && wroteTrace_
            && wroteSolid_ && wrote3dFace_ && wroteRay_ && wroteXline_
            && wrote3dLine_;
    }
    bool wroteAdvancedEntities() const {
        return wroteOldPolyline_ && wroteSpline_ && wroteHelix_;
    }
    bool wroteOldPolyline() const { return wroteOldPolyline_; }
    bool wroteSpline() const { return wroteSpline_; }
    bool wroteHelix() const { return wroteHelix_; }
    bool readSplineR2013Fields() const { return readSplineR2013Fields_; }
    bool rejectedMalformedHelix() const { return rejectedMalformedHelix_; }
    bool wroteCamera() const { return wroteCamera_; }
    bool rejectedMalformedCamera() const { return rejectedMalformedCamera_; }
    bool wroteGeoPositionMarker() const { return wroteGeoPositionMarker_; }
    bool rejectedMalformedGeoPositionMarker() const {
        return rejectedMalformedGeoPositionMarker_;
    }
    bool wrotePointCloud() const { return wrotePointCloud_; }
    bool wrotePointCloudEx() const { return wrotePointCloudEx_; }
    bool rejectedMalformedPointCloudEntity() const {
        return rejectedMalformedPointCloudEntity_;
    }
    bool rejectedMalformedPointCloudEx() const {
        return rejectedMalformedPointCloudEx_;
    }
    bool wroteHatch() const { return wroteHatch_; }
    bool wroteLeader() const { return wroteLeader_; }
    bool wroteTolerance() const { return wroteTolerance_; }
    bool rejectedMalformedTolerance() const {
        return rejectedMalformedTolerance_;
    }
    bool wroteRText() const { return wroteRText_; }
    bool wroteArcAlignedText() const { return wroteArcAlignedText_; }
    bool rejectedMalformedRText() const { return rejectedMalformedRText_; }
    bool rejectedMalformedArcAlignedText() const {
        return rejectedMalformedArcAlignedText_;
    }
    bool wroteDimensionAssociation() const {
        return wroteDimensionAssociation_;
    }
    bool wroteEvaluationGraph() const { return wroteEvaluationGraph_; }
    bool rejectedMalformedDimensionAssociation() const {
        return rejectedMalformedDimensionAssociation_;
    }
    bool rejectedMalformedEvaluationGraph() const {
        return rejectedMalformedEvaluationGraph_;
    }
    bool wroteBlockRepresentationData() const {
        return wroteBlockRepresentationData_;
    }
    bool rejectedMalformedBlockRepresentationData() const {
        return rejectedMalformedBlockRepresentationData_;
    }
    bool wroteBlock() const {
        return wroteBlock_ && wroteBlockPolyline_ && wroteBlockContent_;
    }
    bool wroteInsert() const { return wroteInsert_; }
    bool wroteAttrib() const { return wroteAttrib_; }
    bool wroteGroup() const { return wroteGroup_; }
    bool wroteObjectSet() const {
        return wroteDictionary_ && wroteXRecord_ && wrotePlotSettings_
            && wroteLayout_ && wroteMLineStyle_ && wroteMLeaderStyle_
            && wroteDictionaryVar_ && wroteDictionaryWithDefault_
            && wroteSortEntsTable_ && wroteFieldList_ && wroteField_
            && wroteRasterVariables_ && wroteWipeoutVariables_
            && wroteVisualStyle_ && wroteRenderSettings_
            && wroteRenderEnvironment_ && wroteRenderGlobal_
            && wroteRenderEntry_ && wroteRenderRapid_ && wroteRenderMental_
            && wroteMaterial_
            && (wroteDbColor_ || rejectedUnsupportedDbColor_)
            && wroteLightList_
            && wroteScale_
            && wroteIDBuffer_
            && wroteLayerIndex_
            && wroteSpatialIndex_
            && (wroteTableStyle_ || rejectedUnsupportedTableStyle_)
            && wroteSpatialFilter_
            && wroteGeoData_
            && wroteGeoDataV2_
            && wrotePdfUnderlay_
            && wroteDgnUnderlay_
            && wroteDwfUnderlay_
            && wrotePointCloudDefinition_
            && wrotePointCloudDefinitionEx_
            && wrotePointCloudReactor_
            && wrotePointCloudReactorEx_
            && wrotePointCloudColorMap_
            && wroteNavisworksModelDef_
            && wroteSunStudy_
            && wroteMotionPath_
            && wroteCurvePath_
            && wrotePointPath_
            && wroteObjectPtr_
            && wrotePartialViewingIndex_
            && wroteSolidBackground_
            && wroteGradientBackground_
            && wroteGroundPlaneBackground_
            && wroteImageBackground_
            && wroteIblBackground_
            && wroteSkylightBackground_
            && (wroteSectionManager_ || rejectedUnsupportedSection_)
            && (wroteSectionSettings_ || rejectedUnsupportedSection_)
            && (wroteDimensionAssociation_
                || expectedVersion_ < DRW::AC1021)
            && (wroteEvaluationGraph_ || expectedVersion_ < DRW::AC1021)
            && wroteBlockRepresentationData_
            && wroteTvDeviceProperties_
            && wroteVxControl_
            && wroteVxTableRecord_
            && wroteGroup_;
    }
    bool rejectedMalformedObject() const { return rejectedMalformedObject_; }
    bool rejectedMalformedStyle() const { return rejectedMalformedStyle_; }
    bool rejectedMalformedMLeaderStyle() const {
        return rejectedMalformedMLeaderStyle_;
    }
    bool rejectedMalformedDictionaryVar() const {
        return rejectedMalformedDictionaryVar_;
    }
    bool rejectedMalformedDictionaryWithDefault() const {
        return rejectedMalformedDictionaryWithDefault_;
    }
    bool rejectedMalformedSortEntsTable() const {
        return rejectedMalformedSortEntsTable_;
    }
    bool rejectedMalformedFieldList() const { return rejectedMalformedFieldList_; }
    bool rejectedMalformedField() const { return rejectedMalformedField_; }
    bool rejectedMalformedRasterVariables() const {
        return rejectedMalformedRasterVariables_;
    }
    bool rejectedMalformedWipeoutVariables() const {
        return rejectedMalformedWipeoutVariables_;
    }
    bool rejectedMalformedVisualStyle() const {
        return rejectedMalformedVisualStyle_;
    }
    bool rejectedMalformedRenderSettings() const {
        return rejectedMalformedRenderSettings_;
    }
    bool rejectedMalformedRenderEnvironment() const {
        return rejectedMalformedRenderEnvironment_;
    }
    bool rejectedMalformedRenderGlobal() const {
        return rejectedMalformedRenderGlobal_;
    }
    bool rejectedMalformedRenderEntry() const {
        return rejectedMalformedRenderEntry_;
    }
    bool rejectedMalformedRenderRapid() const {
        return rejectedMalformedRenderRapid_;
    }
    bool rejectedMalformedRenderMental() const {
        return rejectedMalformedRenderMental_;
    }
    bool rejectedMalformedMaterial() const { return rejectedMalformedMaterial_; }
    bool rejectedUnsupportedDbColor() const { return rejectedUnsupportedDbColor_; }
    bool rejectedMalformedDbColor() const { return rejectedMalformedDbColor_; }
    bool wroteDbColor() const { return wroteDbColor_; }
    bool rejectedMalformedLightList() const { return rejectedMalformedLightList_; }
    bool rejectedMalformedScale() const { return rejectedMalformedScale_; }
    bool rejectedMalformedIDBuffer() const { return rejectedMalformedIDBuffer_; }
    bool rejectedMalformedLayerIndex() const { return rejectedMalformedLayerIndex_; }
    bool rejectedMalformedSpatialIndex() const { return rejectedMalformedSpatialIndex_; }
    bool wroteTableStyle() const { return wroteTableStyle_; }
    bool rejectedMalformedTableStyle() const { return rejectedMalformedTableStyle_; }
    bool rejectedUnsupportedTableStyle() const { return rejectedUnsupportedTableStyle_; }
    bool rejectedMalformedSpatialFilter() const { return rejectedMalformedSpatialFilter_; }
    bool wroteGeoData() const { return wroteGeoData_; }
    bool rejectedMalformedGeoData() const { return rejectedMalformedGeoData_; }
    bool wroteGeoDataV2() const { return wroteGeoDataV2_; }
    bool rejectedMalformedUnderlay() const { return rejectedMalformedUnderlay_; }
    bool wrotePdfUnderlay() const { return wrotePdfUnderlay_; }
    bool wroteDgnUnderlay() const { return wroteDgnUnderlay_; }
    bool wroteDwfUnderlay() const { return wroteDwfUnderlay_; }
    bool wrotePointCloudDefinition() const { return wrotePointCloudDefinition_; }
    bool wrotePointCloudDefinitionEx() const {
        return wrotePointCloudDefinitionEx_;
    }
    bool wrotePointCloudReactor() const { return wrotePointCloudReactor_; }
    bool wrotePointCloudReactorEx() const { return wrotePointCloudReactorEx_; }
    bool rejectedMalformedPointCloud() const {
        return rejectedMalformedPointCloud_;
    }
    bool wrotePointCloudColorMap() const { return wrotePointCloudColorMap_; }
    bool rejectedMalformedPointCloudColorMap() const {
        return rejectedMalformedPointCloudColorMap_;
    }
    bool wroteNavisworksModelDef() const { return wroteNavisworksModelDef_; }
    bool rejectedMalformedNavisworksModelDef() const {
        return rejectedMalformedNavisworksModelDef_;
    }
    bool wroteSunStudy() const { return wroteSunStudy_; }
    bool rejectedMalformedSunStudy() const { return rejectedMalformedSunStudy_; }
    bool wroteMotionPath() const { return wroteMotionPath_; }
    bool rejectedMalformedMotionPath() const {
        return rejectedMalformedMotionPath_;
    }
    bool wroteCurvePath() const { return wroteCurvePath_; }
    bool rejectedMalformedCurvePath() const {
        return rejectedMalformedCurvePath_;
    }
    bool wrotePointPath() const { return wrotePointPath_; }
    bool rejectedMalformedPointPath() const {
        return rejectedMalformedPointPath_;
    }
    bool wroteObjectPtr() const { return wroteObjectPtr_; }
    bool rejectedMalformedObjectPtr() const {
        return rejectedMalformedObjectPtr_;
    }
    bool wrotePartialViewingIndex() const { return wrotePartialViewingIndex_; }
    bool rejectedMalformedPartialViewingIndex() const {
        return rejectedMalformedPartialViewingIndex_;
    }
    bool rejectedMalformedBackground() const {
        return rejectedMalformedBackground_;
    }
    bool wroteBackgrounds() const {
        return wroteSolidBackground_ && wroteGradientBackground_
            && wroteGroundPlaneBackground_ && wroteImageBackground_
            && wroteIblBackground_ && wroteSkylightBackground_;
    }
    bool wroteSectionManager() const { return wroteSectionManager_; }
    bool wroteSectionSettings() const { return wroteSectionSettings_; }
    bool rejectedUnsupportedSection() const { return rejectedUnsupportedSection_; }
    bool rejectedMalformedSection() const { return rejectedMalformedSection_; }
    bool wroteTvDeviceProperties() const { return wroteTvDeviceProperties_; }
    bool wroteVxControl() const { return wroteVxControl_; }
    bool wroteVxTableRecord() const { return wroteVxTableRecord_; }
    bool rejectedMalformedTvDeviceProperties() const {
        return rejectedMalformedTvDeviceProperties_;
    }
    bool rejectedMalformedVxControl() const {
        return rejectedMalformedVxControl_;
    }
    bool rejectedMalformedVxTableRecord() const {
        return rejectedMalformedVxTableRecord_;
    }
    bool wroteImage() const { return wroteImage_; }
    bool rejectedMalformedImage() const { return rejectedMalformedImage_; }
    bool readLineSeen() const { return readLineSeen_; }
    bool readSimpleEntitiesSeen() const {
        return readPointSeen_ && readCircleSeen_ && readArcSeen_
            && readPolylineSeen_ && readTextSeen_ && readMTextSeen_
            && readEllipseSeen_ && readTraceSeen_ && readSolidSeen_
            && read3dFaceSeen_ && readRaySeen_ && readXlineSeen_
            && read3dLineSeen_;
    }
    bool readAdvancedEntitiesSeen() const {
        return readOldPolylineSeen_ && readSplineSeen_ && readHelixSeen_;
    }
    bool readOldPolylineSeen() const { return readOldPolylineSeen_; }
    bool readSplineSeen() const { return readSplineSeen_; }
    bool readHelixSeen() const { return readHelixSeen_; }
    bool readCameraSeen() const { return readCameraSeen_; }
    bool readGeoPositionMarkerSeen() const { return readGeoPositionMarkerSeen_; }
    bool wroteShape() const { return wroteShape_; }
    bool rejectedMalformedShape() const { return rejectedMalformedShape_; }
    bool readShapeSeen() const { return readShapeSeen_; }
    bool wroteMLine() const { return wroteMLine_; }
    bool rejectedMalformedMLine() const { return rejectedMalformedMLine_; }
    bool readMLineSeen() const { return readMLineSeen_; }
    bool wroteLight() const { return wroteLight_; }
    bool rejectedMalformedLight() const { return rejectedMalformedLight_; }
    bool readLightSeen() const { return readLightSeen_; }
    bool wroteMesh() const { return wroteMesh_; }
    bool rejectedMalformedMesh() const { return rejectedMalformedMesh_; }
    bool readMeshSeen() const { return readMeshSeen_; }
    bool wroteWipeout() const { return wroteWipeout_; }
    bool rejectedMalformedWipeout() const { return rejectedMalformedWipeout_; }
    bool readWipeoutSeen() const { return readWipeoutSeen_; }
    bool wroteNavisworksModel() const { return wroteNavisworksModel_; }
    bool rejectedMalformedNavisworksModel() const {
        return rejectedMalformedNavisworksModel_;
    }
    bool readNavisworksModelSeen() const { return readNavisworksModelSeen_; }
    bool wroteUnderlay() const { return wroteUnderlay_; }
    bool readUnderlaySeen() const { return readUnderlaySeen_; }
    bool wroteDgnUnderlayEntity() const { return wroteDgnUnderlayEntity_; }
    bool wroteDwfUnderlayEntity() const { return wroteDwfUnderlayEntity_; }
    bool readDgnUnderlayEntitySeen() const {
        return readDgnUnderlayEntitySeen_;
    }
    bool readDwfUnderlayEntitySeen() const {
        return readDwfUnderlayEntitySeen_;
    }
    bool wroteSurfaceSet() const {
        return wroteSurfaceSet_;
    }
    bool rejectedMalformedSurface() const {
        return rejectedMalformedSurface_;
    }
    bool readSurfaceSetSeen() const {
        return readPlaneSurfaceSeen_ && readExtrudedSurfaceSeen_
            && readRevolvedSurfaceSeen_ && readSweptSurfaceSeen_
            && readLoftedSurfaceSeen_ && readNurbsSurfaceSeen_;
    }
    bool readPointCloudSeen() const { return readPointCloudSeen_; }
    bool readPointCloudExSeen() const { return readPointCloudExSeen_; }
    bool readHatchSeen() const { return readHatchSeen_; }
    bool readLeaderSeen() const { return readLeaderSeen_; }
    bool readToleranceSeen() const { return readToleranceSeen_; }
    bool readRTextSeen() const { return readRTextSeen_; }
    bool readArcAlignedTextSeen() const { return readArcAlignedTextSeen_; }
    bool readDimensionAssociationSeen() const {
        return readDimensionAssociationSeen_;
    }
    bool readEvaluationGraphSeen() const { return readEvaluationGraphSeen_; }
    bool readBlockRepresentationDataSeen() const {
        return readBlockRepresentationDataSeen_;
    }
    bool readInsertSeen() const { return readInsertSeen_; }
    bool readAttribSeen() const { return readAttribSeen_; }
    bool readGroupSeen() const { return readGroupSeen_; }
    bool readObjectRawCarrierSetSeen() const {
        const std::size_t expected = expectedVersion_ == DRW::AC1015
            ? 53u : expectedVersion_ == DRW::AC1018
                ? 54u : expectedVersion_ == DRW::AC1021 ? 56u : 55u;
        return readRawObjectInvariant_
            && readLocalRawObjectHandles_.size() == expected;
    }
    bool readObjectSetSeen() const {
        const bool result = readDictionarySeen_ && readXRecordSeen_
            && readPlotSettingsSeen_ && readLayoutSeen_ && readMLineStyleSeen_
            && readMLeaderStyleSeen_ && readDictionaryVarSeen_
            && readDictionaryWithDefaultSeen_ && readSortEntsTableSeen_
            && readFieldListSeen_ && readFieldSeen_ && readRasterVariablesSeen_
            && readWipeoutVariablesSeen_ && readVisualStyleSeen_
            && readRenderSettingsSeen_ && readRenderEnvironmentSeen_
            && readRenderGlobalSeen_ && readRenderEntrySeen_
            && readRenderRapidSeen_ && readRenderMentalSeen_
            && readMaterialSeen_ && readLightListSeen_ && readScaleSeen_
            && readIDBufferSeen_ && readLayerIndexSeen_ && readSpatialIndexSeen_
            && (readTableStyleSeen_ || !tableStyleExpected_)
            && readSpatialFilterSeen_
            && readGeoDataSeen_
            && readGeoDataV2Seen_
            && readPdfUnderlaySeen_
            && readDgnUnderlaySeen_
            && readDwfUnderlaySeen_
            && readPointCloudDefinitionSeen_
            && readPointCloudDefinitionExSeen_
            && readPointCloudReactorSeen_
            && readPointCloudReactorExSeen_
            && readPointCloudColorMapSeen_
            && readNavisworksModelDefSeen_
            && readSunStudySeen_
            && readMotionPathSeen_
            && readCurvePathSeen_
            && readPointPathSeen_
            && readObjectPtrSeen_
            && readPartialViewingIndexSeen_
            && readSolidBackgroundSeen_
            && readGradientBackgroundSeen_
            && readGroundPlaneBackgroundSeen_
            && readImageBackgroundSeen_
            && readIblBackgroundSeen_
            && readSkylightBackgroundSeen_
            && (expectedVersion_ < DRW::AC1021 || readSectionSetSeen())
            && (expectedVersion_ < DRW::AC1021 || readDimensionAssociationSeen_)
            && (expectedVersion_ < DRW::AC1021 || readEvaluationGraphSeen_)
            && readBlockRepresentationDataSeen_
            && readTvDevicePropertiesSeen_
            && readVxControlSeen_
            && readVxTableRecordSeen_
            && readGroupSeen_;
        return result;
    }
    bool readMalformedObjectSeen() const { return readMalformedObjectSeen_; }
    bool readMalformedStyleSeen() const { return readMalformedStyleSeen_; }
    bool readMalformedMLeaderStyleSeen() const {
        return readMalformedMLeaderStyleSeen_;
    }
    bool readMalformedDictionaryVarSeen() const {
        return readMalformedDictionaryVarSeen_;
    }
    bool readMalformedDictionaryWithDefaultSeen() const {
        return readMalformedDictionaryWithDefaultSeen_;
    }
    bool readMalformedSortEntsTableSeen() const {
        return readMalformedSortEntsTableSeen_;
    }
    bool readMalformedFieldListSeen() const { return readMalformedFieldListSeen_; }
    bool readMalformedFieldSeen() const { return readMalformedFieldSeen_; }
    bool readMalformedRasterVariablesSeen() const {
        return readMalformedRasterVariablesSeen_;
    }
    bool readMalformedWipeoutVariablesSeen() const {
        return readMalformedWipeoutVariablesSeen_;
    }
    bool readVisualStyleSeen() const { return readVisualStyleSeen_; }
    bool readMalformedVisualStyleSeen() const {
        return readMalformedVisualStyleSeen_;
    }
    bool readRenderSettingsSeen() const { return readRenderSettingsSeen_; }
    bool readMalformedRenderSettingsSeen() const {
        return readMalformedRenderSettingsSeen_;
    }
    bool readRenderEnvironmentSeen() const { return readRenderEnvironmentSeen_; }
    bool readMalformedRenderEnvironmentSeen() const {
        return readMalformedRenderEnvironmentSeen_;
    }
    bool readRenderGlobalSeen() const { return readRenderGlobalSeen_; }
    bool readMalformedRenderGlobalSeen() const {
        return readMalformedRenderGlobalSeen_;
    }
    bool readRenderEntrySeen() const { return readRenderEntrySeen_; }
    bool readMalformedRenderEntrySeen() const {
        return readMalformedRenderEntrySeen_;
    }
    bool readRenderRapidSeen() const { return readRenderRapidSeen_; }
    bool readMalformedRenderRapidSeen() const {
        return readMalformedRenderRapidSeen_;
    }
    bool readRenderMentalSeen() const { return readRenderMentalSeen_; }
    bool readMalformedRenderMentalSeen() const {
        return readMalformedRenderMentalSeen_;
    }
    bool readMaterialSeen() const { return readMaterialSeen_; }
    bool readMalformedMaterialSeen() const { return readMalformedMaterialSeen_; }
    bool readDbColorSeen() const { return readDbColorSeen_; }
    bool readMalformedDbColorSeen() const { return readMalformedDbColorSeen_; }
    bool readLightListSeen() const { return readLightListSeen_; }
    bool readMalformedLightListSeen() const { return readMalformedLightListSeen_; }
    bool readScaleSeen() const { return readScaleSeen_; }
    bool readMalformedScaleSeen() const { return readMalformedScaleSeen_; }
    bool readIDBufferSeen() const { return readIDBufferSeen_; }
    bool readMalformedIDBufferSeen() const { return readMalformedIDBufferSeen_; }
    bool readLayerIndexSeen() const { return readLayerIndexSeen_; }
    bool readMalformedLayerIndexSeen() const { return readMalformedLayerIndexSeen_; }
    bool readSpatialIndexSeen() const { return readSpatialIndexSeen_; }
    bool readMalformedSpatialIndexSeen() const { return readMalformedSpatialIndexSeen_; }
    bool readTableStyleSeen() const { return readTableStyleSeen_; }
    bool readMalformedTableStyleSeen() const { return readMalformedTableStyleSeen_; }
    bool readSpatialFilterSeen() const { return readSpatialFilterSeen_; }
    bool readMalformedSpatialFilterSeen() const { return readMalformedSpatialFilterSeen_; }
    bool readGeoDataSeen() const { return readGeoDataSeen_; }
    bool readMalformedGeoDataSeen() const { return readMalformedGeoDataSeen_; }
    bool readGeoDataV2Seen() const { return readGeoDataV2Seen_; }
    bool readPdfUnderlaySeen() const { return readPdfUnderlaySeen_; }
    bool readDgnUnderlaySeen() const { return readDgnUnderlaySeen_; }
    bool readDwfUnderlaySeen() const { return readDwfUnderlaySeen_; }
    bool readPointCloudDefinitionSeen() const {
        return readPointCloudDefinitionSeen_;
    }
    bool readPointCloudDefinitionExSeen() const {
        return readPointCloudDefinitionExSeen_;
    }
    bool readPointCloudReactorSeen() const { return readPointCloudReactorSeen_; }
    bool readPointCloudReactorExSeen() const {
        return readPointCloudReactorExSeen_;
    }
    bool readMalformedPointCloudSeen() const {
        return readMalformedPointCloudSeen_;
    }
    bool readPointCloudColorMapSeen() const {
        return readPointCloudColorMapSeen_;
    }
    bool readMalformedPointCloudColorMapSeen() const {
        return readMalformedPointCloudColorMapSeen_;
    }
    bool readNavisworksModelDefSeen() const {
        return readNavisworksModelDefSeen_;
    }
    bool readSunStudySeen() const { return readSunStudySeen_; }
    bool readMotionPathSeen() const { return readMotionPathSeen_; }
    bool readMalformedSunStudySeen() const { return readMalformedSunStudySeen_; }
    bool readMalformedMotionPathSeen() const {
        return readMalformedMotionPathSeen_;
    }
    bool readCurvePathSeen() const { return readCurvePathSeen_; }
    bool readPointPathSeen() const { return readPointPathSeen_; }
    bool readObjectPtrSeen() const { return readObjectPtrSeen_; }
    bool readMalformedCurvePathSeen() const {
        return readMalformedCurvePathSeen_;
    }
    bool readMalformedPointPathSeen() const {
        return readMalformedPointPathSeen_;
    }
    bool readMalformedObjectPtrSeen() const {
        return readMalformedObjectPtrSeen_;
    }
    bool readPartialViewingIndexSeen() const {
        return readPartialViewingIndexSeen_;
    }
    bool readMalformedPartialViewingIndexSeen() const {
        return readMalformedPartialViewingIndexSeen_;
    }
    bool readBackgroundsSeen() const {
        return readSolidBackgroundSeen_ && readGradientBackgroundSeen_
            && readGroundPlaneBackgroundSeen_ && readImageBackgroundSeen_
            && readIblBackgroundSeen_ && readSkylightBackgroundSeen_;
    }
    bool readSectionManagerSeen() const { return readSectionManagerSeen_; }
    bool readSectionSettingsSeen() const { return readSectionSettingsSeen_; }
    bool readSectionSetSeen() const {
        return readSectionManagerSeen_ && readSectionSettingsSeen_;
    }
    bool readMalformedBackgroundSeen() const {
        return readMalformedBackgroundSeen_;
    }
    bool readMalformedSectionSeen() const { return readMalformedSectionSeen_; }
    bool readTvDevicePropertiesSeen() const {
        return readTvDevicePropertiesSeen_;
    }
    bool readVxControlSeen() const { return readVxControlSeen_; }
    bool readVxTableRecordSeen() const { return readVxTableRecordSeen_; }
    bool readMalformedTvDevicePropertiesSeen() const {
        return readMalformedTvDevicePropertiesSeen_;
    }
    bool readMalformedVxControlSeen() const {
        return readMalformedVxControlSeen_;
    }
    bool readMalformedVxTableRecordSeen() const {
        return readMalformedVxTableRecordSeen_;
    }
    bool readMalformedNavisworksModelDefSeen() const {
        return readMalformedNavisworksModelDefSeen_;
    }
    bool readImageSeen() const { return readImageSeen_; }
    bool readImageDefSeen() const { return readImageDefSeen_; }
    bool readImageReactorSeen() const { return readImageReactorSeen_; }
    void setTableStyleExpected(bool expected) { tableStyleExpected_ = expected; }
    const DRW_Line& readLine() const { return readLine_; }

private:
    static bool isLocalObjectHandle(std::uint32_t handle) {
        switch (handle) {
        case 0xA600u:
        case 0xA601u:
        case 0xA602u:
        case 0xA603u:
        case 0xA700u:
        case 0xA800u:
        case 0xA900u:
        case 0xB000u:
        case 0xB100u:
        case 0xB200u:
        case 0xB300u:
        case 0xB400u:
        case 0xB500u:
        case 0xB600u:
        case 0xC000u:
        case 0xC100u:
        case 0xC200u:
        case 0xC300u:
        case 0xC400u:
        case 0xC600u:
        case 0xC700u:
        case 0xC800u:
        case 0xC900u:
        case 0xCA00u:
        case 0xCB00u:
        case 0xCC00u:
        case 0xCD00u:
        case 0xCE00u:
        case 0xCF00u:
        case 0xD000u:
        case 0xD100u:
        case 0xD200u:
        case 0xD300u:
        case 0xD400u:
        case 0xD500u:
        case 0xD600u:
        case 0xD601u:
        case 0xD800u:
        case 0xD900u:
        case 0xDA00u:
        case 0xDB00u:
        case 0xDC00u:
        case 0xDD00u:
        case 0xDE00u:
        case 0xDF00u:
        case 0xE000u:
        case 0xE100u:
        case 0xE200u:
        case 0xE300u:
        case 0xE400u:
        case 0xE500u:
        case 0xE700u:
        case 0xE800u:
        case 0xEA00u:
        case 0xEB00u:
        case 0xEC00u:
            return true;
        default:
            return false;
        }
    }

    dwgRW* writer_ {nullptr};
    DRW::Version expectedVersion_ {DRW::UNKNOWNV};
    bool wroteLine_ {false};
    std::uint32_t modelSpaceLineHandle_ {0};
    bool wrotePoint_ {false};
    bool wroteCircle_ {false};
    bool wroteArc_ {false};
    bool wrotePolyline_ {false};
    bool wroteText_ {false};
    bool wroteMText_ {false};
    bool wroteEllipse_ {false};
    bool wroteTrace_ {false};
    bool wroteSolid_ {false};
    bool wrote3dFace_ {false};
    bool wroteRay_ {false};
    bool wroteXline_ {false};
    bool wrote3dLine_ {false};
    bool wroteOldPolyline_ {false};
    bool wroteSpline_ {false};
    bool wroteHelix_ {false};
    bool rejectedMalformedHelix_ {false};
    bool wroteCamera_ {false};
    bool rejectedMalformedCamera_ {false};
    bool wroteGeoPositionMarker_ {false};
    bool rejectedMalformedGeoPositionMarker_ {false};
    bool wroteShape_ {false};
    bool rejectedMalformedShape_ {false};
    bool wroteMLine_ {false};
    bool rejectedMalformedMLine_ {false};
    bool registeredLight_ {false};
    bool wroteLight_ {false};
    bool rejectedMalformedLight_ {false};
    bool registeredMesh_ {false};
    bool wroteMesh_ {false};
    bool rejectedMalformedMesh_ {false};
    bool wroteWipeout_ {false};
    bool rejectedMalformedWipeout_ {false};
    bool wroteNavisworksModel_ {false};
    bool rejectedMalformedNavisworksModel_ {false};
    bool wroteUnderlay_ {false};
    bool wroteDgnUnderlayEntity_ {false};
    bool wroteDwfUnderlayEntity_ {false};
    bool wroteSurfaceSet_ {false};
    bool wrotePlaneSurface_ {false};
    bool wroteExtrudedSurface_ {false};
    bool wroteRevolvedSurface_ {false};
    bool wroteSweptSurface_ {false};
    bool wroteLoftedSurface_ {false};
    bool wroteNurbsSurface_ {false};
    bool rejectedMalformedSurface_ {false};
    bool wrotePointCloud_ {false};
    bool wrotePointCloudEx_ {false};
    bool rejectedMalformedPointCloudEntity_ {false};
    bool rejectedMalformedPointCloudEx_ {false};
    bool wroteHatch_ {false};
    bool wroteLeader_ {false};
    bool wroteTolerance_ {false};
    bool rejectedMalformedTolerance_ {false};
    bool wroteRText_ {false};
    bool wroteArcAlignedText_ {false};
    bool rejectedMalformedRText_ {false};
    bool rejectedMalformedArcAlignedText_ {false};
    bool registeredRText_ {false};
    bool registeredArcAlignedText_ {false};
    bool registeredHelix_ {false};
    bool registeredCamera_ {false};
    bool wroteDimensionAssociation_ {false};
    bool wroteEvaluationGraph_ {false};
    bool rejectedMalformedDimensionAssociation_ {false};
    bool rejectedMalformedEvaluationGraph_ {false};
    bool wroteBlockRepresentationData_ {false};
    bool rejectedMalformedBlockRepresentationData_ {false};
    bool wroteBlock_ {false};
    bool wroteBlockPolyline_ {false};
    bool wroteBlockContent_ {false};
    bool wroteInsert_ {false};
    bool wroteAttrib_ {false};
    bool wroteGroup_ {false};
    bool registeredGroup_ {false};
    bool wroteDictionary_ {false};
    bool wroteXRecord_ {false};
    bool wrotePlotSettings_ {false};
    bool wroteLayout_ {false};
    bool wroteMLineStyle_ {false};
    bool wroteMLeaderStyle_ {false};
    bool wroteDictionaryVar_ {false};
    bool wroteDictionaryWithDefault_ {false};
    bool wroteSortEntsTable_ {false};
    bool wroteFieldList_ {false};
    bool wroteField_ {false};
    bool wroteRasterVariables_ {false};
    bool wroteWipeoutVariables_ {false};
    bool wroteVisualStyle_ {false};
    bool wroteRenderSettings_ {false};
    bool wroteRenderEnvironment_ {false};
    bool wroteRenderGlobal_ {false};
    bool wroteRenderEntry_ {false};
    bool wroteRenderRapid_ {false};
    bool wroteRenderMental_ {false};
    bool wroteMaterial_ {false};
    bool wroteDbColor_ {false};
    bool wroteLightList_ {false};
    bool wroteScale_ {false};
    bool wroteIDBuffer_ {false};
    bool wroteLayerIndex_ {false};
    bool wroteSpatialIndex_ {false};
    bool wroteTableStyle_ {false};
    bool wroteSpatialFilter_ {false};
    bool wroteGeoData_ {false};
    bool wroteGeoDataV2_ {false};
    bool rejectedMalformedObject_ {false};
    bool rejectedMalformedStyle_ {false};
    bool rejectedMalformedMLeaderStyle_ {false};
    bool rejectedMalformedDictionaryVar_ {false};
    bool rejectedMalformedDictionaryWithDefault_ {false};
    bool rejectedMalformedSortEntsTable_ {false};
    bool rejectedMalformedFieldList_ {false};
    bool rejectedMalformedField_ {false};
    bool rejectedMalformedRasterVariables_ {false};
    bool rejectedMalformedWipeoutVariables_ {false};
    bool rejectedMalformedVisualStyle_ {false};
    bool rejectedMalformedRenderSettings_ {false};
    bool rejectedMalformedRenderEnvironment_ {false};
    bool rejectedMalformedRenderGlobal_ {false};
    bool rejectedMalformedRenderEntry_ {false};
    bool rejectedMalformedRenderRapid_ {false};
    bool rejectedMalformedRenderMental_ {false};
    bool rejectedMalformedMaterial_ {false};
    bool rejectedUnsupportedDbColor_ {false};
    bool rejectedMalformedDbColor_ {false};
    bool rejectedMalformedLightList_ {false};
    bool rejectedMalformedScale_ {false};
    bool rejectedMalformedIDBuffer_ {false};
    bool rejectedMalformedLayerIndex_ {false};
    bool rejectedMalformedSpatialIndex_ {false};
    bool rejectedMalformedTableStyle_ {false};
    bool rejectedUnsupportedTableStyle_ {false};
    bool rejectedMalformedSpatialFilter_ {false};
    bool rejectedMalformedGeoData_ {false};
    bool wrotePdfUnderlay_ {false};
    bool wroteDgnUnderlay_ {false};
    bool wroteDwfUnderlay_ {false};
    bool rejectedMalformedUnderlay_ {false};
    bool wrotePointCloudDefinition_ {false};
    bool wrotePointCloudDefinitionEx_ {false};
    bool wrotePointCloudReactor_ {false};
    bool wrotePointCloudReactorEx_ {false};
    bool rejectedMalformedPointCloud_ {false};
    bool wrotePointCloudColorMap_ {false};
    bool rejectedMalformedPointCloudColorMap_ {false};
    bool wroteNavisworksModelDef_ {false};
    bool rejectedMalformedNavisworksModelDef_ {false};
    bool wroteSunStudy_ {false};
    bool rejectedMalformedSunStudy_ {false};
    bool wroteMotionPath_ {false};
    bool rejectedMalformedMotionPath_ {false};
    bool wroteCurvePath_ {false};
    bool rejectedMalformedCurvePath_ {false};
    bool wrotePointPath_ {false};
    bool rejectedMalformedPointPath_ {false};
    bool wroteObjectPtr_ {false};
    bool rejectedMalformedObjectPtr_ {false};
    bool wrotePartialViewingIndex_ {false};
    bool rejectedMalformedPartialViewingIndex_ {false};
    bool wroteSolidBackground_ {false};
    bool wroteGradientBackground_ {false};
    bool wroteGroundPlaneBackground_ {false};
    bool wroteImageBackground_ {false};
    bool wroteIblBackground_ {false};
    bool wroteSkylightBackground_ {false};
    bool rejectedMalformedBackground_ {false};
    bool wroteSectionManager_ {false};
    bool wroteSectionSettings_ {false};
    bool rejectedUnsupportedSection_ {false};
    bool rejectedMalformedSection_ {false};
    bool wroteTvDeviceProperties_ {false};
    bool wroteVxControl_ {false};
    bool wroteVxTableRecord_ {false};
    bool rejectedMalformedTvDeviceProperties_ {false};
    bool rejectedMalformedVxControl_ {false};
    bool rejectedMalformedVxTableRecord_ {false};
    bool wroteImage_ {false};
    bool rejectedMalformedImage_ {false};
    bool registeredDictionary_ {false};
    bool registeredMLeaderStyle_ {false};
    bool registeredDictionaryVar_ {false};
    bool registeredDictionaryWithDefault_ {false};
    bool registeredSortEntsTable_ {false};
    bool registeredFieldList_ {false};
    bool registeredField_ {false};
    bool registeredRasterVariables_ {false};
    bool registeredWipeoutVariables_ {false};
    bool registeredVisualStyle_ {false};
    bool registeredRenderSettings_ {false};
    bool registeredRenderEnvironment_ {false};
    bool registeredRenderGlobal_ {false};
    bool registeredRenderEntry_ {false};
    bool registeredRenderRapid_ {false};
    bool registeredRenderMental_ {false};
    bool registeredMaterial_ {false};
    bool registeredDbColor_ {false};
    bool registeredLightList_ {false};
    bool registeredScale_ {false};
    bool registeredIDBuffer_ {false};
    bool registeredLayerIndex_ {false};
    bool registeredSpatialIndex_ {false};
    bool registeredTableStyle_ {false};
    bool registeredSpatialFilter_ {false};
    bool registeredGeoData_ {false};
    bool registeredGeoDataV2_ {false};
    bool registeredPdfUnderlay_ {false};
    bool registeredDgnUnderlay_ {false};
    bool registeredDwfUnderlay_ {false};
    bool registeredSurfaceClasses_ {false};
    bool registeredPointCloudDefinition_ {false};
    bool registeredPointCloudDefinitionEx_ {false};
    bool registeredPointCloudReactor_ {false};
    bool registeredPointCloudReactorEx_ {false};
    bool registeredPointCloudColorMap_ {false};
    bool registeredNavisworksModelDef_ {false};
    bool registeredSunStudy_ {false};
    bool registeredMotionPath_ {false};
    bool registeredCurvePath_ {false};
    bool registeredPointPath_ {false};
    bool registeredObjectPtr_ {false};
    bool registeredPartialViewingIndex_ {false};
    bool registeredSolidBackground_ {false};
    bool registeredGradientBackground_ {false};
    bool registeredGroundPlaneBackground_ {false};
    bool registeredImageBackground_ {false};
    bool registeredIblBackground_ {false};
    bool registeredSkylightBackground_ {false};
    bool registeredSectionManager_ {false};
    bool registeredSectionSettings_ {false};
    bool registeredTvDeviceProperties_ {false};
    bool registeredVxControl_ {false};
    bool registeredVxTableRecord_ {false};
    bool registeredPlotSettings_ {false};
    bool registeredDimensionAssociation_ {false};
    bool registeredEvaluationGraph_ {false};
    bool readLineSeen_ {false};
    bool readPointSeen_ {false};
    bool readCircleSeen_ {false};
    bool readArcSeen_ {false};
    bool readPolylineSeen_ {false};
    bool readTextSeen_ {false};
    bool readMTextSeen_ {false};
    bool readEllipseSeen_ {false};
    bool readTraceSeen_ {false};
    bool readSolidSeen_ {false};
    bool read3dFaceSeen_ {false};
    bool readRaySeen_ {false};
    bool readXlineSeen_ {false};
    bool read3dLineSeen_ {false};
    bool readOldPolylineSeen_ {false};
    bool readSplineSeen_ {false};
    bool readSplineR2013Fields_ {false};
    bool readHelixSeen_ {false};
    bool readCameraSeen_ {false};
    bool readGeoPositionMarkerSeen_ {false};
    bool readShapeSeen_ {false};
    bool readMLineSeen_ {false};
    bool readLightSeen_ {false};
    bool readMeshSeen_ {false};
    bool readWipeoutSeen_ {false};
    bool readNavisworksModelSeen_ {false};
    bool readUnderlaySeen_ {false};
    bool readDgnUnderlayEntitySeen_ {false};
    bool readDwfUnderlayEntitySeen_ {false};
    bool readPlaneSurfaceSeen_ {false};
    bool readExtrudedSurfaceSeen_ {false};
    bool readRevolvedSurfaceSeen_ {false};
    bool readSweptSurfaceSeen_ {false};
    bool readLoftedSurfaceSeen_ {false};
    bool readNurbsSurfaceSeen_ {false};
    bool readPointCloudSeen_ {false};
    bool readPointCloudExSeen_ {false};
    bool readHatchSeen_ {false};
    bool readLeaderSeen_ {false};
    bool readToleranceSeen_ {false};
    bool readRTextSeen_ {false};
    bool readArcAlignedTextSeen_ {false};
    bool readDimensionAssociationSeen_ {false};
    bool readEvaluationGraphSeen_ {false};
    bool readBlockRepresentationDataSeen_ {false};
    bool readInsertSeen_ {false};
    bool readAttribSeen_ {false};
    bool readGroupSeen_ {false};
    bool readDictionarySeen_ {false};
    bool readXRecordSeen_ {false};
    bool readPlotSettingsSeen_ {false};
    bool readLayoutSeen_ {false};
    bool readMLineStyleSeen_ {false};
    bool readMLeaderStyleSeen_ {false};
    bool readDictionaryVarSeen_ {false};
    bool readDictionaryWithDefaultSeen_ {false};
    bool readSortEntsTableSeen_ {false};
    bool readFieldListSeen_ {false};
    bool readFieldSeen_ {false};
    bool readRasterVariablesSeen_ {false};
    bool readWipeoutVariablesSeen_ {false};
    bool readVisualStyleSeen_ {false};
    bool readRenderSettingsSeen_ {false};
    bool readRenderEnvironmentSeen_ {false};
    bool readRenderGlobalSeen_ {false};
    bool readRenderEntrySeen_ {false};
    bool readRenderRapidSeen_ {false};
    bool readRenderMentalSeen_ {false};
    bool readMaterialSeen_ {false};
    bool readDbColorSeen_ {false};
    bool readLightListSeen_ {false};
    bool readScaleSeen_ {false};
    bool readIDBufferSeen_ {false};
    bool readLayerIndexSeen_ {false};
    bool readSpatialIndexSeen_ {false};
    bool readTableStyleSeen_ {false};
    bool readSpatialFilterSeen_ {false};
    bool readGeoDataSeen_ {false};
    bool readGeoDataV2Seen_ {false};
    bool tableStyleExpected_ {false};
    bool readRawObjectInvariant_ {true};
    std::vector<std::uint32_t> readLocalRawObjectHandles_;
    bool readMalformedObjectSeen_ {false};
    bool readMalformedStyleSeen_ {false};
    bool readMalformedMLeaderStyleSeen_ {false};
    bool readMalformedDictionaryVarSeen_ {false};
    bool readMalformedDictionaryWithDefaultSeen_ {false};
    bool readMalformedSortEntsTableSeen_ {false};
    bool readMalformedFieldListSeen_ {false};
    bool readMalformedFieldSeen_ {false};
    bool readMalformedRasterVariablesSeen_ {false};
    bool readMalformedWipeoutVariablesSeen_ {false};
    bool readMalformedVisualStyleSeen_ {false};
    bool readMalformedRenderSettingsSeen_ {false};
    bool readMalformedRenderEnvironmentSeen_ {false};
    bool readMalformedRenderGlobalSeen_ {false};
    bool readMalformedRenderEntrySeen_ {false};
    bool readMalformedRenderRapidSeen_ {false};
    bool readMalformedRenderMentalSeen_ {false};
    bool readMalformedMaterialSeen_ {false};
    bool readMalformedDbColorSeen_ {false};
    bool readMalformedLightListSeen_ {false};
    bool readMalformedScaleSeen_ {false};
    bool readMalformedIDBufferSeen_ {false};
    bool readMalformedLayerIndexSeen_ {false};
    bool readMalformedSpatialIndexSeen_ {false};
    bool readMalformedTableStyleSeen_ {false};
    bool readMalformedSpatialFilterSeen_ {false};
    bool readMalformedGeoDataSeen_ {false};
    bool readPdfUnderlaySeen_ {false};
    bool readDgnUnderlaySeen_ {false};
    bool readDwfUnderlaySeen_ {false};
    bool readPointCloudDefinitionSeen_ {false};
    bool readPointCloudDefinitionExSeen_ {false};
    bool readPointCloudReactorSeen_ {false};
    bool readPointCloudReactorExSeen_ {false};
    bool readMalformedPointCloudSeen_ {false};
    bool readPointCloudColorMapSeen_ {false};
    bool readMalformedPointCloudColorMapSeen_ {false};
    bool readNavisworksModelDefSeen_ {false};
    bool readMalformedNavisworksModelDefSeen_ {false};
    bool readSunStudySeen_ {false};
    bool readMotionPathSeen_ {false};
    bool readMalformedSunStudySeen_ {false};
    bool readMalformedMotionPathSeen_ {false};
    bool readCurvePathSeen_ {false};
    bool readPointPathSeen_ {false};
    bool readObjectPtrSeen_ {false};
    bool readMalformedCurvePathSeen_ {false};
    bool readMalformedPointPathSeen_ {false};
    bool readMalformedObjectPtrSeen_ {false};
    bool readPartialViewingIndexSeen_ {false};
    bool readMalformedPartialViewingIndexSeen_ {false};
    bool readSolidBackgroundSeen_ {false};
    bool readGradientBackgroundSeen_ {false};
    bool readGroundPlaneBackgroundSeen_ {false};
    bool readImageBackgroundSeen_ {false};
    bool readIblBackgroundSeen_ {false};
    bool readSkylightBackgroundSeen_ {false};
    bool readMalformedBackgroundSeen_ {false};
    bool readSectionManagerSeen_ {false};
    bool readSectionSettingsSeen_ {false};
    bool readMalformedSectionSeen_ {false};
    bool readTvDevicePropertiesSeen_ {false};
    bool readVxControlSeen_ {false};
    bool readVxTableRecordSeen_ {false};
    bool readMalformedTvDevicePropertiesSeen_ {false};
    bool readMalformedVxControlSeen_ {false};
    bool readMalformedVxTableRecordSeen_ {false};
    bool readImageSeen_ {false};
    bool readImageDefSeen_ {false};
    bool readImageReactorSeen_ {false};
    std::uint32_t readImageHandle_ {0};
    DRW_Line readLine_;
    dx_data data_;
};

bool expect(bool value, const char* label, int& failures) {
    if (value)
        return true;
    ++failures;
    std::cerr << "FAIL: " << label << '\n';
    return false;
}

std::vector<std::uint8_t> makeLocalSabPayload() {
    std::vector<std::uint8_t> bytes;
    const auto putInt = [&bytes](std::int32_t value) {
        const auto* raw = reinterpret_cast<const std::uint8_t*>(&value);
        bytes.insert(bytes.end(), raw, raw + sizeof(value));
    };
    const auto putDouble = [&bytes](double value) {
        const auto* raw = reinterpret_cast<const std::uint8_t*>(&value);
        bytes.insert(bytes.end(), raw, raw + sizeof(value));
    };
    const auto putString = [&bytes](int tag, const std::string& value) {
        bytes.push_back(static_cast<std::uint8_t>(tag));
        bytes.push_back(static_cast<std::uint8_t>(value.size()));
        bytes.insert(bytes.end(), value.begin(), value.end());
    };
    const auto putVec = [&putDouble](double x, double y, double z) {
        putDouble(x);
        putDouble(y);
        putDouble(z);
    };
    const std::string signature = "ACIS BinaryFile";
    bytes.insert(bytes.end(), signature.begin(), signature.end());
    putInt(1); // SAB version
    putInt(2); // one payload record plus end marker
    putInt(1); // entity count
    putInt(0); // flags
    putString(DRW_SabTag::Str, "LOCAL_PRODUCT");
    putString(DRW_SabTag::Str, "LOCAL_ACIS");
    putString(DRW_SabTag::Str, "LOCAL_DATE");
    bytes.push_back(DRW_SabTag::Double);
    putDouble(1.0);
    bytes.push_back(DRW_SabTag::Double);
    putDouble(1.0e-6);
    bytes.push_back(DRW_SabTag::Double);
    putDouble(1.0e-6);
    putString(DRW_SabTag::EntityType, "vertex");
    bytes.push_back(DRW_SabTag::LocationVec);
    putVec(1.0, 2.0, 3.0);
    bytes.push_back(DRW_SabTag::RecordEnd);
    putString(DRW_SabTag::EntityType, "End-of-ACIS-data");
    bytes.push_back(DRW_SabTag::RecordEnd);
    return bytes;
}

bool runAcisSabFastCheck() {
    const std::vector<std::uint8_t> payload = makeLocalSabPayload();
    DRW_SabData sab;
    const bool parsed = drw_parseSab(payload.data(), payload.size(), sab);
    if (!parsed
        || sab.header.signature != "ACIS BinaryFile"
        || sab.header.numRecords != 2
        || sab.records.size() != 2
        || sab.records.front().type != "vertex"
        || sab.records.back().type != "End-of-ACIS-data") {
        return false;
    }
    const DRW_AcisModel model = drw_buildAcisModel(sab);
    if (model.nodes.size() != 2 || model.nodesOfType("vertex").size() != 1)
        return false;
    DRW_AcisBrep wireframe;
    const bool decoded = drw_decodeAcisWireframe(payload, wireframe);
    if (!decoded || wireframe.vertices.size() != 1)
        return false;
    std::vector<std::uint8_t> truncated(payload.begin(), payload.end() - 5);
    DRW_SabData rejected;
    const bool rejectedOk = !drw_parseSab(truncated.data(), truncated.size(), rejected);
    return rejectedOk;
}

bool runDxfModelerCarrierRoundTrip(DRW::Version version,
                                   const std::vector<std::uint8_t>& payload,
                                   const char* suffix, bool binary) {
    const std::filesystem::path output =
        std::filesystem::temp_directory_path()
        / (std::string("libdxfrw-modeler-") + suffix + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);
    dx_data source;
    auto* modeler = new DRW_ModelerGeometry(DRW::E3DSOLID);
    modeler->handle = 0xFC00u;
    modeler->m_modelerVersion = 7;
    modeler->m_rawBytes = payload;
    source.mBlock->ent.push_back(modeler);
    dx_iface exporter;
    const bool exportOk = exporter.fileExport(output.string(), version, binary,
                                               &source, false);
    if (!exportOk) {
        std::filesystem::remove(output, ec);
        return false;
    }
    dx_data imported;
    dx_iface importer;
    const bool importOk = importer.fileImport(output.string(), &imported, false);
    if (!importOk) {
        std::filesystem::remove(output, ec);
        return false;
    }
    bool found = false;
    std::vector<std::uint8_t> expectedPayload = payload;
    expectedPayload.push_back('\n');
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr || entity->eType != DRW::E3DSOLID)
            continue;
        const auto* decoded = static_cast<const DRW_ModelerGeometry*>(entity);
        bool chunksValid = !decoded->m_dxfPayloadChunks.empty();
        std::size_t expectedOffset = 0;
        for (const DRW_ModelerPayloadChunk& chunk : decoded->m_dxfPayloadChunks) {
            const bool textGroup = chunk.m_groupCode == 1 || chunk.m_groupCode == 3;
            chunksValid = chunksValid && chunk.m_offset == expectedOffset
                && chunk.m_offset <= decoded->m_rawBytes.size()
                && chunk.m_length <= decoded->m_rawBytes.size() - chunk.m_offset
                && textGroup;
            expectedOffset += chunk.m_length;
        }
        found = chunksValid && expectedOffset + 1 == decoded->m_rawBytes.size()
            && decoded->m_rawBytes.back() == '\n'
            && decoded->handle != 0
            && decoded->m_modelerVersion == 7
            && decoded->m_rawBytes == expectedPayload;
    }
    std::filesystem::remove(output, ec);
    return found;
}

bool runDxfModelerEnvelopeRoundTrip(
    bool binary, const std::filesystem::path& directory, bool keepOutput) {
    const std::string encoding = binary ? "binary" : "ascii";
    const std::filesystem::path output = directory
        / ("libdxfrw-ac1027-modeler-envelope-" + encoding + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    dx_data source;
    auto* historyTarget = new DRW_ModelerGeometry(DRW::E3DSOLID);
    historyTarget->handle = 0xFC10u;
    historyTarget->m_hasDxfModelerFlag = true;
    historyTarget->m_dxfModelerFlag = false;
    historyTarget->m_hasDxfModelerUid = true;
    historyTarget->m_dxfModelerUid =
        "{00000000-0000-0000-0000-000000000000}";
    source.mBlock->ent.push_back(historyTarget);

    auto* solid = new DRW_ModelerGeometry(DRW::E3DSOLID);
    solid->handle = 0xFC11u;
    solid->m_hasDxfModelerFlag = true;
    solid->m_dxfModelerFlag = true;
    solid->m_hasDxfModelerUid = true;
    solid->m_dxfModelerUid =
        "{1a113328-eb6d-d44d-824d-78b33668f9e7}";
    solid->m_historyHandle = historyTarget->handle;
    source.mBlock->ent.push_back(solid);

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary,
                             &source, false)) {
        std::filesystem::remove(output, ec);
        return false;
    }

    bool omitsLegacyVersion = true;
    if (!binary) {
        std::ifstream input(output);
        std::string codeLine;
        std::string valueLine;
        std::string section;
        std::string currentRecord;
        bool waitingForSectionName = false;
        bool foundModeler = false;
        bool foundVersionGroup = false;
        const auto trim = [](std::string& value) {
            const std::size_t first = value.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                value.clear();
                return;
            }
            const std::size_t last = value.find_last_not_of(" \t\r\n");
            value = value.substr(first, last - first + 1);
        };
        while (std::getline(input, codeLine)
               && std::getline(input, valueLine)) {
            trim(codeLine);
            trim(valueLine);
            if (waitingForSectionName && codeLine == "2") {
                section = valueLine;
                waitingForSectionName = false;
                continue;
            }
            if (codeLine == "0" && valueLine == "SECTION") {
                waitingForSectionName = true;
                currentRecord.clear();
                continue;
            }
            if (codeLine == "0" && valueLine == "ENDSEC") {
                section.clear();
                currentRecord.clear();
                continue;
            }
            if (codeLine == "0")
                currentRecord = valueLine;
            if (section == "ENTITIES" && currentRecord == "3DSOLID") {
                foundModeler = true;
                if (codeLine == "70")
                    foundVersionGroup = true;
            }
        }
        omitsLegacyVersion = input.eof() && foundModeler
            && !foundVersionGroup;
    }

    dx_data imported;
    dx_iface importer;
    const bool importOk = importer.fileImport(output.string(), &imported,
                                               false);
    const DRW_ModelerGeometry* importedTarget = nullptr;
    const DRW_ModelerGeometry* importedSolid = nullptr;
    if (importOk) {
        for (const DRW_Entity* entity : imported.mBlock->ent) {
            if (entity == nullptr || entity->eType != DRW::E3DSOLID)
                continue;
            const auto* modeler =
                static_cast<const DRW_ModelerGeometry*>(entity);
            if (importedTarget == nullptr)
                importedTarget = modeler;
            else if (importedSolid == nullptr)
                importedSolid = modeler;
        }
    }
    const bool result = importOk && omitsLegacyVersion
        && importedTarget != nullptr && importedSolid != nullptr
        && importedTarget->handle != 0xFC10u
        && importedTarget->m_hasDxfModelerFlag
        && !importedTarget->m_dxfModelerFlag
        && importedTarget->m_hasDxfModelerUid
        && importedTarget->m_dxfModelerUid
            == "{00000000-0000-0000-0000-000000000000}"
        && importedSolid->handle != 0xFC11u
        && importedSolid->m_hasDxfModelerFlag
        && importedSolid->m_dxfModelerFlag
        && importedSolid->m_hasDxfModelerUid
        && importedSolid->m_dxfModelerUid
            == "{1a113328-eb6d-d44d-824d-78b33668f9e7}"
        && importedSolid->m_historyHandle == importedTarget->handle;
    if (!keepOutput || !result)
        std::filesystem::remove(output, ec);
    return result;
}

DRW_RawDxfSection makeLocalAcdsDataSection(
    const std::string& modelerHandle, bool includeOwnerHandle = true,
    bool duplicateOwnerHandle = false) {
    DRW_RawDxfSection section;
    section.m_name = "ACDSDATA";
    section.m_version = DRW::AC1027;
    section.m_groups.emplace_back(0, "ACDSSCHEMA");
    section.m_groups.emplace_back(90, static_cast<std::int32_t>(7));
    section.m_groups.emplace_back(1, "LocalGeneratedSchema");
    section.m_groups.emplace_back(0, "ACDSRECORD");
    section.m_groups.emplace_back(90, static_cast<std::int32_t>(9));
    section.m_groups.emplace_back(2, "AcDbDs::ID");
    section.m_groups.emplace_back(280, static_cast<std::int32_t>(10));
    if (includeOwnerHandle)
        section.m_groups.emplace_back(320, modelerHandle);
    if (duplicateOwnerHandle)
        section.m_groups.emplace_back(320, modelerHandle);
    section.m_groups.emplace_back(2, "ASM_Data");
    section.m_groups.emplace_back(280, static_cast<std::int32_t>(15));
    section.m_groups.emplace_back(94, static_cast<std::int32_t>(4));
    section.m_groups.emplace_back(310, std::string("41434453"));
    section.m_groups.emplace_back(0, "ACDSRECORD");
    section.m_groups.emplace_back(90, static_cast<std::int32_t>(10));
    section.m_groups.emplace_back(2, "OpaqueRecord");
    section.m_groups.emplace_back(320, std::string("FC21"));
    return section;
}

DRW_DataStorageSection makeLocalDataStorageSection(
    std::uint32_t handle, const std::vector<std::uint8_t>& payload) {
    DRW_DataStorageSection section;
    section.m_name = "AcDb:AcDsPrototype_1b";
    section.m_version = DRW::AC1027;
    section.payloadsRetained = true;
    section.schemaCount = 6;
    const std::array<const char*, 7> schemaPropertyNames = {
        "AcDbDs::ID", "Thumbnail_Data", "ASM_Data",
        "AcDbDs::TreatedAsObjectData", "AcDbDs::Legacy",
        "AcDs:Indexable", "AcDbDs::HandleAttribute"};
    section.schemaPropertyNameCount =
        static_cast<std::uint32_t>(schemaPropertyNames.size());
    for (const char* name : schemaPropertyNames)
        section.schemaPropertyNames.emplace_back(name);
    for (std::uint32_t index = 0; index < 6u; ++index) {
        DRW_DataStorageSchema schema;
        schema.index = index;
        const auto property = [](std::uint32_t nameIndex, const char* name,
                                 std::uint32_t type, std::uint32_t flags,
                                 std::uint16_t valueCount,
                                 std::vector<std::vector<std::uint8_t>> values) {
            DRW_DataStorageSchemaProperty result;
            result.nameIndex = nameIndex;
            result.name = name;
            result.type = type;
            result.flags = flags;
            result.valueCount = valueCount;
            result.values = std::move(values);
            return result;
        };
        switch (index) {
        case 0:
            schema.indexes = {4u, 5u};
            schema.properties.push_back(property(
                0u, schemaPropertyNames[0], 10u, 0u, 2u,
                {{6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u},
                 {7u, 0u, 0u, 0u, 0u, 0u, 0u, 0u}}));
            schema.properties.push_back(property(
                1u, schemaPropertyNames[1], 15u, 0u, 0u, {}));
            break;
        case 1:
            schema.indexes = {0u, 1u};
            schema.properties.push_back(property(
                0u, schemaPropertyNames[0], 10u, 0u, 2u,
                {{2u, 0u, 0u, 0u, 0u, 0u, 0u, 0u},
                 {3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u}}));
            schema.properties.push_back(property(
                2u, schemaPropertyNames[2], 15u, 0u, 0u, {}));
            break;
        case 2:
            schema.properties.push_back(property(
                3u, schemaPropertyNames[3], 1u, 0u, 0u, {}));
            break;
        case 3:
            schema.properties.push_back(property(
                4u, schemaPropertyNames[4], 1u, 0u, 0u, {}));
            break;
        case 4:
            schema.properties.push_back(property(
                5u, schemaPropertyNames[5], 1u, 0u, 0u, {}));
            break;
        case 5:
            schema.properties.push_back(property(
                6u, schemaPropertyNames[6], 7u, 8u, 1u, {{0u}}));
            break;
        }
        section.schemas.push_back(std::move(schema));
    }
    DRW_DataStorageRecord record;
    record.handle = handle;
    char handleText[9] = {};
    std::snprintf(handleText, sizeof(handleText), "%X", handle);
    record.handleKey = handleText;
    record.isHandleSafe = true;
    record.schemaIndex = 1;
    record.dataByteLength = static_cast<std::uint32_t>(payload.size());
    record.payload = payload;
    record.hasPayloadMarker = true;
    record.payloadMarkerOffset = 0;
    record.payloadMarkerLength = 15;
    record.payloadMarkerSection = "datastore";
    section.records.push_back(std::move(record));
    return section;
}

DRW_DataStorageSection makeLocalAcadSharpDataStorageSection(
    std::uint32_t handle, const std::vector<std::uint8_t>& payload) {
    DRW_DataStorageSection section =
        makeLocalDataStorageSection(handle, payload);
    static const std::array<const char*, 8> propertyNames = {
        "AcDbDs::ID", "Thumbnail_Data", "AcDbDs::TreatedAsObjectData",
        "AcDbDs::Legacy", "AcDs:Indexable", "AcDbDs::HandleAttribute",
        "AcDbDs::ID", "ASM_Data"};
    section.schemaPropertyNameCount = 2u;
    section.schemaPropertyNames.clear();
    for (const char* name : propertyNames)
        section.schemaPropertyNames.emplace_back(name);
    if (section.schemas.size() != 6u)
        return section;

    const auto property = [](std::uint32_t nameIndex, const char* name,
                             std::uint32_t type, std::uint32_t flags,
                             std::uint16_t valueCount,
                             std::vector<std::vector<std::uint8_t>> values) {
        DRW_DataStorageSchemaProperty result;
        result.nameIndex = nameIndex;
        result.name = name;
        result.type = type;
        result.flags = flags;
        result.valueCount = valueCount;
        result.values = std::move(values);
        return result;
    };
    for (std::size_t index = 0; index < section.schemas.size(); ++index) {
        DRW_DataStorageSchema& schema = section.schemas[index];
        schema.segmentIndex = index == 5u ? 631u : 5u;
        schema.indexes.clear();
        schema.properties.clear();
        switch (index) {
        case 0:
            schema.indexes = {0u, 1u};
            schema.properties.push_back(property(
                0u, propertyNames[0], 10u, 0u, 2u,
                {{6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u},
                 {7u, 0u, 0u, 0u, 0u, 0u, 0u, 0u}}));
            schema.properties.push_back(property(
                1u, propertyNames[1], 15u, 0u, 0u, {}));
            break;
        case 1:
            schema.properties.push_back(property(
                2u, propertyNames[2], 1u, 0u, 0u, {}));
            break;
        case 2:
            schema.properties.push_back(property(
                3u, propertyNames[3], 1u, 0u, 0u, {}));
            break;
        case 3:
            schema.properties.push_back(property(
                4u, propertyNames[4], 1u, 0u, 0u, {}));
            break;
        case 4:
            schema.properties.push_back(property(
                5u, propertyNames[5], 7u, 8u, 1u, {{0u}}));
            break;
        case 5:
            schema.indexes = {6u, 4u};
            schema.properties.push_back(property(
                0u, propertyNames[6], 10u, 0u, 2u,
                {{2u, 0u, 0u, 0u, 0u, 0u, 0u, 0u},
                 {3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u}}));
            schema.properties.push_back(property(
                1u, propertyNames[7], 15u, 0u, 0u, {}));
            break;
        }
    }
    section.records.front().schemaIndex = 5u;
    return section;
}

void addLocalAcdsHistoryClosure(dx_data& source,
                               DRW_ModelerGeometry& modeler,
                               std::uint32_t history = 0x20Eu,
                               std::uint32_t evaluation = 0x20Du,
                               std::uint32_t operation = 0x20Cu,
                               std::uint16_t operationClassId = 519u,
                               const char* operationRecordName =
                                   "ACSH_CONE_CLASS",
                               const char* operationClassName =
                                   "AcDbShCone",
                               const char* operationSubclass =
                                   "cn:AcDbShCone") {
    constexpr std::uint32_t material = 0x96u;
    modeler.m_historyHandle = history;

    const auto proxy = [](std::uint32_t handle, std::uint32_t owner,
                          std::int32_t classId, std::uint32_t bitSize,
                          const char* subclass, int referenceCode,
                          std::uint32_t referenceHandle) {
        DRW_ProxyObject value;
        value.handle = handle;
        value.parentHandle = owner;
        value.m_hasProxyCarrierId = true;
        value.m_proxyCarrierId = DRW_ProxyObject::kDwgType;
        value.m_hasProxyClassId = true;
        value.m_proxyClassId = classId;
        value.m_proxySubclass = subclass;
        value.m_hasProxyDrawingFormat = true;
        value.m_proxyDrawingFormat = 983063u;
        value.m_hasObjectDataBitSize = true;
        value.m_objectDataBitSize = bitSize;
        value.m_objectData.resize((bitSize + 7u) / 8u,
                                  static_cast<std::uint8_t>(handle));
        if ((bitSize & 7u) != 0u)
            value.m_objectData.back() &= static_cast<std::uint8_t>(
                0xFFu << (8u - (bitSize & 7u)));
        DRW_ProxyObjectIdRef reference;
        reference.m_dxfCode = referenceCode;
        reference.m_handleCode = referenceCode == 340 ? 5 : 3;
        reference.m_handle = referenceHandle;
        reference.m_rawHandle = referenceHandle;
        value.m_objectIdRefs.push_back(reference);
        return value;
    };
    source.proxyObjects.push_back(proxy(history, modeler.handle, 521, 32,
                                        "cn:AcDbShHistory",
                                        360, evaluation));
    source.proxyObjects.push_back(proxy(evaluation, history, 520, 190,
                                        "cn:AcDbEvalGraph",
                                        360, operation));
    source.proxyObjects.push_back(proxy(operation, evaluation,
                                        operationClassId, 558,
                                        operationSubclass,
                                        340, material));

    source.dwgClassCoverage.m_status =
        DRW_DwgClassCoverageStatus::FinalizedComplete;
    source.dwgClassCoverage.m_complete = true;
    const std::array<std::tuple<std::uint16_t, const char*, const char*>, 3>
        classes {{
            {521u, "ACSH_HISTORY_CLASS", "AcDbShHistory"},
            {520u, "ACAD_EVALUATION_GRAPH", "AcDbEvalGraph"},
            {operationClassId, operationRecordName, operationClassName}
        }};
    for (const auto& cls : classes) {
        const bool exists = std::any_of(
            source.dwgClassCoverage.m_entries.begin(),
            source.dwgClassCoverage.m_entries.end(),
            [&cls](const DRW_DwgClassCoverageEntry& entry) {
                return entry.m_recordName == std::get<1>(cls)
                    && entry.m_className == std::get<2>(cls);
            });
        if (exists)
            continue;
        DRW_DwgClassCoverageEntry entry;
        entry.m_classNumber = std::get<0>(cls);
        entry.m_recordName = std::get<1>(cls);
        entry.m_className = std::get<2>(cls);
        entry.m_entityFlagRaw = 0x1F3;
        entry.m_state = DRW_DwgClassCoverageState::Published;
        source.dwgClassCoverage.m_entries.push_back(std::move(entry));
    }

    const bool hasMaterialsDictionary = std::any_of(
        source.dictionaries.begin(), source.dictionaries.end(),
        [](const DRW_Dictionary& dictionary) {
            return dictionary.handle == 0x72u;
        });
    if (!hasMaterialsDictionary) {
        DRW_Dictionary materialsDictionary;
        materialsDictionary.handle = 0x72u;
        materialsDictionary.parentHandle = 0xCu;
        materialsDictionary.cloning = 1;
        materialsDictionary.m_entries.push_back({"ByLayer", 0x96u});
        materialsDictionary.m_entries.push_back({"ByBlock", 0x97u});
        materialsDictionary.m_entries.push_back({"Global", 0x98u});
        source.dictionaries.push_back(std::move(materialsDictionary));
    }
    for (const auto& materialInfo : {
             std::pair<std::uint32_t, const char*>{0x96u, "ByLayer"},
             {0x97u, "ByBlock"}, {0x98u, "Global"}}) {
        const bool exists = std::any_of(
            source.materials.begin(), source.materials.end(),
            [&materialInfo](const DRW_Material& materialRecord) {
                return materialRecord.handle == materialInfo.first;
            });
        if (exists)
            continue;
        DRW_Material value;
        value.handle = materialInfo.first;
        value.parentHandle = 0x72u;
        value.m_name = materialInfo.second;
        source.materials.push_back(std::move(value));
    }
}

DRW_ModelerGeometry* addLocalAcdsDataStorageModeler(
    dx_data& source, std::uint32_t handle,
    const std::vector<std::uint8_t>& payload,
    DRW::ETYPE entityType = DRW::E3DSOLID,
    bool addHistoryClosure = true) {
    auto* modeler = new DRW_ModelerGeometry(entityType);
    modeler->handle = handle;
    modeler->m_modelerVersion = 2;
    modeler->m_dwgSourceVersion = DRW::AC1027;
    modeler->m_hasModelerData = true;
    modeler->setHasDataStorageBinaryData(true);
    modeler->hasDataStorageRecord = true;
    modeler->dataStorageHandle = handle;
    char handleText[9] = {};
    std::snprintf(handleText, sizeof(handleText), "%X", handle);
    modeler->dataStorageHandleKey = handleText;
    modeler->dataStorageSchemaIndex = 1;
    modeler->dataStorageData = payload;
    if (addHistoryClosure)
        addLocalAcdsHistoryClosure(source, *modeler);
    source.mBlock->ent.push_back(modeler);
    source.dataStorageSections.push_back(
        makeLocalDataStorageSection(handle, payload));
    return modeler;
}

bool findAcdsRecordPayload(const DRW_RawDxfSection& section,
                           const std::string& recordName,
                           std::string& ownerHandle,
                           std::vector<std::uint8_t>& payload) {
    const auto equalsAsciiCaseInsensitive = [](const std::string& lhs,
                                                const char* rhs) {
        if (rhs == nullptr || lhs.size() != std::strlen(rhs))
            return false;
        for (std::size_t index = 0; index < lhs.size(); ++index) {
            if (std::toupper(static_cast<unsigned char>(lhs[index]))
                != std::toupper(static_cast<unsigned char>(rhs[index])))
                return false;
        }
        return true;
    };
    const auto nibble = [](unsigned char value) -> int {
        if (value >= '0' && value <= '9')
            return value - '0';
        if (value >= 'A' && value <= 'F')
            return value - 'A' + 10;
        if (value >= 'a' && value <= 'f')
            return value - 'a' + 10;
        return -1;
    };
    for (std::size_t start = 0; start < section.m_groups.size();) {
        const DRW_Variant& marker = section.m_groups[start];
        if (marker.code() != 0 || marker.type() != DRW_Variant::STRING
            || marker.c_str() == nullptr) {
            ++start;
            continue;
        }
        std::size_t end = start + 1u;
        while (end < section.m_groups.size()
               && section.m_groups[end].code() != 0)
            ++end;
        if (!equalsAsciiCaseInsensitive(marker.c_str(), "ACDSRECORD")) {
            start = end;
            continue;
        }
        bool inId = false;
        bool hasPayloadName = false;
        std::size_t ownerCount = 0;
        std::string candidateOwner;
        std::vector<std::uint8_t> candidatePayload;
        for (std::size_t index = start + 1u; index < end; ++index) {
            const DRW_Variant& group = section.m_groups[index];
            if (group.code() == 2
                && group.type() == DRW_Variant::STRING
                && group.c_str() != nullptr) {
                inId = equalsAsciiCaseInsensitive(group.c_str(),
                                                  "AcDbDs::ID");
                hasPayloadName = hasPayloadName
                    || equalsAsciiCaseInsensitive(group.c_str(),
                                                  recordName.c_str());
            } else if (inId && group.code() == 320) {
                ++ownerCount;
                if (group.type() != DRW_Variant::STRING
                    || group.c_str() == nullptr)
                    return false;
                candidateOwner = group.c_str();
            } else if (hasPayloadName && group.code() == 310) {
                if (group.type() != DRW_Variant::STRING
                    || group.c_str() == nullptr)
                    return false;
                const std::string hex = group.c_str();
                if ((hex.size() & 1u) != 0u)
                    return false;
                for (std::size_t offset = 0; offset < hex.size(); offset += 2u) {
                    const int high = nibble(
                        static_cast<unsigned char>(hex[offset]));
                    const int low = nibble(
                        static_cast<unsigned char>(hex[offset + 1u]));
                    if (high < 0 || low < 0)
                        return false;
                    candidatePayload.push_back(static_cast<std::uint8_t>(
                        (static_cast<unsigned int>(high) << 4u)
                        | static_cast<unsigned int>(low)));
                }
            }
        }
        if (hasPayloadName) {
            if (ownerCount != 1u)
                return false;
            ownerHandle = std::move(candidateOwner);
            payload = std::move(candidatePayload);
            return true;
        }
        start = end;
    }
    return false;
}

bool hasExpectedAcdsSchemaDefinitions(const DRW_RawDxfSection& section) {
    using Field = std::pair<int, std::string>;
    const std::array<std::vector<Field>, 6> expected = {{
        {{0, "ACDSSCHEMA"}, {90, "0"}, {1, "AcDb_Thumbnail_Schema"},
         {2, "AcDbDs::ID"}, {280, "10"}, {91, "8"},
         {2, "Thumbnail_Data"}, {280, "15"}, {91, "0"}},
        {{0, "ACDSSCHEMA"}, {90, "1"}, {1, "AcDb3DSolid_ASM_Data"},
         {2, "AcDbDs::ID"}, {280, "10"}, {91, "8"},
         {2, "ASM_Data"}, {280, "15"}, {91, "0"}},
        {{0, "ACDSSCHEMA"}, {90, "2"},
         {1, "AcDbDs::TreatedAsObjectDataSchema"},
         {2, "AcDbDs::TreatedAsObjectData"}, {280, "1"}, {91, "0"}},
        {{0, "ACDSSCHEMA"}, {90, "3"}, {1, "AcDbDs::LegacySchema"},
         {2, "AcDbDs::Legacy"}, {280, "1"}, {91, "0"}},
        {{0, "ACDSSCHEMA"}, {90, "4"},
         {1, "AcDbDs::IndexedPropertySchema"},
         {2, "AcDs:Indexable"}, {280, "1"}, {91, "0"}},
        {{0, "ACDSSCHEMA"}, {90, "5"},
         {1, "AcDbDs::HandleAttributeSchema"},
         {2, "AcDbDs::HandleAttribute"}, {280, "7"}, {91, "1"},
         {284, "1"}}
    }};
    std::size_t schemaIndex = 0;
    for (std::size_t start = 0; start < section.m_groups.size();) {
        const DRW_Variant& marker = section.m_groups[start];
        if (marker.code() != 0 || marker.type() != DRW_Variant::STRING
            || marker.c_str() == nullptr) {
            ++start;
            continue;
        }
        std::size_t end = start + 1u;
        while (end < section.m_groups.size()
               && section.m_groups[end].code() != 0)
            ++end;
        if (std::strcmp(marker.c_str(), "ACDSSCHEMA") == 0) {
            if (schemaIndex >= expected.size())
                return false;
            std::size_t headerEnd = start + 1u;
            while (headerEnd < end) {
                const DRW_Variant& group = section.m_groups[headerEnd];
                if (group.code() == 101
                    && group.type() == DRW_Variant::STRING
                    && group.c_str() != nullptr
                    && std::strcmp(group.c_str(), "ACDSRECORD") == 0) {
                    break;
                }
                ++headerEnd;
            }
            std::vector<Field> actual;
            for (std::size_t index = start; index < headerEnd; ++index) {
                const DRW_Variant& group = section.m_groups[index];
                if (group.type() == DRW_Variant::STRING
                    && group.c_str() != nullptr) {
                    actual.emplace_back(group.code(), group.c_str());
                } else if (group.type() == DRW_Variant::INTEGER) {
                    actual.emplace_back(group.code(),
                                        std::to_string(group.i_val()));
                } else {
                    return false;
                }
            }
            if (actual != expected[schemaIndex])
                return false;
            ++schemaIndex;
        }
        start = end;
    }
    return schemaIndex == expected.size();
}

bool runDxfAcdsMultipleDataStorageProjection(
    bool binary, const std::filesystem::path& directory, bool keepOutput) {
    const std::filesystem::path output = directory /
        (std::string("libdxfrw-ac1027-acds-multiple-")
         + (binary ? "binary" : "ascii") + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    const auto sab = [](std::uint8_t suffix) {
        return std::vector<std::uint8_t>{
            'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e',
            suffix, static_cast<std::uint8_t>(suffix + 1u),
            static_cast<std::uint8_t>(suffix + 2u)};
    };
    const std::vector<std::uint8_t> boxPayload = sab(0x21u);
    const std::vector<std::uint8_t> extrusionPayload = sab(0x31u);
    const std::vector<std::uint8_t> regionPayload = sab(0x41u);
    const std::vector<std::uint8_t> pngPayload{
        0x89u, 'P', 'N', 'G', 0x0Du, 0x0Au, 0x1Au, 0x0Au,
        0x00u, 0x00u, 0x00u, 0x0Du, 'I', 'H', 'D', 'R',
        0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x01u,
        0x08u, 0x06u, 0x00u, 0x00u, 0x00u, 0x1Fu, 0x15u, 0xC4u, 0x89u,
        0x00u, 0x00u, 0x00u, 0x0Bu, 'I', 'D', 'A', 'T',
        0x78u, 0xDAu, 0x63u, 0x64u, 0x00u, 0x02u, 0x00u, 0x00u,
        0x05u, 0x00u, 0x01u, 0xA5u, 0x50u, 0x20u, 0xA6u,
        0x00u, 0x00u, 0x00u, 0x00u, 'I', 'E', 'N', 'D',
        0xAEu, 0x42u, 0x60u, 0x82u};

    dx_data source;
    const auto addModeler = [&source](
        DRW::ETYPE type, std::uint32_t handle,
        const std::vector<std::uint8_t>& payload) {
        auto* modeler = new DRW_ModelerGeometry(type);
        modeler->handle = handle;
        modeler->m_modelerVersion = 2;
        modeler->m_dwgSourceVersion = DRW::AC1027;
        modeler->m_hasModelerData = true;
        modeler->setHasDataStorageBinaryData(true);
        modeler->hasDataStorageRecord = true;
        modeler->dataStorageHandle = handle;
        char handleText[9] = {};
        std::snprintf(handleText, sizeof(handleText), "%X", handle);
        modeler->dataStorageHandleKey = handleText;
        // This is a DWG-internal lookup index, not the DXF ACDSRECORD code.
        modeler->dataStorageSchemaIndex = 5u;
        modeler->dataStorageData = payload;
        source.mBlock->ent.push_back(modeler);
        return modeler;
    };
    auto* box = addModeler(DRW::E3DSOLID, 0xFC20u, boxPayload);
    auto* extrusion = addModeler(
        DRW::E3DSOLID, 0xFC21u, extrusionPayload);
    auto* region = addModeler(DRW::REGION, 0xFC22u, regionPayload);
    addLocalAcdsHistoryClosure(source, *box, 0x30Eu, 0x30Du, 0x30Cu,
                               546u, "ACSH_BOX_CLASS", "AcDbShBox",
                               "cn:AcDbShBox");
    addLocalAcdsHistoryClosure(source, *extrusion, 0x40Eu, 0x40Du, 0x40Cu,
                               547u, "ACSH_EXTRUSION_CLASS",
                               "AcDbShExtrusion", "cn:AcDbShExtrusion");

    DRW_DataStorageSection storage = makeLocalAcadSharpDataStorageSection(
        box->handle, boxPayload);
    DRW_DataStorageRecord extrusionRecord =
        makeLocalAcadSharpDataStorageSection(extrusion->handle, extrusionPayload)
            .records.front();
    storage.records.push_back(std::move(extrusionRecord));
    DRW_DataStorageRecord regionRecord =
        makeLocalAcadSharpDataStorageSection(region->handle, regionPayload)
            .records.front();
    storage.records.push_back(std::move(regionRecord));
    for (const std::uint32_t handle : {0xA602u, 0xA770u}) {
        DRW_DataStorageRecord thumbnail;
        thumbnail.handle = handle;
        char handleText[9] = {};
        std::snprintf(handleText, sizeof(handleText), "%X", handle);
        thumbnail.handleKey = handleText;
        thumbnail.isHandleSafe = true;
        thumbnail.schemaIndex = 0u;
        thumbnail.dataByteLength =
            static_cast<std::uint32_t>(pngPayload.size());
        thumbnail.payload = pngPayload;
        storage.records.push_back(std::move(thumbnail));
    }
    storage.orphanRecordCount = 2u;
    source.dataStorageSections.push_back(std::move(storage));

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary,
                             &source, false)) {
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }
    dx_data imported;
    dx_iface importer;
    if (!importer.fileImport(output.string(), &imported, false)
        || imported.rawDxfSections.size() != 1u
        || imported.rawDxfSections.front().m_name != "ACDSDATA") {
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }

    const auto decodeHex = [](const std::string& text,
                              std::vector<std::uint8_t>& bytes) {
        if ((text.size() & 1u) != 0u)
            return false;
        const auto nibble = [](unsigned char value) {
            if (value >= '0' && value <= '9')
                return static_cast<int>(value - '0');
            if (value >= 'A' && value <= 'F')
                return static_cast<int>(value - 'A' + 10);
            if (value >= 'a' && value <= 'f')
                return static_cast<int>(value - 'a' + 10);
            return -1;
        };
        for (std::size_t index = 0; index < text.size(); index += 2u) {
            const int high = nibble(static_cast<unsigned char>(text[index]));
            const int low = nibble(static_cast<unsigned char>(text[index + 1u]));
            if (high < 0 || low < 0)
                return false;
            bytes.push_back(static_cast<std::uint8_t>((high << 4) | low));
        }
        return true;
    };
    const std::map<std::string, std::vector<std::uint8_t>> expectedThumbnails{
        {"A602", pngPayload}, {"A770", pngPayload}};
    std::map<std::string, std::vector<std::uint8_t>> observedRecords;
    const auto& groups = imported.rawDxfSections.front().m_groups;
    bool recordsValid = true;
    std::size_t asmRecordCount = 0;
    std::size_t thumbnailRecordCount = 0;
    for (std::size_t start = 0; start < groups.size();) {
        if (groups[start].code() != 0
            || groups[start].type() != DRW_Variant::STRING
            || groups[start].c_str() == nullptr
            || std::strcmp(groups[start].c_str(), "ACDSRECORD") != 0) {
            ++start;
            continue;
        }
        std::size_t end = start + 1u;
        while (end < groups.size() && groups[end].code() != 0)
            ++end;
        std::string key;
        std::string propertyName;
        std::vector<std::uint8_t> payload;
        std::uint32_t declaredLength = 0;
        std::int32_t schemaCode = -1;
        bool inId = false;
        for (std::size_t index = start + 1u; index < end; ++index) {
            const DRW_Variant& group = groups[index];
            if (group.code() == 90
                && group.type() == DRW_Variant::INTEGER) {
                schemaCode = static_cast<std::int32_t>(group.i_val());
            } else if (group.code() == 2
                       && group.type() == DRW_Variant::STRING
                       && group.c_str() != nullptr) {
                inId = std::strcmp(group.c_str(), "AcDbDs::ID") == 0;
                if (!inId)
                    propertyName = group.c_str();
            } else if (inId && group.code() == 320
                       && group.type() == DRW_Variant::STRING
                       && group.c_str() != nullptr) {
                key = group.c_str();
            } else if (group.code() == 94
                       && group.type() == DRW_Variant::INTEGER
                       && group.i_val() >= 0) {
                declaredLength = static_cast<std::uint32_t>(group.i_val());
            } else if ((propertyName == "ASM_Data"
                        || propertyName == "Thumbnail_Data")
                       && group.code() == 310
                       && group.type() == DRW_Variant::STRING
                       && group.c_str() != nullptr) {
                recordsValid = recordsValid && decodeHex(group.c_str(), payload);
            }
        }
        if (propertyName == "ASM_Data") {
            ++asmRecordCount;
            recordsValid = recordsValid && schemaCode == 1;
        } else if (propertyName == "Thumbnail_Data") {
            ++thumbnailRecordCount;
            recordsValid = recordsValid && schemaCode == 0;
        } else {
            recordsValid = false;
        }
        if (key.empty() || payload.size() != declaredLength
            || !observedRecords.emplace(key, std::move(payload)).second) {
            recordsValid = false;
        }
        start = end;
    }

    std::vector<std::string> modelerHandleKeys;
    std::size_t solidCount = 0;
    std::size_t regionCount = 0;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr)
            continue;
        if (entity->eType == DRW::E3DSOLID)
            ++solidCount;
        else if (entity->eType == DRW::REGION)
            ++regionCount;
        else
            continue;
        const auto* modeler = static_cast<const DRW_ModelerGeometry*>(entity);
        char handleText[9] = {};
        std::snprintf(handleText, sizeof(handleText), "%X", modeler->handle);
        modelerHandleKeys.emplace_back(handleText);
    }
    std::sort(modelerHandleKeys.begin(), modelerHandleKeys.end());
    std::vector<std::vector<std::uint8_t>> modelerSabPayloads;
    bool modelerAssociationsMatch = modelerHandleKeys.size() == 3u;
    for (const std::string& handleKey : modelerHandleKeys) {
        const auto record = observedRecords.find(handleKey);
        if (record == observedRecords.end()
            || record->second.size() < 15u
            || !std::equal(boxPayload.begin(), boxPayload.begin() + 15,
                           record->second.begin())) {
            modelerAssociationsMatch = false;
            continue;
        }
        modelerSabPayloads.push_back(record->second);
    }
    std::sort(modelerSabPayloads.begin(), modelerSabPayloads.end());
    std::vector<std::vector<std::uint8_t>> expectedModelerSabPayloads{
        boxPayload, extrusionPayload, regionPayload};
    std::sort(expectedModelerSabPayloads.begin(),
              expectedModelerSabPayloads.end());
    const bool thumbnailsMatch =
        observedRecords.size() == modelerHandleKeys.size()
                                  + expectedThumbnails.size()
        && std::all_of(
            expectedThumbnails.begin(), expectedThumbnails.end(),
            [&observedRecords](const auto& expected) {
                const auto record = observedRecords.find(expected.first);
                return record != observedRecords.end()
                    && record->second == expected.second;
            });

    const std::array<std::pair<const char*, int>, 5> expectedClasses{{
        {"ACSH_HISTORY_CLASS", 2}, {"ACAD_EVALUATION_GRAPH", 2},
        {"ACSH_BOX_CLASS", 1}, {"ACSH_EXTRUSION_CLASS", 1},
        {"MATERIAL", 3}}};
    bool classesMatch = imported.dxfClasses.size() == expectedClasses.size();
    if (classesMatch) {
        for (std::size_t index = 0; index < expectedClasses.size(); ++index) {
            const DRW_Class& cls = imported.dxfClasses[index];
            if (cls.recName != expectedClasses[index].first
                || cls.instanceCount != expectedClasses[index].second) {
                classesMatch = false;
                break;
            }
        }
    }
    const bool result = recordsValid && observedRecords.size() == 5u
        && asmRecordCount == 3u && thumbnailRecordCount == 2u
        && solidCount == 2u && regionCount == 1u
        && modelerAssociationsMatch
        && modelerSabPayloads == expectedModelerSabPayloads
        && thumbnailsMatch
        && imported.proxyObjects.size() == 6u
        && imported.rawProxyObjects.size() == 6u && classesMatch
        && hasExpectedAcdsSchemaDefinitions(imported.rawDxfSections.front());
    if (!result) {
        std::cerr << "ACDS multi-projection details: recordsValid="
                  << recordsValid << " observedRecords="
                  << observedRecords.size() << " ASM=" << asmRecordCount
                  << " thumbnails=" << thumbnailRecordCount
                  << " solids=" << solidCount << " regions=" << regionCount
                  << " modeler associations=" << modelerAssociationsMatch
                  << " SABs=" << modelerSabPayloads.size() << " proxies="
                  << imported.proxyObjects.size() << "/"
                  << imported.rawProxyObjects.size() << " classes="
                  << classesMatch << " schema="
                  << hasExpectedAcdsSchemaDefinitions(
                         imported.rawDxfSections.front()) << '\n';
        std::cerr << "ACDS multi-projection entity types:";
        for (const DRW_Entity* entity : imported.mBlock->ent) {
            if (entity != nullptr)
                std::cerr << ' ' << static_cast<int>(entity->eType)
                          << ':' << entity->handle;
        }
        std::cerr << '\n';
    }
    if (!keepOutput)
        std::filesystem::remove(output, ec);
    return result;
}

bool runAcdsHistoryCallbackCapture() {
    dx_data captured;
    dx_iface iface;
    iface.cData = &captured;

    DRW_EvaluationGraph graph;
    graph.handle = 0xD67u;
    graph.parentHandle = 0xD66u;
    graph.m_value96 = 96;
    graph.m_value97 = 97;
    DRW_EvaluationGraphNode node;
    node.m_index = 0;
    node.m_expressionHandle = 0xD68u;
    graph.m_nodes.push_back(node);
    iface.addEvaluationGraph(graph);

    DRW_AcShHistoryObject history("ACSH_HISTORY_CLASS");
    history.handle = 0xD66u;
    history.parentHandle = 0xD65u;
    // The common parent is the modeler object (DXF 330); the DWG class's
    // additional owner handle maps to DXF 360 and links the graph.
    history.m_ownerHandle = graph.handle;
    history.m_historyNodeId = 329u;
    history.m_evalExprPrefix.m_unknown = -1;
    history.m_evalExprPrefix.m_value98 = 33;
    history.m_evalExprPrefix.m_value99 = 329;
    history.m_evalExprPrefix.m_id = 1;
    history.m_evalExprPrefix.m_complete = true;
    history.m_historyNodePrefix.m_major = 33;
    history.m_historyNodePrefix.m_minor = 329;
    history.m_historyNodePrefix.m_transform[0] = 1.0;
    history.m_historyNodePrefix.m_colorIndex = 256;
    history.m_historyNodePrefix.m_nodeValue = 3;
    history.m_historyNodePrefix.m_handle = 0xD64u;
    history.m_historyNodePrefix.m_complete = true;
    iface.addAcShHistoryObject(history);

    DRW_AcShHistoryObject box("ACSH_BOX_CLASS");
    box.handle = 0xD68u;
    box.parentHandle = 0xD67u;
    box.m_shapeParams = {5.0, 5.0, 5.0};
    iface.addAcShHistoryObject(box);

    DRW_AcShHistoryObject extrusion("ACSH_EXTRUSION_CLASS");
    extrusion.handle = 0xD6Du;
    extrusion.parentHandle = 0xD6Cu;
    extrusion.m_shapeParams = {4.38, 0.0, 0.0, 1.0};
    iface.addAcShHistoryObject(extrusion);

    DRW_UnsupportedObject graphFrame;
    graphFrame.m_recordName = "ACAD_EVALUATION_GRAPH";
    graphFrame.m_className = "AcDbEvalGraph";
    graphFrame.m_handle = graph.handle;
    graphFrame.m_objectSize = 3;
    graphFrame.m_bodyBitSize = 20;
    graphFrame.m_rawBytes = {0xA1u, 0xB2u, 0xC3u};
    iface.addUnsupportedObject(graphFrame);

    DRW_UnsupportedObject historyFrame;
    historyFrame.m_recordName = "ACSH_HISTORY_CLASS";
    historyFrame.m_className = "AcDbShHistory";
    historyFrame.m_handle = history.handle;
    historyFrame.m_objectSize = 2;
    historyFrame.m_bodyBitSize = 15;
    historyFrame.m_rawBytes = {0xD4u, 0xE5u};
    iface.addUnsupportedObject(historyFrame);

    DRW_UnsupportedObject unrelatedFrame;
    unrelatedFrame.m_recordName = "ACDBASSOCNETWORK";
    unrelatedFrame.m_className = "AcDbAssocNetwork";
    unrelatedFrame.m_handle = 0xD70u;
    unrelatedFrame.m_objectSize = 128;
    unrelatedFrame.m_rawBytes.resize(unrelatedFrame.m_objectSize, 0xFFu);
    iface.addUnsupportedObject(unrelatedFrame);

    graph.m_nodes.front().m_expressionHandle = 0;
    history.m_ownerHandle = 0;
    history.m_evalExprPrefix.m_value98 = 0;
    history.m_evalExprPrefix.m_complete = false;
    history.m_historyNodePrefix.m_transform[0] = 0.0;
    history.m_historyNodePrefix.m_complete = false;
    box.m_shapeParams.clear();
    graphFrame.m_rawBytes.clear();
    historyFrame.m_rawBytes.clear();

    return captured.evaluationGraphs.size() == 1u
        && captured.evaluationGraphs.front().handle == 0xD67u
        && captured.evaluationGraphs.front().parentHandle == history.handle
        && captured.evaluationGraphs.front().m_nodes.size() == 1u
        && captured.evaluationGraphs.front().m_nodes.front()
               .m_expressionHandle == 0xD68u
        && captured.acshHistoryObjects.size() == 3u
        && captured.acshHistoryObjects[0].parentHandle == 0xD65u
        && captured.acshHistoryObjects[0].m_ownerHandle == graph.handle
        && captured.acshHistoryObjects[0].m_evalExprPrefix.m_complete
        && captured.acshHistoryObjects[0].m_evalExprPrefix.m_value98 == 33
        && captured.acshHistoryObjects[0].m_historyNodePrefix.m_complete
        && captured.acshHistoryObjects[0].m_historyNodePrefix.m_minor == 329
        && captured.acshHistoryObjects[0].m_historyNodePrefix.m_transform[0]
               == 1.0
        && captured.acshHistoryObjects[0].m_historyNodePrefix.m_handle
               == 0xD64u
        && captured.acshHistoryObjects[1].m_shapeParams
               == std::vector<double>({5.0, 5.0, 5.0})
        && captured.acshHistoryObjects[2].m_recordName
               == "ACSH_EXTRUSION_CLASS"
        && captured.acdsHistoryDwgFrames.size() == 2u
        && captured.acdsHistoryDwgFrames[0].m_handle == 0xD67u
        && captured.acdsHistoryDwgFrames[0].m_rawBytes
               == std::vector<std::uint8_t>({0xA1u, 0xB2u, 0xC3u})
        && captured.acdsHistoryDwgFrames[1].m_handle == 0xD66u
        && captured.acdsHistoryDwgFrames[1].m_rawBytes
               == std::vector<std::uint8_t>({0xD4u, 0xE5u});
}

bool runDxfAcShExtrusionProjectionCapture(
        const std::filesystem::path& directory, bool duplicateTransformValue,
        std::int32_t alignOption = 0, std::int32_t miterOption = 2) {
    const std::filesystem::path output = directory /
        (duplicateTransformValue
             ? "libdxfrw-acsh-extrusion-projection-malformed.dxf"
             : miterOption != 2
                 ? "libdxfrw-acsh-extrusion-projection-wide-miter.dxf"
                 : alignOption != 0
                     ? "libdxfrw-acsh-extrusion-projection-invalid-align.dxf"
                     : "libdxfrw-acsh-extrusion-projection.dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);
    std::ofstream stream(output);
    if (!stream)
        return false;

    stream << "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n"
              "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
              "0\nACSH_EXTRUSION_CLASS\n5\nD6D\n330\nD6C\n"
              "100\nAcDbShSweepBase\n";
    stream << "90\n33\n91\n329\n10\n0.0\n20\n0.0\n30\n4.380487155114503\n"
              "92\n77\n90\n32\n310\nA1B2C3D4\n93\n0\n"
              "42\n0.0\n43\n0.0\n44\n0.0\n45\n0.0\n48\n1.0\n49\n0.0\n";
    const std::array<double, 16> sweepTransform{{
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
        9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0}};
    const std::array<double, 16> pathTransform{{
        -1.0, -2.0, -3.0, -4.0, -5.0, -6.0, -7.0, -8.0,
        -9.0, -10.0, -11.0, -12.0, -13.0, -14.0, -15.0, -16.0}};
    for (double value : sweepTransform)
        stream << "46\n" << value << '\n';
    if (duplicateTransformValue)
        stream << "46\n17.0\n";
    for (double value : pathTransform)
        stream << "47\n" << value << '\n';
    stream << "70\n" << alignOption << "\n71\n" << miterOption << '\n'
           << "290\n1\n292\n1\n293\n0\n294\n0\n295\n1\n296\n0\n"
              "11\n3.5\n21\n-2.0\n31\n9.0\n"
              "100\nAcDbShExtrusion\n0\nENDSEC\n0\nEOF\n";
    stream.close();
    if (!stream) {
        std::filesystem::remove(output, ec);
        return false;
    }

    class CaptureIface final : public dx_iface {
    public:
        std::vector<DRW_RawDxfObject> rawObjects;
        void addRawDxfObject(const DRW_RawDxfObject& object) override {
            rawObjects.push_back(object);
            dx_iface::addRawDxfObject(object);
        }
    };
    dx_data imported;
    CaptureIface importer;
    const bool readOk = importer.fileImport(output.string(), &imported, false);
    std::filesystem::remove(output, ec);
    if (!readOk || imported.acshHistoryObjects.size() != 1u
        || importer.rawObjects.size() != 1u) {
        return false;
    }

    const DRW_AcShHistoryObject& object = imported.acshHistoryObjects.front();
    const auto& projection = object.m_extrusionDxfProjection;
    const DRW_RawDxfObject& raw = importer.rawObjects.front();
    const std::size_t transformCount = static_cast<std::size_t>(
        std::count_if(raw.groups.begin(), raw.groups.end(),
                      [](const DRW_Variant& value) {
                          return value.code() == 46;
                      }));
    const bool rawOptionsPreserved =
        std::any_of(raw.groups.begin(), raw.groups.end(),
                    [alignOption](const DRW_Variant& value) {
                        return value.code() == 70
                            && value.type() == DRW_Variant::INTEGER
                            && value.i_val() == alignOption;
                    })
        && std::any_of(raw.groups.begin(), raw.groups.end(),
                       [miterOption](const DRW_Variant& value) {
                           return value.code() == 71
                               && value.type() == DRW_Variant::INTEGER
                               && value.i_val() == miterOption;
                       });
    const auto hasIntegerGroup = [&raw](int code, std::int64_t expected) {
        return std::any_of(raw.groups.begin(), raw.groups.end(),
                           [code, expected](const DRW_Variant& value) {
                               return value.code() == code
                                   && value.type() == DRW_Variant::INTEGER
                                   && value.i_val() == expected;
                           });
    };
    const auto hasDoubleGroup = [&raw](int code, double expected) {
        return std::any_of(raw.groups.begin(), raw.groups.end(),
                           [code, expected](const DRW_Variant& value) {
                               return value.code() == code
                                   && value.type() == DRW_Variant::DOUBLE
                                   && value.d_val() == expected;
                           });
    };
    const bool rawUnprojectedFieldsPreserved =
        std::count_if(raw.groups.begin(), raw.groups.end(),
                      [](const DRW_Variant& value) {
                          return value.code() == 90;
                      }) == 2
        && hasIntegerGroup(90, 33) && hasIntegerGroup(90, 32)
        && hasIntegerGroup(91, 329) && hasIntegerGroup(92, 77)
        && hasIntegerGroup(93, 0)
        && hasDoubleGroup(10, 0.0) && hasDoubleGroup(20, 0.0)
        && hasDoubleGroup(30, 4.380487155114503)
        && hasDoubleGroup(42, 0.0) && hasDoubleGroup(43, 0.0)
        && hasDoubleGroup(44, 0.0) && hasDoubleGroup(45, 0.0)
        && hasDoubleGroup(48, 1.0) && hasDoubleGroup(49, 0.0)
        && std::any_of(raw.groups.begin(), raw.groups.end(),
                       [](const DRW_Variant& value) {
                           return value.code() == 310
                               && value.type() == DRW_Variant::STRING
                               && value.c_str() != nullptr
                               && std::strcmp(value.c_str(), "A1B2C3D4") == 0;
                       });
    if (object.m_recordName != "ACSH_EXTRUSION_CLASS"
        || transformCount != 16u + (duplicateTransformValue ? 1u : 0u)
        || !rawOptionsPreserved || !rawUnprojectedFieldsPreserved) {
        return false;
    }
    if (duplicateTransformValue || alignOption < 0 || alignOption > 3) {
        return !projection.m_transformAndOptionFieldsComplete
            && std::all_of(projection.m_sweepEntityTransform.begin(),
                           projection.m_sweepEntityTransform.end(),
                           [](double value) { return value == 0.0; });
    }

    return projection.m_transformAndOptionFieldsComplete
        && projection.m_sweepEntityTransform == sweepTransform
        && projection.m_pathEntityTransform == pathTransform
        && projection.m_alignOption == alignOption
        && projection.m_miterOption == miterOption
        && projection.m_isSolid && projection.m_hasAlignStart
        && !projection.m_bank && !projection.m_basePointSet
        && projection.m_sweepEntityTransformComputed
        && !projection.m_pathEntityTransformComputed
        && projection.m_point.x == 3.5 && projection.m_point.y == -2.0
        && projection.m_point.z == 9.0;
}

bool runDxfTypedAcdsBoxRoundTrip(
    bool binary, const std::filesystem::path& directory, bool keepOutput) {
    const std::filesystem::path output = directory /
        (std::string("libdxfrw-ac1027-typed-acds-box-")
         + (binary ? "binary" : "ascii") + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    constexpr std::uint32_t modelerHandle = 0xFC20u;
    constexpr std::uint32_t historyHandle = 0xFC21u;
    constexpr std::uint32_t graphHandle = 0xFC22u;
    constexpr std::uint32_t boxHandle = 0xFC23u;
    constexpr std::uint32_t materialHandle = 0x96u;
    const std::vector<std::uint8_t> payload{
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e',
        0x21u, 0x22u, 0x23u};

    dx_data source;
    auto* modeler = new DRW_ModelerGeometry(DRW::E3DSOLID);
    modeler->handle = modelerHandle;
    modeler->m_modelerVersion = 2;
    modeler->m_dwgSourceVersion = DRW::AC1027;
    modeler->m_hasModelerData = true;
    modeler->m_historyHandle = historyHandle;
    modeler->setHasDataStorageBinaryData(true);
    modeler->hasDataStorageRecord = true;
    modeler->dataStorageHandle = modelerHandle;
    modeler->dataStorageHandleKey = "FC20";
    modeler->dataStorageSchemaIndex = 5u;
    modeler->dataStorageData = payload;
    source.mBlock->ent.push_back(modeler);
    source.dataStorageSections.push_back(
        makeLocalAcadSharpDataStorageSection(modelerHandle, payload));

    source.dwgClassCoverage.m_status =
        DRW_DwgClassCoverageStatus::FinalizedComplete;
    source.dwgClassCoverage.m_complete = true;
    const std::array<std::tuple<std::uint16_t, const char*, const char*>, 3>
        classes{{
            {521u, "ACSH_HISTORY_CLASS", "AcDbShHistory"},
            {520u, "ACAD_EVALUATION_GRAPH", "AcDbEvalGraph"},
            {546u, "ACSH_BOX_CLASS", "AcDbShBox"}}};
    for (const auto& cls : classes) {
        DRW_DwgClassCoverageEntry entry;
        entry.m_classNumber = std::get<0>(cls);
        entry.m_recordName = std::get<1>(cls);
        entry.m_className = std::get<2>(cls);
        entry.m_entityFlagRaw = 0x1F3u;
        entry.m_state = DRW_DwgClassCoverageState::Published;
        source.dwgClassCoverage.m_entries.push_back(std::move(entry));
    }

    const auto completePrefix = [](
        DRW_AcShHistoryObject& object,
        DRW_AssociativePrefixStatus::Kind kind) {
        DRW_AssociativePrefixStatus status;
        status.m_kind = kind;
        status.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
        object.m_prefixStatuses.push_back(std::move(status));
    };
    DRW_AcShHistoryObject history("ACSH_HISTORY_CLASS");
    history.handle = historyHandle;
    history.parentHandle = modelerHandle;
    history.m_major = 33u;
    history.m_minor = 329u;
    history.m_historyNodeId = 1u;
    history.m_ownerHandle = graphHandle;
    completePrefix(history,
                   DRW_AssociativePrefixStatus::Kind::AcDbShHistoryNode);
    source.acshHistoryObjects.push_back(history);

    DRW_EvaluationGraph graph;
    graph.handle = graphHandle;
    graph.parentHandle = historyHandle;
    graph.m_value96 = 1;
    graph.m_value97 = 1;
    DRW_EvaluationGraphNode graphNode;
    graphNode.m_index = 0;
    graphNode.m_flags = 32;
    graphNode.m_nextNodeIndex = 1;
    graphNode.m_expressionHandle = boxHandle;
    graphNode.m_data1 = -1;
    graphNode.m_data2 = -1;
    graphNode.m_data3 = -1;
    graphNode.m_data4 = -1;
    graph.m_nodes.push_back(graphNode);
    source.evaluationGraphs.push_back(graph);

    DRW_AcShHistoryObject box("ACSH_BOX_CLASS");
    box.handle = boxHandle;
    box.parentHandle = graphHandle;
    box.m_evalExprPrefix.m_id = 1;
    box.m_evalExprPrefix.m_value98 = 33;
    box.m_evalExprPrefix.m_value99 = 329;
    box.m_evalExprPrefix.m_complete = true;
    completePrefix(box, DRW_AssociativePrefixStatus::Kind::AcDbEvalExpr);
    box.m_historyNodePrefix.m_major = 33;
    box.m_historyNodePrefix.m_minor = 329;
    for (std::size_t index = 0; index < 4u; ++index)
        box.m_historyNodePrefix.m_transform[index * 5u] = 1.0;
    box.m_historyNodePrefix.m_colorIndex = 256u;
    box.m_historyNodePrefix.m_nodeValue = 3;
    box.m_historyNodePrefix.m_handle = materialHandle;
    box.m_historyNodePrefix.m_complete = true;
    completePrefix(box,
                   DRW_AssociativePrefixStatus::Kind::AcDbShHistoryNode);
    box.m_major = 33u;
    box.m_minor = 329u;
    box.m_shapeParams = {5.0, 5.0, 5.0};
    completePrefix(box, DRW_AssociativePrefixStatus::Kind::AcShActionBody);
    source.acshHistoryObjects.push_back(box);

    DRW_Dictionary materialDictionary;
    materialDictionary.handle = 0x72u;
    materialDictionary.parentHandle = 0xCu;
    materialDictionary.cloning = 1;
    materialDictionary.m_entries = {
        {"ByLayer", materialHandle}, {"ByBlock", 0x97u},
        {"Global", 0x98u}};
    source.dictionaries.push_back(materialDictionary);
    for (const auto& materialInfo : {
             std::pair<std::uint32_t, const char*>{materialHandle, "ByLayer"},
             {0x97u, "ByBlock"}, {0x98u, "Global"}}) {
        DRW_Material material;
        material.handle = materialInfo.first;
        material.parentHandle = materialDictionary.handle;
        material.m_name = materialInfo.second;
        source.materials.push_back(std::move(material));
    }

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary,
                             &source, false)) {
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }

    class RawCaptureIface final : public dx_iface {
    public:
        std::vector<DRW_RawDxfObject> objects;
        void addRawDxfObject(const DRW_RawDxfObject& object) override {
            objects.push_back(object);
            dx_iface::addRawDxfObject(object);
        }
    };
    dx_data imported;
    RawCaptureIface importer;
    const bool readOk = importer.fileImport(output.string(), &imported, false);
    const auto findRaw = [&importer](const char* name, std::uint32_t handle)
        -> const DRW_RawDxfObject* {
        const DRW_RawDxfObject* found = nullptr;
        for (const DRW_RawDxfObject& object : importer.objects) {
            if (object.name != name || object.handle != handle)
                continue;
            if (found != nullptr)
                return nullptr;
            found = &object;
        }
        return found;
    };
    const auto hasInt = [](const DRW_RawDxfObject* object, int code,
                           std::int32_t expected) {
        return object != nullptr
            && std::any_of(object->groups.begin(), object->groups.end(),
                           [code, expected](const DRW_Variant& group) {
                               return group.code() == code
                                   && group.type() == DRW_Variant::INTEGER
                                   && group.i_val() == expected;
                           });
    };
    const auto hasString = [](const DRW_RawDxfObject* object, int code,
                              const std::string& expected) {
        return object != nullptr
            && std::any_of(object->groups.begin(), object->groups.end(),
                           [code, &expected](const DRW_Variant& group) {
                               return group.code() == code
                                   && group.type() == DRW_Variant::STRING
                                   && group.c_str() != nullptr
                                   && expected == group.c_str();
                           });
    };
    const DRW_RawDxfObject* historyRaw =
        findRaw("ACSH_HISTORY_CLASS", historyHandle);
    const DRW_RawDxfObject* boxRaw =
        findRaw("ACSH_BOX_CLASS", boxHandle);
    const DRW_EvaluationGraph* importedGraph = nullptr;
    for (const DRW_EvaluationGraph& value : imported.evaluationGraphs) {
        if (value.handle == graphHandle)
            importedGraph = &value;
    }
    std::vector<std::pair<std::string, int>> expectedClasses{
        {"ACSH_HISTORY_CLASS", 1}, {"ACAD_EVALUATION_GRAPH", 1},
        {"ACSH_BOX_CLASS", 1}, {"MATERIAL", 3}};
    bool classesMatch = imported.dxfClasses.size() == expectedClasses.size();
    if (classesMatch) {
        for (std::size_t index = 0; index < expectedClasses.size(); ++index) {
            const DRW_Class& value = imported.dxfClasses[index];
            if (value.recName != expectedClasses[index].first
                || value.instanceCount != expectedClasses[index].second
                || value.wasaProxyFlag != 0) {
                classesMatch = false;
                break;
            }
        }
    }
    std::string acdsOwnerHandle;
    std::vector<std::uint8_t> acdsPayload;
    std::uint32_t importedModelerHandle = 0;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity != nullptr && entity->handle != 0
            && entity->eType == DRW::E3DSOLID) {
            importedModelerHandle = entity->handle;
            break;
        }
    }
    char expectedModelerHandle[9] = {};
    std::snprintf(expectedModelerHandle, sizeof(expectedModelerHandle), "%X",
                  importedModelerHandle);
    const bool acdsPayloadMatches = imported.rawDxfSections.size() == 1u
        && findAcdsRecordPayload(imported.rawDxfSections.front(), "ASM_Data",
                                 acdsOwnerHandle, acdsPayload)
        && importedModelerHandle != 0
        && acdsOwnerHandle == expectedModelerHandle && acdsPayload == payload;
    std::vector<std::string> boxSubclasses;
    std::vector<double> boxTransform;
    std::vector<double> boxDimensions;
    bool inBoxShape = false;
    if (boxRaw != nullptr) {
        for (const DRW_Variant& group : boxRaw->groups) {
            if (group.code() == 100
                && group.type() == DRW_Variant::STRING
                && group.c_str() != nullptr) {
                boxSubclasses.emplace_back(group.c_str());
                inBoxShape = std::strcmp(group.c_str(), "AcDbShBox") == 0;
            } else if (group.code() >= 40 && group.code() <= 55
                       && group.type() == DRW_Variant::DOUBLE
                       && !inBoxShape) {
                boxTransform.push_back(group.d_val());
            } else if (group.code() >= 40 && group.code() <= 42
                       && group.type() == DRW_Variant::DOUBLE
                       && inBoxShape) {
                boxDimensions.push_back(group.d_val());
            }
        }
    }
    const std::vector<std::string> expectedBoxSubclasses{
        "AcDbEvalExpr", "AcDbShHistoryNode", "AcDbShPrimitive", "AcDbShBox"};
    const std::vector<double> expectedBoxTransform{
        1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    const bool result = readOk && historyRaw != nullptr && boxRaw != nullptr
        && hasInt(historyRaw, 90, 33) && hasInt(historyRaw, 91, 329)
        && hasInt(historyRaw, 92, 1) && hasInt(historyRaw, 280, 0)
        && hasInt(historyRaw, 281, 0)
        && hasString(historyRaw, 330, expectedModelerHandle)
        && hasString(historyRaw, 360, "FC22")
        && hasInt(boxRaw, 90, 1) && hasInt(boxRaw, 91, 329)
        && hasInt(boxRaw, 92, 3)
        && hasString(boxRaw, 330, "FC22")
        && boxSubclasses == expectedBoxSubclasses
        && boxTransform == expectedBoxTransform
        && boxDimensions == std::vector<double>({5.0, 5.0, 5.0})
        && std::any_of(boxRaw->groups.begin(), boxRaw->groups.end(),
                       [](const DRW_Variant& group) {
                           return group.code() == 347
                               && group.type() == DRW_Variant::STRING
                               && group.c_str() != nullptr
                               && std::strcmp(group.c_str(), "96") == 0;
                       })
        && importedGraph != nullptr && importedGraph->m_value96 == 1
        && importedGraph->m_value97 == 1
        && importedGraph->m_nodes.size() == 1u
        && importedGraph->m_nodes.front().m_index == 0
        && importedGraph->m_nodes.front().m_flags == 32
        && importedGraph->m_nodes.front().m_nextNodeIndex == 1
        && importedGraph->m_nodes.front().m_expressionHandle == boxHandle
        && importedGraph->m_nodes.front().m_data1 == -1
        && importedGraph->m_nodes.front().m_data2 == -1
        && importedGraph->m_nodes.front().m_data3 == -1
        && importedGraph->m_nodes.front().m_data4 == -1
        && classesMatch && imported.materials.size() == 3u
        && imported.rawDxfSections.size() == 1u
        && imported.rawDxfSections.front().m_name == "ACDSDATA"
        && acdsPayloadMatches;
    if (!result) {
        std::cerr << "Typed ACDS BOX round-trip ("
                  << (binary ? "binary" : "ASCII") << "): read=" << readOk
                  << " history=" << (historyRaw != nullptr)
                  << " box=" << (boxRaw != nullptr)
                  << " graph=" << (importedGraph != nullptr)
                  << " classes=" << classesMatch
                  << " materials=" << imported.materials.size()
                  << " sections=" << imported.rawDxfSections.size()
                  << " acdsPayload=" << acdsPayloadMatches
                  << " payloadOwner=" << acdsOwnerHandle
                  << " modeler=" << expectedModelerHandle
                  << " payloadBytes=" << acdsPayload.size() << '\n';
        if (!classesMatch) {
            for (const DRW_Class& value : imported.dxfClasses)
                std::cerr << "  CLASS " << value.recName << " count="
                          << value.instanceCount << " proxy="
                          << value.wasaProxyFlag << '\n';
        }
    }
    if (!keepOutput)
        std::filesystem::remove(output, ec);
    return result;
}

const DRW_Variant* findDynamicDxfGroup(
        const DRW_RawDxfObject& raw, const std::string& subclass, int code,
        std::size_t occurrence = 0) {
    std::string currentSubclass;
    for (const DRW_Variant& group : raw.groups) {
        if (group.code() == 100 && group.type() == DRW_Variant::STRING) {
            currentSubclass = group.c_str();
            continue;
        }
        if (currentSubclass == subclass && group.code() == code) {
            if (occurrence == 0)
                return &group;
            --occurrence;
        }
    }
    return nullptr;
}

bool dynamicDxfIntegerEquals(const DRW_RawDxfObject& raw,
                             const std::string& subclass, int code,
                             std::int64_t expected) {
    const DRW_Variant* group = findDynamicDxfGroup(raw, subclass, code);
    if (group == nullptr)
        return false;
    if (group->type() == DRW_Variant::INTEGER)
        return group->i_val() == expected;
    return group->type() == DRW_Variant::INTEGER64
        && group->i64_val() == expected;
}

bool dynamicDxfDoubleEquals(const DRW_RawDxfObject& raw,
                            const std::string& subclass, int code,
                            double expected) {
    const DRW_Variant* group = findDynamicDxfGroup(raw, subclass, code);
    if (group == nullptr || group->type() != DRW_Variant::DOUBLE)
        return false;
    const double actual = group->d_val();
    const double tolerance = std::max(1.0, std::abs(expected)) * 1e-12;
    return std::abs(actual - expected) <= tolerance;
}

bool sameEvaluationGraph(const DRW_EvaluationGraph& left,
                         const DRW_EvaluationGraph& right) {
    if (left.handle != right.handle
        || left.parentHandle != right.parentHandle
        || left.m_value96 != right.m_value96
        || left.m_value97 != right.m_value97
        || left.m_nodes.size() != right.m_nodes.size()
        || left.m_edges.size() != right.m_edges.size())
        return false;
    for (std::size_t i = 0; i < left.m_nodes.size(); ++i) {
        const DRW_EvaluationGraphNode& a = left.m_nodes[i];
        const DRW_EvaluationGraphNode& b = right.m_nodes[i];
        if (a.m_index != b.m_index || a.m_flags != b.m_flags
            || a.m_nextNodeIndex != b.m_nextNodeIndex
            || a.m_expressionHandle != b.m_expressionHandle
            || a.m_data1 != b.m_data1 || a.m_data2 != b.m_data2
            || a.m_data3 != b.m_data3 || a.m_data4 != b.m_data4)
            return false;
    }
    for (std::size_t i = 0; i < left.m_edges.size(); ++i) {
        const DRW_EvaluationGraphEdge& a = left.m_edges[i];
        const DRW_EvaluationGraphEdge& b = right.m_edges[i];
        if (a.m_value92 != b.m_value92 || a.m_value93 != b.m_value93
            || a.m_value94 != b.m_value94
            || a.m_value91a != b.m_value91a
            || a.m_value91b != b.m_value91b
            || a.m_value92a != b.m_value92a
            || a.m_value92b != b.m_value92b
            || a.m_value92c != b.m_value92c
            || a.m_value92d != b.m_value92d
            || a.m_value92e != b.m_value92e)
            return false;
    }
    return true;
}

struct DxfAsciiPair {
    std::string codeText;
    std::string valueText;
    int code = -1;
};

std::string trimAsciiWhitespace(std::string value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1u);
}

bool readDxfAsciiPairs(const std::filesystem::path& path,
                       std::vector<DxfAsciiPair>& pairs) {
    std::ifstream input(path);
    if (!input)
        return false;
    std::string codeText;
    std::string valueText;
    while (std::getline(input, codeText)) {
        if (!std::getline(input, valueText))
            return false;
        const std::string trimmedCode = trimAsciiWhitespace(codeText);
        std::size_t parsed = 0;
        int code = -1;
        try {
            code = std::stoi(trimmedCode, &parsed);
        } catch (...) {
            return false;
        }
        if (parsed != trimmedCode.size())
            return false;
        pairs.push_back({codeText, valueText, code});
    }
    return input.eof();
}

bool appendDxfSection(const std::vector<DxfAsciiPair>& pairs,
                      const std::string& sectionName,
                      std::ostringstream& output) {
    for (std::size_t start = 0; start + 1u < pairs.size(); ++start) {
        if (pairs[start].code != 0
            || trimAsciiWhitespace(pairs[start].valueText) != "SECTION"
            || pairs[start + 1u].code != 2
            || trimAsciiWhitespace(pairs[start + 1u].valueText)
                != sectionName)
            continue;
        for (std::size_t index = start; index < pairs.size(); ++index) {
            output << pairs[index].codeText << '\n'
                   << pairs[index].valueText << '\n';
            if (pairs[index].code == 0
                && trimAsciiWhitespace(pairs[index].valueText) == "ENDSEC")
                return true;
        }
        return false;
    }
    return false;
}

bool appendSelectedDxfObjects(
        const std::vector<DxfAsciiPair>& pairs,
        const std::unordered_set<std::uint32_t>& selectedHandles,
        std::ostringstream& output) {
    for (std::size_t start = 0; start + 1u < pairs.size(); ++start) {
        if (pairs[start].code != 0
            || trimAsciiWhitespace(pairs[start].valueText) != "SECTION"
            || pairs[start + 1u].code != 2
            || trimAsciiWhitespace(pairs[start + 1u].valueText) != "OBJECTS")
            continue;
        output << pairs[start].codeText << '\n' << pairs[start].valueText
               << '\n' << pairs[start + 1u].codeText << '\n'
               << pairs[start + 1u].valueText << '\n';
        std::vector<DxfAsciiPair> record;
        const auto emitSelectedRecord = [&]() {
            std::uint32_t handle = 0;
            bool hasHandle = false;
            for (const DxfAsciiPair& pair : record) {
                if (pair.code != 5)
                    continue;
                const std::string text = trimAsciiWhitespace(pair.valueText);
                try {
                    std::size_t parsed = 0;
                    const unsigned long value = std::stoul(text, &parsed, 16);
                    if (parsed == text.size()
                        && value <= std::numeric_limits<std::uint32_t>::max()) {
                        handle = static_cast<std::uint32_t>(value);
                        hasHandle = true;
                    }
                } catch (...) {
                    return false;
                }
                break;
            }
            if (hasHandle && selectedHandles.count(handle) != 0) {
                for (const DxfAsciiPair& pair : record)
                    output << pair.codeText << '\n' << pair.valueText << '\n';
            }
            record.clear();
            return true;
        };
        for (std::size_t index = start + 2u; index < pairs.size(); ++index) {
            if (pairs[index].code == 0
                && trimAsciiWhitespace(pairs[index].valueText) == "ENDSEC") {
                if (!emitSelectedRecord())
                    return false;
                output << pairs[index].codeText << '\n'
                       << pairs[index].valueText << '\n';
                return true;
            }
            if (pairs[index].code == 0 && !record.empty()
                && !emitSelectedRecord())
                return false;
            record.push_back(pairs[index]);
        }
        return false;
    }
    return false;
}

bool buildDynamicBlockOracleDxf(
        const std::filesystem::path& path,
        const std::unordered_set<std::uint32_t>& selectedHandles,
        std::string& output) {
    std::vector<DxfAsciiPair> pairs;
    if (!readDxfAsciiPairs(path, pairs))
        return false;
    std::ostringstream projected;
    if (!appendDxfSection(pairs, "HEADER", projected)
        || !appendDxfSection(pairs, "CLASSES", projected)
        || !appendSelectedDxfObjects(pairs, selectedHandles, projected))
        return false;
    projected << "  0\nEOF\n";
    output = projected.str();
    return true;
}

bool runDynamicBlockTypedDxfPairQualification(
        const std::filesystem::path& dwgPath,
        const std::filesystem::path& dxfPath) {
    if (!std::filesystem::is_regular_file(dwgPath)
        || !std::filesystem::is_regular_file(dxfPath))
        return false;

    class DwgCaptureIface final : public dx_iface {
    public:
        std::vector<DRW_DynamicBlockObject> dynamicObjects;
        std::vector<DRW_DwgFramePublication> frames;
        void addDynamicBlockObject(
                const DRW_DynamicBlockObject& data) override {
            dynamicObjects.push_back(data);
        }
        void addDwgFramePublication(
                const DRW_DwgFramePublication& frame) override {
            frames.push_back(frame);
        }
    } dwgIface;
    dx_data dwgData;
    dwgIface.cData = &dwgData;
    dwgRW dwgReader(dwgPath.string().c_str());
    if (!dwgReader.read(&dwgIface, false)
        || dwgReader.getVersion() != DRW::AC1027) {
        std::cerr << "dynamic-block DWG read/version failed\n";
        return false;
    }

    class DxfCaptureIface final : public dx_iface {
    public:
        std::vector<DRW_RawDxfObject> dynamicRecords;
        void addRawDxfObject(const DRW_RawDxfObject& data) override {
            if (DRW_DynamicBlockObject::isDynamicBlockRecName(data.name))
                dynamicRecords.push_back(data);
            dx_iface::addRawDxfObject(data);
        }
    } dxfIface;
    dx_data dxfData;
    dxfIface.cData = &dxfData;
    dxfRW dxfReader(dxfPath.string().c_str());

    std::unordered_map<std::uint32_t, const DRW_DynamicBlockObject*> dwgByHandle;
    for (const DRW_DynamicBlockObject& object : dwgIface.dynamicObjects) {
        if (object.handle == 0
            || !dwgByHandle.emplace(object.handle, &object).second) {
            std::cerr << "duplicate/null DWG dynamic-block handle\n";
            return false;
        }
    }
    const DRW_EvaluationGraph* sourceDynamicGraph = nullptr;
    std::size_t graphCandidates = 0;
    for (const DRW_EvaluationGraph& graph : dwgData.evaluationGraphs) {
        if (graph.m_nodes.size() != 12u || graph.m_edges.size() != 10u)
            continue;
        const bool allTargetsDynamic = std::all_of(
            graph.m_nodes.begin(), graph.m_nodes.end(),
            [&dwgByHandle](const DRW_EvaluationGraphNode& node) {
                return dwgByHandle.count(node.m_expressionHandle) == 1u;
            });
        if (allTargetsDynamic) {
            sourceDynamicGraph = &graph;
            ++graphCandidates;
        }
    }
    if (graphCandidates != 1u || sourceDynamicGraph == nullptr) {
        std::cerr << "expected one 12-node/10-edge dynamic-block graph; found "
                  << graphCandidates << '\n';
        return false;
    }

    std::unordered_set<std::uint32_t> selectedHandles{
        sourceDynamicGraph->handle};
    for (const DRW_EvaluationGraphNode& node : sourceDynamicGraph->m_nodes)
        selectedHandles.insert(node.m_expressionHandle);
    std::string dxfProjection;
    if (!buildDynamicBlockOracleDxf(dxfPath, selectedHandles,
                                    dxfProjection)) {
        std::cerr << "failed to isolate ODA HEADER/CLASSES and witnessed "
                     "OBJECTS records\n";
        return false;
    }
    const bool dxfReadOk = dxfReader.readAscii(
        &dxfIface, false, dxfProjection);
    if (!dxfReadOk) {
        const DRW_OperationDiagnostic diagnostic =
            dxfReader.getLastDiagnostic();
        std::cerr << "dynamic-block ODA projection read/version failed: read="
                  << dxfReadOk << " version="
                  << static_cast<int>(dxfReader.getVersion()) << " error="
                  << static_cast<int>(dxfReader.getError()) << " phase="
                  << static_cast<int>(diagnostic.phase) << " code="
                  << diagnostic.code << " message=" << diagnostic.message
                  << '\n';
        return false;
    }
    std::unordered_map<std::uint32_t, const DRW_RawDxfObject*> dxfByHandle;
    for (const DRW_RawDxfObject& object : dxfIface.dynamicRecords) {
        if (object.handle == 0
            || !dxfByHandle.emplace(object.handle, &object).second) {
            std::cerr << "duplicate/null ODA DXF dynamic-block handle\n";
            return false;
        }
    }

    const DRW_EvaluationGraph* odaDynamicGraph = nullptr;
    for (const DRW_EvaluationGraph& graph : dxfData.evaluationGraphs) {
        if (graph.handle == sourceDynamicGraph->handle) {
            if (odaDynamicGraph != nullptr)
                return false;
            odaDynamicGraph = &graph;
        }
    }
    const bool graphMatches = odaDynamicGraph != nullptr
        && sameEvaluationGraph(*sourceDynamicGraph, *odaDynamicGraph);
    if (!graphMatches) {
        std::cerr << "dynamic-block EvaluationGraph differs at handle "
                  << std::hex << sourceDynamicGraph->handle << std::dec << '\n';
        return false;
    }

    std::size_t commonFieldsCompared = 0;
    std::size_t selector40ValueMismatches = 0;
    std::unordered_map<std::string, std::size_t> targetClassCounts;
    bool targetStructureMatches = true;
    for (const DRW_EvaluationGraphNode &node : sourceDynamicGraph->m_nodes) {
      const DRW_DynamicBlockObject &source =
          *dwgByHandle.at(node.m_expressionHandle);
      const auto dxfIt = dxfByHandle.find(source.handle);
      if (dxfIt == dxfByHandle.end()) {
        targetStructureMatches = false;
        std::cerr << "missing ODA DXF target " << std::hex << source.handle
                  << std::dec << " (" << source.m_recordName << ")\n";
        continue;
      }
      const DRW_RawDxfObject &raw = *dxfIt->second;
      ++targetClassCounts[source.m_recordName];
      const bool identityMatches = raw.name == source.m_recordName &&
                                   raw.parentHandle == source.parentHandle;
      const bool evalFieldsMatch =
          source.m_evalExprParsed &&
          dynamicDxfIntegerEquals(raw, "AcDbEvalExpr", 90, source.m_nodeId) &&
          dynamicDxfIntegerEquals(raw, "AcDbEvalExpr", 98, source.m_major) &&
          dynamicDxfIntegerEquals(raw, "AcDbEvalExpr", 99, source.m_minor) &&
          (source.m_valueCode == -9999
               ? findDynamicDxfGroup(raw, "AcDbEvalExpr", 70) == nullptr
               : dynamicDxfIntegerEquals(raw, "AcDbEvalExpr", 70,
                                         source.m_valueCode));
      const bool evalValueMatches =
          source.m_valueCode != 40 ||
          (source.m_hasValueDouble &&
           dynamicDxfDoubleEquals(raw, "AcDbEvalExpr", 140,
                                  source.m_valueDouble));
      bool typedFieldsMatch = identityMatches && evalFieldsMatch;
      commonFieldsCompared +=
          evalFieldsMatch ? 3u + (source.m_valueCode == -9999 ? 0u : 1u) : 0u;
      commonFieldsCompared +=
          evalValueMatches && source.m_valueCode == 40 ? 1u : 0u;
      if (source.m_valueCode == 40 && !evalValueMatches) {
        ++selector40ValueMismatches;
        const DRW_Variant *dxfValue =
            findDynamicDxfGroup(raw, "AcDbEvalExpr", 140);
        std::cout.flush();
        std::cerr << "dynamic EvalExpr selector-40 value differs @" << std::hex
                  << source.handle << std::dec
                  << " DWG=" << source.m_valueDouble << " DXF140=";
        if (dxfValue != nullptr && dxfValue->type() == DRW_Variant::DOUBLE)
          std::cerr << dxfValue->d_val();
        else
          std::cerr << "<absent-or-nondouble>";
        std::cerr << '\n';
      }
      if (source.m_elementParsed) {
        typedFieldsMatch =
            typedFieldsMatch &&
            findDynamicDxfGroup(raw, "AcDbBlockElement", 300) != nullptr &&
            findDynamicDxfGroup(raw, "AcDbBlockElement", 300)->type() ==
                DRW_Variant::STRING &&
            findDynamicDxfGroup(raw, "AcDbBlockElement", 300)->c_str() ==
                source.m_elementName &&
            dynamicDxfIntegerEquals(raw, "AcDbBlockElement", 98,
                                    source.m_elementMajor) &&
            dynamicDxfIntegerEquals(raw, "AcDbBlockElement", 99,
                                    source.m_elementMinor) &&
            dynamicDxfIntegerEquals(raw, "AcDbBlockElement", 1071,
                                    source.m_eed1071);
        commonFieldsCompared += typedFieldsMatch ? 4u : 0u;
      }
      if (source.m_kind ==
          DRW_DynamicBlockObject::Kind::GripLocationComponent) {
        const DRW_Variant *gripExpr =
            findDynamicDxfGroup(raw, "AcDbBlockGripExpr", 300);
        const bool gripFieldsMatch =
            source.m_bodyFullyDecoded &&
            dynamicDxfIntegerEquals(raw, "AcDbBlockGripExpr", 91,
                                    source.m_gripType) &&
            gripExpr != nullptr && gripExpr->type() == DRW_Variant::STRING &&
            gripExpr->c_str() == source.m_gripExpr;
        typedFieldsMatch = typedFieldsMatch && gripFieldsMatch;
        commonFieldsCompared += gripFieldsMatch ? 2u : 0u;
      }
      if (!typedFieldsMatch) {
        targetStructureMatches = false;
        std::cout.flush();
        std::cerr << "typed common-field mismatch " << source.m_recordName
                  << '@' << std::hex << source.handle << std::dec
                  << " owner=" << std::hex << source.parentHandle << std::dec
                  << " eval=" << source.m_evalExprParsed
                  << " node/major/minor=" << source.m_nodeId << '/'
                  << source.m_major << '/' << source.m_minor
                  << " valueCode=" << source.m_valueCode
                  << " hasValueDouble=" << source.m_hasValueDouble
                  << " valueDouble=" << source.m_valueDouble
                  << " bodyFullyDecoded=" << source.m_bodyFullyDecoded << '\n';
      }
      const DRW_Variant *valueSelector =
          findDynamicDxfGroup(raw, "AcDbEvalExpr", 70);
      std::cout << "  " << source.m_recordName << '@' << std::hex
                << source.handle << std::dec
                << " valueCode=" << source.m_valueCode << " dxf70=";
      if (valueSelector != nullptr &&
          valueSelector->type() == DRW_Variant::INTEGER)
        std::cout << valueSelector->i_val();
      else
        std::cout << "<absent>";
      std::cout << " bodyFullyDecoded=" << source.m_bodyFullyDecoded << '\n';
    }
    bool classesMatch = true;
    for (const auto &expected : targetClassCounts) {
      std::size_t matches = 0;
      int declaredInstances = 0;
      for (const DRW_Class &cls : dxfData.dxfClasses) {
        if (cls.recName != expected.first)
          continue;
        ++matches;
        declaredInstances = cls.instanceCount;
      }
      classesMatch = classesMatch && matches == 1u &&
                     declaredInstances >= static_cast<int>(expected.second);
    }
    if (!classesMatch) {
      std::cerr << "DXF CLASSES rows do not cover the dynamic-block closure\n";
      return false;
    }

    std::cout << "dynamic-block typed-DXF correlation: graph=" << std::hex
              << sourceDynamicGraph->handle << std::dec
              << " nodes=" << sourceDynamicGraph->m_nodes.size()
              << " edges=" << sourceDynamicGraph->m_edges.size()
              << " targets=" << targetClassCounts.size()
              << " classes=" << targetClassCounts.size()
              << " common-fields=" << commonFieldsCompared
              << " graph/owner/structure-match="
              << (graphMatches && targetStructureMatches ? "yes" : "no")
              << " classes-match=" << (classesMatch ? "yes" : "no")
              << " selector40-value-mismatches=" << selector40ValueMismatches
              << " DWG-records=" << dwgIface.dynamicObjects.size()
              << " DXF-records=" << dxfIface.dynamicRecords.size()
              << " (non-matching fields remain unqualified)\n";
    for (const auto &entry : targetClassCounts)
      std::cout << "  " << entry.first << ": " << entry.second << '\n';
    return graphMatches && targetStructureMatches && classesMatch &&
           selector40ValueMismatches == 0u;
}

bool runAcshPrefixSampleQualification(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path))
        return false;

    class FrameCaptureIface final : public dx_iface {
    public:
        std::vector<DRW_DwgFramePublication> frames;
        void addDwgFramePublication(
            const DRW_DwgFramePublication& frame) override {
            frames.push_back(frame);
        }
    };
    dx_data captured;
    FrameCaptureIface iface;
    iface.cData = &captured;
    dwgRW reader(path.string().c_str());
    const bool readOk = reader.read(&iface, false);
    if (!readOk || reader.getVersion() != DRW::AC1027) {
        std::cerr << "ACSH sample read/version: read=" << readOk
                  << " version=" << static_cast<int>(reader.getVersion())
                  << '\n';
        return false;
    }

    const auto schemaByIndex = [&captured](std::uint32_t schemaIndex)
        -> const DRW_DataStorageSchema* {
        if (captured.dataStorageSections.size() != 1u)
            return nullptr;
        for (const DRW_DataStorageSchema& schema :
             captured.dataStorageSections.front().schemas) {
            if (schema.index == schemaIndex)
                return &schema;
        }
        return nullptr;
    };
    const auto propertyMatches = [](const DRW_DataStorageSchema* schema,
                                    std::size_t propertyIndex,
                                    std::uint32_t nameIndex,
                                    const char* name) {
        return schema != nullptr && propertyIndex < schema->properties.size()
            && schema->properties[propertyIndex].nameIndex == nameIndex
            && schema->properties[propertyIndex].name == name;
    };
    bool acdsSchemasMatch = false;
    if (captured.dataStorageSections.size() == 1u) {
        const DRW_DataStorageSection& storage =
            captured.dataStorageSections.front();
        const DRW_DataStorageSchema* thumbnail = schemaByIndex(0u);
        const DRW_DataStorageSchema* treated = schemaByIndex(1u);
        const DRW_DataStorageSchema* legacy = schemaByIndex(2u);
        const DRW_DataStorageSchema* indexable = schemaByIndex(3u);
        const DRW_DataStorageSchema* handleAttribute = schemaByIndex(4u);
        const DRW_DataStorageSchema* modeler = schemaByIndex(5u);
        const std::array<UTF8STRING, 8> propertyNames{{
            "AcDbDs::ID", "Thumbnail_Data", "AcDbDs::TreatedAsObjectData",
            "AcDbDs::Legacy", "AcDs:Indexable", "AcDbDs::HandleAttribute",
            "AcDbDs::ID", "ASM_Data"}};
        bool namesMatch = storage.schemaPropertyNameCount == 2u
            && storage.schemaPropertyNames.size() == 8u;
        if (namesMatch) {
            for (std::size_t index = 0; index < propertyNames.size(); ++index) {
                namesMatch = namesMatch
                    && storage.schemaPropertyNames[index]
                        == propertyNames[index];
            }
        }
        const bool propertiesMatch = namesMatch
            && thumbnail != nullptr && treated != nullptr && legacy != nullptr
            && indexable != nullptr && handleAttribute != nullptr
            && modeler != nullptr
            && thumbnail->segmentIndex == treated->segmentIndex
            && thumbnail->segmentIndex == legacy->segmentIndex
            && thumbnail->segmentIndex == indexable->segmentIndex
            && thumbnail->segmentIndex == handleAttribute->segmentIndex
            && thumbnail->segmentIndex != modeler->segmentIndex
            && thumbnail->properties.size() == 2u
            && propertyMatches(thumbnail, 0u, 0u, "AcDbDs::ID")
            && propertyMatches(thumbnail, 1u, 1u, "Thumbnail_Data")
            && treated != nullptr && treated->properties.size() == 1u
            && propertyMatches(treated, 0u, 2u,
                               "AcDbDs::TreatedAsObjectData")
            && legacy != nullptr && legacy->properties.size() == 1u
            && propertyMatches(legacy, 0u, 3u, "AcDbDs::Legacy")
            && indexable != nullptr && indexable->properties.size() == 1u
            && propertyMatches(indexable, 0u, 4u, "AcDs:Indexable")
            && handleAttribute != nullptr
            && handleAttribute->properties.size() == 1u
            && propertyMatches(handleAttribute, 0u, 5u,
                               "AcDbDs::HandleAttribute")
            && modeler->properties.size() == 2u
            && propertyMatches(modeler, 0u, 0u, "AcDbDs::ID")
            && propertyMatches(modeler, 1u, 1u, "ASM_Data");
        std::size_t thumbnailRecords = 0;
        std::size_t modelerRecords = 0;
        for (const DRW_DataStorageRecord& record : storage.records) {
            const bool isThumbnail = record.schemaIndex == 0u
                && !record.hasPayloadMarker && record.payload.size() >= 8u
                && record.payload[0] == 0x89u && record.payload[1] == 'P'
                && record.payload[2] == 'N' && record.payload[3] == 'G';
            const bool isModeler = record.schemaIndex == 5u
                && record.hasPayloadMarker
                && record.payload.size() >= 15u
                && std::equal(record.payload.begin(),
                              record.payload.begin() + 15,
                              "ACIS BinaryFile");
            thumbnailRecords += isThumbnail ? 1u : 0u;
            modelerRecords += isModeler ? 1u : 0u;
        }
        acdsSchemasMatch = storage.schemaCount == 6u && namesMatch
            && propertiesMatch && thumbnailRecords == 5u
            && modelerRecords == 3u;
        if (!acdsSchemasMatch) {
            std::cerr << "ACDS segment-local schema qualification failed: "
                      << "sections=" << captured.dataStorageSections.size()
                      << " schemas=" << storage.schemaCount
                      << " names=" << storage.schemaPropertyNames.size()
                      << " records=" << storage.records.size()
                      << " thumbnails=" << thumbnailRecords
                      << " modelers=" << modelerRecords
                      << " properties=" << propertiesMatch << '\n';
        }
    }

    const auto findObject = [&captured](const UTF8STRING& name,
                                        std::uint32_t handle)
        -> const DRW_AcShHistoryObject* {
        const DRW_AcShHistoryObject* found = nullptr;
        for (const DRW_AcShHistoryObject& object :
             captured.acshHistoryObjects) {
            if (object.m_recordName != name || object.handle != handle)
                continue;
            if (found != nullptr)
                return nullptr;
            found = &object;
        }
        return found;
    };
    const DRW_AcShHistoryObject* box =
        findObject("ACSH_BOX_CLASS", 0xD68u);
    const DRW_AcShHistoryObject* extrusion =
        findObject("ACSH_EXTRUSION_CLASS", 0xD6Du);
    const DRW_AcShHistoryObject* boxHistory =
        findObject("ACSH_HISTORY_CLASS", 0xD66u);
    const DRW_AcShHistoryObject* extrusionHistory =
        findObject("ACSH_HISTORY_CLASS", 0xD6Bu);
    const auto findGraph = [&captured](std::uint32_t handle)
        -> const DRW_EvaluationGraph* {
        const DRW_EvaluationGraph* found = nullptr;
        for (const DRW_EvaluationGraph& graph : captured.evaluationGraphs) {
            if (graph.handle != handle)
                continue;
            if (found != nullptr)
                return nullptr;
            found = &graph;
        }
        return found;
    };
    const DRW_EvaluationGraph* boxGraph = findGraph(0xD67u);
    const DRW_EvaluationGraph* extrusionGraph = findGraph(0xD6Cu);
    if (box == nullptr || extrusion == nullptr || boxHistory == nullptr
        || extrusionHistory == nullptr || boxGraph == nullptr
        || extrusionGraph == nullptr) {
        std::cerr << "ACSH sample object lookup failed; retained"
                  << captured.acshHistoryObjects.size() << " objects:";
        for (const DRW_AcShHistoryObject& object :
             captured.acshHistoryObjects) {
            std::cerr << ' ' << object.m_recordName << '@' << std::hex
                      << object.handle << std::dec;
        }
        std::cerr << '\n';
        return false;
    }

    const auto findModeler = [&captured](std::uint32_t handle)
        -> const DRW_ModelerGeometry* {
        const DRW_ModelerGeometry* found = nullptr;
        for (const DRW_Entity* entity : captured.mBlock->ent) {
            if (entity == nullptr || entity->handle != handle
                || (entity->eType != DRW::E3DSOLID
                    && entity->eType != DRW::REGION))
                continue;
            if (found != nullptr)
                return nullptr;
            found = static_cast<const DRW_ModelerGeometry*>(entity);
        }
        return found;
    };
    const DRW_ModelerGeometry* boxModeler = findModeler(0xD65u);
    const bool boxClosureLinksMatch = boxModeler != nullptr
        && boxHistory->m_major == 33u && boxHistory->m_minor == 329u
        && boxHistory->m_historyNodeId == 1u
        && boxModeler->m_historyHandle == boxHistory->handle
        && boxHistory->parentHandle == boxModeler->handle
        && boxHistory->m_ownerHandle == boxGraph->handle
        && boxGraph->parentHandle == boxHistory->handle
        && boxGraph->m_value96 == 1 && boxGraph->m_value97 == 1
        && boxGraph->m_nodes.size() == 1u
        && boxGraph->m_nodes.front().m_index == 0
        && boxGraph->m_nodes.front().m_flags == 32
        && boxGraph->m_nodes.front().m_expressionHandle == box->handle
        && boxGraph->m_edges.empty()
        && box->parentHandle == boxGraph->handle
        && box->m_historyNodePrefix.m_handle == 0x96u;
    const DRW_ModelerGeometry* extrusionModeler = findModeler(0xD6Au);
    const bool extrusionClosureLinksMatch = extrusionModeler != nullptr
        && extrusionModeler->m_historyHandle == extrusionHistory->handle
        && extrusionHistory->parentHandle == extrusionModeler->handle
        && extrusionHistory->m_ownerHandle == extrusionGraph->handle
        && extrusionGraph->parentHandle == extrusionHistory->handle
        && extrusionGraph->m_nodes.size() == 1u
        && extrusionGraph->m_nodes.front().m_expressionHandle
            == extrusion->handle
        && extrusion->parentHandle == extrusionGraph->handle;

    const std::array<double, 16> boxTransform{{
        1.0, 0.0, 0.0, 17.77672546982376,
        0.0, 1.0, 0.0, -220.8501226607159,
        0.0, 0.0, 1.0, 2.5,
        0.0, 0.0, 0.0, 1.0}};
    const std::array<double, 16> extrusionTransform{{
        1.0, 0.0, 0.0, 15.27672546982376,
        0.0, 1.0, 0.0, -223.3501226607159,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0}};
    const auto close = [](double left, double right) {
        return std::abs(left - right) <= 1e-12;
    };
    const auto prefixMatches = [&close](
        const DRW_AcShHistoryObject& object,
        const std::array<double, 16>& expectedTransform,
        std::int32_t expectedStep) {
        if (!object.m_evalExprPrefix.m_complete
            || object.m_evalExprPrefix.m_value98 != 33
            || object.m_evalExprPrefix.m_value99 != 329
            || object.m_evalExprPrefix.m_id != 1
            || object.m_evalExprPrefix.m_hasEvaluatedValue
            || !object.m_historyNodePrefix.m_complete
            || object.m_historyNodePrefix.m_major != 33
            || object.m_historyNodePrefix.m_minor != 329
            || object.m_historyNodePrefix.m_colorIndex != 256
            || object.m_historyNodePrefix.m_nodeValue != expectedStep
            || object.m_historyNodePrefix.m_handle != 0x96u)
            return false;
        for (std::size_t index = 0; index < expectedTransform.size(); ++index) {
            if (!close(object.m_historyNodePrefix.m_transform[index],
                       expectedTransform[index]))
                return false;
        }
        return true;
    };
    const bool prefixesMatch = prefixMatches(*box, boxTransform, 3)
        && prefixMatches(*extrusion, extrusionTransform, 4);
    if (!prefixesMatch) {
        const auto printPrefix = [](const DRW_AcShHistoryObject& object) {
            std::cerr << object.m_recordName << '@' << std::hex << object.handle
                      << std::dec << " eval=" << object.m_evalExprPrefix.m_complete
                      << ':' << object.m_evalExprPrefix.m_unknown << '/'
                      << object.m_evalExprPrefix.m_value98 << '/'
                      << object.m_evalExprPrefix.m_value99 << '/'
                      << object.m_evalExprPrefix.m_valueCode << '/'
                      << object.m_evalExprPrefix.m_id << " history="
                      << object.m_historyNodePrefix.m_complete << ':'
                      << object.m_historyNodePrefix.m_major << '/'
                      << object.m_historyNodePrefix.m_minor << '/'
                      << object.m_historyNodePrefix.m_colorIndex << '/'
                      << object.m_historyNodePrefix.m_nodeValue << '/'
                      << std::hex << object.m_historyNodePrefix.m_handle
                      << std::dec << " transform:";
            for (double value : object.m_historyNodePrefix.m_transform)
                std::cerr << ' ' << value;
            std::cerr << '\n';
        };
        printPrefix(*box);
        printPrefix(*extrusion);
    }
    if (prefixesMatch && boxClosureLinksMatch && extrusionClosureLinksMatch) {
        std::cout << "ACSH typed closure witness: modelers=2 graphs="
                  << captured.evaluationGraphs.size() << " history="
                  << captured.acshHistoryObjects.size() << " materials="
                  << captured.materials.size() << " proxies="
                  << captured.proxyObjects.size() << " linked-box=yes"
                  << " linked-extrusion=yes box-history="
                  << boxHistory->m_major << '/' << boxHistory->m_minor << '/'
                  << boxHistory->m_historyNodeId << '\n';
        for (const DRW_EvaluationGraph& graph : captured.evaluationGraphs) {
            if (graph.handle == boxGraph->handle
                || graph.handle == extrusionGraph->handle)
                continue;
            const bool parentIsHistory = std::any_of(
                captured.acshHistoryObjects.begin(),
                captured.acshHistoryObjects.end(),
                [&graph](const DRW_AcShHistoryObject& object) {
                    return object.handle == graph.parentHandle;
                });
            const bool parentIsModeler = std::any_of(
                captured.mBlock->ent.begin(), captured.mBlock->ent.end(),
                [&graph](const DRW_Entity* entity) {
                    return entity != nullptr
                        && entity->handle == graph.parentHandle;
                });
            std::cout << "ACSH additional graph: handle=" << std::hex
                      << graph.handle << " parent=" << graph.parentHandle
                      << std::dec << " parent-is-history=" << parentIsHistory
                      << " parent-is-modeler=" << parentIsModeler
                      << " nodes=" << graph.m_nodes.size()
                      << " edges=" << graph.m_edges.size() << " targets=";
            for (const DRW_EvaluationGraphNode& node : graph.m_nodes)
                std::cout << ' ' << std::hex << node.m_expressionHandle
                          << std::dec;
            std::cout << '\n';
            std::vector<std::uint32_t> associatedHandles{graph.parentHandle};
            for (const DRW_DwgFramePublication& frame : iface.frames) {
                if (frame.m_handle == graph.parentHandle
                    && frame.m_parentHandle != DRW::NoHandle
                    && frame.m_parentHandle != 0)
                    associatedHandles.push_back(frame.m_parentHandle);
            }
            for (const DRW_EvaluationGraphNode& node : graph.m_nodes) {
                if (node.m_expressionHandle != 0)
                    associatedHandles.push_back(node.m_expressionHandle);
            }
            std::sort(associatedHandles.begin(), associatedHandles.end());
            associatedHandles.erase(
                std::unique(associatedHandles.begin(), associatedHandles.end()),
                associatedHandles.end());
            for (const std::uint32_t handle : associatedHandles) {
                for (const DRW_DwgFramePublication& frame : iface.frames) {
                    if (frame.m_handle != handle)
                        continue;
                    std::cout << "ACSH additional graph closure frame: handle="
                              << std::hex << frame.m_handle << std::dec
                              << " record=" << frame.m_recordName
                              << " class=" << frame.m_className
                              << " encoded-type=" << frame.m_encodedType
                              << " resolved-type=" << frame.m_resolvedType
                              << " parent=" << std::hex
                              << frame.m_parentHandle << std::dec << '\n';
                }
            }
        }
    }
    return acdsSchemasMatch && prefixesMatch && boxClosureLinksMatch
        && extrusionClosureLinksMatch
        && box->m_shapeParams == std::vector<double>({5.0, 5.0, 5.0})
        && close(extrusion->m_direction.z, 4.380487155114503);
}

bool runDxfEvaluationGraphRoundTrip(
    bool binary, const std::filesystem::path& directory, bool keepOutput) {
    const std::string encoding = binary ? "binary" : "ascii";
    const std::filesystem::path output = directory /
        ("libdxfrw-ac1027-evaluation-graph-" + encoding + ".dxf");
    const std::filesystem::path rejectedOutput = directory /
        ("libdxfrw-ac1018-evaluation-graph-" + encoding + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);
    std::filesystem::remove(rejectedOutput, ec);

    dx_data source;
    auto* line = new DRW_Line();
    line->handle = DRW::DwgNamedObjectsDictionaryHandle;
    const std::uint32_t sourceLineHandle = line->handle;
    line->basePoint = DRW_Coord(1.0, 2.0, 3.0);
    line->secPoint = DRW_Coord(4.0, 5.0, 6.0);
    source.mBlock->ent.push_back(line);

    DRW_EvaluationGraph graph;
    graph.handle = 0xD67u;
    graph.m_value96 = 26;
    graph.m_value97 = 26;
    DRW_EvaluationGraphNode node;
    node.m_index = 0;
    node.m_flags = 32;
    node.m_nextNodeIndex = 1;
    node.m_expressionHandle = line->handle;
    node.m_data1 = 2;
    node.m_data2 = 5;
    node.m_data3 = 0;
    node.m_data4 = 6;
    graph.m_nodes.push_back(node);
    DRW_EvaluationGraphEdge edge;
    edge.m_value92 = -1;
    edge.m_value93 = -1;
    edge.m_value94 = 2;
    edge.m_value91a = 2;
    edge.m_value91b = 2;
    edge.m_value92a = 1;
    edge.m_value92b = 2;
    edge.m_value92c = 3;
    edge.m_value92d = 4;
    edge.m_value92e = 5;
    graph.m_edges.push_back(edge);
    source.evaluationGraphs.push_back(graph);

    DRW_Class aliasClass;
    if (!dxfRW::dxfClassForRecordName("EVALUATION_GRAPH", aliasClass))
        return false;
    aliasClass.instanceCount = 99;
    source.dxfClasses.push_back(aliasClass);

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary,
                             &source, false)) {
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }
    dx_data imported;
    dx_iface importer;
    if (!importer.fileImport(output.string(), &imported, false)) {
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }

    const DRW_Line* importedLine = nullptr;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::LINE) {
            importedLine = static_cast<const DRW_Line*>(entity);
            break;
        }
    }
    const bool classesMatch = imported.dxfClasses.size() == 1u
        && imported.dxfClasses.front().recName == "ACAD_EVALUATION_GRAPH"
        && imported.dxfClasses.front().instanceCount == 1;
    const bool graphMatch = imported.evaluationGraphs.size() == 1u
        && imported.evaluationGraphs.front().handle == graph.handle
        && imported.evaluationGraphs.front().parentHandle
               == DRW::DwgNamedObjectsDictionaryHandle
        && imported.evaluationGraphs.front().m_value96 == graph.m_value96
        && imported.evaluationGraphs.front().m_value97 == graph.m_value97
        && imported.evaluationGraphs.front().m_nodes.size() == 1u
        && imported.evaluationGraphs.front().m_nodes.front().m_flags == 32
        && imported.evaluationGraphs.front().m_nodes.front()
               .m_expressionHandle != sourceLineHandle
        && imported.evaluationGraphs.front().m_nodes.front()
               .m_expressionHandle
               == (importedLine == nullptr ? 0u : importedLine->handle)
        && imported.evaluationGraphs.front().m_nodes.front().m_data4 == 6
        && imported.evaluationGraphs.front().m_edges.size() == 1u
        && imported.evaluationGraphs.front().m_edges.front().m_value92 == -1
        && imported.evaluationGraphs.front().m_edges.front().m_value92e == 5;

    // AC1021 is the minimum version for this typed OBJECT. A rejected older
    // target must not publish a partial DXF at its final path.
    dx_iface oldVersionExporter;
    const bool oldVersionRejected =
        !oldVersionExporter.fileExport(rejectedOutput.string(), DRW::AC1018,
                                       binary, &source, false)
        && !std::filesystem::exists(rejectedOutput);
    if (!keepOutput)
        std::filesystem::remove(output, ec);
    if (!(importedLine != nullptr && classesMatch && graphMatch
          && oldVersionRejected)) {
        std::cerr << "EvaluationGraph round-trip details (" << encoding
                  << "): line=" << (importedLine != nullptr)
                  << " class=" << classesMatch << " graphs="
                  << imported.evaluationGraphs.size() << " graph="
                  << graphMatch << " old-version=" << oldVersionRejected
                  << '\n';
        if (!imported.evaluationGraphs.empty()) {
            const DRW_EvaluationGraph& actual =
                imported.evaluationGraphs.front();
            std::cerr << "EvaluationGraph values: handle=" << actual.handle
                      << " owner=" << actual.parentHandle << " nodes="
                      << actual.m_nodes.size() << " edges="
                      << actual.m_edges.size();
            if (!actual.m_nodes.empty()) {
                std::cerr << " expression="
                          << actual.m_nodes.front().m_expressionHandle
                          << " target="
                          << (importedLine == nullptr
                                  ? 0u : importedLine->handle);
            }
            std::cerr << '\n';
        }
    }
    return importedLine != nullptr && classesMatch && graphMatch
        && oldVersionRejected;
}

bool runDxfAcdsDataStorageProjection(
    bool binary, const std::filesystem::path& directory, bool keepOutput,
    bool reverseCallbackCollections = false,
    bool reverseEntityOrder = false,
    bool reassignedClassIds = false) {
    const std::string encoding = binary ? "binary" : "ascii";
    const std::filesystem::path output = directory /
        ("libdxfrw-ac1027-acds-projection-" + encoding
         + (reverseCallbackCollections ? "-reversed-callbacks" : "")
         + (reverseEntityOrder ? "-reversed-entities" : "")
         + (reassignedClassIds ? "-reassigned-classes" : "")
         + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    constexpr std::uint32_t sourceHandle = 0xFC20u;
    const std::vector<std::uint8_t> payload {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e',
        0x01u, 0x02u, 0x03u, 0x04u};
    dx_data source;
    auto* neighborLine = new DRW_Line();
    neighborLine->basePoint = DRW_Coord(10.0, 20.0, 30.0);
    neighborLine->secPoint = DRW_Coord(14.0, 25.0, 36.0);
    if (!reverseEntityOrder)
        source.mBlock->ent.push_back(neighborLine);
    addLocalAcdsDataStorageModeler(source, sourceHandle, payload);
    if (reassignedClassIds) {
        const std::int32_t renumbered[] = {500, 501, 502};
        std::size_t index = 0;
        for (DRW_ProxyObject& proxy : source.proxyObjects)
            proxy.m_proxyClassId = renumbered[index++];
        index = 0;
        for (DRW_DwgClassCoverageEntry& cls :
             source.dwgClassCoverage.m_entries) {
            cls.m_classNumber =
                static_cast<std::uint16_t>(renumbered[index++]);
        }
    }
    if (reverseEntityOrder)
        source.mBlock->ent.push_back(neighborLine);
    if (reverseCallbackCollections) {
        source.proxyObjects.reverse();
        source.materials.reverse();
        source.dictionaries.reverse();
    }

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary,
                             &source, false)) {
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }

    dx_data imported;
    dx_iface importer;
    const bool readOk = importer.fileImport(output.string(), &imported, false);
    const DRW_ModelerGeometry* importedModeler = nullptr;
    if (readOk) {
        for (const DRW_Entity* entity : imported.mBlock->ent) {
            if (entity != nullptr && entity->eType == DRW::E3DSOLID) {
                importedModeler =
                    static_cast<const DRW_ModelerGeometry*>(entity);
                break;
            }
        }
    }
    std::string linkedHandle;
    std::vector<std::uint8_t> parsedPayload;
    char expectedHandle[9] = {};
    if (importedModeler != nullptr)
        std::snprintf(expectedHandle, sizeof(expectedHandle), "%X",
                      importedModeler->handle);
    const bool schemaMatch = imported.rawDxfSections.size() == 1u
        && imported.rawDxfSections.front().m_name == "ACDSDATA"
        && hasExpectedAcdsSchemaDefinitions(imported.rawDxfSections.front());
    const bool proxyTerminators = readOk
        && std::all_of(imported.rawProxyObjects.begin(),
                       imported.rawProxyObjects.end(),
                       [](const DRW_RawDxfObject& object) {
                           return std::count_if(
                                      object.groups.begin(),
                                      object.groups.end(),
                                      [](const DRW_Variant& group) {
                                          return group.code() == 94
                                              && group.type()
                                                     == DRW_Variant::INTEGER
                                              && group.i_val() == 0;
                                      }) == 1;
                       });
    const std::array<std::pair<const char*, int>, 4> expectedClasses {{
        {"ACSH_HISTORY_CLASS", 1},
        {"ACAD_EVALUATION_GRAPH", 1},
        {"ACSH_CONE_CLASS", 1},
        {"MATERIAL", 3}
    }};
    bool classesMatch = imported.dxfClasses.size() == expectedClasses.size();
    if (classesMatch) {
        for (std::size_t index = 0; index < expectedClasses.size(); ++index) {
            const DRW_Class& cls = imported.dxfClasses[index];
            if (cls.recName != expectedClasses[index].first
                || cls.instanceCount != expectedClasses[index].second
                || cls.wasaProxyFlag != (index < 3u ? 1 : 0)
                || cls.entityFlag != 0) {
                classesMatch = false;
                break;
            }
        }
    }
    const bool neighborLineMatch = readOk
        && std::any_of(imported.mBlock->ent.begin(), imported.mBlock->ent.end(),
                       [](const DRW_Entity* entity) {
                           if (entity == nullptr || entity->eType != DRW::LINE)
                               return false;
                           const auto* line =
                               static_cast<const DRW_Line*>(entity);
                           return line->basePoint.x == 10.0
                               && line->basePoint.y == 20.0
                               && line->basePoint.z == 30.0
                               && line->secPoint.x == 14.0
                               && line->secPoint.y == 25.0
                               && line->secPoint.z == 36.0;
                       });
    bool result = importedModeler != nullptr
        && neighborLineMatch
        && importedModeler->handle != sourceHandle
        && importedModeler->m_historyHandle == 0x20Eu
        && schemaMatch
        && classesMatch
        && findAcdsRecordPayload(imported.rawDxfSections.front(), "ASM_Data",
                                 linkedHandle, parsedPayload)
        && linkedHandle == expectedHandle
        && parsedPayload == payload
        && imported.proxyObjects.size() == 3u
        && imported.rawProxyObjects.size() == 3u
        && proxyTerminators
        && std::any_of(imported.dictionaries.begin(),
                       imported.dictionaries.end(),
                       [](const DRW_Dictionary& dictionary) {
                           return dictionary.handle == 0x72u
                               && dictionary.parentHandle == 0xCu
                               && dictionary.m_entries.size() == 3u;
                       })
        && imported.materials.size() == 3u;
    if (result) {
        std::unordered_map<std::uint32_t, const DRW_ProxyObject*> proxies;
        for (const DRW_ProxyObject& value : imported.proxyObjects)
            proxies.emplace(value.handle, &value);
        const auto history = proxies.find(0x20Eu);
        const auto evaluation = proxies.find(0x20Du);
        const auto operation = proxies.find(0x20Cu);
        const auto hasReference = [](const DRW_ProxyObject* value,
                                     int code, std::uint32_t handle) {
            const std::uint8_t handleCode = code == 360 ? 3u : 5u;
            return value != nullptr
                && std::any_of(value->m_objectIdRefs.begin(),
                               value->m_objectIdRefs.end(),
                               [code, handle, handleCode](
                                   const DRW_ProxyObjectIdRef& reference) {
                                   return reference.m_dxfCode == code
                                       && reference.m_handleCode == handleCode
                                       && reference.m_handle == handle;
                               });
        };
        const auto hasPayload = [](const DRW_ProxyObject* value,
                                   std::uint32_t bits, std::uint8_t byte) {
            if (value == nullptr || value->m_objectDataBitSize != bits
                || value->m_objectData.size() != (bits + 7u) / 8u)
                return false;
            for (std::size_t index = 0; index < value->m_objectData.size();
                 ++index) {
                const std::uint8_t expected =
                    (index + 1u == value->m_objectData.size()
                     && (bits & 7u) != 0u)
                        ? static_cast<std::uint8_t>(
                              byte & (0xFFu << (8u - (bits & 7u))))
                        : byte;
                if (value->m_objectData[index] != expected)
                    return false;
            }
            return true;
        };
        if (history == proxies.end() || evaluation == proxies.end()
            || operation == proxies.end()
            || history->second->parentHandle != importedModeler->handle
            || evaluation->second->parentHandle != 0x20Eu
            || operation->second->parentHandle != 0x20Du
            || history->second->m_proxyClassId != 500
            || evaluation->second->m_proxyClassId != 501
            || operation->second->m_proxyClassId != 502
            || !hasReference(history->second, 360, 0x20Du)
            || !hasReference(evaluation->second, 360, 0x20Cu)
            || !hasReference(operation->second, 340, 0x96u)
            || !hasPayload(history->second, 32, 0x0Eu)
            || !hasPayload(evaluation->second, 190, 0x0Du)
            || !hasPayload(operation->second, 558, 0x0Cu)) {
            std::filesystem::remove(output, ec);
            return false;
        }
    }
    if (result && !reverseCallbackCollections && !reverseEntityOrder) {
        // Re-exporting DXF input uses the captured CLASS records, not the
        // DWG DataStorage projection. Exercise that distinct writer branch.
        const std::filesystem::path replay = directory /
            (output.stem().string() + "-replay.dxf");
        std::filesystem::remove(replay, ec);
        dx_iface replayExporter;
        dx_data replayed;
        dx_iface replayImporter;
        result = replayExporter.fileExport(replay.string(), DRW::AC1027,
                                           binary, &imported, false)
            && replayImporter.fileImport(replay.string(), &replayed, false)
            && replayed.dxfClasses.size() == imported.dxfClasses.size()
            && replayed.rawProxyObjects.size() == 3u
            && replayed.rawDxfSections.size() == 1u;
        if (result) {
            for (std::size_t index = 0; index < imported.dxfClasses.size();
                 ++index) {
                const DRW_Class& before = imported.dxfClasses[index];
                const DRW_Class& after = replayed.dxfClasses[index];
                if (before.recName != after.recName
                    || before.className != after.className
                    || before.instanceCount != after.instanceCount
                    || before.wasaProxyFlag != after.wasaProxyFlag) {
                    result = false;
                    break;
                }
            }
        }
        if (result) {
            const DRW_ModelerGeometry* replayedModeler = nullptr;
            for (const DRW_Entity* entity : replayed.mBlock->ent) {
                if (entity != nullptr && entity->eType == DRW::E3DSOLID) {
                    replayedModeler =
                        static_cast<const DRW_ModelerGeometry*>(entity);
                    break;
                }
            }
            std::string replayedKey;
            std::vector<std::uint8_t> replayedPayload;
            char replayedExpectedHandle[9] = {};
            if (replayedModeler != nullptr) {
                std::snprintf(replayedExpectedHandle,
                              sizeof(replayedExpectedHandle), "%X",
                              replayedModeler->handle);
            }
            result = replayedModeler != nullptr
                && replayedModeler->m_historyHandle == 0x20Eu
                && findAcdsRecordPayload(replayed.rawDxfSections.front(),
                                         "ASM_Data", replayedKey,
                                         replayedPayload)
                && replayedKey == replayedExpectedHandle
                && replayedPayload == payload;
        }
        if (!keepOutput || !result)
            std::filesystem::remove(replay, ec);
    }
    if (!keepOutput || !result)
        std::filesystem::remove(output, ec);
    return result;
}

bool runDxfAcdsRegionWithoutHistoryProjection(
    bool binary, const std::filesystem::path& directory, bool keepOutputs) {
    const std::string encoding = binary ? "binary" : "ascii";
    const std::filesystem::path output = directory /
        ("libdxfrw-ac1027-acds-region-no-history-" + encoding + ".dxf");
    const std::filesystem::path replay = directory /
        ("libdxfrw-ac1027-acds-region-no-history-replay-" + encoding
         + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);
    std::filesystem::remove(replay, ec);

    constexpr std::uint32_t sourceHandle = 0xFC20u;
    // Deliberately opaque marker bytes: this regression checks the REGION /
    // ACDSDATA transport and owner key only, not ACIS validity or geometry.
    const std::vector<std::uint8_t> payload {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e',
        0x01u, 0x02u, 0x03u, 0x04u};
    dx_data source;
    addLocalAcdsDataStorageModeler(source, sourceHandle, payload,
                                   DRW::REGION, false);

    const auto hasExpectedRegionAndStorage = [&payload](const dx_data& data) {
        if (data.mBlock == nullptr || data.rawDxfSections.size() != 1u
            || data.rawDxfSections.front().m_name != "ACDSDATA"
            || !hasExpectedAcdsSchemaDefinitions(
                data.rawDxfSections.front())
            || !data.proxyObjects.empty() || !data.rawProxyObjects.empty()
            || !data.acshHistoryObjects.empty()
            || !data.evaluationGraphs.empty()) {
            std::cerr << "REGION ACDSDATA structure mismatch: block="
                      << (data.mBlock != nullptr) << " sections="
                      << data.rawDxfSections.size() << " proxy="
                      << data.proxyObjects.size() << '/'
                      << data.rawProxyObjects.size() << " history="
                      << data.acshHistoryObjects.size() << " graphs="
                      << data.evaluationGraphs.size() << '\n';
            return false;
        }

        const DRW_ModelerGeometry* region = nullptr;
        std::size_t regionCount = 0u;
        std::size_t solidCount = 0u;
        for (const DRW_Entity* entity : data.mBlock->ent) {
            if (entity == nullptr)
                continue;
            if (entity->eType == DRW::REGION) {
                ++regionCount;
                region = static_cast<const DRW_ModelerGeometry*>(entity);
            } else if (entity->eType == DRW::E3DSOLID) {
                ++solidCount;
            }
        }
        if (regionCount != 1u || solidCount != 0u || region == nullptr
            || region->handle == 0u || region->m_historyHandle != 0u) {
            std::cerr << "REGION ACDSDATA entity mismatch: regions="
                      << regionCount << " solids=" << solidCount
                      << " handle=" << (region == nullptr ? 0u
                                                           : region->handle)
                      << " history=" << (region == nullptr ? 0u
                                                            : region->m_historyHandle)
                      << '\n';
            return false;
        }

        char expectedHandle[9] = {};
        std::snprintf(expectedHandle, sizeof(expectedHandle), "%X",
                      region->handle);
        std::string linkedHandle;
        std::vector<std::uint8_t> parsedPayload;
        const bool recordMatches = findAcdsRecordPayload(
            data.rawDxfSections.front(), "ASM_Data", linkedHandle,
            parsedPayload)
            && linkedHandle == expectedHandle && parsedPayload == payload;
        if (!recordMatches) {
            std::cerr << "REGION ACDSDATA record mismatch: expected owner="
                      << expectedHandle << " actual owner=" << linkedHandle
                      << " bytes=" << parsedPayload.size() << '\n';
        }
        return recordMatches;
    };

    dx_iface exporter;
    dx_data imported;
    dx_iface importer;
    const bool exportOk = exporter.fileExport(output.string(), DRW::AC1027,
                                              binary, &source, false);
    const bool importOk = exportOk
        && importer.fileImport(output.string(), &imported, false);
    bool result = importOk && hasExpectedRegionAndStorage(imported);
    if (!result) {
        std::cerr << "REGION ACDSDATA first pass failed: export=" << exportOk
                  << " import=" << importOk << '\n';
    }

    // Re-exporting imported DXF exercises the separate DXF ACDSDATA replay
    // path, including the REGION handle-to-record owner mapping.
    if (result) {
        dx_iface replayExporter;
        dx_data replayed;
        dx_iface replayImporter;
        result = replayExporter.fileExport(replay.string(), DRW::AC1027,
                                           binary, &imported, false)
            && replayImporter.fileImport(replay.string(), &replayed, false)
            && hasExpectedRegionAndStorage(replayed);
    }

    if (!keepOutputs) {
        std::filesystem::remove(output, ec);
        std::filesystem::remove(replay, ec);
    }
    return result;
}

bool runDxfAcdsHistoryClosureRejectsAmbiguity(
    const std::filesystem::path& directory, bool duplicateHistoryProxy,
    bool duplicateMaterialDictionary) {
    const std::string suffix = duplicateHistoryProxy
        ? "duplicate-proxy" : "duplicate-material-dictionary";
    const std::filesystem::path output = directory /
        ("libdxfrw-ac1027-acds-" + suffix + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);
    const std::vector<std::uint8_t> payload {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e',
        0x01u, 0x02u, 0x03u, 0x04u};
    dx_data source;
    addLocalAcdsDataStorageModeler(source, 0xFC20u, payload);
    if (duplicateHistoryProxy) {
        source.proxyObjects.push_back(source.proxyObjects.front());
    } else {
        DRW_Dictionary duplicate = source.dictionaries.front();
        duplicate.handle = 0x73u;
        source.dictionaries.push_back(std::move(duplicate));
    }

    dx_iface exporter;
    const bool exportOk = exporter.fileExport(
        output.string(), DRW::AC1027, false, &source, false);
    const bool rejectedWithoutPublication = !exportOk
        && !std::filesystem::exists(output);
    std::filesystem::remove(output, ec);
    return rejectedWithoutPublication;
}

bool runDxfAcdsHistoryClosureRejectsMixedClasses(
    const std::filesystem::path& directory) {
    const std::vector<std::uint8_t> payload {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e',
        0x01u, 0x02u, 0x03u, 0x04u};
    for (int mixed = 0; mixed < 2; ++mixed) {
        const std::filesystem::path output = directory /
            (mixed == 0 ? "libdxfrw-acds-mixed-raw-proxy.dxf"
                        : "libdxfrw-acds-mixed-class.dxf");
        std::error_code ec;
        std::filesystem::remove(output, ec);
        dx_data source;
        addLocalAcdsDataStorageModeler(source, 0xFC20u, payload);
        if (mixed == 0)
            source.rawProxyObjects.emplace_back();
        else
            source.dxfClasses.emplace_back();
        dx_iface exporter;
        if (exporter.fileExport(output.string(), DRW::AC1027, false,
                                &source, false)
            || std::filesystem::exists(output)) {
            std::filesystem::remove(output, ec);
            return false;
        }
    }
    return true;
}

bool runDxfAcdsSchemaFingerprintRejectsMismatch(
    const std::filesystem::path& directory) {
    const std::vector<std::uint8_t> payload {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e',
        0x01u, 0x02u, 0x03u, 0x04u};
    for (int mismatch = 0; mismatch < 3; ++mismatch) {
        const std::filesystem::path output = directory /
            ("libdxfrw-ac1027-acds-schema-mismatch-"
             + std::to_string(mismatch) + ".dxf");
        std::error_code ec;
        std::filesystem::remove(output, ec);
        dx_data source;
        addLocalAcdsDataStorageModeler(source, 0xFC20u, payload);
        DRW_DataStorageSection& storage = source.dataStorageSections.front();
        if (mismatch == 0) {
            storage.schemaPropertyNames[2] = "Unqualified_Property";
        } else if (mismatch == 1) {
            storage.schemas[1].properties[1].type = 14u;
        } else {
            storage.schemas[0].indexes[0] = 5u;
        }

        dx_iface exporter;
        const bool exportOk = exporter.fileExport(
            output.string(), DRW::AC1027, false, &source, false);
        const bool rejectedWithoutPublication = !exportOk
            && !std::filesystem::exists(output);
        std::filesystem::remove(output, ec);
        if (!rejectedWithoutPublication)
            return false;
    }
    return true;
}

bool runDxfAcdsHistoryClosureRejectsMalformedEdges(
    const std::filesystem::path& directory) {
    static const std::array<const char*, 11> cases = {
        "missing-history", "wrong-class", "wrong-owner", "wrong-reference",
        "missing-material", "wrong-subclass", "proxy-bit-size",
        "proxy-padding", "missing-class-report", "duplicate-class-binding",
        "wrong-class-name"};
    const std::vector<std::uint8_t> payload {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e',
        0x01u, 0x02u, 0x03u, 0x04u};
    for (std::size_t index = 0; index < cases.size(); ++index) {
        const std::filesystem::path output = directory /
            (std::string("libdxfrw-ac1027-acds-edge-") + cases[index]
             + ".dxf");
        std::error_code ec;
        std::filesystem::remove(output, ec);
        dx_data source;
        addLocalAcdsDataStorageModeler(source, 0xFC20u, payload);
        switch (index) {
        case 0:
            source.proxyObjects.pop_front();
            break;
        case 1:
            source.proxyObjects.front().m_proxyClassId = 520;
            break;
        case 2:
            source.proxyObjects.front().parentHandle = 0x1234u;
            break;
        case 3:
            source.proxyObjects.front().m_objectIdRefs.front().m_dxfCode = 340;
            break;
        case 4:
            source.materials.remove_if([](const DRW_Material& material) {
                return material.handle == 0x96u;
            });
            break;
        case 5:
            source.proxyObjects.front().m_proxySubclass = "cn:Other";
            break;
        case 6:
            ++source.proxyObjects.front().m_objectDataBitSize;
            break;
        case 7:
            source.proxyObjects.back().m_objectData.back() |= 1u;
            break;
        case 8:
            source.dwgClassCoverage.m_complete = false;
            break;
        case 9:
            source.dwgClassCoverage.m_entries.push_back(
                source.dwgClassCoverage.m_entries.front());
            break;
        case 10:
            source.dwgClassCoverage.m_entries.front().m_className = "Other";
            break;
        }

        dx_iface exporter;
        const bool exportOk = exporter.fileExport(
            output.string(), DRW::AC1027, false, &source, false);
        const bool rejectedWithoutPublication = !exportOk
            && !std::filesystem::exists(output);
        std::filesystem::remove(output, ec);
        if (!rejectedWithoutPublication)
            return false;
    }
    return true;
}

bool findAcdsRecordOwner(const DRW_RawDxfSection& section,
                         const std::string& recordPayloadName,
                         std::string& ownerHandle) {
    const auto equalsAsciiCaseInsensitive = [](const std::string& lhs,
                                                const char* rhs) {
        if (rhs == nullptr || lhs.size() != std::strlen(rhs))
            return false;
        for (std::size_t index = 0; index < lhs.size(); ++index) {
            if (std::toupper(static_cast<unsigned char>(lhs[index]))
                != std::toupper(static_cast<unsigned char>(rhs[index])))
                return false;
        }
        return true;
    };
    const auto& groups = section.m_groups;
    for (std::size_t recordStart = 0; recordStart < groups.size();) {
        if (groups[recordStart].code() != 0
            || groups[recordStart].type() != DRW_Variant::STRING
            || groups[recordStart].c_str() == nullptr) {
            ++recordStart;
            continue;
        }
        std::size_t recordEnd = recordStart + 1;
        while (recordEnd < groups.size() && groups[recordEnd].code() != 0)
            ++recordEnd;
        if (equalsAsciiCaseInsensitive(groups[recordStart].c_str(),
                                       "ACDSRECORD")) {
            bool inId = false;
            bool hasPayload = false;
            std::size_t ownerCount = 0;
            std::string candidate;
            for (std::size_t index = recordStart + 1; index < recordEnd;
                 ++index) {
                const DRW_Variant& group = groups[index];
                if (group.code() == 2
                    && group.type() == DRW_Variant::STRING
                    && group.c_str() != nullptr) {
                    inId = equalsAsciiCaseInsensitive(group.c_str(),
                                                      "AcDbDs::ID");
                    if (equalsAsciiCaseInsensitive(group.c_str(),
                                                   recordPayloadName.c_str()))
                        hasPayload = true;
                } else if (group.code() == 320
                           && (recordPayloadName != "ASM_Data" || inId)) {
                    ++ownerCount;
                    if (group.type() == DRW_Variant::STRING
                        && group.c_str() != nullptr)
                        candidate = group.c_str();
                }
            }
            if (hasPayload) {
                if (ownerCount != 1 || candidate.empty())
                    return false;
                ownerHandle = candidate;
                return true;
            }
        }
        recordStart = recordEnd;
    }
    return false;
}

bool runDxfDimstyleRejectsInvalidBooleans(
    const std::filesystem::path& directory) {
    for (bool binary : {false, true}) {
        for (int code : {290, 295}) {
            for (int value : {-1, 2}) {
                const std::filesystem::path output = directory /
                    ("libdxfrw-invalid-dimstyle-bool-"
                     + std::to_string(code) + "-" + std::to_string(value)
                     + (binary ? "-binary.dxf" : "-ascii.dxf"));
                std::error_code ec;
                std::filesystem::remove(output, ec);

                dx_data source;
                DRW_Dimstyle style;
                style.name = "LOCAL_INVALID_DIMSTYLE";
                if (code == 290)
                    style.dimfxlon = value;
                else
                    style.add("$DIMTXTDIRECTION", code, value);
                source.dimStyles.push_back(style);

                dx_iface exporter;
                const bool exported = exporter.fileExport(
                    output.string(), DRW::AC1027, binary, &source, false);
                const bool outputAbsent = !std::filesystem::exists(output);
                std::filesystem::remove(output, ec);
                if (exported || !outputAbsent)
                    return false;
            }
        }
    }
    return true;
}

bool runDxfViewportLightingRoundTrip(
    bool binary, const std::filesystem::path& directory) {
    const std::filesystem::path output = directory /
        (binary ? "libdxfrw-viewport-lighting-binary.dxf"
                : "libdxfrw-viewport-lighting-ascii.dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    dx_data source;
    auto* viewport = new DRW_Viewport();
    viewport->basePoint = DRW_Coord(10.0, 20.0, 30.0);
    viewport->vpID = 7;
    viewport->useDefaultLighting = false;
    viewport->defaultLightingType = 2;
    viewport->brightness = 0.25;
    viewport->contrast = 0.75;
    source.mBlock->ent.push_back(viewport);

    dx_iface exporter;
    const bool exportOk = exporter.fileExport(
        output.string(), DRW::AC1027, binary, &source, false);
    dx_data imported;
    dx_iface importer;
    const bool importOk = exportOk
        && importer.fileImport(output.string(), &imported, false);
    const DRW_Viewport* roundTripped = nullptr;
    if (importOk) {
        for (const DRW_Entity* entity : imported.mBlock->ent) {
            if (entity != nullptr && entity->eType == DRW::VIEWPORT) {
                roundTripped = static_cast<const DRW_Viewport*>(entity);
                break;
            }
        }
    }
    const bool result = roundTripped != nullptr
        && roundTripped->basePoint.x == 10.0
        && roundTripped->basePoint.y == 20.0
        && roundTripped->basePoint.z == 30.0
        && roundTripped->vpID == 7
        && !roundTripped->useDefaultLighting
        && roundTripped->defaultLightingType == 2
        && roundTripped->brightness == 0.25
        && roundTripped->contrast == 0.75;
    std::filesystem::remove(output, ec);
    return result;
}

bool runDxfRayXlineRoundTrip(
    bool binary, const std::filesystem::path& directory) {
    const std::filesystem::path output = directory /
        (binary ? "libdxfrw-ray-xline-binary.dxf"
                : "libdxfrw-ray-xline-ascii.dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    dx_data source;
    auto* ray = new DRW_Ray();
    ray->basePoint = DRW_Coord(1.0, 2.0, 3.0);
    ray->secPoint = DRW_Coord(0.0, 0.0, 1.0);
    source.mBlock->ent.push_back(ray);
    auto* xline = new DRW_Xline();
    xline->basePoint = DRW_Coord(4.0, 5.0, 6.0);
    xline->secPoint = DRW_Coord(0.0, 0.6, 0.8);
    source.mBlock->ent.push_back(xline);

    dx_iface exporter;
    const bool exportOk = exporter.fileExport(
        output.string(), DRW::AC1027, binary, &source, false);
    dx_data imported;
    dx_iface importer;
    const bool importOk = exportOk
        && importer.fileImport(output.string(), &imported, false);
    const DRW_Ray* roundTrippedRay = nullptr;
    const DRW_Xline* roundTrippedXline = nullptr;
    if (importOk) {
        for (const DRW_Entity* entity : imported.mBlock->ent) {
            if (entity == nullptr)
                continue;
            if (entity->eType == DRW::RAY)
                roundTrippedRay = static_cast<const DRW_Ray*>(entity);
            else if (entity->eType == DRW::XLINE)
                roundTrippedXline = static_cast<const DRW_Xline*>(entity);
        }
    }
    const bool result = roundTrippedRay != nullptr
        && roundTrippedXline != nullptr
        && roundTrippedRay->basePoint.x == 1.0
        && roundTrippedRay->basePoint.y == 2.0
        && roundTrippedRay->basePoint.z == 3.0
        && roundTrippedRay->secPoint.x == 0.0
        && roundTrippedRay->secPoint.y == 0.0
        && roundTrippedRay->secPoint.z == 1.0
        && roundTrippedXline->basePoint.x == 4.0
        && roundTrippedXline->basePoint.y == 5.0
        && roundTrippedXline->basePoint.z == 6.0
        && roundTrippedXline->secPoint.x == 0.0
        && roundTrippedXline->secPoint.y == 0.6
        && roundTrippedXline->secPoint.z == 0.8;
    std::filesystem::remove(output, ec);
    return result;
}

bool runDxfAcdsModelerOwnerRemap(bool binary,
                                 const std::filesystem::path& directory,
                                 bool keepOutputs) {
    const std::string encoding = binary ? "binary" : "ascii";
    const std::filesystem::path output = directory
        / ("libdxfrw-ac1027-acds-owner-" + encoding + ".dxf");
    const std::filesystem::path output2 = directory
        / ("libdxfrw-ac1027-acds-owner-second-" + encoding + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);
    std::filesystem::remove(output2, ec);

    dx_data source;
    auto* modeler = new DRW_ModelerGeometry(DRW::E3DSOLID);
    modeler->handle = 0xFC20u;
    source.mBlock->ent.push_back(modeler);
    auto* line = new DRW_Line();
    line->handle = 0xFC21u;
    line->basePoint = DRW_Coord(1.0, 2.0, 3.0);
    line->secPoint = DRW_Coord(4.0, 5.0, 6.0);
    source.mBlock->ent.push_back(line);
    source.rawDxfSections.push_back(makeLocalAcdsDataSection("FC20"));

    dx_iface exporter;
    std::string originalLinkedHandle;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary,
                             &source, false)
        || !findAcdsRecordOwner(source.rawDxfSections.front(), "ASM_Data",
                                originalLinkedHandle)
        || originalLinkedHandle != "FC20") {
        if (!keepOutputs) {
            std::filesystem::remove(output, ec);
            std::filesystem::remove(output2, ec);
        }
        return false;
    }

    dx_data imported;
    dx_iface importer;
    if (!importer.fileImport(output.string(), &imported, false)) {
        if (!keepOutputs) {
            std::filesystem::remove(output, ec);
            std::filesystem::remove(output2, ec);
        }
        return false;
    }
    const DRW_ModelerGeometry* importedModeler = nullptr;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::E3DSOLID) {
            importedModeler =
                static_cast<const DRW_ModelerGeometry*>(entity);
            break;
        }
    }
    std::string linkedHandle;
    std::string opaqueHandle;
    const bool firstPassValid = importedModeler != nullptr
        && importedModeler->handle != 0xFC20u
        && !imported.rawDxfSections.empty()
        && findAcdsRecordOwner(imported.rawDxfSections.front(), "ASM_Data",
                               linkedHandle)
        && findAcdsRecordOwner(imported.rawDxfSections.front(), "OpaqueRecord",
                               opaqueHandle)
        && std::stoul(linkedHandle, nullptr, 16) == importedModeler->handle
        && opaqueHandle == "FC21";
    if (!firstPassValid) {
        if (!keepOutputs) {
            std::filesystem::remove(output, ec);
            std::filesystem::remove(output2, ec);
        }
        return false;
    }

    dx_iface secondExporter;
    dx_data secondImported;
    dx_iface secondImporter;
    const bool secondPassValid =
        secondExporter.fileExport(output2.string(), DRW::AC1027, binary,
                                  &imported, false)
        && secondImporter.fileImport(output2.string(), &secondImported,
                                     false);
    const DRW_ModelerGeometry* secondModeler = nullptr;
    if (secondPassValid) {
        for (const DRW_Entity* entity : secondImported.mBlock->ent) {
            if (entity != nullptr && entity->eType == DRW::E3DSOLID) {
                secondModeler =
                    static_cast<const DRW_ModelerGeometry*>(entity);
                break;
            }
        }
    }
    linkedHandle.clear();
    opaqueHandle.clear();
    const bool result = secondModeler != nullptr
        && !secondImported.rawDxfSections.empty()
        && findAcdsRecordOwner(secondImported.rawDxfSections.front(),
                               "ASM_Data", linkedHandle)
        && findAcdsRecordOwner(secondImported.rawDxfSections.front(),
                               "OpaqueRecord", opaqueHandle)
        && std::stoul(linkedHandle, nullptr, 16) == secondModeler->handle
        && opaqueHandle == "FC21";
    if (!keepOutputs) {
        std::filesystem::remove(output, ec);
        std::filesystem::remove(output2, ec);
    }
    return result;
}

bool runDxfAcdsModelerOwnerRejectsMalformed(
    bool includeOwnerHandle, bool duplicateOwnerHandle,
    const std::string& ownerHandle,
    bool duplicateModelerSourceHandle = false) {
    const std::filesystem::path output =
        std::filesystem::temp_directory_path()
        / "libdxfrw-acds-owner-malformed.dxf";
    std::error_code ec;
    std::filesystem::remove(output, ec);
    dx_data source;
    auto* modeler = new DRW_ModelerGeometry(DRW::E3DSOLID);
    modeler->handle = 0xFC20u;
    source.mBlock->ent.push_back(modeler);
    if (duplicateModelerSourceHandle) {
        auto* duplicate = new DRW_ModelerGeometry(DRW::REGION);
        duplicate->handle = 0xFC20u;
        source.mBlock->ent.push_back(duplicate);
    }
    source.rawDxfSections.push_back(makeLocalAcdsDataSection(
        ownerHandle, includeOwnerHandle, duplicateOwnerHandle));
    dx_iface exporter;
    const bool exported = exporter.fileExport(output.string(), DRW::AC1027,
                                               false, &source, false);
    const bool outputAbsent = !std::filesystem::exists(output);
    std::filesystem::remove(output, ec);
    return !exported && outputAbsent;
}

bool runDxfMalformedModelerCarrier(const char* chunk) {
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / "libdxfrw-modeler-malformed.dxf";
    std::error_code ec;
    std::filesystem::remove(output, ec);
    std::ofstream stream(output);
    stream << "0\nSECTION\n2\nENTITIES\n"
              "0\n3DSOLID\n5\n4A\n330\n1F\n100\nAcDbEntity\n"
              "100\nAcDbModelerGeometry\n100\nAcDb3dSolid\n70\n1\n"
           << "310\n" << chunk << "\n"
              "0\nENDSEC\n0\nEOF\n";
    stream.close();
    dx_data imported;
    dx_iface importer;
    const bool importOk = importer.fileImport(output.string(), &imported, false);
    std::filesystem::remove(output, ec);
    return !importOk && imported.mBlock->ent.empty();
}

bool runDxfModelerRejectsUnassociatedSab(
    const std::vector<std::uint8_t>& payload) {
    const std::filesystem::path output = std::filesystem::temp_directory_path()
        / "libdxfrw-modeler-unassociated-sab.dxf";
    std::error_code ec;
    std::filesystem::remove(output, ec);
    dx_data source;
    auto* modeler = new DRW_ModelerGeometry(DRW::E3DSOLID);
    modeler->handle = 0xFC01u;
    modeler->m_rawBytes = payload;
    source.mBlock->ent.push_back(modeler);
    dx_iface exporter;
    const bool exportOk = exporter.fileExport(output.string(), DRW::AC1027,
                                               false, &source, false);
    std::filesystem::remove(output, ec);
    return !exportOk;
}

bool runDxfModelerRejectsDwgFramePayload() {
    const std::filesystem::path output = std::filesystem::temp_directory_path()
        / "libdxfrw-modeler-dwg-frame.dxf";
    std::error_code ec;
    std::filesystem::remove(output, ec);
    dx_data source;
    auto* modeler = new DRW_ModelerGeometry(DRW::E3DSOLID);
    modeler->handle = 0xFC02u;
    modeler->m_rawBytes = {'A', 'B', 'C'};
    modeler->m_bodyBitSize = 24;
    modeler->m_objectSize = 3;
    source.mBlock->ent.push_back(modeler);
    dx_iface exporter;
    const bool exportOk = exporter.fileExport(output.string(), DRW::AC1024,
                                               false, &source, false);
    std::filesystem::remove(output, ec);
    if (exportOk)
        return false;

    dx_data extractedSource;
    auto* extracted = new DRW_ModelerGeometry(DRW::E3DSOLID);
    extracted->handle = 0xFC03u;
    extracted->m_dwgAcisPayload = {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e'};
    extractedSource.mBlock->ent.push_back(extracted);
    dx_iface extractedExporter;
    const bool extractedExportOk = extractedExporter.fileExport(
        output.string(), DRW::AC1024, false, &extractedSource, false);
    std::filesystem::remove(output, ec);
    return !extractedExportOk;
}

bool runDxfSurfaceSatCarrierRoundTrip() {
    const std::filesystem::path output = std::filesystem::temp_directory_path()
        / "libdxfrw-surface-sat-carrier.dxf";
    std::error_code ec;
    std::filesystem::remove(output, ec);
    const std::vector<std::uint8_t> payload{
        'A', 'C', 'I', 'S', ' ', 'S', 'A', 'T'};
    dx_data source;
    auto* surface = new DRW_PlaneSurface();
    surface->handle = 0xFB10u;
    surface->rawAcisData = payload;
    source.mBlock->ent.push_back(surface);
    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1024, false,
                             &source, false)) {
        std::filesystem::remove(output, ec);
        return false;
    }
    dx_data imported;
    dx_iface importer;
    if (!importer.fileImport(output.string(), &imported, false)) {
        std::filesystem::remove(output, ec);
        return false;
    }
    std::filesystem::remove(output, ec);
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr || entity->eType != DRW::PLANESURFACE)
            continue;
        const auto* decoded = static_cast<const DRW_PlaneSurface*>(entity);
        return decoded->rawAcisData == payload
            && !decoded->dxfPayloadChunks.empty()
            && std::all_of(decoded->dxfPayloadChunks.begin(),
                           decoded->dxfPayloadChunks.end(),
                           [](const DRW_ModelerPayloadChunk& chunk) {
                               return chunk.m_groupCode == 1 || chunk.m_groupCode == 3;
                           });
    }
    return false;
}

bool runDxfSurfaceRejectsUnassociatedSab(
    const std::vector<std::uint8_t>& payload) {
    const std::filesystem::path output = std::filesystem::temp_directory_path()
        / "libdxfrw-surface-unassociated-sab.dxf";
    std::error_code ec;
    std::filesystem::remove(output, ec);
    dx_data source;
    auto* surface = new DRW_PlaneSurface();
    surface->handle = 0xFB11u;
    surface->rawAcisData = payload;
    source.mBlock->ent.push_back(surface);
    dx_iface exporter;
    const bool exportOk = exporter.fileExport(output.string(), DRW::AC1027,
                                               false, &source, false);
    std::filesystem::remove(output, ec);
    return !exportOk;
}

bool runDxfMixedModelerCarrierChunks() {
    const std::filesystem::path output = std::filesystem::temp_directory_path()
        / "libdxfrw-modeler-mixed-carrier.dxf";
    std::error_code ec;
    std::filesystem::remove(output, ec);
    std::ofstream stream(output);
    stream << "0\nSECTION\n2\nENTITIES\n"
              "0\n3DSOLID\n5\n4A\n330\n1F\n100\nAcDbEntity\n"
              "100\nAcDbModelerGeometry\n100\nAcDb3dSolid\n70\n1\n"
              "1\nabc\n310\n4142\n"
              "0\nENDSEC\n0\nEOF\n";
    stream.close();

    dx_data imported;
    dx_iface importer;
    const bool importOk = importer.fileImport(output.string(), &imported, false);
    std::filesystem::remove(output, ec);
    if (!importOk)
        return false;

    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr || entity->eType != DRW::E3DSOLID)
            continue;
        const auto* decoded = static_cast<const DRW_ModelerGeometry*>(entity);
        const bool chunksPreserved =
            decoded->m_rawBytes == std::vector<std::uint8_t>{'a', 'b', 'c', 0x41, 0x42}
            && decoded->m_dxfPayloadChunks.size() == 2
            && decoded->m_dxfPayloadChunks[0].m_groupCode == 1
            && decoded->m_dxfPayloadChunks[0].m_offset == 0
            && decoded->m_dxfPayloadChunks[0].m_length == 3
            && decoded->m_dxfPayloadChunks[1].m_groupCode == 310
            && decoded->m_dxfPayloadChunks[1].m_offset == 3
            && decoded->m_dxfPayloadChunks[1].m_length == 2;
        if (!chunksPreserved)
            return false;
        const std::filesystem::path rejectedOutput =
            std::filesystem::temp_directory_path()
            / "libdxfrw-modeler-mixed-carrier-reject.dxf";
        std::filesystem::remove(rejectedOutput, ec);
        dx_iface exporter;
        const bool exportOk = exporter.fileExport(rejectedOutput.string(),
            DRW::AC1024, false, &imported, false);
        std::filesystem::remove(rejectedOutput, ec);
        return !exportOk;
    }
    return false;
}

class LocalModelerGeometry final : public DRW_ModelerGeometry {
public:
    using DRW_ModelerGeometry::DRW_ModelerGeometry;
    using DRW_ModelerGeometry::parseDwg;
    using DRW_Entity::encodeDwgCommon;
    using DRW_Entity::encodeDwgEntHandle;

    void setDwgType(std::uint16_t type) { oType = type; }
};

bool patchModelerObjectSize(std::vector<std::uint8_t>& bytes,
                            std::uint64_t sizeFieldBit,
                            std::uint32_t objectSizeBits) {
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(bytes.size()) * 8u;
    if (sizeFieldBit > totalBits || totalBits - sizeFieldBit < 32u)
        return false;

    // RL fields use little-endian bytes whose bits are stored most-significant
    // bit first, including when the field begins at a non-byte boundary.
    for (std::uint32_t byteIndex = 0; byteIndex < 4; ++byteIndex) {
        const std::uint8_t rawByte = static_cast<std::uint8_t>(
            objectSizeBits >> (byteIndex * 8u));
        for (std::uint32_t bitIndex = 0; bitIndex < 8; ++bitIndex) {
            const std::uint64_t destinationBit = sizeFieldBit
                + byteIndex * 8u + bitIndex;
            const std::uint8_t mask = static_cast<std::uint8_t>(
                1u << (7u - static_cast<unsigned>(destinationBit & 7u)));
            const std::uint8_t value = static_cast<std::uint8_t>(
                (rawByte >> (7u - bitIndex)) & 1u);
            std::uint8_t& destination =
                bytes[static_cast<std::size_t>(destinationBit >> 3)];
            destination = static_cast<std::uint8_t>(
                (destination & static_cast<std::uint8_t>(~mask))
                | (value != 0 ? mask : 0));
        }
    }
    return true;
}

bool buildModelerVersionFrame(bool empty, std::uint16_t modelerVersion,
                              std::vector<std::uint8_t>& bytes) {
    constexpr DRW::Version version = DRW::AC1018;
    constexpr std::uint16_t modelerObjectType = 38; // §20.4.41 3DSOLID
    LocalModelerGeometry source(DRW::E3DSOLID);
    source.setDwgType(modelerObjectType);
    source.handle = 0x120u;

    dwgBufferW frame;
    if (!source.encodeDwgCommon(version, &frame))
        return false;

    frame.putBit(empty ? 1u : 0u);
    frame.putBit(0u); // unknown bit
    if (!empty) {
        frame.putBitShort(modelerVersion);
        if (modelerVersion == 1)
            frame.putBitLong(0); // zero-length SAT block terminator
        else if (modelerVersion == 2)
            frame.putRawChar8(0xA5u); // bounded opaque test payload byte
    }
    if (!frame.isGood() || frame.bitCount() >
            std::numeric_limits<std::uint32_t>::max())
        return false;
    const std::uint32_t objectSizeBits = frame.bitCount();

    dwgBuffer typeReader(frame.data().data(), frame.data().size());
    if (typeReader.getObjType(version) != modelerObjectType)
        return false;
    const std::uint64_t sizeFieldBit = typeReader.getPosition() * 8u
        + typeReader.getBitPos();

    if (!source.encodeDwgEntHandle(version, &frame)
        || !frame.isGood())
        return false;
    bytes = frame.data();
    return patchModelerObjectSize(bytes, sizeFieldBit, objectSizeBits);
}

bool buildModelerSatV1Frame(
    DRW::Version version,
    const std::vector<std::vector<std::uint8_t>>& blocks,
    const std::vector<std::uint32_t>& declaredSizes,
    bool includeTerminator, std::vector<std::uint8_t>& bytes) {
    constexpr std::uint16_t modelerObjectType = 38; // §20.4.41 3DSOLID
    if (blocks.size() != declaredSizes.size())
        return false;
    LocalModelerGeometry source(DRW::E3DSOLID);
    source.setDwgType(modelerObjectType);
    source.handle = 0x120u;

    dwgBufferW frame;
    if (!source.encodeDwgCommon(version, &frame))
        return false;
    frame.putBit(0); // non-empty ACIS body
    frame.putBit(1); // opaque ODA “unknown” bit, also present in Cone.dwg
    frame.putBitShort(1);
    for (std::size_t block = 0; block < blocks.size(); ++block) {
        if (declaredSizes[block] >
            static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
            return false;
        frame.putBitLong(static_cast<std::int32_t>(declaredSizes[block]));
        for (std::uint8_t value : blocks[block]) {
            // ODA §20.4.41 SAT v1 character transform: preserve values at or
            // below space; printable characters above space use 0x9F - byte.
            const std::uint8_t encoded = value > 0x20u && value <= 0x7Eu
                ? static_cast<std::uint8_t>(0x9Fu - value) : value;
            frame.putRawChar8(encoded);
        }
    }
    if (includeTerminator)
        frame.putBitLong(0);
    frame.putBit(0); // no DWG wireframe block in this generated control
    frame.putBit(1); // empty wireframe-side ACIS body
    if (!frame.isGood() || frame.bitCount() >
            std::numeric_limits<std::uint32_t>::max())
        return false;
    const std::uint32_t objectSizeBits = frame.bitCount();

    dwgBuffer typeReader(frame.data().data(), frame.data().size());
    if (typeReader.getObjType(version) != modelerObjectType)
        return false;
    const std::uint64_t sizeFieldBit = typeReader.getPosition() * 8u
        + typeReader.getBitPos();
    if (!source.encodeDwgEntHandle(version, &frame) || !frame.isGood())
        return false;
    bytes = frame.data();
    return patchModelerObjectSize(bytes, sizeFieldBit, objectSizeBits);
}

bool runModelerDwgSatV1Extraction() {
    const auto failStage = [](const char *stage) {
        std::cerr << "SAT v1 extraction check failed: " << stage << '\n';
        return false;
    };
    std::vector<std::uint8_t> satText{
        '4', '0', '0', ' ', '2', '7', ' ', '1', ' ', '0', '\n',
        'A', 'C', 'I', 'S', ' ', 'S', 'A', 'T', '\t', 'C', 'O', 'N', 'T',
        'R', 'O', 'L', ' ', '^', 'X'};
    satText.insert(satText.end(), 280u, static_cast<std::uint8_t>('Z'));
    satText.push_back('\n');
    const std::vector<std::vector<std::uint8_t>> blocks{
        std::vector<std::uint8_t>(satText.begin(), satText.begin() + 11),
        std::vector<std::uint8_t>(satText.begin() + 11, satText.end())};
    const std::vector<std::uint32_t> declaredSizes{
        static_cast<std::uint32_t>(blocks[0].size()),
        static_cast<std::uint32_t>(blocks[1].size())};
    std::vector<std::uint8_t> frame;
    if (!buildModelerSatV1Frame(DRW::AC1015, blocks, declaredSizes, true,
                                frame))
        return failStage("build AC1015 frame");

    dwgBuffer buffer(frame.data(), frame.size());
    LocalModelerGeometry parsed(DRW::E3DSOLID);
    if (!parsed.parseDwg(DRW::AC1015, &buffer) || !buffer.isGood()
        || parsed.handle != 0x120u || parsed.m_modelerVersion != 1
        || !parsed.m_modelerDataUnknownBit
        || parsed.m_dwgSourceVersion != DRW::AC1015
        || parsed.m_dwgAcisPayload.size() != satText.size()
        || parsed.m_payloadRanges.size() != 2)
        return failStage("parse/qualify AC1015 SAT blocks");
    if (parsed.m_objectSize != frame.size())
        return failStage("complete DWG frame boundary");
    // dwgReader attaches this complete frame after entity parsing so the
    // adapter can preserve it for raw replay while the DXF writer uses only
    // the separately-qualified SAT payload.
    parsed.m_rawBytes = frame;
    std::vector<std::uint8_t> expectedPayload = satText;
    for (std::uint8_t& value : expectedPayload) {
        if (value == '\t')
            value = ' ';
    }
    if (parsed.m_dwgAcisPayload != expectedPayload)
        return failStage("de-obfuscated payload bytes");
    std::vector<std::uint8_t> expectedDxfPayload = expectedPayload;
    for (std::uint8_t& value : expectedDxfPayload) {
        if (value > 0x20u && value <= 0x7Eu)
            value = static_cast<std::uint8_t>(0x9Fu - value);
    }
    std::size_t rangedBytes = 0;
    for (const DRW_ModelerPayloadRange& range : parsed.m_payloadRanges) {
        if (range.m_kind != DRW_ModelerPayloadRange::Kind::Sat
            || range.m_section != DRW_ModelerPayloadRange::Section::Body
            || range.m_consistency !=
                DRW_ModelerPayloadRange::Consistency::Exact
            || range.m_confidence !=
                DRW_ModelerPayloadRange::Confidence::DeclaredSize
            || range.m_declaredByteSize != range.m_length)
            return failStage("payload range semantics");
        rangedBytes += range.m_length;
    }
    if (rangedBytes != expectedPayload.size() || parsed.decodeWireframe()
        || !parsed.m_wireframe.empty())
        return failStage("range coverage or SAT/SAB decoder separation");

    // The newly extracted SAT1 payload is emitted only in the exact source
    // revision's text-carrier lane, then re-read through the public DXF path.
    const std::filesystem::path output = std::filesystem::temp_directory_path()
        / "libdxfrw-modeler-ac1015-satv1.dxf";
    const std::filesystem::path rejectedOutput =
        std::filesystem::temp_directory_path()
        / "libdxfrw-modeler-ac1015-satv1-wrong-source.dxf";
    const std::filesystem::path reexportOutput =
        std::filesystem::temp_directory_path()
        / "libdxfrw-modeler-ac1015-satv1-reexport.dxf";
    std::error_code ec;
    std::filesystem::remove(output, ec);
    std::filesystem::remove(rejectedOutput, ec);
    std::filesystem::remove(reexportOutput, ec);
    dx_data source;
    source.mBlock->ent.push_back(new DRW_ModelerGeometry(parsed));
    dx_iface exporter;
    const bool exportOk = exporter.fileExport(output.string(), DRW::AC1015,
                                               false, &source, false);
    if (!exportOk) {
        std::filesystem::remove(output, ec);
        return failStage("export AC1015 SAT text carrier");
    }
    dx_data imported;
    dx_iface importer;
    const bool importOk = importer.fileImport(output.string(), &imported, false);
    bool found = false;
    if (importOk) {
        for (const DRW_Entity* entity : imported.mBlock->ent) {
            if (entity == nullptr || entity->eType != DRW::E3DSOLID)
                continue;
            const auto* decoded =
                static_cast<const DRW_ModelerGeometry*>(entity);
            std::size_t textBytes = 0;
            bool validChunks = !decoded->m_dxfPayloadChunks.empty();
            for (const DRW_ModelerPayloadChunk& chunk :
                 decoded->m_dxfPayloadChunks) {
                if (chunk.m_groupCode == 1 && textBytes != 0) {
                    validChunks = validChunks
                        && textBytes < decoded->m_rawBytes.size()
                        && decoded->m_rawBytes[textBytes] == '\n';
                    ++textBytes;
                }
                validChunks = validChunks
                    && (chunk.m_groupCode == 1 || chunk.m_groupCode == 3)
                    && chunk.m_offset == textBytes
                    && chunk.m_offset <= decoded->m_rawBytes.size()
                    && chunk.m_length
                        <= decoded->m_rawBytes.size() - chunk.m_offset;
                textBytes += chunk.m_length;
            }
            found = validChunks && textBytes + 1 == expectedDxfPayload.size()
                && decoded->m_rawBytes.back() == '\n'
                && decoded->m_rawBytes == expectedDxfPayload
                && decoded->m_modelerVersion == 1
                && decoded->m_dwgAcisPayload.empty()
                && decoded->m_dwgSourceVersion == DRW::UNKNOWNV;
            break;
        }
    }
    if (!found) {
        std::cerr << "SAT v1 readback: import=" << importOk
                  << " entities=" << imported.mBlock->ent.size()
                  << " file=" << output.string() << '\n';
        return failStage("DXF public readback semantics");
    }

    // Re-exporting an imported DXF must preserve group-1 SAT line starts,
    // group-3 continuations, and the caret escape rather than flattening them.
    dx_iface reExporter;
    if (!reExporter.fileExport(reexportOutput.string(), DRW::AC1015, false,
                               &imported, false))
        return failStage("re-export grouped SAT DXF carrier");
    dx_data reimported;
    dx_iface reimporter;
    if (!reimporter.fileImport(reexportOutput.string(), &reimported, false))
        return failStage("re-import grouped SAT DXF carrier");
    bool reimportedPayloadMatches = false;
    for (const DRW_Entity* entity : reimported.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::E3DSOLID) {
            const auto* decoded =
                static_cast<const DRW_ModelerGeometry*>(entity);
            reimportedPayloadMatches = decoded->m_rawBytes == expectedDxfPayload
                && decoded->m_dxfPayloadChunks.size() == 3
                && decoded->m_dxfPayloadChunks[0].m_groupCode == 1
                && decoded->m_dxfPayloadChunks[1].m_groupCode == 1
                && decoded->m_dxfPayloadChunks[2].m_groupCode == 3;
            break;
        }
    }
    if (!reimportedPayloadMatches)
        return failStage("re-export/readback SAT text semantics");
    std::filesystem::remove(output, ec);
    std::filesystem::remove(reexportOutput, ec);

    DRW_ModelerGeometry mismatched(parsed);
    mismatched.m_dwgSourceVersion = DRW::AC1018;
    dx_data rejectedSource;
    rejectedSource.mBlock->ent.push_back(
        new DRW_ModelerGeometry(mismatched));
    dx_iface rejectedExporter;
    const bool rejected = !rejectedExporter.fileExport(
        rejectedOutput.string(), DRW::AC1015, false, &rejectedSource, false);
    const bool noPartialOutput = !std::filesystem::exists(rejectedOutput);
    std::filesystem::remove(rejectedOutput, ec);
    if (!rejected || !noPartialOutput)
        return failStage("reject source-version mismatch atomically");

    // An incomplete SAT1 block sequence remains a successfully parsed typed
    // entity but is not promoted to a qualified payload or writable DXF.
    if (!buildModelerSatV1Frame(DRW::AC1015, blocks, declaredSizes, false,
                                frame))
        return failStage("build unterminated AC1015 frame");
    dwgBuffer incompleteBuffer(frame.data(), frame.size());
    LocalModelerGeometry incomplete(DRW::E3DSOLID);
    if (!incomplete.parseDwg(DRW::AC1015, &incompleteBuffer)
        || !incompleteBuffer.isGood()
        || !incomplete.m_dwgAcisPayload.empty()
        || !incomplete.m_payloadRanges.empty()
        || incomplete.m_dwgSourceVersion != DRW::UNKNOWNV)
        return failStage("keep unterminated carrier opaque");

    // The same byte sequence is deliberately not interpreted using another
    // revision's layout, even if the text resembles a valid SAT carrier.
    if (!buildModelerSatV1Frame(DRW::AC1018, blocks, declaredSizes, true,
                                frame))
        return failStage("build AC1018 control frame");
    dwgBuffer otherVersionBuffer(frame.data(), frame.size());
    LocalModelerGeometry otherVersion(DRW::E3DSOLID);
    const bool opaqueOtherVersion =
        otherVersion.parseDwg(DRW::AC1018, &otherVersionBuffer)
        && otherVersionBuffer.isGood()
        && otherVersion.m_dwgAcisPayload.empty()
        && otherVersion.m_payloadRanges.empty()
        && otherVersion.m_dwgSourceVersion == DRW::UNKNOWNV;
    return opaqueOtherVersion ? true : failStage("keep AC1018 carrier opaque");
}

bool buildModelerInlineSabFrame(DRW::Version version,
                                const std::vector<std::uint8_t>& payload,
                                std::vector<std::uint8_t>& bytes,
                                std::uint32_t& handleBitSize) {
    constexpr std::uint16_t modelerObjectType = 38; // §20.4.41 3DSOLID
    LocalModelerGeometry source(DRW::E3DSOLID);
    source.setDwgType(modelerObjectType);
    source.handle = 0x120u;

    dwgBufferW body;
    dwgBufferW handles;
    if (!source.encodeDwgCommon(version, &body))
        return false;
    body.putBit(0); // non-empty ACIS body
    body.putBit(0); // modeler data unknown bit
    body.putBitShort(2);
    body.putBytes(payload.data(), payload.size());
    body.putBit(0); // empty R2007+ string stream
    body.alignToByte();
    if (!body.isGood())
        return false;

    if (version == DRW::AC1021) {
        if (body.bitCount() > std::numeric_limits<std::uint32_t>::max())
            return false;
        const std::uint32_t objectSizeBits =
            static_cast<std::uint32_t>(body.bitCount());
        dwgBuffer typeReader(body.data().data(), body.data().size());
        if (typeReader.getObjType(version) != modelerObjectType)
            return false;
        const std::uint64_t sizeFieldBit = typeReader.getPosition() * 8u
            + typeReader.getBitPos();
        // R2007 carries the object-data bit size in its common prefix and
        // appends handles in the same object body (R2010 splits them out).
        if (!source.encodeDwgEntHandle(version, &body)
            || !body.isGood())
            return false;
        bytes = body.data();
        handleBitSize = 0;
        return patchModelerObjectSize(bytes, sizeFieldBit, objectSizeBits);
    }

    if (version != DRW::AC1024
        || !source.encodeDwgEntHandle(version, &body, &handles)
        || !body.isGood() || !handles.isGood()
        || body.size() > std::numeric_limits<std::uint32_t>::max() / 8u
        || handles.size() > std::numeric_limits<std::uint32_t>::max() / 8u)
        return false;

    handleBitSize = static_cast<std::uint32_t>(handles.size() * 8u);
    bytes = body.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return true;
}

bool buildModelerExternalSabFrame(std::uint16_t entityModelerVersion,
                                  bool hasDataStorage,
                                  std::vector<std::uint8_t>& bytes,
                                  std::uint32_t& handleBitSize) {
    constexpr DRW::Version version = DRW::AC1027;
    constexpr std::uint16_t modelerObjectType = 38; // §20.4.41 3DSOLID
    LocalModelerGeometry source(DRW::E3DSOLID);
    source.setDwgType(modelerObjectType);
    source.handle = 0x120u;
    source.setHasDataStorageBinaryData(hasDataStorage);

    dwgBufferW body;
    dwgBufferW handles;
    if (!source.encodeDwgCommon(version, &body))
        return false;
    body.putBit(0); // nonempty ACIS body
    body.putBit(1); // modeler-data unknown bit
    body.putBitShort(entityModelerVersion);
    body.putBit(0); // empty R2007+ string stream
    body.alignToByte();
    if (!body.isGood() || !source.encodeDwgEntHandle(version, &body, &handles)
        || !body.isGood() || !handles.isGood()
        || body.size() > std::numeric_limits<std::uint32_t>::max() / 8u
        || handles.size() > std::numeric_limits<std::uint32_t>::max() / 8u)
        return false;

    handleBitSize = static_cast<std::uint32_t>(handles.size() * 8u);
    bytes = body.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return true;
}

bool parseModelerVersionFrame(bool empty, std::uint16_t modelerVersion,
                              bool expectedSuccess) {
    std::vector<std::uint8_t> bytes;
    if (!buildModelerVersionFrame(empty, modelerVersion, bytes))
        return false;

    dwgBuffer buffer(bytes.data(), bytes.size());
    LocalModelerGeometry parsed(DRW::E3DSOLID);
    const bool parseSucceeded = parsed.parseDwg(DRW::AC1018, &buffer);
    if (parseSucceeded != expectedSuccess)
        return false;
    if (!expectedSuccess) {
        return !buffer.isGood() && parsed.handle == DRW::NoHandle
            && parsed.m_modelerVersion == 0 && parsed.m_rawBytes.empty()
            && parsed.m_dwgAcisPayload.empty()
            && parsed.m_payloadRanges.empty();
    }
    if (parsed.m_isEmpty != empty || parsed.m_hasModelerData == empty)
        return false;
    return parsed.m_modelerVersion == (empty ? 0 : modelerVersion)
        && parsed.handle == 0x120u && parsed.m_dwgAcisPayload.empty()
        && parsed.m_payloadRanges.empty();
}

bool runModelerDwgVersionValidation() {
    if (!(parseModelerVersionFrame(true, 0, true)
        && parseModelerVersionFrame(false, 1, true)
        && parseModelerVersionFrame(false, 2, true)
        && parseModelerVersionFrame(false, 0, false)
        && parseModelerVersionFrame(false, 3, false)))
        return false;

    // A 2013+ entity advertises an external AcDs/SAB carrier. Its entity-local
    // BS must remain opaque rather than being mistaken for the SAB version;
    // this mirrors the independently observed AC1027 Cover.dwg decode where
    // the field reads as 168 and the linked AcDs record supplies SAB v2.
    std::vector<std::uint8_t> externalFrame;
    std::uint32_t externalHandleBits = 0;
    if (!buildModelerExternalSabFrame(168, true, externalFrame,
                                      externalHandleBits))
        return false;
    dwgBuffer externalBuffer(externalFrame.data(), externalFrame.size());
    LocalModelerGeometry external(DRW::E3DSOLID);
    if (!external.parseDwg(DRW::AC1027, &externalBuffer,
                           externalHandleBits)
        || !externalBuffer.isGood() || external.handle != 0x120u
        || !external.hasDataStorageBinaryData() || external.m_isEmpty
        || !external.m_hasModelerData || external.m_modelerVersion != 168
        || !external.m_dwgAcisPayload.empty()
        || !external.m_payloadRanges.empty()) {
        std::cerr << "modeler AC1027 external-carrier parse/check failed: good="
                  << externalBuffer.isGood()
                  << " handle=" << external.handle
                  << " hasDsData=" << external.hasDataStorageBinaryData()
                  << " version=" << external.m_modelerVersion << '\n';
        return false;
    }

    std::vector<std::uint8_t> unadvertisedFrame;
    std::uint32_t unadvertisedHandleBits = 0;
    if (!buildModelerExternalSabFrame(168, false, unadvertisedFrame,
                                      unadvertisedHandleBits))
        return false;
    dwgBuffer unadvertisedBuffer(unadvertisedFrame.data(),
                                 unadvertisedFrame.size());
    LocalModelerGeometry unadvertised(DRW::E3DSOLID);
    if (unadvertised.parseDwg(DRW::AC1027, &unadvertisedBuffer,
                              unadvertisedHandleBits)
        || unadvertisedBuffer.isGood()) {
        std::cerr << "modeler AC1027 unadvertised external version was accepted\n";
        return false;
    }

    static constexpr std::uint8_t marker[] = {
        0x0E, 0x03, 'E', 'n', 'd', 0x0E, 0x02, 'o', 'f',
        0x0E, 0x04, 'A', 'C', 'I', 'S', 0x0D, 0x04,
        'd', 'a', 't', 'a'};
    std::vector<std::uint8_t> sab = {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e',
        0xFC, 0x53, 0x00, 0x00};
    sab.insert(sab.end(), std::begin(marker), std::end(marker));
    const std::vector<std::uint8_t> exactSab = sab;
    sab.insert(sab.end(), {0xA5, 0x5A}); // wireframe/body bytes after ACIS

    const auto checkVersion = [&](DRW::Version version) {
        std::vector<std::uint8_t> frame;
        std::uint32_t handleBits = 0;
        if (!buildModelerInlineSabFrame(version, sab, frame, handleBits)) {
            std::cerr << "modeler inline SAB frame builder failed for version "
                      << static_cast<int>(version) << '\n';
            return false;
        }
        dwgBuffer buffer(frame.data(), frame.size());
        LocalModelerGeometry parsed(DRW::E3DSOLID);
        const bool parseSucceeded = parsed.parseDwg(version, &buffer, handleBits);
        if (!parseSucceeded || parsed.m_modelerVersion != 2
            || parsed.handle != 0x120u || parsed.m_dwgSourceVersion != version
            || parsed.m_dwgAcisPayload != exactSab
            || parsed.m_payloadRanges.size() != 1
            || parsed.m_payloadRanges.front().m_kind
                   != DRW_ModelerPayloadRange::Kind::Sab
            || parsed.m_payloadRanges.front().m_consistency
                   != DRW_ModelerPayloadRange::Consistency::Exact
            || parsed.m_payloadRanges.front().m_confidence
                   != DRW_ModelerPayloadRange::Confidence::Marker
            || parsed.m_payloadRanges.front().m_length != exactSab.size()
            || parsed.m_payloadRanges.front().m_bitOffset > 7) {
            std::cerr << "modeler inline SAB parse/check failed: version="
                      << static_cast<int>(version)
                      << " parse=" << parseSucceeded
                      << " modeler-version=" << parsed.m_modelerVersion
                      << " handle=" << parsed.handle
                      << " payload=" << parsed.m_dwgAcisPayload.size()
                      << " expected=" << exactSab.size()
                      << " ranges=" << parsed.m_payloadRanges.size()
                      << " good=" << buffer.isGood() << '\n';
            return false;
        }

        // A duplicate terminator makes the boundary ambiguous; absence also
        // remains a successful typed read with no promoted payload.
        std::vector<std::uint8_t> ambiguous = exactSab;
        ambiguous.insert(ambiguous.end(), std::begin(marker), std::end(marker));
        const auto rejectsExtraction = [version](
            const std::vector<std::uint8_t>& body) {
            std::vector<std::uint8_t> frameBytes;
            std::uint32_t frameHandleBits = 0;
            if (!buildModelerInlineSabFrame(version, body, frameBytes,
                                            frameHandleBits)) {
                std::cerr << "modeler negative frame builder failed for version "
                          << static_cast<int>(version) << '\n';
                return false;
            }
            dwgBuffer localBuffer(frameBytes.data(), frameBytes.size());
            LocalModelerGeometry local(DRW::E3DSOLID);
            const bool localParse = local.parseDwg(version, &localBuffer,
                                                   frameHandleBits);
            if (!localParse || !local.m_dwgAcisPayload.empty()
                || !local.m_payloadRanges.empty()) {
                std::cerr << "modeler negative parse failed: version="
                          << static_cast<int>(version)
                          << " parse=" << localParse
                          << " payload=" << local.m_dwgAcisPayload.size()
                          << " ranges=" << local.m_payloadRanges.size()
                          << '\n';
                return false;
            }
            // The dispatcher's opaque frame carrier must not be decoded as
            // SAB when no exact inline carrier was extracted.
            local.m_rawBytes = frameBytes;
            const bool decodeRejected = !local.decodeWireframe()
                && local.m_wireframe.empty();
            if (!decodeRejected)
                std::cerr << "modeler frame fallback was decoded\n";
            return decodeRejected;
        };
        return rejectsExtraction(ambiguous)
            && rejectsExtraction({0xFC, 0x53, 0x00, 0x00, 0xA5, 0x5A});
    };
    return checkVersion(DRW::AC1021) && checkVersion(DRW::AC1024);
}

dwgHandle localRawObjectHandle(std::uint8_t code, std::uint32_t ref) {
    dwgHandle handle;
    handle.code = ref == 0 ? 0 : code;
    handle.size = 0;
    handle.ref = ref;
    handle.ref64 = ref;
    return handle;
}

DRW_UnsupportedObject makeLocalRawReplayObject(
    DRW::Version version, std::uint16_t classNumber, std::uint32_t handle,
    std::uint32_t encodedHandle = 0) {
    if (encodedHandle == 0)
        encodedHandle = handle;

    dwgBufferW body;
    dwgBufferW handles;
    body.putObjType(version, classNumber);
    body.putHandle(localRawObjectHandle(4, encodedHandle));
    body.putBitShort(0); // EED size
    body.putBitLong(0);  // reactor count
    body.putBit(0);      // no extension dictionary
    body.putBit(0);      // no DataStorage payload
    body.putRawLong32(0x12345678u);
    body.putRawShort16(0xABCDu);
    body.putRawChar8(0x5Au);
    body.alignToByte();
    for (int i = 0; i < 7; ++i)
        body.putBit(0); // empty R2007+ string stream
    body.putRawShort16(0);
    body.putBit(0);
    body.alignToByte();

    handles.putHandle(localRawObjectHandle(4, 0)); // owner
    handles.putHandle(localRawObjectHandle(0, 0)); // xdictionary
    handles.alignToByte();

    DRW_UnsupportedObject object;
    object.m_version = version;
    object.m_objectType = classNumber;
    object.m_handle = handle;
    object.m_parentHandle = DRW::NoHandle;
    object.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::ValidatedAbsent);
    object.m_bodyBitSize = static_cast<std::uint32_t>(handles.size() * 8u);
    object.m_objectSize = static_cast<std::uint32_t>(
        body.data().size() + handles.data().size());
    object.m_isEntity = false;
    object.m_isCustomClass = true;
    object.m_recordName = "LOCAL_RAW_REPLAY";
    object.m_className = "AcDbLocalRawReplay";
    object.m_hasClassDefinition = true;
    object.m_classProxyFlag = 0x401;
    object.m_classAppName = "LOCAL_S110";
    object.m_classEntityFlagRaw = 0x1F3;
    object.m_rawBytes = body.data();
    object.m_rawBytes.insert(object.m_rawBytes.end(), handles.data().begin(),
                             handles.data().end());
    return object;
}

class LocalRawReplayInterface final : public dx_iface {
public:
    explicit LocalRawReplayInterface(dwgRW* writer = nullptr)
        : writer_(writer), first_(makeLocalRawReplayObject(
              DRW::AC1027, 500, 0x700u, 0x6FFu)),
          second_(makeLocalRawReplayObject(DRW::AC1027, 501, 0x702u)),
          third_(makeLocalRawReplayObject(DRW::AC1027, 500, 0x706u)) {
        first_.m_objectSize = static_cast<std::uint32_t>(first_.m_rawBytes.size());
        second_.m_objectSize = static_cast<std::uint32_t>(second_.m_rawBytes.size());
        third_.m_objectSize = static_cast<std::uint32_t>(third_.m_rawBytes.size());
        third_.m_recordName = "LOCAL_RAW_REPLAY_ALT";
        third_.m_className = "AcDbLocalRawReplayAlt";
        third_.m_classAppName = "LOCAL_S113";
        section_.m_name = "LocalRawS110";
        section_.m_version = DRW::AC1027;
        section_.m_data = {0x53u, 0x31u, 0x31u, 0x30u, 0x01u};
        cData = &data_;
        currentBlock = data_.mBlock;
    }

    void writeHeader(DRW_Header& data) override { data.vars.clear(); }

    void writeDwgClasses() override {
        if (writer_ == nullptr)
            return;
        registeredFirst_ = writer_->registerRawDwgObjectClass(&first_);
        registeredSecond_ = writer_->registerRawDwgObjectClass(&second_);
        registeredThird_ = writer_->registerRawDwgObjectClass(&third_);
        rejectedNullClass_ = !writer_->registerRawDwgObjectClass(nullptr);
    }

    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {
        if (writer_ == nullptr)
            return;
        replayedFirst_ = writer_->writeRawDwgObject(&first_);
        capturedFirstFrame_ = writer_->getLastDwgObjectFrame(firstFrame_);
        if (replayedFirst_)
            writeEvents_.push_back("O:" + std::to_string(first_.m_handle));

        DRW_UnsupportedObject malformed = second_;
        malformed.m_handle = 0x703u;
        malformed.m_objectType = 102;
        malformed.m_isCustomClass = false;
        malformed.m_className.clear();
        malformed.m_recordName.clear();
        malformed.m_rawBytes = {0x00u};
        malformed.m_objectSize = 1;
        malformed.m_bodyBitSize = 0;
        rejectedMalformed_ = !writer_->writeRawDwgObject(&malformed);
        rejectedNullObject_ = !writer_->writeRawDwgObject(nullptr);
        DRW_UnsupportedObject emptyObject = second_;
        emptyObject.m_handle = 0x705u;
        emptyObject.m_rawBytes.clear();
        emptyObject.m_objectSize = 0;
        rejectedEmptyObject_ = !writer_->writeRawDwgObject(&emptyObject);

        DRW_UnsupportedObject wrongVersion = second_;
        wrongVersion.m_handle = 0x704u;
        wrongVersion.m_version = DRW::AC1024;
        rejectedWrongVersion_ = !writer_->writeRawDwgObject(&wrongVersion);

        DRW_RawDwgSection wrongSection = section_;
        wrongSection.m_version = DRW::AC1024;
        rejectedWrongSectionVersion_ = !writer_->writeRawDwgSection(&wrongSection);
        DRW_RawDwgSection invalidEncoding = section_;
        invalidEncoding.m_name = "LocalRawS112Encoding";
        invalidEncoding.m_encoding = 3;
        rejectedInvalidEncoding_ = !writer_->writeRawDwgSection(&invalidEncoding);
        DRW_RawDwgSection encrypted = section_;
        encrypted.m_name = "LocalRawS112Encrypted";
        encrypted.m_encrypted = 1;
        rejectedEncrypted_ = !writer_->writeRawDwgSection(&encrypted);
        DRW_RawDwgSection oversized = section_;
        oversized.m_name = "LocalRawS112Oversized";
        oversized.m_maxSize = 0xFFFFFFFFu;
        rejectedOversized_ = !writer_->writeRawDwgSection(&oversized);
        DRW_RawDwgSection emptyName = section_;
        emptyName.m_name.clear();
        rejectedEmptySectionName_ = !writer_->writeRawDwgSection(&emptyName);

        replayedSecond_ = writer_->writeRawDwgObject(&second_);
        capturedSecondFrame_ = writer_->getLastDwgObjectFrame(secondFrame_);
        if (replayedSecond_)
            writeEvents_.push_back("O:" + std::to_string(second_.m_handle));
        DRW_UnsupportedObject duplicate = first_;
        rejectedDuplicateHandle_ = !writer_->writeRawDwgObject(&duplicate);
        replayedThird_ = writer_->writeRawDwgObject(&third_);
        capturedThirdFrame_ = writer_->getLastDwgObjectFrame(thirdFrame_);
        if (replayedThird_)
            writeEvents_.push_back("O:" + std::to_string(third_.m_handle));
        replayedSection_ = writer_->writeRawDwgSection(&section_);
        if (replayedSection_)
            writeEvents_.push_back("S:" + section_.m_name);
        rejectedDuplicateSection_ = !writer_->writeRawDwgSection(&section_);
    }
    void writeAppId() override {}

    void addUnsupportedObject(const DRW_UnsupportedObject& object) override {
        readObjects_.push_back(object);
        readEvents_.push_back("O:" + std::to_string(object.m_handle));
    }
    void addRawDwgSection(const DRW_RawDwgSection& section) override {
        readSections_.push_back(section);
        readEvents_.push_back("S:" + section.m_name);
    }

    dwgRW* writer_ {nullptr};
    DRW_UnsupportedObject first_;
    DRW_UnsupportedObject second_;
    DRW_UnsupportedObject third_;
    DRW_RawDwgSection section_;
    std::vector<DRW_UnsupportedObject> readObjects_;
    std::vector<DRW_RawDwgSection> readSections_;
    std::vector<std::string> writeEvents_;
    std::vector<std::string> readEvents_;
    DRW::DwgObjectFrameReceipt firstFrame_;
    DRW::DwgObjectFrameReceipt secondFrame_;
    DRW::DwgObjectFrameReceipt thirdFrame_;
    bool registeredFirst_ {false};
    bool registeredSecond_ {false};
    bool registeredThird_ {false};
    bool rejectedNullClass_ {false};
    bool replayedFirst_ {false};
    bool capturedFirstFrame_ {false};
    bool rejectedMalformed_ {false};
    bool rejectedNullObject_ {false};
    bool rejectedEmptyObject_ {false};
    bool rejectedWrongVersion_ {false};
    bool rejectedWrongSectionVersion_ {false};
    bool rejectedInvalidEncoding_ {false};
    bool rejectedEncrypted_ {false};
    bool rejectedOversized_ {false};
    bool rejectedEmptySectionName_ {false};
    bool replayedSecond_ {false};
    bool capturedSecondFrame_ {false};
    bool rejectedDuplicateHandle_ {false};
    bool replayedThird_ {false};
    bool capturedThirdFrame_ {false};
    bool replayedSection_ {false};
    bool rejectedDuplicateSection_ {false};
    dx_data data_;
};

bool runRawDwgReplayContract() {
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / "libdxfrw-s110-raw-replay.dwg";
    std::error_code ec;
    std::filesystem::remove(output, ec);

    dwgRW writer(output.string().c_str());
    LocalRawReplayInterface writeIface(&writer);
    const bool writeOk = writer.write(&writeIface, DRW::AC1027, true);
    const dwgRW::WriteSkipCounters skips = writer.getWriteSkipCounters();
    if (!writeOk
        || !writeIface.registeredFirst_ || !writeIface.registeredSecond_
        || !writeIface.registeredThird_
        || !writeIface.rejectedNullClass_ || !writeIface.replayedFirst_
        || !writeIface.rejectedMalformed_ || !writeIface.rejectedNullObject_
        || !writeIface.rejectedEmptyObject_ || !writeIface.rejectedWrongVersion_
        || !writeIface.rejectedWrongSectionVersion_
        || !writeIface.rejectedInvalidEncoding_ || !writeIface.rejectedEncrypted_
        || !writeIface.rejectedOversized_ || !writeIface.rejectedEmptySectionName_
        || !writeIface.replayedSecond_
        || !writeIface.rejectedDuplicateHandle_ || !writeIface.replayedThird_
        || !writeIface.replayedSection_ || !writeIface.rejectedDuplicateSection_) {
        std::filesystem::remove(output, ec);
        return false;
    }
    if (skips.rawObjectWrites < 5 || skips.rawSectionWrites < 6
        || skips.classRegistrations < 1) {
        std::filesystem::remove(output, ec);
        return false;
    }

    dwgRW reader(output.string().c_str());
    LocalRawReplayInterface readIface;
    const bool readOk = reader.read(&readIface, true);
    const auto findObject = [&readIface](std::uint32_t handle)
        -> const DRW_UnsupportedObject* {
        for (const DRW_UnsupportedObject& object : readIface.readObjects_) {
            if (object.m_handle == handle)
                return &object;
        }
        return nullptr;
    };
    const DRW_UnsupportedObject* first = findObject(0x700u);
    const DRW_UnsupportedObject* second = findObject(0x702u);
    const DRW_UnsupportedObject* third = findObject(0x706u);
    const std::vector<std::string> expectedWriteEvents {
        "O:1792", "O:1794", "O:1798", "S:LocalRawS110"};
    const std::vector<std::string> expectedReadEvents {
        "O:1794", "O:1792", "O:1798", "S:LocalRawS110"};
    const bool readContract = readOk && first != nullptr && second != nullptr
        && third != nullptr && readIface.readSections_.size() == 1
        && readIface.readSections_.front().m_name == "LocalRawS110"
        && readIface.readSections_.front().m_data == writeIface.section_.m_data
        && first->m_handle == 0x700u && second->m_handle == 0x702u
        && first->m_className == "AcDbLocalRawReplay"
        && second->m_className == "AcDbLocalRawReplay"
        && third->m_className == "AcDbLocalRawReplayAlt";
    const auto rawEvents = [](const std::vector<std::string>& events) {
        std::vector<std::string> result;
        for (const std::string& event : events) {
            if (event == "O:1792" || event == "O:1794"
                || event == "O:1798" || event == "S:LocalRawS110")
                result.push_back(event);
        }
        return result;
    };
    const bool eventContract = writeIface.writeEvents_ == expectedWriteEvents
        && rawEvents(readIface.readEvents_) == expectedReadEvents;
    bool rejectedMutation = false;
    std::ifstream encoded(output, std::ios::binary);
    std::vector<std::uint8_t> encodedBytes(
        (std::istreambuf_iterator<char>(encoded)),
        std::istreambuf_iterator<char>());
    constexpr std::array<std::uint8_t, 4> payloadMarker {
        0x78u, 0x56u, 0x34u, 0x12u};
    const auto marker = std::search(encodedBytes.begin(), encodedBytes.end(),
                                    payloadMarker.begin(), payloadMarker.end());
    bool validBufferRead = false;
    if (marker != encodedBytes.end()) {
        dwgRW bufferReader(output.string().c_str());
        LocalRawReplayInterface bufferIface;
        const auto hasBufferHandle = [&bufferIface](std::uint32_t handle) {
            return std::any_of(bufferIface.readObjects_.begin(),
                               bufferIface.readObjects_.end(),
                               [handle](const DRW_UnsupportedObject& object) {
                                   return object.m_handle == handle;
                               });
        };
        validBufferRead = bufferReader.readBuffer(
            encodedBytes.data(), encodedBytes.size(), &bufferIface, true)
            && hasBufferHandle(0x700u) && hasBufferHandle(0x702u)
            && hasBufferHandle(0x706u)
            && bufferIface.readSections_.size() == 1;
        *marker ^= 0x01u;
        dwgRW corruptedReader(output.string().c_str());
        LocalRawReplayInterface corruptedIface;
        const bool rejectedBufferMutation = !corruptedReader.readBuffer(
            encodedBytes.data(), encodedBytes.size(), &corruptedIface, true)
            && corruptedIface.readObjects_.empty();
        const DRW::error bufferMutationError = corruptedReader.getError();
        const DRW_OperationDiagnostic bufferDiagnostic =
            corruptedReader.getLastDiagnostic();
        const std::filesystem::path corruptedPath =
            std::filesystem::temp_directory_path()
            / "libdxfrw-s116-corrupted-replay.dwg";
        std::ofstream corruptedOutput(corruptedPath, std::ios::binary);
        corruptedOutput.write(
            reinterpret_cast<const char*>(encodedBytes.data()),
            static_cast<std::streamsize>(encodedBytes.size()));
        corruptedOutput.close();
        dwgRW corruptedFileReader(corruptedPath.string().c_str());
        LocalRawReplayInterface corruptedFileIface;
        const bool rejectedFileMutation = !corruptedFileReader.read(
            &corruptedFileIface, true) && corruptedFileIface.readObjects_.empty();
        const DRW::error fileMutationError = corruptedFileReader.getError();
        const DRW_OperationDiagnostic fileDiagnostic =
            corruptedFileReader.getLastDiagnostic();
        std::filesystem::remove(corruptedPath, ec);
        const bool equivalentDiagnostic =
            bufferDiagnostic.operation == fileDiagnostic.operation
            && bufferDiagnostic.phase == fileDiagnostic.phase
            && bufferDiagnostic.cause == fileDiagnostic.cause
            && bufferDiagnostic.code == fileDiagnostic.code
            && bufferDiagnostic.secondary.size() == fileDiagnostic.secondary.size();
        rejectedMutation = rejectedBufferMutation && rejectedFileMutation
            && bufferMutationError == fileMutationError && equivalentDiagnostic;
    }
    std::filesystem::remove(output, ec);
    const bool result = readContract && eventContract && validBufferRead
        && rejectedMutation
        && writeIface.capturedFirstFrame_
        && writeIface.firstFrame_.objectHandle == 0x700u
        && writeIface.firstFrame_.classNumber >= 500
        && writeIface.capturedSecondFrame_
        && writeIface.secondFrame_.objectHandle == 0x702u
        && writeIface.secondFrame_.classNumber >= 500
        && writeIface.capturedThirdFrame_
        && writeIface.thirdFrame_.objectHandle == 0x706u
        && writeIface.thirdFrame_.classNumber >= 500
        && writeIface.thirdFrame_.classNumber != writeIface.firstFrame_.classNumber;
    return result;
}

bool runDxfSurfaceRoundTrip(bool binary,
                            const std::filesystem::path& directory,
                            bool keepOutput) {
    const std::string encoding = binary ? "binary" : "ascii";
    const std::filesystem::path output =
        directory / ("libdxfrw-surface-roundtrip-" + encoding + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    dx_data source;
    auto plane = new DRW_PlaneSurface();
    plane->handle = 0xFB00u;
    plane->modelerFormatVersion = 1;
    plane->uIsolines = 2;
    plane->vIsolines = 3;
    source.mBlock->ent.push_back(plane);
    auto extruded = new DRW_ExtrudedSurface();
    extruded->handle = 0xFB01u;
    extruded->modelerFormatVersion = 1;
    extruded->uIsolines = 4;
    extruded->vIsolines = 5;
    extruded->classId = 2;
    extruded->dxfBinaryData = {0x21u, 0x43u};
    extruded->sweepVector = DRW_Coord(0.0, 0.0, 1.0);
    extruded->draftAngle = 0.1;
    extruded->draftStartDistance = 0.25;
    extruded->draftEndDistance = 0.5;
    extruded->twistAngle = 0.75;
    extruded->scaleFactor = 1.25;
    extruded->alignAngle = 0.5;
    extruded->solid = true;
    extruded->sweepAlignmentFlags = 3;
    extruded->pathFlags = 7;
    extruded->alignStart = true;
    extruded->bank = true;
    extruded->basePointSet = true;
    extruded->sweepEntityTransformComputed = true;
    extruded->pathEntityTransformComputed = true;
    extruded->referenceVector = DRW_Coord(1.0, 2.0, 3.0);
    extruded->extrudedTransform[3] = 0.25;
    extruded->extrudedTransform[7] = 0.5;
    extruded->extrudedTransform[11] = 0.75;
    extruded->sweepEntityTransform[3] = 1.25;
    extruded->pathEntityTransform[7] = 1.5;
    source.mBlock->ent.push_back(extruded);
    auto revolved = new DRW_RevolvedSurface();
    revolved->handle = 0xFB02u;
    revolved->modelerFormatVersion = 1;
    revolved->uIsolines = 6;
    revolved->vIsolines = 7;
    revolved->id = 2;
    revolved->dxfBinaryData = {0x65u, 0x87u};
    revolved->axisPoint = DRW_Coord(1.0, 2.0, 3.0);
    revolved->axisVector = DRW_Coord(0.0, 1.0, 2.0);
    revolved->revolveAngle = 1.5;
    revolved->startAngle = 0.25;
    revolved->draftAngle = 0.5;
    revolved->draftStartDistance = 0.75;
    revolved->draftEndDistance = 1.0;
    revolved->twistAngle = 1.25;
    revolved->solid = true;
    revolved->closeToAxis = true;
    revolved->transform[3] = 0.25;
    revolved->transform[7] = 0.5;
    revolved->transform[11] = 0.75;
    source.mBlock->ent.push_back(revolved);
    auto swept = new DRW_SweptSurface();
    swept->handle = 0xFB03u;
    swept->modelerFormatVersion = 1;
    swept->uIsolines = 8;
    swept->vIsolines = 9;
    swept->sweepEntityId = 4;
    swept->sweepData = {0x01u, 0xA5u};
    swept->pathEntityId = 5;
    swept->pathData = {0xFEu, 0x7Fu};
    swept->referenceVector = DRW_Coord(1.0, 2.0, 3.0);
    swept->sweepEntityTransform[3] = 0.25;
    swept->pathEntityTransform[7] = 0.5;
    swept->sweepEntityTransformed[11] = 0.75;
    swept->pathEntityTransformed[3] = 1.0;
    swept->draftAngle = 0.125;
    swept->draftStartDistance = 0.25;
    swept->draftEndDistance = 0.5;
    swept->twistAngle = 0.75;
    swept->scaleFactor = 1.1;
    swept->alignAngle = 0.875;
    swept->solid = true;
    swept->sweepAlignmentFlags = 2;
    swept->pathFlags = 9;
    swept->alignStart = true;
    swept->bank = true;
    swept->basePointSet = true;
    swept->sweepEntityTransformComputed = true;
    swept->pathEntityTransformComputed = true;
    source.mBlock->ent.push_back(swept);
    auto lofted = new DRW_LoftedSurface();
    lofted->handle = 0xFB04u;
    lofted->modelerFormatVersion = 1;
    lofted->uIsolines = 10;
    lofted->vIsolines = 11;
    lofted->planeNormalLoftingType = 1;
    lofted->startDraftAngle = 0.2;
    lofted->endDraftAngle = 0.3;
    lofted->startDraftMagnitude = 0.4;
    lofted->endDraftMagnitude = 0.5;
    lofted->arcLengthParameterization = true;
    lofted->noTwist = false;
    lofted->alignDirection = false;
    lofted->simpleSurfaces = false;
    lofted->closedSurfaces = true;
    lofted->solid = true;
    lofted->ruledSurface = true;
    lofted->virtualGuide = true;
    lofted->pathCurveHandle = 0x1234u;
    lofted->loftEntityTransform[3] = 0.25;
    lofted->loftEntityTransform[7] = 0.5;
    lofted->loftEntityTransform[11] = 0.75;
    lofted->dxfReferenceData.emplace_back(90, std::int32_t{2});
    lofted->dxfReferenceData.emplace_back(
        310, std::vector<std::uint8_t>{0x12u, 0x34u});
    source.mBlock->ent.push_back(lofted);
    auto nurbs = new DRW_NurbsSurface();
    nurbs->handle = 0xFB05u;
    nurbs->modelerFormatVersion = 1;
    nurbs->uIsolines = 12;
    nurbs->vIsolines = 13;
    nurbs->short170 = 14;
    nurbs->cvHullDisplay = true;
    nurbs->uvec1 = DRW_Coord(1.0, 0.0, 0.0);
    nurbs->vvec1 = DRW_Coord(0.0, 1.0, 0.0);
    nurbs->uvec2 = DRW_Coord(2.0, 0.0, 0.0);
    nurbs->vvec2 = DRW_Coord(0.0, 2.0, 0.0);
    source.mBlock->ent.push_back(nurbs);

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary, &source,
                             false)) {
        std::cerr << "DXF surface export failed (" << encoding << ")\n";
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }
    dx_data imported;
    dx_iface importer;
    if (!importer.fileImport(output.string(), &imported, false)) {
        std::cerr << "DXF surface import failed (" << encoding << ")\n";
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }
    std::array<bool, 6> seen{};
    const auto sameCoord = [](const DRW_Coord& a, const DRW_Coord& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    };
    const auto sameMatrix = [](const auto& a, const auto& b) {
        return a == b;
    };
    bool fieldsMatch = true;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr)
            continue;
        switch (entity->eType) {
        case DRW::PLANESURFACE: {
            seen[0] = true;
            const auto* value = static_cast<const DRW_PlaneSurface*>(entity);
            fieldsMatch = fieldsMatch
                && value->modelerFormatVersion == plane->modelerFormatVersion
                && value->uIsolines == plane->uIsolines
                && value->vIsolines == plane->vIsolines;
            break;
        }
        case DRW::EXTRUDEDSURFACE: {
            seen[1] = true;
            const auto* value = static_cast<const DRW_ExtrudedSurface*>(entity);
            fieldsMatch = fieldsMatch
                && value->modelerFormatVersion == extruded->modelerFormatVersion
                && value->uIsolines == extruded->uIsolines
                && value->vIsolines == extruded->vIsolines
                && value->classId == extruded->classId
                && value->dxfBinaryData == extruded->dxfBinaryData
                && sameCoord(value->sweepVector, extruded->sweepVector)
                && sameCoord(value->referenceVector, extruded->referenceVector)
                && sameMatrix(value->extrudedTransform, extruded->extrudedTransform)
                && sameMatrix(value->sweepEntityTransform, extruded->sweepEntityTransform)
                && sameMatrix(value->pathEntityTransform, extruded->pathEntityTransform)
                && value->draftAngle == extruded->draftAngle
                && value->draftStartDistance == extruded->draftStartDistance
                && value->draftEndDistance == extruded->draftEndDistance
                && value->twistAngle == extruded->twistAngle
                && value->scaleFactor == extruded->scaleFactor
                && value->alignAngle == extruded->alignAngle
                && value->solid == extruded->solid
                && value->sweepAlignmentFlags == extruded->sweepAlignmentFlags
                && value->pathFlags == extruded->pathFlags
                && value->alignStart == extruded->alignStart
                && value->bank == extruded->bank
                && value->basePointSet == extruded->basePointSet
                && value->sweepEntityTransformComputed
                       == extruded->sweepEntityTransformComputed
                && value->pathEntityTransformComputed
                       == extruded->pathEntityTransformComputed;
            break;
        }
        case DRW::REVOLVEDSURFACE: {
            seen[2] = true;
            const auto* value = static_cast<const DRW_RevolvedSurface*>(entity);
            fieldsMatch = fieldsMatch
                && value->modelerFormatVersion == revolved->modelerFormatVersion
                && value->uIsolines == revolved->uIsolines
                && value->vIsolines == revolved->vIsolines
                && value->id == revolved->id
                && value->dxfBinaryData == revolved->dxfBinaryData
                && sameCoord(value->axisPoint, revolved->axisPoint)
                && sameCoord(value->axisVector, revolved->axisVector)
                && value->revolveAngle == revolved->revolveAngle
                && value->startAngle == revolved->startAngle
                && sameMatrix(value->transform, revolved->transform)
                && value->draftAngle == revolved->draftAngle
                && value->draftStartDistance == revolved->draftStartDistance
                && value->draftEndDistance == revolved->draftEndDistance
                && value->twistAngle == revolved->twistAngle
                && value->solid == revolved->solid
                && value->closeToAxis == revolved->closeToAxis;
            break;
        }
        case DRW::SWEPTSURFACE: {
            seen[3] = true;
            const auto* value = static_cast<const DRW_SweptSurface*>(entity);
            fieldsMatch = fieldsMatch
                && value->modelerFormatVersion == swept->modelerFormatVersion
                && value->uIsolines == swept->uIsolines
                && value->vIsolines == swept->vIsolines
                && value->sweepEntityId == swept->sweepEntityId
                && value->sweepData == swept->sweepData
                && value->pathEntityId == swept->pathEntityId
                && value->pathData == swept->pathData
                && sameCoord(value->referenceVector, swept->referenceVector)
                && sameMatrix(value->sweepEntityTransform, swept->sweepEntityTransform)
                && sameMatrix(value->pathEntityTransform, swept->pathEntityTransform)
                && sameMatrix(value->sweepEntityTransformed, swept->sweepEntityTransformed)
                && sameMatrix(value->pathEntityTransformed, swept->pathEntityTransformed)
                && value->draftAngle == swept->draftAngle
                && value->draftStartDistance == swept->draftStartDistance
                && value->draftEndDistance == swept->draftEndDistance
                && value->twistAngle == swept->twistAngle
                && value->scaleFactor == swept->scaleFactor
                && value->alignAngle == swept->alignAngle
                && value->solid == swept->solid
                && value->sweepAlignmentFlags == swept->sweepAlignmentFlags
                && value->pathFlags == swept->pathFlags
                && value->alignStart == swept->alignStart
                && value->bank == swept->bank
                && value->basePointSet == swept->basePointSet
                && value->sweepEntityTransformComputed
                       == swept->sweepEntityTransformComputed
                && value->pathEntityTransformComputed
                       == swept->pathEntityTransformComputed;
            break;
        }
        case DRW::LOFTEDSURFACE: {
            seen[4] = true;
            const auto* value = static_cast<const DRW_LoftedSurface*>(entity);
            fieldsMatch = fieldsMatch
                && value->modelerFormatVersion == lofted->modelerFormatVersion
                && value->uIsolines == lofted->uIsolines
                && value->vIsolines == lofted->vIsolines
                && value->planeNormalLoftingType == lofted->planeNormalLoftingType
                && value->startDraftAngle == lofted->startDraftAngle
                && value->endDraftAngle == lofted->endDraftAngle
                && value->startDraftMagnitude == lofted->startDraftMagnitude
                && value->endDraftMagnitude == lofted->endDraftMagnitude
                && value->arcLengthParameterization == lofted->arcLengthParameterization
                && value->noTwist == lofted->noTwist
                && value->alignDirection == lofted->alignDirection
                && value->simpleSurfaces == lofted->simpleSurfaces
                && value->closedSurfaces == lofted->closedSurfaces
                && value->solid == lofted->solid
                && value->ruledSurface == lofted->ruledSurface
                && value->virtualGuide == lofted->virtualGuide
                && value->pathCurveHandle == lofted->pathCurveHandle
                && sameMatrix(value->loftEntityTransform, lofted->loftEntityTransform)
                && value->dxfReferenceData.size() == lofted->dxfReferenceData.size()
                && value->dxfReferenceData[0].code() == 90
                && value->dxfReferenceData[0].i_val() == 2
                && value->dxfReferenceData[1].code() == 310
                && value->dxfReferenceData[1].binary()
                && *value->dxfReferenceData[1].binary()
                       == std::vector<std::uint8_t>{0x12u, 0x34u};
            break;
        }
        case DRW::NURBSURFACE: {
            seen[5] = true;
            const auto* value = static_cast<const DRW_NurbsSurface*>(entity);
            fieldsMatch = fieldsMatch
                && value->modelerFormatVersion == nurbs->modelerFormatVersion
                && value->uIsolines == nurbs->uIsolines
                && value->vIsolines == nurbs->vIsolines
                && value->short170 == nurbs->short170
                && value->cvHullDisplay == nurbs->cvHullDisplay
                && sameCoord(value->uvec1, nurbs->uvec1)
                && sameCoord(value->vvec1, nurbs->vvec1)
                && sameCoord(value->uvec2, nurbs->uvec2)
                && sameCoord(value->vvec2, nurbs->vvec2);
            break;
        }
        default: break;
        }
    }
    if (!keepOutput)
        std::filesystem::remove(output, ec);
    const bool result =
        std::all_of(seen.begin(), seen.end(), [](bool value) { return value; })
        && fieldsMatch;
    if (!result)
        std::cerr << "DXF surface semantic mismatch (" << encoding
                  << "), seen=" << seen[0] << seen[1] << seen[2]
                  << seen[3] << seen[4] << seen[5]
                  << ", fields=" << fieldsMatch << '\n';
    return result;
}

bool runDxfSurfaceRejectsMalformed(const std::filesystem::path& directory) {
    const std::filesystem::path sourcePath =
        directory / "libdxfrw-surface-malformed-source.dxf";
    std::error_code ec;
    std::filesystem::remove(sourcePath, ec);

    dx_data source;
    auto* extruded = new DRW_ExtrudedSurface();
    extruded->handle = 0xFBD0u;
    extruded->classId = 7;
    extruded->dxfBinaryData = {0x12u, 0x34u};
    extruded->solid = true;
    source.mBlock->ent.push_back(extruded);
    auto* lofted = new DRW_LoftedSurface();
    lofted->handle = 0xFBD1u;
    lofted->loftEntityTransform[3] = 0.25;
    source.mBlock->ent.push_back(lofted);

    dx_iface exporter;
    if (!exporter.fileExport(sourcePath.string(), DRW::AC1027, false, &source,
                             false)) {
        std::filesystem::remove(sourcePath, ec);
        return false;
    }
    std::ifstream input(sourcePath, std::ios::binary);
    if (!input) {
        std::filesystem::remove(sourcePath, ec);
        return false;
    }
    std::string original{std::istreambuf_iterator<char>{input},
                         std::istreambuf_iterator<char>{}};
    std::filesystem::remove(sourcePath, ec);

    // Windows text-mode output expands LF to CRLF; normalize before the exact
    // group-code edits below so the malformed cases are platform-independent.
    original.erase(std::remove(original.begin(), original.end(), '\r'),
                   original.end());

    const auto writeText = [](const std::filesystem::path& path,
                              const std::string& text) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        return output.good();
    };
    const auto rejects = [&](std::string text, const char* suffix) {
        const std::filesystem::path malformed = directory /
            (std::string("libdxfrw-surface-malformed-") + suffix + ".dxf");
        std::filesystem::remove(malformed, ec);
        if (!writeText(malformed, text)) {
            std::filesystem::remove(malformed, ec);
            return false;
        }
        dx_data imported;
        dx_iface importer;
        const bool rejected =
            !importer.fileImport(malformed.string(), &imported, false);
        std::filesystem::remove(malformed, ec);
        return rejected;
    };

    std::string badSize = original;
    const std::size_t extrudedAt = badSize.find("AcDbExtrudedSurface");
    const std::string group90 = "\n 90\n";
    const std::size_t classIdAt = extrudedAt == std::string::npos
        ? std::string::npos : badSize.find(group90, extrudedAt);
    const std::size_t sizeAt = classIdAt == std::string::npos
        ? std::string::npos : badSize.find(group90, classIdAt + group90.size());
    bool sizeChanged = false;
    if (sizeAt != std::string::npos) {
        const std::size_t valueStart = sizeAt + group90.size();
        const std::size_t valueDigit = badSize.find_first_not_of(" \t", valueStart);
        const std::size_t valueEnd = badSize.find('\n', valueStart);
        if (valueDigit != std::string::npos && valueDigit < valueEnd
            && badSize[valueDigit] == '2') {
            badSize[valueDigit] = '3';
            sizeChanged = true;
        }
    }

    std::string badBoolean = original;
    const std::size_t solidAt = badBoolean.find("\n290\n1\n", extrudedAt);
    const bool booleanChanged = solidAt != std::string::npos
        && (badBoolean[solidAt + 5] = '2') == '2';

    std::string partialMatrix = original;
    const std::size_t loftedAt = partialMatrix.find("AcDbLoftedSurface");
    std::size_t matrixAt = loftedAt == std::string::npos
        ? std::string::npos : partialMatrix.find("\n 40\n", loftedAt);
    for (int index = 1; matrixAt != std::string::npos && index < 16; ++index)
        matrixAt = partialMatrix.find("\n 40\n", matrixAt + 1);
    bool matrixRemoved = false;
    if (matrixAt != std::string::npos) {
        const std::size_t valueEnd = partialMatrix.find('\n', matrixAt + 5);
        if (valueEnd != std::string::npos) {
            partialMatrix.erase(matrixAt, valueEnd - matrixAt + 1);
            matrixRemoved = true;
        }
    }

    return sizeChanged && booleanChanged && matrixRemoved
        && rejects(std::move(badSize), "size")
        && rejects(std::move(badBoolean), "boolean")
        && rejects(std::move(partialMatrix), "matrix");
}

bool runDxfSplineSurfaceRejectsInvalidPayload(
        const std::filesystem::path& directory) {
    const auto rejects = [&](DRW_Entity* entity, const char* suffix) {
        const std::filesystem::path output = directory /
            (std::string("libdxfrw-invalid-3d-payload-") + suffix + ".dxf");
        std::error_code ec;
        std::filesystem::remove(output, ec);
        dx_data source;
        source.mBlock->ent.push_back(entity);
        dx_iface exporter;
        const bool rejected = !exporter.fileExport(output.string(),
            DRW::AC1027, false, &source, false);
        std::filesystem::remove(output, ec);
        return rejected;
    };

    auto* invalidHelix = new DRW_Helix();
    invalidHelix->degree = 2;
    invalidHelix->knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
    invalidHelix->controllist = {
        std::make_shared<DRW_Coord>(0.0, 0.0, 0.0),
        std::make_shared<DRW_Coord>(1.0, 1.0, 1.0),
        std::make_shared<DRW_Coord>(2.0, 2.0, 2.0)};
    invalidHelix->constraintType = 3;

    auto* partialWeights = new DRW_Spline();
    partialWeights->degree = 2;
    partialWeights->flags = 4;
    partialWeights->knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
    partialWeights->controllist = {
        std::make_shared<DRW_Coord>(0.0, 0.0, 0.0),
        std::make_shared<DRW_Coord>(1.0, 1.0, 1.0),
        std::make_shared<DRW_Coord>(2.0, 2.0, 2.0)};
    partialWeights->weightlist = {1.0, 2.0};

    auto* invalidSurface = new DRW_ExtrudedSurface();
    invalidSurface->sweepAlignmentFlags = 4;

    return rejects(invalidHelix, "helix-constraint")
        && rejects(partialWeights, "spline-weights")
        && rejects(invalidSurface, "surface-alignment");
}

bool runDxfSplineHelixRoundTrip(bool binary,
                               const std::filesystem::path& directory,
                               bool keepOutput) {
    const std::string encoding = binary ? "binary" : "ascii";
    const std::filesystem::path output = directory /
        ("libdxfrw-spline-helix-" + encoding + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    dx_data source;
    auto* controlSpline = new DRW_Spline();
    controlSpline->handle = 0xFC00u;
    controlSpline->degree = 2;
    controlSpline->flags = 4 | 8;
    controlSpline->normalVec = DRW_Coord(0.0, 0.0, 1.0);
    controlSpline->knotslist = {0.0, 0.0, 0.0, 0.5, 1.0, 1.0};
    controlSpline->weightlist = {1.0, 0.5, 2.0};
    controlSpline->tolknot = 0.125;
    controlSpline->tolcontrol = 0.25;
    controlSpline->tolfit = 0.5;
    controlSpline->tgStart = DRW_Coord(1.0, 2.0, 3.0);
    controlSpline->tgEnd = DRW_Coord(4.0, 5.0, 6.0);
    controlSpline->controllist = {
        std::make_shared<DRW_Coord>(1.0, 2.0, 3.0),
        std::make_shared<DRW_Coord>(4.0, 5.0, 6.0),
        std::make_shared<DRW_Coord>(7.0, 8.0, 9.0)};
    source.mBlock->ent.push_back(controlSpline);

    auto* fitSpline = new DRW_Spline();
    fitSpline->handle = 0xFC01u;
    fitSpline->degree = 3;
    fitSpline->tgStart = DRW_Coord(0.25, 0.5, 0.75);
    fitSpline->tgEnd = DRW_Coord(1.0, 1.25, 1.5);
    fitSpline->fitlist = {
        std::make_shared<DRW_Coord>(2.0, 3.0, 4.0),
        std::make_shared<DRW_Coord>(5.0, 6.0, 7.0),
        std::make_shared<DRW_Coord>(8.0, 9.0, 10.0),
        std::make_shared<DRW_Coord>(11.0, 12.0, 13.0)};
    source.mBlock->ent.push_back(fitSpline);

    auto* helix = new DRW_Helix();
    helix->handle = 0xFC02u;
    helix->degree = 2;
    helix->flags = 4;
    helix->knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
    helix->weightlist = {1.0, 0.75, 1.5};
    helix->controllist = {
        std::make_shared<DRW_Coord>(14.0, 15.0, 16.0),
        std::make_shared<DRW_Coord>(17.0, 18.0, 19.0),
        std::make_shared<DRW_Coord>(20.0, 21.0, 22.0)};
    helix->m_majorVersion = 1;
    helix->m_maintVersion = 2;
    helix->axisBasePt = DRW_Coord(3.0, 4.0, 5.0);
    helix->startPt = DRW_Coord(6.0, 7.0, 8.0);
    helix->axisVector = DRW_Coord(0.0, 0.0, 1.0);
    helix->radius = 2.5;
    helix->turns = 3.25;
    helix->turnHeight = 4.5;
    helix->handedness = true;
    helix->constraintType = 2;
    source.mBlock->ent.push_back(helix);

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary, &source,
                             false)) {
        std::cerr << "DXF spline/helix export failed (" << encoding << ")\n";
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }
    dx_data imported;
    dx_iface importer;
    if (!importer.fileImport(output.string(), &imported, false)) {
        std::cerr << "DXF spline/helix import failed (" << encoding << ")\n";
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }

    const DRW_Spline* importedControl = nullptr;
    const DRW_Spline* importedFit = nullptr;
    const DRW_Helix* importedHelix = nullptr;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr)
            continue;
        if (entity->eType == DRW::SPLINE) {
            const auto* value = static_cast<const DRW_Spline*>(entity);
            if (value->handle == controlSpline->handle)
                importedControl = value;
            else if (value->handle == fitSpline->handle)
                importedFit = value;
        } else if (entity->eType == DRW::HELIX) {
            importedHelix = static_cast<const DRW_Helix*>(entity);
        }
    }
    const auto sameCoord = [](const DRW_Coord& a, const DRW_Coord& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    };
    const auto samePoints = [&sameCoord](const auto& a, const auto& b) {
        if (a.size() != b.size())
            return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (!a[i] || !b[i] || !sameCoord(*a[i], *b[i]))
                return false;
        }
        return true;
    };
    const bool controlMatches = importedControl != nullptr
        && importedControl->degree == controlSpline->degree
        && importedControl->flags == controlSpline->flags
        && sameCoord(importedControl->normalVec, controlSpline->normalVec)
        && importedControl->knotslist == controlSpline->knotslist
        && importedControl->weightlist == controlSpline->weightlist
        && samePoints(importedControl->controllist, controlSpline->controllist)
        && importedControl->fitlist.empty()
        && importedControl->tolknot == controlSpline->tolknot
        && importedControl->tolcontrol == controlSpline->tolcontrol
        && importedControl->tolfit == controlSpline->tolfit
        && sameCoord(importedControl->tgStart, controlSpline->tgStart)
        && sameCoord(importedControl->tgEnd, controlSpline->tgEnd);
    const bool fitMatches = importedFit != nullptr
        && importedFit->degree == fitSpline->degree
        && sameCoord(importedFit->normalVec, fitSpline->normalVec)
        && importedFit->knotslist.empty()
        && importedFit->weightlist.empty()
        && importedFit->controllist.empty()
        && samePoints(importedFit->fitlist, fitSpline->fitlist)
        && sameCoord(importedFit->tgStart, fitSpline->tgStart)
        && sameCoord(importedFit->tgEnd, fitSpline->tgEnd);
    const bool helixMatches = importedHelix != nullptr
        && importedHelix->m_majorVersion == helix->m_majorVersion
        && importedHelix->m_maintVersion == helix->m_maintVersion
        && importedHelix->degree == helix->degree
        && importedHelix->flags == helix->flags
        && importedHelix->knotslist == helix->knotslist
        && importedHelix->weightlist == helix->weightlist
        && samePoints(importedHelix->controllist, helix->controllist)
        && sameCoord(importedHelix->axisBasePt, helix->axisBasePt)
        && sameCoord(importedHelix->startPt, helix->startPt)
        && sameCoord(importedHelix->axisVector, helix->axisVector)
        && importedHelix->radius == helix->radius
        && importedHelix->turns == helix->turns
        && importedHelix->turnHeight == helix->turnHeight
        && importedHelix->handedness == helix->handedness
        && importedHelix->constraintType == helix->constraintType;
    if (!keepOutput)
        std::filesystem::remove(output, ec);
    if (!controlMatches || !fitMatches || !helixMatches)
        std::cerr << "DXF SPLINE/HELIX semantic mismatch (" << encoding
                  << "): control=" << controlMatches << ", fit=" << fitMatches
                  << ", helix=" << helixMatches << '\n';
    if (!fitMatches && importedFit != nullptr)
        std::cerr << "  fit spline: degree=" << importedFit->degree << "/"
                  << fitSpline->degree << " normal=(" << importedFit->normalVec.x
                  << "," << importedFit->normalVec.y << ","
                  << importedFit->normalVec.z << ") expected=("
                  << fitSpline->normalVec.x << "," << fitSpline->normalVec.y
                  << "," << fitSpline->normalVec.z << ") points="
                  << importedFit->fitlist.size() << "/" << fitSpline->fitlist.size()
                  << " tangents=(" << importedFit->tgStart.x << ","
                  << importedFit->tgStart.y << "," << importedFit->tgStart.z
                  << ";" << importedFit->tgEnd.x << "," << importedFit->tgEnd.y
                  << "," << importedFit->tgEnd.z << ") expected=("
                  << fitSpline->tgStart.x << "," << fitSpline->tgStart.y << ","
                  << fitSpline->tgStart.z << ";" << fitSpline->tgEnd.x << ","
                  << fitSpline->tgEnd.y << "," << fitSpline->tgEnd.z << ")\n";
    if (!controlMatches && importedControl != nullptr)
        std::cerr << "  control spline: flags=" << importedControl->flags
                  << "/" << controlSpline->flags
                  << " normal=(" << importedControl->normalVec.x << ","
                  << importedControl->normalVec.y << ","
                  << importedControl->normalVec.z << ") knots="
                  << importedControl->knotslist.size() << " weights="
                  << importedControl->weightlist.size() << " controls="
                  << importedControl->controllist.size() << " fit="
                  << importedControl->fitlist.size() << " tolerances=("
                  << importedControl->tolknot << ","
                  << importedControl->tolcontrol << ","
                  << importedControl->tolfit << ") tangents=("
                  << importedControl->tgStart.x << ","
                  << importedControl->tgStart.y << ","
                  << importedControl->tgStart.z << ";"
                  << importedControl->tgEnd.x << ","
                  << importedControl->tgEnd.y << ","
                  << importedControl->tgEnd.z << ")\n";
    if (!helixMatches && importedHelix != nullptr)
        std::cerr << "  helix: version="
                  << importedHelix->m_majorVersion << "/"
                  << helix->m_majorVersion << ","
                  << importedHelix->m_maintVersion << "/"
                  << helix->m_maintVersion << " degree="
                  << importedHelix->degree << "/" << helix->degree
                  << " flags=" << importedHelix->flags << "/"
                  << helix->flags << " knots="
                  << importedHelix->knotslist.size() << " weights="
                  << importedHelix->weightlist.size() << " controls="
                  << importedHelix->controllist.size() << " axisBase=("
                  << importedHelix->axisBasePt.x << ","
                  << importedHelix->axisBasePt.y << ","
                  << importedHelix->axisBasePt.z << ") radius="
                  << importedHelix->radius << "/" << helix->radius
                  << " turns=" << importedHelix->turns << "/"
                  << helix->turns << " height=" << importedHelix->turnHeight
                  << "/" << helix->turnHeight << " handed="
                  << importedHelix->handedness << "/" << helix->handedness
                  << " constraint="
                  << static_cast<int>(importedHelix->constraintType) << "/"
                  << static_cast<int>(helix->constraintType) << " points="
                  << samePoints(importedHelix->controllist, helix->controllist)
                  << " knotsSame="
                  << (importedHelix->knotslist == helix->knotslist)
                  << " weightsSame="
                  << (importedHelix->weightlist == helix->weightlist)
                  << " start=" << importedHelix->startPt.x << ","
                  << importedHelix->startPt.y << ","
                  << importedHelix->startPt.z << "/" << helix->startPt.x
                  << "," << helix->startPt.y << "," << helix->startPt.z
                  << " axis=" << importedHelix->axisVector.x << ","
                  << importedHelix->axisVector.y << ","
                  << importedHelix->axisVector.z << "/" << helix->axisVector.x
                  << "," << helix->axisVector.y << "," << helix->axisVector.z
                  << '\n';
    return controlMatches && fitMatches && helixMatches;
}

class DxfMeshCaptureIface final : public dx_iface {
public:
    void addMesh(const DRW_Mesh& data) override {
        mesh = data;
        seen = true;
    }

    DRW_Mesh mesh;
    bool seen = false;
};

bool runDxfTopologyRoundTrip(bool binary, const std::filesystem::path& directory,
                             bool keepOutput) {
    const std::string encoding = binary ? "binary" : "ascii";
    const std::filesystem::path output = directory /
        ("libdxfrw-dxf-topology-" + encoding + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    dx_data source;
    auto* face = new DRW_3Dface();
    face->handle = 0xFA01u;
    face->basePoint = DRW_Coord(1.0, 2.0, 3.0);
    face->secPoint = DRW_Coord(4.0, 5.0, 6.0);
    face->thirdPoint = DRW_Coord(7.0, 8.0, 9.0);
    face->fourPoint = DRW_Coord(10.0, 11.0, 12.0);
    face->invisibleflag = DRW_3Dface::FirstEdge | DRW_3Dface::ThirdEdge;
    source.mBlock->ent.push_back(face);

    auto* solid = new DRW_Solid();
    solid->handle = 0xFA06u;
    solid->basePoint = DRW_Coord(22.0, 23.0, 24.0);
    solid->secPoint = DRW_Coord(25.0, 26.0, 27.0);
    solid->thirdPoint = DRW_Coord(28.0, 29.0, 30.0);
    solid->fourPoint = DRW_Coord(31.0, 32.0, 33.0);
    source.mBlock->ent.push_back(solid);

    auto* trace = new DRW_Trace();
    trace->handle = 0xFA07u;
    trace->basePoint = DRW_Coord(34.0, 35.0, 36.0);
    trace->secPoint = DRW_Coord(37.0, 38.0, 39.0);
    trace->thirdPoint = DRW_Coord(40.0, 41.0, 42.0);
    trace->fourPoint = DRW_Coord(43.0, 44.0, 45.0);
    trace->thickness = 2.5;
    trace->extPoint = DRW_Coord(0.0, 0.0, -1.0);
    source.mBlock->ent.push_back(trace);

    auto* poly3d = new DRW_Polyline();
    poly3d->handle = 0xFA02u;
    poly3d->flags = 8;
    poly3d->vertexcount = 3;
    // DWG 3D polylines do not store a meaningful extrusion vector. A zero
    // initialized DXF normal must not leak into WCS-only POLYLINE output.
    poly3d->extPoint = DRW_Coord(0.0, 0.0, 0.0);
    poly3d->addVertex(DRW_Vertex(13.0, 14.0, 15.0, 0.0));
    poly3d->addVertex(DRW_Vertex(16.0, 17.0, 18.0, 0.0));
    poly3d->addVertex(DRW_Vertex(19.0, 20.0, 21.0, 0.0));
    for (const auto& vertex : poly3d->vertlist)
        vertex->flags = 32;
    source.mBlock->ent.push_back(poly3d);

    auto* polyface = new DRW_Polyline();
    polyface->handle = 0xFA03u;
    polyface->flags = 64;
    polyface->vertexcount = 4;
    polyface->facecount = 1;
    polyface->extPoint = DRW_Coord(0.0, 0.0, 0.0);
    const std::array<DRW_Coord, 4> polyfacePoints{{
        DRW_Coord(0.0, 0.0, 0.0), DRW_Coord(1.0, 0.0, 0.0),
        DRW_Coord(1.0, 1.0, 0.0), DRW_Coord(0.0, 1.0, 0.0)}};
    for (const DRW_Coord& point : polyfacePoints) {
        DRW_Vertex vertex(point.x, point.y, point.z, 0.0);
        vertex.flags = 192;
        polyface->addVertex(vertex);
    }
    DRW_Vertex polyfaceFace;
    // DWG PFACE face records carry indices but no flags field. The writer
    // must recover the DXF face-record marker from this typed subtype.
    polyfaceFace.setDwgSubtype(DRW_Vertex::DwgSubtype::PolyfaceFace);
    polyfaceFace.vindex1 = 1;
    polyfaceFace.vindex2 = -2;
    polyfaceFace.vindex3 = 3;
    polyfaceFace.vindex4 = -4;
    polyface->addVertex(polyfaceFace);
    source.mBlock->ent.push_back(polyface);

    auto* polygonMesh = new DRW_Polyline();
    polygonMesh->handle = 0xFA0Au;
    polygonMesh->flags = 16;
    polygonMesh->vertexcount = 2;
    polygonMesh->facecount = 2;
    polygonMesh->extPoint = DRW_Coord(0.0, 0.0, 0.0);
    const std::array<DRW_Coord, 4> polygonMeshPoints{{
        DRW_Coord(0.0, 0.0, 0.0), DRW_Coord(2.0, 0.0, 0.0),
        DRW_Coord(2.0, 2.0, 4.0), DRW_Coord(0.0, 2.0, 0.0)}};
    for (const DRW_Coord& point : polygonMeshPoints) {
        DRW_Vertex vertex(point.x, point.y, point.z, 0.0);
        vertex.flags = 64;
        polygonMesh->addVertex(vertex);
    }
    source.mBlock->ent.push_back(polygonMesh);

    auto* ocsClassicPolyline = new DRW_Polyline();
    ocsClassicPolyline->handle = 0xFA0Bu;
    ocsClassicPolyline->flags = 1;
    ocsClassicPolyline->basePoint.z = 5.0;
    ocsClassicPolyline->extPoint = DRW_Coord(0.0, 0.6, 0.8);
    ocsClassicPolyline->addVertex(DRW_Vertex(2.0, 3.0, 0.0, 0.0));
    ocsClassicPolyline->addVertex(DRW_Vertex(4.0, 5.0, 0.0, 0.0));
    source.mBlock->ent.push_back(ocsClassicPolyline);

    auto* ocsPolyline = new DRW_LWPolyline();
    ocsPolyline->handle = 0xFA04u;
    ocsPolyline->elevation = 5.0;
    ocsPolyline->extPoint = DRW_Coord(0.0, 1.0, 0.0);
    ocsPolyline->addVertex(DRW_Vertex2D(2.0, 3.0, 0.25));
    ocsPolyline->addVertex(DRW_Vertex2D(4.0, 5.0, 0.0));
    source.mBlock->ent.push_back(ocsPolyline);

    auto* mesh = new DRW_Mesh();
    mesh->handle = 0xFA05u;
    mesh->version = 2;
    mesh->blendCrease = true;
    mesh->subdivisionLevel = 2;
    mesh->vertices = {
        DRW_Coord(0.0, 0.0, 0.0), DRW_Coord(2.0, 0.0, 0.0),
        DRW_Coord(2.0, 2.0, 1.0), DRW_Coord(0.0, 2.0, 0.0)};
    mesh->faces = {{0, 1, 2, 3}};
    mesh->edges = {{0, 1}, {1, 2}};
    mesh->creases = {0.25, 0.75};
    source.mBlock->ent.push_back(mesh);

    auto* hatch = new DRW_Hatch();
    hatch->handle = 0xFA08u;
    hatch->name = "SOLID";
    auto hatchLoop = std::make_shared<DRW_HatchLoop>(0);
    auto hatchArc = std::make_shared<DRW_Arc>();
    hatchArc->basePoint = DRW_Coord(1.0, 2.0, 0.0);
    hatchArc->radious = 3.0;
    hatchArc->staangle = 0.25;
    hatchArc->endangle = 1.5;
    hatchArc->isccw = 0;
    auto hatchEllipse = std::make_shared<DRW_Ellipse>();
    hatchEllipse->basePoint = DRW_Coord(4.0, 5.0, 0.0);
    hatchEllipse->secPoint = DRW_Coord(7.0, 9.0, 0.0);
    hatchEllipse->ratio = 0.5;
    hatchEllipse->staparam = 0.25;
    hatchEllipse->endparam = 1.5;
    hatchEllipse->isccw = 0;
    hatchLoop->objlist.push_back(hatchArc);
    hatchLoop->objlist.push_back(hatchEllipse);
    hatchLoop->update();
    hatch->appendLoop(hatchLoop);
    source.mBlock->ent.push_back(hatch);

    auto* mpolygon = new DRW_MPolygon();
    mpolygon->handle = 0xFA09u;
    mpolygon->name = "SOLID";
    mpolygon->solid = 1;
    mpolygon->annotatedBoundary = 1;
    mpolygon->xDirX = 0.25;
    mpolygon->xDirY = -0.5;
    auto mpolygonLoop = std::make_shared<DRW_HatchLoop>(2);
    auto mpolygonBoundary = std::make_shared<DRW_LWPolyline>();
    mpolygonBoundary->flags = 1;
    mpolygonBoundary->addVertex(DRW_Vertex2D(1.0, 2.0, 0.0));
    mpolygonBoundary->addVertex(DRW_Vertex2D(4.0, 2.0, 0.0));
    mpolygonBoundary->addVertex(DRW_Vertex2D(3.0, 5.0, 0.0));
    mpolygonLoop->objlist.push_back(mpolygonBoundary);
    mpolygonLoop->update();
    mpolygon->appendLoop(mpolygonLoop);
    source.mBlock->ent.push_back(mpolygon);

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary, &source,
                             false)) {
        std::cerr << "DXF topology export failed (" << encoding << "): "
                  << output << '\n';
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }

    dx_data imported;
    DxfMeshCaptureIface importer;
    if (!importer.fileImport(output.string(), &imported, false)) {
        std::cerr << "DXF topology import failed (" << encoding << "): "
                  << output << '\n';
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }
    const DRW_3Dface* decodedFace = nullptr;
    const DRW_Solid* decodedSolid = nullptr;
    const DRW_Trace* decodedTrace = nullptr;
    const DRW_Polyline* decodedPoly3d = nullptr;
    const DRW_Polyline* decodedPolyface = nullptr;
    const DRW_Polyline* decodedPolygonMesh = nullptr;
    const DRW_Polyline* decodedOcsClassicPolyline = nullptr;
    const DRW_LWPolyline* decodedOcsPolyline = nullptr;
    const DRW_Hatch* decodedHatch = nullptr;
    const DRW_MPolygon* decodedMPolygon = nullptr;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr)
            continue;
        if (entity->eType == DRW::E3DFACE)
            decodedFace = static_cast<const DRW_3Dface*>(entity);
        else if (entity->eType == DRW::SOLID)
            decodedSolid = static_cast<const DRW_Solid*>(entity);
        else if (entity->eType == DRW::DXF_TRACE)
            decodedTrace = static_cast<const DRW_Trace*>(entity);
        else if (entity->eType == DRW::POLYLINE) {
            const auto* polyline = static_cast<const DRW_Polyline*>(entity);
            if (polyline->flags == 8)
                decodedPoly3d = polyline;
            else if (polyline->flags == 16)
                decodedPolygonMesh = polyline;
            else if (polyline->flags == 64)
                decodedPolyface = polyline;
            else if (polyline->flags == 1)
                decodedOcsClassicPolyline = polyline;
        } else if (entity->eType == DRW::LWPOLYLINE)
            decodedOcsPolyline = static_cast<const DRW_LWPolyline*>(entity);
        else if (entity->eType == DRW::HATCH)
            decodedHatch = static_cast<const DRW_Hatch*>(entity);
        else if (entity->eType == DRW::MPOLYGON)
            decodedMPolygon = static_cast<const DRW_MPolygon*>(entity);
    }

    const bool faceValid = decodedFace != nullptr
        && decodedFace->basePoint.x == 1.0
        && decodedFace->basePoint.z == 3.0
        && decodedFace->fourPoint.x == 10.0
        && decodedFace->fourPoint.z == 12.0
        && decodedFace->invisibleflag == (DRW_3Dface::FirstEdge
                                           | DRW_3Dface::ThirdEdge);
    const bool solidValid = decodedSolid != nullptr
        && decodedSolid->basePoint.x == 22.0
        && decodedSolid->basePoint.y == 23.0
        && decodedSolid->basePoint.z == 24.0
        && decodedSolid->secPoint.x == 25.0
        && decodedSolid->secPoint.y == 26.0
        && decodedSolid->secPoint.z == 27.0
        && decodedSolid->thirdPoint.x == 28.0
        && decodedSolid->thirdPoint.y == 29.0
        && decodedSolid->thirdPoint.z == 30.0
        && decodedSolid->fourPoint.x == 31.0
        && decodedSolid->fourPoint.y == 32.0
        && decodedSolid->fourPoint.z == 33.0;
    const bool traceValid = decodedTrace != nullptr
        && decodedTrace->basePoint.x == 34.0
        && decodedTrace->basePoint.y == 35.0
        && decodedTrace->basePoint.z == 36.0
        && decodedTrace->secPoint.x == 37.0
        && decodedTrace->secPoint.y == 38.0
        && decodedTrace->secPoint.z == 39.0
        && decodedTrace->thirdPoint.x == 40.0
        && decodedTrace->thirdPoint.y == 41.0
        && decodedTrace->thirdPoint.z == 42.0
        && decodedTrace->fourPoint.x == 43.0
        && decodedTrace->fourPoint.y == 44.0
        && decodedTrace->fourPoint.z == 45.0
        && decodedTrace->thickness == 2.5
        && decodedTrace->extPoint.x == 0.0
        && decodedTrace->extPoint.y == 0.0
        && decodedTrace->extPoint.z == -1.0;
    const bool poly3dValid = decodedPoly3d != nullptr
        && decodedPoly3d->flags == 8 && decodedPoly3d->vertlist.size() == 3
        && !decodedPoly3d->haveExtrusion
        && decodedPoly3d->extPoint.x == 0.0
        && decodedPoly3d->extPoint.y == 0.0
        && decodedPoly3d->extPoint.z == 1.0
        && decodedPoly3d->vertlist[1]->basePoint.x == 16.0
        && decodedPoly3d->vertlist[1]->basePoint.z == 18.0;
    const bool polygonMeshValid = decodedPolygonMesh != nullptr
        && decodedPolygonMesh->flags == 16
        && decodedPolygonMesh->vertexcount == 2
        && decodedPolygonMesh->facecount == 2
        && decodedPolygonMesh->vertlist.size() == 4
        && !decodedPolygonMesh->haveExtrusion
        && decodedPolygonMesh->extPoint.x == 0.0
        && decodedPolygonMesh->extPoint.y == 0.0
        && decodedPolygonMesh->extPoint.z == 1.0
        && decodedPolygonMesh->vertlist[2]->basePoint.z == 4.0;
    const bool polyfaceValid = decodedPolyface != nullptr
        && decodedPolyface->flags == 64
        && decodedPolyface->vertexcount == 4 && decodedPolyface->facecount == 1
        && decodedPolyface->vertlist.size() == 5
        && !decodedPolyface->haveExtrusion
        && decodedPolyface->extPoint.x == 0.0
        && decodedPolyface->extPoint.y == 0.0
        && decodedPolyface->extPoint.z == 1.0
        && decodedPolyface->vertlist[0]->flags == 192
        && decodedPolyface->vertlist[4]->flags == 128
        && decodedPolyface->vertlist[4]->vindex1 == 1
        && decodedPolyface->vertlist[4]->vindex2 == -2
        && decodedPolyface->vertlist[4]->vindex3 == 3
        && decodedPolyface->vertlist[4]->vindex4 == -4;
    const bool ocsClassicPolylineValid = decodedOcsClassicPolyline != nullptr
        && decodedOcsClassicPolyline->flags == 1
        && decodedOcsClassicPolyline->basePoint.z == 5.0
        && decodedOcsClassicPolyline->haveExtrusion
        && decodedOcsClassicPolyline->extPoint.x == 0.0
        && decodedOcsClassicPolyline->extPoint.y == 0.6
        && decodedOcsClassicPolyline->extPoint.z == 0.8
        && decodedOcsClassicPolyline->vertlist.size() == 2
        && decodedOcsClassicPolyline->vertlist[1]->basePoint.x == 4.0
        && decodedOcsClassicPolyline->vertlist[1]->basePoint.y == 5.0;
    const bool ocsValid = decodedOcsPolyline != nullptr
        && decodedOcsPolyline->elevation == 5.0
        && decodedOcsPolyline->extPoint.x == 0.0
        && decodedOcsPolyline->extPoint.y == 1.0
        && decodedOcsPolyline->extPoint.z == 0.0
        && decodedOcsPolyline->vertlist.size() == 2
        && decodedOcsPolyline->vertlist[0]->x == 2.0
        && decodedOcsPolyline->vertlist[0]->y == 3.0
        && decodedOcsPolyline->vertlist[0]->bulge == 0.25;
    const bool meshValid = importer.seen && importer.mesh.version == 2
        && importer.mesh.blendCrease
        && importer.mesh.subdivisionLevel == 2
        && importer.mesh.vertices.size() == mesh->vertices.size()
        && std::equal(importer.mesh.vertices.begin(), importer.mesh.vertices.end(),
                      mesh->vertices.begin(), [](const DRW_Coord& lhs,
                                                 const DRW_Coord& rhs) {
                          return lhs.x == rhs.x && lhs.y == rhs.y
                              && lhs.z == rhs.z;
                      })
        && importer.mesh.faces == mesh->faces
        && importer.mesh.edges == mesh->edges
        && importer.mesh.creases == mesh->creases;
    const DRW_Arc* decodedHatchArc = nullptr;
    const DRW_Ellipse* decodedHatchEllipse = nullptr;
    if (decodedHatch != nullptr && decodedHatch->looplist.size() == 1u
        && decodedHatch->looplist.front()->objlist.size() == 2u) {
        decodedHatchArc = dynamic_cast<const DRW_Arc*>(
            decodedHatch->looplist.front()->objlist[0].get());
        decodedHatchEllipse = dynamic_cast<const DRW_Ellipse*>(
            decodedHatch->looplist.front()->objlist[1].get());
    }
    const bool hatchOrientationValid = decodedHatchArc != nullptr
        && decodedHatchArc->isccw == 0
        && decodedHatchArc->basePoint.x == 1.0
        && decodedHatchArc->radious == 3.0
        && decodedHatchEllipse != nullptr
        && decodedHatchEllipse->isccw == 0
        && decodedHatchEllipse->basePoint.x == 4.0
        && decodedHatchEllipse->secPoint.x == 7.0
        && decodedHatchEllipse->secPoint.y == 9.0
        && decodedHatchEllipse->ratio == 0.5
        && decodedHatchEllipse->staparam == 0.25
        && decodedHatchEllipse->endparam == 1.5;
    const DRW_LWPolyline* decodedMPolygonBoundary = nullptr;
    if (decodedMPolygon != nullptr && decodedMPolygon->looplist.size() == 1u
        && !decodedMPolygon->looplist.front()->objlist.empty()) {
        decodedMPolygonBoundary = dynamic_cast<const DRW_LWPolyline*>(
            decodedMPolygon->looplist.front()->objlist.front().get());
    }
    const bool mpolygonValid = decodedMPolygon != nullptr
        && decodedMPolygon->annotatedBoundary == 1
        && decodedMPolygon->xDirX == 0.25
        && decodedMPolygon->xDirY == -0.5
        && decodedMPolygonBoundary != nullptr
        && (decodedMPolygonBoundary->flags & 1) != 0
        && decodedMPolygonBoundary->vertlist.size() == 3u
        && decodedMPolygonBoundary->vertlist[0]->x == 1.0
        && decodedMPolygonBoundary->vertlist[1]->y == 2.0
        && decodedMPolygonBoundary->vertlist[2]->x == 3.0
        && decodedMPolygonBoundary->vertlist[2]->y == 5.0;
    if (!keepOutput)
        std::filesystem::remove(output, ec);
    if (!(faceValid && solidValid && traceValid && poly3dValid
          && polygonMeshValid && polyfaceValid && ocsClassicPolylineValid
          && ocsValid && meshValid
          && hatchOrientationValid
          && mpolygonValid))
        std::cerr << "DXF topology semantic mismatch (" << encoding
                  << "): face=" << faceValid << " solid=" << solidValid
                  << " trace=" << traceValid << " poly3d=" << poly3dValid
                  << " polygon-mesh=" << polygonMeshValid
                  << " polyface=" << polyfaceValid
                  << " classic-ocs-polyline=" << ocsClassicPolylineValid
                  << " ocs=" << ocsValid
                  << " mesh=" << meshValid
                  << " hatch-orientation=" << hatchOrientationValid
                  << " mpolygon=" << mpolygonValid << '\n';
    return faceValid && solidValid && traceValid && poly3dValid
        && polygonMeshValid && polyfaceValid && ocsClassicPolylineValid
        && ocsValid && meshValid
        && hatchOrientationValid
        && mpolygonValid;
}

bool runDxfLegacyEllipseDowngrade(const std::filesystem::path& directory,
                                  bool keepOutput) {
    const std::filesystem::path output = directory /
        "libdxfrw-dxf-r12-ellipse-downgrade.dxf";
    const std::filesystem::path invalidOutput = directory /
        "libdxfrw-dxf-r12-invalid-ellipse.dxf";
    std::error_code ec;
    std::filesystem::remove(output, ec);
    std::filesystem::remove(invalidOutput, ec);

    dx_data source;
    auto addEllipse = [&source](const DRW_Coord& center,
                                const DRW_Coord& majorAxis,
                                const DRW_Coord& normal,
                                double ratio, double start, double end) {
        auto* ellipse = new DRW_Ellipse();
        ellipse->basePoint = center;
        ellipse->secPoint = majorAxis;
        ellipse->extPoint = normal;
        ellipse->ratio = ratio;
        ellipse->staparam = start;
        ellipse->endparam = end;
        source.mBlock->ent.push_back(ellipse);
    };
    const double fullTurn = 2.0 * std::acos(-1.0);
    addEllipse(DRW_Coord(10.0, 20.0, 30.0),
               DRW_Coord(2.0, 0.0, -1.5),
               DRW_Coord(0.6, 0.0, 0.8),
               0.5, 0.0, fullTurn);
    addEllipse(DRW_Coord(0.0, 0.0, 0.0),
               DRW_Coord(5.0, 0.0, 0.0),
               DRW_Coord(0.0, 0.0, 1.0),
               0.5, 0.0, fullTurn);
    addEllipse(DRW_Coord(-10.0, 5.0, 2.0),
               DRW_Coord(3.0, 0.0, 0.0),
               DRW_Coord(0.0, 0.6, 0.8),
               2.0, 0.25, 1.75);
    addEllipse(DRW_Coord(20.0, -5.0, 3.0),
               DRW_Coord(2.0, 0.0, 0.0),
               DRW_Coord(0.0, 0.6, 0.8),
               1.5, 1.75, 0.25);

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1009, false, &source,
                             false)) {
        std::cerr << "DXF R12 ellipse downgrade export failed: "
                  << output << '\n';
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }

    dx_data imported;
    dx_iface importer;
    if (!importer.fileImport(output.string(), &imported, false)) {
        std::cerr << "DXF R12 ellipse downgrade readback failed: "
                  << output << '\n';
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }

    std::vector<const DRW_Polyline*> polylines;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::POLYLINE)
            polylines.push_back(static_cast<const DRW_Polyline*>(entity));
    }
    const DRW_Polyline* tiltedFull = nullptr;
    const DRW_Polyline* planarFull = nullptr;
    std::vector<const DRW_Polyline*> tiltedArcs;
    for (const DRW_Polyline* polyline : polylines) {
        if (polyline->flags == 9)
            tiltedFull = polyline;
        else if (polyline->flags == 1)
            planarFull = polyline;
        else if (polyline->flags == 8)
            tiltedArcs.push_back(polyline);
    }

    const auto near = [](double actual, double expected) {
        return std::isfinite(actual)
            && std::abs(actual - expected) < 1e-9;
    };
    const auto pointNear = [&near](const DRW_Coord& actual,
                                   const DRW_Coord& expected) {
        return near(actual.x, expected.x) && near(actual.y, expected.y)
            && near(actual.z, expected.z);
    };
    const auto ellipsePoint = [](const DRW_Coord& center,
                                 const DRW_Coord& major,
                                 const DRW_Coord& normal,
                                 double ratio, double parameter) {
        const double normalLength = std::sqrt(
            normal.x * normal.x + normal.y * normal.y
            + normal.z * normal.z);
        const DRW_Coord n(normal.x / normalLength,
                          normal.y / normalLength,
                          normal.z / normalLength);
        const DRW_Coord minor(
            (n.y * major.z - n.z * major.y) * ratio,
            (n.z * major.x - n.x * major.z) * ratio,
            (n.x * major.y - n.y * major.x) * ratio);
        const double cosine = std::cos(parameter);
        const double sine = std::sin(parameter);
        return DRW_Coord(center.x + major.x * cosine + minor.x * sine,
                         center.y + major.y * cosine + minor.y * sine,
                         center.z + major.z * cosine + minor.z * sine);
    };

    bool tiltedFullValid = tiltedFull != nullptr
        && tiltedFull->vertlist.size() == 128
        && tiltedFull->vertlist.front() != nullptr
        && tiltedFull->vertlist.front()->flags == 32
        && pointNear(tiltedFull->vertlist[0]->basePoint,
                     DRW_Coord(12.0, 20.0, 28.5))
        && pointNear(tiltedFull->vertlist[32]->basePoint,
                     DRW_Coord(10.0, 21.25, 30.0))
        && pointNear(tiltedFull->vertlist[64]->basePoint,
                     DRW_Coord(8.0, 20.0, 31.5))
        && pointNear(tiltedFull->vertlist[96]->basePoint,
                     DRW_Coord(10.0, 18.75, 30.0));
    bool planarFullValid = planarFull != nullptr
        && planarFull->vertlist.size() == 128
        && planarFull->vertlist.front() != nullptr
        && pointNear(planarFull->vertlist.front()->basePoint,
                     DRW_Coord(5.0, 0.0, 0.0))
        && std::all_of(planarFull->vertlist.cbegin(),
                       planarFull->vertlist.cend(),
                       [](const std::shared_ptr<DRW_Vertex>& vertex) {
                           return vertex != nullptr
                               && vertex->basePoint.z == 0.0
                               && (vertex->flags & 32) == 0;
                       });
    const DRW_Polyline* tiltedArc = tiltedArcs.size() >= 1
        ? tiltedArcs[0] : nullptr;
    const DRW_Polyline* tiltedArcReverse = tiltedArcs.size() >= 2
        ? tiltedArcs[1] : nullptr;
    const DRW_Coord arcCenter(-10.0, 5.0, 2.0);
    const DRW_Coord arcMajor(3.0, 0.0, 0.0);
    const DRW_Coord arcNormal(0.0, 0.6, 0.8);
    bool tiltedArcEndpointsValid = tiltedArc != nullptr
        && tiltedArc->vertlist.size() == 32
        && tiltedArc->vertlist.front() != nullptr
        && tiltedArc->vertlist.back() != nullptr
        && tiltedArc->vertlist.front()->flags == 32
        && tiltedArc->vertlist.back()->flags == 32
        && pointNear(tiltedArc->vertlist.front()->basePoint,
                     ellipsePoint(arcCenter, arcMajor, arcNormal, 2.0, 0.25))
        && pointNear(tiltedArc->vertlist.back()->basePoint,
                     ellipsePoint(arcCenter, arcMajor, arcNormal, 2.0, 1.75))
        && std::all_of(tiltedArc->vertlist.cbegin(),
                       tiltedArc->vertlist.cend(),
                       [](const std::shared_ptr<DRW_Vertex>& vertex) {
                           return vertex != nullptr && vertex->flags == 32;
                       });
    bool tiltedArcSamplesValid = tiltedArcEndpointsValid;
    if (tiltedArcSamplesValid) {
        for (std::size_t i = 0; i < tiltedArc->vertlist.size(); ++i) {
            const double parameter = 0.25 + 1.5
                * static_cast<double>(i) / 31.0;
            if (!pointNear(tiltedArc->vertlist[i]->basePoint,
                           ellipsePoint(arcCenter, arcMajor, arcNormal,
                                        2.0, parameter))) {
                tiltedArcSamplesValid = false;
                break;
            }
        }
    }
    const bool tiltedArcValid = tiltedArcSamplesValid;
    const DRW_Coord reverseArcCenter(20.0, -5.0, 3.0);
    const DRW_Coord reverseArcMajor(2.0, 0.0, 0.0);
    const DRW_Coord reverseArcNormal(0.0, 0.6, 0.8);
    bool tiltedReverseArcValid = tiltedArcReverse != nullptr
        && tiltedArcReverse->vertlist.size() == 32
        && tiltedArcReverse->vertlist.front() != nullptr
        && tiltedArcReverse->vertlist.back() != nullptr
        && pointNear(tiltedArcReverse->vertlist.front()->basePoint,
                     ellipsePoint(reverseArcCenter, reverseArcMajor,
                                  reverseArcNormal, 1.5, 1.75))
        && pointNear(tiltedArcReverse->vertlist.back()->basePoint,
                     ellipsePoint(reverseArcCenter, reverseArcMajor,
                                  reverseArcNormal, 1.5, 0.25));
    if (tiltedReverseArcValid) {
        for (std::size_t i = 0; i < tiltedArcReverse->vertlist.size(); ++i) {
            const double parameter = 1.75 - 1.5
                * static_cast<double>(i) / 31.0;
            if (!pointNear(tiltedArcReverse->vertlist[i]->basePoint,
                           ellipsePoint(reverseArcCenter, reverseArcMajor,
                                        reverseArcNormal, 1.5, parameter))) {
                tiltedReverseArcValid = false;
                break;
            }
        }
    }

    if (!keepOutput)
        std::filesystem::remove(output, ec);
    dx_data invalidSource;
    auto* invalidEllipse = new DRW_Ellipse();
    invalidEllipse->basePoint = DRW_Coord(1.0, 2.0, 3.0);
    invalidEllipse->secPoint = DRW_Coord(4.0, 0.0, 0.0);
    invalidEllipse->extPoint = DRW_Coord(0.0, 0.0, 0.0);
    invalidSource.mBlock->ent.push_back(invalidEllipse);
    dx_iface invalidExporter;
    const bool invalidRejected = !invalidExporter.fileExport(
        invalidOutput.string(), DRW::AC1009, false, &invalidSource, false)
        && !std::filesystem::exists(invalidOutput);

    const bool polylineCountValid = polylines.size() == 4
        && tiltedArcs.size() == 2;
    if (!(polylineCountValid && tiltedFullValid && planarFullValid
          && tiltedArcValid && tiltedReverseArcValid && invalidRejected))
        std::cerr << "DXF R12 ellipse downgrade mismatch: tilted-full="
                  << tiltedFullValid << " planar-full=" << planarFullValid
                  << " tilted-arc-ratio-over-one=" << tiltedArcValid
                  << " polyline-count=" << polylines.size()
                  << " tilted-reverse-arc=" << tiltedReverseArcValid
                  << " invalid-rejected=" << invalidRejected << '\n';
    return polylineCountValid && tiltedFullValid && planarFullValid
        && tiltedArcValid && tiltedReverseArcValid && invalidRejected;
}

bool runDxfEllipseRejectsInvalidGeometry(
        const std::filesystem::path& directory) {
    const std::filesystem::path validPath = directory /
        "libdxfrw-ellipse-normal-control.dxf";
    const std::filesystem::path zeroNormalPath = directory /
        "libdxfrw-ellipse-zero-normal.dxf";
    const std::filesystem::path nonFiniteNormalPath = directory /
        "libdxfrw-ellipse-nonfinite-normal.dxf";
    const std::filesystem::path nonPerpendicularPath = directory /
        "libdxfrw-ellipse-nonperpendicular-axis.dxf";
    const std::filesystem::path zeroMajorAxisPath = directory /
        "libdxfrw-ellipse-zero-major-axis.dxf";
    const std::filesystem::path zeroRatioPath = directory /
        "libdxfrw-ellipse-zero-ratio.dxf";
    const std::filesystem::path negativeRatioPath = directory /
        "libdxfrw-ellipse-negative-ratio.dxf";
    std::error_code ec;
    for (const std::filesystem::path& path :
         {validPath, zeroNormalPath, nonFiniteNormalPath,
          nonPerpendicularPath, zeroMajorAxisPath, zeroRatioPath,
          negativeRatioPath})
        std::filesystem::remove(path, ec);

    dx_data source;
    auto* defaultEllipse = new DRW_Ellipse();
    defaultEllipse->basePoint = DRW_Coord(1.0, 2.0, 3.0);
    defaultEllipse->secPoint = DRW_Coord(4.0, 0.0, 0.0);
    defaultEllipse->ratio = 0.5;
    defaultEllipse->staparam = 0.25;
    defaultEllipse->endparam = 5.5;
    source.mBlock->ent.push_back(defaultEllipse);

    auto* obliqueEllipse = new DRW_Ellipse();
    obliqueEllipse->basePoint = DRW_Coord(-2.0, 5.0, 7.0);
    obliqueEllipse->secPoint = DRW_Coord(4.0, 0.0, -3.0);
    obliqueEllipse->extPoint = DRW_Coord(0.6, 0.0, 0.8);
    obliqueEllipse->ratio = 0.75;
    obliqueEllipse->staparam = 0.5;
    obliqueEllipse->endparam = 4.5;
    source.mBlock->ent.push_back(obliqueEllipse);

    dx_iface exporter;
    if (!exporter.fileExport(validPath.string(), DRW::AC1027, false,
                             &source, false)) {
        std::filesystem::remove(validPath, ec);
        return false;
    }
    std::ifstream input(validPath);
    if (!input)
        return false;
    std::string validDxf{std::istreambuf_iterator<char>{input},
                         std::istreambuf_iterator<char>{}};

    const auto replaceEllipseGroupValue = [](std::string& text,
                                            std::size_t ellipseIndex,
                                            int groupCode,
                                            const std::string& replacement) {
        std::istringstream inputText(text);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(inputText, line))
            lines.push_back(line);
        std::size_t currentEllipse = 0;
        bool replaced = false;
        for (std::size_t index = 0; index + 1u < lines.size(); index += 2u) {
            std::string normalized = lines[index];
            normalized.erase(std::remove_if(normalized.begin(), normalized.end(),
                [](unsigned char value) {
                    return value == ' ' || value == '\t' || value == '\r';
                }), normalized.end());
            std::string value = lines[index + 1u];
            value.erase(std::remove_if(value.begin(), value.end(),
                [](unsigned char character) {
                    return character == ' ' || character == '\t'
                        || character == '\r';
                }), value.end());
            if (normalized == "0" && value == "ELLIPSE") {
                ++currentEllipse;
                continue;
            }
            if (currentEllipse == ellipseIndex && !replaced
                && normalized == std::to_string(groupCode)) {
                lines[index + 1u] = replacement;
                replaced = true;
            }
        }
        if (!replaced)
            return false;
        std::ostringstream outputText;
        for (const std::string& outputLine : lines)
            outputText << outputLine << '\n';
        text = outputText.str();
        return true;
    };
    const auto writeText = [](const std::filesystem::path& path,
                              const std::string& text) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << text;
        return static_cast<bool>(output);
    };

    std::string zeroNormal = validDxf;
    const bool zeroMutation =
        replaceEllipseGroupValue(zeroNormal, 2u, 210, "0.0")
        && replaceEllipseGroupValue(zeroNormal, 2u, 220, "0.0")
        && replaceEllipseGroupValue(zeroNormal, 2u, 230, "0.0");
    std::string nonFiniteNormal = validDxf;
    const bool nonFiniteMutation =
        replaceEllipseGroupValue(nonFiniteNormal, 2u, 220, "nan");
    std::string nonPerpendicularAxis = validDxf;
    const bool nonPerpendicularMutation =
        replaceEllipseGroupValue(nonPerpendicularAxis, 2u, 31, "-2.0");
    std::string zeroMajorAxis = validDxf;
    const bool zeroMajorAxisMutation =
        replaceEllipseGroupValue(zeroMajorAxis, 2u, 11, "0.0")
        && replaceEllipseGroupValue(zeroMajorAxis, 2u, 21, "0.0")
        && replaceEllipseGroupValue(zeroMajorAxis, 2u, 31, "0.0");
    std::string zeroRatio = validDxf;
    const bool zeroRatioMutation =
        replaceEllipseGroupValue(zeroRatio, 2u, 40, "0.0");
    std::string negativeRatio = validDxf;
    const bool negativeRatioMutation =
        replaceEllipseGroupValue(negativeRatio, 2u, 40, "-0.5");
    if (!zeroMutation || !nonFiniteMutation
        || !nonPerpendicularMutation || !zeroMajorAxisMutation
        || !zeroRatioMutation || !negativeRatioMutation
        || !writeText(zeroNormalPath, zeroNormal)
        || !writeText(nonFiniteNormalPath, nonFiniteNormal)
        || !writeText(nonPerpendicularPath, nonPerpendicularAxis)
        || !writeText(zeroMajorAxisPath, zeroMajorAxis)
        || !writeText(zeroRatioPath, zeroRatio)
        || !writeText(negativeRatioPath, negativeRatio)) {
        for (const std::filesystem::path& path :
             {validPath, zeroNormalPath, nonFiniteNormalPath,
              nonPerpendicularPath, zeroMajorAxisPath, zeroRatioPath,
              negativeRatioPath})
            std::filesystem::remove(path, ec);
        return false;
    }

    class EllipseCaptureIface final : public dx_iface {
    public:
        std::size_t ellipseCount = 0;
        void addEllipse(const DRW_Ellipse& data) override {
            ++ellipseCount;
            dx_iface::addEllipse(data);
        }
    };
    const auto importedValidModes = [&validPath](bool applyExtrusion) {
        dx_data imported;
        EllipseCaptureIface importer;
        const bool readOk = importer.fileImport(validPath.string(), &imported,
                                                applyExtrusion);
        if (!readOk || importer.ellipseCount != 2u
            || imported.mBlock->ent.size() != 2u) {
            std::cerr << "ELLIPSE valid read: ok=" << readOk
                      << " callbacks=" << importer.ellipseCount
                      << " entities=" << imported.mBlock->ent.size()
                      << " applyExt=" << applyExtrusion << '\n';
            return false;
        }
        const DRW_Ellipse* parsedDefault = nullptr;
        const DRW_Ellipse* parsedOblique = nullptr;
        for (const DRW_Entity* entity : imported.mBlock->ent) {
            if (entity == nullptr || entity->eType != DRW::ELLIPSE)
                return false;
            const auto* ellipse = static_cast<const DRW_Ellipse*>(entity);
            if (std::abs(ellipse->basePoint.x - 1.0) <= 1.0e-9)
                parsedDefault = ellipse;
            else if (std::abs(ellipse->basePoint.x + 2.0) <= 1.0e-9)
                parsedOblique = ellipse;
        }
        const auto near = [](double value, double expected) {
            return std::isfinite(value)
                && std::abs(value - expected) <= 1.0e-9;
        };
        return parsedDefault != nullptr && parsedOblique != nullptr
            && !parsedDefault->haveExtrusion
            && parsedDefault->extPoint.x == 0.0
            && parsedDefault->extPoint.y == 0.0
            && parsedDefault->extPoint.z == 1.0
            && parsedOblique->haveExtrusion
            && near(parsedDefault->basePoint.x, 1.0)
            && near(parsedDefault->basePoint.y, 2.0)
            && near(parsedDefault->basePoint.z, 3.0)
            && near(parsedDefault->secPoint.x, 4.0)
            && near(parsedDefault->secPoint.y, 0.0)
            && near(parsedDefault->secPoint.z, 0.0)
            && near(parsedOblique->basePoint.x, -2.0)
            && near(parsedOblique->basePoint.y, 5.0)
            && near(parsedOblique->basePoint.z, 7.0)
            && near(parsedOblique->secPoint.x, 4.0)
            && near(parsedOblique->secPoint.y, 0.0)
            && near(parsedOblique->secPoint.z, -3.0)
            && near(parsedOblique->extPoint.x, 0.6)
            && near(parsedOblique->extPoint.y, 0.0)
            && near(parsedOblique->extPoint.z, 0.8);
    };
    const auto rejectedWithoutPublishingEllipse =
        [](const std::filesystem::path& path) {
        dx_data imported;
        EllipseCaptureIface importer;
        const bool importOk = importer.fileImport(path.string(), &imported,
                                                  false);
        const bool onlyEarlierValidEllipseWasPublished =
            importer.ellipseCount == 1u && imported.mBlock->ent.size() == 1u
            && imported.mBlock->ent.front() != nullptr
            && imported.mBlock->ent.front()->eType == DRW::ELLIPSE;
        if (importOk || !onlyEarlierValidEllipseWasPublished) {
            std::cerr << "ELLIPSE malformed read: path=" << path.filename()
                      << " ok=" << importOk
                      << " callbacks=" << importer.ellipseCount;
            std::cerr << '\n';
        }
        return !importOk && onlyEarlierValidEllipseWasPublished;
    };

    struct WriterNegative {
        const char* name;
        std::function<void(DRW_Ellipse&)> corrupt;
    };
    const std::vector<WriterNegative> writerNegatives = {
        {"zero-normal", [](DRW_Ellipse& ellipse) {
            ellipse.extPoint = DRW_Coord(0.0, 0.0, 0.0);
        }},
        {"nonfinite-center", [](DRW_Ellipse& ellipse) {
            ellipse.basePoint.x = std::numeric_limits<double>::quiet_NaN();
        }},
        {"nonfinite-major-axis", [](DRW_Ellipse& ellipse) {
            ellipse.secPoint.z = std::numeric_limits<double>::infinity();
        }},
        {"zero-major-axis", [](DRW_Ellipse& ellipse) {
            ellipse.secPoint = DRW_Coord(0.0, 0.0, 0.0);
        }},
        {"nonperpendicular-axis", [](DRW_Ellipse& ellipse) {
            ellipse.extPoint = DRW_Coord(0.6, 0.0, 0.8);
        }},
        {"zero-ratio", [](DRW_Ellipse& ellipse) {
            ellipse.ratio = 0.0;
        }},
        {"negative-ratio", [](DRW_Ellipse& ellipse) {
            ellipse.ratio = -0.5;
        }}
    };
    bool writerNegativesPass = true;
    for (const WriterNegative& invalid : writerNegatives) {
        for (const auto& testCase :
             {std::make_pair(DRW::AC1027, false),
              std::make_pair(DRW::AC1027, true),
              std::make_pair(DRW::AC1009, false)}) {
            const std::string encoding = testCase.second ? "binary" : "ascii";
            const std::filesystem::path output = directory /
                (std::string("libdxfrw-ellipse-writer-") + invalid.name + "-"
                 + encoding + "-"
                 + std::to_string(static_cast<int>(testCase.first)) + ".dxf");
            std::filesystem::remove(output, ec);
            dx_data invalidSource;
            auto* invalidEllipse = new DRW_Ellipse();
            invalidEllipse->basePoint = DRW_Coord(1.0, 2.0, 3.0);
            invalidEllipse->secPoint = DRW_Coord(4.0, 0.0, 0.0);
            invalidEllipse->ratio = 0.5;
            invalidEllipse->staparam = 0.25;
            invalidEllipse->endparam = 5.5;
            invalid.corrupt(*invalidEllipse);
            invalidSource.mBlock->ent.push_back(invalidEllipse);
            dx_iface invalidExporter;
            const bool rejected = !invalidExporter.fileExport(
                output.string(), testCase.first, testCase.second,
                &invalidSource, false)
                && !std::filesystem::exists(output);
            if (!rejected) {
                std::cerr << "ELLIPSE writer accepted invalid geometry: "
                          << invalid.name << " version="
                          << static_cast<int>(testCase.first)
                          << " binary=" << testCase.second << '\n';
            }
            writerNegativesPass = writerNegativesPass && rejected;
            std::filesystem::remove(output, ec);
        }
    }

    const bool validDefaultMode = importedValidModes(false);
    const bool validLegacyMode = importedValidModes(true);
    const bool zeroRejected =
        rejectedWithoutPublishingEllipse(zeroNormalPath);
    const bool nonFiniteRejected =
        rejectedWithoutPublishingEllipse(nonFiniteNormalPath);
    const bool nonPerpendicularRejected =
        rejectedWithoutPublishingEllipse(nonPerpendicularPath);
    const bool zeroMajorAxisRejected =
        rejectedWithoutPublishingEllipse(zeroMajorAxisPath);
    const bool zeroRatioRejected =
        rejectedWithoutPublishingEllipse(zeroRatioPath);
    const bool negativeRatioRejected =
        rejectedWithoutPublishingEllipse(negativeRatioPath);
    const bool result = validDefaultMode && validLegacyMode && zeroRejected
        && nonFiniteRejected && nonPerpendicularRejected
        && zeroMajorAxisRejected && zeroRatioRejected && negativeRatioRejected
        && writerNegativesPass;
    if (!result) {
        std::cerr << "ELLIPSE extrusion gate: default=" << validDefaultMode
                  << " legacy=" << validLegacyMode << " zero="
                  << zeroRejected << " non-finite=" << nonFiniteRejected
                  << " non-perpendicular=" << nonPerpendicularRejected
                  << " zero-axis=" << zeroMajorAxisRejected
                  << " zero-ratio=" << zeroRatioRejected
                  << " negative-ratio=" << negativeRatioRejected
                  << " writer=" << writerNegativesPass << '\n';
    }
    for (const std::filesystem::path& path :
         {validPath, zeroNormalPath, nonFiniteNormalPath,
          nonPerpendicularPath, zeroMajorAxisPath, zeroRatioPath,
          negativeRatioPath})
        std::filesystem::remove(path, ec);
    return result;
}

bool runDxfArcCircleThicknessRoundTrip(
        bool binary, const std::filesystem::path& directory) {
    const std::filesystem::path output = directory /
        (binary ? "libdxfrw-arc-circle-thickness-binary.dxf"
                : "libdxfrw-arc-circle-thickness-ascii.dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    dx_data source;
    auto* circle = new DRW_Circle();
    circle->basePoint = DRW_Coord(2.0, 3.0, 4.0);
    circle->extPoint = DRW_Coord(0.6, 0.0, 0.8);
    circle->radious = 2.5;
    circle->thickness = 3.75;
    source.mBlock->ent.push_back(circle);

    auto* arc = new DRW_Arc();
    arc->basePoint = DRW_Coord(2.0, 3.0, 4.0);
    arc->extPoint = DRW_Coord(0.6, 0.0, 0.8);
    arc->radious = 1.75;
    arc->thickness = 1.25;
    arc->staangle = 0.25;
    arc->endangle = 1.5;
    source.mBlock->ent.push_back(arc);

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary,
                             &source, false)) {
        std::filesystem::remove(output, ec);
        return false;
    }

    class ArcCircleCapture final : public dx_iface {
    public:
        dx_data data;
        DRW_Circle circle;
        DRW_Arc arc;
        bool hasCircle = false;
        bool hasArc = false;

        ArcCircleCapture() {
            cData = &data;
            currentBlock = data.mBlock;
        }

        void addCircle(const DRW_Circle& value) override {
            circle = value;
            hasCircle = true;
        }

        void addArc(const DRW_Arc& value) override {
            arc = value;
            hasArc = true;
        }
    };

    const auto readAndCheck = [&](bool applyExtrusion) {
        const std::string outputName = output.string();
        dxfRW reader(outputName.c_str());
        ArcCircleCapture capture;
        const bool readOk = reader.read(&capture, applyExtrusion);
        if (!readOk || !capture.hasCircle || !capture.hasArc) {
            std::cerr << "ARC/CIRCLE thickness read failed: binary=" << binary
                      << " applyExt=" << applyExtrusion << " ok=" << readOk
                      << " circle=" << capture.hasCircle
                      << " arc=" << capture.hasArc << '\n';
            return false;
        }
        const DRW_Circle* parsedCircle = &capture.circle;
        const DRW_Arc* parsedArc = &capture.arc;
        const auto near = [](double actual, double expected) {
            return std::isfinite(actual)
                && std::abs(actual - expected) <= 1.0e-9;
        };
        const auto pointNear = [&near](const DRW_Coord& actual,
                                       const DRW_Coord& expected) {
            return near(actual.x, expected.x)
                && near(actual.y, expected.y)
                && near(actual.z, expected.z);
        };
        const DRW_Coord expectedCenter = applyExtrusion
            ? DRW_Coord(0.0, 2.0, 5.0)
            : DRW_Coord(2.0, 3.0, 4.0);
        const bool fieldsMatch = parsedCircle != nullptr && parsedArc != nullptr
            && near(parsedCircle->thickness, 3.75)
            && near(parsedArc->thickness, 1.25)
            && near(parsedCircle->radious, 2.5)
            && near(parsedArc->radious, 1.75)
            && pointNear(parsedCircle->basePoint, expectedCenter)
            && pointNear(parsedArc->basePoint, expectedCenter)
            && pointNear(parsedCircle->extPoint, DRW_Coord(0.6, 0.0, 0.8))
            && pointNear(parsedArc->extPoint, DRW_Coord(0.6, 0.0, 0.8))
            && near(parsedArc->staangle, 0.25)
            && near(parsedArc->endangle, 1.5);
        if (!fieldsMatch) {
            std::cerr << "ARC/CIRCLE thickness fields differ: binary=" << binary
                      << " applyExt=" << applyExtrusion
                      << " circle-thickness="
                      << (parsedCircle ? parsedCircle->thickness : -999.0)
                      << " arc-thickness="
                      << (parsedArc ? parsedArc->thickness : -999.0)
                      << " circle-center-x="
                      << (parsedCircle ? parsedCircle->basePoint.x : -999.0)
                      << " circle-center-y="
                      << (parsedCircle ? parsedCircle->basePoint.y : -999.0)
                      << " circle-center-z="
                      << (parsedCircle ? parsedCircle->basePoint.z : -999.0)
                      << " arc-center-x="
                      << (parsedArc ? parsedArc->basePoint.x : -999.0)
                      << " arc-center-y="
                      << (parsedArc ? parsedArc->basePoint.y : -999.0)
                      << " arc-center-z="
                      << (parsedArc ? parsedArc->basePoint.z : -999.0)
                      << '\n';
        }
        return fieldsMatch;
    };

    const bool valid = readAndCheck(false) && readAndCheck(true);
    std::filesystem::remove(output, ec);
    return valid;
}

bool runDxfRejectsInvalidFaceFlags(bool binary,
                                   const std::filesystem::path& directory) {
    const std::string encoding = binary ? "binary" : "ascii";
    const std::array<int, 3> invalidFlags{{-1, 16, 65536}};
    bool allRejected = true;
    for (const int flags : invalidFlags) {
        const std::filesystem::path output = directory /
            ("libdxfrw-invalid-3dface-flags-" + encoding + "-"
             + std::to_string(flags) + ".dxf");
        std::error_code ec;
        std::filesystem::remove(output, ec);

        dx_data source;
        auto* face = new DRW_3Dface();
        face->basePoint = DRW_Coord(1.0, 2.0, 3.0);
        face->secPoint = DRW_Coord(4.0, 5.0, 6.0);
        face->thirdPoint = DRW_Coord(7.0, 8.0, 9.0);
        face->fourPoint = DRW_Coord(10.0, 11.0, 12.0);
        face->invisibleflag = flags;
        source.mBlock->ent.push_back(face);

        dx_iface exporter;
        const bool writeSucceeded = exporter.fileExport(
            output.string(), DRW::AC1027, binary, &source, false);
        if (writeSucceeded || std::filesystem::exists(output))
            allRejected = false;
        std::filesystem::remove(output, ec);
    }
    return allRejected;
}

DRW_Coord referenceInsertTransform(const DRW_Coord& point,
                                   const DRW_Coord& blockBase,
                                   const DRW_Insert& insert) {
    DRW_Coord normal = insert.extPoint;
    const double normalLength = std::sqrt(normal.x * normal.x
        + normal.y * normal.y + normal.z * normal.z);
    if (!std::isfinite(normalLength) || normalLength == 0.0)
        return DRW_Coord(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0);
    normal.x /= normalLength;
    normal.y /= normalLength;
    normal.z /= normalLength;

    DRW_Coord axisX;
    if (std::abs(normal.x) < 1.0 / 64.0
        && std::abs(normal.y) < 1.0 / 64.0) {
        // Autodesk's arbitrary-axis rule uses Wy x N close to world Z.
        axisX = DRW_Coord(normal.z, 0.0, -normal.x);
    } else {
        // Otherwise use Wz x N.
        axisX = DRW_Coord(-normal.y, normal.x, 0.0);
    }
    const double axisLength = std::sqrt(axisX.x * axisX.x
        + axisX.y * axisX.y + axisX.z * axisX.z);
    if (!std::isfinite(axisLength) || axisLength == 0.0)
        return DRW_Coord(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0);
    axisX.x /= axisLength;
    axisX.y /= axisLength;
    axisX.z /= axisLength;
    const DRW_Coord axisY(
        normal.y * axisX.z - normal.z * axisX.y,
        normal.z * axisX.x - normal.x * axisX.z,
        normal.x * axisX.y - normal.y * axisX.x);

    const double sx = (point.x - blockBase.x) * insert.xscale;
    const double sy = (point.y - blockBase.y) * insert.yscale;
    const double sz = (point.z - blockBase.z) * insert.zscale;
    const double cosine = std::cos(insert.angle);
    const double sine = std::sin(insert.angle);
    const double rx = sx * cosine - sy * sine;
    const double ry = sx * sine + sy * cosine;
    return DRW_Coord(
        insert.basePoint.x * axisX.x + insert.basePoint.y * axisY.x
            + insert.basePoint.z * normal.x + rx * axisX.x + ry * axisY.x
            + sz * normal.x,
        insert.basePoint.x * axisX.y + insert.basePoint.y * axisY.y
            + insert.basePoint.z * normal.y + rx * axisX.y + ry * axisY.y
            + sz * normal.y,
        insert.basePoint.x * axisX.z + insert.basePoint.y * axisY.z
            + insert.basePoint.z * normal.z + rx * axisX.z + ry * axisY.z
            + sz * normal.z);
}

bool nearDxfValue(double value, double expected) {
    return std::isfinite(value)
        && std::abs(value - expected) <= 1.0e-9;
}

class SplineBodyParserProbe : public DRW_Spline {
public:
    bool parseBody(DRW::Version version, dwgBuffer& buffer,
                   std::uint64_t bodyEndBit) {
        if (bodyEndBit > std::numeric_limits<std::uint32_t>::max())
            return false;
        objSize = static_cast<std::uint32_t>(bodyEndBit);
        dwgDataEndBit = bodyEndBit;
        return parseDwgSplineBody(version, &buffer);
    }
};

bool runR2013SplineBitLongBoundaryTest() {
    // ODA v5.4.1 §20.4.40 defines both R2013+ fields as BL. Build this
    // scenario-2 body directly from bitstream primitives rather than through
    // DRW_Spline::encodeDwgSplineBody(), then place a sentinel at the exact
    // entity-body boundary to detect a field-width/cursor regression.
    constexpr std::int32_t splineFlags1 = 13; // fit points, use knot param, closed
    constexpr std::int32_t knotParameter = 2;
    constexpr std::int32_t sentinel = 777;
    const DRW_Coord startTangent(0.0, 0.0, 0.0);
    const DRW_Coord endTangent(0.0, 0.0, 0.0);
    const DRW_Coord firstFit(1.0, 2.0, 0.0);
    const DRW_Coord secondFit(3.0, 0.0, 4.0);

    dwgBufferW encoded;
    encoded.putBitLong(2); // scenario
    encoded.putBitLong(splineFlags1);
    encoded.putBitLong(knotParameter);
    encoded.putBitLong(1); // degree
    encoded.putBitDouble(0.125); // fit tolerance
    encoded.put3BitDouble(startTangent);
    encoded.put3BitDouble(endTangent);
    encoded.putBitLong(2); // fit-point count
    encoded.put3BitDouble(firstFit);
    encoded.put3BitDouble(secondFit);
    if (!encoded.isGood())
        return false;

    const std::uint64_t encodedBits =
        static_cast<std::uint64_t>(encoded.size()) * 8u;
    const std::uint64_t bodyEndBit = encoded.bitPos() == 0
        ? encodedBits
        : encodedBits - (8u - encoded.bitPos());
    if (bodyEndBit == 0
        || bodyEndBit > std::numeric_limits<std::uint32_t>::max())
        return false;
    encoded.putBitLong(sentinel);
    if (!encoded.isGood())
        return false;

    dwgBuffer input(encoded.data().data(), encoded.data().size());
    SplineBodyParserProbe parsed;
    if (!parsed.parseBody(DRW::AC1027, input, bodyEndBit))
        return false;

    const bool fieldsMatch = parsed.m_scenario == 2
        && parsed.m_splineFlags1 == splineFlags1
        && parsed.m_knotParam == knotParameter
        && parsed.degree == 1 && parsed.flags == 1
        && parsed.nfit == 2 && parsed.tolfit == 0.125
        && parsed.tgStart.x == startTangent.x
        && parsed.tgStart.y == startTangent.y
        && parsed.tgStart.z == startTangent.z
        && parsed.tgEnd.x == endTangent.x
        && parsed.tgEnd.y == endTangent.y
        && parsed.tgEnd.z == endTangent.z
        && parsed.fitlist.size() == 2
        && parsed.fitlist[0] != nullptr
        && parsed.fitlist[0]->x == firstFit.x
        && parsed.fitlist[0]->y == firstFit.y
        && parsed.fitlist[0]->z == firstFit.z
        && parsed.fitlist[1] != nullptr
        && parsed.fitlist[1]->x == secondFit.x
        && parsed.fitlist[1]->y == secondFit.y
        && parsed.fitlist[1]->z == secondFit.z;
    return fieldsMatch && input.isGood() && input.getBitLong() == sentinel
        && input.isGood();
}

bool runAc1024SplineVersionBoundaryTest() {
    // The R2013+ BL fields must not be consumed by AC1024. The three older
    // scenario flags remain separate B fields immediately after degree.
    constexpr std::int32_t sentinel = 509;
    dwgBufferW encoded;
    encoded.putBitLong(1); // control-point scenario
    encoded.putBitLong(1); // degree
    encoded.putBoolBit(true);  // rational
    encoded.putBoolBit(true);  // closed
    encoded.putBoolBit(false); // periodic
    encoded.putBitDouble(0.125); // knot tolerance
    encoded.putBitDouble(0.25);  // control-point tolerance
    encoded.putBitLong(4); // knot count = control count + degree + 1
    encoded.putBitLong(2); // control count
    encoded.putBoolBit(true); // weights present
    encoded.putBitDouble(0.0);
    encoded.putBitDouble(0.0);
    encoded.putBitDouble(1.0);
    encoded.putBitDouble(1.0);
    encoded.put3BitDouble(DRW_Coord(0.0, 0.0, 0.0));
    encoded.putBitDouble(1.0);
    encoded.put3BitDouble(DRW_Coord(2.0, 0.0, 0.0));
    encoded.putBitDouble(1.5);
    if (!encoded.isGood())
        return false;

    const std::uint64_t encodedBits =
        static_cast<std::uint64_t>(encoded.size()) * 8u;
    const std::uint64_t bodyEndBit = encoded.bitPos() == 0
        ? encodedBits
        : encodedBits - (8u - encoded.bitPos());
    if (bodyEndBit == 0
        || bodyEndBit > std::numeric_limits<std::uint32_t>::max())
        return false;
    encoded.putBitLong(sentinel);
    if (!encoded.isGood())
        return false;

    dwgBuffer input(encoded.data().data(), encoded.data().size());
    SplineBodyParserProbe parsed;
    if (!parsed.parseBody(DRW::AC1024, input, bodyEndBit))
        return false;

    const bool fieldsMatch = parsed.m_scenario == 1
        && parsed.m_splineFlags1 == 0 && parsed.m_knotParam == 15
        && parsed.degree == 1 && parsed.flags == 5
        && parsed.nknots == 4 && parsed.ncontrol == 2
        && parsed.nfit == 0 && parsed.tolknot == 0.125
        && parsed.tolcontrol == 0.25
        && parsed.knotslist == std::vector<double>({0.0, 0.0, 1.0, 1.0})
        && parsed.weightlist == std::vector<double>({1.0, 1.5})
        && parsed.controllist.size() == 2
        && parsed.controllist[0] != nullptr
        && parsed.controllist[0]->x == 0.0
        && parsed.controllist[0]->y == 0.0
        && parsed.controllist[0]->z == 0.0
        && parsed.controllist[1] != nullptr
        && parsed.controllist[1]->x == 2.0
        && parsed.controllist[1]->y == 0.0
        && parsed.controllist[1]->z == 0.0;
    return fieldsMatch && input.isGood() && input.getBitLong() == sentinel
        && input.isGood();
}

bool runDxfInsertTransformRoundTrip(bool binary,
                                    const std::filesystem::path& directory,
                                    bool keepOutput) {
    const std::string encoding = binary ? "binary" : "ascii";
    const std::filesystem::path output = directory /
        ("libdxfrw-dxf-insert-transform-" + encoding + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);

    dx_data source;
    auto* innerBlock = new dx_ifaceBlock();
    innerBlock->name = "LOCAL_INNER_3D";
    innerBlock->basePoint = DRW_Coord(1.0, 2.0, 3.0);
    auto* blockPoint = new DRW_Point();
    blockPoint->basePoint = DRW_Coord(4.0, 6.0, 8.0);
    innerBlock->ent.push_back(blockPoint);
    source.blocks.push_back(innerBlock);

    auto* outerBlock = new dx_ifaceBlock();
    outerBlock->name = "LOCAL_OUTER_3D";
    outerBlock->basePoint = DRW_Coord(2.0, -1.0, 3.0);
    auto* nested = new DRW_Insert();
    nested->name = innerBlock->name;
    nested->basePoint = DRW_Coord(6.0, 7.0, 8.0);
    nested->xscale = 2.0;
    nested->yscale = 0.5;
    nested->zscale = 1.0;
    nested->angle = 1.57079632679489661923;
    outerBlock->ent.push_back(nested);
    source.blocks.push_back(outerBlock);

    auto* root = new DRW_Insert();
    root->name = outerBlock->name;
    root->basePoint = DRW_Coord(10.0, 20.0, 30.0);
    root->xscale = -2.0;
    root->yscale = 3.0;
    root->zscale = 0.5;
    root->angle = 1.57079632679489661923;
    const double rootNormalComponent = std::sqrt(0.5);
    root->extPoint = DRW_Coord(0.0, rootNormalComponent,
                               rootNormalComponent);
    auto attribute = std::make_shared<DRW_Attrib>();
    attribute->tag = "LOCAL_3D_TAG";
    attribute->text = "LOCAL_3D_VALUE";
    attribute->basePoint = DRW_Coord(11.0, 12.0, 13.0);
    attribute->height = 1.25;
    root->attlist.push_back(attribute);
    source.mBlock->ent.push_back(root);

    auto* array = new DRW_Insert();
    array->name = outerBlock->name;
    array->basePoint = DRW_Coord(-3.0, 4.0, 5.0);
    array->xscale = 1.5;
    array->yscale = -0.5;
    array->zscale = 2.0;
    array->angle = -1.04719755119659774615;
    array->extPoint = DRW_Coord(0.0, 0.0, -1.0);
    array->colcount = 3;
    array->rowcount = 2;
    array->colspace = 4.25;
    array->rowspace = 6.5;
    source.mBlock->ent.push_back(array);

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, binary, &source,
                             false)) {
        std::cerr << "DXF INSERT export failed (" << encoding << "): "
                  << output << '\n';
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }
    dx_data imported;
    dx_iface importer;
    if (!importer.fileImport(output.string(), &imported, false)) {
        std::cerr << "DXF INSERT import failed (" << encoding << "): "
                  << output << '\n';
        if (!keepOutput)
            std::filesystem::remove(output, ec);
        return false;
    }

    const dx_ifaceBlock* decodedInner = nullptr;
    const dx_ifaceBlock* decodedOuter = nullptr;
    for (const dx_ifaceBlock* block : imported.blocks) {
        if (block->name == "LOCAL_INNER_3D")
            decodedInner = block;
        else if (block->name == "LOCAL_OUTER_3D")
            decodedOuter = block;
    }
    const DRW_Point* decodedPoint = nullptr;
    if (decodedInner != nullptr) {
        for (const DRW_Entity* entity : decodedInner->ent) {
            if (entity != nullptr && entity->eType == DRW::POINT)
                decodedPoint = static_cast<const DRW_Point*>(entity);
        }
    }
    const DRW_Insert* decodedNested = nullptr;
    if (decodedOuter != nullptr) {
        for (const DRW_Entity* entity : decodedOuter->ent) {
            if (entity != nullptr && entity->eType == DRW::INSERT)
                decodedNested = static_cast<const DRW_Insert*>(entity);
        }
    }
    const DRW_Insert* decodedRoot = nullptr;
    const DRW_Insert* decodedArray = nullptr;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr || entity->eType != DRW::INSERT)
            continue;
        const auto* insert = static_cast<const DRW_Insert*>(entity);
        if (insert->isMInsert())
            decodedArray = insert;
        else
            decodedRoot = insert;
    }

    const bool blockFieldsValid = decodedInner != nullptr
        && decodedOuter != nullptr && decodedPoint != nullptr
        && decodedInner->basePoint.x == 1.0
        && decodedInner->basePoint.y == 2.0
        && decodedInner->basePoint.z == 3.0
        && decodedPoint->basePoint.x == 4.0
        && decodedPoint->basePoint.y == 6.0
        && decodedPoint->basePoint.z == 8.0
        && decodedOuter->basePoint.x == 2.0
        && decodedOuter->basePoint.y == -1.0
        && decodedOuter->basePoint.z == 3.0;
    const bool nestedFieldsValid = decodedNested != nullptr
        && decodedNested->name == "LOCAL_INNER_3D"
        && decodedNested->basePoint.x == 6.0
        && decodedNested->basePoint.y == 7.0
        && decodedNested->basePoint.z == 8.0
        && decodedNested->xscale == 2.0 && decodedNested->yscale == 0.5
        && decodedNested->zscale == 1.0
        && nearDxfValue(decodedNested->angle, 1.57079632679489661923);
    const bool rootFieldsValid = decodedRoot != nullptr
        && decodedRoot->name == "LOCAL_OUTER_3D"
        && decodedRoot->basePoint.x == 10.0
        && decodedRoot->basePoint.y == 20.0
        && decodedRoot->basePoint.z == 30.0
        && decodedRoot->xscale == -2.0 && decodedRoot->yscale == 3.0
        && decodedRoot->zscale == 0.5
        && nearDxfValue(decodedRoot->angle, 1.57079632679489661923)
        && nearDxfValue(decodedRoot->extPoint.y, rootNormalComponent)
        && nearDxfValue(decodedRoot->extPoint.z, rootNormalComponent)
        && decodedRoot->attlist.size() == 1
        && decodedRoot->attlist.front() != nullptr
        && decodedRoot->attlist.front()->tag == "LOCAL_3D_TAG"
        && decodedRoot->attlist.front()->text == "LOCAL_3D_VALUE"
        && decodedRoot->attlist.front()->basePoint.x == 11.0
        && decodedRoot->attlist.front()->basePoint.y == 12.0
        && decodedRoot->attlist.front()->basePoint.z == 13.0;
    const bool arrayFieldsValid = decodedArray != nullptr
        && decodedArray->name == "LOCAL_OUTER_3D"
        && decodedArray->basePoint.x == -3.0
        && decodedArray->basePoint.y == 4.0
        && decodedArray->basePoint.z == 5.0
        && decodedArray->xscale == 1.5 && decodedArray->yscale == -0.5
        && decodedArray->zscale == 2.0
        && nearDxfValue(decodedArray->angle, -1.04719755119659774615)
        && decodedArray->extPoint.z == -1.0
        && decodedArray->colcount == 3 && decodedArray->rowcount == 2
        && decodedArray->colspace == 4.25 && decodedArray->rowspace == 6.5;

    bool oracleValid = false;
    bool arrayOracleValid = false;
    if (blockFieldsValid && nestedFieldsValid && rootFieldsValid) {
        const DRW_Coord inOuter = referenceInsertTransform(
            decodedPoint->basePoint, decodedInner->basePoint, *decodedNested);
        const DRW_Coord inWorld = referenceInsertTransform(
            inOuter, decodedOuter->basePoint, *decodedRoot);
        oracleValid = nearDxfValue(inWorld.x, 32.0)
            && nearDxfValue(inWorld.y, 19.0 / std::sqrt(2.0))
            && nearDxfValue(inWorld.z, 51.0 / std::sqrt(2.0));
        if (decodedArray != nullptr) {
            DRW_Insert arrayCell = *decodedArray;
            const double columnOffset = 2.0 * decodedArray->colspace;
            const double rowOffset = decodedArray->rowspace;
            const double cosine = std::cos(decodedArray->angle);
            const double sine = std::sin(decodedArray->angle);
            // MINSERT grid offsets are in OCS, rotate with the array, and are
            // not scaled by the referenced block's scale factors.
            arrayCell.basePoint.x += columnOffset * cosine - rowOffset * sine;
            arrayCell.basePoint.y += columnOffset * sine + rowOffset * cosine;
            const DRW_Coord arrayWorldPoint = referenceInsertTransform(
                inOuter, decodedOuter->basePoint, arrayCell);
            arrayOracleValid = nearDxfValue(
                    arrayWorldPoint.x, (-11.0 + std::sqrt(3.0)) / 4.0)
                && nearDxfValue(arrayWorldPoint.y,
                                (15.0 - 23.0 * std::sqrt(3.0)) / 4.0)
                && nearDxfValue(arrayWorldPoint.z, -25.0);
        }
    }
    if (!keepOutput)
        std::filesystem::remove(output, ec);
    const bool valid = blockFieldsValid && nestedFieldsValid && rootFieldsValid
        && arrayFieldsValid && oracleValid && arrayOracleValid;
    if (!valid)
        std::cerr << "DXF INSERT semantic mismatch (" << encoding
                  << "): block=" << blockFieldsValid
                  << " nested=" << nestedFieldsValid << " root=" << rootFieldsValid
                  << " array=" << arrayFieldsValid << " oracle=" << oracleValid
                  << " arrayOracle=" << arrayOracleValid
                  << '\n';
    return valid;
}

bool runDxfInsertRejectsInvalidPayload() {
    const std::filesystem::path output =
        std::filesystem::temp_directory_path()
        / "libdxfrw-dxf-insert-invalid-payload.dxf";
    const auto reject = [&](const DRW_Insert& invalid,
                            const char* suffix) {
        std::error_code ec;
        const std::filesystem::path target = output.parent_path()
            / (output.stem().string() + suffix + output.extension().string());
        std::filesystem::remove(target, ec);
        dx_data source;
        source.mBlock->ent.push_back(new DRW_Insert(invalid));
        dx_iface exporter;
        const bool rejected = !exporter.fileExport(target.string(),
            DRW::AC1027, false, &source, false);
        std::filesystem::remove(target, ec);
        return rejected;
    };
    DRW_Insert invalidCount;
    invalidCount.name = "LOCAL_BAD_MINSERT";
    invalidCount.colcount = std::numeric_limits<std::int16_t>::max() + 1;
    DRW_Insert invalidPoint;
    invalidPoint.name = "LOCAL_BAD_INSERT_POINT";
    invalidPoint.basePoint.x = std::numeric_limits<double>::quiet_NaN();
    return reject(invalidCount, "-count") && reject(invalidPoint, "-point");
}

} // namespace

int main(int argc, char** argv) {
    int failures = 0;
    const std::vector<DRW::Version> versions {
        DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032};
    std::filesystem::path directory = std::filesystem::temp_directory_path();
    bool keepOutputs = false;
    if (argc == 3
        && std::string(argv[1]) == "--verify-acsh-prefix-sample") {
        const bool passed = runAcshPrefixSampleQualification(argv[2]);
        std::cout << "ACSH pinned-sample prefix qualification: "
                  << (passed ? "PASS" : "FAIL") << '\n';
        return passed ? 0 : 1;
    } else if (argc == 4
        && std::string(argv[1]) == "--verify-dynamicblock-typed-pair") {
        const bool passed = runDynamicBlockTypedDxfPairQualification(
            argv[2], argv[3]);
        std::cout << "dynamic-block typed-DXF pair qualification: "
                  << (passed ? "PASS" : "FAIL") << '\n';
        return passed ? 0 : 1;
    } else if (argc == 3 && std::string(argv[1]) == "--keep-dir") {
        directory = argv[2];
        std::filesystem::create_directories(directory);
        keepOutputs = true;
    } else if (argc != 1) {
        std::cerr << "usage: " << argv[0]
                  << " [--keep-dir DIRECTORY]"
                     " [--verify-acsh-prefix-sample DWG]"
                     " [--verify-dynamicblock-typed-pair DWG DXF]\n";
        return 2;
    }
    expect(runR2013SplineBitLongBoundaryTest(),
           "synthetic AC1027 SPLINE BL fields preserve body-boundary alignment",
           failures);
    expect(runAc1024SplineVersionBoundaryTest(),
           "synthetic AC1024 SPLINE excludes R2013+ fields and preserves alignment",
           failures);
    std::error_code ec;
    for (const DRW::Version version : versions) {
        const std::filesystem::path output = directory /
            ("libdxfrw-local-roundtrip-" +
             std::to_string(static_cast<int>(version)) + ".dwg");
        std::filesystem::remove(output, ec);

        dwgRW writer(output.string().c_str());
        LocalDwgInterface writeIface(&writer, version);
        const bool writeOk = writer.write(&writeIface, version, true);
        const std::string suffix =
            " version " + std::to_string(static_cast<int>(version));
        expect(writeOk, ("local DWG writer succeeds" + suffix).c_str(), failures);
        expect(writeIface.wroteLine(),
               ("local DWG writer emitted a line" + suffix).c_str(), failures);
        expect(writeIface.wroteSimpleEntities(),
               ("local DWG writer emitted simple entity set" + suffix).c_str(),
               failures);
        expect(writeIface.wroteAdvancedEntities(),
               ("local DWG writer emitted advanced entity set" + suffix).c_str(),
               failures);
        expect(writeIface.wroteOldPolyline(),
               ("local DWG writer emitted POLYLINE" + suffix).c_str(), failures);
        expect(writeIface.wroteSpline(),
               ("local DWG writer emitted SPLINE" + suffix).c_str(), failures);
        expect(writeIface.wroteHelix(),
               ("local DWG writer emitted HELIX" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedHelix(),
               ("local DWG writer rejected malformed HELIX transaction" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteCamera()
                   : !writeIface.wroteCamera(),
               ("local DWG CAMERA capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedCamera(),
               ("local DWG writer rejected malformed CAMERA transaction" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1027
                   ? writeIface.wroteGeoPositionMarker()
                   : !writeIface.wroteGeoPositionMarker(),
               ("local DWG GEOPOSITIONMARKER capability gate" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedGeoPositionMarker(),
               ("local DWG writer rejected malformed GEOPOSITIONMARKER transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteShape()
                   : !writeIface.wroteShape(),
               ("local DWG SHAPE capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedShape(),
               ("local DWG writer rejected malformed SHAPE transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteMLine()
                   : !writeIface.wroteMLine(),
               ("local DWG MLINE capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedMLine(),
               ("local DWG writer rejected malformed MLINE transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteLight()
                   : !writeIface.wroteLight(),
               ("local DWG LIGHT capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedLight(),
               ("local DWG writer rejected malformed LIGHT transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteMesh()
                   : !writeIface.wroteMesh(),
               ("local DWG MESH capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedMesh(),
               ("local DWG writer rejected malformed MESH transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteWipeout()
                   : !writeIface.wroteWipeout(),
               ("local DWG WIPEOUT capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedWipeout(),
               ("local DWG writer rejected malformed WIPEOUT transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteNavisworksModel()
                   : !writeIface.wroteNavisworksModel(),
               ("local DWG NAVISWORKSMODEL capability gate"
                + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedNavisworksModel(),
               ("local DWG writer rejected malformed NAVISWORKSMODEL transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteUnderlay()
                   : !writeIface.wroteUnderlay(),
               ("local DWG UNDERLAY capability gate" + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteDgnUnderlayEntity()
                   : !writeIface.wroteDgnUnderlayEntity(),
               ("local DWG DGNUNDERLAY capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteDwfUnderlayEntity()
                   : !writeIface.wroteDwfUnderlayEntity(),
               ("local DWG DWFUNDERLAY capability gate" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedUnderlay(),
               ("local DWG writer rejected malformed UNDERLAY transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteSurfaceSet()
                   : !writeIface.wroteSurfaceSet(),
               ("local DWG SURFACE variant capability gate" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedSurface(),
               ("local DWG writer rejected malformed SURFACE transaction"
                + suffix).c_str(), failures);
        expect(version > DRW::AC1018
                   ? writeIface.wrotePointCloud()
                   : !writeIface.wrotePointCloud(),
               ("local DWG POINTCLOUD capability gate" + suffix).c_str(),
               failures);
        expect(version > DRW::AC1024
                   ? writeIface.wrotePointCloudEx()
                   : !writeIface.wrotePointCloudEx(),
               ("local DWG POINTCLOUDEX capability gate" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPointCloudEntity(),
               ("local DWG writer rejected malformed POINTCLOUD transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPointCloudEx(),
               ("local DWG writer rejected malformed POINTCLOUDEX transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteHatch(),
               ("local DWG writer emitted HATCH" + suffix).c_str(), failures);
        expect(writeIface.wroteLeader(),
               ("local DWG writer emitted LEADER" + suffix).c_str(), failures);
        expect(writeIface.wroteTolerance(),
               ("local DWG writer emitted TOLERANCE" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedTolerance(),
               ("local DWG writer rejected malformed TOLERANCE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteRText(),
               ("local DWG writer emitted RTEXT" + suffix).c_str(), failures);
        expect(writeIface.wroteArcAlignedText(),
               ("local DWG writer emitted ARCALIGNEDTEXT" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRText(),
               ("local DWG writer rejected malformed RTEXT transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedArcAlignedText(),
               ("local DWG writer rejected malformed ARCALIGNEDTEXT transaction" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteDimensionAssociation()
                   : !writeIface.wroteDimensionAssociation(),
               ("local DWG DIMASSOC capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteEvaluationGraph()
                   : !writeIface.wroteEvaluationGraph(),
               ("local DWG EVALUATION_GRAPH capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.rejectedMalformedDimensionAssociation()
                   : true,
               ("local DWG writer rejected malformed DIMASSOC transaction" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.rejectedMalformedEvaluationGraph()
                   : true,
               ("local DWG writer rejected malformed EVALUATION_GRAPH transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteBlockRepresentationData(),
               ("local DWG writer emitted BLOCKREPRESENTATIONDATA" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedBlockRepresentationData(),
               ("local DWG writer rejected malformed BLOCKREPRESENTATIONDATA transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteBlock(),
               ("local DWG writer emitted user block" + suffix).c_str(), failures);
        expect(writeIface.wroteInsert(),
               ("local DWG writer emitted INSERT" + suffix).c_str(), failures);
        expect(writeIface.wroteAttrib(),
               ("local DWG writer emitted ATTRIB/SEQEND" + suffix).c_str(), failures);
        expect(writeIface.wroteGroup(),
               ("local DWG writer emitted GROUP object" + suffix).c_str(), failures);
        expect(writeIface.wroteObjectSet(),
               ("local DWG writer emitted object carrier set" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedObject(),
               ("local DWG writer rejected malformed object transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedStyle(),
               ("local DWG writer rejected malformed MLINESTYLE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedMLeaderStyle(),
               ("local DWG writer rejected malformed MLEADERSTYLE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedDictionaryVar(),
               ("local DWG writer rejected malformed DICTIONARYVAR transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedDictionaryWithDefault(),
               ("local DWG writer rejected malformed DICTIONARYWDFLT transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedSortEntsTable(),
               ("local DWG writer rejected malformed SORTENTSTABLE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedFieldList(),
               ("local DWG writer rejected malformed FIELDLIST transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedField(),
               ("local DWG writer rejected malformed FIELD transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRasterVariables(),
               ("local DWG writer rejected malformed RASTERVARIABLES transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedWipeoutVariables(),
               ("local DWG writer rejected malformed WIPEOUTVARIABLES transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedVisualStyle(),
               ("local DWG writer rejected malformed VISUALSTYLE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRenderSettings(),
               ("local DWG writer rejected malformed RENDERSETTINGS transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRenderEnvironment(),
               ("local DWG writer rejected malformed Environment RENDERSETTINGS transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRenderGlobal(),
               ("local DWG writer rejected malformed Global RENDERSETTINGS transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRenderEntry(),
               ("local DWG writer rejected malformed Entry RENDERSETTINGS transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRenderRapid(),
               ("local DWG writer rejected malformed RapidRT RENDERSETTINGS transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedMaterial(),
               ("local DWG writer rejected malformed MATERIAL transaction" + suffix).c_str(),
               failures);
        expect(version == DRW::AC1015
                   ? writeIface.rejectedUnsupportedDbColor()
                   : writeIface.wroteDbColor(),
               ("local DWG DBCOLOR version gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedDbColor(),
               ("local DWG writer rejected malformed DBCOLOR transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedLightList(),
               ("local DWG writer rejected malformed LIGHTLIST transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedScale(),
               ("local DWG writer rejected malformed SCALE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedIDBuffer(),
               ("local DWG writer rejected malformed IDBUFFER transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedLayerIndex(),
               ("local DWG writer rejected malformed LAYER_INDEX transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedSpatialIndex(),
               ("local DWG writer rejected malformed SPATIAL_INDEX transaction" + suffix).c_str(),
               failures);
        expect(version <= DRW::AC1021
                   ? writeIface.wroteTableStyle()
                   : writeIface.rejectedUnsupportedTableStyle(),
               ("local DWG TABLESTYLE capability gate" + suffix).c_str(), failures);
        expect(version <= DRW::AC1021
                   ? writeIface.rejectedMalformedTableStyle()
                   : true,
               ("local DWG writer rejected malformed TABLESTYLE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedSpatialFilter(),
               ("local DWG writer rejected malformed SPATIAL_FILTER transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteGeoData(),
               ("local DWG writer emitted GEODATA object" + suffix).c_str(), failures);
        expect(writeIface.wroteGeoDataV2(),
               ("local DWG writer emitted GEODATA v2 object" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedGeoData(),
               ("local DWG writer rejected malformed GEODATA transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePdfUnderlay(),
               ("local DWG writer emitted PDFDEFINITION" + suffix).c_str(), failures);
        expect(writeIface.wroteDgnUnderlay(),
               ("local DWG writer emitted DGNDEFINITION" + suffix).c_str(), failures);
        expect(writeIface.wroteDwfUnderlay(),
               ("local DWG writer emitted DWFDEFINITION" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedUnderlay(),
               ("local DWG writer rejected malformed UNDERLAYDEFINITION transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointCloudDefinition(),
               ("local DWG writer emitted POINTCLOUDDEFINITION" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointCloudDefinitionEx(),
               ("local DWG writer emitted POINTCLOUDDEFINITIONEX" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointCloudReactor(),
               ("local DWG writer emitted POINTCLOUDDEFREACTOR" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointCloudReactorEx(),
               ("local DWG writer emitted POINTCLOUDDEFREACTOREX" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPointCloud(),
               ("local DWG writer rejected malformed POINTCLOUDDEFINITION transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointCloudColorMap(),
               ("local DWG writer emitted POINTCLOUDCOLORMAP" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPointCloudColorMap(),
               ("local DWG writer rejected malformed POINTCLOUDCOLORMAP transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteNavisworksModelDef(),
               ("local DWG writer emitted NAVISWORKSMODELDEF" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedNavisworksModelDef(),
               ("local DWG writer rejected malformed NAVISWORKSMODELDEF transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteSunStudy(),
               ("local DWG writer emitted SUNSTUDY" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedSunStudy(),
               ("local DWG writer rejected malformed SUNSTUDY transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteMotionPath(),
               ("local DWG writer emitted MOTIONPATH" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedMotionPath(),
               ("local DWG writer rejected malformed MOTIONPATH transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteCurvePath(),
               ("local DWG writer emitted CURVEPATH" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedCurvePath(),
               ("local DWG writer rejected malformed CURVEPATH transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointPath(),
               ("local DWG writer emitted POINTPATH" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPointPath(),
               ("local DWG writer rejected malformed POINTPATH transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteObjectPtr(),
               ("local DWG writer emitted OBJECT_PTR" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedObjectPtr(),
               ("local DWG writer rejected malformed OBJECT_PTR transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePartialViewingIndex(),
               ("local DWG writer emitted PARTIAL_VIEWING_INDEX" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPartialViewingIndex(),
               ("local DWG writer rejected malformed PARTIAL_VIEWING_INDEX transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteBackgrounds(),
               ("local DWG writer emitted all BACKGROUND kinds" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedBackground(),
               ("local DWG writer rejected malformed BACKGROUND transaction" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteSectionManager()
                   : writeIface.rejectedUnsupportedSection(),
               ("local DWG SECTION_MANAGER capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteSectionSettings()
                   : writeIface.rejectedUnsupportedSection(),
               ("local DWG SECTION_SETTINGS capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.rejectedMalformedSection()
                   : true,
               ("local DWG writer rejected malformed SECTION transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteTvDeviceProperties(),
               ("local DWG writer emitted TVDEVICEPROPERTIES" + suffix).c_str(),
               failures);
        expect(writeIface.wroteVxControl(),
               ("local DWG writer emitted VXCONTROL" + suffix).c_str(),
               failures);
        expect(writeIface.wroteVxTableRecord(),
               ("local DWG writer emitted VXTABLERECORD" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedTvDeviceProperties(),
               ("local DWG writer rejected malformed TVDEVICEPROPERTIES transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedVxControl(),
               ("local DWG writer rejected malformed VXCONTROL transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedVxTableRecord(),
               ("local DWG writer rejected malformed VXTABLERECORD transaction" + suffix).c_str(),
               failures);
        expect(version < DRW::AC1018
                   ? !writeIface.wroteImage()
                   : writeIface.wroteImage(),
               ("local DWG IMAGE capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedImage(),
               ("local DWG writer rejected malformed IMAGE transaction" + suffix).c_str(),
               failures);
        expect(std::filesystem::exists(output),
               ("local DWG output is published" + suffix).c_str(), failures);

        if (version == DRW::AC1024) {
            const Ac1024ClassesIntegrityReceipt classes =
                inspectAc1024ClassesIntegrity(output);
            expect(classes.valid,
                   "AC1024 independent CLASSES raw parser succeeds", failures);
            expect(classes.maxClassNumber == 1328u,
                   "AC1024 CLASSES reaches class 1328", failures);
            expect(classes.usedExtendedStringSize
                       && classes.stringBitSize == 536312u,
                   "AC1024 CLASSES uses the expected extended string footer",
                   failures);
            expect(classes.declaredCrcOffset == classes.footerCrcOffset,
                   "AC1024 CLASSES RL and footer identify the same CRC byte",
                   failures);
            expect(classes.storedCrc != 0u
                       && classes.calculatedCrc == classes.storedCrc,
                   "AC1024 CLASSES stored CRC matches independent ODA CRC",
                   failures);
        }

        dwgRW reader(output.string().c_str());
        LocalDwgInterface readIface(nullptr, version);
        readIface.setTableStyleExpected(version <= DRW::AC1021);
        const bool readOk = reader.read(&readIface, false);
        expect(readOk, ("local DWG reader self-read succeeds" + suffix).c_str(),
               failures);
        expect(reader.getVersion() == version,
               ("local DWG self-read preserves version" + suffix).c_str(), failures);
        if (version >= DRW::AC1021) {
            expect(reader.getClassesCrcMismatch() == 0u,
                   ("local DWG self-read has no CLASSES CRC mismatch" + suffix).c_str(),
                   failures);
            const std::vector<DwgIntegrityDiagnostic> diagnostics =
                reader.getIntegrityDiagnostics();
            const bool hasClassesCrc = std::any_of(
                diagnostics.cbegin(), diagnostics.cend(),
                [](const DwgIntegrityDiagnostic& diagnostic) {
                    return diagnostic.kind == DwgIntegrityCheckKind::ClassesCrc;
                });
            expect(!hasClassesCrc,
                   ("local DWG self-read has no CLASSES CRC diagnostic" + suffix).c_str(),
                   failures);
        }
        if (version == DRW::AC1024) {
            expect(reader.getIntegrityDiagnosticsDropped() == 0u,
                   "AC1024 self-read drops no integrity diagnostics", failures);
        }
        expect(readIface.readLineSeen(),
               ("local DWG self-read publishes a line" + suffix).c_str(), failures);
        expect(readIface.readSimpleEntitiesSeen(),
               ("local DWG self-read publishes simple entity set" + suffix).c_str(),
               failures);
        expect(readIface.readAdvancedEntitiesSeen(),
               ("local DWG self-read publishes advanced entity set" + suffix).c_str(),
               failures);
        expect(readIface.readOldPolylineSeen(),
               ("local DWG self-read publishes POLYLINE" + suffix).c_str(), failures);
        expect(readIface.readSplineSeen(),
               ("local DWG self-read publishes SPLINE" + suffix).c_str(), failures);
        expect(version > DRW::AC1024
                   ? readIface.readSplineR2013Fields()
                   : true,
               ("local DWG R2013+ SPLINE fields stay cursor-aligned" + suffix).c_str(),
               failures);
        expect(readIface.readHelixSeen(),
               ("local DWG self-read publishes HELIX" + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? readIface.readCameraSeen()
                   : !readIface.readCameraSeen(),
               ("local DWG self-read CAMERA capability gate" + suffix).c_str(), failures);
        expect(version >= DRW::AC1027
                   ? readIface.readGeoPositionMarkerSeen()
                   : !readIface.readGeoPositionMarkerSeen(),
               ("local DWG self-read GEOPOSITIONMARKER capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? readIface.readShapeSeen()
                   : !readIface.readShapeSeen(),
               ("local DWG self-read SHAPE capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? readIface.readMLineSeen()
                   : !readIface.readMLineSeen(),
               ("local DWG self-read MLINE capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? readIface.readLightSeen()
                   : !readIface.readLightSeen(),
               ("local DWG self-read LIGHT capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? readIface.readMeshSeen()
                   : !readIface.readMeshSeen(),
               ("local DWG self-read MESH capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? readIface.readWipeoutSeen()
                   : !readIface.readWipeoutSeen(),
               ("local DWG self-read WIPEOUT capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? readIface.readNavisworksModelSeen()
                   : !readIface.readNavisworksModelSeen(),
               ("local DWG self-read NAVISWORKSMODEL capability gate"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? readIface.readUnderlaySeen()
                   : !readIface.readUnderlaySeen(),
               ("local DWG self-read UNDERLAY capability gate"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? readIface.readDgnUnderlayEntitySeen()
                   : !readIface.readDgnUnderlayEntitySeen(),
               ("local DWG self-read DGNUNDERLAY capability gate"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? readIface.readDwfUnderlayEntitySeen()
                   : !readIface.readDwfUnderlayEntitySeen(),
               ("local DWG self-read DWFUNDERLAY capability gate"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1021
                   ? readIface.readSurfaceSetSeen()
                   : !readIface.readSurfaceSetSeen(),
               ("local DWG self-read SURFACE variant capability gate"
                + suffix).c_str(), failures);
        expect(version > DRW::AC1018
                   ? readIface.readPointCloudSeen()
                   : !readIface.readPointCloudSeen(),
               ("local DWG POINTCLOUD capability-gated self-read" + suffix).c_str(),
               failures);
        expect(version > DRW::AC1024
                   ? readIface.readPointCloudExSeen()
                   : !readIface.readPointCloudExSeen(),
               ("local DWG POINTCLOUDEX capability-gated self-read" + suffix).c_str(),
               failures);
        expect(readIface.readHatchSeen(),
               ("local DWG self-read publishes HATCH" + suffix).c_str(), failures);
        expect(readIface.readLeaderSeen(),
               ("local DWG self-read publishes LEADER" + suffix).c_str(), failures);
        expect(readIface.readToleranceSeen(),
               ("local DWG self-read publishes TOLERANCE" + suffix).c_str(), failures);
        expect(readIface.readRTextSeen(),
               ("local DWG self-read publishes RTEXT" + suffix).c_str(), failures);
        expect(readIface.readArcAlignedTextSeen(),
               ("local DWG self-read publishes ARCALIGNEDTEXT" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? readIface.readDimensionAssociationSeen()
                   : !readIface.readDimensionAssociationSeen(),
               ("local DWG self-read publishes DIMASSOC" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? readIface.readEvaluationGraphSeen()
                   : !readIface.readEvaluationGraphSeen(),
               ("local DWG self-read publishes EVALUATION_GRAPH" + suffix).c_str(),
               failures);
        expect(readIface.readBlockRepresentationDataSeen(),
               ("local DWG self-read publishes BLOCKREPRESENTATIONDATA" + suffix).c_str(),
               failures);
        expect(readIface.readInsertSeen(),
               ("local DWG self-read publishes INSERT" + suffix).c_str(), failures);
        expect(readIface.readAttribSeen(),
               ("local DWG self-read publishes ATTRIB" + suffix).c_str(), failures);
        expect(readIface.readGroupSeen(),
               ("local DWG self-read publishes GROUP" + suffix).c_str(), failures);
        expect(readIface.readObjectSetSeen(),
               ("local DWG self-read publishes object carrier set" + suffix).c_str(),
               failures);
        expect(readIface.readObjectRawCarrierSetSeen(),
               ("local DWG self-read pairs typed OBJECTS with valid raw carriers"
                + suffix).c_str(), failures);
        expect(readIface.readTvDevicePropertiesSeen(),
               ("local DWG self-read publishes TVDEVICEPROPERTIES" + suffix).c_str(),
               failures);
        expect(readIface.readVxControlSeen(),
               ("local DWG self-read publishes VXCONTROL" + suffix).c_str(),
               failures);
        expect(readIface.readVxTableRecordSeen(),
               ("local DWG self-read publishes VXTABLERECORD" + suffix).c_str(),
               failures);
        expect(readIface.readVisualStyleSeen(),
               ("local DWG self-read publishes VISUALSTYLE" + suffix).c_str(),
               failures);
        expect(readIface.readRenderSettingsSeen(),
               ("local DWG self-read publishes RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(readIface.readRenderEnvironmentSeen(),
               ("local DWG self-read publishes Environment RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(readIface.readRenderGlobalSeen(),
               ("local DWG self-read publishes Global RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(readIface.readRenderEntrySeen(),
               ("local DWG self-read publishes Entry RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(readIface.readRenderRapidSeen(),
               ("local DWG self-read publishes RapidRT RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(readIface.readMaterialSeen(),
               ("local DWG self-read publishes MATERIAL" + suffix).c_str(),
               failures);
        expect(version == DRW::AC1015
                   ? !readIface.readDbColorSeen()
                   : readIface.readDbColorSeen(),
               ("local DWG DBCOLOR version-gated self-read" + suffix).c_str(),
               failures);
        expect(readIface.readLightListSeen(),
               ("local DWG self-read publishes LIGHTLIST" + suffix).c_str(),
               failures);
        expect(readIface.readScaleSeen(),
               ("local DWG self-read publishes SCALE" + suffix).c_str(),
               failures);
        expect(readIface.readIDBufferSeen(),
               ("local DWG self-read publishes IDBUFFER" + suffix).c_str(),
               failures);
        expect(readIface.readLayerIndexSeen(),
               ("local DWG self-read publishes LAYER_INDEX" + suffix).c_str(),
               failures);
        expect(readIface.readSpatialIndexSeen(),
               ("local DWG self-read publishes SPATIAL_INDEX" + suffix).c_str(),
               failures);
        expect(version <= DRW::AC1021
                   ? readIface.readTableStyleSeen()
                   : !readIface.readTableStyleSeen(),
               ("local DWG TABLESTYLE capability-gated self-read" + suffix).c_str(),
               failures);
        expect(readIface.readSpatialFilterSeen(),
               ("local DWG self-read publishes SPATIAL_FILTER" + suffix).c_str(),
               failures);
        expect(readIface.readGeoDataSeen(),
               ("local DWG self-read publishes GEODATA" + suffix).c_str(), failures);
        expect(readIface.readGeoDataV2Seen(),
               ("local DWG self-read publishes GEODATA v2" + suffix).c_str(), failures);
        expect(readIface.readPdfUnderlaySeen(),
               ("local DWG self-read publishes PDFDEFINITION" + suffix).c_str(), failures);
        expect(readIface.readDgnUnderlaySeen(),
               ("local DWG self-read publishes DGNDEFINITION" + suffix).c_str(), failures);
        expect(readIface.readDwfUnderlaySeen(),
               ("local DWG self-read publishes DWFDEFINITION" + suffix).c_str(), failures);
        expect(readIface.readPointCloudDefinitionSeen(),
               ("local DWG self-read publishes POINTCLOUDDEFINITION" + suffix).c_str(),
               failures);
        expect(readIface.readPointCloudDefinitionExSeen(),
               ("local DWG self-read publishes POINTCLOUDDEFINITIONEX" + suffix).c_str(),
               failures);
        expect(readIface.readPointCloudReactorSeen(),
               ("local DWG self-read publishes POINTCLOUDDEFREACTOR" + suffix).c_str(),
               failures);
        expect(readIface.readPointCloudReactorExSeen(),
               ("local DWG self-read publishes POINTCLOUDDEFREACTOREX" + suffix).c_str(),
               failures);
        expect(readIface.readPointCloudColorMapSeen(),
               ("local DWG self-read publishes POINTCLOUDCOLORMAP" + suffix).c_str(),
               failures);
        expect(readIface.readNavisworksModelDefSeen(),
               ("local DWG self-read publishes NAVISWORKSMODELDEF" + suffix).c_str(),
               failures);
        expect(readIface.readSunStudySeen(),
               ("local DWG self-read publishes SUNSTUDY" + suffix).c_str(),
               failures);
        expect(readIface.readMotionPathSeen(),
               ("local DWG self-read publishes MOTIONPATH" + suffix).c_str(),
               failures);
        expect(readIface.readCurvePathSeen(),
               ("local DWG self-read publishes CURVEPATH" + suffix).c_str(),
               failures);
        expect(readIface.readPointPathSeen(),
               ("local DWG self-read publishes POINTPATH" + suffix).c_str(),
               failures);
        expect(readIface.readObjectPtrSeen(),
               ("local DWG self-read publishes OBJECT_PTR" + suffix).c_str(),
               failures);
        expect(readIface.readPartialViewingIndexSeen(),
               ("local DWG self-read publishes PARTIAL_VIEWING_INDEX" + suffix).c_str(),
               failures);
        expect(readIface.readBackgroundsSeen(),
               ("local DWG self-read publishes all BACKGROUND kinds" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? readIface.readSectionSetSeen()
                   : !readIface.readSectionManagerSeen()
                       && !readIface.readSectionSettingsSeen(),
               ("local DWG SECTION capability-gated self-read" + suffix).c_str(),
               failures);
        expect(version < DRW::AC1018 || readIface.readImageSeen(),
               ("local DWG self-read publishes IMAGE" + suffix).c_str(), failures);
        expect(version < DRW::AC1018 || readIface.readImageDefSeen(),
               ("local DWG self-read publishes IMAGEDEF" + suffix).c_str(), failures);
        expect(version < DRW::AC1018 || readIface.readImageReactorSeen(),
               ("local DWG self-read publishes IMAGEDEF_REACTOR" + suffix).c_str(), failures);
        expect(!readIface.readMalformedObjectSeen(),
               ("local DWG self-read omits rolled-back malformed object" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedStyleSeen(),
               ("local DWG self-read omits rolled-back malformed MLINESTYLE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedMLeaderStyleSeen(),
               ("local DWG self-read omits rolled-back malformed MLEADERSTYLE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedDictionaryVarSeen(),
               ("local DWG self-read omits rolled-back malformed DICTIONARYVAR" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedDictionaryWithDefaultSeen(),
               ("local DWG self-read omits rolled-back malformed DICTIONARYWDFLT" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedSortEntsTableSeen(),
               ("local DWG self-read omits rolled-back malformed SORTENTSTABLE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedFieldListSeen(),
               ("local DWG self-read omits rolled-back malformed FIELDLIST" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedFieldSeen(),
               ("local DWG self-read omits rolled-back malformed FIELD" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRasterVariablesSeen(),
               ("local DWG self-read omits rolled-back malformed RASTERVARIABLES" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedWipeoutVariablesSeen(),
               ("local DWG self-read omits rolled-back malformed WIPEOUTVARIABLES" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedVisualStyleSeen(),
               ("local DWG self-read omits rolled-back malformed VISUALSTYLE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRenderSettingsSeen(),
               ("local DWG self-read omits rolled-back malformed RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRenderEnvironmentSeen(),
               ("local DWG self-read omits rolled-back malformed Environment RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRenderGlobalSeen(),
               ("local DWG self-read omits rolled-back malformed Global RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRenderEntrySeen(),
               ("local DWG self-read omits rolled-back malformed Entry RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRenderRapidSeen(),
               ("local DWG self-read omits rolled-back malformed RapidRT RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedMaterialSeen(),
               ("local DWG self-read omits rolled-back malformed MATERIAL" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedDbColorSeen(),
               ("local DWG self-read omits rolled-back malformed DBCOLOR" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedLightListSeen(),
               ("local DWG self-read omits rolled-back malformed LIGHTLIST" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedScaleSeen(),
               ("local DWG self-read omits rolled-back malformed SCALE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedIDBufferSeen(),
               ("local DWG self-read omits rolled-back malformed IDBUFFER" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedLayerIndexSeen(),
               ("local DWG self-read omits rolled-back malformed LAYER_INDEX" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedSpatialIndexSeen(),
               ("local DWG self-read omits rolled-back malformed SPATIAL_INDEX" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedTableStyleSeen(),
               ("local DWG self-read omits rolled-back malformed TABLESTYLE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedSpatialFilterSeen(),
               ("local DWG self-read omits rolled-back malformed SPATIAL_FILTER" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedGeoDataSeen(),
               ("local DWG self-read omits rolled-back malformed GEODATA" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedPointCloudSeen(),
               ("local DWG self-read omits rolled-back malformed POINTCLOUDDEFINITION" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedPointCloudColorMapSeen(),
               ("local DWG self-read omits rolled-back malformed POINTCLOUDCOLORMAP" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedNavisworksModelDefSeen(),
               ("local DWG self-read omits rolled-back malformed NAVISWORKSMODELDEF" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedSunStudySeen(),
               ("local DWG self-read omits rolled-back malformed SUNSTUDY" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedMotionPathSeen(),
               ("local DWG self-read omits rolled-back malformed MOTIONPATH" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedCurvePathSeen(),
               ("local DWG self-read omits rolled-back malformed CURVEPATH" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedPointPathSeen(),
               ("local DWG self-read omits rolled-back malformed POINTPATH" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedObjectPtrSeen(),
               ("local DWG self-read omits rolled-back malformed OBJECT_PTR" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedPartialViewingIndexSeen(),
               ("local DWG self-read omits rolled-back malformed PARTIAL_VIEWING_INDEX" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedBackgroundSeen(),
               ("local DWG self-read omits rolled-back malformed BACKGROUND" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedSectionSeen(),
               ("local DWG self-read omits rolled-back malformed SECTION" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedTvDevicePropertiesSeen(),
               ("local DWG self-read omits rolled-back malformed TVDEVICEPROPERTIES" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedVxControlSeen(),
               ("local DWG self-read omits rolled-back malformed VXCONTROL" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedVxTableRecordSeen(),
               ("local DWG self-read omits rolled-back malformed VXTABLERECORD" + suffix).c_str(),
               failures);
        if (readIface.readLineSeen()) {
            const DRW_Line& line = readIface.readLine();
            expect(line.basePoint.x == 1.0 && line.basePoint.y == 2.0
                       && line.basePoint.z == 3.0,
                   ("local DWG self-read preserves line start" + suffix).c_str(),
                   failures);
            expect(line.secPoint.x == 4.0 && line.secPoint.y == 5.0
                       && line.secPoint.z == 6.0,
                   ("local DWG self-read preserves line end" + suffix).c_str(),
                   failures);
        }
        if (!keepOutputs)
            std::filesystem::remove(output, ec);
    }
    expect(runDxfSurfaceRoundTrip(false, directory, keepOutputs),
           "local DXF ASCII SURFACE subtype semantic round-trip", failures);
    expect(runDxfSurfaceRoundTrip(true, directory, keepOutputs),
           "local DXF binary SURFACE subtype semantic round-trip", failures);
    expect(runDxfSurfaceRejectsMalformed(directory),
           "local DXF SURFACE rejects malformed size, boolean, and matrix fields",
           failures);
    expect(runDxfSplineSurfaceRejectsInvalidPayload(directory),
           "local DXF HELIX/SPLINE/SURFACE writers reject invalid field values",
           failures);
    expect(runDxfSplineHelixRoundTrip(false, directory, keepOutputs),
           "local DXF ASCII SPLINE fit/control and HELIX semantic round-trip",
           failures);
    expect(runDxfSplineHelixRoundTrip(true, directory, keepOutputs),
           "local DXF binary SPLINE fit/control and HELIX semantic round-trip",
           failures);
    expect(runDxfTopologyRoundTrip(false, directory, keepOutputs),
           "local DXF ASCII 3D topology and OCS/WCS round-trip", failures);
    expect(runDxfTopologyRoundTrip(true, directory, keepOutputs),
           "local DXF binary 3D topology and OCS/WCS round-trip", failures);
    expect(runDxfLegacyEllipseDowngrade(directory, keepOutputs),
           "local DXF R12 ellipse downgrade preserves 3D geometry", failures);
    expect(runDxfEllipseRejectsInvalidGeometry(directory),
           "local DXF ELLIPSE rejects undefined extrusion normals",
           failures);
    expect(runDxfArcCircleThicknessRoundTrip(false, directory),
           "local DXF ASCII ARC/CIRCLE preserves thickness independently of OCS projection",
           failures);
    expect(runDxfArcCircleThicknessRoundTrip(true, directory),
           "local DXF binary ARC/CIRCLE preserves thickness independently of OCS projection",
           failures);
    expect(runDxfRejectsInvalidFaceFlags(false, directory),
           "local DXF ASCII writer rejects invalid 3DFACE edge flags", failures);
    expect(runDxfRejectsInvalidFaceFlags(true, directory),
           "local DXF binary writer rejects invalid 3DFACE edge flags", failures);
    expect(runDxfInsertTransformRoundTrip(false, directory, keepOutputs),
           "local DXF ASCII nested INSERT/MINSERT transform fields", failures);
    expect(runDxfInsertTransformRoundTrip(true, directory, keepOutputs),
           "local DXF binary nested INSERT/MINSERT transform fields", failures);
    expect(runDxfInsertRejectsInvalidPayload(),
           "local DXF INSERT rejects out-of-range MINSERT counts", failures);
    const std::vector<std::uint8_t> sabPayload = makeLocalSabPayload();
    expect(runAcisSabFastCheck(), "local ACIS SAB parser fast check", failures);
    const std::vector<std::uint8_t> textCarrier {
        'A', 'C', 'I', 'S', ' ', 'S', 'A', 'T', ' ', 'L', 'O', 'C', 'A', 'L'};
    expect(runDxfModelerCarrierRoundTrip(DRW::AC1018, textCarrier, "text", false),
           "local DXF AC1018 text modeler carrier round-trip", failures);
    expect(runDxfModelerCarrierRoundTrip(DRW::AC1015, textCarrier, "ac1015", false),
           "local DXF AC1015 SAT text modeler carrier round-trip", failures);
    expect(runDxfModelerCarrierRoundTrip(DRW::AC1021, textCarrier, "ac1021", true),
           "local DXF AC1021 binary-file SAT text modeler carrier round-trip", failures);
    expect(runDxfModelerCarrierRoundTrip(DRW::AC1024, textCarrier, "ac1024", false),
           "local DXF AC1024 SAT text modeler carrier round-trip", failures);
    expect(runDxfModelerEnvelopeRoundTrip(false, directory, keepOutputs),
           "local DXF AC1027 modeler shell fields and history-handle remap",
           failures);
    expect(runDxfModelerEnvelopeRoundTrip(true, directory, keepOutputs),
           "local binary DXF AC1027 modeler shell fields and history-handle remap",
           failures);
    expect(runDxfDimstyleRejectsInvalidBooleans(directory),
           "local ASCII/binary DXF writers reject invalid DIMSTYLE booleans",
           failures);
    expect(runDxfViewportLightingRoundTrip(false, directory),
           "local ASCII DXF viewport lighting round-trip", failures);
    expect(runDxfViewportLightingRoundTrip(true, directory),
           "local binary DXF viewport boolean-width round-trip", failures);
    expect(runDxfRayXlineRoundTrip(false, directory),
           "local ASCII DXF RAY/XLINE WCS round-trip", failures);
    expect(runDxfRayXlineRoundTrip(true, directory),
           "local binary DXF RAY/XLINE WCS round-trip", failures);
    expect(runDxfAcdsModelerOwnerRemap(false, directory, keepOutputs),
           "local ASCII DXF AC1027 ACDSDATA ASM_Data owner remap and second pass",
           failures);
    expect(runDxfAcdsModelerOwnerRemap(true, directory, keepOutputs),
           "local binary DXF AC1027 ACDSDATA ASM_Data owner remap and second pass",
           failures);
    expect(runDxfAcdsDataStorageProjection(false, directory, keepOutputs),
           "local ASCII DWG DataStorage to AC1027 ACDSDATA projection",
           failures);
    expect(runDxfAcdsDataStorageProjection(true, directory, keepOutputs),
           "local binary DWG DataStorage to AC1027 ACDSDATA projection",
           failures);
    expect(runDxfAcdsRegionWithoutHistoryProjection(false, directory,
                                                    keepOutputs),
           "local ASCII AC1027 REGION ACDSDATA projection without history",
           failures);
    expect(runDxfAcdsRegionWithoutHistoryProjection(true, directory,
                                                    keepOutputs),
           "local binary AC1027 REGION ACDSDATA projection without history",
           failures);
    expect(runDxfAcdsMultipleDataStorageProjection(
               false, directory, keepOutputs),
           "local ASCII multi-modeler ACDS projection with thumbnails",
           failures);
    expect(runDxfAcdsMultipleDataStorageProjection(
               true, directory, keepOutputs),
           "local binary multi-modeler ACDS projection with thumbnails",
           failures);
    expect(runAcdsHistoryCallbackCapture(),
           "local typed ACIS history/evaluation callback retention",
           failures);
    expect(runDxfAcShExtrusionProjectionCapture(directory, false),
           "local DXF ACSH extrusion projection captures fixed fields atomically",
           failures);
    expect(runDxfAcShExtrusionProjectionCapture(directory, true),
           "local DXF ACSH extrusion projection declines malformed transform count",
           failures);
    expect(runDxfAcShExtrusionProjectionCapture(directory, false, 0, 128),
           "local DXF ACSH extrusion projection accepts int16 miter options",
           failures);
    expect(runDxfAcShExtrusionProjectionCapture(directory, false, 4, 2),
           "local DXF ACSH extrusion projection rejects unknown alignment options",
           failures);
    expect(runDxfTypedAcdsBoxRoundTrip(false, directory, keepOutputs),
           "local ASCII typed ACDS BOX closure round-trip", failures);
    expect(runDxfTypedAcdsBoxRoundTrip(true, directory, keepOutputs),
           "local binary typed ACDS BOX closure round-trip", failures);
    expect(runDxfEvaluationGraphRoundTrip(false, directory, keepOutputs),
           "local ASCII EvaluationGraph typed writer and handle remap",
           failures);
    expect(runDxfEvaluationGraphRoundTrip(true, directory, keepOutputs),
           "local binary EvaluationGraph typed writer and handle remap",
           failures);
    expect(runDxfAcdsDataStorageProjection(false, directory, keepOutputs,
                                           false, false, true),
           "local AC1027 ACDS projection accepts ODA-reassigned DWG class IDs",
           failures);
    expect(runDxfAcdsDataStorageProjection(false, directory, keepOutputs,
                                           true),
           "local ASCII AC1027 ACDS projection with reversed object callbacks",
           failures);
    expect(runDxfAcdsDataStorageProjection(true, directory, keepOutputs,
                                           true),
           "local binary AC1027 ACDS projection with reversed object callbacks",
           failures);
    expect(runDxfAcdsDataStorageProjection(false, directory, keepOutputs,
                                           false, true),
           "local ASCII AC1027 ACDS projection with reversed entity order",
           failures);
    expect(runDxfAcdsDataStorageProjection(true, directory, keepOutputs,
                                           false, true),
           "local binary AC1027 ACDS projection with reversed entity order",
           failures);
    expect(runDxfAcdsHistoryClosureRejectsAmbiguity(directory, true, false),
           "local DWG ACDS projection rejects duplicate history proxies",
           failures);
    expect(runDxfAcdsHistoryClosureRejectsAmbiguity(directory, false, true),
           "local DWG ACDS projection rejects ambiguous material dictionaries",
           failures);
    expect(runDxfAcdsHistoryClosureRejectsMixedClasses(directory),
           "local DWG ACDS projection rejects unrelated custom classes and raw proxies",
           failures);
    expect(runDxfAcdsSchemaFingerprintRejectsMismatch(directory),
           "local DWG ACDS projection rejects unqualified schema fingerprints",
           failures);
    expect(runDxfAcdsHistoryClosureRejectsMalformedEdges(directory),
           "local DWG ACDS projection rejects malformed history closure edges",
           failures);
    expect(runDxfAcdsModelerOwnerRejectsMalformed(false, false, "FC20"),
           "local DXF rejects ASM_Data association without an owner key",
           failures);
    expect(runDxfAcdsModelerOwnerRejectsMalformed(true, true, "FC20"),
           "local DXF rejects ambiguous duplicate ASM_Data owner keys",
           failures);
    expect(runDxfAcdsModelerOwnerRejectsMalformed(true, false, "FC99"),
           "local DXF rejects orphan ASM_Data owner keys",
           failures);
    expect(runDxfAcdsModelerOwnerRejectsMalformed(true, false, "FC20", true),
           "local DXF rejects ASM_Data keys ambiguous across modeler entities",
           failures);
    expect(runDxfAcdsModelerOwnerRejectsMalformed(true, false, "ZZ"),
           "local DXF rejects malformed ASM_Data owner handles",
           failures);
    expect(runDxfModelerRejectsUnassociatedSab(sabPayload),
           "local DXF rejects unassociated AC1027 SAB entity payload", failures);
    expect(runDxfModelerRejectsDwgFramePayload(),
           "local DXF rejects DWG modeler frame bytes as ACIS", failures);
    expect(runDxfSurfaceSatCarrierRoundTrip(),
           "local DXF AC1024 surface SAT text carrier round-trip", failures);
    expect(runDxfSurfaceRejectsUnassociatedSab(sabPayload),
           "local DXF rejects unassociated AC1027 surface SAB payload", failures);
    expect(runDxfMalformedModelerCarrier("ABC"),
           "local malformed DXF modeler odd hex rejection", failures);
    expect(runDxfMalformedModelerCarrier("GG"),
           "local malformed DXF modeler non-hex rejection", failures);
    expect(runDxfMixedModelerCarrierChunks(),
           "local mixed DXF modeler carrier chunk identity", failures);
    expect(runRawDwgReplayContract(),
           "local DWG raw-object/raw-section replay contract", failures);
    expect(runModelerDwgVersionValidation(),
           "local DWG modeler version range and empty-body validation", failures);
    expect(runModelerDwgSatV1Extraction(),
           "local AC1015 DWG SAT v1 bounded extraction and DXF text carrier",
           failures);
    if (failures != 0) {
        std::cerr << failures << " local DWG round-trip assertion(s) failed\n";
        return 1;
    }
    std::cout << "Local DWG round-trip: PASS\n";
    return 0;
}
