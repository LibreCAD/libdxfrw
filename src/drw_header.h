/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2022 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DRW_HEADER_H
#define DRW_HEADER_H

// X11's object-like None macro must not alter public enum definitions.
// Save it locally; callers retain their exact macro after this header.
// Units::Unitless is the macro-safe spelling of the legacy Units::None.
#if defined(None)
# if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#  pragma push_macro("None")
#  undef None
#  define LIBDXFRW_DRW_HEADER_RESTORE_NONE
# else
#  error "libdxfrw requires push_macro/pop_macro support with a caller None macro"
# endif
#endif

#include <bitset>
#include <map>
#include <memory>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <optional>
#include "drw_base.h"

class dxfReader;
class dxfWriter;
class dwgBuffer;
class dwgBufferW;
class DxfReadRetainedLease;
class DrwHeaderEncodeTestAccess;  // test-only friend; defined in tests/dwg_header_encode_round_trip_tests.cpp
class DwgHandseedTestAccess;      // test-only friend; defined in tests/dwg_handseed_tests.cpp
class dwgWriter24;                // forward declaration for friend access

#define SETHDRFRIENDS  friend class dxfRW; \
                       friend class dwgReader; \
                       friend class dwgWriter15; \
                       friend class dwgWriter24; \
                       friend class DwgHandseedTestAccess; \
                       friend class DrwHeaderEncodeTestAccess;

//! Class to handle header entries
/*!
*  Class to handle header vars, to read iterate over "std::unordered_map vars"
*  to write add a DRW_Variant* into "std::unordered_map vars" (do not delete it, are cleared in dtor)
*  or use add* helper functions.
*  @author Rallaz
*/
class DRW_Header {
    SETHDRFRIENDS
public:
    DRW_Header();
    ~DRW_Header() {
        clearVars();
    }

    enum Units {
        /** $ISUNITS header variable, since ACAD2000/AC1015 */
        None = 0,               ///< No unit (unit from parent)
        Unitless = None,        ///< Macro-safe spelling for X11 consumers
        Inch = 1,               ///< 25.4 mm
        Foot = 2,               ///< 12 Inches = 0.3048 m
        Mile = 3,               ///< 1760 Yards = 1609 m
        Millimeter = 4,         ///< 0.001 m
        Centimeter = 5,         ///< 0.01 m
        Meter = 6,
        Kilometer = 7,          ///< 1000 m
        Microinch = 8,          ///< 0.000001 Inch = 0.0000254 mm = 25.4 Nanometer
        Mil = 9,                ///< 0.001 Inch = 0.0254 mm = 25.4 Micron
        Yard = 10,              ///< 3 Feet = 0.9144 m
        Angstrom = 11,          ///< 10^-10 m
        Nanometer = 12,         ///< 10^-9 m
        Micron = 13,            ///< 10^-6 m
        Decimeter = 14,         ///< 0.1 m
        Decameter = 15,         ///< 10 m
        Hectometer = 16,        ///< 100 m
        Gigameter = 17,         ///< 10^9 m
        Astro = 18,             ///< ~149.6 x 10^9 m
        Lightyear = 19,         ///< ~9.46 x 10^15 m
        Parsec = 20,            ///< ~3.0857 x 10^16 m
        UnitCount = 21,         ///< Used to iterate through units

        /** $MEASUREMENT header variable, since R14/AC1014 */
        English = 0,            ///< English/Imperial drawing */
        Metric = 1,             ///< Metric drawing */
    };

    DRW_Header(const DRW_Header& h)
        : m_handseedValueOffset(std::streampos(-1)),
          handSeed(h.handSeed),
          m_dwgHandseedBitOffset(kInvalidDwgHandseedBitOffset) {
        copyScalarStateFrom(h);
        try {
            copyVariantMap(vars, h.vars);
            copyVariantMap(customVars, h.customVars);
        } catch (...) {
            clearVars();
            throw;
        }
        curr = nullptr;
        name.clear();
    }

    DRW_Header(DRW_Header&& h) noexcept
        : m_handseedValueOffset(h.m_handseedValueOffset),
          vars(std::move(h.vars)),
          customVars(std::move(h.customVars)),
          customProperties(std::move(h.customProperties)),
          comments(std::move(h.comments)),
          name(std::move(h.name)),
          curr(h.curr),
          version(h.version),
          m_dxfHeaderFinalized(h.m_dxfHeaderFinalized),
          m_dxfSourceVersion(h.m_dxfSourceVersion),
          m_dxfCodePageSource(std::move(h.m_dxfCodePageSource)),
          m_dxfCustomPropertiesWellFormed(h.m_dxfCustomPropertiesWellFormed),
          m_dxfHeaderStructureWellFormed(
              h.m_dxfHeaderStructureWellFormed),
          m_dxfHeaderCurrentGroupCodes(h.m_dxfHeaderCurrentGroupCodes),
          m_dxfUnknownHeaderLease(std::move(h.m_dxfUnknownHeaderLease)),
          m_dxfRequiredVersionsWellFormed(
              h.m_dxfRequiredVersionsWellFormed),
          m_dxfPlotStyleIdWellFormed(h.m_dxfPlotStyleIdWellFormed),
          m_pendingCustomPropertyTag(std::move(h.m_pendingCustomPropertyTag)),
          m_customPropertyFieldValueSeen(h.m_customPropertyFieldValueSeen),
          m_hasDerivedDimunitAlias(h.m_hasDerivedDimunitAlias),
          m_dimunitSourceCode(h.m_dimunitSourceCode),
          m_dimunitSourceValue(h.m_dimunitSourceValue),
          m_dimunitAliasCode(h.m_dimunitAliasCode),
          m_dimunitAliasValue(h.m_dimunitAliasValue),
          linetypeCtrl(h.linetypeCtrl),
          layerCtrl(h.layerCtrl),
          styleCtrl(h.styleCtrl),
          dimstyleCtrl(h.dimstyleCtrl),
          appidCtrl(h.appidCtrl),
          blockCtrl(h.blockCtrl),
          viewCtrl(h.viewCtrl),
          ucsCtrl(h.ucsCtrl),
          vportCtrl(h.vportCtrl),
          vpEntHeaderCtrl(h.vpEntHeaderCtrl),
          handSeed(h.handSeed),
          m_dwgHandseedBitOffset(h.m_dwgHandseedBitOffset),
          m_dwgNamedObjectDictionaryReference(h.m_dwgNamedObjectDictionaryReference) {
        h.curr = nullptr;
        h.m_dxfHeaderFinalized = false;
        h.m_dxfSourceVersion = DRW::UNKNOWNV;
        h.m_dxfCustomPropertiesWellFormed = true;
        h.m_dxfHeaderStructureWellFormed = true;
        h.m_dxfHeaderCurrentGroupCodes.reset();
        h.m_dxfUnknownHeaderLease.reset();
        h.m_dxfRequiredVersionsWellFormed = true;
        h.m_dxfPlotStyleIdWellFormed = true;
        h.m_pendingCustomPropertyTag.reset();
        h.m_customPropertyFieldValueSeen = false;
        h.clearDxfDimunitAliasProvenance();
        h.m_dwgNamedObjectDictionaryReference.reset();
    }

    DRW_Header& operator=(const DRW_Header &h) {
        if (this != &h) {
            DRW_Header copy(h);
            swap(copy);
        }
        return *this;
    }

    DRW_Header& operator=(DRW_Header&& h) noexcept {
        if (this != &h) {
            DRW_Header moved(std::move(h));
            swap(moved);
        }
        return *this;
    }

    void addDouble(std::string key, double value, int code);
    void addInt(std::string key, int value, int code);
    void addInt64(std::string key, std::int64_t value, int code);
    void addStr(std::string key, std::string value, int code);
    void addCoord(std::string key, DRW_Coord value, int code);
    /// Validate HEADER hard pointers to VISUALSTYLE objects. The current
    /// conservative writer floor is R2007 (AC1021) for all three; this is not
    /// a claim that their individual introduction releases are identical.
    /// Presence remains distinct from an explicit null handle.
    bool canWriteDxfVisualStyleReferences(DRW::Version target) const;
    /// Read one named DXF HEADER VisualStyle handle without consuming it.
    /// Returns false for an unknown name, duplicate aliases, or malformed
    /// value; `present` distinguishes an absent variable from explicit null.
    bool getDxfVisualStyleReferenceHandle(const char* name,
                                          std::uint32_t& handle,
                                          bool& present) const;
    /// Optional R2007+ HEADER material hard pointer (group347). Accept either
    /// dollar or bare spelling, never both. Validate the full scalar string;
    /// `present` distinguishes absence from an explicitly supplied null handle.
    bool getDxfMaterialReferenceHandle(std::uint32_t& handle,
                                       bool& present) const noexcept;
    bool canWriteDxfMaterialReference(DRW::Version target) const noexcept;
    /// Return whether the dimension-variable family can be represented in the
    /// requested DXF revision without dropping modern split-variable values.
    bool canWriteDxfDimensionVariables(DRW::Version target) const;
    void addCustomVar(std::string key, std::string value, int code = 1) {
        storeCustomVar(key, std::make_unique<DRW_Variant>(
            code, UTF8STRING(std::move(value))));
    }
    /// Ordered DXF drawing properties, serialized as alternating
    /// $CUSTOMPROPERTYTAG/$CUSTOMPROPERTY group-1 pairs. Repeated tags are
    /// retained in order. DXF R2004 (AC1018) or later is required for output.
    void addCustomProperty(std::string tag, std::string value) {
        customProperties.emplace_back(std::move(tag), std::move(value));
    }
    /// Discard parsed custom-property data and clear any malformed-source
    /// state so callers can intentionally replace it with a new list.
    void clearDxfCustomProperties() noexcept {
        customProperties.clear();
        m_pendingCustomPropertyTag.reset();
        m_customPropertyFieldValueSeen = false;
        m_dxfCustomPropertiesWellFormed = true;
    }
    /// Check whether custom properties can be emitted without loss for the
    /// requested target revision.
    bool canWriteDxfCustomProperties(DRW::Version target) const noexcept;
    /// Refuse rewriting a DXF HEADER that repeated an ordinary variable or a
    /// group code within one variable. The parser remains tolerant; callers
    /// may explicitly acknowledge a repaired/replaced value before writing.
    bool canWriteDxfHeaderStructure() const noexcept {
        return m_dxfHeaderStructureWellFormed;
    }
    /// Validate scalar unknown HEADER values retained with a supported DXF
    /// group type. Unknown compounds and non-null reference roles fail closed.
    bool canWriteDxfUnknownHeaderVariables() const noexcept;
    void clearDxfHeaderStructureError() noexcept {
        m_dxfHeaderStructureWellFormed = true;
        m_dxfHeaderCurrentGroupCodes.reset();
    }
    /// `$REQUIREDVERSIONS` is a 64-bit HEADER value introduced in AC1027.
    /// Preserve its exact bits for AC1027+, and refuse older targets rather
    /// than silently dropping an unknown required-feature mask.
    bool canWriteDxfRequiredVersions(DRW::Version target) const noexcept;
    /// Remove the parsed or caller-provided `$REQUIREDVERSIONS` entry and
    /// clear malformed/repeated-source state so a caller can replace it.
    void clearDxfRequiredVersions();
    /// Validate the `$CEPSNID` handle's shape and target-version availability.
    /// A graph-aware adapter must separately close and remap non-null targets.
    bool canWriteDxfPlotStyleId(DRW::Version target) const noexcept;
    bool getDxfPlotStyleIdHandle(std::uint64_t& handle,
                                 bool& present) const noexcept;
    /// Discard parsed `$CEPSNID` state and clear malformed-source status.
    void clearDxfPlotStyleId();
    std::string getComments() const {return comments;}
    void write(const std::unique_ptr<dxfWriter>& writer, DRW::Version ver);
    void addComment(std::string c);
    // Append a borrowed value transactionally without first copying it into a
    // by-value argument. The read facade uses this after reserving retained
    // storage for pre-SECTION comments.
    void appendComment(const std::string& c);

    /// HANDSEED accessors.  The DWG writer uses these to propagate the
    /// document's high-water-mark handle so AutoCAD does not refresh
    /// HANDSEED on first save.  See [Risk 4j] in the writer plan.
    std::uint32_t getHandSeed() const { return handSeed; }
    void    setHandSeed(std::uint32_t h) { handSeed = h; }

    static constexpr std::uint32_t kInvalidDwgHandseedBitOffset =
        (std::numeric_limits<std::uint32_t>::max)();
    std::uint32_t dwgHandseedBitOffset() const {
        return m_dwgHandseedBitOffset;
    }

    /// Stream offset of the $HANDSEED value field written by the ASCII DXF
    /// header writer (DRW_Header::write).  The value is emitted as a fixed
    /// 8-hex-digit placeholder so dxfRW can seek back here after the OBJECTS
    /// section and overwrite it with the final handle high-water mark
    /// (highWaterHandle).  -1 means "not recorded" (binary DXF / pre-2000).
    std::streampos m_handseedValueOffset {std::streampos(-1)};
    /// Fixed character width of the back-patchable $HANDSEED value field.
    static constexpr int kHandseedFieldWidth = 8;

    static int measurement(const int unit);

    /// Highest recognized source DXF declaration accepted during this read.
    /// A later lower/unknown $ACADVER remains visible in vars but cannot lower
    /// this value or the reader's structural checks. Fresh/API/DWG headers
    /// have no DXF declaration and report UNKNOWNV.
    DRW::Version dxfSourceVersion() const noexcept { return m_dxfSourceVersion; }

    /// Original DWG HEADER named-object-dictionary H field, copied by value.
    /// Missing (fresh/DXF/failed HEADER) and explicitly null are distinct.
    /// ref64 retains the complete encoded value; ref is the legacy low32 view.
    /// A decoded reference is not proof of a matching complete root object,
    /// and this provenance does not control regenerated writer identities.
    std::optional<dwgHandle> dwgNamedObjectDictionaryReference() const noexcept {
        return m_dwgNamedObjectDictionaryReference;
    }
    /// Checks only the absolute HEADER handle role and current32-bit map
    /// capacity, not existence/type/completeness of its referenced object.
    static bool isUsableDwgNamedObjectDictionaryReference(const dwgHandle& source) noexcept {
        if (source.size > sizeof(source.ref64)
            || source.ref != static_cast<std::uint32_t>(source.ref64)
            || (source.size < sizeof(source.ref64)
                && (source.ref64 >> (source.size * 8u)) != 0u)
            || source.ref64 > (std::numeric_limits<std::uint32_t>::max)())
            return false;
        if (source.ref64 != 0u)
            return source.code == DRW::DwgHardOwnership;
        return source.code == DRW::DwgNullReference
            || source.code == DRW::DwgSoftOwnership
            || source.code == DRW::DwgHardOwnership
            || source.code == DRW::DwgSoftPointer
            || source.code == DRW::DwgHardPointer;
    }

    /// Last complete physically read DXF $DWGCODEPAGE group-3 value, before Unicode
    /// decoding or compatibility normalization. Reader delimiters and ASCII's
    /// terminal CR are excluded; binary payload CR is retained. Absent and
    /// explicitly empty are distinct.
    /// Header copies share immutable owned bytes, never a reader/caller view.
    bool hasDxfCodePageDeclaration() const noexcept {
        return m_dxfCodePageSource != nullptr;
    }
    /// Bytes may not be UTF-8; this is provenance, not a decoding certificate.
    /// The reference lasts while this header (or a retaining header copy)
    /// owns the snapshot. Editing vars does not change this historical value.
    const std::string& dxfCodePageDeclaration() const noexcept;
    /// Detach only provenance, leaving the normalized vars entry unchanged.
    void clearDxfCodePageDeclaration() noexcept { m_dxfCodePageSource.reset(); }

    enum class DxfDimunitAliasState {
        None,        ///< No unchanged derived alias; a present alias is explicit.
        Derived,     ///< Both legacy source and compatibility alias match capture.
        StaleSource  ///< Alias is unchanged but its legacy source changed/disappeared.
    };
    /// Legacy DXF keeps $DIMUNIT and exposes a derived $DIMLUNIT for existing
    /// 2D consumers. This is not the split-variable semantic conversion.
    DxfDimunitAliasState dxfDimunitAliasState() const;
    /// Public vars permits direct edits. A changed alias is automatically
    /// explicit; call this for same-value edits that assert modern meaning.
    /// addInt/addStr/etc. for $DIMLUNIT do this automatically.
    void clearDxfDimunitAliasProvenance() noexcept {
        m_hasDerivedDimunitAlias = false;
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader);
    [[nodiscard]] bool parseDwgImpl(DRW::Version version, dwgBuffer *buf,
                                    dwgBuffer *hBbuf,
                                    std::uint8_t mv = 0);
    [[nodiscard]] bool parseDwgBody(DRW::Version version, dwgBuffer *buf,
                                    dwgBuffer *hBbuf, std::uint8_t mv,
                                    bool consumeTrailer);
    [[nodiscard]] bool parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer *hBbuf, std::uint8_t mv=0);
    /// Inverse of parseDwg: emits the bit-packed body of the HEADER
    /// section.  For R2000 (AC1015), `buf` and `hBbuf` may alias the
    /// same accumulator since the handle stream is inline.  Order of
    /// emission matches parseDwg byte-for-byte.
    /// For R2007+ (AC1021+), TV/TU header strings are written to `strBuf`
    /// (the separate string stream); dwgWriter appends them + the footer.
    [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf, dwgBufferW *hBbuf,
                   dwgBufferW *strBuf = nullptr);
private:
    /// Writer-only variant supplies the root object identity from the
    /// writer's reserved graph; source HEADER provenance remains separate.
    [[nodiscard]] bool encodeDwgForWriter(
        DRW::Version version, dwgBufferW *buf, dwgBufferW *hBbuf,
        dwgBufferW *strBuf, std::uint32_t namedObjectDictionaryHandle,
        std::uint32_t plotSettingsDictionaryHandle);
    bool parseCodeImpl(int code, const std::unique_ptr<dxfReader>& reader,
                       bool deferText);
    bool finalizeDxfHeader(const std::unique_ptr<dxfReader>& reader);
    bool captureDxfCodePageDeclaration(const std::unique_ptr<dxfReader>& reader);
    bool resolveDxfDimensionVariables(DRW::Version target,
                                      int& legacyUnit, int& legacyFit,
                                      int& linearUnit, int& fraction,
                                      int& textArrowFit, int& textMove) const;
    static bool matchesIntVar(const std::unordered_map<std::string, DRW_Variant*>& values,
                              const char* key, int code, int value);
    template <typename Map>
    static void copyVariantMap(Map& destination, const Map& source) {
        try {
            for (const auto& item : source) {
                std::unique_ptr<DRW_Variant> copy = item.second != nullptr
                    ? std::make_unique<DRW_Variant>(*item.second) : nullptr;
                auto result = destination.emplace(item.first, copy.get());
                if (result.second)
                    copy.release();
            }
        } catch (...) {
            for (auto& item : destination)
                delete item.second;
            destination.clear();
            throw;
        }
    }

    void copyScalarStateFrom(const DRW_Header& h) {
        comments = h.comments;
        version = h.version;
        m_dxfHeaderFinalized = h.m_dxfHeaderFinalized;
        m_dxfSourceVersion = h.m_dxfSourceVersion;
        m_dxfCodePageSource = h.m_dxfCodePageSource;
        customProperties = h.customProperties;
        m_dxfCustomPropertiesWellFormed = h.m_dxfCustomPropertiesWellFormed;
        m_dxfHeaderStructureWellFormed =
            h.m_dxfHeaderStructureWellFormed;
        m_dxfHeaderCurrentGroupCodes = h.m_dxfHeaderCurrentGroupCodes;
        m_dxfUnknownHeaderLease = h.m_dxfUnknownHeaderLease;
        m_dxfRequiredVersionsWellFormed =
            h.m_dxfRequiredVersionsWellFormed;
        m_dxfPlotStyleIdWellFormed = h.m_dxfPlotStyleIdWellFormed;
        m_pendingCustomPropertyTag = h.m_pendingCustomPropertyTag;
        m_customPropertyFieldValueSeen = h.m_customPropertyFieldValueSeen;
        m_hasDerivedDimunitAlias = h.m_hasDerivedDimunitAlias;
        m_dimunitSourceCode = h.m_dimunitSourceCode;
        m_dimunitSourceValue = h.m_dimunitSourceValue;
        m_dimunitAliasCode = h.m_dimunitAliasCode;
        m_dimunitAliasValue = h.m_dimunitAliasValue;
        linetypeCtrl = h.linetypeCtrl;
        layerCtrl = h.layerCtrl;
        styleCtrl = h.styleCtrl;
        dimstyleCtrl = h.dimstyleCtrl;
        appidCtrl = h.appidCtrl;
        blockCtrl = h.blockCtrl;
        viewCtrl = h.viewCtrl;
        ucsCtrl = h.ucsCtrl;
        vportCtrl = h.vportCtrl;
        vpEntHeaderCtrl = h.vpEntHeaderCtrl;
        m_dwgNamedObjectDictionaryReference = h.m_dwgNamedObjectDictionaryReference;
    }

    void swap(DRW_Header& other) noexcept {
        using std::swap;
        swap(m_handseedValueOffset, other.m_handseedValueOffset);
        vars.swap(other.vars);
        customVars.swap(other.customVars);
        customProperties.swap(other.customProperties);
        swap(comments, other.comments);
        swap(name, other.name);
        swap(curr, other.curr);
        swap(version, other.version);
        swap(m_dxfHeaderFinalized, other.m_dxfHeaderFinalized);
        swap(m_dxfSourceVersion, other.m_dxfSourceVersion);
        swap(m_dxfCodePageSource, other.m_dxfCodePageSource);
        swap(m_dxfCustomPropertiesWellFormed,
             other.m_dxfCustomPropertiesWellFormed);
        swap(m_dxfHeaderStructureWellFormed,
             other.m_dxfHeaderStructureWellFormed);
        swap(m_dxfHeaderCurrentGroupCodes,
             other.m_dxfHeaderCurrentGroupCodes);
        swap(m_dxfUnknownHeaderLease, other.m_dxfUnknownHeaderLease);
        swap(m_dxfRequiredVersionsWellFormed,
             other.m_dxfRequiredVersionsWellFormed);
        swap(m_dxfPlotStyleIdWellFormed,
             other.m_dxfPlotStyleIdWellFormed);
        swap(m_pendingCustomPropertyTag, other.m_pendingCustomPropertyTag);
        swap(m_customPropertyFieldValueSeen,
             other.m_customPropertyFieldValueSeen);
        swap(m_hasDerivedDimunitAlias, other.m_hasDerivedDimunitAlias);
        swap(m_dimunitSourceCode, other.m_dimunitSourceCode);
        swap(m_dimunitSourceValue, other.m_dimunitSourceValue);
        swap(m_dimunitAliasCode, other.m_dimunitAliasCode);
        swap(m_dimunitAliasValue, other.m_dimunitAliasValue);
        swap(linetypeCtrl, other.linetypeCtrl);
        swap(layerCtrl, other.layerCtrl);
        swap(styleCtrl, other.styleCtrl);
        swap(dimstyleCtrl, other.dimstyleCtrl);
        swap(appidCtrl, other.appidCtrl);
        swap(blockCtrl, other.blockCtrl);
        swap(viewCtrl, other.viewCtrl);
        swap(ucsCtrl, other.ucsCtrl);
        swap(vportCtrl, other.vportCtrl);
        swap(vpEntHeaderCtrl, other.vpEntHeaderCtrl);
        swap(handSeed, other.handSeed);
        swap(m_dwgHandseedBitOffset, other.m_dwgHandseedBitOffset);
        swap(m_dwgNamedObjectDictionaryReference, other.m_dwgNamedObjectDictionaryReference);
    }

    bool getDouble(std::string key, double *varDouble);
    bool getInt(std::string key, int *varInt);
    bool getStr(std::string key, std::string *varStr);
    bool getCoord(std::string key, DRW_Coord *varStr);
    void clearVars(){
        for (auto it=vars.begin(); it!=vars.end(); ++it)
            delete it->second;
        vars.clear();
        for (auto it=customVars.begin(); it!=customVars.end(); ++it)
            delete it->second;
        customVars.clear();
        customProperties.clear();
        m_dxfCustomPropertiesWellFormed = true;
        m_dxfHeaderStructureWellFormed = true;
        m_dxfHeaderCurrentGroupCodes.reset();
        m_dxfUnknownHeaderLease.reset();
        m_dxfRequiredVersionsWellFormed = true;
        m_dxfPlotStyleIdWellFormed = true;
        m_pendingCustomPropertyTag.reset();
        m_customPropertyFieldValueSeen = false;
        m_dxfHeaderFinalized = false;
        m_dxfSourceVersion = DRW::UNKNOWNV;
        clearDxfCodePageDeclaration();
        clearDxfDimunitAliasProvenance();
        m_dwgNamedObjectDictionaryReference.reset();
    }
    /// Store v under key in vars, freeing any previously-stored variant
    /// (avoids a leak when a header variable appears more than once).
    /// Updates curr so subsequent parseCode value reads land in v.
    void storeVar(const std::string& key, DRW_Variant* v) {
        if (v == nullptr) {
            curr = nullptr;
            return;
        }
        if (key == "$DIMLUNIT")
            clearDxfDimunitAliasProvenance();
        std::unique_ptr<DRW_Variant> replacement(v);
        auto it = vars.find(key);
        if (it == vars.end()) {
            auto result = vars.emplace(key, replacement.get());
            curr = result.second ? replacement.release() : result.first->second;
        } else {
            DRW_Variant* previous = it->second;
            curr = replacement.release();
            it->second = curr;
            delete previous;
        }
    }

    void storeCustomVar(const std::string& key,
                        std::unique_ptr<DRW_Variant> replacement) {
        if (!replacement)
            return;
        auto it = customVars.find(key);
        if (it == customVars.end()) {
            auto result = customVars.emplace(key, replacement.get());
            if (result.second)
                replacement.release();
            return;
        }
        DRW_Variant* previous = it->second;
        it->second = replacement.release();
        delete previous;
    }

public:
    std::unordered_map<std::string,DRW_Variant*> vars;
    std::map<std::string, DRW_Variant*> customVars; /*!< custom/unknown header variables */
    std::vector<std::pair<std::string, std::string>> customProperties;
private:
    std::string comments;
    std::string name;
    DRW_Variant* curr {nullptr};
    int version; //to use on read
    bool m_dxfHeaderFinalized {false};
    DRW::Version m_dxfSourceVersion {DRW::UNKNOWNV};
    struct DxfCodePageSource;
    std::shared_ptr<const DxfCodePageSource> m_dxfCodePageSource;
    bool m_dxfCustomPropertiesWellFormed {true};
    bool m_dxfHeaderStructureWellFormed {true};
    std::bitset<1072> m_dxfHeaderCurrentGroupCodes;
    std::shared_ptr<DxfReadRetainedLease> m_dxfUnknownHeaderLease;
    bool m_dxfRequiredVersionsWellFormed {true};
    bool m_dxfPlotStyleIdWellFormed {true};
    std::optional<std::string> m_pendingCustomPropertyTag;
    bool m_customPropertyFieldValueSeen {false};
    bool m_hasDerivedDimunitAlias {false};
    int m_dimunitSourceCode {0};
    int m_dimunitSourceValue {0};
    int m_dimunitAliasCode {0};
    int m_dimunitAliasValue {0};

    std::uint32_t linetypeCtrl;
    std::uint32_t layerCtrl;
    std::uint32_t styleCtrl;
    std::uint32_t dimstyleCtrl;
    std::uint32_t appidCtrl;
    std::uint32_t blockCtrl;
    std::uint32_t viewCtrl;
    std::uint32_t ucsCtrl;
    std::uint32_t vportCtrl;
    std::uint32_t vpEntHeaderCtrl;
    /// HANDSEED: the document's high-water-mark allocated handle.
    /// parseDwg captures it from the data stream; a fresh DWG encode writes a
    /// fixed-width placeholder and finalizes it after object handles exist.
    /// For round-trip preservation, populate via the captured source value.
    std::uint32_t handSeed {0};
    /// Bit offset of the fixed-width HANDSEED payload relative to the start
    /// of the buffer passed to encodeDwg(). It is a write-time locator, never
    /// persisted.
    std::uint32_t m_dwgHandseedBitOffset {kInvalidDwgHandseedBitOffset};
    std::optional<dwgHandle> m_dwgNamedObjectDictionaryReference;


};

#ifdef LIBDXFRW_DRW_HEADER_RESTORE_NONE
# pragma pop_macro("None")
# undef LIBDXFRW_DRW_HEADER_RESTORE_NONE
#endif

#endif
