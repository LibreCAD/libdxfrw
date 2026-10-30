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

#ifndef DRW_OBJECTS_H
#define DRW_OBJECTS_H

// X11's object-like None macro must not alter public enum definitions.
// Save it locally; callers retain their exact macro after this header.
// Coverage reason enums provide NoReason as their macro-safe zero spelling.
#if defined(None)
# if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#  pragma push_macro("None")
#  undef None
#  define LIBDXFRW_DRW_OBJECTS_RESTORE_NONE
# else
#  error "libdxfrw requires push_macro/pop_macro support with a caller None macro"
# endif
#endif

#include "drw_base.h"
#include <array>
#include <cstddef>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

class dxfReader;
class dxfWriter;
class DxfReadRetainedLease;
namespace dxfField { class Decoder; }
namespace dxfBlockRecord { class Decoder; }
namespace dxfLayout { class Decoder; }
namespace dxfViewUcs { class Decoder; }
namespace dxfRenderSettings { class Decoder; }
namespace dxfRendering { class Observation; }
class dwgBuffer;
class dwgRW;

namespace DRW {

//! Table entries type.
enum TTYPE {
  UNKNOWNT,
  LTYPE,
  LAYER,
  STYLE,
  DIMSTYLE,
  VPORT,
  BLOCK_RECORD,
  APPID,
  IMAGEDEF,
  PLOTSETTINGS,
  VIEW,
  UCS,
  MLINESTYLE,
  LAYOUT,
  DICTIONARY,
  DICTIONARYVAR,
  DICTIONARYWDFLT,
  XRECORD,
  FIELD,
  FIELDLIST,
  RASTERVARIABLES,
  SORTENTSTABLE,
  MATERIAL,
  TABLESTYLE,
  TABLECONTENT,
  CELLSTYLEMAP,
  MLEADERSTYLE,
  DBCOLOR,
  VISUALSTYLE,
  UNDERLAYDEFINITION,
  SCALE,
  DIMASSOC,
  EVALUATIONGRAPH,
  BLOCKREPRESENTATIONDATA,
  DETAILVIEWSTYLE,
  SECTIONVIEWSTYLE,
  BREAKDATA,
  BREAKPOINTREF,
  GROUP,
  IMAGEDEFREACTOR,
  SPATIALFILTER,
  GEODATA,
  TABLEGEOMETRY,
  ACDBPLACEHOLDER,
  VBAPROJECT,
  SUN,
  LIGHTLIST,
  DATALINK,
  GEOMAPIMAGE,
  ASSOCIATIVEOBJECT,
  ACSHHISTORYOBJECT,
  IDBUFFER,
  LAYERFILTER,
  LAYERINDEX,
  SPATIALINDEX,
  BACKGROUND,
  POINTCLOUDDEF,
  NAVISWORKSMODELDEF,
  POINTCLOUDCOLORMAP,
  SUNSTUDY,
  MOTIONPATH,
  CURVEPATH,
  POINTPATH,
  OBJECTPTR,
  PARTIALVIEWINGINDEX,
  RENDERSETTINGS,
  SECTIONOBJ,
  OBJECTCONTEXTDATA,
  DATATABLE,
  DYNAMICBLOCKOBJECT,
  VPORT_ENTITY_HEADER,
  VXCONTROL,
  VXTABLERECORD,
  CSACDOCUMENTOPTIONS,
  TVDEVICEPROPERTIES,
  CONTEXTDATAMANAGER,
  INDEX,
  PROXYOBJECT
};

// pending VP_ENT_HDR, ACAD_TABLE,
} // namespace DRW

struct DRW_DwgSubrecordRange {
  UTF8STRING m_name;
  std::uint64_t m_startBit = 0;
  std::uint64_t m_bitSize = 0;
  DRW::Version m_version = DRW::UNKNOWNV;
  std::uint32_t m_count = 0;
  bool m_parseComplete = true;
};

//! Decode-only context for a DICTIONARY count that exceeds complete
//! name/handle pairs available in its bounded object streams.
struct DRW_DictionaryCountCap {
  std::uint32_t declaredCount = 0;
  std::uint32_t clampedCount = 0;
  std::uint64_t bitOffset = 0;
  std::uint64_t remainingBytes = 0;
  std::uint64_t remainingBits = 0;
};

class dwgBufferW;
class DwgBlockOwnershipTestAccess;

// Read-only name-field evidence shared by the APPID/LAYER DWG parsers. It is
// not an entire table-record certificate, and cannot be installed by callers.
class DRW_DwgTableNameSource {
public:
  [[nodiscard]] bool matches(std::uint32_t sourceHandle, const UTF8STRING& value) const noexcept {
    return handle == sourceHandle && name == value;
  }
  [[nodiscard]] bool bound(DRW::Version sourceVersion, const std::string& page) const noexcept {
    return profileBound && version == sourceVersion && codePage == page;
  }
  [[nodiscard]] std::uint32_t decodeIssues() const noexcept { return issues; }
  [[nodiscard]] std::size_t retainedBytes() const noexcept {
    return sizeof(DRW_DwgTableNameSource) + 2u * sizeof(void*)
        + name.size() + codePage.size() + encodedField.size() + 2u;
  }
  [[nodiscard]] bool equal(const DRW_DwgTableNameSource& other) const noexcept {
    return handle == other.handle && version == other.version && name == other.name
        && codePage == other.codePage && issues == other.issues
        && profileBound == other.profileBound && startBit == other.startBit
        && bitCount == other.bitCount && encodedField == other.encodedField;
  }
private:
  friend class DRW_TableEntry;
  UTF8STRING name;
  std::string codePage;
  std::vector<std::uint8_t> encodedField;
  DRW::Version version{DRW::UNKNOWNV};
  std::uint32_t handle{0u};
  std::uint32_t issues{0u};
  std::uint64_t startBit{0u}, bitCount{0u};
  bool profileBound{false};
};

#define SETOBJFRIENDS                                                          \
  friend class dxfRW;                                                          \
  friend class dwgReader;                                                      \
  friend class dwgRW;                                                          \
  friend class dwgWriter15;                                                    \
  friend class DrwObjectEncodeTestAccess;                                      \
  friend class DwgBlockOwnershipTestAccess;

//! Base class for tables entries
/*!
 *  Base class for tables entries
 *  @author Rallaz
 */
class DRW_TableEntry {
  SETOBJFRIENDS
  friend class dxfField::Decoder;
  friend class dxfBlockRecord::Decoder;
  friend class dxfLayout::Decoder;
  friend class dxfViewUcs::Decoder;
  friend class dxfRendering::Observation;
public:
  static constexpr std::size_t kMaxExtendedDataItems = 1000000u;

  DRW_TableEntry() = default;

  virtual ~DRW_TableEntry() { clearExtData(); }

  DRW_TableEntry(const DRW_TableEntry &e)
      : tType{e.tType}, handle{e.handle}, parentHandle{e.parentHandle},
        name{e.name}, flags{e.flags}, appData{e.appData},
        reactorHandles{e.reactorHandles}, xDictHandle{e.xDictHandle},
        pendingAppIdResolutions{e.pendingAppIdResolutions},
        pendingLayerRefResolutions{e.pendingLayerRefResolutions},
        xDictFlag{e.xDictFlag}, numReactors{e.numReactors},
        commonLinkTailValidated{e.commonLinkTailValidated},
        hasDsData{e.hasDsData}, curr{nullptr} {
    // Match copy-assign (which copies these); set in the body to avoid
    // a member-init-order (-Wreorder) constraint.
    oType = e.oType;
    objSize = e.objSize;
    handleStreamBitSize = e.handleStreamBitSize;
    m_dxfRetainedStringLease = e.m_dxfRetainedStringLease;
    m_dxfRetainedNameBytes = e.m_dxfRetainedNameBytes;
    copyExtDataFrom(e);
  }

  DRW_TableEntry(DRW_TableEntry &&e) noexcept
      : tType(e.tType), handle(e.handle), parentHandle(e.parentHandle),
        name(std::move(e.name)), flags(e.flags), extData(std::move(e.extData)),
        appData(std::move(e.appData)),
        reactorHandles(std::move(e.reactorHandles)), xDictHandle(e.xDictHandle),
        pendingAppIdResolutions(std::move(e.pendingAppIdResolutions)),
        pendingLayerRefResolutions(std::move(e.pendingLayerRefResolutions)),
        xDictFlag(e.xDictFlag), numReactors(e.numReactors),
        commonLinkTailValidated(e.commonLinkTailValidated),
        hasDsData(e.hasDsData), curr(e.curr), currCoordMask(e.currCoordMask),
        m_dxfRetainedStringLease(std::move(e.m_dxfRetainedStringLease)),
        m_dxfRetainedNameBytes(e.m_dxfRetainedNameBytes) {
    oType = e.oType;
    objSize = e.objSize;
    handleStreamBitSize = e.handleStreamBitSize;
    e.extData.clear();
    e.curr = nullptr;
    e.currCoordMask = 0;
  }

  // Clone before replacing owned variants so allocation failure does not
  // leak a partially copied extData vector.
  DRW_TableEntry &operator=(const DRW_TableEntry &e) {
    if (this != &e) {
      std::vector<std::unique_ptr<DRW_Variant>> copies = cloneExtData(e);
      tType = e.tType;
      handle = e.handle;
      parentHandle = e.parentHandle;
      name = e.name;
      flags = e.flags;
      appData = e.appData;
      reactorHandles = e.reactorHandles;
      xDictHandle = e.xDictHandle;
      pendingAppIdResolutions = e.pendingAppIdResolutions;
      pendingLayerRefResolutions = e.pendingLayerRefResolutions;
      xDictFlag = e.xDictFlag;
      numReactors = e.numReactors;
      commonLinkTailValidated = e.commonLinkTailValidated;
      hasDsData = e.hasDsData;
      oType = e.oType;
      objSize = e.objSize;
      handleStreamBitSize = e.handleStreamBitSize;
      m_dxfRetainedStringLease = e.m_dxfRetainedStringLease;
      m_dxfRetainedNameBytes = e.m_dxfRetainedNameBytes;
      clearExtData();
      installExtData(std::move(copies), e);
    }
    return *this;
  }

  DRW_TableEntry &operator=(DRW_TableEntry &&e) noexcept {
    if (this == &e)
      return *this;
    clearExtData();
    tType = e.tType;
    handle = e.handle;
    parentHandle = e.parentHandle;
    name = std::move(e.name);
    flags = e.flags;
    extData = std::move(e.extData);
    appData = std::move(e.appData);
    reactorHandles = std::move(e.reactorHandles);
    xDictHandle = e.xDictHandle;
    pendingAppIdResolutions = std::move(e.pendingAppIdResolutions);
    pendingLayerRefResolutions = std::move(e.pendingLayerRefResolutions);
    xDictFlag = e.xDictFlag;
    numReactors = e.numReactors;
    commonLinkTailValidated = e.commonLinkTailValidated;
    hasDsData = e.hasDsData;
    oType = e.oType;
    objSize = e.objSize;
    handleStreamBitSize = e.handleStreamBitSize;
    curr = e.curr;
    currCoordMask = e.currCoordMask;
    m_dxfRetainedStringLease = std::move(e.m_dxfRetainedStringLease);
    m_dxfRetainedNameBytes = e.m_dxfRetainedNameBytes;
    e.extData.clear();
    e.curr = nullptr;
    e.currCoordMask = 0;
    return *this;
  }

protected:
  // Derived table records use the same shared lease as common name/XDATA
  // fields for parsed strings that outlive the current DXF record.
  bool replaceDxfRetainedStringField(
      const std::unique_ptr<dxfReader> &reader, std::size_t &ownerBytes,
      std::string &destination, std::string value);
  bool replaceDxfRetainedStringBytes(
      const std::unique_ptr<dxfReader> &reader, std::size_t &ownerBytes,
      std::size_t newOwnerBytes);
  bool appendDxfRetainedStringField(
      const std::unique_ptr<dxfReader> &reader, std::size_t &ownerBytes,
      std::vector<std::string> &destination, std::string value);
  // Charge a string retained inside a compound record whose container is not
  // a vector of strings (for example, a vector of named handle entries).
  bool retainDxfRetainedStringBytes(
      const std::unique_ptr<dxfReader> &reader, std::size_t &ownerBytes,
      std::size_t valueBytes);
  // Use when a derived record maps another DXF code to the inherited name
  // field, so it shares the same live-byte counter as group 2.
  bool replaceDxfRetainedNameField(
      const std::unique_ptr<dxfReader> &reader, std::string value);
  bool replaceDxfRetainedNameAndStringField(
      const std::unique_ptr<dxfReader> &reader,
      std::size_t &secondaryOwnerBytes, std::string &secondaryDestination,
      std::string nameValue, std::string secondaryValue);
  bool releaseDxfRetainedStringBytes(std::size_t bytes) noexcept;
  virtual bool parseCode(int code, const std::unique_ptr<dxfReader> &reader);
  // Parses and consumes one DXF 102 application group.  The staged common
  // references are published only after the matching close marker.
  bool parseDxfGroups(int code, const std::unique_ptr<dxfReader> &reader);
  [[nodiscard]] virtual bool parseDwg(DRW::Version version, dwgBuffer *buf,
                                      std::uint32_t bs = 0) = 0;
  [[nodiscard]] bool parseDwg(DRW::Version version, dwgBuffer *buf,
                              dwgBuffer *strBuf, std::uint32_t bs = 0);
  [[nodiscard]] bool parseDwgImpl(DRW::Version version, dwgBuffer *buf,
                                  dwgBuffer *strBuf, std::uint32_t bs);
  // Parse the standard OBJECTS handle tail from a detached cursor.  The
  // common object header must have been parsed first so objSize, handle,
  // reactor count, and xdictionary flag are available.
  [[nodiscard]] bool
  parseDwgCommonHandleData(DRW::Version version, dwgBuffer *buf,
                           bool includeLegacyObjectSize = false);
  // Bound the shared OBJECTS handle tail to the frame declared by the
  // R2010+ handle-bit prefix. Older frames are already isolated buffers.
  [[nodiscard]] bool readDwgObjectCommonHandles(
      DRW::Version version, dwgBuffer *buf,
      std::uint32_t *parentHandle = nullptr,
      std::vector<std::uint32_t> *reactorHandles = nullptr,
      std::uint32_t *xDictHandle = nullptr) const;
  [[nodiscard]] bool getDwgHandleEndBit(DRW::Version version,
                                        const dwgBuffer &buf,
                                        std::uint64_t &endBit) const;
  [[nodiscard]] bool readDwgObjectHandleWithinFrame(DRW::Version version,
                                                    dwgBuffer &buf, bool offset,
                                                    dwgHandle &value) const;
  [[nodiscard]] bool
  readDwgObjectHandleListWithinFrame(DRW::Version version, dwgBuffer &buf,
                                     std::int32_t count, bool offset,
                                     std::vector<std::uint32_t> *values) const;
  void reset() {
    handle = 0;
    parentHandle = 0;
    name.clear();
    flags = 0;
    clearExtData();
    appData.clear();
    m_dxfRetainedStringLease.reset();
    m_dxfRetainedNameBytes = 0u;
    reactorHandles.clear();
    pendingAppIdResolutions.clear();
    pendingLayerRefResolutions.clear();
    xDictHandle = 0;
    xDictFlag = 0;
    numReactors = 0;
    commonLinkTailValidated = false;
    hasDsData = 0;
    oType = 0;
    objSize = 0;
    handleStreamBitSize = 0;
    curr = nullptr;
    currCoordMask = 0;
  }

public:
  enum DRW::TTYPE tType{DRW::UNKNOWNT}; /*!< enum: entity type, code 0 */
  std::uint32_t handle{0};              /*!< entity identifier, code 5 */
  std::uint32_t parentHandle{
      0};          /*!< Soft-pointer ID/handle to owner object, code 330 */
  UTF8STRING name; /*!< entry name, code 2 */
  int flags{0};    /*!< Flags relevant to entry, code 70 */
  // Ordered typed XDATA: strings1000/1001/1003 are semantic UTF-8;
  // controls1002, binary1004 and handle1005 retain their distinct roles.
  std::vector<DRW_Variant *>
      extData; /*!< FIFO list of extended data, codes 1000 to 1071*/
  std::list<std::list<DRW_Variant>> appData; /*!< application data, code 102 */

  //! Append an owned XDATA variant without exposing a transfer window.
  [[nodiscard]] bool addExtData(std::unique_ptr<DRW_Variant> value) {
    return appendExtData(std::move(value));
  }

  std::vector<std::uint32_t>
      reactorHandles; /*!< persisted reactor handles (ODA §19.4.2); DWG
                         round-trip (Phase 2a) */
  std::uint32_t xDictHandle{0}; /*!< extension-dictionary handle (ODA §19.4.2);
                                   DWG round-trip (Phase 2a) */

  struct PendingHandleRef {
    std::size_t indexInExtData{0};
    std::uint32_t handleRef{0};
    std::uint64_t rawHandleRef{0};
    bool useRawHandleRef{false};
  };
  std::vector<PendingHandleRef> pendingAppIdResolutions;
  std::vector<PendingHandleRef> pendingLayerRefResolutions;

  //! AC1027+ common-object flag advertising an AcDb:AcDsPrototype_1b
  //! DataStorage payload. The payload itself remains in the raw carrier.
  [[nodiscard]] bool hasDataStorageBinaryData() const noexcept {
    return hasDsData != 0;
  }

  [[nodiscard]] std::int32_t reactorCount() const noexcept {
    return numReactors;
  }

  [[nodiscard]] std::uint8_t extensionDictionaryFlag() const noexcept {
    return xDictFlag;
  }
  [[nodiscard]] bool hasDwgCommonLinkTail() const noexcept {
    return commonLinkTailValidated;
  }
  void setDwgCommonLinkTailValidated(bool value) noexcept {
    commonLinkTailValidated = value;
  }

  // Keep the common-object header state explicit for typed metadata
  // carriers.  The payload and handle vectors remain independently owned.
  void setDwgCommonObjectState(std::int32_t reactorCountValue,
                               std::uint8_t extensionDictionaryFlagValue,
                               bool dataStorageBinaryData) noexcept {
    numReactors = reactorCountValue;
    xDictFlag = extensionDictionaryFlagValue;
    hasDsData = dataStorageBinaryData ? 1 : 0;
  }

  //***** dwg parse ********/
protected:
  static bool readDwgNameWithSource(DRW::Version version, dwgBuffer* bodyBuf,
      dwgBuffer* stringBuf, std::uint32_t bodyEnd, std::uint32_t stringEnd,
      std::int16_t encodedType, std::int16_t expectedType, std::uint32_t handle,
      UTF8STRING& parsedName, std::shared_ptr<const DRW_DwgTableNameSource>& sourceResult);
  std::int16_t oType{0};
  std::uint8_t xDictFlag{0};
  std::int32_t numReactors{0};
  bool commonLinkTailValidated{false};
  std::uint8_t hasDsData{0};
  std::uint32_t objSize{0}; // RL 32bits object data size in bits
  std::uint32_t handleStreamBitSize{0};

private:
  bool appendExtData(std::unique_ptr<DRW_Variant> value) {
    if (!value || extData.size() >= kMaxExtendedDataItems)
      return false;
    try {
      extData.push_back(value.get());
    } catch (...) {
      return false;
    }
    value.release();
    return true;
  }

  void clearExtData() noexcept {
    for (DRW_Variant *value : extData)
      delete value;
    extData.clear();
    curr = nullptr;
    currCoordMask = 0;
  }

  void copyExtDataFrom(const DRW_TableEntry &source) {
    installExtData(cloneExtData(source), source);
  }

  static std::vector<std::unique_ptr<DRW_Variant>>
  cloneExtData(const DRW_TableEntry &source) {
    std::vector<std::unique_ptr<DRW_Variant>> copies;
    copies.reserve(source.extData.size());
    for (const DRW_Variant *value : source.extData)
      copies.emplace_back(
          value != nullptr ? std::make_unique<DRW_Variant>(*value) : nullptr);
    return copies;
  }

  void installExtData(std::vector<std::unique_ptr<DRW_Variant>> copies,
                      const DRW_TableEntry &source) {
    curr = nullptr;
    currCoordMask = 0;
    extData.reserve(copies.size());
    for (std::size_t i = 0; i < copies.size(); ++i) {
      DRW_Variant *value = copies[i].get();
      extData.push_back(value);
      if (source.extData[i] == source.curr)
        curr = value;
      copies[i].release();
    }
    if (curr != nullptr)
      currCoordMask = source.currCoordMask;
  }

  DRW_Variant *curr{nullptr};
  std::uint8_t currCoordMask{0}; // transient X/Y/Z presence for one XDATA tuple
  // Shared accounting for parsed names and 102 appData strings.
  std::shared_ptr<DxfReadRetainedLease> m_dxfRetainedStringLease;
  std::size_t m_dxfRetainedNameBytes{0u};
};

//! What the reader established about the common owner/reactor/xdictionary
//! links before publishing a DWG frame.
enum class DRW_DwgCommonLinkEvidence : std::uint8_t {
  //! No bounded common-link decoder ran for this carrier.
  Unknown,
  //! At least one common link was bounded and decoded.
  Validated,
  //! The bounded decoder established that all applicable common links are
  //! absent.
  ValidatedAbsent,
  //! The frame kind has no common-link contract.
  NotApplicable
};

inline constexpr bool
drwDwgCommonLinkEvidenceValidated(DRW_DwgCommonLinkEvidence evidence) noexcept {
  return evidence == DRW_DwgCommonLinkEvidence::Validated ||
         evidence == DRW_DwgCommonLinkEvidence::ValidatedAbsent;
}

inline DRW_DwgCommonLinkEvidence drwDwgCommonLinkEvidenceForLinks(
    bool tailValidated, std::uint32_t parentHandle,
    const std::vector<std::uint32_t> &reactorHandles, std::int32_t reactorCount,
    std::uint32_t xDictHandle) noexcept {
  if (!tailValidated)
    return DRW_DwgCommonLinkEvidence::Unknown;
  return parentHandle != DRW::NoHandle || !reactorHandles.empty() ||
                 reactorCount != 0 || xDictHandle != DRW::NoHandle
             ? DRW_DwgCommonLinkEvidence::Validated
             : DRW_DwgCommonLinkEvidence::ValidatedAbsent;
}

//! Raw carrier for a DWG object/entity class that libdxfrw does not model yet.
struct DRW_UnsupportedObject {
  //! Stable callback name for the R13-R2004 viewport-entity-header control
  //! record. It is a DWG control list, not an opaque drawing object.
  static const char *vportEntityHeaderControlRecordName() noexcept {
    return "VPORT_ENTITY_HEADER_CONTROL";
  }

  void setCommonLinkEvidence(DRW_DwgCommonLinkEvidence evidence) noexcept {
    m_commonLinkEvidence = evidence;
    m_commonHandleDataValidated = drwDwgCommonLinkEvidenceValidated(evidence);
  }

  // Fixed entity codes with standard framing but no typed body model.
  // Their bodies remain opaque until libdxfrw gains the corresponding
  // vertical-product or dynamic-block support.
  static constexpr int kFixedAecEntityShellType = 1127;
  static constexpr int kFixedEntityShellFirstType = 1176;
  static constexpr int kFixedEntityShellLastType = 1194;

  static constexpr bool isFixedEntityShellType(int type) noexcept {
    return type == kFixedAecEntityShellType ||
           (type >= kFixedEntityShellFirstType &&
            type <= kFixedEntityShellLastType);
  }

  static const char *fixedEntityShellName(int type) noexcept {
    switch (type) {
    case kFixedAecEntityShellType:
      return "AEC_WALL";
    case 1176:
      return "ALIGNMENTPARAMETERENTITY";
    case 1177:
      return "BASEPOINTPARAMETERENTITY";
    case 1178:
      return "ENDREP";
    case 1179:
      return "FLIPGRIPENTITY";
    case 1180:
      return "FLIPPARAMETERENTITY";
    case 1181:
      return "JUMP";
    case 1182:
      return "LAYOUTPRINTCONFIG";
    case 1183:
      return "LINEARGRIPENTITY";
    case 1184:
      return "LINEARPARAMETERENTITY";
    case 1185:
      return "LOAD";
    case 1186:
      return "POINTPARAMETERENTITY";
    case 1187:
      return "POLARGRIPENTITY";
    case 1188:
      return "REPEAT";
    case 1189:
      return "ROTATIONGRIPENTITY";
    case 1190:
      return "ROTATIONPARAMETERENTITY";
    case 1191:
      return "VISIBILITYGRIPENTITY";
    case 1192:
      return "VISIBILITYPARAMETERENTITY";
    case 1193:
      return "XYGRIPENTITY";
    case 1194:
      return "XYPARAMETERENTITY";
    default:
      return nullptr;
    }
  }

  // Fixed OBJECTS codes with no typed body model. The standard OBJECTS
  // header and handle tail are retained before their bodies are preserved
  // for same-version replay.
  static constexpr int kFixedAecObjectShellFirstType = 1128;
  static constexpr int kFixedAecObjectShellLastType = 1130;
  static constexpr int kFixedObjectShellFirstType = 1195;
  static constexpr int kFixedObjectShellLastType = 1197;

  static constexpr bool isFixedObjectShellType(int type) noexcept {
    return (type >= kFixedAecObjectShellFirstType &&
            type <= kFixedAecObjectShellLastType) ||
           (type >= kFixedObjectShellFirstType &&
            type <= kFixedObjectShellLastType);
  }

  static const char *fixedObjectShellName(int type) noexcept {
    switch (type) {
    case 1128:
      return "AEC_WALL_STYLE";
    case 1129:
      return "AEC_CLEANUP_GROUP";
    case 1130:
      return "AECDBBINRECORD";
    case 1195:
      return "ACMECOMMANDHISTORY";
    case 1196:
      return "ACMESCOPE";
    case 1197:
      return "ACMESTATEMGR";
    default:
      return nullptr;
    }
  }

  int m_objectType = 0;
  std::uint32_t m_handle = 0;
  // Validated common owner. For entities this is the BLOCK_RECORD owner
  // when the reader has one; retain it while raw replay remains gated on
  // writer-side ownership registration.
  std::uint32_t m_parentHandle = DRW::NoHandle;
  // Compatibility projection of m_commonLinkEvidence. New consumers must
  // use the evidence state so an unverified zero is never treated as a
  // proven ownerless/no-xdictionary state.
  bool m_commonHandleDataValidated = false;
  DRW_DwgCommonLinkEvidence m_commonLinkEvidence =
      DRW_DwgCommonLinkEvidence::Unknown;
  std::vector<std::uint32_t> m_reactorHandles;
  std::uint32_t m_xDictHandle = DRW::NoHandle;
  std::int32_t m_numReactors = 0;
  std::uint8_t m_xDictFlag = 0;
  // BLOCK_RECORD reachability owner. Model/paper-space entities have an
  // ownerless common header, so this is distinct from m_parentHandle.
  std::uint32_t m_blockOwnerHandle = DRW::NoHandle;
  // The raw carrier is the validated object body only.  The enclosing
  // modular-size prefix, optional AC1024+ handle-bit prefix, and CRC are
  // rebuilt by the writer and are never part of m_rawBytes.
  std::uint32_t m_bodyBitSize = 0;
  std::uint64_t m_objectOffset = 0;
  std::uint32_t m_objectSize = 0;
  bool m_isEntity = false;
  bool m_isCustomClass = false;
  UTF8STRING m_recordName;
  UTF8STRING m_className;
  std::vector<std::uint8_t> m_rawBytes;
  // Set only when the reader also completed the typed parser for this
  // known record. It prevents an injected raw body from masquerading as a
  // replayable TABLESTYLE payload.
  bool m_typedPayloadValidated = false;
  // Raw object bytes are only valid for the DWG version that encoded them.
  // Keep the provenance with the bytes so direct writer users cannot
  // accidentally replay an otherwise unlabelled carrier into another format.
  // Append it to preserve the layout of the long-standing carrier fields.
  DRW::Version m_version = DRW::UNKNOWNV;
  // AC1027+ common entity/object header bit. The opaque DataStorage section
  // is carried separately, so replay must not advertise an unavailable
  // payload.
  bool m_hasDataStorage = false;
  // Source DWG CLASSES metadata for same-version raw replay. The instance
  // count is deliberately not carried: the writer recomputes it from the
  // handles it actually emits.
  bool m_hasClassDefinition = false;
  std::uint16_t m_classProxyFlag = 0x401;
  UTF8STRING m_classAppName;
  bool m_classWasProxy = false;
  std::uint16_t m_classEntityFlagRaw = 0x1F3;
  std::int32_t m_classDwgVersion = 0;
  std::int32_t m_classMaintenanceVersion = 0;
  std::int32_t m_classUnknown1 = 0;
  std::int32_t m_classUnknown2 = 0;
};

//! Address space of a DWG object-map location carried by a frame receipt.
//! Decoded-buffer offsets are not seekable offsets in the source file.
enum class DRW_DwgFrameOffsetSpace : std::uint8_t {
  Unknown,
  PhysicalFile,
  DecodedBuffer
};

//! Reader disposition of one HANDLE-map source frame.
//!
//! Pending, Deferred, and Staged are transient. Unresolved is assigned only by
//! the final report when the reader stops before a frame reached a final state.
enum class DRW_DwgFrameDisposition : std::uint8_t {
  Pending,
  Deferred,
  Staged,
  Published,
  Quarantined,
  Failed,
  Unresolved
};

//! Stable reason for the latest source-frame disposition.
enum class DRW_DwgFrameCoverageReason : std::uint8_t {
  None,
  NoReason = None,
  ReceiptPublished,
  CompoundDeferred,
  CompoundStaged,
  TableDeferred,
  ParseFailure,
  Quarantined,
  PhaseAborted,
  FinalizationUnresolved,
  MissingSource,
  InvalidTransition,
  CallbackException,
  ReceiptFailure
};

//! Lifecycle of a HANDLE-map coverage report.
enum class DRW_DwgFrameCoverageStatus : std::uint8_t {
  NotAvailable,
  InProgress,
  FinalizedComplete,
  FinalizedPartial
};

//! Stable source identity and disposition for one HANDLE-map frame.
struct DRW_DwgFrameCoverageEntry {
  std::uint32_t m_handle = 0;
  std::uint32_t m_sourceOffset = 0;
  std::uint64_t m_sourceMapOrdinal = 0;
  DRW_DwgFrameOffsetSpace m_sourceOffsetSpace =
      DRW_DwgFrameOffsetSpace::Unknown;
  DRW_DwgFrameDisposition m_disposition = DRW_DwgFrameDisposition::Pending;
  DRW_DwgFrameCoverageReason m_reason = DRW_DwgFrameCoverageReason::None;
  std::uint8_t m_publicationCount = 0;
};

//! Read-side summary of whether every HANDLE-map source frame reached one
//! compatible terminal publication. Non-published rows explain partial
//! coverage without relying on container iteration order.
struct DRW_DwgFrameCoverageReport {
  DRW_DwgFrameCoverageStatus m_status =
      DRW_DwgFrameCoverageStatus::NotAvailable;
  bool m_complete = false;
  std::vector<DRW_DwgFrameCoverageEntry> m_entries;
};

//! Lifecycle of a CLASSES-section coverage report. This is deliberately
//! independent of HANDLE-map frame coverage: class records have no handle-map
//! source and are parsed before the handle map exists.
enum class DRW_DwgClassCoverageStatus : std::uint8_t {
  NotAvailable,
  InProgress,
  FinalizedComplete,
  FinalizedPartial
};

//! Parse/publication state of one CLASSES stream record.
enum class DRW_DwgClassCoverageState : std::uint8_t {
  Parsed,
  Published,
  Failed
};

//! Reason a CLASSES stream record did not become usable for DWG dispatch.
enum class DRW_DwgClassCoverageReason : std::uint8_t {
  None,
  NoReason = None,
  Parse,
  Bounds,
  Duplicate,
  Publish,
  Callback
};

//! One bounded bit range within the source buffer used to parse a class.
//! Decoded-buffer positions need the enclosing section id to be meaningful;
//! physical-file positions are absolute and therefore not section-relative.
struct DRW_DwgClassBitRange {
  std::uint64_t m_startBit = 0;
  std::uint64_t m_endBit = 0;
  bool m_present = false;
  bool m_sectionRelative = true;
  DRW_DwgFrameOffsetSpace m_offsetSpace = DRW_DwgFrameOffsetSpace::Unknown;
};

//! Parser-derived provenance for one CLASSES stream record.
struct DRW_DwgClassCoverageEntry {
  std::uint64_t m_streamOrdinal = 0;
  std::uint16_t m_classNumber = 0;
  UTF8STRING m_recordName;
  UTF8STRING m_className;
  UTF8STRING m_appName;
  std::int32_t m_proxyFlag = 0;
  std::int32_t m_wasAProxyFlag = 0;
  std::uint16_t m_entityFlagRaw = 0x1F3;
  std::int32_t m_entityFlag = 0;
  std::int32_t m_instanceCount = 0;
  std::int32_t m_dwgVersion = 0;
  std::int32_t m_maintenanceVersion = 0;
  std::int32_t m_unknown1 = 0;
  std::int32_t m_unknown2 = 0;
  std::int32_t m_sectionDescriptorId = -1;
  DRW_DwgClassBitRange m_dataRange;
  DRW_DwgClassBitRange m_stringRange;
  DRW_DwgClassCoverageState m_state = DRW_DwgClassCoverageState::Parsed;
  DRW_DwgClassCoverageReason m_reason = DRW_DwgClassCoverageReason::None;
};

//! Ordered, parser-owned summary of the DWG CLASSES section.
struct DRW_DwgClassCoverageReport {
  DRW_DwgClassCoverageStatus m_status =
      DRW_DwgClassCoverageStatus::NotAvailable;
  bool m_complete = false;
  std::vector<DRW_DwgClassCoverageEntry> m_entries;
};

namespace DRW {
constexpr std::size_t kMaxDwgClassSourceBytes = 64u * 1024u * 1024u;
}

//! Optional byte-capture state, independent of parser/semantic completeness.
enum class DRW_DwgClassSourceStatus : std::uint8_t {
  NotRequested, NotCaptured, Captured, TooLarge, InvalidRange,
  CaptureFailed, RequestFailed, DeliveryFailed
};

//! Actual physical/decoded CLASSES bytes, never reconstructed from class rows.
//! Copied shared owners outlive the reader's temporary decoded storage. The
//! immutable bytes and metadata do not certify CRCs, framing, text or support.
struct DRW_DwgClassSourceReport {
  DRW_DwgClassSourceStatus m_status = DRW_DwgClassSourceStatus::NotRequested;
  DRW::Version m_version = DRW::UNKNOWNV;
  std::uint8_t m_appMaintenanceVersion = 0;
  std::uint16_t m_codePageId = 0;
  bool m_codePagePresent = false;
  bool m_codePageRecognized = false;
  std::int32_t m_sectionDescriptorId = -1;
  DRW_DwgFrameOffsetSpace m_offsetSpace = DRW_DwgFrameOffsetSpace::Unknown;
  std::uint64_t m_baseByteOffset = 0;
  std::uint64_t m_declaredSectionSize = 0;
  std::uint64_t m_byteCount = 0;
  bool m_classReadCompleted = false;
  bool m_parserCoverageComplete = false;
  std::shared_ptr<const std::vector<std::uint8_t>> m_bytes;
};

//! Read-side publication receipt for one fully validated DWG object/entity
//! frame. It records which representation reached the consumer without
//! inferring that fact from optional raw preservation callbacks.
struct DRW_DwgFramePublication {
  enum class Carrier : std::uint8_t { Typed, Raw, TypedAndRaw, Control };

  void setCommonLinkEvidence(DRW_DwgCommonLinkEvidence evidence) noexcept {
    m_commonLinkEvidence = evidence;
    m_commonHandleDataValidated = drwDwgCommonLinkEvidenceValidated(evidence);
  }

  DRW::Version m_version = DRW::UNKNOWNV;
  std::uint32_t m_handle = 0;
  std::uint32_t m_sourceOffset = 0;
  std::uint64_t m_sourceMapOrdinal = 0;
  DRW_DwgFrameOffsetSpace m_sourceOffsetSpace =
      DRW_DwgFrameOffsetSpace::Unknown;
  bool m_hasSourceLocation = false;
  int m_encodedType = 0;
  int m_resolvedType = 0;
  bool m_isEntity = false;
  bool m_isCustomClass = false;
  UTF8STRING m_recordName;
  UTF8STRING m_className;
  // Present only when a resolved custom class was published by the CLASSES
  // transaction in this reader run. Fixed types and unresolved raw classes
  // intentionally retain no guessed stream ordinal.
  std::optional<std::uint64_t> m_classStreamOrdinal;
  // Compatibility projection of m_commonLinkEvidence. See
  // DRW_UnsupportedObject for the meaning of the state values.
  bool m_commonHandleDataValidated = false;
  DRW_DwgCommonLinkEvidence m_commonLinkEvidence =
      DRW_DwgCommonLinkEvidence::Unknown;
  std::uint32_t m_parentHandle = DRW::NoHandle;
  std::vector<std::uint32_t> m_reactorHandles;
  std::uint32_t m_xDictHandle = DRW::NoHandle;
  std::int32_t m_numReactors = 0;
  std::uint8_t m_xDictFlag = 0;
  // Standard TABLES controls declare their record handles in wire order.
  // Body receipts leave this empty.
  std::vector<std::uint32_t> m_controlHandles;
  Carrier m_carrier = Carrier::Typed;
};

//! Parser-owned, complete membership of one fixed DICTIONARY object.
//!
//! This remains separate from DRW_Dictionary's typed callback: the parser may
//! retain a bounded prefix of a truncated dictionary, but graph construction
//! may only consume membership when every declared pair was decoded.
struct DRW_DwgDictionaryMembership {
  struct Entry {
    UTF8STRING m_name;
    std::uint32_t m_handle = DRW::NoHandle;
  };

  DRW::Version m_version = DRW::UNKNOWNV;
  std::uint32_t m_dictionaryHandle = DRW::NoHandle;
  std::uint32_t m_sourceOffset = 0;
  std::uint64_t m_sourceMapOrdinal = 0;
  DRW_DwgFrameOffsetSpace m_sourceOffsetSpace =
      DRW_DwgFrameOffsetSpace::Unknown;
  bool m_hasSourceLocation = false;
  bool m_complete = false;
  std::vector<Entry> m_entries;
};

//! Parser-owned, complete membership of one AcDbDictionaryWithDefault object.
//!
//! The custom object inherits DICTIONARY's ordered entries and adds one hard
//! default pointer. The resolved class tuple is retained so writers cannot
//! replace an opaque source frame merely because its record name resembles the
//! known class.
struct DRW_DwgDictionaryWithDefaultMembership {
  struct Entry {
    UTF8STRING m_name;
    std::uint32_t m_handle = DRW::NoHandle;
  };

  DRW::Version m_version = DRW::UNKNOWNV;
  std::uint32_t m_dictionaryHandle = DRW::NoHandle;
  std::uint32_t m_sourceOffset = 0;
  std::uint64_t m_sourceMapOrdinal = 0;
  DRW_DwgFrameOffsetSpace m_sourceOffsetSpace =
      DRW_DwgFrameOffsetSpace::Unknown;
  bool m_hasSourceLocation = false;
  bool m_complete = false;
  int m_encodedType = 0;
  int m_resolvedType = 0;
  UTF8STRING m_recordName;
  UTF8STRING m_className;
  std::optional<std::uint64_t> m_classStreamOrdinal;
  int m_cloning = 0;
  int m_hardOwner = 0;
  std::uint32_t m_defaultEntryHandle = DRW::NoHandle;
  std::vector<Entry> m_entries;
};

//! Parser-owned, ordered membership of one fixed GROUP object.
//!
//! GROUP stores a counted sequence of hard entity pointers. The ordinal is
//! retained because the DWG list permits repeated entity handles.
struct DRW_DwgGroupMembership {
  struct Entry {
    std::uint32_t m_handle = DRW::NoHandle;
    std::uint32_t m_ordinal = 0;
  };

  DRW::Version m_version = DRW::UNKNOWNV;
  std::uint32_t m_groupHandle = DRW::NoHandle;
  std::uint32_t m_sourceOffset = 0;
  std::uint64_t m_sourceMapOrdinal = 0;
  DRW_DwgFrameOffsetSpace m_sourceOffsetSpace =
      DRW_DwgFrameOffsetSpace::Unknown;
  bool m_hasSourceLocation = false;
  bool m_complete = false;
  std::vector<Entry> m_entries;
};

//! Parser-owned, ordered draw-order mapping from one AcDbSortentsTable.
//!
//! Each declared row has an entity soft pointer and an inline sort handle. A
//! null sort handle is meaningful: it requests the entity's own handle as the
//! sort key, so it must not be removed from the counted sequence.
struct DRW_DwgSortEntsMembership {
  struct Entry {
    std::uint32_t m_entityHandle = DRW::NoHandle;
    std::uint32_t m_sortHandle = DRW::NoHandle;
    std::uint32_t m_ordinal = 0;
    bool m_sortFallsBackToEntity = false;
  };

  DRW::Version m_version = DRW::UNKNOWNV;
  std::uint32_t m_tableHandle = DRW::NoHandle;
  std::uint32_t m_sourceOffset = 0;
  std::uint64_t m_sourceMapOrdinal = 0;
  DRW_DwgFrameOffsetSpace m_sourceOffsetSpace =
      DRW_DwgFrameOffsetSpace::Unknown;
  bool m_hasSourceLocation = false;
  bool m_complete = false;
  int m_encodedType = 0;
  int m_resolvedType = 0;
  UTF8STRING m_recordName;
  UTF8STRING m_className;
  std::optional<std::uint64_t> m_classStreamOrdinal;
  std::uint32_t m_blockOwnerHandle = DRW::NoHandle;
  std::vector<Entry> m_entries;
};

//! Parser-owned, ordered FIELDLIST soft-pointer membership.
//!
//! A null slot is a valid counted position and therefore remains in the
//! receipt even though it creates no source-graph edge.
struct DRW_DwgFieldListMembership {
  struct Entry {
    std::uint32_t m_fieldHandle = DRW::NoHandle;
    std::uint32_t m_ordinal = 0;
  };

  DRW::Version m_version = DRW::UNKNOWNV;
  std::uint32_t m_listHandle = DRW::NoHandle;
  std::uint32_t m_sourceOffset = 0;
  std::uint64_t m_sourceMapOrdinal = 0;
  DRW_DwgFrameOffsetSpace m_sourceOffsetSpace =
      DRW_DwgFrameOffsetSpace::Unknown;
  bool m_hasSourceLocation = false;
  bool m_complete = false;
  int m_encodedType = 0;
  int m_resolvedType = 0;
  UTF8STRING m_recordName;
  UTF8STRING m_className;
  std::optional<std::uint64_t> m_classStreamOrdinal;
  std::vector<Entry> m_entries;
};

//! A named parser contract for a typed object-handle field.
enum class DRW_DwgTypedReferenceField : std::uint8_t {
  Unknown,
  DictionaryDefault
};

//! Parser-owned reference from one fully decoded typed DWG object field.
struct DRW_DwgTypedReference {
  DRW::Version m_version = DRW::UNKNOWNV;
  std::uint32_t m_sourceHandle = DRW::NoHandle;
  std::uint32_t m_sourceOffset = 0;
  std::uint64_t m_sourceMapOrdinal = 0;
  DRW_DwgFrameOffsetSpace m_sourceOffsetSpace =
      DRW_DwgFrameOffsetSpace::Unknown;
  bool m_hasSourceLocation = false;
  bool m_complete = false;
  int m_encodedType = 0;
  int m_resolvedType = 0;
  std::optional<std::uint64_t> m_classStreamOrdinal;
  DRW_DwgTypedReferenceField m_field = DRW_DwgTypedReferenceField::Unknown;
  DRW::DwgHandleReferenceCode m_referenceCode = DRW::DwgNullReference;
  std::uint32_t m_targetHandle = DRW::NoHandle;
};

//! Stable HANDLE-map identity for a DWG object/entity frame.
//!
//! Semantic receipts use this compact value rather than exposing the reader's
//! private source-frame bookkeeping type.  A receipt is complete only when
//! every referenced source has a HANDLE-map location.
struct DRW_DwgSourceFrame {
  std::uint32_t m_handle = DRW::NoHandle;
  std::uint32_t m_sourceOffset = 0;
  std::uint64_t m_sourceMapOrdinal = 0;
  DRW_DwgFrameOffsetSpace m_sourceOffsetSpace =
      DRW_DwgFrameOffsetSpace::Unknown;
  bool m_hasSourceLocation = false;
};

//! Parser-owned, ordered reachability contract for one modern BLOCK_RECORD.
//!
//! R2004+ BLOCK_RECORD.entMap supplies the authoritative source order.  This
//! receipt is published only after the matching BLOCK scope is fully staged.
struct DRW_DwgBlockReachability {
  DRW::Version m_version = DRW::UNKNOWNV;
  bool m_complete = false;
  DRW_DwgSourceFrame m_blockRecord;
  DRW_DwgSourceFrame m_block;
  DRW_DwgSourceFrame m_endBlock;
  std::vector<DRW_DwgSourceFrame> m_entities;
};

//! Lossless carrier for non-object DWG data sections preserved byte-for-byte.
struct DRW_VbaProjectSection {
  // The optional-section layout is only partially specified.  Keep the
  // bounded views used by the cross-project readers without interpreting
  // the opaque project container itself.
  std::vector<std::uint8_t> m_header;
  std::vector<std::uint8_t> m_projectData;
  bool m_hasTrailingValue = false;
  std::int32_t m_trailingValue = 0;
};

struct DRW_RawDwgSection {
  UTF8STRING m_name;
  DRW::Version m_version = DRW::UNKNOWNV;
  std::vector<std::uint8_t> m_data;
  // R2007 section-map metadata. Encoding 4 is the current writer default;
  // captured sections retain 1 or 4 so replay can preserve RS layout.
  std::uint64_t m_encoding = 4;
  std::uint64_t m_encrypted = 0;
  std::uint64_t m_maxSize = 0;
  bool m_hasVbaProjectView = false;
  DRW_VbaProjectSection m_vbaProject;
};

//! Populate the bounded view for an AcDb:VBAProject section.
void populateVbaProjectSectionView(DRW_RawDwgSection &section);

//! Lossless DXF passthrough carrier (slice A1) for an OBJECTS-section object
//! that libdxfrw does not model as a typed DXF object. Each group is retained
//! with its typed value; rawValues, when populated by an ASCII reader, retain
//! the original value spelling for same-format replay. Binary writers use the
//! typed value.
//! String-valued groups retain source-encoding bytes (not semantic UTF-8),
//! including XDATA strings; version-bound raw replay must not transcode them.
struct DRW_RawDxfObject {
  UTF8STRING name;                 /*!< object type name (the code-0 string) */
  std::uint32_t handle = 0;        /*!< code 5 (for dedup vs typed writers) */
  std::uint32_t parentHandle = 0;  /*!< code 330 owner */
  std::vector<DRW_Variant> groups; /*!< every (code,value) pair */
  std::vector<UTF8STRING>
      rawValues; /*!< parallel source spellings, ASCII only */
  // Binary captures retain matching empty placeholders in rawValues. This
  // flag distinguishes those from actual empty ASCII fields.
  bool hasRawValues = false;
  DRW::Version m_version =
      DRW::UNKNOWNV; /*!< source DXF version, if captured */
  // Shared logical-byte owner for captured string copies. Copies made by a
  // consumer callback share this lease; they are not charged as library-owned
  // duplicate destinations. This read-time lease tracks library-owned parsed
  // copies only: the public fields remain mutable, so caller changes or copies
  // retained after callback publication are caller-owned and are not monitored
  // or charged to the source read's retained-byte budget. It counts logical
  // string bytes, not allocator capacity or RSS.
  std::shared_ptr<DxfReadRetainedLease> m_retainedTextLease;
};

//! Lossless carrier for a DXF section that libdxfrw does not model.
//! Comments are intentionally excluded by the reader's documented section
//! comment policy; groups retain typed values and ASCII source spellings.
struct DRW_RawDxfSection {
  UTF8STRING m_name;
  std::vector<DRW_Variant> m_groups;
  std::vector<UTF8STRING> m_rawValues;
  bool m_hasRawValues = false;
  DRW::Version m_version = DRW::UNKNOWNV;
  // The shared lease covers library-owned parsed text copies only. These
  // public vectors can be changed or copied by the consumer after callback
  // publication; such caller-owned growth is outside the read budget and is
  // not monitored for allocator capacity or RSS.
  std::shared_ptr<DxfReadRetainedLease> m_retainedTextLease;
};

//! GROUP object (fixed type 72), carrying a named set of entity handles.
class DRW_Group : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::int16_t kDwgFixedType = 72;
  static constexpr std::size_t kMaxEntityHandles = 100000;

  DRW_Group() { reset(); }
  void reset();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool validatePayloadFields() const;
  [[nodiscard]] bool validateDxf() const;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  UTF8STRING m_description;
  bool m_isUnnamed = false;
  bool m_selectable = true;
  std::vector<std::uint32_t> m_entityHandles;

  //! True only after parseDwg decoded every member in the counted DWG list.
  [[nodiscard]] bool hasCompleteDwgEntityHandles() const noexcept {
    return m_dwgEntityHandlesComplete;
  }

private:
  bool m_dwgEntityHandlesComplete = false;
  // Logical bytes retained by DXF-parsed GROUP description text only.
  std::size_t m_dxfRetainedDescriptionBytes = 0u;
};

struct DRW_LightListEntry {
  std::uint32_t m_handle = 0;
  UTF8STRING m_name;
};

//! LIGHTLIST object (AcDbLightList), carrying named LIGHT references.
class DRW_LightList : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint32_t kMaxLightCount = 100000;

  DRW_LightList() { reset(); }
  DRW_LightList(const DRW_LightList& o): DRW_TableEntry(o),
      m_classVersion(o.m_classVersion), m_lightCount(o.m_lightCount),
      m_lights(o.m_lights), m_dxfInLightListSubclass(false),
      m_dxfPendingLightHandle(0), m_dxfSeen90(0),
      m_dxfRetainedLightNameBytes(o.m_dxfRetainedLightNameBytes) {
    tType = o.tType;
  }
  DRW_LightList& operator=(const DRW_LightList& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_classVersion = o.m_classVersion;
      m_lightCount = o.m_lightCount;
      m_lights = o.m_lights;
      m_dxfInLightListSubclass = false;
      m_dxfPendingLightHandle = 0;
      m_dxfSeen90 = 0;
      m_dxfRetainedLightNameBytes = o.m_dxfRetainedLightNameBytes;
      tType = o.tType;
    }
    return *this;
  }
  void reset();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  std::uint32_t m_classVersion = 0;
  std::uint32_t m_lightCount = 0;
  std::vector<DRW_LightListEntry> m_lights;

private:
  bool m_dxfInLightListSubclass = false;
  std::uint32_t m_dxfPendingLightHandle = 0;
  std::uint32_t m_dxfSeen90 = 0;
  std::size_t m_dxfRetainedLightNameBytes{0u};
};

//! LAYERFILTER (AcDbLayerFilter), carrying the names in a layer filter.
class DRW_LayerFilter : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint32_t kMaxNameCount = 100000;

  DRW_LayerFilter() { reset(); }
  void reset() {
    DRW_TableEntry::reset();
    tType = DRW::LAYERFILTER;
    m_nameCount = 0;
    m_names.clear();
    m_dxfRetainedNameBytes = 0u;
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  std::uint32_t m_nameCount = 0;
  std::vector<UTF8STRING> m_names;

private:
  std::size_t m_dxfRetainedNameBytes{0u};
};

struct DRW_DataLinkCustomData {
  std::uint32_t m_targetHandle = 0;
  UTF8STRING m_text;
  std::size_t m_dxfRetainedTextBytes{0u};
};

//! DATALINK (AcDbDataLink), preserving external-data connection metadata.
class DRW_DataLink : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::int32_t kMaxCustomDataCount = 100000;

  DRW_DataLink() { reset(); }
  DRW_DataLink(const DRW_DataLink& o): DRW_TableEntry(o),
      m_dataAdapter(o.m_dataAdapter), m_description(o.m_description),
      m_tooltip(o.m_tooltip), m_connectionString(o.m_connectionString),
      m_option(o.m_option), m_updateOption(o.m_updateOption),
      m_value92(o.m_value92), m_year(o.m_year), m_month(o.m_month),
      m_day(o.m_day), m_hour(o.m_hour), m_minute(o.m_minute),
      m_seconds(o.m_seconds), m_milliseconds(o.m_milliseconds),
      m_pathOption(o.m_pathOption), m_value93(o.m_value93),
      m_updateStatus(o.m_updateStatus),
      m_customDataCount(o.m_customDataCount), m_customData(o.m_customData),
      m_hardOwnerHandle(o.m_hardOwnerHandle), m_dxfInDataLinkBody(false),
      m_dxfInCustomData(false), m_dxfDataAdapterSeen(false),
      m_dxfPendingTargetHandle(0), m_dxfHasPendingTargetHandle(false),
      m_dxfRetainedTextBytes(o.m_dxfRetainedTextBytes) {
    tType = o.tType;
  }
  DRW_DataLink& operator=(const DRW_DataLink& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_dataAdapter = o.m_dataAdapter;
      m_description = o.m_description;
      m_tooltip = o.m_tooltip;
      m_connectionString = o.m_connectionString;
      m_option = o.m_option;
      m_updateOption = o.m_updateOption;
      m_value92 = o.m_value92;
      m_year = o.m_year;
      m_month = o.m_month;
      m_day = o.m_day;
      m_hour = o.m_hour;
      m_minute = o.m_minute;
      m_seconds = o.m_seconds;
      m_milliseconds = o.m_milliseconds;
      m_pathOption = o.m_pathOption;
      m_value93 = o.m_value93;
      m_updateStatus = o.m_updateStatus;
      m_customDataCount = o.m_customDataCount;
      m_customData = o.m_customData;
      m_hardOwnerHandle = o.m_hardOwnerHandle;
      m_dxfInDataLinkBody = false;
      m_dxfInCustomData = false;
      m_dxfDataAdapterSeen = false;
      m_dxfPendingTargetHandle = 0;
      m_dxfHasPendingTargetHandle = false;
      m_dxfRetainedTextBytes = o.m_dxfRetainedTextBytes;
      tType = o.tType;
    }
    return *this;
  }
  void reset() {
    DRW_TableEntry::reset();
    tType = DRW::DATALINK;
    m_dataAdapter.clear();
    m_description.clear();
    m_tooltip.clear();
    m_connectionString.clear();
    m_option = 0;
    m_updateOption = 0;
    m_value92 = 0;
    m_year = 0;
    m_month = 0;
    m_day = 0;
    m_hour = 0;
    m_minute = 0;
    m_seconds = 0;
    m_milliseconds = 0;
    m_pathOption = 0;
    m_value93 = 0;
    m_updateStatus.clear();
    m_customDataCount = 0;
    m_customData.clear();
    m_hardOwnerHandle = 0;
    m_dxfInDataLinkBody = false;
    m_dxfInCustomData = false;
    m_dxfDataAdapterSeen = false;
    m_dxfPendingTargetHandle = 0;
    m_dxfHasPendingTargetHandle = false;
    m_dxfRetainedTextBytes.fill(0u);
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  UTF8STRING m_dataAdapter;
  UTF8STRING m_description;
  UTF8STRING m_tooltip;
  UTF8STRING m_connectionString;
  std::int32_t m_option = 0;
  std::int32_t m_updateOption = 0;
  std::int32_t m_value92 = 0;
  std::int16_t m_year = 0;
  std::int16_t m_month = 0;
  std::int16_t m_day = 0;
  std::int16_t m_hour = 0;
  std::int16_t m_minute = 0;
  std::int16_t m_seconds = 0;
  std::int16_t m_milliseconds = 0;
  std::int16_t m_pathOption = 0;
  std::int32_t m_value93 = 0;
  UTF8STRING m_updateStatus;
  std::int32_t m_customDataCount = 0;
  std::vector<DRW_DataLinkCustomData> m_customData;
  std::uint32_t m_hardOwnerHandle = 0;

private:
  bool m_dxfInDataLinkBody = false;
  bool m_dxfInCustomData = false;
  bool m_dxfDataAdapterSeen = false;
  std::uint32_t m_dxfPendingTargetHandle = 0;
  bool m_dxfHasPendingTargetHandle = false;
  // Adapter, description, tooltip, connection string, update status.
  std::array<std::size_t, 5> m_dxfRetainedTextBytes{};
};

//! GEOMAPIMAGE (AcDbGeomapImage), the LiveMap raster image object.
class DRW_GeoMapImage : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_GeoMapImage() { reset(); }
  DRW_GeoMapImage(const DRW_GeoMapImage& o): DRW_TableEntry(o),
      m_classVersion(o.m_classVersion), m_insertionPoint(o.m_insertionPoint),
      m_imageSize(o.m_imageSize), m_displayProps(o.m_displayProps),
      m_clipping(o.m_clipping), m_brightness(o.m_brightness),
      m_contrast(o.m_contrast), m_fade(o.m_fade),
      m_dxfInGeoMapImageBody(false) {
    tType = o.tType;
  }
  DRW_GeoMapImage& operator=(const DRW_GeoMapImage& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_classVersion = o.m_classVersion;
      m_insertionPoint = o.m_insertionPoint;
      m_imageSize = o.m_imageSize;
      m_displayProps = o.m_displayProps;
      m_clipping = o.m_clipping;
      m_brightness = o.m_brightness;
      m_contrast = o.m_contrast;
      m_fade = o.m_fade;
      m_dxfInGeoMapImageBody = false;
      tType = o.tType;
    }
    return *this;
  }
  void reset() {
    DRW_TableEntry::reset();
    tType = DRW::GEOMAPIMAGE;
    m_classVersion = 0;
    m_insertionPoint = DRW_Coord();
    m_imageSize = DRW_Coord();
    m_displayProps = 0;
    m_clipping = false;
    m_brightness = 0;
    m_contrast = 0;
    m_fade = 0;
    m_dxfInGeoMapImageBody = false;
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  std::int32_t m_classVersion = 0;
  DRW_Coord m_insertionPoint;
  DRW_Coord m_imageSize;
  std::uint16_t m_displayProps = 0;
  bool m_clipping = false;
  std::uint8_t m_brightness = 0;
  std::uint8_t m_contrast = 0;
  std::uint8_t m_fade = 0;

private:
  bool m_dxfInGeoMapImageBody = false;
};

//! IMAGEDEF_REACTOR object.  The image relationship is carried by object
//! ownership.
class DRW_ImageDefinitionReactor : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::int32_t kMaxClassVersion = 10;

  // File-local custom class ordinal; matches dwgWriter bootstrap (532).
  static constexpr std::uint16_t kDwgClassNum = 532;

  DRW_ImageDefinitionReactor() { reset(); }
  void reset();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  std::int32_t m_classVersion = 0;
};

//! SPATIAL_FILTER object used for clipped external references.
class DRW_SpatialFilter : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 507;
  // libreDWG dwg2.spec: SPATIAL_FILTER num_clip_verts <= 10000.
  static constexpr std::size_t kMaxBoundaryPoints = 10000;

  DRW_SpatialFilter() { reset(); }
  DRW_SpatialFilter(const DRW_SpatialFilter& o): DRW_TableEntry(o),
      m_dxfCode40Values(), m_boundaryPoints(o.m_boundaryPoints),
      m_normal(o.m_normal), m_origin(o.m_origin),
      m_displayBoundary(o.m_displayBoundary),
      m_clipFrontPlane(o.m_clipFrontPlane),
      m_clipBackPlane(o.m_clipBackPlane), m_frontDistance(o.m_frontDistance),
      m_backDistance(o.m_backDistance),
      m_inverseInsertTransform(o.m_inverseInsertTransform),
      m_insertTransform(o.m_insertTransform) {
    tType = o.tType;
  }
  DRW_SpatialFilter& operator=(const DRW_SpatialFilter& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_dxfCode40Values.clear();
      m_boundaryPoints = o.m_boundaryPoints;
      m_normal = o.m_normal;
      m_origin = o.m_origin;
      m_displayBoundary = o.m_displayBoundary;
      m_clipFrontPlane = o.m_clipFrontPlane;
      m_clipBackPlane = o.m_clipBackPlane;
      m_frontDistance = o.m_frontDistance;
      m_backDistance = o.m_backDistance;
      m_inverseInsertTransform = o.m_inverseInsertTransform;
      m_insertTransform = o.m_insertTransform;
      m_dxfBoundaryPointYSeen.clear();
      m_dxfBoundaryPointCount = 0;
      m_dxfFlagsSeenMask = 0;
      m_dxfBoundaryPointCountSeen = false;
      m_dxfBackDistanceSeen = false;
      tType = o.tType;
    }
    return *this;
  }
  void reset();
  [[nodiscard]] bool hasValidDxfFields() const;
  [[nodiscard]] bool hasValidDxfSourceFields() const;

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;
  std::vector<double> m_dxfCode40Values;
  std::vector<std::uint8_t> m_dxfBoundaryPointYSeen;
  std::int32_t m_dxfBoundaryPointCount = 0;
  std::uint8_t m_dxfFlagsSeenMask = 0;
  bool m_dxfBoundaryPointCountSeen = false;
  bool m_dxfBackDistanceSeen = false;

public:
  std::vector<DRW_Coord> m_boundaryPoints;
  DRW_Coord m_normal{0.0, 0.0, 1.0};
  DRW_Coord m_origin{0.0, 0.0, 0.0};
  bool m_displayBoundary = false;
  bool m_clipFrontPlane = false;
  bool m_clipBackPlane = false;
  double m_frontDistance = 0.0;
  double m_backDistance = 0.0;
  std::vector<double> m_inverseInsertTransform;
  std::vector<double> m_insertTransform;
};

struct DRW_GeoMeshPoint {
  DRW_Coord m_source;
  DRW_Coord m_destination;
};

struct DRW_GeoMeshFace {
  std::int32_t m_index1 = 0;
  std::int32_t m_index2 = 0;
  std::int32_t m_index3 = 0;
};

//! CIVIC3D_DATA values used only by the archival GEODATA DXF v1 grammar.
//! Kept separate from similarly numbered modern GEODATA fields.
struct DRW_GeoDataLegacyCivil {
  bool m_present = false;
  bool m_transformLocalToGrid = false;
  std::array<double, 2> m_localReferencePoint{{0.0, 0.0}};
  std::array<double, 2> m_gridReferencePoint{{0.0, 0.0}};
  std::int32_t m_localReferencePointNumber = 0;
  std::int32_t m_gridReferencePointNumber = 0;
  bool m_useRotationPoints = false;
  std::array<double, 2> m_localRotationPoint{{0.0, 0.0}};
  std::array<double, 2> m_gridRotationPoint{{0.0, 0.0}};
  double m_rotationAngle = 0.0;
  double m_gridAzimuth = 0.0;
  std::int32_t m_scaleEstimationMethod = 0;
  // The 2009 table calls group 141 "Scale estimation method"; later Autodesk
  // tables call it "User specified scale factor". Preserve without inference.
  double m_ambiguous141Value = 0.0;
  bool m_enableSeaLevelCorrection = false;
  double m_seaLevelElevation = 0.0;
  double m_coordinateProjectionRadius = 0.0;
};

//! Values unique to the archival GEODATA DXF v1 field map.
//! Group 70 is retained as uninterpreted flags because Autodesk's 2009 DXF
//! reference conflicts with later tables and current peer implementations.
struct DRW_GeoDataLegacy {
  std::int32_t m_controlFlags = 0;
  std::array<double, 3> m_geolocation{{0.0, 0.0, 0.0}};
  DRW_Coord m_secondWcsPoint;
  double m_northDirectionAngle = 0.0;
  std::array<double, 3> m_drawingScales{{1.0, 1.0, 1.0}};
  double m_mapUnits = 0.0;
  std::int32_t m_dwgUnits = 0;
  UTF8STRING m_coordinateSystemDescription;
  UTF8STRING m_coordinateSystemDatum;
  DRW_GeoDataLegacyCivil m_civil;
};

//! DWG-only CIVIL tail carried by GEODATA in AC1021 and AC1024.
//! The fields are retained in wire order; several are undocumented reserved
//! values and are deliberately not interpreted or folded into the main body.
struct DRW_GeoCivilTail {
  bool m_present = false;
  bool m_hasCivilData = true;
  bool m_reservedFlag1 = false;
  // ODA §20.4.78 order: Reference Y, Reference X, Reference Y, Reference X.
  std::array<double, 4> m_referenceCoordinates{{0.0, 0.0, 0.0, 0.0}};
  std::array<std::int32_t, 2> m_reservedLongs{{0, 0}};
  // Two 2RD pairs, retained as X/Y scalar slots in their original order.
  std::array<double, 4> m_reservedCoordinates{{0.0, 0.0, 0.0, 0.0}};
  bool m_reservedFlag2 = false;
  double m_northDirectionDegrees = 0.0;
  double m_northDirectionRadians = 0.0;
  std::int32_t m_scaleEstimationMethod = 1;
  double m_userSpecifiedScaleFactor = 1.0;
  bool m_enableSeaLevelCorrection = false;
  double m_seaLevelElevation = 0.0;
  double m_coordinateProjectionRadius = 0.0;
};

//! GEODATA object carrying drawing geolocation metadata.
class DRW_GeoData : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 506;
  static constexpr std::int32_t kMaxMeshItems = 50000;

  DRW_GeoData() { reset(); }
  DRW_GeoData(const DRW_GeoData& o): DRW_TableEntry(o),
      m_dxfInGeoDataSubclass(false), m_dxfSourcePointIndex(0),
      m_dxfTargetPointIndex(0), m_dxfFaceIndex(0), m_version(o.m_version),
      m_hostBlockHandle(o.m_hostBlockHandle),
      m_coordinatesType(o.m_coordinatesType), m_designPoint(o.m_designPoint),
      m_referencePoint(o.m_referencePoint), m_upDirection(o.m_upDirection),
      m_northDirection(o.m_northDirection),
      m_horizontalUnitScale(o.m_horizontalUnitScale),
      m_verticalUnitScale(o.m_verticalUnitScale),
      m_horizontalUnits(o.m_horizontalUnits),
      m_verticalUnits(o.m_verticalUnits),
      m_scaleEstimationMethod(o.m_scaleEstimationMethod),
      m_userSpecifiedScaleFactor(o.m_userSpecifiedScaleFactor),
      m_enableSeaLevelCorrection(o.m_enableSeaLevelCorrection),
      m_seaLevelElevation(o.m_seaLevelElevation),
      m_coordinateProjectionRadius(o.m_coordinateProjectionRadius),
      m_coordinateSystemDefinition(o.m_coordinateSystemDefinition),
      m_geoRssTag(o.m_geoRssTag), m_observationFromTag(o.m_observationFromTag),
      m_observationToTag(o.m_observationToTag),
      m_observationCoverageTag(o.m_observationCoverageTag),
      m_points(o.m_points), m_faces(o.m_faces), m_civilTail(o.m_civilTail),
      m_legacy(o.m_legacy),
      m_dxfRetainedTextBytes(o.m_dxfRetainedTextBytes) {
    tType = o.tType;
  }
  DRW_GeoData& operator=(const DRW_GeoData& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_dxfInGeoDataSubclass = false;
      m_dxfInLegacyCivilData = false;
      m_dxfSourcePointIndex = 0;
      m_dxfTargetPointIndex = 0;
      m_dxfFaceIndex = 0;
      m_version = o.m_version;
      m_hostBlockHandle = o.m_hostBlockHandle;
      m_coordinatesType = o.m_coordinatesType;
      m_designPoint = o.m_designPoint;
      m_referencePoint = o.m_referencePoint;
      m_upDirection = o.m_upDirection;
      m_northDirection = o.m_northDirection;
      m_horizontalUnitScale = o.m_horizontalUnitScale;
      m_verticalUnitScale = o.m_verticalUnitScale;
      m_horizontalUnits = o.m_horizontalUnits;
      m_verticalUnits = o.m_verticalUnits;
      m_scaleEstimationMethod = o.m_scaleEstimationMethod;
      m_userSpecifiedScaleFactor = o.m_userSpecifiedScaleFactor;
      m_enableSeaLevelCorrection = o.m_enableSeaLevelCorrection;
      m_seaLevelElevation = o.m_seaLevelElevation;
      m_coordinateProjectionRadius = o.m_coordinateProjectionRadius;
      m_coordinateSystemDefinition = o.m_coordinateSystemDefinition;
      m_geoRssTag = o.m_geoRssTag;
      m_observationFromTag = o.m_observationFromTag;
      m_observationToTag = o.m_observationToTag;
      m_observationCoverageTag = o.m_observationCoverageTag;
      m_points = o.m_points;
      m_faces = o.m_faces;
      m_civilTail = o.m_civilTail;
      m_legacy = o.m_legacy;
      m_dxfRetainedTextBytes = o.m_dxfRetainedTextBytes;
      tType = o.tType;
    }
    return *this;
  }
  void reset();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;
  bool m_dxfInGeoDataSubclass = false;
  bool m_dxfInLegacyCivilData = false;
  std::size_t m_dxfSourcePointIndex = 0;
  std::size_t m_dxfTargetPointIndex = 0;
  std::size_t m_dxfFaceIndex = 0;

public:
  std::int32_t m_version = 0;
  std::uint32_t m_hostBlockHandle = 0;
  std::int16_t m_coordinatesType = 0;
  DRW_Coord m_designPoint;
  DRW_Coord m_referencePoint;
  DRW_Coord m_upDirection{0.0, 0.0, 1.0};
  DRW_Coord m_northDirection{0.0, 1.0, 0.0};
  double m_horizontalUnitScale = 1.0;
  double m_verticalUnitScale = 1.0;
  std::int32_t m_horizontalUnits = 0;
  std::int32_t m_verticalUnits = 0;
  std::int32_t m_scaleEstimationMethod = 0;
  double m_userSpecifiedScaleFactor = 1.0;
  bool m_enableSeaLevelCorrection = false;
  double m_seaLevelElevation = 0.0;
  double m_coordinateProjectionRadius = 0.0;
  UTF8STRING m_coordinateSystemDefinition;
  UTF8STRING m_geoRssTag;
  UTF8STRING m_observationFromTag;
  UTF8STRING m_observationToTag;
  UTF8STRING m_observationCoverageTag;
  std::vector<DRW_GeoMeshPoint> m_points;
  std::vector<DRW_GeoMeshFace> m_faces;
  DRW_GeoCivilTail m_civilTail;
  DRW_GeoDataLegacy m_legacy;

private:
  enum class DxfRetainedTextField : std::size_t {
    CoordinateSystemDefinition,
    GeoRssTag,
    ObservationFromTag,
    ObservationToTag,
    ObservationCoverageTag,
    LegacyCoordinateSystemDescription,
    LegacyCoordinateSystemDatum,
    Count
  };
  std::array<std::size_t,
             static_cast<std::size_t>(DxfRetainedTextField::Count)>
      m_dxfRetainedTextBytes{};
};

struct DRW_TableGeometryContent {
  DRW_Coord m_topLeft;
  DRW_Coord m_center;
  double m_contentWidth = 0.0;
  double m_contentHeight = 0.0;
  double m_width = 0.0;
  double m_height = 0.0;
  std::int32_t m_unknown = 0;
};

struct DRW_TableGeometryCell {
  std::int32_t m_flags = 0;
  double m_widthWithGap = 0.0;
  double m_heightWithGap = 0.0;
  std::uint32_t m_unknownHandle = 0;
  std::vector<DRW_TableGeometryContent> m_contents;
};

//! TABLEGEOMETRY object introduced for table geometry cache data.
class DRW_TableGeometry : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::int32_t kMaxRows = 5000;
  static constexpr std::int32_t kMaxColumns = 5000;
  static constexpr std::int32_t kMaxCells = 10000;
  static constexpr std::int32_t kMaxContentGeometry = 10000;

  DRW_TableGeometry() { reset(); }
  void reset() {
    tType = DRW::TABLEGEOMETRY;
    m_rowCount = 0;
    m_columnCount = 0;
    m_cellCount = 0;
    m_cells.clear();
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  std::int32_t m_rowCount = 0;
  std::int32_t m_columnCount = 0;
  std::int32_t m_cellCount = 0;
  std::vector<DRW_TableGeometryCell> m_cells;
};

//! Raw DWG CMC provenance kept alongside a class-specific scalar projection.
struct DRW_DwgCmcColorMetadata {
  bool present = false;
  std::uint8_t method = 0;
  std::uint8_t flags = 0;
  std::uint16_t rawIndex = 0;
  std::int32_t rgb24 = -1;
  bool hasRgb = false;
  UTF8STRING colorName;
  UTF8STRING bookName;
};

//! Class to handle dimstyle entries
/*!
 *  Class to handle dim style symbol table entries
 *  @author Rallaz
 */
class DRW_Dimstyle : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_Dimstyle() { reset(); }
  ~DRW_Dimstyle() { clearVars(); }

  // Rule-of-Five: vars owns raw DRW_Variant* (deleted in the dtor), so a
  // default shallow copy would double-free. The base DRW_TableEntry copy
  // ctor already deep-copies extData; here we additionally deep-copy vars
  // and copy the POD fields. (Phase 3A.0)
  DRW_Dimstyle(const DRW_Dimstyle &o) : DRW_TableEntry(o) {
    copyPodFrom(o);
    deepCopyVarsFrom(o);
  }
  DRW_Dimstyle &operator=(const DRW_Dimstyle &o) {
    if (this != &o) {
      // Build the complete replacement before touching this object so
      // allocation failure preserves the existing style.
      DRW_Dimstyle copy(o);
      *this = std::move(copy);
    }
    return *this;
  }
  DRW_Dimstyle(DRW_Dimstyle &&o) noexcept : DRW_TableEntry(std::move(o)) {
    copyPodFrom(o, false);
    moveStringFieldsFrom(o);
    vars = std::move(o.vars);
    o.vars.clear();
  }
  DRW_Dimstyle &operator=(DRW_Dimstyle &&o) noexcept {
    if (this != &o) {
      clearVars();
      DRW_TableEntry::operator=(std::move(o));
      copyPodFrom(o, false);
      moveStringFieldsFrom(o);
      vars = std::move(o.vars);
      o.vars.clear();
    }
    return *this;
  }

  void add(const std::string &key, int code, int value) {
    replaceVar(key, std::make_unique<DRW_Variant>(code, value));
  }
  void add(const std::string &key, int code, double value) {
    replaceVar(key, std::make_unique<DRW_Variant>(code, value));
  }
  void add(const std::string &key, int code, std::string value) {
    replaceVar(
        key, std::make_unique<DRW_Variant>(code, UTF8STRING(std::move(value))));
  }
  DRW_Variant *get(const std::string &key) const {
    auto it = vars.find(key);
    return (it != vars.end()) ? it->second : nullptr;
  }

  // Populate the vars map from the parsed/struct fields so the LibreCAD
  // createDimStyle consumer (which reads $DIM* keys) receives imported
  // values. Idempotent: never clobbers a key already present. (Phase 3A.0)
  void syncStructToVars();
  /// Record the accepted DXF source revision and migrate legacy-only DIMUNIT /
  /// DIMFIT values into their modern split fields. Explicit modern fields,
  /// including zero, take precedence over obsolete tags.
  void finalizeDxfDimensionVariables(DRW::Version sourceVersion);
  /// API-created records normally treat legacy fields as authoritative when
  /// written to pre-R2000 DXF. Set this when populating modern split fields
  /// directly and expecting the writer to perform a checked legacy downgrade.
  void setDxfDimensionSourceVersion(DRW::Version sourceVersion) noexcept {
    m_dxfDimensionSourceVersion = sourceVersion;
  }
  /// Resolve a lossless legacy representation for a pre-R2000 DXF target.
  /// Returns false for modern combinations that the legacy enums cannot hold.
  bool resolveLegacyDxfDimensionVariables(int& unit, int& fit) const;
  bool hasDxfDimensionLinetypeReference(int code) const noexcept {
    switch (code) {
    case 345: return (m_dxfDimensionPresence & kDxfDxfDimltype) != 0;
    case 346: return (m_dxfDimensionPresence & kDxfDxfDimltex1) != 0;
    case 347: return (m_dxfDimensionPresence & kDxfDxfDimltex2) != 0;
    default: return false;
    }
  }
  //! The DIMSTYLE DXF contract exposes these colors as ACI/index values only.
  //! Return false if retained DWG CMC detail has no verified DXF projection.
  bool canWriteDxfDimensionColors() const noexcept;

  void reset() {
    tType = DRW::DIMSTYLE;
    dimpost.clear();
    dimapost.clear();
    dimblk.clear();
    dimblk1.clear();
    dimblk2.clear();
    dimasz = dimtxt = dimexe = 0.18;
    dimexo = 0.0625;
    dimgap = dimcen = 0.09;
    dimtxsty = "Standard";
    dimldrblk.clear();
    dimscale = dimlfac = dimtfac = dimfxl = 1.0;
    dimdli = 0.38;
    dimrnd = dimdle = dimtp = dimtm = dimtsz = dimtvp = 0.0;
    dimaltf = 25.4;
    dimtol = dimlim = dimse1 = dimse2 = dimtad = dimzin = 0;
    dimtoh = dimtolj = 1;
    dimalt = dimtofl = dimsah = dimtix = dimsoxd = dimfxlon = 0;
    dimaltd = dimunit = dimaltu = dimalttd = dimlunit = 2;
    dimclrd = dimclre = dimclrt = dimjust = dimupt = 0;
    dimazin = dimaltz = dimaltttz = dimtzin = dimfrac = 0;
    dimtih = dimadec = dimaunit = dimsd1 = dimsd2 = dimtmove = 0;
    dimaltrnd = 0.0;
    dimdec = dimtdec = 4;
    dimfit = dimatfit = 3;
    m_dxfDimensionPresence = 0;
    m_dxfDimensionSourceVersion = DRW::UNKNOWNV;
    dimdsep = '.';
    dimlwd = dimlwe = -2;
    // Phase 3A.1: R2007+/R2010+ members + handle refs.
    dimjogang = 0.0;
    dimtfill = dimtfillclr = dimarcsym = 0;
    dimtfillclrRgb = -1;
    dimtfillclrMethod = dimtfillclrFlags = 0;
    dimtfillclrRawIndex = 0;
    dimtfillclrName.clear();
    dimtfillclrBookName.clear();
    dimColorCmc = {};
    dimtxtdirection = 0;
    dimaltmzf = dimmzf = 1.0;
    dimaltmzs.clear();
    dimmzs.clear();
    dimtxstyH = dimldrblkH = dimblkH = dimblk1H = dimblk2H = dwgHandle{};
    dimltypeH = dimltex1H = dimltex2H = dwgHandle{};
    m_dxfRetainedTextBytes.fill(0u);
    clearVars();
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *hdlBuf = nullptr) const;

  // V12
  UTF8STRING dimpost;  /*!< code 3 */
  UTF8STRING dimapost; /*!< code 4 */
                       /* handle are code 105 */
  UTF8STRING dimblk;   /*!< code 5, code 342 V2000+ */
  UTF8STRING dimblk1;  /*!< code 6, code 343 V2000+ */
  UTF8STRING dimblk2;  /*!< code 7, code 344 V2000+ */
  double dimscale;     /*!< code 40 */
  double dimasz;       /*!< code 41 */
  double dimexo;       /*!< code 42 */
  double dimdli;       /*!< code 43 */
  double dimexe;       /*!< code 44 */
  double dimrnd;       /*!< code 45 */
  double dimdle;       /*!< code 46 */
  double dimtp;        /*!< code 47 */
  double dimtm;        /*!< code 48 */
  double dimfxl;       /*!< code 49 V2007+ */
  double dimtxt;       /*!< code 140 */
  double dimcen;       /*!< code 141 */
  double dimtsz;       /*!< code 142 */
  double dimaltf;      /*!< code 143 */
  double dimlfac;      /*!< code 144 */
  double dimtvp;       /*!< code 145 */
  double dimtfac;      /*!< code 146 */
  double dimgap;       /*!< code 147 */
  double dimaltrnd;    /*!< code 148 V2000+ */
  int dimtol;          /*!< code 71 */
  int dimlim;          /*!< code 72 */
  int dimtih;          /*!< code 73 */
  int dimtoh;          /*!< code 74 */
  int dimse1;          /*!< code 75 */
  int dimse2;          /*!< code 76 */
  int dimtad;          /*!< code 77 */
  int dimzin;          /*!< code 78 */
  int dimazin;         /*!< code 79 V2000+ */
  int dimalt;          /*!< code 170 */
  int dimaltd;         /*!< code 171 */
  int dimtofl;         /*!< code 172 */
  int dimsah;          /*!< code 173 */
  int dimtix;          /*!< code 174 */
  int dimsoxd;         /*!< code 175 */
  int dimclrd;         /*!< code 176 */
  int dimclre;         /*!< code 177 */
  int dimclrt;         /*!< code 178 */
  int dimadec;         /*!< code 179 V2000+ */
  int dimunit;   /*!< code 270 R13+ (obsolete 2000+, use dimlunit & dimfrac) */
  int dimdec;    /*!< code 271 R13+ */
  int dimtdec;   /*!< code 272 R13+ */
  int dimaltu;   /*!< code 273 R13+ */
  int dimalttd;  /*!< code 274 R13+ */
  int dimaunit;  /*!< code 275 R13+ */
  int dimfrac;   /*!< code 276 V2000+ */
  int dimlunit;  /*!< code 277 V2000+ */
  int dimdsep;   /*!< code 278 V2000+ */
  int dimtmove;  /*!< code 279 V2000+ */
  int dimjust;   /*!< code 280 R13+ */
  int dimsd1;    /*!< code 281 R13+ */
  int dimsd2;    /*!< code 282 R13+ */
  int dimtolj;   /*!< code 283 R13+ */
  int dimtzin;   /*!< code 284 R13+ */
  int dimaltz;   /*!< code 285 R13+ */
  int dimaltttz; /*!< code 286 R13+ */
  int dimfit;    /*!< code 287 R13+  (obsolete 2000+, use dimatfit & dimtmove)*/
  int dimupt;    /*!< code 288 R13+ */
  int dimatfit;  /*!< code 289 V2000+ */
  int dimfxlon;  /*!< code 290 V2007+ */
  UTF8STRING dimtxsty;  /*!< code 340 R13+ */
  UTF8STRING dimldrblk; /*!< code 341 V2000+ */
  int dimlwd;           /*!< code 371 V2000+ */
  int dimlwe;           /*!< code 372 V2000+ */
  // Phase 3A.1: R2007+ numeric/color members (dwg.spec:4766-4790).
  // dimfxl/dimfxlon already exist above — do not re-add.
  double dimjogang{0.0}; /*!< code 50  BD  V2007+ */
  int dimtfill{0};       /*!< code 69  BS  V2007+ */
  int dimtfillclr{0};    /*!< code 70  CMC V2007+ */
  int dimarcsym{0};      /*!< code 90  BS  V2007+ */
  // Phase 3A.1: R2010+ members (dwg.spec:4842-4849).
  int dimtxtdirection{0}; /*!< code 295 B   V2010+ */
  double dimaltmzf{1.0};  /*!< BD  V2010+ */
  UTF8STRING dimaltmzs;   /*!< T   V2010+ */
  double dimmzf{1.0};     /*!< BD  V2010+ */
  UTF8STRING dimmzs;      /*!< T   V2010+ */
  // Phase 3A.1: resolved handle-stream refs (mirror DRW_Layer::lTypeH).
  dwgHandle dimtxstyH;  /*!< code 340 */
  dwgHandle dimldrblkH; /*!< code 341 */
  dwgHandle dimblkH;    /*!< code 342 */
  dwgHandle dimblk1H;   /*!< code 343 */
  dwgHandle dimblk2H;   /*!< code 344 */
  dwgHandle dimltypeH;  /*!< code 345 */
  dwgHandle dimltex1H;  /*!< code 346 */
  dwgHandle dimltex2H;  /*!< code 347 */
  std::map<std::string, DRW_Variant *>
      vars; /*!< extra/override variables written after standard fields */
  // DWG-only CMC companions for DIMTFILLCLR. No class-specific DXF mapping
  // is established by the current Autodesk DIMSTYLE reference.
  std::int32_t dimtfillclrRgb{-1};
  std::uint8_t dimtfillclrMethod{0};
  std::uint8_t dimtfillclrFlags{0};
  UTF8STRING dimtfillclrName;
  UTF8STRING dimtfillclrBookName;
  std::uint16_t dimtfillclrRawIndex{0}; /*!< R2004+ reserved CMC BS; ODA requires zero */
  //! DWG CMC provenance aligned with DIMCLRD, DIMCLRE, and DIMCLRT.
  std::array<DRW_DwgCmcColorMetadata, 3> dimColorCmc{};

private:
  static constexpr std::uint8_t kDxfLegacyDimunit = 0x01;
  static constexpr std::uint8_t kDxfLegacyDimfit = 0x02;
  static constexpr std::uint8_t kDxfModernDimfrac = 0x04;
  static constexpr std::uint8_t kDxfModernDimlunit = 0x08;
  static constexpr std::uint8_t kDxfModernDimtmove = 0x10;
  static constexpr std::uint8_t kDxfModernDimatfit = 0x20;
  static constexpr std::uint16_t kDxfDxfDimltype = 0x0040;
  static constexpr std::uint16_t kDxfDxfDimltex1 = 0x0080;
  static constexpr std::uint16_t kDxfDxfDimltex2 = 0x0100;
  std::uint16_t m_dxfDimensionPresence {0};
  DRW::Version m_dxfDimensionSourceVersion {DRW::UNKNOWNV};
  // Live logical text bytes retained by the DIMSTYLE DXF parser fields.
  std::array<std::size_t, 7> m_dxfRetainedTextBytes{};

  bool syncStructToVarsImpl(dxfReader* reader);
  bool syncStructToVarsForDxfRead(dxfReader& reader);

  void clearVars() {
    for (auto &kv : vars)
      delete kv.second;
    vars.clear();
  }
  void replaceVar(const std::string &key,
                  std::unique_ptr<DRW_Variant> replacement) {
    auto it = vars.find(key);
    if (it == vars.end()) {
      auto result = vars.emplace(key, replacement.get());
      if (result.second)
        replacement.release();
      return;
    }
    DRW_Variant *previous = it->second;
    it->second = replacement.release();
    delete previous;
  }

  void deepCopyVarsFrom(const DRW_Dimstyle &o) {
    clearVars();
    try {
      for (const auto &kv : o.vars) {
        std::unique_ptr<DRW_Variant> copy =
            kv.second != nullptr ? std::make_unique<DRW_Variant>(*kv.second)
                                 : nullptr;
        auto result = vars.emplace(kv.first, copy.get());
        if (result.second)
          copy.release();
      }
    } catch (...) {
      clearVars();
      throw;
    }
  }
  // Copy every owned-by-value POD field and optionally the strings (NOT the
  // base members, NOT vars). Move operations skip string copies before their
  // noexcept move helper transfers those owners. New value-type members added
  // in later phases ride this list.
  void copyPodFrom(const DRW_Dimstyle &o, bool copyStrings = true) {
    m_dxfDimensionPresence = o.m_dxfDimensionPresence;
    m_dxfDimensionSourceVersion = o.m_dxfDimensionSourceVersion;
    m_dxfRetainedTextBytes = o.m_dxfRetainedTextBytes;
    if (copyStrings) {
      dimpost = o.dimpost;
      dimapost = o.dimapost;
      dimblk = o.dimblk;
      dimblk1 = o.dimblk1;
      dimblk2 = o.dimblk2;
    }
    dimscale = o.dimscale;
    dimasz = o.dimasz;
    dimexo = o.dimexo;
    dimdli = o.dimdli;
    dimexe = o.dimexe;
    dimrnd = o.dimrnd;
    dimdle = o.dimdle;
    dimtp = o.dimtp;
    dimtm = o.dimtm;
    dimfxl = o.dimfxl;
    dimtxt = o.dimtxt;
    dimcen = o.dimcen;
    dimtsz = o.dimtsz;
    dimaltf = o.dimaltf;
    dimlfac = o.dimlfac;
    dimtvp = o.dimtvp;
    dimtfac = o.dimtfac;
    dimgap = o.dimgap;
    dimaltrnd = o.dimaltrnd;
    dimtol = o.dimtol;
    dimlim = o.dimlim;
    dimtih = o.dimtih;
    dimtoh = o.dimtoh;
    dimse1 = o.dimse1;
    dimse2 = o.dimse2;
    dimtad = o.dimtad;
    dimzin = o.dimzin;
    dimazin = o.dimazin;
    dimalt = o.dimalt;
    dimaltd = o.dimaltd;
    dimtofl = o.dimtofl;
    dimsah = o.dimsah;
    dimtix = o.dimtix;
    dimsoxd = o.dimsoxd;
    dimclrd = o.dimclrd;
    dimclre = o.dimclre;
    dimclrt = o.dimclrt;
    dimadec = o.dimadec;
    dimunit = o.dimunit;
    dimdec = o.dimdec;
    dimtdec = o.dimtdec;
    dimaltu = o.dimaltu;
    dimalttd = o.dimalttd;
    dimaunit = o.dimaunit;
    dimfrac = o.dimfrac;
    dimlunit = o.dimlunit;
    dimdsep = o.dimdsep;
    dimtmove = o.dimtmove;
    dimjust = o.dimjust;
    dimsd1 = o.dimsd1;
    dimsd2 = o.dimsd2;
    dimtolj = o.dimtolj;
    dimtzin = o.dimtzin;
    dimaltz = o.dimaltz;
    dimaltttz = o.dimaltttz;
    dimfit = o.dimfit;
    dimupt = o.dimupt;
    dimatfit = o.dimatfit;
    dimfxlon = o.dimfxlon;
    if (copyStrings) {
      dimtxsty = o.dimtxsty;
      dimldrblk = o.dimldrblk;
    }
    dimlwd = o.dimlwd;
    dimlwe = o.dimlwe;
    // Phase 3A.1 members — all value types, plain member-wise copy.
    dimjogang = o.dimjogang;
    dimtfill = o.dimtfill;
    dimtfillclr = o.dimtfillclr;
    dimtfillclrRgb = o.dimtfillclrRgb;
    dimtfillclrMethod = o.dimtfillclrMethod;
    dimtfillclrFlags = o.dimtfillclrFlags;
    dimtfillclrRawIndex = o.dimtfillclrRawIndex;
    for (std::size_t i = 0; i < dimColorCmc.size(); ++i) {
      dimColorCmc[i].present = o.dimColorCmc[i].present;
      dimColorCmc[i].method = o.dimColorCmc[i].method;
      dimColorCmc[i].flags = o.dimColorCmc[i].flags;
      dimColorCmc[i].rawIndex = o.dimColorCmc[i].rawIndex;
      dimColorCmc[i].rgb24 = o.dimColorCmc[i].rgb24;
      dimColorCmc[i].hasRgb = o.dimColorCmc[i].hasRgb;
      if (copyStrings) {
        dimColorCmc[i].colorName = o.dimColorCmc[i].colorName;
        dimColorCmc[i].bookName = o.dimColorCmc[i].bookName;
      }
    }
    if (copyStrings) {
      dimtfillclrName = o.dimtfillclrName;
      dimtfillclrBookName = o.dimtfillclrBookName;
    }
    dimarcsym = o.dimarcsym;
    dimtxtdirection = o.dimtxtdirection;
    dimaltmzf = o.dimaltmzf;
    if (copyStrings)
      dimaltmzs = o.dimaltmzs;
    dimmzf = o.dimmzf;
    if (copyStrings)
      dimmzs = o.dimmzs;
    dimtxstyH = o.dimtxstyH;
    dimldrblkH = o.dimldrblkH;
    dimblkH = o.dimblkH;
    dimblk1H = o.dimblk1H;
    dimblk2H = o.dimblk2H;
    dimltypeH = o.dimltypeH;
    dimltex1H = o.dimltex1H;
    dimltex2H = o.dimltex2H;
  }
  void moveStringFieldsFrom(DRW_Dimstyle &o) noexcept {
    dimpost = std::move(o.dimpost);
    dimapost = std::move(o.dimapost);
    dimblk = std::move(o.dimblk);
    dimblk1 = std::move(o.dimblk1);
    dimblk2 = std::move(o.dimblk2);
    dimtxsty = std::move(o.dimtxsty);
    dimldrblk = std::move(o.dimldrblk);
    dimaltmzs = std::move(o.dimaltmzs);
    dimmzs = std::move(o.dimmzs);
    dimtfillclrName = std::move(o.dimtfillclrName);
    dimtfillclrBookName = std::move(o.dimtfillclrBookName);
    for (std::size_t i = 0; i < dimColorCmc.size(); ++i) {
      dimColorCmc[i].colorName = std::move(o.dimColorCmc[i].colorName);
      dimColorCmc[i].bookName = std::move(o.dimColorCmc[i].bookName);
    }
    m_dxfRetainedTextBytes = o.m_dxfRetainedTextBytes;
    o.m_dxfRetainedTextBytes.fill(0u);
  }
};

//! One dash, dot, embedded shape, or embedded text item in a line type.
struct DRW_LTypeSegment {
  double length{0.0};
  int complexShapeCode{0};
  dwgHandle styleHandle;
  double xOffset{0.0};
  double yOffset{0.0};
  double scale{1.0};
  double rotation{0.0}; // radians
  int shapeFlags{0};
  UTF8STRING text;
};

//! Class to handle line type entries
/*!
 *  Class to handle line type symbol table entries
 *  @author Rallaz
 */
class DRW_LType : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_LType() { reset(); }

  void reset() {
    tType = DRW::LTYPE;
    desc = "";
    size = 0;
    length = 0.0;
    alignment = 'A';
    pathIdx = 0;
    declaredSize = -1;
    synchronizedLengths.clear();
    hasSynchronizedLengths = false;
    m_dxfRetainedDescBytes = 0u;
    m_dxfRetainedSegmentTextBytes.clear();
    path.clear();
    segments.clear();
    shapeHandles.clear();
    xrefBlockHandle = dwgHandle{};
    DRW_TableEntry::reset();
  }

public:
  void updateValues(const UTF8STRING &lTypeName,
                    const UTF8STRING &ltDescription, int ltSize,
                    double ltLength, const std::vector<double> &ltPath) {
    reset();
    name = lTypeName;
    desc = ltDescription;
    size = ltSize;
    length = ltLength;
    path.clear();
    for (auto it : ltPath) {
      path.push_back(it);
    }
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool validatePayloadFields() const;
  [[nodiscard]] bool validateDxf() const;
  [[nodiscard]] bool update();
  bool encodeDwgResolved(DRW::Version version, dwgBufferW *buf,
                        dwgBufferW *strBuf, dwgBufferW *hdlBuf) const;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *hdlBuf = nullptr) const;
  UTF8STRING desc; /*!< descriptive string, code 3 */
  //    int align;               /*!< align code, always 65 ('A') code 72 */
  int size;      /*!< element number, code 73 */
  double length; /*!< total length of pattern, code 40 */
  int alignment; /*!< alignment code, code 72 (normally 'A') */
  std::vector<double>
      path; /*!< trace, point or space length sequence, code 49 */
  std::vector<DRW_LTypeSegment> segments; /*!< complex segment payload */
  std::vector<std::uint32_t>
      shapeHandles;          /*!< DWG shapefile handles, one per dash */
  // Read-only counts let adapters account for parser-owned vectors that are
  // intentionally kept private, without exposing or mutating their contents.
  // Narrow source/live admission for parsed DXF LTYPE records whose admitted
  // grammar specifies no dash elements. This does not qualify other patterns.
  [[nodiscard]] bool sameSelectedDxfEmptyPatternState(const DRW_LType& other) const noexcept {
    const auto selected=[](const DRW_LType& v) noexcept {
      return v.size==0 && v.pathIdx==0 && v.declaredSize==0 && v.hasSynchronizedLengths
          && v.synchronizedLengths.empty() && v.m_dxfRetainedSegmentTextBytes.empty()
          && v.path.empty() && v.segments.empty() && v.shapeHandles.empty();
    };
    return selected(*this) && selected(other)
        && m_dxfRetainedDescBytes==other.m_dxfRetainedDescBytes;
  }
  [[nodiscard]] std::size_t synchronizedLengthCount() const noexcept {
    return synchronizedLengths.size();
  }
  [[nodiscard]] std::size_t retainedSegmentTextCount() const noexcept {
    return m_dxfRetainedSegmentTextBytes.size();
  }
  dwgHandle xrefBlockHandle; /*!< external-reference block handle in the common
                                stream */
private:
  int pathIdx;
  int declaredSize;
  std::vector<double> synchronizedLengths;
  bool hasSynchronizedLengths {false};
  // Per-field charges pair with the base table-entry lease. They track only
  // strings admitted by DXF parsing, not caller mutations or DWG reads.
  std::size_t m_dxfRetainedDescBytes {0u};
  std::vector<std::size_t> m_dxfRetainedSegmentTextBytes;
};

//! Class to handle layer entries
/*!
 *  Class to handle layer symbol table entries
 *  @author Rallaz
 */
class DRW_Layer : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_Layer() { reset(); }

  void reset() {
    tType = DRW::LAYER;
    lineType = "CONTINUOUS";
    color = 7;                           // default BYLAYER (256)
    plotF = true;                        // default TRUE (plot yes)
    lWeight = DRW_LW_Conv::widthDefault; // default BYDEFAULT (dxf -3, dwg 31)
    color24 = -1;                        // default -1 not set
    colorName.clear();
    m_colorDwgMethod = 0;
    m_hasDwgColorCmcMetadata = false;
    m_colorDwgFlags = 0u;
    m_colorDwgRawIndex = 0u;
    m_colorDwgName.clear();
    m_colorDwgBookName.clear();
    m_dxfRetainedColorNameBytes = 0u;
    m_dxfRetainedLineTypeBytes = 0u;
    m_dxfRetainedPlotStyleHandleBytes = 0u;
    m_dxfRetainedMaterialHandleBytes = 0u;
    handlePlotS.clear();
    handleMaterialS.clear();
    xrefBlockHandle = dwgHandle{};
    plotStyleHandle = dwgHandle{};
    materialHandle = dwgHandle{};
    unknownHandle = dwgHandle{};
    lTypeH = dwgHandle{};
    hasInvalidDxfLineweight = false;
    rawInvalidDxfLineweight = 0;
    m_dwgNameSource.reset();
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool validatePayloadFields() const;
  [[nodiscard]] bool validateDxf() const;

public:
  [[nodiscard]] bool hasDwgNameSource() const noexcept { return m_dwgNameSource != nullptr; }
  // Ownership identity, unlike content equality below. Read-only access does
  // not install evidence or certify the entire record or its containing file.
  [[nodiscard]] std::shared_ptr<const DRW_DwgTableNameSource> dwgNameSource() const noexcept {
    return m_dwgNameSource;
  }
  [[nodiscard]] bool hasUnchangedDwgSourceName() const noexcept {
    return m_dwgNameSource && m_dwgNameSource->matches(handle, name);
  }
  [[nodiscard]] bool hasBoundDwgNameProfile(DRW::Version version, const std::string& page) const noexcept {
    return m_dwgNameSource && m_dwgNameSource->bound(version, page);
  }
  [[nodiscard]] std::uint32_t dwgNameDecodeIssues() const noexcept {
    return m_dwgNameSource ? m_dwgNameSource->decodeIssues() : 0u;
  }
  [[nodiscard]] std::size_t dwgNameSourceRetainedBytes() const noexcept {
    return m_dwgNameSource ? m_dwgNameSource->retainedBytes() : 0u;
  }
  [[nodiscard]] bool sameDwgNameSource(const DRW_Layer& other) const noexcept {
    return m_dwgNameSource && other.m_dwgNameSource
        ? m_dwgNameSource->equal(*other.m_dwgNameSource)
        : m_dwgNameSource == other.m_dwgNameSource;
  }
  //! True when a DXF LAYER group 370 held a value outside its defined range.
  [[nodiscard]] bool hasInvalidDxfLineweightValue() const noexcept {
    return hasInvalidDxfLineweight;
  }
  //! Original group 370 value when hasInvalidDxfLineweightValue() is true.
  [[nodiscard]] int invalidDxfLineweightValue() const noexcept {
    return rawInvalidDxfLineweight;
  }
  //! Raw source DWG CMC method byte; zero means authored/DXF-origin state.
  [[nodiscard]] std::uint8_t colorDwgMethod() const noexcept {
    return m_colorDwgMethod;
  }
  //! True when this LAYER was parsed from an R2004+ DWG CMC value.
  [[nodiscard]] bool hasDwgColorCmcMetadata() const noexcept {
    return m_hasDwgColorCmcMetadata;
  }
  //! Raw R2004+ CMC color-name flags byte (ODA §2.11: bits 0 and 1).
  [[nodiscard]] std::uint8_t colorDwgFlags() const noexcept {
    return m_colorDwgFlags;
  }
  //! Raw leading CMC BS value; reserved and required to be zero in R2004+.
  [[nodiscard]] std::uint16_t colorDwgRawIndex() const noexcept {
    return m_colorDwgRawIndex;
  }
  //! Separate DWG CMC color name, before the public BOOK$ENTRY projection.
  [[nodiscard]] const UTF8STRING& colorDwgName() const noexcept {
    return m_colorDwgName;
  }
  //! Separate DWG CMC color-book name, before the public BOOK$ENTRY projection.
  [[nodiscard]] const UTF8STRING& colorDwgBookName() const noexcept {
    return m_colorDwgBookName;
  }
  //! Explicitly replace a malformed source lineweight with a legal LAYER value.
  bool repairDxfLineweight(DRW_LW_Conv::lineWidth value) noexcept {
    const int lineWeight = static_cast<int>(value);
    const bool valid = (lineWeight >= static_cast<int>(DRW_LW_Conv::width00)
                        && lineWeight <= static_cast<int>(DRW_LW_Conv::width23))
        || value == DRW_LW_Conv::widthDefault;
    if (!valid)
      return false;
    lWeight = value;
    hasInvalidDxfLineweight = false;
    rawInvalidDxfLineweight = 0;
    return true;
  }
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *hdlBuf = nullptr) const;
  /// Resolve the optional DXF group-347 material pointer without consuming it.
  /// Empty text/native zero is absent; text "0" is explicitly null. Nonzero
  /// native aliases must agree, and all supplied handles must fit 32 bits.
  /// This validates representation only; the exporter owns graph closure.
  bool getDxfMaterialHandle(std::uint32_t &handle, bool &present) const noexcept;
  UTF8STRING lineType;  /*!< line type, code 6 */
  int color;            /*!< layer color, code 62 */
  int color24;          /*!< 24-bit color, code 420 */
  UTF8STRING colorName; /*!< color book name, code 430 ("BOOK$ENTRY") */
  bool plotF;           /*!< Plot flag, code 290 */
  enum DRW_LW_Conv::lineWidth lWeight; /*!< layer lineweight, code 370 */
  std::string handlePlotS; /*!< Hard-pointer ID/handle of plotstyle, code 390 */
  std::string handleMaterialS; /*!< Optional material hard pointer, DXF code 347 */
  dwgHandle lTypeH;
  dwgHandle xrefBlockHandle; /*!< external-reference block handle in DWG */
  dwgHandle plotStyleHandle; /*!< plot-style hard pointer in DWG */
  dwgHandle materialHandle;  /*!< material hard pointer in DWG */
  dwgHandle unknownHandle;   /*!< AC1027 trailing hard pointer in DWG */

private:
  std::shared_ptr<const DRW_DwgTableNameSource> m_dwgNameSource;
  bool hasInvalidDxfLineweight {false};
  int rawInvalidDxfLineweight {0};
  std::uint8_t m_colorDwgMethod {0};
  bool m_hasDwgColorCmcMetadata {false};
  std::uint8_t m_colorDwgFlags {0u};
  std::uint16_t m_colorDwgRawIndex {0u};
  UTF8STRING m_colorDwgName;
  UTF8STRING m_colorDwgBookName;
  std::size_t m_dxfRetainedColorNameBytes {0u};
  std::size_t m_dxfRetainedLineTypeBytes {0u};
  std::size_t m_dxfRetainedPlotStyleHandleBytes {0u};
  std::size_t m_dxfRetainedMaterialHandleBytes {0u};
};

//! Class to handle block record entries
/*!
 *  Class to handle block record table entries
 *  @author Rallaz
 */
class DRW_Block_Record : public DRW_TableEntry {
  SETOBJFRIENDS
  friend class dxfBlockRecord::Decoder;
public:
  DRW_Block_Record() { reset(); }
  void reset() {
    tType = DRW::BLOCK_RECORD;
    flags = 0;
    xrefUnloaded = false;
    insUnits = 0;
    basePoint = DRW_Coord(0.0, 0.0, 0.0);
    xrefIndex = -1;
    xrefPath.clear();
    description.clear();
    previewData.clear();
    canExplode = true;
    blockScaling = 0;
    layoutHandle = 0;
    xrefBlockHandle = dwgHandle{};
    insertCount = 0;
    insertHandles.clear();
    firstEH = lastEH = DRW::NoHandle;
    block = endBlock = 0;
    entMap.clear();
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  // Note:    int DRW_TableEntry::flags; contains code 70 of block
  bool xrefUnloaded {false}; /*!< DWG loaded bit / DXF BLOCK group 71; separate
                                from BLOCK group-70 resolved/dependency bits */
  int insUnits;        /*!< block insertion units, code 70 of block_record*/
  DRW_Coord basePoint; /*!<  block insertion base point dwg only */
  std::int32_t xrefIndex = -1; /*!< DWG XREF table index; -1 when absent */
  UTF8STRING xrefPath; /*!< Xref path name for XREF block_records (DWG: parsed
                          from BLOCK_HEADER, DXF: code 1) */
  UTF8STRING description; /*!< block description (DXF 4); DWG read */
  std::vector<std::uint8_t>
      previewData; /*!< bitmap preview bytes (DXF 310; DWG preview data) */
  bool canExplode =
      true; /*!< whether the block can be exploded (DXF 280); R2007+ DWG */
  std::uint8_t blockScaling =
      0; /*!< block scaling flag (DXF 281); R2007+ DWG */
  std::uint32_t layoutHandle =
      0; /*!< hard pointer to associated LAYOUT (DXF 340); DWG read */
  dwgHandle xrefBlockHandle; /*!< external-reference block handle (DWG common
                                table stream) */
  std::uint32_t insertCount = 0; /*!< number of DWG INSERT reference slots */
  std::vector<std::uint32_t>
      insertHandles; /*!< non-null DWG INSERT references */
  // Adapter retention accounting needs only the private entity-map size; keep
  // the mapping itself encapsulated because it is parser/writer bookkeeping.
  [[nodiscard]] std::size_t entityMapCount() const noexcept {
    return entMap.size();
  }
protected:
  // dwg parser
private:
  std::uint32_t block;    // handle for block entity
  std::uint32_t endBlock; // handle for end block entity
  std::uint32_t firstEH;  // handle of first entity, only in pre-2004
  std::uint32_t lastEH;   // handle of last entity, only in pre-2004
  std::vector<std::uint32_t> entMap;
};

//! Class to handle text style entries
/*!
 *  Class to handle text style symbol table entries
 *  @author Rallaz
 */
class DRW_Textstyle : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_Textstyle() { reset(); }

  void reset() {
    tType = DRW::STYLE;
    height = oblique = 0.0;
    width = lastHeight = 1.0;
    font = "txt";
    genFlag = 0; // 2= X mirror, 4= Y mirror
    fontFamily = 0;
    bigFont.clear();
    m_dxfRetainedTextBytes.fill(0u);
    xrefBlockHandle = dwgHandle{};
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool validatePayloadFields() const;
  [[nodiscard]] bool validateDxf() const;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *hdlBuf = nullptr) const;
  double height;      /*!< Fixed text height (0 not set), code 40 */
  double width;       /*!< Width factor, code 41 */
  double oblique;     /*!< Oblique angle in degrees, code 50 (DWG radians converted at the boundary) */
  int genFlag;        /*!< Text generation flags, code 71 */
  double lastHeight;  /*!< Last height used, code 42 */
  UTF8STRING font;    /*!< primary font file name, code 3 */
  UTF8STRING bigFont; /*!< bigfont file name or blank if none, code 4 */
  int fontFamily;     /*!< ttf font family, italic and bold flags, code 1071 */
  dwgHandle xrefBlockHandle; /*!< external-reference block handle in the common
                                stream */

private:
  // Parsed DXF font names retained on the shared table-entry text lease.
  // Caller mutations and DWG string storage are outside this accounting.
  std::array<std::size_t, 2> m_dxfRetainedTextBytes{};
};

//! Class to handle vport entries
/*!
 *  Class to handle vport symbol table entries
 *  @author Rallaz
 */
class DRW_Vport : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint32_t kDwgControlHandle = 0x08;
  static constexpr std::uint32_t kDwgActiveHandle = 0x16;

  DRW_Vport() { reset(); }

  void reset() {
    m_dxfAmbientColorNameBytes = 0u;
    tType = DRW::VPORT;
    UpperRight.x = UpperRight.y = 1.0;
    snapSpacing.x = snapSpacing.y = 10.0;
    gridSpacing = snapSpacing;
    center.x = 0.651828;
    center.y = -0.16;
    viewDir.z = 1;
    height = 5.13732;
    ratio = 2.4426877;
    lensHeight = 50;
    frontClip = backClip = snapAngle = twistAngle = 0.0;
    viewMode = snap = grid = snapStyle = snapIsopair = 0;
    fastZoom = 1;
    circleZoom = 100;
    renderMode = 0;
    useDefaultLighting = true;
    defaultLightingType = 1;
    brightness = 0.0;
    contrast = 0.0;
    ambientColor = 250;
    ambientColorRgb = -1;
    ambientColorName.clear();
    m_ambientColorCmc = {};
    ucsIcon = 3;
    gridBehavior = 7;
    gridMajorLines = 5;
    shadePlotMode = 0;
    hasDxfShadePlotMode = false;
    shadePlotHandle = 0;
    hasDxfShadePlotHandle = false;
    visualStyleHandle = 0;
    m_sunHandle = 0;
    backgroundHandle = 0;
    namedUcsHandle = 0;
    baseUcsHandle = 0;
    ucsOrigin = DRW_Coord(0.0, 0.0, 0.0);
    ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
    ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
    ucsElevation = 0.0;
    ucsOrthoType = 0;
    ucsPerVP = false;
    xrefBlockHandle = dwgHandle{};
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool validatePayloadFields() const;
  [[nodiscard]] bool validateDxf() const;
  [[nodiscard]] bool validateDxf(DRW::Version version) const;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *hdlBuf = nullptr) const;
  [[nodiscard]] const DRW_DwgCmcColorMetadata&
  ambientColorDwgCmc() const noexcept { return m_ambientColorCmc; }
  DRW_Coord lowerLeft;   /*!< Lower left corner, code 10 & 20 */
  DRW_Coord UpperRight;  /*!< Upper right corner, code 11 & 21 */
  DRW_Coord center;      /*!< center point in WCS, code 12 & 22 */
  DRW_Coord snapBase;    /*!< snap base point in DCS, code 13 & 23 */
  DRW_Coord snapSpacing; /*!< snap Spacing, code 14 & 24 */
  DRW_Coord gridSpacing; /*!< grid Spacing, code 15 & 25 */
  DRW_Coord viewDir; /*!< view direction from target point, code 16, 26 & 36 */
  DRW_Coord viewTarget;    /*!< view target point, code 17, 27 & 37 */
  double height;           /*!< view height, code 40 */
  double ratio;            /*!< viewport aspect ratio, code 41 */
  double lensHeight;       /*!< lens height, code 42 */
  double frontClip;        /*!< front clipping plane, code 43 */
  double backClip;         /*!< back clipping plane, code 44 */
  double snapAngle;        /*!< snap rotation angle, code 50 */
  double twistAngle;       /*!< view twist angle, code 51 */
  int viewMode;            /*!< view mode, code 71 */
  int circleZoom;          /*!< circle zoom percent, code 72 */
  int fastZoom;            /*!< fast zoom setting, code 73 */
  int ucsIcon;             /*!< UCSICON setting, code 74 */
  int snap;                /*!< snap on/off, code 75 */
  int grid;                /*!< grid on/off, code 76 */
  int snapStyle;           /*!< snap style, code 77 */
  int snapIsopair;         /*!< snap isopair, code 78 */
  int gridBehavior;        /*!< grid behavior, code 60, undocummented */
  unsigned int renderMode; /*!< render mode, code 281 (R2000+) */
  bool useDefaultLighting; /*!< default lighting flag, code 292 (R2007+) */
  std::uint8_t
      defaultLightingType; /*!< default lighting type, code 282 (R2007+) */
  double brightness;       /*!< viewport brightness, code 141 (R2007+) */
  double contrast;         /*!< viewport contrast, code 142 (R2007+) */
  std::uint32_t
      ambientColor;         /*!< ambient CMC indexed color, code 63 (R2007+) */
  int ambientColorRgb = -1; /*!< ambient CMC truecolor, code 421 (R2007+) */
  UTF8STRING
      ambientColorName; /*!< ambient color-book name, code 431 (R2007+) */
  int gridMajorLines;   /*!< major grid line interval, code 61 (R2007+) */
  /** code 60, bit coded possible value are
   * bit 1 (1) show out of limits
   * bit 2 (2) adaptive grid
   * bit 3 (4) allow subdivision
   * bit 4 (8) follow dynamic SCP
   **/
  /** VPORT shade-plot fields retained from DXF. Modes 0-3 with a null target
   * are writable; modern modes and non-null targets remain fail-closed until
   * their VPORT-specific target graph is qualified. */
  int shadePlotMode = 0;       /*!< DXF group 170 */
  bool hasDxfShadePlotMode = false;
  std::uint32_t shadePlotHandle = 0; /*!< DXF group 333 soft pointer */
  bool hasDxfShadePlotHandle = false;
  std::uint32_t visualStyleHandle =
      0;                         /*!< DXF group 348 / R2007+ DWG visual style */
  std::uint32_t m_sunHandle = 0; /*!< R2007+ SUN hard-owner ref (DWG-only) */
  std::uint32_t backgroundHandle = 0; /*!< R2007+ background ref, DXF code 332 */
  std::uint32_t namedUcsHandle = 0;   /*!< R2000+ named UCS ref (DWG-only) */
  std::uint32_t baseUcsHandle = 0;    /*!< R2000+ base UCS ref (DWG-only) */
  dwgHandle xrefBlockHandle; /*!< external-reference block ref (DWG-only) */
  DRW_Coord ucsOrigin;       /*!< R2000+ per-viewport UCS origin (DWG-only) */
  DRW_Coord ucsXAxis;        /*!< R2000+ per-viewport UCS X axis (DWG-only) */
  DRW_Coord ucsYAxis;        /*!< R2000+ per-viewport UCS Y axis (DWG-only) */
  double ucsElevation =
      0.0;               /*!< R2000+ per-viewport UCS elevation (DWG-only) */
  int ucsOrthoType = 0;  /*!< R2000+ UCS orthographic view type (DWG-only) */
  bool ucsPerVP = false; /*!< R2000+ UCS-per-viewport flag (DWG-only) */

private:
  DRW_DwgCmcColorMetadata m_ambientColorCmc;
  std::size_t m_dxfAmbientColorNameBytes {0u};
};

//! Class to handle fixed type 71 VPORT_ENTITY_HEADER records.
class DRW_ViewportEntityHeader : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgType = 71;
  static constexpr std::uint16_t kDwgControlType = 70;
  static constexpr std::uint32_t kDwgControlHandle = 0x0B;

  DRW_ViewportEntityHeader() { reset(); }

  void reset() {
    tType = DRW::VPORT_ENTITY_HEADER;
    isOn = false;
    xrefBlockHandle = 0;
    viewportEntityHandle = 0;
    previousViewportEntityHeaderHandle = 0;
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *hdlBuf = nullptr) const;
  bool isOn = false; /*!< viewport entity header enabled flag, DXF bit 1 */
  std::uint32_t xrefBlockHandle = 0; /*!< external-reference block handle */
  std::uint32_t viewportEntityHandle = 0; /*!< associated viewport entity */
  std::uint32_t previousViewportEntityHeaderHandle = 0;
};

//! Class to handle the modern AcDbVxControl object (DWG type 1327).
class DRW_VxControl : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 1327;

  DRW_VxControl() { reset(); }

  void reset() {
    tType = DRW::VXCONTROL;
    classVersion = 0;
    flags = 0;
    recordHandles.clear();
    m_rawData.clear();
    m_rawDataBitSize = 0;
    m_rawDataValid = false;
    m_rawDataVersion = DRW::UNKNOWNV;
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *hdlBuf = nullptr) const;
  //! Preserve an opaque AC1021+ body tail for same-version replay.
  [[nodiscard]] bool setDwgRawData(std::vector<std::uint8_t> bytes,
                                   std::uint32_t bitSize,
                                   DRW::Version version) {
    if (version <= DRW::AC1018 || version == DRW::UNKNOWNV ||
        bytes.size() > (std::numeric_limits<std::uint32_t>::max)() / 8u ||
        bitSize > bytes.size() * 8u)
      return false;
    m_rawData = std::move(bytes);
    m_rawDataBitSize = bitSize;
    m_rawDataValid = true;
    m_rawDataVersion = version;
    return true;
  }
  [[nodiscard]] const std::vector<std::uint8_t> &dwgRawData() const noexcept {
    return m_rawData;
  }
  [[nodiscard]] std::uint32_t dwgRawDataBitSize() const noexcept {
    return m_rawDataBitSize;
  }
  [[nodiscard]] bool hasDwgRawData() const noexcept { return m_rawDataValid; }
  [[nodiscard]] DRW::Version dwgRawDataVersion() const noexcept {
    return m_rawDataVersion;
  }
  std::uint32_t classVersion = 0;
  std::uint32_t flags = 0;
  std::vector<std::uint32_t> recordHandles;

private:
  std::vector<std::uint8_t> m_rawData;
  std::uint32_t m_rawDataBitSize = 0;
  bool m_rawDataValid = false;
  DRW::Version m_rawDataVersion = DRW::UNKNOWNV;
};

//! Class to handle the modern AcDbVxTableRecord object (DWG type 1328).
class DRW_VxTableRecord : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 1328;

  DRW_VxTableRecord() { reset(); }

  void reset() {
    tType = DRW::VXTABLERECORD;
    classVersion = 0;
    flags = 0;
    m_rawData.clear();
    m_rawDataBitSize = 0;
    m_rawDataValid = false;
    m_rawDataVersion = DRW::UNKNOWNV;
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *hdlBuf = nullptr) const;
  //! Preserve an opaque AC1021+ body tail for same-version replay.
  [[nodiscard]] bool setDwgRawData(std::vector<std::uint8_t> bytes,
                                   std::uint32_t bitSize,
                                   DRW::Version version) {
    if (version <= DRW::AC1018 || version == DRW::UNKNOWNV ||
        bytes.size() > (std::numeric_limits<std::uint32_t>::max)() / 8u ||
        bitSize > bytes.size() * 8u)
      return false;
    m_rawData = std::move(bytes);
    m_rawDataBitSize = bitSize;
    m_rawDataValid = true;
    m_rawDataVersion = version;
    return true;
  }
  [[nodiscard]] const std::vector<std::uint8_t> &dwgRawData() const noexcept {
    return m_rawData;
  }
  [[nodiscard]] std::uint32_t dwgRawDataBitSize() const noexcept {
    return m_rawDataBitSize;
  }
  [[nodiscard]] bool hasDwgRawData() const noexcept { return m_rawDataValid; }
  [[nodiscard]] DRW::Version dwgRawDataVersion() const noexcept {
    return m_rawDataVersion;
  }
  std::uint32_t classVersion = 0;
  std::uint32_t flags = 0;

private:
  std::vector<std::uint8_t> m_rawData;
  std::uint32_t m_rawDataBitSize = 0;
  bool m_rawDataValid = false;
  DRW::Version m_rawDataVersion = DRW::UNKNOWNV;
};

//! Class to handle AcDbTvDeviceProperties (DWG type 1326).
class DRW_TvDeviceProperties : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 1326;

  DRW_TvDeviceProperties() { reset(); }

  void reset() {
    tType = DRW::TVDEVICEPROPERTIES;
    flags = 0;
    maxRegenThreads = 0;
    useLutPalette = 0;
    alternateHighlight = 0;
    alternateHighlightColor = 0;
    geometryShaderUsage = 0;
    blendingMode = 0;
    antialiasingLevel = 0.0;
    valueBd2 = 0.0;
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;
  std::int32_t flags = 0;
  std::int32_t maxRegenThreads = 0;
  std::int32_t useLutPalette = 0;
  std::uint64_t alternateHighlight = 0;
  std::uint64_t alternateHighlightColor = 0;
  std::uint64_t geometryShaderUsage = 0;
  std::int32_t blendingMode = 0;
  double antialiasingLevel = 0.0;
  double valueBd2 = 0.0;
};

//! Class to handle CSAC document options (DWG type 1293).
class DRW_CsacDocumentOptions : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 1293;

  DRW_CsacDocumentOptions() { reset(); }

  void reset() {
    tType = DRW::CSACDOCUMENTOPTIONS;
    classVersion = 0;
    flags = 0;
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  std::uint32_t classVersion = 0;
  std::uint32_t flags = 0;
};

//! Class to handle CONTEXTDATAMANAGER objects (DWG type 1145).
/*!
 * The manager owns the per-annotation-context sub-manager names and their
 * object handles. LibreCAD does not currently consume the annotation data,
 * so the DWG reader exposes the bounded metadata and the normal raw-object
 * callback remains the lossless replay path.
 */
struct DRW_ContextDataManagerEntry {
  UTF8STRING text;
  std::uint32_t itemHandle = 0;
};

struct DRW_ContextDataSubManager {
  std::uint32_t handle = 0;
  std::uint32_t entryCount = 0;
  std::vector<DRW_ContextDataManagerEntry> entries;
};

class DRW_ContextDataManager : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 1145;

  DRW_ContextDataManager() { reset(); }

  void reset() {
    DRW_TableEntry::reset();
    tType = DRW::CONTEXTDATAMANAGER;
    objectContextHandle = 0;
    subManagerCount = 0;
    subManagers.clear();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  std::uint32_t objectContextHandle = 0;
  std::uint32_t subManagerCount = 0;
  std::vector<DRW_ContextDataSubManager> subManagers;
};

//! Class to handle imagedef entries
/*!
 *  Class to handle image definitions object entries
 *  @author Rallaz
 */
class DRW_ImageDef : public DRW_TableEntry { //
  SETOBJFRIENDS
public:
  static constexpr std::int32_t kMaxClassVersion = 10;

  DRW_ImageDef() { reset(); }

  void reset() {
    tType = DRW::IMAGEDEF;
    imgVersion = 0;
    name.clear();
    u = v = up = vp = 0.0;
    loaded = 0;
    resolution = 0;
    reactors.clear();
    m_dxfRetainedImageNameBytes = 0u;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  /// Inverse of parseDwg. Fixed DWG object type 102 (no CLASSES entry).
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  //    std::string handle;       /*!< entity identifier, code 5 */
  UTF8STRING name; /*!< File name of image, code 1 */
  int imgVersion;  /*!< class version, code 90, 0=R14 version */
  double u;        /*!< image size in pixels U value, code 10 */
  double v;        /*!< image size in pixels V value, code 20 */
  double up;       /*!< default size of one pixel U value, code 11 */
  double vp;      /*!< default pixel V size, documented DXF code 12; reader also accepts 21 */
  int loaded;     /*!< image is loaded flag, code 280, 0=unloaded, 1=loaded */
  int resolution; /*!< resolution units, code 281, 0=no, 2=centimeters, 5=inch
                   */

  std::map<std::string, std::string> reactors;

private:
  std::size_t m_dxfRetainedImageNameBytes{0u};
};

//! Class to handle plotsettings entries
/*!
 *  Class to handle plot settings object entries
 *  @author baranovskiykonstantin@gmail.com
 */
class DRW_PlotSettings : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_PlotSettings() { reset(); }

  void reset() {
    tType = DRW::PLOTSETTINGS;
    // Full plot field set (P4-01) — member names mirror DRW_Layout's plot
    // prefix so the read/encode code can be shared verbatim.
    pageSetupName.clear();
    printerConfig.clear();
    m_dxfRetainedTextBytes.fill(0u);
    plotLayoutFlags = 0;
    marginLeft = marginBottom = marginRight = marginTop = 0.0;
    paperWidth = paperHeight = 0.0;
    paperSize.clear();
    plotOriginX = plotOriginY = 0.0;
    paperUnits = 0;
    plotRotation = 0;
    plotType = 0;
    windowMinX = windowMinY = windowMaxX = windowMaxY = 0.0;
    plotViewName.clear();
    realWorldUnits = 1.0;
    drawingUnits = 1.0;
    currentStyleSheet.clear();
    scaleType = 0;
    scaleFactor = 1.0;
    paperImageOriginX = paperImageOriginY = 0.0;
    shadePlotMode = 0;
    shadePlotResLevel = 0;
    shadePlotCustomDPI = 0;
    plotViewHandle = dwgHandle{};
    shadePlotHandle = dwgHandle{};
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  // Full PlotSettings field set per ODA §20.4.84 (mirrors DRW_Layout's
  // embedded plot prefix; member names kept identical so P4-02's lifted
  // parse/encode is a verbatim copy of the Layout path).
  UTF8STRING pageSetupName; /*!< code 1, TV */
  UTF8STRING printerConfig; /*!< code 2, TV */
  int plotLayoutFlags;      /*!< code 70, BS */
  double marginLeft;        /*!< code 40, BD, millimeters */
  double marginBottom;      /*!< code 41, BD */
  double marginRight;       /*!< code 42, BD */
  double marginTop;         /*!< code 43, BD */
  double paperWidth;        /*!< code 44, BD */
  double paperHeight;       /*!< code 45, BD */
  UTF8STRING paperSize;     /*!< code 4, TV */
  double plotOriginX;       /*!< code 46, 2BD */
  double plotOriginY;       /*!< code 47, 2BD */
  int paperUnits;           /*!< code 72, BS */
  int plotRotation;         /*!< code 73, BS */
  int plotType;             /*!< code 74, BS */
  double windowMinX;        /*!< code 48, 2BD */
  double windowMinY;        /*!< code 49, 2BD */
  double windowMaxX;        /*!< code 140, 2BD */
  double windowMaxY;        /*!< code 141, 2BD */
  UTF8STRING plotViewName;  /*!< code 6, T (R13-R2000 only) */
  double realWorldUnits;    /*!< code 142, BD — numerator of print scale */
  double drawingUnits;      /*!< code 143, BD — denominator of print scale */
  UTF8STRING currentStyleSheet; /*!< code 7, TV */
  int scaleType;                /*!< code 75, BS — standard scale type */
  double scaleFactor;           /*!< code 147, BD */
  double paperImageOriginX;     /*!< code 148, 2BD */
  double paperImageOriginY;     /*!< code 149, 2BD */
  int shadePlotMode;            /*!< code 76, BS (R2004+) */
  int shadePlotResLevel;        /*!< code 77, BS (R2004+) */
  int shadePlotCustomDPI;       /*!< code 78, BS (R2004+) */
  dwgHandle plotViewHandle;     /*!< code 6, R2004+ handle stream */
  dwgHandle shadePlotHandle;    /*!< code 333, R2007+ handle stream */

private:
  // Retained logical bytes for the five DXF text fields above; caller
  // mutations and DWG strings are outside this per-read accounting scope.
  std::array<std::size_t, 5> m_dxfRetainedTextBytes{};
};

//! Class to handle UCS entries
/*!
 *  Class to handle UCS symbol-table entries (named user coordinate systems).
 */
struct DRW_UCSOrthoPoint {
  int type{0};
  DRW_Coord point;
};

class DRW_UCS : public DRW_TableEntry {
  SETOBJFRIENDS
  friend class dxfViewUcs::Decoder;
public:
  DRW_UCS() { reset(); }

  static constexpr std::uint32_t kMaxOrthographicOptions = 10000;

  void reset() {
    tType = DRW::UCS;
    origin.x = origin.y = origin.z = 0.0;
    xAxisDirection.x = xAxisDirection.y = xAxisDirection.z = 0.0;
    yAxisDirection.x = yAxisDirection.y = yAxisDirection.z = 0.0;
    orthoOrigin.x = orthoOrigin.y = orthoOrigin.z = 0.0;
    elevation = 0.0;
    orthoType = 0;
    orthographicOptions.clear();
    xrefBlockHandle = dwgHandle{};
    baseUcsHandle = dwgHandle{};
    namedUcsHandle = dwgHandle{};
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool validatePayloadFields() const;
  [[nodiscard]] bool validatePayloadFields(unsigned int flagMask) const;
  [[nodiscard]] bool validateDxf() const;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  DRW_Coord origin;         /*!< UCS origin, codes 10/20/30 */
  DRW_Coord xAxisDirection; /*!< UCS X-axis direction, codes 11/21/31 */
  DRW_Coord yAxisDirection; /*!< UCS Y-axis direction, codes 12/22/32 */
  DRW_Coord orthoOrigin;    /*!< Origin for orthographic UCS, codes 13/23/33 */
  double elevation;         /*!< Elevation, code 146 */
  int orthoType; /*!< Shared DWG orthographic metadata / DXF scalar code 79;
                      not the repeated DXF code-71 option selector */
  std::vector<DRW_UCSOrthoPoint>
      orthographicOptions;   /*!< R2000+ orthographic UCS options */
  dwgHandle xrefBlockHandle; /*!< external-reference block handle in the common
                                stream */
  // Phase 4 (P4-04): resolved handle-stream refs (R2000+).
  dwgHandle baseUcsHandle;  /*!< base UCS handle (code 346) */
  dwgHandle namedUcsHandle; /*!< named UCS handle (code 345) */
};

//! Class to handle VIEW entries
/*!
 *  Class to handle VIEW symbol-table entries (named views).
 */
class DRW_View : public DRW_TableEntry {
  SETOBJFRIENDS
  friend class dxfViewUcs::Decoder;
public:
  DRW_View() { reset(); }

  void reset() {
    m_dxfAmbientColorNameBytes = 0u;
    tType = DRW::VIEW;
    size.x = size.y = size.z = 0.0;
    center.x = center.y = center.z = 0.0;
    viewDirectionFromTarget.x = viewDirectionFromTarget.y =
        viewDirectionFromTarget.z = 0.0;
    targetPoint.x = targetPoint.y = targetPoint.z = 0.0;
    lensLen = 0.0;
    frontClippingPlaneOffset = 0.0;
    backClippingPlaneOffset = 0.0;
    twistAngle = 0.0;
    viewMode = 0;
    renderMode = 0;
    hasUCS = false;
    cameraPlottable = false;
    ucsOrigin.x = ucsOrigin.y = ucsOrigin.z = 0.0;
    ucsXAxis.x = ucsXAxis.y = ucsXAxis.z = 0.0;
    ucsYAxis.x = ucsYAxis.y = ucsYAxis.z = 0.0;
    ucsOrthoType = 1;
    ucsElevation = 0.0;
    namedUCS_ID = 0;
    baseUCS_ID = 0;
    m_useDefaultLights = true;
    m_defaultLightingType = 1;
    m_brightness = 0.0;
    m_contrast = 0.0;
    m_ambientColor = 250;
    m_ambientColorRgb = -1;
    m_ambientColorName.clear();
    m_ambientColorCmc = {};
    m_backgroundHandle = 0;
    m_visualStyleHandle = 0;
    m_sunHandle = 0;
    m_liveSectionHandle = 0;
    xrefBlockHandle = dwgHandle{};
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool validatePayloadFields() const;
  [[nodiscard]] bool validatePayloadFields(unsigned int flagMask) const;
  [[nodiscard]] bool validateDxf() const;
  [[nodiscard]] bool validateDxf(DRW::Version version) const;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *hdlBuf = nullptr) const;

public:
  [[nodiscard]] const DRW_DwgCmcColorMetadata&
  ambientColorDwgCmc() const noexcept { return m_ambientColorCmc; }
  DRW_Coord size;   /*!< View width/height in DCS, codes 40 & 41 */
  DRW_Coord center; /*!< View center point in DCS, codes 10 & 20 */
  DRW_Coord viewDirectionFromTarget; /*!< View direction from target in WCS,
                                        codes 11/21/31 */
  DRW_Coord targetPoint;             /*!< Target point in WCS, codes 12/22/32 */
  double lensLen;                    /*!< Lens length, code 42 */
  double frontClippingPlaneOffset; /*!< Front clipping plane offset from target,
                                      code 43 */
  double backClippingPlaneOffset;  /*!< Back clipping plane offset from target,
                                      code 44 */
  double twistAngle;               /*!< Twist angle, code 50 */
  int viewMode;                    /*!< View mode, code 71 */
  unsigned int renderMode;         /*!< Render mode, code 281 */
  bool hasUCS;                     /*!< 1 if a UCS is associated, code 72 */
  bool cameraPlottable;            /*!< 1 if camera is plottable, code 73 */
  DRW_Coord ucsOrigin;             /*!< UCS origin, codes 110/120/130 */
  DRW_Coord ucsXAxis;              /*!< UCS X axis, codes 111/121/131 */
  DRW_Coord ucsYAxis;              /*!< UCS Y axis, codes 112/122/132 */
  int ucsOrthoType;                /*!< Orthographic type, code 79 */
  double ucsElevation;             /*!< UCS elevation, code 146 */
  std::uint32_t namedUCS_ID; /*!< Handle of named UCS table record, code 345 */
  std::uint32_t baseUCS_ID;  /*!< Handle of base UCS table record, code 346 */
  bool m_useDefaultLights =
      true; /*!< R2007+ default-lighting flag (DWG-only) */
  std::uint8_t m_defaultLightingType =
      1;                     /*!< R2007+ default-lighting type (DWG-only) */
  double m_brightness = 0.0; /*!< R2007+ view brightness (DWG-only) */
  double m_contrast = 0.0;   /*!< R2007+ view contrast (DWG-only) */
  std::uint32_t m_ambientColor =
      250; /*!< R2007+ ambient CMC indexed color (DWG-only) */
  int m_ambientColorRgb = -1;    /*!< R2007+ ambient CMC truecolor */
  UTF8STRING m_ambientColorName; /*!< R2007+ ambient color-book name */
  std::uint32_t m_backgroundHandle =
      0; /*!< R2007+ background soft-pointer ref (DWG-only) */
  std::uint32_t m_visualStyleHandle =
      0; /*!< R2007+ visual-style hard-pointer ref (DWG-only) */
  std::uint32_t m_sunHandle = 0; /*!< R2007+ SUN hard-owner ref (DWG-only) */
  std::uint32_t m_liveSectionHandle =
      0; /*!< R2007+ live-section soft-pointer ref (DWG-only) */
  dwgHandle xrefBlockHandle; /*!< external-reference block handle in the common
                                stream */

private:
  DRW_DwgCmcColorMetadata m_ambientColorCmc;
  std::size_t m_dxfAmbientColorNameBytes {0u};
};

//! Class to handle AppId entries
/*!
 *  Class to handle AppId symbol table entries
 *  @author Rallaz
 */
class DRW_AppId : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_AppId() { reset(); }

  void reset() {
    tType = DRW::APPID;
    flags = 0;
    name = "";
    unknown71 = 0;
    m_dwgNameSource.reset();
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool validatePayloadFields() const;
  [[nodiscard]] bool validateDxf() const;

public:
  std::uint8_t unknown71{0}; /*!< Undocumented group-71 byte. */
  // Read-only evidence from the DWG name field, not a public-name hash or a
  // semantic certificate. Copies share immutable source storage; reset clears
  // it. Editing name/handle never refreshes it, and authored/DXF rows have none.
  [[nodiscard]] bool hasDwgNameSource() const noexcept {
    return m_dwgNameSource != nullptr;
  }
  [[nodiscard]] std::shared_ptr<const DRW_DwgTableNameSource> dwgNameSource() const noexcept {
    return m_dwgNameSource;
  }
  [[nodiscard]] bool hasUnchangedDwgSourceName() const noexcept {
    return m_dwgNameSource && m_dwgNameSource->matches(handle, name);
  }
  [[nodiscard]] bool hasBoundDwgNameProfile(DRW::Version version,
                                           const std::string& page) const noexcept {
    return m_dwgNameSource && m_dwgNameSource->bound(version, page);
  }
  [[nodiscard]] std::uint32_t dwgNameDecodeIssues() const noexcept {
    return m_dwgNameSource ? m_dwgNameSource->decodeIssues() : 0u;
  }
  [[nodiscard]] std::size_t dwgNameSourceRetainedBytes() const noexcept {
    return m_dwgNameSource ? m_dwgNameSource->retainedBytes() : 0u;
  }
  [[nodiscard]] bool sameDwgNameSource(const DRW_AppId& other) const noexcept {
    return m_dwgNameSource && other.m_dwgNameSource
        ? m_dwgNameSource->equal(*other.m_dwgNameSource)
        : m_dwgNameSource == other.m_dwgNameSource;
  }
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *hdlBuf = nullptr) const;
private:
  std::shared_ptr<const DRW_DwgTableNameSource> m_dwgNameSource;
};

//! Class to handle Dictionary (ODA spec sec 20.4.44 fixed type 42)
/*!
 *  Carrier for named-object dictionaries, including entry names and handles.
 */
class DRW_Dictionary : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr int kDwgFixedType = 42;
  static constexpr std::size_t kMaxEntries = 100000;

  DRW_Dictionary() { reset(); }
  DRW_Dictionary(const DRW_Dictionary& o): DRW_TableEntry(o),
      cloning(o.cloning), hardOwner(o.hardOwner), m_entries(o.m_entries),
      countCap(o.countCap), m_dwgEntriesComplete(o.m_dwgEntriesComplete),
      m_dxfRetainedEntryNameBytes(o.m_dxfRetainedEntryNameBytes) {
    tType = o.tType;
    m_pendingEntryName.clear();
  }
  DRW_Dictionary& operator=(const DRW_Dictionary& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      cloning = o.cloning;
      hardOwner = o.hardOwner;
      m_entries = o.m_entries;
      countCap = o.countCap;
      m_dwgEntriesComplete = o.m_dwgEntriesComplete;
      m_dxfRetainedEntryNameBytes = o.m_dxfRetainedEntryNameBytes;
      m_dxfRetainedPendingEntryNameBytes = 0u;
      m_pendingEntryName.clear();
      tType = o.tType;
    }
    return *this;
  }
  void reset() {
    tType = DRW::DICTIONARY;
    cloning = 0;
    hardOwner = 0;
    countCap.reset();
    // A constructed or DXF-populated dictionary owns its complete entry
    // vector. DWG decoding changes this to false until the complete,
    // counted name/handle stream has passed validation.
    m_dwgEntriesComplete = true;
    name.clear();
    m_entries.clear();
    m_pendingEntryName.clear();
    m_dxfRetainedEntryNameBytes = 0u;
    m_dxfRetainedPendingEntryNameBytes = 0u;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool parseDwgImpl(DRW::Version version, dwgBuffer *buf,
                                  std::uint32_t bs, bool withDefaultHandle);
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;
  [[nodiscard]] bool encodeDwgImpl(DRW::Version version, dwgBufferW *buf,
                                   dwgBufferW *strBuf, dwgBufferW *handleBuf,
                                   bool withDefaultHandle) const;
  [[nodiscard]] bool
  isDwgPayloadValidImpl(DRW::Version version,
                        bool withDefaultHandle) const noexcept;

public:
  struct Entry {
    enum class DxfHandleCode : std::uint16_t {
      Unspecified = 0,
      SoftOwner = 350,
      HardOwner = 360
    };
    UTF8STRING m_name;
    // Some valid DWGs encode a dictionary entry with a null child reference.
    std::uint32_t m_handle = 0;
    // Preserve the source edge code per key; zero keeps the legacy writer
    // policy for newly constructed or DWG-origin entries.
    DxfHandleCode m_dxfHandleCode = DxfHandleCode::Unspecified;
  };
  int cloning;                  /*!< duplicate-record handling (BS) */
  int hardOwner;                /*!< hard-owner flag (RC, R2007+) */
  std::vector<Entry> m_entries; /*!< dictionary entry names and item handles */
  std::optional<DRW_DictionaryCountCap> countCap;
  //! True only when a DWG parser validated every declared name/handle pair.
  [[nodiscard]] bool hasCompleteDwgEntries() const noexcept {
    return m_dwgEntriesComplete;
  }
  //! True when the DXF name/handle stream is structurally complete.
  [[nodiscard]] bool hasCompleteDxfEntries() const noexcept;
  //! Selected proof helper for a parsed, complete DXF dictionary body.
  //! Compares parser-private pending-name and retained-name state without
  //! exposing or changing the dictionary layout.
  [[nodiscard]] bool sameSelectedDxfEntryState(const DRW_Dictionary& other) const noexcept {
    return m_dwgEntriesComplete && other.m_dwgEntriesComplete
        && m_pendingEntryName.empty() && other.m_pendingEntryName.empty()
        && m_dxfRetainedPendingEntryNameBytes == 0u
        && other.m_dxfRetainedPendingEntryNameBytes == 0u
        && m_dxfRetainedEntryNameBytes == other.m_dxfRetainedEntryNameBytes;
  }
  [[nodiscard]] bool isDwgPayloadValid(DRW::Version version) const noexcept;
  [[nodiscard]] std::uint8_t dwgItemHandleCode() const noexcept;

private:
  // Keep a DXF entry name private until its following handle is decoded.
  UTF8STRING m_pendingEntryName;
  bool m_dwgEntriesComplete = false;
  std::size_t m_dxfRetainedEntryNameBytes{0u};
  std::size_t m_dxfRetainedPendingEntryNameBytes{0u};
};

//! Class to handle DICTIONARYWDFLT (AcDbDictionaryWithDefault).
class DRW_DictionaryWithDefault : public DRW_Dictionary {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 513;

  DRW_DictionaryWithDefault() { reset(); }
  DRW_DictionaryWithDefault(const DRW_DictionaryWithDefault& o):
      DRW_Dictionary(o), m_defaultEntryHandle(o.m_defaultEntryHandle),
      m_dxfDefaultEntrySeen(false),
      m_hasDxfDefaultEntry(o.m_hasDxfDefaultEntry) {
    tType = DRW::DICTIONARYWDFLT;
  }
  DRW_DictionaryWithDefault& operator=(const DRW_DictionaryWithDefault& o) {
    if (this != &o) {
      DRW_Dictionary::operator=(o);
      m_defaultEntryHandle = o.m_defaultEntryHandle;
      m_dxfDefaultEntrySeen = false;
      m_hasDxfDefaultEntry = o.m_hasDxfDefaultEntry;
      tType = DRW::DICTIONARYWDFLT;
    }
    return *this;
  }
  void reset() {
    DRW_Dictionary::reset();
    tType = DRW::DICTIONARYWDFLT;
    m_defaultEntryHandle = 0;
    m_dxfDefaultEntrySeen = false;
    m_hasDxfDefaultEntry = false;
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  std::uint32_t m_defaultEntryHandle = 0;
  //! True when the inherited DXF entries and required default pointer exist.
  [[nodiscard]] bool hasCompleteDxfPayload() const noexcept;
  [[nodiscard]] bool isDwgPayloadValid(DRW::Version version) const noexcept;

private:
  // Per-parse duplicate guard: never copy it into a new parser walk.
  bool m_dxfDefaultEntrySeen = false;
  // Persistent completeness evidence for the model, retained by value copies.
  bool m_hasDxfDefaultEntry = false;
};

//! Class to handle DICTIONARYVAR (AcDbDictionaryVar).
class DRW_DictionaryVar : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 512;

  DRW_DictionaryVar() { reset(); }
  void reset() {
    tType = DRW::DICTIONARYVAR;
    m_schema = 0;
    m_value.clear();
    m_dxfRetainedValueBytes = 0u;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  int m_schema = 0;   /*!< code 280 */
  UTF8STRING m_value; /*!< code 1 */

private:
  std::size_t m_dxfRetainedValueBytes{0u};
};

//! Class to handle XRECORD (fixed type 0x4f).
class DRW_XRecord : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_XRecord() { reset(); }
  DRW_XRecord(const DRW_XRecord& o): DRW_TableEntry(o),
      m_cloning(o.m_cloning), m_values(o.m_values),
      m_handleValues(o.m_handleValues), m_dataEntries(o.m_dataEntries),
      m_rawData(o.m_rawData), m_rawDataValid(o.m_rawDataValid),
      m_dxfBodySeen(false), m_dxfHeaderPending(false), m_dxfXdataSeen(false),
      m_dxfRetainedVariantStringBytes(
          o.m_dxfRetainedVariantStringBytes),
      m_dxfCoordinatesExact(o.hasExactDxfCoordinateTuples()),
      m_rawDataVersion(o.m_rawDataVersion),
      m_rawDataSourceRecord(o.m_rawDataSourceRecord) {
    tType = o.tType;
  }
  DRW_XRecord& operator=(const DRW_XRecord& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_cloning = o.m_cloning;
      m_values = o.m_values;
      m_handleValues = o.m_handleValues;
      m_dataEntries = o.m_dataEntries;
      m_dxfRetainedVariantStringBytes =
          o.m_dxfRetainedVariantStringBytes;
      m_rawData = o.m_rawData;
      m_rawDataValid = o.m_rawDataValid;
      m_dxfBodySeen = false;
      m_dxfHeaderPending = false;
      m_dxfXdataSeen = false;
      m_dxfCoordinateMask = 0;
      m_dxfCoordinatesExact = o.hasExactDxfCoordinateTuples();
      m_rawDataVersion = o.m_rawDataVersion;
      m_rawDataSourceRecord = o.m_rawDataSourceRecord;
      tType = o.tType;
    }
    return *this;
  }
  void reset() {
    tType = DRW::XRECORD;
    m_cloning = 0;
    m_values.clear();
    m_handleValues.clear();
    m_dataEntries.clear();
    m_dxfRetainedVariantStringBytes = 0u;
    m_rawData.clear();
    m_rawDataValid = false;
    m_dxfBodySeen = false;
    m_dxfHeaderPending = false;
    m_dxfXdataSeen = false;
    m_dxfCoordinateMask = 0;
    m_dxfCoordinatesExact = true;
    m_rawDataVersion = DRW::UNKNOWNV;
    m_rawDataSourceRecord.reset();
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  int m_cloning = 0; /*!< duplicate-record handling, code 280 */
  std::vector<DRW_Variant> m_values;
  std::vector<std::pair<int, std::uint32_t>>
      m_handleValues; /*!< DXF code + object id */
  // Ordered DWG data-block pairs. m_values and m_handleValues remain the
  // compatibility views used by existing callers; this carrier preserves
  // interleaving and the full 64-bit payload of handle-valued pairs.
  std::vector<DRW_Variant> m_dataEntries;
  // Exact declared DWG data-block bytes for a bounded but unsupported pair.
  // They are replayed relative to the current bit cursor, not byte-aligned.
  std::vector<std::uint8_t> m_rawData;
  bool m_rawDataValid = false;

  [[nodiscard]] DRW::Version dwgRawDataVersion() const noexcept {
    return m_rawDataVersion;
  }
  // An opaque block can contain undiscovered references and versioned text.
  // Replay requires the parser's exact source state; reset() before replacing
  // it with a complete typed payload. Clearing the validity flag alone must
  // not silently discard the unsupported part of a parsed record.
  [[nodiscard]] bool canReplayDwgRawData(DRW::Version version) const noexcept;
  [[nodiscard]] bool hasDwgRawDataSourceRecord() const noexcept {
    return m_rawDataSourceRecord != nullptr;
  }

  // Canonical complete XYZ tuples fit COORD exactly. Partial, repeated or
  // reordered physical components remain tolerant-readable, with their exact
  // spelling in the raw companion, but cannot be emitted through this typed
  // carrier without an explicit reconstruction. reset() before replacing an
  // inexact parsed payload; editing the vectors does not clear its provenance.
  [[nodiscard]] bool hasExactDxfCoordinateTuples() const noexcept {
    return m_dxfCoordinatesExact && m_dxfCoordinateMask == 0;
  }

private:
  bool m_dxfBodySeen = false;
  bool m_dxfHeaderPending = false;
  bool m_dxfXdataSeen = false;
  std::size_t m_dxfRetainedVariantStringBytes{0u};
  std::uint8_t m_dxfCoordinateMask = 0;
  bool m_dxfCoordinatesExact = true;
  DRW::Version m_rawDataVersion = DRW::UNKNOWNV;
  // Copies share this immutable snapshot, while all editable public payloads
  // remain deep copies. The snapshot itself has no snapshot pointer/cycle.
  std::shared_ptr<const DRW_XRecord> m_rawDataSourceRecord;
};

//! Value payload used by FIELD and TABLE/TABLECONTENT objects.
struct DRW_CadValue {
  int m_formatFlags = 0;
  int m_dataType = 0;
  std::uint32_t m_dataSize = 0;
  int m_unitType = 0;
  DRW_Variant m_value;
  UTF8STRING m_formatString;
  UTF8STRING m_valueString;
  std::uint32_t m_handle = 0;
  std::vector<std::uint8_t> m_rawData;
  // ODA §20.4.99's first flag bit suppresses the value payload, not the
  // preceding data-type BL. Other retained flags do not imply emptiness.
  [[nodiscard]] bool hasDwgPayload(DRW::Version version) const noexcept {
    return version <= DRW::AC1018 || (m_formatFlags & 0x01) == 0;
  }
};

//! Class to handle FIELD (AcDbField).
class DRW_Field : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 516;
  static constexpr std::size_t kMaxItems = 100000;

  enum class DwgReferenceKind {
    ChildField,
    Object,
    CadValueObjectId,
    FieldListMember
  };
  struct DwgReferencePolicy {
    DRW::DwgHandleReferenceCode wireHandleCode = DRW::DwgNullReference;
    bool hasHandleStreamSlot = false;
    bool allowsNull = false;
  };

  struct ChildValue {
    UTF8STRING m_key;
    DRW_CadValue m_value;
  };

  DRW_Field() { reset(); }
  void reset() {
    tType = DRW::FIELD;
    m_evaluatorId.clear();
    m_fieldCode.clear();
    m_formatString.clear();
    m_evaluationOptionFlags = 0;
    m_filingOptionFlags = 0;
    m_fieldStateFlags = 0;
    m_evaluationStatusFlags = 0;
    m_evaluationErrorCode = 0;
    m_evaluationErrorMessage.clear();
    m_value = DRW_CadValue();
    m_valueString.clear();
    m_valueStringLength = 0;
    m_childHandles.clear();
    m_objectHandles.clear();
    m_childValues.clear();
    m_dwgPayloadComplete = true;
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  //! False only when a DWG FIELD body was retained as raw data after an
  //! unsupported but bounded decode. Constructed and DXF FIELDs are complete.
  [[nodiscard]] bool hasCompleteDwgPayload() const noexcept {
    return m_dwgPayloadComplete;
  }
  //! True when a type-64 CadValue occupies a FIELD handle-stream slot.
  [[nodiscard]] static bool
  cadValueHasDwgObjectHandle(DRW::Version version,
                             const DRW_CadValue &value) noexcept;
  //! DWG FIELD/FIELDLIST handle-stream policy shared by parsers and writers.
  [[nodiscard]] static DwgReferencePolicy
  dwgReferencePolicy(DRW::Version version, DwgReferenceKind kind,
                     const DRW_CadValue *value = nullptr) noexcept;
  [[nodiscard]] bool isDwgPayloadValid(DRW::Version version) const noexcept;

  UTF8STRING m_evaluatorId;
  UTF8STRING m_fieldCode;
  UTF8STRING m_formatString;
  int m_evaluationOptionFlags = 0;
  int m_filingOptionFlags = 0;
  int m_fieldStateFlags = 0;
  int m_evaluationStatusFlags = 0;
  int m_evaluationErrorCode = 0;
  UTF8STRING m_evaluationErrorMessage;
  DRW_CadValue m_value;
  UTF8STRING m_valueString;
  int m_valueStringLength = 0;
  std::vector<std::uint32_t> m_childHandles;
  std::vector<std::uint32_t> m_objectHandles;
  std::vector<ChildValue> m_childValues;

private:
  bool m_dwgPayloadComplete = true;
};

//! Immutable parser receipt for one complete AcDbField custom object.
//!
//! The payload is retained in its typed representation so a writer can prove
//! that a DWG source object has not been changed before replacing its raw
//! frame. Constructed and DXF-backed FIELD objects intentionally have no such
//! receipt.
struct DRW_DwgFieldPayloadReceipt {
  DRW::Version m_version = DRW::UNKNOWNV;
  std::uint32_t m_fieldHandle = DRW::NoHandle;
  std::uint32_t m_sourceOffset = 0;
  std::uint64_t m_sourceMapOrdinal = 0;
  DRW_DwgFrameOffsetSpace m_sourceOffsetSpace =
      DRW_DwgFrameOffsetSpace::Unknown;
  bool m_hasSourceLocation = false;
  bool m_complete = false;
  int m_encodedType = 0;
  int m_resolvedType = 0;
  UTF8STRING m_recordName;
  UTF8STRING m_className;
  std::optional<std::uint64_t> m_classStreamOrdinal;
  DRW_Field m_field;
};

//! Class to handle FIELDLIST (AcDbFieldList).
class DRW_FieldList : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 515;

  DRW_FieldList() { reset(); }
  void reset() {
    tType = DRW::FIELDLIST;
    m_unknown = 0;
    m_fieldHandles.clear();
    m_dwgEntriesComplete = false;
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  //! True only after DWG parsing retained every declared handle slot.
  bool hasCompleteDwgEntries() const noexcept { return m_dwgEntriesComplete; }
  [[nodiscard]] bool isDwgPayloadValid(DRW::Version version) const noexcept;

  int m_unknown = 0;
  std::vector<std::uint32_t> m_fieldHandles;

private:
  bool m_dwgEntriesComplete = false;
};

//! Class to handle DATATABLE (AcDbDataTable) — a hidden key/value table that
//! carries no geometry.  ODA / libreDWG dwg2.spec DATATABLE.  The body layout
//! (a columns×rows array of bit-packed cell values) is only partially
//! documented — libreDWG marks it "(varies) TODO" / DEBUG_CLASS. The reliable
//! prefix is decoded first, and the body walk is bounded by the object and
//! detached-string streams. An incomplete primitive fails closed; the raw
//! shelf still preserves the exact byte image for same-version replay.
class DRW_DataTable : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  // 520 is MESH (AcDbSubDMesh) in the writer's fixed entity class map.
  // DATATABLE is an object class — keep it off the MESH ordinal.
  static constexpr std::uint16_t kDwgClassNum = 531;

  //! One cell of a DATATABLE column.  A cell always carries all three
  //! variants on the wire (long/double/string); the column's type selects
  //! which one is meaningful.
  struct Row {
    std::int32_t dataLong = 0; /*!< DXF 93 */
    double dataDouble = 0.0;   /*!< DXF 40 */
    UTF8STRING dataString;     /*!< DXF 3 */
  };
  //! One DATATABLE column: a cell type, a column name and the rows.
  struct Column {
    std::int32_t type = 0; /*!< DXF 92 */
    UTF8STRING text;       /*!< DXF 2 (column name) */
    std::vector<Row> rows;
  };

  DRW_DataTable() { reset(); }
  void reset() {
    tType = DRW::DATATABLE;
    flags = 0;
    columnCount = 0;
    rowCount = 0;
    tableName.clear();
    columns.clear();
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  int flags = 0;        /*!< DXF 70 */
  int columnCount = 0;  /*!< DXF 90 (num_cols) */
  int rowCount = 0;     /*!< DXF 91 (num_rows) */
  UTF8STRING tableName; /*!< DXF 1 */
  std::vector<Column> columns;
};

//! Class to handle RASTERVARIABLES (AcDbRasterVariables).
class DRW_RasterVariables : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 505;

  DRW_RasterVariables() { reset(); }
  void reset() {
    tType = DRW::RASTERVARIABLES;
    m_classVersion = 0;
    m_imageFrame = 0;
    m_imageQuality = 0;
    m_units = 0;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  int m_classVersion = 0;
  int m_imageFrame = 0;
  int m_imageQuality = 0;
  int m_units = 0;
};

//! Class to handle WIPEOUTVARIABLES (AcDbWipeoutVariables) — the drawing-wide
//! wipeout/image display-frame flag. ODA / libreDWG dwg2.spec WIPEOUTVARIABLES.
class DRW_WipeoutVariables : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 529;

  DRW_WipeoutVariables() { reset(); }
  void reset() {
    tType = DRW::UNKNOWNT;
    m_displayFrame = 0;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  std::uint16_t m_displayFrame = 0; /*!< global display-frame flag, DXF 70 */
};

//! Class to handle SORTENTSTABLE (AcDbSortentsTable).
class DRW_SortEntsTable : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 514;
  static constexpr std::size_t kMaxEntries = 100000;

  DRW_SortEntsTable() { reset(); }
  DRW_SortEntsTable(const DRW_SortEntsTable& o): DRW_TableEntry(o),
      m_sortHandles(o.m_sortHandles),
      m_blockOwnerHandle(o.m_blockOwnerHandle),
      m_entityHandles(o.m_entityHandles), m_dxfInBody(false),
      m_dxfHasBlockOwner(false), m_dxfPendingEntity(std::nullopt),
      m_dwgEntriesComplete(o.m_dwgEntriesComplete) {
    tType = o.tType;
  }
  DRW_SortEntsTable& operator=(const DRW_SortEntsTable& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_sortHandles = o.m_sortHandles;
      m_blockOwnerHandle = o.m_blockOwnerHandle;
      m_entityHandles = o.m_entityHandles;
      m_dxfInBody = false;
      m_dxfHasBlockOwner = false;
      m_dxfPendingEntity.reset();
      m_dwgEntriesComplete = o.m_dwgEntriesComplete;
      tType = o.tType;
    }
    return *this;
  }
  void reset() {
    tType = DRW::SORTENTSTABLE;
    m_sortHandles.clear();
    m_blockOwnerHandle = DRW::NoHandle;
    m_entityHandles.clear();
    m_dwgEntriesComplete = false;
    m_dxfInBody = false;
    m_dxfHasBlockOwner = false;
    m_dxfPendingEntity.reset();
    DRW_TableEntry::reset();
  }
  bool hasCompleteDwgEntries() const noexcept {
    return m_dwgEntriesComplete && m_blockOwnerHandle != DRW::NoHandle &&
           m_sortHandles.size() == m_entityHandles.size();
  }
  bool finalizeDxf();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  std::vector<std::uint32_t> m_sortHandles;
  std::uint32_t m_blockOwnerHandle = 0;
  std::vector<std::uint32_t> m_entityHandles;

private:
  /* DXF: true once past the "100 AcDbSortentsTable" marker, so body codes
     330/331/5 (block owner / entity handles / sort handles) are not confused
     with the object's own header handle (5) and owner (330). */
  bool m_dxfInBody = false;
  bool m_dxfHasBlockOwner = false;
  std::optional<std::uint32_t> m_dxfPendingEntity;
  bool m_dwgEntriesComplete = false;
};

//! Class to handle MATERIAL (AcDbMaterial) fields.
class DRW_Material : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  // MATERIAL is a file-local custom class. The writer remaps this ordinal
  // when another class already occupies it in the same output file.
  static constexpr std::uint16_t kDwgClassNum = 504;

  DRW_Material() { reset(); }
  void reset() {
    tType = DRW::MATERIAL;
    m_name.clear();
    m_description.clear();
    m_ambientColorMethod = 0u;
    m_ambientColorFactor = 1.0;
    m_ambientColorValue = 0u;
    m_hasAmbientColorMethod = false;
    m_hasAmbientColorFactor = false;
    m_hasAmbientColorValue = false;
    m_diffuseColorMethod = 0u;
    m_diffuseColorFactor = 1.0;
    m_diffuseColorValue = 0u;
    m_hasDiffuseColorMethod = false;
    m_hasDiffuseColorFactor = false;
    m_hasDiffuseColorValue = false;
    m_diffuseMapBlendFactor = 1.0;
    m_diffuseMapSource = 1u;
    m_diffuseMapFileName.clear();
    m_diffuseMapProjection = 1u;
    m_diffuseMapTiling = 1u;
    m_diffuseMapAutoTransform = 1u;
    m_hasDiffuseMapBlendFactor = false;
    m_hasDiffuseMapSource = false;
    m_hasDiffuseMapFileName = false;
    m_hasDiffuseMapProjection = false;
    m_hasDiffuseMapTiling = false;
    m_hasDiffuseMapAutoTransform = false;
    m_diffuseMapTransform = {{1.0, 0.0, 0.0, 0.0,
                              0.0, 1.0, 0.0, 0.0,
                              0.0, 0.0, 1.0, 0.0,
                              0.0, 0.0, 0.0, 1.0}};
    m_hasDiffuseMapTransform = false;
    m_dxfDiffuseMapTransformCount = 0u;
    m_specularGlossFactor = 0.5;
    m_specularColorMethod = 0u;
    m_specularColorFactor = 1.0;
    m_specularColorValue = 0u;
    m_hasSpecularGlossFactor = false;
    m_hasSpecularColorMethod = false;
    m_hasSpecularColorFactor = false;
    m_hasSpecularColorValue = false;
    m_specularMapBlendFactor = 1.0;
    m_specularMapSource = 1u;
    m_specularMapFileName.clear();
    m_specularMapProjection = 1u;
    m_specularMapTiling = 1u;
    m_specularMapAutoTransform = 1u;
    m_hasSpecularMapBlendFactor = false;
    m_hasSpecularMapSource = false;
    m_hasSpecularMapFileName = false;
    m_hasSpecularMapProjection = false;
    m_hasSpecularMapTiling = false;
    m_hasSpecularMapAutoTransform = false;
    m_specularMapTransform = {{1.0, 0.0, 0.0, 0.0,
                               0.0, 1.0, 0.0, 0.0,
                               0.0, 0.0, 1.0, 0.0,
                               0.0, 0.0, 0.0, 1.0}};
    m_hasSpecularMapTransform = false;
    m_dxfSpecularMapTransformCount = 0u;
    m_reflectionMapBlendFactor = 1.0;
    m_reflectionMapSource = 1u;
    m_reflectionMapFileName.clear();
    m_reflectionMapProjection = 1u;
    m_reflectionMapTiling = 1u;
    m_reflectionMapAutoTransform = 1u;
    m_hasReflectionMapBlendFactor = false;
    m_hasReflectionMapSource = false;
    m_hasReflectionMapFileName = false;
    m_hasReflectionMapProjection = false;
    m_hasReflectionMapTiling = false;
    m_hasReflectionMapAutoTransform = false;
    m_reflectionMapTransform = {{1.0, 0.0, 0.0, 0.0,
                                 0.0, 1.0, 0.0, 0.0,
                                 0.0, 0.0, 1.0, 0.0,
                                 0.0, 0.0, 0.0, 1.0}};
    m_hasReflectionMapTransform = false;
    m_dxfReflectionMapTransformCount = 0u;
    m_opacityPercent = 1.0;
    m_opacityMapBlendFactor = 1.0;
    m_opacityMapSource = 1u;
    m_opacityMapFileName.clear();
    m_opacityMapProjection = 1u;
    m_opacityMapTiling = 1u;
    m_opacityMapAutoTransform = 1u;
    m_hasOpacityPercent = false;
    m_hasOpacityMapBlendFactor = false;
    m_hasOpacityMapSource = false;
    m_hasOpacityMapFileName = false;
    m_hasOpacityMapProjection = false;
    m_hasOpacityMapTiling = false;
    m_hasOpacityMapAutoTransform = false;
    m_opacityMapTransform = {{1.0, 0.0, 0.0, 0.0,
                              0.0, 1.0, 0.0, 0.0,
                              0.0, 0.0, 1.0, 0.0,
                              0.0, 0.0, 0.0, 1.0}};
    m_hasOpacityMapTransform = false;
    m_dxfOpacityMapTransformCount = 0u;
    m_bumpMapBlendFactor = 1.0;
    m_bumpMapSource = 1u;
    m_bumpMapFileName.clear();
    m_bumpMapProjection = 1u;
    m_bumpMapTiling = 1u;
    m_bumpMapAutoTransform = 1u;
    m_hasBumpMapBlendFactor = false;
    m_hasBumpMapSource = false;
    m_hasBumpMapFileName = false;
    m_hasBumpMapProjection = false;
    m_hasBumpMapTiling = false;
    m_hasBumpMapAutoTransform = false;
    m_bumpMapTransform = {{1.0, 0.0, 0.0, 0.0,
                           0.0, 1.0, 0.0, 0.0,
                           0.0, 0.0, 1.0, 0.0,
                           0.0, 0.0, 0.0, 1.0}};
    m_hasBumpMapTransform = false;
    m_dxfBumpMapTransformCount = 0u;
    m_refractionIndex = 1.0;
    m_refractionMapBlendFactor = 1.0;
    m_refractionMapSource = 1u;
    m_refractionMapFileName.clear();
    m_refractionMapProjection = 1u;
    m_refractionMapTiling = 1u;
    m_refractionMapAutoTransform = 1u;
    m_hasRefractionIndex = false;
    m_hasRefractionMapBlendFactor = false;
    m_hasRefractionMapSource = false;
    m_hasRefractionMapFileName = false;
    m_hasRefractionMapProjection = false;
    m_hasRefractionMapTiling = false;
    m_hasRefractionMapAutoTransform = false;
    m_refractionMapTransform = {{1.0, 0.0, 0.0, 0.0,
                                 0.0, 1.0, 0.0, 0.0,
                                 0.0, 0.0, 1.0, 0.0,
                                 0.0, 0.0, 0.0, 1.0}};
    m_hasRefractionMapTransform = false;
    m_dxfRefractionMapTransformCount = 0u;
    m_colorBleedScale = 0.0;
    m_indirectDumpScale = 0.0;
    m_reflectanceScale = 0.0;
    m_transmittanceScale = 0.0;
    m_hasColorBleedScale = false;
    m_hasIndirectDumpScale = false;
    m_hasReflectanceScale = false;
    m_hasTransmittanceScale = false;
    m_twoSidedMaterial = false;
    m_hasTwoSidedMaterial = false;
    m_materialsAnonymous = false;
    m_hasMaterialsAnonymous = false;
    m_luminance = 0.0;
    m_luminanceMode = 0;
    m_normalMapMethod = 0;
    m_normalMapStrength = 0.0;
    m_hasLuminance = false;
    m_hasLuminanceMode = false;
    m_hasNormalMapMethod = false;
    m_hasNormalMapStrength = false;
    m_normalMapBlendFactor = 1.0;
    m_normalMapSource = 1u;
    m_normalMapFileName.clear();
    m_normalMapProjection = 1u;
    m_normalMapTiling = 1u;
    m_normalMapAutoTransform = 1u;
    m_hasNormalMapBlendFactor = false;
    m_hasNormalMapSource = false;
    m_hasNormalMapFileName = false;
    m_hasNormalMapProjection = false;
    m_hasNormalMapTiling = false;
    m_hasNormalMapAutoTransform = false;
    m_normalMapTransform = {{1.0, 0.0, 0.0, 0.0,
                             0.0, 1.0, 0.0, 0.0,
                             0.0, 0.0, 1.0, 0.0,
                             0.0, 0.0, 0.0, 1.0}};
    m_hasNormalMapTransform = false;
    m_dxfNormalMapTransformCount = 0u;
    m_dxfMaterialGenProcTableStarted = false;
    m_dxfMaterialGenProcTableEnded = false;
    m_globalIlluminationMode = 0;
    m_finalGatherMode = 0;
    m_hasGlobalIlluminationMode = false;
    m_hasFinalGatherMode = false;
    m_genProcTableEnd = false;
    m_hasGenProcTableEnd = false;
    m_mapUTile = 0;
    m_hasMapUTile = false;
    m_translucence = 0.0;
    m_hasTranslucence = false;
    m_reflectivity = 0.0;
    m_hasReflectivity = false;
    m_illuminationModel = 0;
    m_hasIlluminationModel = false;
    m_channelFlags = 0;
    m_hasChannelFlags = false;
    m_dxfRetainedNameBytes = 0u;
    m_dxfRetainedDescriptionBytes = 0u;
    m_dxfRetainedDiffuseMapFileNameBytes = 0u;
    m_dxfRetainedSpecularMapFileNameBytes = 0u;
    m_dxfRetainedReflectionMapFileNameBytes = 0u;
    m_dxfRetainedOpacityMapFileNameBytes = 0u;
    m_dxfRetainedBumpMapFileNameBytes = 0u;
    m_dxfRetainedRefractionMapFileNameBytes = 0u;
    m_dxfRetainedNormalMapFileNameBytes = 0u;
    m_dxfPastPreNormalMapFields = false;
    m_dxfCompleteDiffuseBeforeRefraction = false;
    m_dxfInferredNormalMapBoundary = false;
    m_dxfRepeatedDiffuseMapField = false;
    m_dxfPreStrengthNormalMapMethod = false;
    m_rawData.clear();
    m_rawDataBitSize = 0;
    m_rawDataValid = false;
    m_rawDataVersion = DRW::UNKNOWNV;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  [[nodiscard]] bool validateDwgPayload(DRW::Version version) const;
  UTF8STRING m_name;
  UTF8STRING m_description;
  //! Ambient color controls: DXF groups 70 (method) and 40 (factor).
  std::uint8_t m_ambientColorMethod {0u};
  double m_ambientColorFactor {1.0};
  std::uint32_t m_ambientColorValue {0u};
  bool m_hasAmbientColorMethod {false};
  bool m_hasAmbientColorFactor {false};
  bool m_hasAmbientColorValue {false};
  //! Diffuse color controls: DXF groups 71 (method) and 41 (factor).
  std::uint8_t m_diffuseColorMethod {0u};
  double m_diffuseColorFactor {1.0};
  std::uint32_t m_diffuseColorValue {0u};
  bool m_hasDiffuseColorMethod {false};
  bool m_hasDiffuseColorFactor {false};
  bool m_hasDiffuseColorValue {false};
  //! Diffuse-map controls: DXF groups 42, 72, 3, 73, 74, and 75.
  double m_diffuseMapBlendFactor {1.0};
  std::uint8_t m_diffuseMapSource {1u};
  UTF8STRING m_diffuseMapFileName;
  std::uint8_t m_diffuseMapProjection {1u};
  std::uint8_t m_diffuseMapTiling {1u};
  std::uint8_t m_diffuseMapAutoTransform {1u};
  bool m_hasDiffuseMapBlendFactor {false};
  bool m_hasDiffuseMapSource {false};
  bool m_hasDiffuseMapFileName {false};
  bool m_hasDiffuseMapProjection {false};
  bool m_hasDiffuseMapTiling {false};
  bool m_hasDiffuseMapAutoTransform {false};
  //! Diffuse-map mapper transform: DXF group 43, 16 row-major reals.
  std::array<double, 16> m_diffuseMapTransform {{
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0}};
  bool m_hasDiffuseMapTransform {false};
  //! Specular controls: DXF groups 44/76/45/92.
  double m_specularGlossFactor {0.5};
  std::uint8_t m_specularColorMethod {0u};
  double m_specularColorFactor {1.0};
  std::uint32_t m_specularColorValue {0u};
  bool m_hasSpecularGlossFactor {false};
  bool m_hasSpecularColorMethod {false};
  bool m_hasSpecularColorFactor {false};
  bool m_hasSpecularColorValue {false};
  //! Specular-map fields: groups 46/77/4/78/79/170 plus transform group 47.
  double m_specularMapBlendFactor {1.0};
  std::uint8_t m_specularMapSource {1u};
  UTF8STRING m_specularMapFileName;
  std::uint8_t m_specularMapProjection {1u};
  std::uint8_t m_specularMapTiling {1u};
  std::uint8_t m_specularMapAutoTransform {1u};
  bool m_hasSpecularMapBlendFactor {false};
  bool m_hasSpecularMapSource {false};
  bool m_hasSpecularMapFileName {false};
  bool m_hasSpecularMapProjection {false};
  bool m_hasSpecularMapTiling {false};
  bool m_hasSpecularMapAutoTransform {false};
  //! Specular-map mapper transform: DXF group 47, 16 row-major reals.
  std::array<double, 16> m_specularMapTransform {{
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0}};
  bool m_hasSpecularMapTransform {false};
  //! Reflection-map fields: groups 48/171/6/172/173/174 plus transform 49.
  double m_reflectionMapBlendFactor {1.0};
  std::uint8_t m_reflectionMapSource {1u};
  UTF8STRING m_reflectionMapFileName;
  std::uint8_t m_reflectionMapProjection {1u};
  std::uint8_t m_reflectionMapTiling {1u};
  std::uint8_t m_reflectionMapAutoTransform {1u};
  bool m_hasReflectionMapBlendFactor {false};
  bool m_hasReflectionMapSource {false};
  bool m_hasReflectionMapFileName {false};
  bool m_hasReflectionMapProjection {false};
  bool m_hasReflectionMapTiling {false};
  bool m_hasReflectionMapAutoTransform {false};
  //! Reflection-map mapper transform: DXF group 49, 16 row-major reals.
  std::array<double, 16> m_reflectionMapTransform {{
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0}};
  bool m_hasReflectionMapTransform {false};
  //! Opacity and opacity-map fields: groups 140/141/175/7/176/177/178/142.
  double m_opacityPercent {1.0};
  double m_opacityMapBlendFactor {1.0};
  std::uint8_t m_opacityMapSource {1u};
  UTF8STRING m_opacityMapFileName;
  std::uint8_t m_opacityMapProjection {1u};
  std::uint8_t m_opacityMapTiling {1u};
  std::uint8_t m_opacityMapAutoTransform {1u};
  bool m_hasOpacityPercent {false};
  bool m_hasOpacityMapBlendFactor {false};
  bool m_hasOpacityMapSource {false};
  bool m_hasOpacityMapFileName {false};
  bool m_hasOpacityMapProjection {false};
  bool m_hasOpacityMapTiling {false};
  bool m_hasOpacityMapAutoTransform {false};
  //! Opacity-map mapper transform: DXF group 142, 16 row-major reals.
  std::array<double, 16> m_opacityMapTransform {{
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0}};
  bool m_hasOpacityMapTransform {false};
  //! Bump-map controls and mapper transform: groups 143/179/8/270/271/272/144.
  double m_bumpMapBlendFactor {1.0};
  std::uint8_t m_bumpMapSource {1u};
  UTF8STRING m_bumpMapFileName;
  std::uint8_t m_bumpMapProjection {1u};
  std::uint8_t m_bumpMapTiling {1u};
  std::uint8_t m_bumpMapAutoTransform {1u};
  bool m_hasBumpMapBlendFactor {false};
  bool m_hasBumpMapSource {false};
  bool m_hasBumpMapFileName {false};
  bool m_hasBumpMapProjection {false};
  bool m_hasBumpMapTiling {false};
  bool m_hasBumpMapAutoTransform {false};
  //! Bump-map mapper transform: DXF group 144, 16 row-major reals.
  std::array<double, 16> m_bumpMapTransform {{
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0}};
  bool m_hasBumpMapTransform {false};
  //! Refraction fields and map: groups 145/146/273/9/274/275/276/147.
  double m_refractionIndex {1.0};
  double m_refractionMapBlendFactor {1.0};
  std::uint8_t m_refractionMapSource {1u};
  UTF8STRING m_refractionMapFileName;
  std::uint8_t m_refractionMapProjection {1u};
  std::uint8_t m_refractionMapTiling {1u};
  std::uint8_t m_refractionMapAutoTransform {1u};
  bool m_hasRefractionIndex {false};
  bool m_hasRefractionMapBlendFactor {false};
  bool m_hasRefractionMapSource {false};
  bool m_hasRefractionMapFileName {false};
  bool m_hasRefractionMapProjection {false};
  bool m_hasRefractionMapTiling {false};
  bool m_hasRefractionMapAutoTransform {false};
  //! Refraction-map mapper transform: DXF group 147, 16 row-major reals.
  std::array<double, 16> m_refractionMapTransform {{
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0}};
  bool m_hasRefractionMapTransform {false};
  //! Pre-luminance physical controls: groups 460-463 and 290.
  double m_colorBleedScale {0.0};
  double m_indirectDumpScale {0.0};
  double m_reflectanceScale {0.0};
  double m_transmittanceScale {0.0};
  bool m_hasColorBleedScale {false};
  bool m_hasIndirectDumpScale {false};
  bool m_hasReflectanceScale {false};
  bool m_hasTransmittanceScale {false};
  bool m_twoSidedMaterial {false};
  bool m_hasTwoSidedMaterial {false};
  //! Materials Anonymous: DXF group 293.
  bool m_materialsAnonymous {false};
  bool m_hasMaterialsAnonymous {false};
  //! Luminance and normal-map controls: groups 464/270/271/465.
  double m_luminance {0.0};
  std::int16_t m_luminanceMode {0};
  std::int16_t m_normalMapMethod {0};
  double m_normalMapStrength {0.0};
  bool m_hasLuminance {false};
  bool m_hasLuminanceMode {false};
  bool m_hasNormalMapMethod {false};
  bool m_hasNormalMapStrength {false};
  //! Normal-map controls and mapper transform: groups 42/72/3/73/74/75/43.
  double m_normalMapBlendFactor {1.0};
  std::uint8_t m_normalMapSource {1u};
  UTF8STRING m_normalMapFileName;
  std::uint8_t m_normalMapProjection {1u};
  std::uint8_t m_normalMapTiling {1u};
  std::uint8_t m_normalMapAutoTransform {1u};
  bool m_hasNormalMapBlendFactor {false};
  bool m_hasNormalMapSource {false};
  bool m_hasNormalMapFileName {false};
  bool m_hasNormalMapProjection {false};
  bool m_hasNormalMapTiling {false};
  bool m_hasNormalMapAutoTransform {false};
  std::array<double, 16> m_normalMapTransform {{
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0}};
  bool m_hasNormalMapTransform {false};
  //! MATERIAL tail fields with distinct parser contexts and DXF codes.
  std::int16_t m_globalIlluminationMode {0};
  std::int16_t m_finalGatherMode {0};
  bool m_hasGlobalIlluminationMode {false};
  bool m_hasFinalGatherMode {false};
  bool m_genProcTableEnd {false};
  bool m_hasGenProcTableEnd {false};
  std::int16_t m_mapUTile {0};
  bool m_hasMapUTile {false};
  double m_translucence {0.0};
  bool m_hasTranslucence {false};
  double m_reflectivity {0.0};
  bool m_hasReflectivity {false};
  std::int32_t m_illuminationModel {0};
  bool m_hasIlluminationModel {false};
  std::int32_t m_channelFlags {0};
  bool m_hasChannelFlags {false};
  [[nodiscard]] bool hasCompleteDxfMapTransforms() const noexcept {
    // Without an explicit normal-map marker, the reader only admits the
    // complete distinguishing tuple. A repeated pre-strength method must be
    // followed immediately by its strength boundary before publication.
    return !m_dxfPreStrengthNormalMapMethod
        && (!m_dxfInferredNormalMapBoundary
            || (m_hasNormalMapBlendFactor && m_hasNormalMapSource
                && m_hasNormalMapFileName && m_hasNormalMapProjection
                && m_hasNormalMapTiling && m_hasNormalMapAutoTransform))
        && (!m_hasDiffuseMapTransform
            || m_dxfDiffuseMapTransformCount
                == m_diffuseMapTransform.size())
        && (!m_hasSpecularMapTransform
            || m_dxfSpecularMapTransformCount
                == m_specularMapTransform.size())
        && (!m_hasReflectionMapTransform
            || m_dxfReflectionMapTransformCount
                == m_reflectionMapTransform.size())
        && (!m_hasOpacityMapTransform
            || m_dxfOpacityMapTransformCount
                == m_opacityMapTransform.size())
        && (!m_hasBumpMapTransform
            || m_dxfBumpMapTransformCount
                == m_bumpMapTransform.size())
        && (!m_hasRefractionMapTransform
            || m_dxfRefractionMapTransformCount
                == m_refractionMapTransform.size())
        && (!m_hasNormalMapTransform
            || m_dxfNormalMapTransformCount == m_normalMapTransform.size());
  }

  //! Preserve an opaque modern MATERIAL body tail for same-version replay.
  [[nodiscard]] bool setDwgRawData(std::vector<std::uint8_t> bytes,
                                   std::uint32_t bitSize,
                                   DRW::Version version) {
    if (version <= DRW::AC1018 || version == DRW::UNKNOWNV ||
        bytes.size() > (std::numeric_limits<std::uint32_t>::max)() / 8u ||
        bitSize > bytes.size() * 8u)
      return false;
    m_rawData = std::move(bytes);
    m_rawDataBitSize = bitSize;
    m_rawDataValid = true;
    m_rawDataVersion = version;
    return true;
  }
  [[nodiscard]] const std::vector<std::uint8_t> &dwgRawData() const noexcept {
    return m_rawData;
  }
  [[nodiscard]] std::uint32_t dwgRawDataBitSize() const noexcept {
    return m_rawDataBitSize;
  }
  [[nodiscard]] bool hasDwgRawData() const noexcept { return m_rawDataValid; }
  [[nodiscard]] DRW::Version dwgRawDataVersion() const noexcept {
    return m_rawDataVersion;
  }

private:
  std::size_t m_dxfRetainedNameBytes{0u};
  std::size_t m_dxfRetainedDescriptionBytes{0u};
  std::size_t m_dxfRetainedDiffuseMapFileNameBytes{0u};
  std::size_t m_dxfRetainedSpecularMapFileNameBytes{0u};
  std::size_t m_dxfRetainedReflectionMapFileNameBytes{0u};
  std::size_t m_dxfRetainedOpacityMapFileNameBytes{0u};
  std::size_t m_dxfRetainedBumpMapFileNameBytes{0u};
  std::size_t m_dxfRetainedRefractionMapFileNameBytes{0u};
  std::size_t m_dxfRetainedNormalMapFileNameBytes{0u};
  // The Normal Map block follows luminance/strength and reuses groups
  // 3/42/43/72-75. Keep those post-boundary fields out of the base-map slots.
  bool m_dxfPastPreNormalMapFields{false};
  bool m_dxfCompleteDiffuseBeforeRefraction{false};
  bool m_dxfInferredNormalMapBoundary{false};
  bool m_dxfRepeatedDiffuseMapField{false};
  // A repeated 271 with a bump tiling already bound must be followed by 465.
  bool m_dxfPreStrengthNormalMapMethod{false};
  bool m_dxfMaterialGenProcTableStarted{false};
  bool m_dxfMaterialGenProcTableEnded{false};
  std::size_t m_dxfDiffuseMapTransformCount{0u};
  std::size_t m_dxfSpecularMapTransformCount{0u};
  std::size_t m_dxfReflectionMapTransformCount{0u};
  std::size_t m_dxfOpacityMapTransformCount{0u};
  std::size_t m_dxfBumpMapTransformCount{0u};
  std::size_t m_dxfRefractionMapTransformCount{0u};
  std::size_t m_dxfNormalMapTransformCount{0u};
  std::vector<std::uint8_t> m_rawData;
  std::uint32_t m_rawDataBitSize = 0;
  bool m_rawDataValid = false;
  DRW::Version m_rawDataVersion = DRW::UNKNOWNV;
};

//! Render background object family (AcDb*Background: solid / gradient / ground-
//! plane / image / IBL / skylight).  A single class with a `m_kind`
//! discriminator (set from the entity name) because the variants share the
//! OBJECTS-section slot and overload group codes (notably 90).  DWG/DXF fields
//! are decoded into the typed carrier while the raw object remains available
//! for opaque-tail replay. Not rendered by LibreCAD (viewport backdrops) —
//! read/write parity with dwgTs.
class DRW_Background : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  enum Kind { Solid, Gradient, GroundPlane, Image, Ibl, Skylight };
  // These are file-local CLASSES ordinals used by the fixed DWG object
  // layouts.  They are not the DWG object type values (1323, 1302, ...).
  static constexpr std::uint16_t kDwgClassNumSkylight = 533;
  static constexpr std::uint16_t kDwgClassNumSolid = 542;
  static constexpr std::uint16_t kDwgClassNumGradient = 543;
  static constexpr std::uint16_t kDwgClassNumGroundPlane = 544;
  static constexpr std::uint16_t kDwgClassNumIbl = 545;
  static constexpr std::uint16_t kDwgClassNumImage = 546;
  DRW_Background() { tType = DRW::BACKGROUND; }
  DRW_Background(const DRW_Background& o):
      DRW_TableEntry(o), m_kind(o.m_kind), m_classVersion(o.m_classVersion),
      m_solidColor(o.m_solidColor), m_colorTop(o.m_colorTop),
      m_colorMiddle(o.m_colorMiddle), m_colorBottom(o.m_colorBottom),
      m_horizon(o.m_horizon), m_height(o.m_height), m_rotation(o.m_rotation),
      m_colorSkyZenith(o.m_colorSkyZenith),
      m_colorSkyHorizon(o.m_colorSkyHorizon),
      m_colorUndergroundHorizon(o.m_colorUndergroundHorizon),
      m_colorUndergroundAzimuth(o.m_colorUndergroundAzimuth),
      m_colorNear(o.m_colorNear), m_colorFar(o.m_colorFar),
      m_fileName(o.m_fileName), m_fitToScreen(o.m_fitToScreen),
      m_maintainAspect(o.m_maintainAspect), m_useTiling(o.m_useTiling),
      m_offset(o.m_offset), m_scale(o.m_scale), m_iblName(o.m_iblName),
      m_enabled(o.m_enabled), m_displayImage(o.m_displayImage),
      m_secondaryBackgroundHandle(o.m_secondaryBackgroundHandle),
      m_sunHandle(o.m_sunHandle), m_seen90(0), m_seen290(0),
      m_dxfRetainedFileNameBytes(o.m_dxfRetainedFileNameBytes),
      m_dxfRetainedIblNameBytes(o.m_dxfRetainedIblNameBytes) {
    tType = DRW::BACKGROUND;
  }
  DRW_Background& operator=(const DRW_Background& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_kind = o.m_kind;
      m_classVersion = o.m_classVersion;
      m_solidColor = o.m_solidColor;
      m_colorTop = o.m_colorTop;
      m_colorMiddle = o.m_colorMiddle;
      m_colorBottom = o.m_colorBottom;
      m_horizon = o.m_horizon;
      m_height = o.m_height;
      m_rotation = o.m_rotation;
      m_colorSkyZenith = o.m_colorSkyZenith;
      m_colorSkyHorizon = o.m_colorSkyHorizon;
      m_colorUndergroundHorizon = o.m_colorUndergroundHorizon;
      m_colorUndergroundAzimuth = o.m_colorUndergroundAzimuth;
      m_colorNear = o.m_colorNear;
      m_colorFar = o.m_colorFar;
      m_fileName = o.m_fileName;
      m_fitToScreen = o.m_fitToScreen;
      m_maintainAspect = o.m_maintainAspect;
      m_useTiling = o.m_useTiling;
      m_offset = o.m_offset;
      m_scale = o.m_scale;
      m_iblName = o.m_iblName;
      m_enabled = o.m_enabled;
      m_displayImage = o.m_displayImage;
      m_secondaryBackgroundHandle = o.m_secondaryBackgroundHandle;
      m_sunHandle = o.m_sunHandle;
      m_seen90 = 0;
      m_seen290 = 0;
      m_dxfRetainedFileNameBytes = o.m_dxfRetainedFileNameBytes;
      m_dxfRetainedIblNameBytes = o.m_dxfRetainedIblNameBytes;
      tType = DRW::BACKGROUND;
    }
    return *this;
  }

  Kind m_kind = Solid;
  std::int32_t m_classVersion = 0;
  /* solid / gradient / ground-plane colors (raw DXF color ints) */
  int m_solidColor = 0;
  int m_colorTop = 0;                /*!< gradient code 90(#2) */
  int m_colorMiddle = 0;             /*!< gradient code 91 */
  int m_colorBottom = 0;             /*!< gradient code 92 */
  double m_horizon = 0.0;            /*!< gradient code 140 */
  double m_height = 0.0;             /*!< gradient code 141 */
  double m_rotation = 0.0;           /*!< gradient code 142 / ibl code 40 */
  int m_colorSkyZenith = 0;          /*!< ground-plane code 90(#2) */
  int m_colorSkyHorizon = 0;         /*!< code 91 */
  int m_colorUndergroundHorizon = 0; /*!< code 92 */
  int m_colorUndergroundAzimuth = 0; /*!< code 93 */
  int m_colorNear = 0;               /*!< code 94 */
  int m_colorFar = 0;                /*!< code 95 */
  /* image */
  UTF8STRING m_fileName;            /*!< image code 300 */
  bool m_fitToScreen = false;       /*!< code 290 */
  bool m_maintainAspect = false;    /*!< code 291 */
  bool m_useTiling = false;         /*!< code 292 */
  DRW_Coord m_offset;               /*!< image codes 140/141 */
  DRW_Coord m_scale{1.0, 1.0, 0.0}; /*!< image codes 142/143 */
  /* ibl */
  UTF8STRING m_iblName;                          /*!< ibl code 1 */
  bool m_enabled = false;                        /*!< ibl code 290(#1) */
  bool m_displayImage = false;                   /*!< ibl code 290(#2) */
  std::uint32_t m_secondaryBackgroundHandle = 0; /*!< ibl code 340 */
  /* skylight */
  std::uint32_t m_sunHandle = 0; /*!< skylight code 340 */
protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  int m_seen90 = 0;  /*!< gradient/ground-plane/solid: 90 appears twice */
  int m_seen290 = 0; /*!< ibl: 290 appears twice (enable, displayImage) */
  std::size_t m_dxfRetainedFileNameBytes{0u};
  std::size_t m_dxfRetainedIblNameBytes{0u};
};

//! Point-cloud definition objects (AcDbPointCloudDef / ...DefEx and their
//! reactors).  File reference + load state + WCS extents.  DXF-only structured
//! decode; DWG stays raw.  Not rendered by LibreCAD (point clouds) — read
//! parity with dwgTs.
class DRW_PointCloudDef : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  enum Kind { Definition, DefinitionEx, Reactor, ReactorEx };
  static constexpr std::uint16_t kDwgClassNumDefinition = 535;
  static constexpr std::uint16_t kDwgClassNumDefinitionEx = 536;
  static constexpr std::uint16_t kDwgClassNumReactor = 537;
  static constexpr std::uint16_t kDwgClassNumReactorEx = 538;
  static constexpr std::uint64_t kMaxPointCount = 0x7fffffffffffffffULL;
  static constexpr std::int32_t kMaxClassVersion = 1000000;
  DRW_PointCloudDef() { reset(); }
  void reset();

  Kind m_kind = Definition;
  std::int32_t m_classVersion = 0; /*!< code 90 */
  UTF8STRING m_sourceFilename;     /*!< code 1 (definition/ex) */
  bool m_isLoaded = false;         /*!< code 280 */
  std::uint64_t m_pointCount = 0;  /*!< code 160 / DWG RLL */
  DRW_Coord m_extentsMin;          /*!< code 10/20/30 */
  DRW_Coord m_extentsMax;          /*!< code 11/21/31 */
protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  std::size_t m_dxfRetainedSourceFilenameBytes{0u};
};

//! NAVISWORKSMODELDEF object (AcDbNavisworksModelDef).
//! The external model is intentionally kept as metadata; the referenced file
//! is not loaded by libdxfrw.  The DWG body is a regular table-entry payload.
class DRW_NavisworksModelDef : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 539;
  static constexpr std::int32_t kMaxFlags = 0xffff;
  static constexpr std::size_t kMaxPathLength = 1u << 20;

  DRW_NavisworksModelDef() { reset(); }
  DRW_NavisworksModelDef(const DRW_NavisworksModelDef& o):
      DRW_TableEntry(o), m_flags(o.m_flags), m_path(o.m_path),
      m_status(o.m_status), m_minExtent(o.m_minExtent),
      m_maxExtent(o.m_maxExtent),
      m_hostDrawingVisibility(o.m_hostDrawingVisibility), m_seen290(0),
      m_dxfRetainedPathBytes(o.m_dxfRetainedPathBytes) {
    tType = DRW::NAVISWORKSMODELDEF;
  }
  DRW_NavisworksModelDef& operator=(const DRW_NavisworksModelDef& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_flags = o.m_flags;
      m_path = o.m_path;
      m_status = o.m_status;
      m_minExtent = o.m_minExtent;
      m_maxExtent = o.m_maxExtent;
      m_hostDrawingVisibility = o.m_hostDrawingVisibility;
      m_seen290 = 0;
      m_dxfRetainedPathBytes = o.m_dxfRetainedPathBytes;
      tType = DRW::NAVISWORKSMODELDEF;
    }
    return *this;
  }
  void reset();

  std::int32_t m_flags = 0;             /*!< code 70 / DWG BS */
  UTF8STRING m_path;                    /*!< code 1 */
  bool m_status = false;                /*!< code 290 / DWG B */
  DRW_Coord m_minExtent;                /*!< codes 10/20/30 */
  DRW_Coord m_maxExtent;                /*!< codes 11/21/31 */
  bool m_hostDrawingVisibility = false; /*!< code 291 / DWG B */
protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  int m_seen290 = 0;
  std::size_t m_dxfRetainedPathBytes{0u};
};

//! One PointCloudColorMap ramp.  DXF stores the declared color count before
//! the ramp's class version; DWG stores the same values as BL/BS.  Some files
//! declare more colors than they provide, so the declared count is preserved
//! separately from the decoded color-scheme strings.
struct DRW_PointCloudColorMapRamp {
  std::int32_t m_classVersion = 0;
  std::uint32_t m_rampCount = 0;
  std::vector<UTF8STRING> m_colorSchemes;
};

//! POINTCLOUDCOLORMAP object (AcDbPointCloudColorMap).
class DRW_PointCloudColorMap : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 540;
  static constexpr std::int32_t kMaxClassVersion =
      (std::numeric_limits<std::uint16_t>::max)();
  static constexpr std::uint32_t kMaxRamps = 4096;
  static constexpr std::uint32_t kMaxColorsPerRamp = 65536;
  static constexpr std::size_t kMaxColorSchemeLength = 1u << 20;

  DRW_PointCloudColorMap() { reset(); }
  DRW_PointCloudColorMap(const DRW_PointCloudColorMap& o):
      DRW_TableEntry(o), m_classVersion(o.m_classVersion),
      m_defaultIntensityColorScheme(o.m_defaultIntensityColorScheme),
      m_defaultElevationColorScheme(o.m_defaultElevationColorScheme),
      m_defaultClassificationColorScheme(o.m_defaultClassificationColorScheme),
      m_colorRampCount(o.m_colorRampCount), m_colorRamps(o.m_colorRamps),
      m_classificationColorRampCount(o.m_classificationColorRampCount),
      m_classificationColorRamps(o.m_classificationColorRamps),
      m_dxfClassification(false), m_dxfActiveRamp(-1),
      m_dxfDefaultSchemeCount(0),
      m_dxfRetainedDefaultSchemeBytes(o.m_dxfRetainedDefaultSchemeBytes),
      m_dxfRetainedColorRampBytes(o.m_dxfRetainedColorRampBytes),
      m_dxfRetainedClassificationRampBytes(
          o.m_dxfRetainedClassificationRampBytes) {
    tType = DRW::POINTCLOUDCOLORMAP;
  }
  DRW_PointCloudColorMap& operator=(const DRW_PointCloudColorMap& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_classVersion = o.m_classVersion;
      m_defaultIntensityColorScheme = o.m_defaultIntensityColorScheme;
      m_defaultElevationColorScheme = o.m_defaultElevationColorScheme;
      m_defaultClassificationColorScheme =
          o.m_defaultClassificationColorScheme;
      m_colorRampCount = o.m_colorRampCount;
      m_colorRamps = o.m_colorRamps;
      m_classificationColorRampCount = o.m_classificationColorRampCount;
      m_classificationColorRamps = o.m_classificationColorRamps;
      m_dxfRetainedDefaultSchemeBytes = o.m_dxfRetainedDefaultSchemeBytes;
      m_dxfRetainedColorRampBytes = o.m_dxfRetainedColorRampBytes;
      m_dxfRetainedClassificationRampBytes =
          o.m_dxfRetainedClassificationRampBytes;
      m_dxfClassification = false;
      m_dxfActiveRamp = -1;
      m_dxfDefaultSchemeCount = 0;
      tType = DRW::POINTCLOUDCOLORMAP;
    }
    return *this;
  }
  void reset();

  std::int32_t m_classVersion = 0; /*!< code 90 / DWG BS */
  UTF8STRING m_defaultIntensityColorScheme;
  UTF8STRING m_defaultElevationColorScheme;
  UTF8STRING m_defaultClassificationColorScheme;
  std::uint32_t m_colorRampCount = 0;
  std::vector<DRW_PointCloudColorMapRamp> m_colorRamps;
  std::uint32_t m_classificationColorRampCount = 0;
  std::vector<DRW_PointCloudColorMapRamp> m_classificationColorRamps;

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  bool m_dxfClassification = false;
  std::int32_t m_dxfActiveRamp = -1;
  std::size_t m_dxfDefaultSchemeCount = 0;
  std::array<std::size_t, 3> m_dxfRetainedDefaultSchemeBytes{};
  std::vector<std::size_t> m_dxfRetainedColorRampBytes;
  std::vector<std::size_t> m_dxfRetainedClassificationRampBytes;
};

struct DRW_SunStudyDate {
  std::int32_t m_julianDay = 0;
  std::int32_t m_milliseconds = 0;
};

//! SUNSTUDY (AcDbSunStudy) object.  The bounded date/hour vectors are modeled
//! as well as the scalar study configuration and four reference handles.
//! Not rendered by LibreCAD, but fully decoded for DWG/DXF parity.
class DRW_SunStudy : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 548;
  static constexpr std::uint16_t kDwgType = 1325;
  static constexpr std::int32_t kMaxDateCount = 10000;
  static constexpr std::int32_t kMaxHourCount = 10000;
  DRW_SunStudy() { reset(); }
  DRW_SunStudy(const DRW_SunStudy& o):
      DRW_TableEntry(o), m_classVersion(o.m_classVersion),
      m_setupName(o.m_setupName), m_description(o.m_description),
      m_sheetSetName(o.m_sheetSetName), m_sheetSubsetName(o.m_sheetSubsetName),
      m_outputType(o.m_outputType), m_useSubset(o.m_useSubset),
      m_selectDatesFromCalendar(o.m_selectDatesFromCalendar),
      m_selectRangeOfDates(o.m_selectRangeOfDates),
      m_lockViewports(o.m_lockViewports), m_labelViewports(o.m_labelViewports),
      m_startTime(o.m_startTime), m_endTime(o.m_endTime),
      m_interval(o.m_interval), m_shadePlotType(o.m_shadePlotType),
      m_viewportCount(o.m_viewportCount), m_rowCount(o.m_rowCount),
      m_columnCount(o.m_columnCount), m_spacing(o.m_spacing),
      m_dates(o.m_dates), m_hours(o.m_hours),
      m_pageSetupWizardHandle(o.m_pageSetupWizardHandle),
      m_viewHandle(o.m_viewHandle),
      m_visualStyleHandle(o.m_visualStyleHandle),
      m_textStyleHandle(o.m_textStyleHandle),
      m_dxfRetainedTextBytes(o.m_dxfRetainedTextBytes),
      m_seen90(0), m_seen290(0),
      m_dxfDateCountSeen(false), m_dxfHourCountSeen(false),
      m_dxfDateValuesRemaining(0), m_dxfDateValue(0),
      m_dxfHasPendingDateValue(false) {
    tType = DRW::SUNSTUDY;
  }
  DRW_SunStudy& operator=(const DRW_SunStudy& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_classVersion = o.m_classVersion;
      m_setupName = o.m_setupName;
      m_description = o.m_description;
      m_sheetSetName = o.m_sheetSetName;
      m_sheetSubsetName = o.m_sheetSubsetName;
      m_outputType = o.m_outputType;
      m_useSubset = o.m_useSubset;
      m_selectDatesFromCalendar = o.m_selectDatesFromCalendar;
      m_selectRangeOfDates = o.m_selectRangeOfDates;
      m_lockViewports = o.m_lockViewports;
      m_labelViewports = o.m_labelViewports;
      m_startTime = o.m_startTime;
      m_endTime = o.m_endTime;
      m_interval = o.m_interval;
      m_shadePlotType = o.m_shadePlotType;
      m_viewportCount = o.m_viewportCount;
      m_rowCount = o.m_rowCount;
      m_columnCount = o.m_columnCount;
      m_spacing = o.m_spacing;
      m_dates = o.m_dates;
      m_hours = o.m_hours;
      m_pageSetupWizardHandle = o.m_pageSetupWizardHandle;
      m_viewHandle = o.m_viewHandle;
      m_visualStyleHandle = o.m_visualStyleHandle;
      m_textStyleHandle = o.m_textStyleHandle;
      m_seen90 = 0;
      m_seen290 = 0;
      m_dxfDateCountSeen = false;
      m_dxfHourCountSeen = false;
      m_dxfDateValuesRemaining = 0;
      m_dxfDateValue = 0;
      m_dxfHasPendingDateValue = false;
      m_dxfRetainedTextBytes = o.m_dxfRetainedTextBytes;
      tType = DRW::SUNSTUDY;
    }
    return *this;
  }

  void reset();

  std::int32_t m_classVersion = 0;        /*!< code 90 (first) */
  UTF8STRING m_setupName;                 /*!< code 1 */
  UTF8STRING m_description;               /*!< code 2 */
  UTF8STRING m_sheetSetName;              /*!< code 3 */
  UTF8STRING m_sheetSubsetName;           /*!< code 4 */
  std::int32_t m_outputType = 0;          /*!< code 70, DWG BL */
  bool m_useSubset = false;               /*!< code 290 (first) */
  bool m_selectDatesFromCalendar = false; /*!< code 291 */
  bool m_selectRangeOfDates = false;      /*!< code 292 */
  bool m_lockViewports = false;           /*!< code 293 */
  bool m_labelViewports = false;          /*!< code 294 */
  std::int32_t m_startTime = 0;           /*!< code 93 */
  std::int32_t m_endTime = 0;             /*!< code 94 */
  std::int32_t m_interval = 0;            /*!< code 95 */
  std::int32_t m_shadePlotType = 0;       /*!< code 74, DWG BL */
  std::int32_t m_viewportCount = 0;       /*!< code 75, DWG BL */
  std::int32_t m_rowCount = 0;            /*!< code 76, DWG BL */
  std::int32_t m_columnCount = 0;         /*!< code 77, DWG BL */
  double m_spacing = 0.0;                 /*!< code 40 */
  std::vector<DRW_SunStudyDate> m_dates;
  std::vector<bool> m_hours;
  std::uint32_t m_pageSetupWizardHandle = 0; /*!< first code 340 */
  std::uint32_t m_viewHandle = 0;            /*!< code 341 */
  std::uint32_t m_visualStyleHandle = 0;     /*!< code 342 */
  std::uint32_t m_textStyleHandle = 0;       /*!< code 343 */
protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  std::array<std::size_t, 4> m_dxfRetainedTextBytes{};
  int m_seen90 = 0;
  int m_seen290 = 0;
  bool m_dxfDateCountSeen = false;
  bool m_dxfHourCountSeen = false;
  std::int32_t m_dxfDateValuesRemaining = 0;
  std::int32_t m_dxfDateValue = 0;
  bool m_dxfHasPendingDateValue = false;
};

//! MOTIONPATH (AcDbMotionPath) object.  The three path references are hard
//! pointers in the DWG body; owner/reactor/extension handles use the common
//! object handle stream.
class DRW_MotionPath : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 552;
  static constexpr std::uint16_t kDwgType = 1310;

  DRW_MotionPath() { reset(); }
  DRW_MotionPath(const DRW_MotionPath& o):
      DRW_TableEntry(o), m_classVersion(o.m_classVersion),
      m_cameraPathHandle(o.m_cameraPathHandle),
      m_targetPathHandle(o.m_targetPathHandle),
      m_viewTableHandle(o.m_viewTableHandle), m_frames(o.m_frames),
      m_frameRate(o.m_frameRate), m_cornerDeceleration(o.m_cornerDeceleration),
      m_seen90(0), m_seen340(0) {
    tType = DRW::MOTIONPATH;
  }
  DRW_MotionPath& operator=(const DRW_MotionPath& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_classVersion = o.m_classVersion;
      m_cameraPathHandle = o.m_cameraPathHandle;
      m_targetPathHandle = o.m_targetPathHandle;
      m_viewTableHandle = o.m_viewTableHandle;
      m_frames = o.m_frames;
      m_frameRate = o.m_frameRate;
      m_cornerDeceleration = o.m_cornerDeceleration;
      m_seen90 = 0;
      m_seen340 = 0;
      tType = DRW::MOTIONPATH;
    }
    return *this;
  }
  void reset();

  std::int32_t m_classVersion = 0;      /*!< code 90 */
  std::uint32_t m_cameraPathHandle = 0; /*!< code 340 (first) */
  std::uint32_t m_targetPathHandle = 0; /*!< code 340 (second) */
  std::uint32_t m_viewTableHandle = 0;  /*!< code 340 (third) */
  std::int32_t m_frames = 0;            /*!< code 90 (second) */
  std::int32_t m_frameRate = 0;         /*!< code 90 (third) */
  bool m_cornerDeceleration = false;    /*!< code 290 */
protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  int m_seen90 = 0;
  int m_seen340 = 0;
};

//! CURVEPATH (AcDbCurvePath) object.  The entity reference is a hard pointer
//! in the DWG body; common owner/reactor/extension handles follow it.
class DRW_CurvePath : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 553;
  static constexpr std::uint16_t kDwgType = 1294;

  DRW_CurvePath() { reset(); }
  DRW_CurvePath(const DRW_CurvePath& o):
      DRW_TableEntry(o), m_classVersion(o.m_classVersion),
      m_entityHandle(o.m_entityHandle), m_seen90(0), m_seen340(0) {
    tType = DRW::CURVEPATH;
  }
  DRW_CurvePath& operator=(const DRW_CurvePath& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_classVersion = o.m_classVersion;
      m_entityHandle = o.m_entityHandle;
      m_seen90 = 0;
      m_seen340 = 0;
      tType = DRW::CURVEPATH;
    }
    return *this;
  }
  void reset();

  std::int32_t m_classVersion = 0;  /*!< code 90 */
  std::uint32_t m_entityHandle = 0; /*!< code 340 */
protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  int m_seen90 = 0;
  int m_seen340 = 0;
};

//! POINTPATH (AcDbPointPath) object.  The path point is encoded as a 3BD in
//! DWG and as the positional 10/20/30 coordinate triplet in DXF.
class DRW_PointPath : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 554;
  static constexpr std::uint16_t kDwgType = 1314;

  DRW_PointPath() { reset(); }
  DRW_PointPath(const DRW_PointPath& o):
      DRW_TableEntry(o), m_classVersion(o.m_classVersion),
      m_point(o.m_point), m_seen90(0), m_seen10(0) {
    tType = DRW::POINTPATH;
  }
  DRW_PointPath& operator=(const DRW_PointPath& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_classVersion = o.m_classVersion;
      m_point = o.m_point;
      m_seen90 = 0;
      m_seen10 = 0;
      tType = DRW::POINTPATH;
    }
    return *this;
  }
  void reset();

  std::int32_t m_classVersion = 0; /*!< code 90 */
  DRW_Coord m_point;               /*!< codes 10/20/30 */
protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  int m_seen90 = 0;
  int m_seen10 = 0;
};

//! OBJECT_PTR (AcDbObjectPtr) object.  It has no type-specific body; only the
//! common object handle stream is present after the standard DWG frame.
class DRW_ObjectPtr : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 555;
  static constexpr std::uint16_t kDwgType = 1311;

  DRW_ObjectPtr() { reset(); }
  void reset();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;
};

//! PARTIAL_VIEWING_INDEX (AcDbPartialViewingIndex) object.  Each entry stores
//! a pair of extents followed by a hard-pointer reference to the indexed
//! object.  The common object handle stream follows the entry list.
struct DRW_PartialViewingIndexEntry {
  DRW_Coord extentsMin;
  DRW_Coord extentsMax;
  std::uint32_t objectHandle = 0;
};

class DRW_PartialViewingIndex : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 559;
  static constexpr std::uint16_t kDwgType = 1312;
  static constexpr std::uint32_t kMaxEntryCount = 1000000;

  DRW_PartialViewingIndex() { reset(); }
  DRW_PartialViewingIndex(const DRW_PartialViewingIndex& o):
      DRW_TableEntry(o), m_entryCount(o.m_entryCount),
      m_hasEntries(o.m_hasEntries), m_entries(o.m_entries),
      m_pendingEntry(), m_pendingFields(0) {
    tType = DRW::PARTIALVIEWINGINDEX;
  }
  DRW_PartialViewingIndex& operator=(const DRW_PartialViewingIndex& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_entryCount = o.m_entryCount;
      m_hasEntries = o.m_hasEntries;
      m_entries = o.m_entries;
      m_pendingEntry = DRW_PartialViewingIndexEntry{};
      m_pendingFields = 0;
      tType = DRW::PARTIALVIEWINGINDEX;
    }
    return *this;
  }
  void reset();

  std::uint32_t m_entryCount = 0;
  bool m_hasEntries = false;
  std::vector<DRW_PartialViewingIndexEntry> m_entries;

  //! Validate the final entry assembled by the positional DXF parser.
  bool finalizeDxf();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  DRW_PartialViewingIndexEntry m_pendingEntry;
  std::uint8_t m_pendingFields = 0;
};

//! Render-settings object family (AcDbRenderSettings / Global / Environment /
//! Entry / MentalRay / RapidRT).  These DXF records are POSITIONAL — codes
//! 90/1/ 290/70/280/40 repeat and are assigned by order, not by distinct codes
//! — so the raw values are captured into per-code vectors.  The DWG base and
//! derived bodies are decoded with their bounded object frame; unknown render
//! settings are still retained by the raw-object path for same-version replay.
class DRW_RenderSettings : public DRW_TableEntry {
  SETOBJFRIENDS
  friend class dxfRenderSettings::Decoder;
public:
  enum Kind { Settings, Global, Environment, Entry, RapidRT, MentalRay };
  static constexpr std::uint16_t kDwgClassNumEntry = 549;
  static constexpr std::uint16_t kDwgClassNumEnvironment = 550;
  static constexpr std::uint16_t kDwgClassNumGlobal = 551;
  static constexpr std::uint16_t kDwgClassNumSettings = 556;
  static constexpr std::uint16_t kDwgClassNumMentalRay = 557;
  static constexpr std::uint16_t kDwgClassNumRapidRT = 558;
  static constexpr std::uint16_t kDwgTypeMentalRay = 1309;
  static constexpr std::uint16_t kDwgTypeRapidRT = 1315;
  static constexpr std::uint16_t kDwgTypeEntry = 1316;
  static constexpr std::uint16_t kDwgTypeEnvironment = 1317;
  static constexpr std::uint16_t kDwgTypeGlobal = 1318;
  static constexpr std::uint16_t kDwgTypeSettings = 1319;
  DRW_RenderSettings() { tType = DRW::RENDERSETTINGS; }

  void reset();

  Kind m_kind = Settings;
  std::int32_t m_classVersion = 0; /*!< first code 90 */
  UTF8STRING m_name; /*!< first code 1 (filename/preset/save name) */
  /* positional value capture */
  std::vector<std::int32_t> m_longs;  /*!< code 90 */
  std::vector<UTF8STRING> m_strings;  /*!< code 1 */
  std::vector<bool> m_bools;          /*!< code 290 */
  std::vector<std::int32_t> m_shorts; /*!< code 70 */
  std::vector<std::int32_t> m_bytes;  /*!< code 280 (RC) */
  std::vector<double> m_doubles;      /*!< code 40 */
  /* named fields finalized for the documented kinds */
  // Preset records retain the historical aliases: materials, texture sampling
  // and shadows use fogEnabled, fogBackgroundEnabled and environmentImageEnabled.
  // Environment records use the literal fog/image meanings. Preset image text
  // is the preview filename, not an environment map.
  bool m_fogEnabled = false;
  bool m_fogBackgroundEnabled = false;
  bool m_backfacesEnabled = false;
  bool m_environmentImageEnabled = false;
  UTF8STRING m_environmentImageFilename;
  UTF8STRING m_description;
  std::int32_t m_displayIndex = 0;
  bool m_hasPredefined = false;
  bool m_hasPredefinedPresent = false;
  int m_fogColorR = 0, m_fogColorG = 0, m_fogColorB = 0;
  double m_fogDensityNear = 0.0, m_fogDensityFar = 0.0;
  double m_fogDistanceNear = 0.0, m_fogDistanceFar = 0.0;
  std::int32_t m_procedure = 0;   /*!< Global: longs[1] */
  std::int32_t m_destination = 0; /*!< Global: longs[2] */

  //! Preserve an opaque modern render-settings body tail for same-version
  //! replay.
  [[nodiscard]] bool setDwgRawData(std::vector<std::uint8_t> bytes,
                                   std::uint32_t bitSize,
                                   DRW::Version version) {
    if (version <= DRW::AC1018 || version == DRW::UNKNOWNV ||
        bytes.size() > (std::numeric_limits<std::uint32_t>::max)() / 8u ||
        bitSize > bytes.size() * 8u)
      return false;
    m_rawData = std::move(bytes);
    m_rawDataBitSize = bitSize;
    m_rawDataValid = true;
    m_rawDataVersion = version;
    return true;
  }
  [[nodiscard]] const std::vector<std::uint8_t> &dwgRawData() const noexcept {
    return m_rawData;
  }
  [[nodiscard]] std::uint32_t dwgRawDataBitSize() const noexcept {
    return m_rawDataBitSize;
  }
  [[nodiscard]] bool hasDwgRawData() const noexcept { return m_rawDataValid; }
  [[nodiscard]] DRW::Version dwgRawDataVersion() const noexcept {
    return m_rawDataVersion;
  }

  //! Assign named fields from the captured vectors (after the parse loop).
  void finalize();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  // Internal DXF decoder supplies string roles from the original subclass.
  // Roles:1 name,2 image/preview filename,4 description.8 selects the legacy,
  // markerless parseCode projection. No parser state is
  // added to installed models and native DWG finalization remains separate.
  bool parseDxfValue(int code, const std::unique_ptr<dxfReader>& reader,
                     unsigned stringRoles);
  std::size_t m_dxfRetainedStringVectorBytes{0u};
  std::size_t m_dxfRetainedNameBytes{0u};
  std::size_t m_dxfRetainedEnvironmentFilenameBytes{0u};
  std::size_t m_dxfRetainedDescriptionBytes{0u};
  std::vector<std::uint8_t> m_rawData;
  std::uint32_t m_rawDataBitSize = 0;
  bool m_rawDataValid = false;
  DRW::Version m_rawDataVersion = DRW::UNKNOWNV;
};

struct DRW_SectionGeometrySettings {
  std::int32_t m_numGeometries = 0;
  std::int32_t m_hexIndex = 0;
  std::int32_t m_flags = 0;
  std::int32_t m_color = 256;
  std::int32_t m_rgbColor = 0;
  bool m_hasRgbColor = false;
  UTF8STRING m_layer;
  UTF8STRING m_lineType;
  double m_lineTypeScale = 1.0;
  UTF8STRING m_plotStyle;
  std::int32_t m_lineWeight = -1;
  std::uint16_t m_faceTransparency = 0;
  std::uint16_t m_edgeTransparency = 0;
  std::uint16_t m_hatchType = 0;
  UTF8STRING m_hatchPattern = "SOLID";
  double m_hatchAngle = 0.0;
  double m_hatchSpacing = 1.0;
  double m_hatchScale = 1.0;
  // DWG CMC source details are appended so existing member offsets remain
  // stable.  The CMC is represented in DXF only by groups 62/420, so names
  // and source flags must be retained here for DWG replay and guarded when
  // projecting to that scalar DXF representation.
  std::uint8_t m_colorMethod = 0;
  std::uint8_t m_colorFlags = 0;
  std::uint16_t m_colorRawIndex = 0;
  UTF8STRING m_colorName;
  UTF8STRING m_colorBookName;

private:
  friend class DRW_Section;
  std::size_t m_dxfRetainedLayerBytes{0u};
  std::size_t m_dxfRetainedLineTypeBytes{0u};
  std::size_t m_dxfRetainedPlotStyleBytes{0u};
  std::size_t m_dxfRetainedHatchPatternBytes{0u};
};

struct DRW_SectionTypeSettings {
  std::int32_t m_type = 0;
  std::int32_t m_generation = 0;
  std::int32_t m_numSources = 0;
  std::vector<std::uint32_t> m_sourceHandles;
  std::uint32_t m_destinationBlockHandle = 0;
  UTF8STRING m_destinationFile;
  std::int32_t m_numGeometrySettings = 0;
  std::vector<DRW_SectionGeometrySettings> m_geometry;

private:
  friend class DRW_Section;
  std::size_t m_dxfRetainedDestinationFileBytes{0u};
};

//! Section objects: SECTION_MANAGER (live flag + section-object handle list)
//! and SECTION_SETTINGS (current type + bounded type/geometry settings). Body
//! handles are read after the common object handles and are retained in the
//! typed carriers; the raw object carrier still preserves same-version bytes.
class DRW_Section : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  enum Kind { Manager, Settings };
  static constexpr std::uint16_t kDwgClassNumManager = 1321;
  static constexpr std::uint16_t kDwgClassNumSettings = 1322;
  static constexpr std::int32_t kMaxSectionCount = 100000;
  static constexpr std::int32_t kMaxSectionTypeCount = 4;
  static constexpr std::int32_t kMaxSectionSourceCount = 100000;
  static constexpr std::int32_t kMaxSectionGeometryCount = 100000;
  static constexpr std::int32_t kMaxSectionGeometryEntryCount = 100000;

  DRW_Section() { reset(); }
  DRW_Section(const DRW_Section& o):
      DRW_TableEntry(o), m_kind(o.m_kind), m_isLive(o.m_isLive),
      m_sectionCount(o.m_sectionCount), m_sectionHandles(o.m_sectionHandles),
      m_classVersion(o.m_classVersion), m_sectionType(o.m_sectionType),
      m_generationOptions(o.m_generationOptions), m_currentType(o.m_currentType),
      m_typeCount(o.m_typeCount), m_types(o.m_types),
      m_sourceHandle(o.m_sourceHandle),
      m_destinationBlockHandle(o.m_destinationBlockHandle),
      m_destinationFileHandle(o.m_destinationFileHandle),
      m_destinationFile(o.m_destinationFile),
      m_dxfRetainedDestinationTextBytes(
          o.m_dxfRetainedDestinationTextBytes),
      m_dxfRetainedDestinationFileMirrorBytes(
          o.m_dxfRetainedDestinationFileMirrorBytes),
      m_dxfInBody(false),
      m_dxfTypeIndex(-1), m_dxfGeometryIndex(-1), m_settingsInts(),
      m_rawData(o.m_rawData), m_rawDataBitSize(o.m_rawDataBitSize),
      m_rawDataValid(o.m_rawDataValid), m_rawDataVersion(o.m_rawDataVersion) {
    tType = DRW::SECTIONOBJ;
  }
  DRW_Section& operator=(const DRW_Section& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_kind = o.m_kind;
      m_isLive = o.m_isLive;
      m_sectionCount = o.m_sectionCount;
      m_sectionHandles = o.m_sectionHandles;
      m_classVersion = o.m_classVersion;
      m_sectionType = o.m_sectionType;
      m_generationOptions = o.m_generationOptions;
      m_currentType = o.m_currentType;
      m_typeCount = o.m_typeCount;
      m_types = o.m_types;
      m_sourceHandle = o.m_sourceHandle;
      m_destinationBlockHandle = o.m_destinationBlockHandle;
      m_destinationFileHandle = o.m_destinationFileHandle;
      m_destinationFile = o.m_destinationFile;
      m_dxfRetainedDestinationTextBytes =
          o.m_dxfRetainedDestinationTextBytes;
      m_dxfRetainedDestinationFileMirrorBytes =
          o.m_dxfRetainedDestinationFileMirrorBytes;
      m_dxfInBody = false;
      m_dxfTypeIndex = -1;
      m_dxfGeometryIndex = -1;
      m_settingsInts.clear();
      m_rawData = o.m_rawData;
      m_rawDataBitSize = o.m_rawDataBitSize;
      m_rawDataValid = o.m_rawDataValid;
      m_rawDataVersion = o.m_rawDataVersion;
      tType = DRW::SECTIONOBJ;
    }
    return *this;
  }

  void reset() {
    const Kind kind = m_kind;
    tType = DRW::SECTIONOBJ;
    m_isLive = false;
    m_sectionCount = 0;
    m_sectionHandles.clear();
    m_classVersion = 0;
    m_sectionType = 0;
    m_generationOptions = 0;
    m_currentType = 0;
    m_typeCount = 0;
    m_types.clear();
    m_sourceHandle = 0;
    m_destinationBlockHandle = 0;
    m_destinationFileHandle = 0;
    m_destinationFile.clear();
    m_dxfRetainedDestinationTextBytes = 0u;
    m_dxfRetainedDestinationFileMirrorBytes = 0u;
    m_rawData.clear();
    m_rawDataBitSize = 0;
    m_rawDataValid = false;
    m_rawDataVersion = DRW::UNKNOWNV;
    m_dxfInBody = false;
    m_dxfTypeIndex = -1;
    m_dxfGeometryIndex = -1;
    m_settingsInts.clear();
    DRW_TableEntry::reset();
    m_kind = kind;
  }

  Kind m_kind = Manager;
  /* manager */
  bool m_isLive = false;                       /*!< code 70 */
  std::int32_t m_sectionCount = 0;             /*!< code 90 (manager) */
  std::vector<std::uint32_t> m_sectionHandles; /*!< body code 330 */
  /* settings */
  std::int32_t m_classVersion = 0;              /*!< ints[0] (90/91) */
  std::int32_t m_sectionType = 0;               /*!< ints[1] */
  std::int32_t m_generationOptions = 0;         /*!< ints[2] */
  std::int32_t m_currentType = 0;               /*!< DWG curr_type */
  std::int32_t m_typeCount = 0;                 /*!< DWG num_types (max 4) */
  std::vector<DRW_SectionTypeSettings> m_types; /*!< DWG type settings */
  std::uint32_t m_sourceHandle = 0;             /*!< DWG source handle */
  std::uint32_t m_destinationBlockHandle = 0;   /*!< code 331 */
  std::uint32_t m_destinationFileHandle = 0;    /*!< DWG destination file */
  UTF8STRING m_destinationFile;                 /*!< DWG destination file */

  //! Preserve an opaque modern SECTION body tail for same-version replay.
  [[nodiscard]] bool setDwgRawData(std::vector<std::uint8_t> bytes,
                                   std::uint32_t bitSize,
                                   DRW::Version version) {
    if (version <= DRW::AC1018 || version == DRW::UNKNOWNV ||
        bytes.size() > (std::numeric_limits<std::uint32_t>::max)() / 8u ||
        bitSize > bytes.size() * 8u)
      return false;
    m_rawData = std::move(bytes);
    m_rawDataBitSize = bitSize;
    m_rawDataValid = true;
    m_rawDataVersion = version;
    return true;
  }
  [[nodiscard]] const std::vector<std::uint8_t> &dwgRawData() const noexcept {
    return m_rawData;
  }
  [[nodiscard]] std::uint32_t dwgRawDataBitSize() const noexcept {
    return m_rawDataBitSize;
  }
  [[nodiscard]] bool hasDwgRawData() const noexcept { return m_rawDataValid; }
  [[nodiscard]] DRW::Version dwgRawDataVersion() const noexcept {
    return m_rawDataVersion;
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

private:
  // Group 1 is retained in both the current type and this top-level mirror.
  std::size_t m_dxfRetainedDestinationTextBytes{0u};
  std::size_t m_dxfRetainedDestinationFileMirrorBytes{0u};
  bool m_dxfInBody = false;
  int m_dxfTypeIndex = -1;
  int m_dxfGeometryIndex = -1;
  std::vector<std::int32_t> m_settingsInts; /*!< settings: 90/91 accumulation */
  std::vector<std::uint8_t> m_rawData;
  std::uint32_t m_rawDataBitSize = 0;
  bool m_rawDataValid = false;
  DRW::Version m_rawDataVersion = DRW::UNKNOWNV;
};

//! DWG CMC companions for the scalar TABLESTYLE color fields.
//! A zero method denotes caller-authored/DXF-origin data that may be inferred.
struct DRW_TableStyleCmcColor {
  std::int32_t m_rgb = -1;
  UTF8STRING m_name;
  UTF8STRING m_bookName;
  std::uint8_t m_method = 0;
  std::uint8_t m_flags = 0;
  std::uint16_t m_reservedIndex = 0;

private:
  friend class DRW_TableStyle;
  friend class DRW_CellStyleMap;
  std::size_t m_dxfRetainedNameBytes{0u};
  std::size_t m_dxfRetainedBookNameBytes{0u};
  bool m_dxfNamePresent{false};
  bool m_dxfBookNamePresent{false};
};

struct DRW_TableStyleBorder {
  int m_edgeFlags = 0;
  int m_propertyOverrideFlags = 0;
  int m_borderType = 0;
  int m_color = 0;
  int m_lineWeight = 0;
  std::uint32_t m_lineTypeHandle = 0;
  // Context-dependent wire integer: legacy row borders use visibility;
  // rich cell-style group 93 is invisibility (0 visible, 1 invisible).
  // Do not interpret this shared carrier as a universal semantic bool.
  int m_visible = 0;
  double m_doubleLineSpacing = 0.0;
  DRW_TableStyleCmcColor m_colorCmc;
};

struct DRW_TableStyleContentFormat {
  std::uint32_t m_propertyOverrideFlags = 0;
  std::uint32_t m_propertyFlags = 0;
  int m_valueDataType = 0;
  int m_valueUnitType = 0;
  UTF8STRING m_valueFormatString;
  double m_rotation = 0.0;
  double m_blockScale = 1.0;
  int m_cellAlignment = 0;
  int m_contentColor = 0;
  DRW_TableStyleCmcColor m_contentColorCmc;
  std::uint32_t m_textStyleHandle = 0;
  double m_textHeight = 0.0;

private:
  friend class DRW_TableStyle;
  friend class DRW_CellStyleMap;
  std::size_t m_dxfRetainedValueFormatBytes{0u};
};

struct DRW_TableStyleCellStyle {
  int m_type = 0;
  bool m_hasData = false;
  std::uint32_t m_propertyOverrideFlags = 0;
  std::uint32_t m_mergeFlags = 0;
  int m_backgroundColor = 0;
  DRW_TableStyleCmcColor m_backgroundColorCmc;
  std::uint32_t m_contentLayout = 0;
  DRW_TableStyleContentFormat m_contentFormat;
  int m_marginOverrideFlags = 0;
  double m_verticalMargin = 0.0;
  double m_horizontalMargin = 0.0;
  double m_bottomMargin = 0.0;
  double m_rightMargin = 0.0;
  double m_marginHorizontalSpacing = 0.0;
  double m_marginVerticalSpacing = 0.0;
  int m_id = 0;
  int m_styleClass = 0;
  UTF8STRING m_name;
  std::vector<DRW_TableStyleBorder> m_borders;

private:
  friend class DRW_TableStyle;
  friend class DRW_CellStyleMap;
  std::size_t m_dxfRetainedNameBytes{0u};
};

struct DRW_TableStyleRowStyle {
  UTF8STRING m_textStyleName;
  std::uint32_t m_textStyleHandle = 0;
  double m_textHeight = 0.0;
  int m_textAlignment = 0;
  int m_textColor = 0;
  int m_fillColor = 0;
  DRW_TableStyleCmcColor m_textColorCmc;
  DRW_TableStyleCmcColor m_fillColorCmc;
  bool m_hasBackgroundColor = false;
  int m_valueDataType = 0;
  int m_valueUnitType = 0;
  UTF8STRING m_valueFormatString;
  std::vector<DRW_TableStyleBorder> m_borders;

private:
  friend class DRW_TableStyle;
  std::uint16_t m_dxfFieldsPresent{0u};
  std::uint8_t m_dxfLineWeightMask{0u};
  std::uint8_t m_dxfVisibilityMask{0u};
  std::uint8_t m_dxfBorderColorMask{0u};
  std::size_t m_dxfRetainedTextStyleNameBytes{0u};
};

//! Class to handle TABLESTYLE (AcDbTableStyle) identity/core fields.
class DRW_TableStyle : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 509;

  DRW_TableStyle() { reset(); }
  void reset() {
    tType = DRW::TABLESTYLE;
    m_name.clear();
    m_dxfRetainedTableStyleNameBytes = 0u;
    m_dxfOriginalName.clear();
    m_dxfOriginalNameRaw.clear();
    m_dxfRetainedTableStyleOriginalNameBytes = 0u;
    m_dxfRetainedTableStyleRawNameBytes = 0u;
    m_dxfHasDescriptionBaseline = false;
    m_flowDirection = 0;
    m_flags = 0;
    m_horizontalCellMargin = 0.0;
    m_verticalCellMargin = 0.0;
    m_titleSuppressed = false;
    m_headerSuppressed = false;
    m_unknownHandle = 0;
    m_tableCellStyle = DRW_TableStyleCellStyle();
    m_rowStyles.clear();
    m_cellStyles.clear();
    m_subrecordRanges.clear();
    m_dxfCurrentRowStyle = -1;
    m_dxfCurrentBorder = -1;
    m_dxfColorTarget = DxfColorTarget::None;
    m_dxfNestedRowsValid = true;
    m_dxfSawTableStyleBody = false;
    m_dxfSawTableStyleHeader = false;
    m_dxfCoreFieldsPresent = 0u;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  [[nodiscard]] bool validateDwgPayload(DRW::Version version) const;
  [[nodiscard]] bool finalizeDxfNestedStyles() noexcept;
  [[nodiscard]] bool hasDxfDescriptionBaseline() const noexcept {
    return m_dxfHasDescriptionBaseline;
  }
  [[nodiscard]] const UTF8STRING& dxfOriginalDescription() const noexcept {
    return m_dxfOriginalName;
  }
  [[nodiscard]] const std::string& dxfOriginalDescriptionRaw() const noexcept {
    return m_dxfOriginalNameRaw;
  }
  UTF8STRING m_name;
  int m_flowDirection = 0;
  int m_flags = 0;
  double m_horizontalCellMargin = 0.0;
  double m_verticalCellMargin = 0.0;
  bool m_titleSuppressed = false;
  bool m_headerSuppressed = false;
  std::uint32_t m_unknownHandle = 0;
  DRW_TableStyleCellStyle m_tableCellStyle;
  std::vector<DRW_TableStyleRowStyle> m_rowStyles;
  std::vector<DRW_TableStyleCellStyle> m_cellStyles;
  std::vector<DRW_DwgSubrecordRange> m_subrecordRanges;

private:
  enum class DxfColorTarget : std::uint8_t {
    None,
    Text,
    Fill,
    Border
  };
  int m_dxfCurrentRowStyle{-1};
  int m_dxfCurrentBorder{-1};
  DxfColorTarget m_dxfColorTarget{DxfColorTarget::None};
  bool m_dxfNestedRowsValid{true};
  bool m_dxfSawTableStyleBody{false};
  bool m_dxfSawTableStyleHeader{false};
  std::uint16_t m_dxfCoreFieldsPresent{0u};
  std::size_t m_dxfRetainedTableStyleNameBytes{0u};
  UTF8STRING m_dxfOriginalName;
  std::string m_dxfOriginalNameRaw;
  std::size_t m_dxfRetainedTableStyleOriginalNameBytes{0u};
  std::size_t m_dxfRetainedTableStyleRawNameBytes{0u};
  bool m_dxfHasDescriptionBaseline{false};
};

//! Class to handle CELLSTYLEMAP (AcDbCellStyleMap).
class DRW_CellStyleMap : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_CellStyleMap() { reset(); }
  void reset() {
    tType = DRW::CELLSTYLEMAP;
    m_cellStyles.clear();
    m_subrecordRanges.clear();
    m_dxfSection = DxfSection::None;
    m_dxfColorTarget = DxfColorTarget::None;
    m_dxfSawSubclass = false;
    m_dxfValid = true;
    m_dxfSawCount = false;
    m_dxfDeclaredCount = 0;
    m_dxfCompletedCount = 0;
    m_dxfCellStyleIds.clear();
    m_dxfCurrentStyle = -1;
    m_dxfCurrentBorder = -1;
    m_dxfTableFieldsPresent = 0;
    m_dxfContentFieldsPresent = 0;
    m_dxfGridFieldsPresent = 0;
    m_dxfIdentityFieldsPresent = 0;
    m_dxfColorFieldsPresent = 0;
    m_dxfMarginsRead = 0;
    m_dxfDeclaredBorderCount = 0;
    m_dxfBorderSlotsRead = 0;
    m_dxfSawContentMarker = false;
    m_dxfSawMarginMarker = false;
    m_dxfSawGridMarker = false;
    m_dxfSawTableEnd = false;
    m_dxfZeroGridEndExpected = false;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  [[nodiscard]] bool validateDwgPayload(DRW::Version version) const;
  [[nodiscard]] bool finalizeDxf() const noexcept;
  std::vector<DRW_TableStyleCellStyle> m_cellStyles;
  std::vector<DRW_DwgSubrecordRange> m_subrecordRanges;

private:
  enum class DxfSection : std::uint8_t {
    None,
    Table,
    Content,
    Margin,
    Grid,
    Identity
  };
  enum class DxfColorTarget : std::uint8_t {
    None,
    Background,
    Content,
    Border
  };
  DxfSection m_dxfSection{DxfSection::None};
  DxfColorTarget m_dxfColorTarget{DxfColorTarget::None};
  bool m_dxfSawSubclass{false};
  bool m_dxfValid{true};
  bool m_dxfSawCount{false};
  std::size_t m_dxfDeclaredCount{0};
  std::size_t m_dxfCompletedCount{0};
  std::set<int> m_dxfCellStyleIds;
  int m_dxfCurrentStyle{-1};
  int m_dxfCurrentBorder{-1};
  std::uint16_t m_dxfTableFieldsPresent{0};
  std::uint16_t m_dxfContentFieldsPresent{0};
  std::uint16_t m_dxfGridFieldsPresent{0};
  std::uint8_t m_dxfIdentityFieldsPresent{0};
  std::uint8_t m_dxfColorFieldsPresent{0};
  std::size_t m_dxfMarginsRead{0};
  std::size_t m_dxfDeclaredBorderCount{0};
  std::size_t m_dxfBorderSlotsRead{0};
  bool m_dxfSawContentMarker{false};
  bool m_dxfSawMarginMarker{false};
  bool m_dxfSawGridMarker{false};
  bool m_dxfSawTableEnd{false};
  bool m_dxfZeroGridEndExpected{false};
};

//! Class to handle Layout (ODA spec sec 19.4.85 fixed type 82)
/*!
 *  Minimal carrier for paperspace layouts; full field parsing pending.
 */
class DRW_Layout : public DRW_TableEntry {
  SETOBJFRIENDS
  friend class dxfLayout::Decoder;
public:
  static constexpr std::int32_t kMaxViewportCount = 10000;

  DRW_Layout() { reset(); }
  DRW_Layout(const DRW_Layout& o): DRW_TableEntry(o), m_dxfSubclass(0) {
    copyPersistentFields(o);
    tType = DRW::LAYOUT;
  }
  DRW_Layout& operator=(const DRW_Layout& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      copyPersistentFields(o);
      m_dxfSubclass = 0;
      tType = DRW::LAYOUT;
    }
    return *this;
  }
  void reset() {
    tType = DRW::LAYOUT;
    m_dxfRetainedTextBytes.fill(0u);
    // PlotSettings prefix
    pageSetupName.clear();
    printerConfig.clear();
    plotLayoutFlags = 0;
    marginLeft = marginBottom = marginRight = marginTop = 0.0;
    paperWidth = paperHeight = 0.0;
    paperSize.clear();
    plotOriginX = plotOriginY = 0.0;
    paperUnits = 0;
    plotRotation = 0;
    plotType = 0;
    windowMinX = windowMinY = windowMaxX = windowMaxY = 0.0;
    plotViewName.clear();
    realWorldUnits = 1.0;
    drawingUnits = 1.0;
    currentStyleSheet.clear();
    scaleType = 0;
    scaleFactor = 1.0;
    paperImageOriginX = paperImageOriginY = 0.0;
    shadePlotMode = 0;
    shadePlotResLevel = 0;
    shadePlotCustomDPI = 0;
    // Layout-specific
    name.clear();
    tabOrder = 0;
    layoutFlags = 0;
    ucsOrigin = DRW_Coord();
    limMinX = limMinY = limMaxX = limMaxY = 0.0;
    insPoint = DRW_Coord();
    ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
    ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
    elevation = 0.0;
    orthoViewType = 0;
    extMin = DRW_Coord();
    extMax = DRW_Coord();
    viewportCount = 0;
    // Handle refs
    plotViewHandle = dwgHandle{};
    shadePlotHandle = dwgHandle{};
    paperSpaceBlockRecordHandle = dwgHandle{};
    lastActiveViewportHandle = dwgHandle{};
    baseUcsHandle = dwgHandle{};
    namedUcsHandle = dwgHandle{};
    viewportHandles.clear();
    m_dxfSubclass = 0;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;
  //! Transient DXF-parse state: which 100-subclass we are in
  //! (0 = AcDbPlotSettings prefix, 1 = AcDbLayout). Disambiguates the
  //! codes 1/70/76/330 that appear in both subclasses.
  int m_dxfSubclass = 0;
  // Retained logical bytes for the five embedded PlotSettings strings and
  // the subclass-scoped Layout name; public mutation and DWG are outside this
  // DXF parse-accounting scope.
  std::array<std::size_t, 6> m_dxfRetainedTextBytes{};

private:
  void copyPersistentFields(const DRW_Layout& o) {
    m_dxfRetainedTextBytes = o.m_dxfRetainedTextBytes;
    pageSetupName = o.pageSetupName;
    printerConfig = o.printerConfig;
    plotLayoutFlags = o.plotLayoutFlags;
    marginLeft = o.marginLeft;
    marginBottom = o.marginBottom;
    marginRight = o.marginRight;
    marginTop = o.marginTop;
    paperWidth = o.paperWidth;
    paperHeight = o.paperHeight;
    paperSize = o.paperSize;
    plotOriginX = o.plotOriginX;
    plotOriginY = o.plotOriginY;
    paperUnits = o.paperUnits;
    plotRotation = o.plotRotation;
    plotType = o.plotType;
    windowMinX = o.windowMinX;
    windowMinY = o.windowMinY;
    windowMaxX = o.windowMaxX;
    windowMaxY = o.windowMaxY;
    plotViewName = o.plotViewName;
    realWorldUnits = o.realWorldUnits;
    drawingUnits = o.drawingUnits;
    currentStyleSheet = o.currentStyleSheet;
    scaleType = o.scaleType;
    scaleFactor = o.scaleFactor;
    paperImageOriginX = o.paperImageOriginX;
    paperImageOriginY = o.paperImageOriginY;
    shadePlotMode = o.shadePlotMode;
    shadePlotResLevel = o.shadePlotResLevel;
    shadePlotCustomDPI = o.shadePlotCustomDPI;
    layoutFlags = o.layoutFlags;
    tabOrder = o.tabOrder;
    ucsOrigin = o.ucsOrigin;
    limMinX = o.limMinX;
    limMinY = o.limMinY;
    limMaxX = o.limMaxX;
    limMaxY = o.limMaxY;
    insPoint = o.insPoint;
    ucsXAxis = o.ucsXAxis;
    ucsYAxis = o.ucsYAxis;
    elevation = o.elevation;
    orthoViewType = o.orthoViewType;
    extMin = o.extMin;
    extMax = o.extMax;
    viewportCount = o.viewportCount;
    plotViewHandle = o.plotViewHandle;
    shadePlotHandle = o.shadePlotHandle;
    paperSpaceBlockRecordHandle = o.paperSpaceBlockRecordHandle;
    lastActiveViewportHandle = o.lastActiveViewportHandle;
    baseUcsHandle = o.baseUcsHandle;
    namedUcsHandle = o.namedUcsHandle;
    viewportHandles = o.viewportHandles;
  }

public:
  // PlotSettings prefix per ODA §20.4.84 (the LAYOUT object embeds these
  // inline)
  UTF8STRING pageSetupName; /*!< code 1, TV */
  UTF8STRING printerConfig; /*!< code 2, TV */
  int plotLayoutFlags;      /*!< code 70, BS */
  double marginLeft;        /*!< code 40, BD, millimeters */
  double marginBottom;      /*!< code 41, BD */
  double marginRight;       /*!< code 42, BD */
  double marginTop;         /*!< code 43, BD */
  double paperWidth;        /*!< code 44, BD */
  double paperHeight;       /*!< code 45, BD */
  UTF8STRING paperSize;     /*!< code 4, TV */
  double plotOriginX;       /*!< code 46, 2BD */
  double plotOriginY;       /*!< code 47, 2BD */
  int paperUnits;           /*!< code 72, BS */
  int plotRotation;         /*!< code 73, BS */
  int plotType;             /*!< code 74, BS */
  double windowMinX;        /*!< code 48, 2BD */
  double windowMinY;        /*!< code 49, 2BD */
  double windowMaxX;        /*!< code 140, 2BD */
  double windowMaxY;        /*!< code 141, 2BD */
  UTF8STRING plotViewName;  /*!< code 6, T (R13-R2000 only) */
  double realWorldUnits;    /*!< code 142, BD — numerator of print scale */
  double drawingUnits;      /*!< code 143, BD — denominator of print scale */
  UTF8STRING currentStyleSheet; /*!< code 7, TV */
  int scaleType;                /*!< code 75, BS — standard scale type */
  double scaleFactor;           /*!< code 147, BD */
  double paperImageOriginX;     /*!< code 148, 2BD */
  double paperImageOriginY;     /*!< code 149, 2BD */
  int shadePlotMode;            /*!< code 76, BS (R2004+) */
  int shadePlotResLevel;        /*!< code 77, BS (R2004+) */
  int shadePlotCustomDPI;       /*!< code 78, BS (R2004+) */
  // Layout-specific fields
  int layoutFlags;     /*!< code 70, BS */
  int tabOrder;        /*!< code 71, BL */
  DRW_Coord ucsOrigin; /*!< code 13, 3BD */
  double limMinX;      /*!< code 10, 2RD */
  double limMinY;
  double limMaxX; /*!< code 11, 2RD */
  double limMaxY;
  DRW_Coord insPoint; /*!< code 12, 3BD */
  DRW_Coord ucsXAxis; /*!< code 16, 3BD */
  DRW_Coord ucsYAxis; /*!< code 17, 3BD */
  double elevation;   /*!< code 146, BD */
  int orthoViewType;  /*!< code 76, BS — orthographic view type of UCS */
  DRW_Coord extMin;   /*!< code 14, 3BD — layout extent min */
  DRW_Coord extMax;   /*!< code 15, 3BD — layout extent max */
  std::int32_t viewportCount; /*!< BL — viewport count (R2004+) */
  // Handle refs (the common parentHandle is on DRW_TableEntry)
  dwgHandle plotViewHandle;                   /*!< code 6 (R2004+) */
  dwgHandle shadePlotHandle;                  /*!< code 333 (R2007+) */
  dwgHandle paperSpaceBlockRecordHandle;      /*!< code 330 */
  dwgHandle lastActiveViewportHandle;         /*!< code 331 */
  dwgHandle baseUcsHandle;                    /*!< code 346 */
  dwgHandle namedUcsHandle;                   /*!< code 345 */
  std::vector<std::uint32_t> viewportHandles; /*!< viewport handles (R2004+) */
};

//! One parallel line element within a MLineStyle (ODA §19.4.73).
struct DRW_MLineElement {
  double offset = 0.0; /*!< BD — perpendicular offset from centerline */
  int color = 256;     /*!< CMC index — 256 = ByLayer */
  int color24 = -1;    /*!< true-color RGB (-1 = none) */
  std::uint8_t colorMethod = 0; /*!< CMC method byte; 0 means infer for authored data */
  std::uint8_t colorFlags = 0; /*!< raw CMC name/book flag byte */
  UTF8STRING colorName; /*!< CMC color name (DWG only) */
  UTF8STRING colorBookName; /*!< CMC color-book name (DWG only) */
  int linetypeIndex =
      -1; /*!< BSd inline lt index PRE-R2018 (DXF 6); -1 = unset.
           *   R2018+ uses linetypeHandle instead (0B.4b). */
  std::uint32_t linetypeHandle = 0; /*!< H — linetype object reference */
  UTF8STRING linetype;              /*!< resolved linetype name (DXF 6) */
  //! Raw R2004+ CMC BS index. Nonzero source values are retained but not writable.
  std::uint16_t colorDwgIndex = 0u;

private:
  friend class DRW_MLineStyle;
  std::size_t m_dxfRetainedLinetypeBytes{0u};
};

//! Class to handle MLineStyle (ODA spec sec 19.4.73 fixed type 73)
/*!
 *  Carrier for multiline styles. Per-line `elements` define each parallel
 *  line's offset, color and linetype. The MLINE entity references this
 *  style by handle; entries are populated in addMLineStyle on import.
 */
class DRW_MLineStyle : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_MLineStyle() { reset(); }
  void reset() {
    tType = DRW::MLINESTYLE;
    flags = 0;
    startAngle = endAngle = 0.0;
    fillColor = 256;
    fillColor24 = -1;
    fillColorMethod = 0;
    fillColorFlags = 0;
    fillColorDwgIndex = 0u;
    fillColorName.clear();
    fillColorBookName.clear();
    name.clear();
    description.clear();
    m_dxfRetainedDescriptionBytes = 0u;
    elements.clear();
    dxfDeclaredElementCount = -1;
    dxfElementCountSeen = false;
    dxfStyleFieldsSeen = 0u;
    dxfElementFieldMasks.clear();
    DRW_TableEntry::reset();
  }
  [[nodiscard]] bool validateDxfRecord() const noexcept;

protected:
  // DXF cap-angle defaults differ from the historical API/DWG reset contract.
  // No fields or virtuals are added; each DXF record starts independently.
  void resetDxf() {
    reset();
    startAngle = endAngle = 90.0;
  }
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  int flags;              /*!< style flags (BS) */
  double startAngle;      /*!< start angle (BD) */
  double endAngle;        /*!< end angle (BD) */
  UTF8STRING description; /*!< description (TV / DXF 3) */
  int fillColor = 256;    /*!< CMC fill color / DXF 62 (256 = ByLayer) */
  int fillColor24 = -1;   /*!< DWG CMC RGB (-1 = absent; not mapped by DXF MLINESTYLE) */
  std::uint8_t fillColorMethod = 0; /*!< DWG CMC method byte; 0 means infer */
  std::uint8_t fillColorFlags = 0; /*!< raw DWG CMC name/book flag byte */
  UTF8STRING fillColorName; /*!< DWG CMC color name */
  UTF8STRING fillColorBookName; /*!< DWG CMC color-book name */
  std::vector<DRW_MLineElement> elements;
  //! Raw R2004+ CMC BS index. Nonzero source values are retained but not writable.
  std::uint16_t fillColorDwgIndex = 0u;

private:
  int dxfDeclaredElementCount = -1;
  bool dxfElementCountSeen = false;
  // Bit mask for singleton DXF fields: flags, description, fill color,
  // start angle and end angle. Element tuples have their own per-item masks.
  std::uint8_t dxfStyleFieldsSeen = 0u;
  std::vector<std::uint8_t> dxfElementFieldMasks;
  // Logical bytes retained by DXF-parsed description text only.
  std::size_t m_dxfRetainedDescriptionBytes {0u};
};

//! Class to handle MLEADERSTYLE (ODA spec §20.4.87, AcDbMLeaderStyle).
/*!
 *  MLEADER style dictionary entry.  Lives under the root ACAD_MLEADERSTYLE
 *  dictionary; each MLEADER entity carries a style handle that resolves to
 *  one of these.  Override flags on the MLEADER (BL 90) shadow individual
 *  fields here.
 */
class DRW_MLeaderStyle : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 504;
  //! Validate a represented MLEADERSTYLE DXF integer field.
  [[nodiscard]] static bool isValidDxfIntegerValue(
      int code, std::int32_t value) noexcept;

  DRW_MLeaderStyle() { reset(); }
  void reset() {
    DRW_TableEntry::reset();
    tType = DRW::MLEADERSTYLE;
    styleVersion = 2;
    contentType = 2;
    drawMLeaderOrder = 0;
    drawLeaderOrder = 0;
    maxLeaderPoints = 0;
    firstSegmentAngle = 0.0;
    secondSegmentAngle = 0.0;
    leaderType = 1;
    leaderColor = 0;
    leaderColorRgb = -1;
    leaderColorName.clear();
    leaderColorBookName.clear();
    leaderColorDwgMethod = 0;
    leaderColorDwgFlags = 0;
    leaderColorDwgIndex = 0;
    leaderColorDwgMetadataPresent = false;
    leaderLineTypeHandle = {};
    leaderLineWeight = 0;
    landingEnabled = true;
    landingGap = 0.0;
    autoIncludeLanding = true;
    landingDistance = 0.0;
    description.clear();
    arrowHeadBlockHandle = {};
    arrowHeadSize = 0.0;
    textDefault.clear();
    textStyleHandle = {};
    leftAttachment = 0;
    rightAttachment = 0;
    textAngleType = 0;
    textAlignmentType = 0;
    textColor = 0;
    textColorRgb = -1;
    textColorName.clear();
    textColorBookName.clear();
    textColorDwgMethod = 0;
    textColorDwgFlags = 0;
    textColorDwgIndex = 0;
    textColorDwgMetadataPresent = false;
    textHeight = 0.0;
    textFrameEnabled = false;
    alwaysAlignTextLeft = false;
    alignSpace = 0.0;
    blockHandle = {};
    blockColor = 0;
    blockColorRgb = -1;
    blockColorName.clear();
    blockColorBookName.clear();
    blockColorDwgMethod = 0;
    blockColorDwgFlags = 0;
    blockColorDwgIndex = 0;
    blockColorDwgMetadataPresent = false;
    blockScale = DRW_Coord(1, 1, 1);
    blockScaleEnabled = false;
    blockRotation = 0.0;
    blockRotationEnabled = false;
    blockConnectionType = 0;
    scaleFactor = 1.0;
    propertyChanged = false;
    isAnnotative = false;
    breakSize = 0.0;
    attachmentDirection = 0;
    topAttachment = 0;
    bottomAttachment = 0;
    textExtended = false;
    m_dxfRetainedTextBytes.fill(0u);
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  [[nodiscard]] bool validateDwgPayload(DRW::Version version) const;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

  /* Per §20.4.87.  Names follow the spec column for traceability. */
  std::uint16_t styleVersion = 2; /*!< code 179, R2010+ */
  std::uint16_t contentType =
      2; /*!< code 170: 0=None,1=Block,2=MText,3=Tolerance */
  std::uint16_t drawMLeaderOrder =
      0; /*!< code 171: 0=content first,1=leader head first */
  std::uint16_t drawLeaderOrder = 0;     /*!< code 172 */
  std::int32_t maxLeaderPoints = 0;      /*!< code 90 */
  double firstSegmentAngle = 0.0;        /*!< code 40 (radians) */
  double secondSegmentAngle = 0.0;       /*!< code 41 (radians) */
  std::uint16_t leaderType = 1;          /*!< code 173 */
  int leaderColor = 0;                   /*!< code 91 (CMC) */
  dwgHandle leaderLineTypeHandle{};      /*!< code 340 (handle stream) */
  std::int32_t leaderLineWeight = 0;     /*!< code 92 */
  bool landingEnabled = true;            /*!< code 290 */
  double landingGap = 0.0;               /*!< code 42 */
  bool autoIncludeLanding = true;        /*!< code 291 */
  double landingDistance = 0.0;          /*!< code 43 */
  UTF8STRING description;                /*!< code 3 */
  dwgHandle arrowHeadBlockHandle{};      /*!< code 341 */
  double arrowHeadSize = 0.0;            /*!< code 44 */
  UTF8STRING textDefault;                /*!< code 300 */
  dwgHandle textStyleHandle{};           /*!< code 342 */
  std::uint16_t leftAttachment = 0;      /*!< code 174 */
  std::uint16_t rightAttachment = 0;     /*!< code 178 */
  std::uint16_t textAngleType = 0;       /*!< code 175 (R2010+) */
  std::uint16_t textAlignmentType = 0;   /*!< code 176 */
  int textColor = 0;                     /*!< code 93 */
  double textHeight = 0.0;               /*!< code 45 */
  bool textFrameEnabled = false;         /*!< code 292 */
  bool alwaysAlignTextLeft = false;      /*!< code 297 */
  double alignSpace = 0.0;               /*!< code 46 */
  dwgHandle blockHandle{};               /*!< code 343 */
  int blockColor = 0;                    /*!< code 94 */
  DRW_Coord blockScale{1, 1, 1};         /*!< code 47/49/140 */
  bool blockScaleEnabled = false;        /*!< code 293 */
  double blockRotation = 0.0;            /*!< code 141 (radians) */
  bool blockRotationEnabled = false;     /*!< code 294 */
  std::uint16_t blockConnectionType = 0; /*!< code 177 */
  double scaleFactor = 1.0;              /*!< code 142 */
  bool propertyChanged = false;          /*!< code 295 */
  bool isAnnotative = false;             /*!< code 296 */
  double breakSize = 0.0;                /*!< code 143 */
  /* R2010+ */
  std::uint16_t attachmentDirection = 0; /*!< code 271 */
  std::uint16_t topAttachment = 0;       /*!< code 273 */
  std::uint16_t bottomAttachment = 0;    /*!< code 272 */
  bool textExtended = false;             /*!< code 298, R2013+ */

private:
  // Logical bytes retained by DXF-parsed description/default text only.
  std::array<std::size_t, 2> m_dxfRetainedTextBytes{};

public:
  // R2004+ DWG truecolor companions for the three CMC-valued style fields.
  // Appended after existing state to preserve all earlier member offsets.
  std::int32_t leaderColorRgb = -1;
  std::int32_t textColorRgb = -1;
  std::int32_t blockColorRgb = -1;
  UTF8STRING leaderColorName;
  UTF8STRING leaderColorBookName;
  UTF8STRING textColorName;
  UTF8STRING textColorBookName;
  UTF8STRING blockColorName;
  UTF8STRING blockColorBookName;
  // Raw DWG CMC method bytes. Zero means infer for authored/DXF-origin data.
  std::uint8_t leaderColorDwgMethod = 0;
  std::uint8_t textColorDwgMethod = 0;
  std::uint8_t blockColorDwgMethod = 0;
  std::uint8_t leaderColorDwgFlags = 0;
  std::uint16_t leaderColorDwgIndex = 0;
  bool leaderColorDwgMetadataPresent = false;
  std::uint8_t textColorDwgFlags = 0;
  std::uint16_t textColorDwgIndex = 0;
  bool textColorDwgMetadataPresent = false;
  std::uint8_t blockColorDwgFlags = 0;
  std::uint16_t blockColorDwgIndex = 0;
  bool blockColorDwgMetadataPresent = false;
};

//! Class to handle DBCOLOR (AcDbColor), a custom OBJECT class (§20.4).
/*!
 *  Named/true-color reference target. R2004+ entities with the ENC flag bit
 *  0x40 set carry a hard-pointer handle to one of these instead of an inline
 *  RGB value. Holds the resolved RGB plus the optional book/color names.
 *
 *  Layout (libreDWG dwg2.spec:2404-2408):
 *    - Common entity preamble
 *    - FIELD_CMC color  : §2.7 CMC = BS index, BL rgb (high byte = method),
 *                          RC method-byte, optional TV color name (bit 1),
 *                          optional TV book name (bit 2)
 *    - START_OBJECT_HANDLE_STREAM  (parent dictionary, reactors, xdic)
 */
class DRW_DbColor : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  // 1004 is libdxfrw's semantic dispatch token.  On disk DBCOLOR is an
  // unstable/custom class and uses a file-local CLASSES ordinal instead.
  static constexpr std::uint16_t kDwgType = 1004;
  static constexpr std::uint16_t kDwgClassNum = 563;

  DRW_DbColor() { reset(); }
  void reset() {
    DRW_TableEntry::reset();
    tType = DRW::DBCOLOR;
    rgb = -1;
    colorIndex = 0;
    colorMethod = 0;
    colorDwgFlags = 0u;
    colorDwgIndex = 0u;
    colorDwgMetadataPresent = false;
    name.clear();
    bookName.clear();
    m_dxfRetainedBookNameBytes = 0u;
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  int rgb = -1; /*!< 24-bit RGB value, code 420 (or -1 if not RGB type) */
  std::uint16_t colorIndex = 0; /*!< ACI fallback when method=0xC3, code 62 */
  std::uint8_t colorMethod =
      0;               /*!< 0xC0 ByLayer / 0xC1 ByBlock / 0xC2 RGB / 0xC3 ACI */
  UTF8STRING bookName; /*!< color book name, code 430 prefix */
  // name (inherited from DRW_TableEntry) is the entry name within the book
  // Appended R2004+ source-wire provenance for the DBCOLOR CMC.
  std::uint8_t colorDwgFlags = 0u;
  std::uint16_t colorDwgIndex = 0u;
  bool colorDwgMetadataPresent = false;

private:
  std::size_t m_dxfRetainedBookNameBytes{0u};
};

//! Class to handle SCALE (AcDbScale) — annotation-scale entry, custom-class
//! object.
/*!
 *  Lives in the OBJECTS section under the named-object-dictionary
 *  ACAD_SCALELIST. Each entry maps a scale name like "1:48" to a
 *  paper-units / drawing-units ratio that consumers (MTEXT, MLEADER,
 *  DIMENSION, ATTRIB) reference via their AcDbAnnotScaleObjectContextData
 *  per-scale ctx data.  ODA spec §20.4.93; libreDWG dwg2.spec:1195-1203
 *  (DWG_OBJECT (SCALE)).
 *
 *  Body fields after the AcDbScale subclass marker:
 *    BS  flag           always 0
 *    TV  name           e.g., "1:48"
 *    BD  paperUnits     numerator (paper space)
 *    BD  drawingUnits   denominator (drawing/model space)
 *    B   isUnitScale    true when ratio is 1:1
 *
 *  Scale factor used at render time = drawingUnits / paperUnits
 *  (e.g., "1:48" → drawingUnits=48, paperUnits=1 → factor=48).
 */
class DRW_Scale : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 508;

  DRW_Scale() { tType = DRW::SCALE; }

  double scaleFactor() const {
    return paperUnits == 0.0 ? 1.0 : drawingUnits / paperUnits;
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr) const;

public:
  std::uint16_t flag = 0;    /*!< always 0, code 70 */
  double paperUnits = 1.0;   /*!< numerator,  code 140 */
  double drawingUnits = 1.0; /*!< denominator, code 141 */
  bool isUnitScale = false;  /*!< true for the 1:1 entry, code 290 */
  // name (inherited from DRW_TableEntry) carries the user-visible label, code
  // 300
};

//! Standalone AcDb*ObjectContextData shell (annotation-scale context metadata).
/*!
 *  These OBJECTS records carry per-annotation-scale overrides for visible
 *  entities (TEXT/MTEXT/DIMENSION/etc.). LibreCAD has no annotation-scale
 *  renderer yet, so this parser deliberately captures metadata only and leaves
 *  raw bytes replayable through DRW_UnsupportedObject. The stable common
 *  scaffold is AcDbObjectContextData:
 *    BS classVersion, then for R2010+ B hasFileToExtensionDictionary,
 *    B defaultFlag, handle stream ... scaleHandle.
 *
 *  Text, dimension, leader, block-reference, and FCF families are decoded far
 *  enough to expose their stable placement/scale links. MLeader contexts keep
 *  their class/default shell typed while their version-specific body remains
 *  raw-preserved.
 */
class DRW_ObjectContextData : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  enum class Kind {
    Unknown,
    AnnotScale,
    Text,
    MText,
    MTextAttribute,
    OrdinateDimension,
    AlignedDimension,
    AngularDimension,
    RadialDimension,
    LargeRadialDimension,
    DiameterDimension,
    Leader,
    BlockReference,
    Fcf,
    MLeader
  };

  explicit DRW_ObjectContextData(const UTF8STRING &recordName = UTF8STRING(),
                                 Kind kind = Kind::Unknown)
      : m_recordName(recordName), m_kind(kind) {
    reset();
  }

  bool isTextFamily() const {
    return m_kind == Kind::Text || m_kind == Kind::MText ||
           m_kind == Kind::MTextAttribute;
  }
  bool isDimensionFamily() const {
    return m_kind == Kind::OrdinateDimension ||
           m_kind == Kind::AlignedDimension ||
           m_kind == Kind::AngularDimension ||
           m_kind == Kind::RadialDimension ||
           m_kind == Kind::LargeRadialDimension ||
           m_kind == Kind::DiameterDimension;
  }

protected:
  void reset();
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  UTF8STRING m_recordName;
  Kind m_kind = Kind::Unknown;
  std::uint16_t m_classVersion = 0;
  bool m_hasFileToExtensionDictionary = true;
  bool m_defaultFlag = false;
  std::uint32_t m_scaleHandle = 0;
  std::uint32_t m_blockHandle = 0; /*!< dimension contexts only */
  std::uint32_t m_annotatedHandle =
      0; /*!< common owner/annotated object handle */

  // Text/MTEXT placement subset.
  std::uint16_t m_horizontalMode = 0;
  std::int32_t m_attachment = 0;
  double m_rotation = 0.0;
  DRW_Coord m_insertionPoint;
  DRW_Coord m_alignmentPoint;
  DRW_Coord m_xAxisDir{1.0, 0.0, 0.0};
  double m_rectangleWidth = 0.0;
  double m_rectangleHeight = 0.0;
  double m_extentsWidth = 0.0;
  double m_extentsHeight = 0.0;

  // MText object-context column data.  The DWG body carries the declared
  // count even when the context uses automatic height; keep the bounded
  // decoded count and only materialize explicit column heights.
  std::int32_t m_columnType = 0;
  std::int32_t m_columnHeightCount = 0;
  std::int32_t m_fragmentCount = 0;
  double m_columnWidth = 0.0;
  double m_columnGutter = 0.0;
  bool m_columnAutoHeight = false;
  bool m_columnFlowReversed = false;
  std::vector<double> m_columnHeights;

  // Embedded annotation scale carried by MTEXTATTRIBUTE contexts.
  bool m_embeddedScaleEnabled = false;
  std::uint16_t m_embeddedScaleFlag = 0;
  UTF8STRING m_embeddedScaleName;
  double m_embeddedScalePaperUnits = 1.0;
  double m_embeddedScaleDrawingUnits = 1.0;
  bool m_embeddedScaleUnitScale = false;

  // Dimension-family common context subset.
  DRW_Coord m_definitionPoint;
  bool m_isDefaultTextLocation = false;
  double m_textRotation = 0.0;
  bool m_unknown293 = false;
  bool m_dimTofl = false;
  bool m_dimOsxd = false;
  bool m_dimAtFit = false;
  bool m_dimTix = false;
  bool m_dimTMove = false;
  std::uint8_t m_overrideCode = 0;
  bool m_hasArrow2 = false;
  bool m_flipArrow2 = false;
  bool m_flipArrow1 = false;
  DRW_Coord m_featureLocationPoint; /*!< ordinate dimensions only */
  DRW_Coord m_leaderEndpoint;       /*!< ordinate dimensions only */

  // Leader context subset (LEADEROBJECTCONTEXTDATA).
  std::vector<DRW_Coord> m_leaderPoints;
  bool m_leaderUnknown290 = false;
  DRW_Coord m_leaderXDir{1.0, 0.0, 0.0};
  DRW_Coord m_leaderInsertionOffset;
  DRW_Coord m_leaderEndpointProjection;

  // Block-reference context subset (BLKREFOBJECTCONTEXTDATA).
  double m_blkRefRotation = 0.0;
  DRW_Coord m_blkRefInsertionPoint;
  DRW_Coord m_blkRefScale{1.0, 1.0, 1.0};

  // FCF context subset (FCFOBJECTCONTEXTDATA).
  DRW_Coord m_fcfLocation;
  DRW_Coord m_fcfHorizontalDirection{1.0, 0.0, 0.0};
};

struct DRW_DimensionAssociationOsnapRef {
  UTF8STRING m_className;
  std::uint8_t m_objectOsnapType = 0;
  bool m_hasObjectOsnapType = false;
  std::uint32_t m_objectHandle = 0;
  bool m_hasSubentType = false;
  std::int32_t m_subentType = 0;
  bool m_hasGsMarker = false;
  std::int32_t m_gsMarker = 0;
  bool m_hasNearParameter = false;
  double m_nearParameter = 0.0;
  bool m_hasOsnapPoint = false;
  DRW_Coord m_osnapPoint;
  bool m_hasLastPointRef = false;
  std::int32_t m_lastPointRef = 0;
  bool m_hasIntersectionObjectHandle = false;
  std::uint32_t m_intersectionObjectHandle = 0;
  bool m_hasIntersectionSubentType = false;
  std::int32_t m_intersectionSubentType = 0;
  bool m_hasIntersectionGsMarker = false;
  std::int32_t m_intersectionGsMarker = 0;
  // DXF groups 301/302 carry serialized XREF-database handle text. Keep the
  // exact value opaque: these are not host-drawing handles and must not pass
  // through the local handle resolver/remapper.
  bool m_hasXrefObjectHandleString = false;
  std::string m_xrefObjectHandleString;
  std::size_t m_dxfRetainedXrefObjectHandleStringBytes{0u};
  bool m_hasIntersectionXrefObjectHandleString = false;
  std::string m_intersectionXrefObjectHandleString;
  std::size_t m_dxfRetainedIntersectionXrefObjectHandleStringBytes{0u};
  std::size_t m_dxfRetainedClassNameBytes{0u};
  // Parser-only component presence catches incomplete/duplicate WCS tuples.
  // A complete point is represented by m_hasOsnapPoint and m_osnapPoint.
  std::uint8_t m_dxfPointComponentMask = 0;
};

//! Shell parser for DIMASSOC (AcDbDimAssoc) associative dimension metadata.
class DRW_DimensionAssociation : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 526;

  DRW_DimensionAssociation() { reset(); }
  DRW_DimensionAssociation(const DRW_DimensionAssociation& o):
      DRW_TableEntry(o), m_dimensionHandle(o.m_dimensionHandle),
      m_associativityFlags(o.m_associativityFlags),
      m_isTransSpace(o.m_isTransSpace),
      m_rotatedDimensionType(o.m_rotatedDimensionType),
      m_osnapRefs(o.m_osnapRefs),
      m_hasUnrepresentableDetail(o.m_hasUnrepresentableDetail),
      m_dxfInBody(false) {
    tType = o.tType;
  }
  DRW_DimensionAssociation& operator=(const DRW_DimensionAssociation& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_dimensionHandle = o.m_dimensionHandle;
      m_associativityFlags = o.m_associativityFlags;
      m_isTransSpace = o.m_isTransSpace;
      m_rotatedDimensionType = o.m_rotatedDimensionType;
      m_osnapRefs = o.m_osnapRefs;
      m_hasUnrepresentableDetail = o.m_hasUnrepresentableDetail;
      m_dxfInBody = false;
      tType = o.tType;
    }
    return *this;
  }

  void reset() {
    DRW_TableEntry::reset();
    tType = DRW::DIMASSOC;
    m_dimensionHandle = 0;
    m_associativityFlags = 0;
    m_isTransSpace = false;
    m_rotatedDimensionType = 0;
    m_osnapRefs.clear();
    m_hasUnrepresentableDetail = false;
    m_dxfInBody = false;
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  std::uint32_t m_dimensionHandle = 0;
  std::uint32_t m_associativityFlags = 0;
  bool m_isTransSpace = false;
  std::uint8_t m_rotatedDimensionType = 0;
  std::vector<DRW_DimensionAssociationOsnapRef> m_osnapRefs;
  // The public metadata model deliberately retains one object handle per
  // osnap reference. Set when an input carries details that the typed model
  // cannot emit without loss; the validated raw frame remains the replay
  // source for those fields.
  bool m_hasUnrepresentableDetail = false;

private:
  /* DXF: true once past "100 AcDbDimAssoc" — body code 330 is the dimension
     handle, not the object owner. */
  bool m_dxfInBody = false;
};

struct DRW_EvaluationGraphNode {
  std::int32_t m_index = 0;
  std::int32_t m_flags = 0;
  std::int32_t m_nextNodeIndex = 0;
  std::uint32_t m_expressionHandle = 0;
  std::int32_t m_data1 = 0;
  std::int32_t m_data2 = 0;
  std::int32_t m_data3 = 0;
  std::int32_t m_data4 = 0;
};

struct DRW_EvaluationGraphEdge {
  std::int32_t m_value92 = 0;
  std::int32_t m_value93 = 0;
  std::int32_t m_value94 = 0;
  std::int32_t m_value91a = 0;
  std::int32_t m_value91b = 0;
  std::int32_t m_value92a = 0;
  std::int32_t m_value92b = 0;
  std::int32_t m_value92c = 0;
  std::int32_t m_value92d = 0;
  std::int32_t m_value92e = 0;
};

//! Shell parser for ACAD_EVALUATION_GRAPH dynamic/associative metadata.
class DRW_EvaluationGraph : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 516;
  static constexpr std::size_t kMaxEntries = 100000;

  DRW_EvaluationGraph() { tType = DRW::EVALUATIONGRAPH; }
  DRW_EvaluationGraph(const DRW_EvaluationGraph& o): DRW_TableEntry(o),
      m_value96(o.m_value96), m_value97(o.m_value97), m_nodes(o.m_nodes),
      m_edges(o.m_edges), m_dxfInBody(false), m_dxfEdges(false),
      m_dxfMalformed(false), m_dxfHasValue96(false), m_dxfHasValue97(false),
      m_dxfNodeHasFlags(false), m_dxfNodeHasNextNodeIndex(false),
      m_dxfNodeHasExpressionHandle(false), m_dxfEdgeHasValue93(false),
      m_dxfEdgeHasValue94(false), m_dxfNodeDataCount(0),
      m_dxfEdgeDataCount(0), m_dxfEdge91Count(0) {
    tType = o.tType;
  }
  DRW_EvaluationGraph& operator=(const DRW_EvaluationGraph& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_value96 = o.m_value96;
      m_value97 = o.m_value97;
      m_nodes = o.m_nodes;
      m_edges = o.m_edges;
      m_dxfInBody = false;
      m_dxfEdges = false;
      m_dxfMalformed = false;
      m_dxfHasValue96 = false;
      m_dxfHasValue97 = false;
      m_dxfNodeHasFlags = false;
      m_dxfNodeHasNextNodeIndex = false;
      m_dxfNodeHasExpressionHandle = false;
      m_dxfEdgeHasValue93 = false;
      m_dxfEdgeHasValue94 = false;
      m_dxfNodeDataCount = 0;
      m_dxfEdgeDataCount = 0;
      m_dxfEdge91Count = 0;
      tType = o.tType;
    }
    return *this;
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  std::int32_t m_value96 = 0;
  std::int32_t m_value97 = 0;
  std::vector<DRW_EvaluationGraphNode> m_nodes;
  std::vector<DRW_EvaluationGraphEdge> m_edges;

  // DXF uses repeated 91/92/93/94/95 codes rather than explicit node/edge
  // counts. These fields keep the small state machine transactional until
  // processObjects reaches the next record.
  bool finalizeDxf();

private:
  bool m_dxfInBody = false;
  bool m_dxfEdges = false;
  bool m_dxfMalformed = false;
  bool m_dxfHasValue96 = false;
  bool m_dxfHasValue97 = false;
  bool m_dxfNodeHasFlags = false;
  bool m_dxfNodeHasNextNodeIndex = false;
  bool m_dxfNodeHasExpressionHandle = false;
  bool m_dxfEdgeHasValue93 = false;
  bool m_dxfEdgeHasValue94 = false;
  int m_dxfNodeDataCount = 0;
  int m_dxfEdgeDataCount = 0;
  int m_dxfEdge91Count = 0;
};

//! Fixed BLOCKREPRESENTATIONDATA object (DWG type 1120).
class DRW_BlockRepresentationData : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 1120;

  DRW_BlockRepresentationData() { reset(); }

  void reset() {
    DRW_TableEntry::reset();
    tType = DRW::BLOCKREPRESENTATIONDATA;
    m_flag = 0;
    m_blockHandle = 0;
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  std::uint16_t m_flag = 0;
  std::uint32_t m_blockHandle = 0;
};

//! Minimal ACDBPLACEHOLDER carrier (ODA fixed type 80).
class DRW_AcDbPlaceholder : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_AcDbPlaceholder() { reset(); }
  void reset() {
    tType = DRW::ACDBPLACEHOLDER;
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *handleBuf = nullptr) const;
};

//! VBA project OBJECTS record (fixed DWG type 81).
class DRW_VbaProject : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgType = 81;

  DRW_VbaProject() { reset(); }

  void reset() {
    tType = DRW::VBAPROJECT;
    m_dataSize = 0;
    m_data.clear();
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;
  std::uint32_t m_dataSize = 0;
  std::vector<std::uint8_t> m_data;
};

//! A decoded ACAD_PROXY_OBJECT OBJECTS record (fixed DWG type 499).
struct DRW_ProxyObjectIdRef {
  int m_dxfCode = 330;
  std::uint8_t m_handleCode = 0;
  std::uint64_t m_handle = 0;
  std::uint64_t m_rawHandle = 0;
};

class DRW_ProxyObject : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgType = 499;

  DRW_ProxyObject() { reset(); }

  void reset() {
    tType = DRW::PROXYOBJECT;
    m_hasProxyClassId = false;
    m_proxyClassId = 0;
    m_hasProxyCarrierId = false;
    m_proxyCarrierId = 0;
    m_proxySubclass.clear();
    m_hasProxyDrawingFormat = false;
    m_proxyDrawingFormat = 0;
    m_proxyDwgVersion = 0;
    m_proxyMaintenanceVersion = 0;
    m_hasFromDxf = false;
    m_fromDxf = false;
    m_hasProxyGraphicsByteSize = false;
    m_proxyGraphicsByteSize = 0;
    m_hasObjectDataBitSize = false;
    m_binaryData.clear();
    m_objectData.clear();
    m_objectDataBitSize = 0;
    m_hasUnknownDataByteSize = false;
    m_unknownDataByteSize = 0;
    m_unknownData.clear();
    m_objectIdRefs.clear();
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  // DXF proxy metadata (codes 90/91/92/93/94/95/96/160/161/162/70).
  // The raw DXF object remains the authoritative replay representation.
  bool m_hasProxyClassId = false;
  std::int32_t m_proxyClassId = 0;
  bool m_hasProxyCarrierId = false;
  std::int32_t m_proxyCarrierId = 0;
  UTF8STRING m_proxySubclass;
  bool m_hasProxyDrawingFormat = false;
  std::uint32_t m_proxyDrawingFormat = 0;
  std::uint16_t m_proxyDwgVersion = 0;
  std::uint16_t m_proxyMaintenanceVersion = 0;
  bool m_hasFromDxf = false;
  bool m_fromDxf = false;
  bool m_hasProxyGraphicsByteSize = false;
  std::uint64_t m_proxyGraphicsByteSize = 0;
  bool m_hasObjectDataBitSize = false;
  std::vector<std::uint8_t> m_binaryData;
  std::vector<std::uint8_t> m_objectData;
  std::uint32_t m_objectDataBitSize = 0;
  bool m_hasUnknownDataByteSize = false;
  std::uint64_t m_unknownDataByteSize = 0;
  std::vector<std::uint8_t> m_unknownData;
  std::vector<DRW_ProxyObjectIdRef> m_objectIdRefs;
};

//! SUN object metadata used by R2007+ view/vport lighting.
class DRW_Sun : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 517;

  DRW_Sun() { reset(); }
  void reset() {
    tType = DRW::SUN;
    m_classVersion = 0;
    m_isOn = false;
    m_color = 0;
    m_color24 = -1;
    m_colorCmc = DRW_DwgCmcColorMetadata{};
    m_colorName.clear();
    m_intensity = 0.0;
    m_hasShadow = false;
    m_julianDay = 0;
    m_milliseconds = 0;
    m_isDaylightSavings = false;
    m_shadowType = 0;
    m_shadowMapSize = 0;
    m_shadowSoftness = 0;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *handleBuf = nullptr,
                               dwgBufferW *strBuf = nullptr) const;

  std::uint32_t m_classVersion = 0;
  bool m_isOn = false;
  std::uint32_t m_color = 0; /*!< ACI index, DXF code 63 */
  std::int32_t m_color24 = -1; /*!< Optional 24-bit DWG CMC RGB; -1 = absent */
  double m_intensity = 0.0;
  bool m_hasShadow = false;
  std::int32_t m_julianDay = 0;
  std::int32_t m_milliseconds = 0;
  bool m_isDaylightSavings = false;
  std::uint32_t m_shadowType = 0;
  std::uint16_t m_shadowMapSize = 0;
  std::uint8_t m_shadowSoftness = 0;
  DRW_DwgCmcColorMetadata m_colorCmc;
  UTF8STRING m_colorName; /*!< DWG CMC name/book projection; no SUN DXF group */
};

struct DRW_AssociativeHandleRef {
  bool m_isOwned = false;
  std::uint32_t m_handle = 0;
};

struct DRW_AssociativePrefixStatus {
  enum class Kind {
    AcDbAssocAction,
    AcDbAssocActionParam,
    AcDbAssocDependency,
    AcDbAssocGeomDependency,
    AcDbAssocNetwork,
    AcDbAssocActionBody,
    AcDbEvalExpr,
    AcDbShHistoryNode,
    AcShActionBody,
    AcDbAssocDependencyBody
  };

  enum class ParseStatus {
    Complete,
    Partial,
    Missing,
    UnsupportedVersion,
    BoundedCountOverflow
  };

  Kind m_kind = Kind::AcDbAssocAction;
  std::uint64_t m_startBit = 0;
  std::uint64_t m_bitSize = 0;
  std::uint16_t m_classVersion = 0;
  ParseStatus m_status = ParseStatus::Missing;
  size_t m_decodedHandleCount = 0;
  size_t m_decodedValueCount = 0;
  // A partial prefix may use -1 when its count field is absent/undecoded.
  std::int32_t m_decodedCountValue = 0;
  UTF8STRING m_sourceAssumption;
};

//! Shell parser for ACDBASSOC* action/dependency/action-param objects.
class DRW_AssociativeObject : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  explicit DRW_AssociativeObject(const UTF8STRING &recordName = UTF8STRING())
      : m_recordName(recordName) {
    tType = DRW::ASSOCIATIVEOBJECT;
  }
  DRW_AssociativeObject(const DRW_AssociativeObject& o): DRW_TableEntry(o),
      m_dxfPersistentSubentityBody(false),
      m_dxfPersistentSubentityClassValueCount(0) {
    copyPersistentFields(o);
    tType = DRW::ASSOCIATIVEOBJECT;
  }
  DRW_AssociativeObject& operator=(const DRW_AssociativeObject& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      copyPersistentFields(o);
      m_dxfPersistentSubentityBody = false;
      m_dxfPersistentSubentityClassValueCount = 0;
      tType = DRW::ASSOCIATIVEOBJECT;
    }
    return *this;
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  void reset();
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  UTF8STRING m_recordName;
  std::uint16_t m_classVersion = 0;
  std::uint16_t m_dependencyBodyVersion = 0;
  std::uint16_t m_dimBaseVersion = 0;
  bool m_dimensionDependencyBodyParsed = false;
  std::int32_t m_geometryStatus = 0;
  std::uint32_t m_owningNetworkHandle = 0;
  std::uint32_t m_actionBodyHandle = 0;
  std::int32_t m_actionIndex = 0;
  std::int32_t m_maxDependencyIndex = 0;
  std::vector<DRW_AssociativeHandleRef> m_dependencies;
  std::vector<DRW_AssociativeHandleRef> m_actions;
  std::vector<std::uint32_t> m_ownedParams;
  std::vector<std::uint32_t> m_ownedActions;
  // DXF PERSUBENTMGR stores associated subentities as repeated 330 handles.
  // Keep these separate from the scalar vectors in the fixed DWG type 1313
  // body; the dynamic DWG type 1248 remains an opaque shell.
  std::uint32_t m_persistentSubentityCount = 0;
  std::vector<std::uint32_t> m_persistentSubentityHandles;
  std::uint32_t m_persistentUnknown0 = 0;
  std::uint32_t m_persistentUnknown2 = 0;
  std::uint32_t m_persistentAssocStepCount = 0;
  std::uint32_t m_persistentAssocSubentityCount = 0;
  std::vector<std::uint32_t> m_persistentSteps;
  std::vector<std::uint32_t> m_persistentSubentityIds;
  size_t m_valueParamCount = 0;
  size_t m_ownedParamPrefixCount = 0;
  bool m_valueParamsParsed = false;
  bool m_actionParamPrefixParsed = false;
  bool m_singleDependencyActionParamParsed = false;
  bool m_compoundActionParamParsed = false;
  std::uint32_t m_dependencyHandle = 0;
  std::uint32_t m_readDependencyHandle = 0;
  std::uint32_t m_writeDependencyHandle = 0;
  // ACDBASSOCDEPENDENCY's explicit dep_body handle, kept distinct from its
  // owner and read/write dependency references for graph closure.
  std::uint32_t m_dependencyBodyHandle = 0;
  std::uint32_t m_rNodeHandle = 0;
  std::uint32_t m_dNodeHandle = 0;
  std::int32_t m_status = 0;
  std::uint8_t m_osnapMode = 0;
  double m_parameter = 0.0;
  DRW_Coord m_point;
  std::vector<DRW_AssociativePrefixStatus> m_prefixStatuses;

private:
  void copyPersistentFields(const DRW_AssociativeObject& o) {
    m_recordName = o.m_recordName;
    m_classVersion = o.m_classVersion;
    m_dependencyBodyVersion = o.m_dependencyBodyVersion;
    m_dimBaseVersion = o.m_dimBaseVersion;
    m_dimensionDependencyBodyParsed = o.m_dimensionDependencyBodyParsed;
    m_geometryStatus = o.m_geometryStatus;
    m_owningNetworkHandle = o.m_owningNetworkHandle;
    m_actionBodyHandle = o.m_actionBodyHandle;
    m_actionIndex = o.m_actionIndex;
    m_maxDependencyIndex = o.m_maxDependencyIndex;
    m_dependencies = o.m_dependencies;
    m_actions = o.m_actions;
    m_ownedParams = o.m_ownedParams;
    m_ownedActions = o.m_ownedActions;
    m_persistentSubentityCount = o.m_persistentSubentityCount;
    m_persistentSubentityHandles = o.m_persistentSubentityHandles;
    m_persistentUnknown0 = o.m_persistentUnknown0;
    m_persistentUnknown2 = o.m_persistentUnknown2;
    m_persistentAssocStepCount = o.m_persistentAssocStepCount;
    m_persistentAssocSubentityCount = o.m_persistentAssocSubentityCount;
    m_persistentSteps = o.m_persistentSteps;
    m_persistentSubentityIds = o.m_persistentSubentityIds;
    m_valueParamCount = o.m_valueParamCount;
    m_ownedParamPrefixCount = o.m_ownedParamPrefixCount;
    m_valueParamsParsed = o.m_valueParamsParsed;
    m_actionParamPrefixParsed = o.m_actionParamPrefixParsed;
    m_singleDependencyActionParamParsed = o.m_singleDependencyActionParamParsed;
    m_compoundActionParamParsed = o.m_compoundActionParamParsed;
    m_dependencyHandle = o.m_dependencyHandle;
    m_readDependencyHandle = o.m_readDependencyHandle;
    m_writeDependencyHandle = o.m_writeDependencyHandle;
    m_dependencyBodyHandle = o.m_dependencyBodyHandle;
    m_rNodeHandle = o.m_rNodeHandle;
    m_dNodeHandle = o.m_dNodeHandle;
    m_status = o.m_status;
    m_osnapMode = o.m_osnapMode;
    m_parameter = o.m_parameter;
    m_point = o.m_point;
    m_prefixStatuses = o.m_prefixStatuses;
  }
  bool m_dxfPersistentSubentityBody = false;
  std::uint32_t m_dxfPersistentSubentityClassValueCount = 0;
};

//! Shell parser for ACSH_* history/action objects.
class DRW_AcShHistoryObject : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  explicit DRW_AcShHistoryObject(const UTF8STRING &recordName = UTF8STRING())
      : m_recordName(recordName) {
    tType = DRW::ACSHHISTORYOBJECT;
  }

protected:
  void reset();
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  //! Bounded, typed representation of the AcDbEvalExpr prefix used by the
  //! observed ACSH history classes. A prefix is writable only when complete.
  struct EvalExprPrefix {
    std::int32_t m_unknown = 0;
    std::int32_t m_value98 = 0;
    std::int32_t m_value99 = 0;
    std::int16_t m_valueCode = 0;
    bool m_hasEvaluatedValue = false;
    double m_doubleValue = 0.0;
    DRW_Coord m_pointValue;
    UTF8STRING m_textValue;
    std::int32_t m_longValue = 0;
    std::uint32_t m_handleValue = 0;
    std::int16_t m_shortValue = 0;
    std::int32_t m_id = 0;
    bool m_complete = false;
  };

  //! Bounded, typed representation of the common AcDbShHistoryNode prefix.
  struct HistoryNodePrefix {
    std::int32_t m_major = 0;
    std::int32_t m_minor = 0;
    std::array<double, 16> m_transform{};
    std::uint32_t m_colorIndex = 0;
    std::int32_t m_rgbColor = -1;
    bool m_hasRgbColor = false;
    UTF8STRING m_colorName;
    UTF8STRING m_colorBookName;
    std::int32_t m_nodeValue = 0;
    std::uint32_t m_handle = 0;
    bool m_complete = false;
    //! Raw CMC provenance retained for consumers that only project the
    //! indexed DXF color. A false presence flag means caller-created data.
    std::uint8_t m_colorMethod = 0;
    std::uint8_t m_colorFlags = 0;
    std::uint16_t m_colorIndexRaw = 0;
    bool m_colorIndexIsReserved = false;
    bool m_hasColorCmcMetadata = false;
  };

  UTF8STRING m_recordName;
  EvalExprPrefix m_evalExprPrefix;
  HistoryNodePrefix m_historyNodePrefix;
  std::uint32_t m_major = 0;
  std::uint32_t m_minor = 0;
  std::uint32_t m_ownerHandle = 0;
  std::uint32_t m_historyNodeId = 0;
  bool m_showHistory = false;
  bool m_recordHistory = false;
  DRW_Coord m_direction;
  double m_draftAngle = 0.0;
  double m_startDraftDistance = 0.0;
  double m_endDraftDistance = 0.0;
  double m_scaleFactor = 1.0;
  double m_twistAngle = 0.0;
  double m_alignAngle = 0.0;
  std::vector<std::uint8_t> m_binaryBlob1;
  std::vector<std::uint8_t> m_binaryBlob2;
  //! Decoded primitive dimensions for ACSH_*_CLASS shape objects.
  //! BOX/WEDGE = {length, width, height}; SPHERE = {radius};
  //! CYLINDER/CONE = {height, majorRadius, minorRadius, xRadius};
  //! TORUS = {majorRadius, minorRadius}.
  //! Empty for shape classes that are only prefix-decoded + raw-shelved.
  std::vector<double> m_shapeParams;

  // --- ACSH_BOOLEAN_CLASS body (AcDbShBoolean) ---
  std::int32_t m_operation =
      0; //!< RCd operation (0 union, 1 intersect, 2 subtract)
  std::uint32_t m_operand1 = 0; //!< BL operand1 step id
  std::uint32_t m_operand2 = 0; //!< BL operand2 step id
  // --- ACSH_CHAMFER_CLASS / ACSH_FILLET_CLASS shared body ---
  std::uint32_t m_bl92 = 0;            //!< BL bl92 (chamfer + fillet)
  std::uint32_t m_bl95 = 0;            //!< BL bl95 (chamfer only)
  double m_baseDist = 0.0;             //!< BD base_dist (chamfer)
  double m_otherDist = 0.0;            //!< BD other_dist (chamfer)
  std::vector<std::uint32_t> m_edges;  //!< BL edge indices (chamfer + fillet)
  std::vector<double> m_radiuses;      //!< BD radii (fillet)
  std::vector<double> m_startSetbacks; //!< BD start setbacks (fillet)
  std::vector<double> m_endSetbacks;   //!< BD end setbacks (fillet)
  // --- ACSH_REVOLVE_CLASS body (AcDbShRevolve) ---
  DRW_Coord m_axisPoint;        //!< 3BD axis_pt
  DRW_Coord m_revolveDirection; //!< 2RD direction (x,y; z unused)
  double m_revolveAngle = 0.0;  //!< BD revolve_angle
  double m_startAngle = 0.0;    //!< BD start_angle
  double m_bd44 = 0.0;          //!< BD bd44
  double m_bd45 = 0.0;          //!< BD bd45
  bool m_b290 = false;          //!< B b290
  bool m_isCloseToAxis = false; //!< B is_close_to_axis
  // --- ACSH_LOFT_CLASS body (AcDbShLoft) ---
  std::uint32_t m_numCrossSections = 0; //!< BL num_crosssects
  std::uint32_t m_numGuides = 0;        //!< BL num_guides

  //! Exact DXF projection fields observed in AcDbShSweepBase for
  //! ACSH_EXTRUSION_CLASS. This is a capture-only subset: it does not qualify
  //! the DWG body layout or make the history object writable.
  struct ExtrusionDxfProjection {
    std::array<double, 16> m_sweepEntityTransform{};
    std::array<double, 16> m_pathEntityTransform{};
    std::int32_t m_alignOption = 0;
    std::int32_t m_miterOption = 0;
    bool m_isSolid = false;
    bool m_hasAlignStart = false;
    bool m_bank = false;
    bool m_basePointSet = false;
    bool m_sweepEntityTransformComputed = false;
    bool m_pathEntityTransformComputed = false;
    DRW_Coord m_point;
    bool m_transformAndOptionFieldsComplete = false;
  };
  ExtrusionDxfProjection m_extrusionDxfProjection;

  std::vector<DRW_AssociativePrefixStatus> m_prefixStatuses;
};

//! Shell parser for the dynamic-block object family (BLOCK*PARAMETER /
//! BLOCK*ACTION / BLOCK*GRIP / BLOCKGRIPLOCATIONCOMPONENT / DYNAMICBLOCK*
//! and the singletons).  This is the largest custom-class family; almost none
//! of it has a native LibreCAD consumer, so the goal is dwgTs-style typed
//! CAPTURE: every dynamic-block recName is delivered as a
//! DRW_DynamicBlockObject (instead of raw-only) with the SHARED,
//! oracle-verified prefix decoded — AcDbEvalExpr for every evalexpr-bearing
//! class, plus AcDbBlockElement / AcDbBlockParameter element data for the
//! parameter/grip/action classes.  The validation classes BLOCKPOINTPARAMETER,
//! BLOCKBASEPOINTPARAMETER, BLOCKLOOKUPPARAMETER, BLOCKFLIPPARAMETER,
//! BLOCKROTATIONPARAMETER, BLOCKLINEARPARAMETER, BLOCKPOLARPARAMETER,
//! BLOCKALIGNMENTPARAMETER, BLOCKXYPARAMETER, BLOCKALIGNEDCONSTRAINTPARAMETER,
//! BLOCKHORIZONTALCONSTRAINTPARAMETER, BLOCKVERTICALCONSTRAINTPARAMETER,
//! BLOCKVISIBILITYPARAMETER, and BLOCKMOVEACTION additionally decode their
//! full body.  Deeper per-class params are left raw-shelved.  Known typed
//! layouts are parsed transactionally and reject a truncated or structurally
//! invalid body; intentional prefix-only shells may still publish their
//! validated prefix and retain the remaining bytes in the raw shelf.
class DRW_DynamicBlockObject : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  //! How much of the body layout is known for a given recName.  Selects the
  //! decode depth in parseDwg; classify() maps a recName to a Kind.
  enum class Kind {
    Bare,           /*!< HANDLE_UNKNOWN_BITS / no evalexpr — shell only */
    PurgePreventer, /*!< flag + block handle, no evalexpr */
    EvalExprOnly, /*!< AcDbEvalExpr prefix only (e.g. DYNAMICBLOCKPROXYNODE) */
    GripLocationComponent, /*!< evalexpr + AcDbBlockGripExpr (grip_type +
                              grip_expr) */
    Grip,                  /*!< evalexpr + AcDbBlockElement prefix */
    Parameter,          /*!< evalexpr + element + AcDbBlockParameter 2 bits */
    PointParameter,     /*!< full BLOCKPOINTPARAMETER body */
    BasePointParameter, /*!< full BLOCKBASEPOINTPARAMETER body */
    LookupParameter,    /*!< full BLOCKLOOKUPPARAMETER body */
    FlipParameter,      /*!< full BLOCKFLIPPARAMETER body */
    RotationParameter,  /*!< full BLOCKROTATIONPARAMETER body */
    LinearParameter,    /*!< full BLOCKLINEARPARAMETER body */
    PolarParameter,     /*!< full BLOCKPOLARPARAMETER body */
    AlignmentParameter, /*!< full BLOCKALIGNMENTPARAMETER body */
    XYParameter,        /*!< full BLOCKXYPARAMETER body */
    AlignedConstraintParameter, /*!< full BLOCKALIGNEDCONSTRAINTPARAMETER body
                                 */
    HorizontalConstraintParameter, /*!< full BLOCKHORIZONTALCONSTRAINTPARAMETER
                                      body */
    VerticalConstraintParameter, /*!< full BLOCKVERTICALCONSTRAINTPARAMETER body
                                  */
    AngularConstraintParameter,  /*!< full BLOCKANGULARCONSTRAINTPARAMETER body
                                  */
    DiametricConstraintParameter, /*!< full BLOCKDIAMETRICCONSTRAINTPARAMETER
                                     body */
    RadialConstraintParameter, /*!< full BLOCKRADIALCONSTRAINTPARAMETER body */
    Action,                    /*!< evalexpr + element prefix */
    VisibilityParameter,       /*!< full BLOCKVISIBILITYPARAMETER body */
    MoveAction,                /*!< full BLOCKMOVEACTION body */
    ScaleAction,               /*!< full BLOCKSCALEACTION body */
    StretchAction,             /*!< full BLOCKSTRETCHACTION body */
    FlipAction,                /*!< full BLOCKFLIPACTION body */
    RotateAction,              /*!< full BLOCKROTATEACTION body */
    ArrayAction,               /*!< full BLOCKARRAYACTION body */
    LookupAction,              /*!< full BLOCKLOOKUPACTION body */
    PolarStretchAction,        /*!< full BLOCKPOLARSTRETCHACTION body */
    ParameterDependencyBody    /*!< fixed BLOCKPARAMDEPENDENCYBODY body */
  };

  explicit DRW_DynamicBlockObject(const UTF8STRING &recordName = UTF8STRING())
      : m_recordName(recordName) {
    tType = DRW::DYNAMICBLOCKOBJECT;
    m_kind = classify(recordName);
  }

  //! True for every dynamic-block custom-class recName (PARAMETER/ACTION/GRIP
  //! suffixes + the named singletons).  Explicitly rejects the structural
  //! BLOCK_HEADER / BLOCK_CONTROL / BLOCK_RECORD table objects.
  static bool isDynamicBlockRecName(const UTF8STRING &rn);
  //! Maps a dynamic-block recName to its decode Kind.
  static Kind classify(const UTF8STRING &rn);

protected:
  void reset();
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  UTF8STRING m_recordName;
  Kind m_kind = Kind::Bare;

  // AcDbEvalExpr prefix (shared by every evalexpr-bearing class).
  bool m_evalExprParsed = false;
  std::uint32_t m_parentId = 0; /*!< BLd evalexpr.parentid (0xFFFFFFFF = -1) */
  std::uint32_t m_major = 0;    /*!< BL evalexpr.major (DXF 98) */
  std::uint32_t m_minor = 0;    /*!< BL evalexpr.minor (DXF 99) */
  std::int16_t m_valueCode =
      0; /*!< BSd evalexpr.value_code (DXF 70; -9999 = none) */
  double m_valueDouble = 0.0; /*!< selector-40 EvalExpr payload (DXF 140) */
  bool m_hasValueDouble = false;
  std::uint32_t m_valueHandle = 0; /*!< handle value when m_valueCode == 91 */
  std::uint32_t m_nodeId = 0;      /*!< BL evalexpr.nodeid (DXF 90) */

  // AcDbDynamicBlockPurgePreventer.  This class intentionally has no
  // AcDbEvalExpr prefix; its body is the flag followed by one block handle.
  std::uint16_t m_purgeFlag = 0;
  std::uint32_t m_purgeBlockHandle = 0;

  // AcDbBlockElement prefix (parameter/grip/action classes).
  bool m_elementParsed = false;
  UTF8STRING m_elementName;         /*!< T name (DXF 300) */
  std::uint32_t m_elementMajor = 0; /*!< BL be_major (DXF 98) */
  std::uint32_t m_elementMinor = 0; /*!< BL be_minor (DXF 99) */
  std::int32_t m_eed1071 = 0;       /*!< BL eed1071 (DXF 1071) */

  // AcDbBlockParameter (parameter classes).
  bool m_showProperties = false; /*!< B (DXF 280) */
  bool m_chainActions = false;   /*!< B (DXF 281) */

  // AcDbBlockGripExpr (BLOCKGRIPLOCATIONCOMPONENT).
  std::int32_t m_gripType = 0; /*!< BL grip_type (DXF 91) */
  UTF8STRING m_gripExpr;       /*!< T grip_expr (DXF 300) */

  // AcDbBlockGrip body shared by BLOCK*GRIP classes.
  std::int32_t m_gripValue91 = 0;
  std::int32_t m_gripValue92 = 0;
  DRW_Coord m_gripLocation;
  bool m_gripValue280 = false;
  std::int32_t m_gripValue93 = 0;
  std::int32_t m_gripCombinedState = 0;
  bool m_gripHasOrientation = false;
  DRW_Coord m_gripOrientation;

  // AcDbBlock1PtParameter shared one-point parameter body.
  DRW_Coord m_defPoint; /*!< 3BD def_pt (DXF 1010) */
  std::int32_t m_propertyInfo1Count = 0;
  std::vector<std::int32_t> m_propertyInfo1Codes;
  std::vector<UTF8STRING> m_propertyInfo1Names;
  std::int32_t m_propertyInfo2Count = 0;
  std::vector<std::int32_t> m_propertyInfo2Codes;
  std::vector<UTF8STRING> m_propertyInfo2Names;
  std::int32_t m_propertyInfoCount = 0; /*!< BL num_propinfos */

  // Simple one-point parameter variant used by base-point and lookup
  // parameters (value170/value171/value93 follow the location).
  std::int16_t m_parameterValue170 = 0;
  std::int16_t m_parameterValue171 = 0;
  std::int32_t m_parameterValue93 = 0;

  // AcDbBlock2PtParameter shared body.  The four groups contain the
  // property connection code/name pairs; the four state values are separate
  // because they are present even when a group has no connections.
  DRW_Coord m_firstPoint;
  DRW_Coord m_secondPoint;
  std::array<std::vector<std::int32_t>, 4> m_twoPointPropertyCodes;
  std::array<std::vector<UTF8STRING>, 4> m_twoPointPropertyNames;
  std::array<std::int32_t, 4> m_propertyStates{};
  std::int16_t m_parameterBaseLocation = 0;

  // AcDbBlockFlipParameter body.
  UTF8STRING m_flipCaption;
  UTF8STRING m_flipDescription;
  UTF8STRING m_flipBaseStateName;
  UTF8STRING m_flipFlippedStateName;
  DRW_Coord m_flipCaptionLocation;
  UTF8STRING m_flipTooltip;
  std::int32_t m_flipValue96 = 0;

  // AcDbBlockRotationParameter body.
  DRW_Coord m_rotationBaseAnglePoint;
  UTF8STRING m_rotationAngleName;
  UTF8STRING m_rotationAngleDescription;
  double m_rotationAngle = 0.0;
  std::int32_t m_rotationValueSetFlags = 0;
  double m_rotationMinimum = 0.0;
  double m_rotationMaximum = 0.0;
  double m_rotationIncrement = 0.0;
  std::uint16_t m_rotationDeclaredValueCount = 0;
  std::vector<double> m_rotationValues;

  // AcDbBlockLinearParameter body.
  UTF8STRING m_linearDistanceName;
  UTF8STRING m_linearDistanceDescription;
  double m_linearDistance = 0.0;
  std::int32_t m_linearValue96 = 0;
  double m_linearValue141 = 0.0;
  double m_linearValue142 = 0.0;
  double m_linearValue143 = 0.0;
  std::uint16_t m_linearDeclaredValueCount = 0;
  std::vector<double> m_linearValues;

  // AcDbBlockPolarParameter body.
  UTF8STRING m_polarAngleName;
  UTF8STRING m_polarAngleDescription;
  UTF8STRING m_polarDistanceName;
  UTF8STRING m_polarDistanceDescription;
  double m_polarOffset = 0.0;
  std::int32_t m_polarAngleValueSetFlags = 0;
  double m_polarAngleMinimum = 0.0;
  double m_polarAngleMaximum = 0.0;
  double m_polarAngleIncrement = 0.0;
  std::uint16_t m_polarAngleDeclaredValueCount = 0;
  std::vector<double> m_polarAngleValues;
  std::int32_t m_polarDistanceValueSetFlags = 0;
  double m_polarDistanceMinimum = 0.0;
  double m_polarDistanceMaximum = 0.0;
  double m_polarDistanceIncrement = 0.0;
  std::uint16_t m_polarDistanceDeclaredValueCount = 0;
  std::vector<double> m_polarDistanceValues;

  // AcDbBlockAlignmentParameter body.
  bool m_alignPerpendicular = false;

  // AcDbBlockXYParameter body.  DWG stores the X value set before the Y
  // value set; the fields stay separate so the code mapping is not lost.
  UTF8STRING m_xyXLabel;
  UTF8STRING m_xyXDescription;
  UTF8STRING m_xyYLabel;
  UTF8STRING m_xyYDescription;
  double m_xyXValue = 0.0;
  double m_xyYValue = 0.0;
  std::int32_t m_xyXValueSetFlags = 0;
  double m_xyXMinimum = 0.0;
  double m_xyXMaximum = 0.0;
  double m_xyXIncrement = 0.0;
  std::uint16_t m_xyXDeclaredValueCount = 0;
  std::vector<double> m_xyXValues;
  std::int32_t m_xyYValueSetFlags = 0;
  double m_xyYMinimum = 0.0;
  double m_xyYMaximum = 0.0;
  double m_xyYIncrement = 0.0;
  std::uint16_t m_xyYDeclaredValueCount = 0;
  std::vector<double> m_xyYValues;

  // AcDbBlockLinearConstraintParameter body.  The dependency is in the
  // object handle stream for R2007+ and inline for older versions.
  std::uint32_t m_constraintDependencyHandle = 0;
  UTF8STRING m_constraintExpressionName;
  UTF8STRING m_constraintExpressionDescription;
  double m_constraintValue = 0.0;
  std::int32_t m_constraintValueSetFlags = 0;
  double m_constraintMinimum = 0.0;
  double m_constraintMaximum = 0.0;
  double m_constraintIncrement = 0.0;
  std::uint16_t m_constraintDeclaredValueCount = 0;
  std::vector<double> m_constraintValues;
  DRW_Coord m_angularCenterPoint;
  DRW_Coord m_angularEndPoint;
  bool m_angularOrientationOnBothGrips = false;

  // AcDbBlockPointParameter body.
  UTF8STRING m_positionName;        /*!< T position_name (DXF 303) */
  UTF8STRING m_positionDescription; /*!< T position_desc (DXF 304) */
  DRW_Coord m_labelPoint;           /*!< 3BD def_label_pt (DXF 1011) */

  // AcDbBlockBasepointParameter body.
  DRW_Coord m_point;
  DRW_Coord m_basePoint;

  // AcDbBlockLookupParameter body.
  std::int32_t m_lookupIndex = 0;
  UTF8STRING m_lookupName;
  UTF8STRING m_lookupDescription;
  UTF8STRING m_lookupUnknownText;

  // AcDbBlockVisibilityParameter body.
  bool m_isInitialized = false;              /*!< B is_initialized (DXF 281) */
  UTF8STRING m_visibilityName;               /*!< T blockvisi_name (DXF 301) */
  UTF8STRING m_visibilityDescription;        /*!< T blockvisi_desc (DXF 302) */
  bool m_unknownBool = false;                /*!< B (DXF 91) */
  std::int32_t m_blockCount = 0;             /*!< BL num_blocks (DXF 93) */
  std::int32_t m_stateCount = 0;             /*!< BL num_states (DXF 92) */
  std::vector<UTF8STRING> m_stateNames;      /*!< per-state name (DXF 303) */
  std::vector<std::uint32_t> m_blockHandles; /*!< deferred entity handles */
  std::vector<std::vector<std::uint32_t>> m_stateBlockHandles;
  std::vector<std::vector<std::uint32_t>> m_stateParameterHandles;

  // AcDbBlockAction / AcDbBlockMoveAction body.
  DRW_Coord m_displayLocation;        /*!< 3BD display_location */
  std::int32_t m_dependencyCount = 0; /*!< BL num_deps (DXF 71) */
  std::int32_t m_actionCount = 0;     /*!< BL num_actions (DXF 70) */
  std::vector<std::uint32_t>
      m_dependencyHandles;                   /*!< deferred dependency handles */
  std::vector<std::int32_t> m_actionIndexes; /*!< BL[] actions (DXF 91) */
  std::vector<UTF8STRING>
      m_connectionNames; /*!< action connection-point names */
  std::vector<std::int32_t>
      m_connectionCodes;        /*!< action connection-point codes */
  double m_actionOffsetX = 0.0; /*!< BD action_offset_x (DXF 140) */
  double m_actionOffsetY = 0.0; /*!< BD action_offset_y (DXF 141) */
  double m_angleOffset = 0.0;   /*!< BD angle_offset */

  // AcDbBlockActionWithBasePt / AcDbBlockRotateAction body.
  DRW_Coord m_rotateActionOffset;
  std::vector<UTF8STRING> m_rotateBaseConnectionNames;
  std::vector<std::int32_t> m_rotateBaseConnectionCodes;
  bool m_rotateActionDependent = false;
  DRW_Coord m_rotateActionBasePoint;
  std::int32_t m_rotateConnectionCode = 0;
  UTF8STRING m_rotateConnectionName;

  // AcDbBlockArrayAction body.
  double m_arrayColumnOffset = 0.0;
  double m_arrayRowOffset = 0.0;

  // AcDbBlockLookupAction body.
  std::int32_t m_lookupRowCount = 0;
  std::int32_t m_lookupColumnCount = 0;
  std::vector<UTF8STRING> m_lookupExpressions; // row-major rows * columns
  // The AC1024 profile stores one metadata record per column. Other versions
  // retain the prior decoder's per-cell interpretation pending evidence.
  std::vector<std::vector<std::int32_t>> m_lookupConnectionCodes;
  std::vector<std::vector<UTF8STRING>> m_lookupConnectionNames;
  std::vector<bool> m_lookupEntryFlags282;
  std::vector<bool> m_lookupEntryFlags281;
  bool m_lookupFlag280 = false;

  // AcDbBlockPolarStretchAction body.
  std::int32_t m_polarStretchPointCount = 0;
  std::vector<DRW_Coord> m_polarStretchPoints;
  std::int32_t m_polarStretchHandleCount = 0;
  std::vector<std::uint32_t> m_polarStretchHandles;
  std::vector<std::int16_t> m_polarStretchHandleShorts;
  std::int32_t m_polarStretchCodeCount = 0;
  std::vector<std::int32_t> m_polarStretchCodes;

  // AcDbBlockScaleAction body.
  DRW_Coord m_scaleOffset; /*!< 3BD offset */
  std::vector<UTF8STRING> m_baseConnectionNames;
  std::vector<std::int32_t> m_baseConnectionCodes;
  bool m_scaleDependent = false; /*!< B dependent */
  DRW_Coord m_scaleBasePoint;    /*!< 3BD base point */
  std::vector<UTF8STRING> m_scaleConnectionNames;
  std::vector<std::int32_t> m_scaleConnectionCodes;

  // AcDbBlockStretchAction body.
  std::int32_t m_stretchPointCount = 0;
  std::vector<DRW_Coord> m_stretchPoints;
  std::int32_t m_handleReferenceCount = 0;
  std::vector<std::uint32_t> m_handleReferenceHandles;
  std::vector<std::int32_t> m_handleReferenceIndexCounts;
  std::vector<std::vector<std::int32_t>> m_handleReferenceIndexes;
  std::int32_t m_codeReferenceCount = 0;
  std::vector<std::int32_t> m_codeReferenceCodes;
  std::vector<std::int32_t> m_codeReferenceIndexCounts;
  std::vector<std::vector<std::int32_t>> m_codeReferenceIndexes;

  // AcDbBlockParameterDependencyBody body.
  std::int32_t m_adbVersion = 0;
  std::int32_t m_dimensionBaseVersion = 0;
  UTF8STRING m_dependencyName;
  std::int32_t m_classVersion = 0;

  //! True only when a full validation class (GripLocationComponent / Grip /
  //! PointParameter /
  //! BasePointParameter / LookupParameter / FlipParameter /
  //! RotationParameter / LinearParameter / PolarParameter /
  //! AlignmentParameter / XYParameter / constraint parameters /
  //! VisibilityParameter /
  //! MoveAction / ScaleAction / StretchAction / FlipAction / RotateAction /
  //! ArrayAction / LookupAction / PolarStretchAction /
  //! ParameterDependencyBody)
  //! decoded its whole
  //! body without hitting a short read.
  bool m_bodyFullyDecoded = false;
};

//! Common AcDbModelDocViewStyle header shared by detail/section view styles.
struct DRW_ModelDocViewStyle {
  std::uint16_t m_modelDocClassVersion =
      0;                               /*!< DXF 70, AcDbModelDocViewStyle */
  UTF8STRING m_description;            /*!< DXF 3 */
  bool m_modifiedForRecompute = false; /*!< DXF 290 */
  UTF8STRING m_displayName;            /*!< DXF 300, R2018+ */
  std::uint32_t m_viewStyleFlags = 0;  /*!< DXF 90, R2018+ */

private:
  friend class DRW_DetailViewStyle;
  friend class DRW_SectionViewStyle;
  std::array<std::size_t, 2> m_dxfRetainedTextBytes{};
};

//! Class to handle DETAILVIEWSTYLE / AcDbDetailViewStyle objects.
class DRW_DetailViewStyle : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_DetailViewStyle() { reset(); }
  DRW_DetailViewStyle(const DRW_DetailViewStyle& o): DRW_TableEntry(o),
      m_dxfSubclass(), m_dxfGroup(-1), m_dxfHandleCount(0),
      m_dxfColorCount(0), m_dxfDoubleCount(0), m_dxfLongCount(0) {
    copyPersistentFields(o);
    tType = DRW::DETAILVIEWSTYLE;
  }
  DRW_DetailViewStyle& operator=(const DRW_DetailViewStyle& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      copyPersistentFields(o);
      m_dxfSubclass = DxfSubclass::Unknown;
      m_dxfGroup = -1;
      m_dxfHandleCount = 0;
      m_dxfColorCount = 0;
      m_dxfDoubleCount = 0;
      m_dxfLongCount = 0;
      tType = DRW::DETAILVIEWSTYLE;
    }
    return *this;
  }
  void reset();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  DRW_ModelDocViewStyle m_modelDoc;
  std::uint16_t m_classVersion = 0;
  std::uint32_t m_flags = 0;
  std::uint32_t m_identifierStyleHandle = 0;
  int m_identifierColor = 256;
  double m_identifierHeight = 0.0;
  UTF8STRING m_identifierExcludeCharacters;
  double m_identifierOffset = 0.0;
  std::uint8_t m_identifierPlacement = 0;
  std::uint32_t m_arrowSymbolHandle = 0;
  int m_arrowSymbolColor = 256;
  double m_arrowSymbolSize = 0.0;
  std::uint32_t m_boundaryLineTypeHandle = 0;
  std::int32_t m_boundaryLineWeight = 0;
  int m_boundaryLineColor = 256;
  std::uint32_t m_viewLabelTextStyleHandle = 0;
  int m_viewLabelTextColor = 256;
  double m_viewLabelTextHeight = 0.0;
  std::uint32_t m_viewLabelAttachment = 0;
  double m_viewLabelOffset = 0.0;
  std::uint32_t m_viewLabelAlignment = 0;
  UTF8STRING m_viewLabelPattern;
  std::uint32_t m_connectionLineTypeHandle = 0;
  std::int32_t m_connectionLineWeight = 0;
  int m_connectionLineColor = 256;
  std::uint32_t m_borderLineTypeHandle = 0;
  std::int32_t m_borderLineWeight = 0;
  int m_borderLineColor = 256;
  std::uint8_t m_modelEdge = 0;

private:
  // The DXF subclass marker only selects one of these parser states; never
  // retain an arbitrary group-100 string for this transient discriminator.
  enum class DxfSubclass : std::uint8_t {
    Unknown,
    ModelDocViewStyle,
    DetailViewStyle
  };

  void copyPersistentFields(const DRW_DetailViewStyle& o) {
    m_modelDoc = o.m_modelDoc;
    m_classVersion = o.m_classVersion;
    m_flags = o.m_flags;
    m_identifierStyleHandle = o.m_identifierStyleHandle;
    m_identifierColor = o.m_identifierColor;
    m_identifierHeight = o.m_identifierHeight;
    m_identifierExcludeCharacters = o.m_identifierExcludeCharacters;
    m_identifierOffset = o.m_identifierOffset;
    m_identifierPlacement = o.m_identifierPlacement;
    m_arrowSymbolHandle = o.m_arrowSymbolHandle;
    m_arrowSymbolColor = o.m_arrowSymbolColor;
    m_arrowSymbolSize = o.m_arrowSymbolSize;
    m_boundaryLineTypeHandle = o.m_boundaryLineTypeHandle;
    m_boundaryLineWeight = o.m_boundaryLineWeight;
    m_boundaryLineColor = o.m_boundaryLineColor;
    m_viewLabelTextStyleHandle = o.m_viewLabelTextStyleHandle;
    m_viewLabelTextColor = o.m_viewLabelTextColor;
    m_viewLabelTextHeight = o.m_viewLabelTextHeight;
    m_viewLabelAttachment = o.m_viewLabelAttachment;
    m_viewLabelOffset = o.m_viewLabelOffset;
    m_viewLabelAlignment = o.m_viewLabelAlignment;
    m_viewLabelPattern = o.m_viewLabelPattern;
    m_connectionLineTypeHandle = o.m_connectionLineTypeHandle;
    m_connectionLineWeight = o.m_connectionLineWeight;
    m_connectionLineColor = o.m_connectionLineColor;
    m_borderLineTypeHandle = o.m_borderLineTypeHandle;
    m_borderLineWeight = o.m_borderLineWeight;
    m_borderLineColor = o.m_borderLineColor;
    m_modelEdge = o.m_modelEdge;
    m_colorCmc = o.m_colorCmc;
    m_dxfRetainedTextBytes = o.m_dxfRetainedTextBytes;
  }
  DxfSubclass m_dxfSubclass = DxfSubclass::Unknown;
  int m_dxfGroup = -1;
  int m_dxfHandleCount = 0;
  int m_dxfColorCount = 0;
  int m_dxfDoubleCount = 0;
  int m_dxfLongCount = 0;
  std::array<std::size_t, 2> m_dxfRetainedTextBytes{};

public:
  //! DWG CMC state aligned with the scalar colors above:
  //! identifier, arrow, boundary, label, connection, border.
  std::array<DRW_DwgCmcColorMetadata, 6> m_colorCmc{};
};

//! Class to handle SECTIONVIEWSTYLE / AcDbSectionViewStyle objects.
class DRW_SectionViewStyle : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::int32_t kMaxHatchAngles = 100000;
  DRW_SectionViewStyle() { reset(); }
  DRW_SectionViewStyle(const DRW_SectionViewStyle& o): DRW_TableEntry(o),
      m_dxfSubclass(), m_dxfGroup(-1), m_dxfHandleCount(0),
      m_dxfColorCount(0), m_dxfDoubleCount(0), m_dxfLongCount(0),
      m_dxfBoolCount(0), m_dxfExpectedHatchAngles(0) {
    copyPersistentFields(o);
    tType = DRW::SECTIONVIEWSTYLE;
  }
  DRW_SectionViewStyle& operator=(const DRW_SectionViewStyle& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      copyPersistentFields(o);
      m_dxfSubclass = DxfSubclass::Unknown;
      m_dxfGroup = -1;
      m_dxfHandleCount = 0;
      m_dxfColorCount = 0;
      m_dxfDoubleCount = 0;
      m_dxfLongCount = 0;
      m_dxfBoolCount = 0;
      m_dxfExpectedHatchAngles = 0;
      m_dxfHasExpectedHatchAngles = false;
      tType = DRW::SECTIONVIEWSTYLE;
    }
    return *this;
  }
  void reset();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  DRW_ModelDocViewStyle m_modelDoc;
  std::uint16_t m_classVersion = 0;
  std::uint32_t m_flags = 0;
  std::uint32_t m_identifierStyleHandle = 0;
  int m_identifierColor = 256;
  double m_identifierHeight = 0.0;
  std::uint32_t m_arrowStartSymbolHandle = 0;
  std::uint32_t m_arrowEndSymbolHandle = 0;
  int m_arrowSymbolColor = 256;
  double m_arrowSymbolSize = 0.0;
  UTF8STRING m_identifierExcludeCharacters;
  std::int32_t m_identifierPosition = 0;
  double m_identifierOffset = 0.0;
  std::int32_t m_arrowPosition = 0;
  double m_arrowSymbolExtensionLength = 0.0;
  std::uint32_t m_planeLineTypeHandle = 0;
  std::int32_t m_planeLineWeight = 0;
  int m_planeLineColor = 256;
  std::uint32_t m_bendLineTypeHandle = 0;
  std::int32_t m_bendLineWeight = 0;
  int m_bendLineColor = 256;
  double m_bendLineLength = 0.0;
  double m_endLineOvershoot = 0.0;
  double m_endLineLength = 0.0;
  std::uint32_t m_viewLabelTextStyleHandle = 0;
  int m_viewLabelTextColor = 256;
  double m_viewLabelTextHeight = 0.0;
  std::uint32_t m_viewLabelAttachment = 0;
  double m_viewLabelOffset = 0.0;
  std::uint32_t m_viewLabelAlignment = 0;
  UTF8STRING m_viewLabelPattern;
  int m_hatchColor = 256;
  int m_hatchBackgroundColor = 257;
  UTF8STRING m_hatchPattern;
  double m_hatchScale = 1.0;
  std::int32_t m_hatchTransparency = 0;
  bool m_unknownB1 = false;
  bool m_unknownB2 = false;
  std::vector<double> m_hatchAngles;

  //! Require an explicit matching DXF count whenever angle values are present.
  //! The presence flag is parser state and is not copied with style values.
  bool dxfHatchAngleCountMatches() const noexcept {
    if (!m_dxfHasExpectedHatchAngles)
      return m_hatchAngles.empty();
    return static_cast<std::size_t>(m_dxfExpectedHatchAngles)
        == m_hatchAngles.size();
  }

private:
  // The DXF subclass marker only selects one of these parser states; never
  // retain an arbitrary group-100 string for this transient discriminator.
  enum class DxfSubclass : std::uint8_t {
    Unknown,
    ModelDocViewStyle,
    SectionViewStyle
  };

  void copyPersistentFields(const DRW_SectionViewStyle& o) {
    m_modelDoc = o.m_modelDoc;
    m_classVersion = o.m_classVersion;
    m_flags = o.m_flags;
    m_identifierStyleHandle = o.m_identifierStyleHandle;
    m_identifierColor = o.m_identifierColor;
    m_identifierHeight = o.m_identifierHeight;
    m_arrowStartSymbolHandle = o.m_arrowStartSymbolHandle;
    m_arrowEndSymbolHandle = o.m_arrowEndSymbolHandle;
    m_arrowSymbolColor = o.m_arrowSymbolColor;
    m_arrowSymbolSize = o.m_arrowSymbolSize;
    m_identifierExcludeCharacters = o.m_identifierExcludeCharacters;
    m_identifierPosition = o.m_identifierPosition;
    m_identifierOffset = o.m_identifierOffset;
    m_arrowPosition = o.m_arrowPosition;
    m_arrowSymbolExtensionLength = o.m_arrowSymbolExtensionLength;
    m_planeLineTypeHandle = o.m_planeLineTypeHandle;
    m_planeLineWeight = o.m_planeLineWeight;
    m_planeLineColor = o.m_planeLineColor;
    m_bendLineTypeHandle = o.m_bendLineTypeHandle;
    m_bendLineWeight = o.m_bendLineWeight;
    m_bendLineColor = o.m_bendLineColor;
    m_bendLineLength = o.m_bendLineLength;
    m_endLineOvershoot = o.m_endLineOvershoot;
    m_endLineLength = o.m_endLineLength;
    m_viewLabelTextStyleHandle = o.m_viewLabelTextStyleHandle;
    m_viewLabelTextColor = o.m_viewLabelTextColor;
    m_viewLabelTextHeight = o.m_viewLabelTextHeight;
    m_viewLabelAttachment = o.m_viewLabelAttachment;
    m_viewLabelOffset = o.m_viewLabelOffset;
    m_viewLabelAlignment = o.m_viewLabelAlignment;
    m_viewLabelPattern = o.m_viewLabelPattern;
    m_hatchColor = o.m_hatchColor;
    m_hatchBackgroundColor = o.m_hatchBackgroundColor;
    m_hatchPattern = o.m_hatchPattern;
    m_hatchScale = o.m_hatchScale;
    m_hatchTransparency = o.m_hatchTransparency;
    m_unknownB1 = o.m_unknownB1;
    m_unknownB2 = o.m_unknownB2;
    m_hatchAngles = o.m_hatchAngles;
    m_colorCmc = o.m_colorCmc;
    m_dxfRetainedTextBytes = o.m_dxfRetainedTextBytes;
  }
  DxfSubclass m_dxfSubclass = DxfSubclass::Unknown;
  int m_dxfGroup = -1;
  int m_dxfHandleCount = 0;
  int m_dxfColorCount = 0;
  int m_dxfDoubleCount = 0;
  int m_dxfLongCount = 0;
  int m_dxfBoolCount = 0;
  std::uint32_t m_dxfExpectedHatchAngles = 0;
  bool m_dxfHasExpectedHatchAngles = false;
  std::array<std::size_t, 3> m_dxfRetainedTextBytes{};

public:
  //! DWG CMC state aligned with the scalar colors above:
  //! identifier, arrow, plane, bend, label, hatch, hatch background.
  std::array<DRW_DwgCmcColorMetadata, 7> m_colorCmc{};
};

//! Class to handle BREAKDATA / AcDbBreakData objects.
class DRW_BreakData : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_BreakData() { reset(); }
  DRW_BreakData(const DRW_BreakData& o): DRW_TableEntry(o),
      m_pointRefHandles(o.m_pointRefHandles),
      m_dimensionHandle(o.m_dimensionHandle), m_dxfInBreakData(false) {
    tType = DRW::BREAKDATA;
  }
  DRW_BreakData& operator=(const DRW_BreakData& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      m_pointRefHandles = o.m_pointRefHandles;
      m_dimensionHandle = o.m_dimensionHandle;
      m_dxfInBreakData = false;
      tType = DRW::BREAKDATA;
    }
    return *this;
  }
  void reset();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  std::vector<std::uint32_t> m_pointRefHandles;
  std::uint32_t m_dimensionHandle = 0;

private:
  bool m_dxfInBreakData = false;
};

//! Placeholder carrier for BREAKPOINTREF / AcDbBreakPointRef objects.
class DRW_BreakPointRef : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  DRW_BreakPointRef() { reset(); }
  void reset() {
    tType = DRW::BREAKPOINTREF;
    DRW_TableEntry::reset();
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
};

//! Owned CMC source fields; the legacy scalar color is only a normalized view.
struct DRW_VisualStyleColor {
  bool present = false;
  bool modern = false; // R2004+ BS/BL/RC rather than an index-only BS.
  std::uint16_t rawIndex = 0;
  std::uint32_t rawRgb = 0; // Includes the method byte, even if unrecognized.
  std::uint8_t flags = 0;
  int normalizedIndex = 0;
  UTF8STRING name, bookName; // Decoded strings; not raw text bytes.
};

//! Source property metadata, not a DXF tag stream or a conformance receipt.
struct DRW_VisualStyleEncoding {
  static constexpr std::size_t kPropertyCount = 58;
  static constexpr std::size_t kColorCount = 8;
  std::array<std::uint16_t, kPropertyCount> companions{};
  std::uint8_t companionCount = 0;
  // face mono, intersection, obscured, edge, silhouette, prop29/2c/33.
  std::array<DRW_VisualStyleColor, kColorCount> colors{};
  std::uint8_t colorCount = 0;
  bool hasSourceIsolines = false;
  std::int32_t sourceIsolines = 0;
};

//! AcDbVisualStyle body — face/edge/display property groups + r2013b expansion.
/*!
 *  Named primary values retain their legacy meanings. Interleaved BS values
 *  and CMC source fields are owned in m_encoding without interpreting them as
 *  ignorable sentinels or normalizing unknown color methods. Subsets per version:
 *  legacy (< AC1024) fills face/edge/display; r2010b (>= AC1024) fills those
 *  plus extLightingModel; r2013b (>= AC1027) additionally fills the b/bl/bd/c
 *  prop* expansion. Scalar colors retain the existing normalized convenience
 *  index; m_encoding preserves the distinct source representation. Consumers
 *  must rebuild; this adds no callback or binary-ABI guarantee.
 */
struct DRW_VisualStyleBody {
  static constexpr int kMaxEdgeIsolines = 5000;

  // face group
  int faceLightingModel = 0, faceLightingQuality = 0, faceColorMode = 0,
      faceModifier = 0;
  double faceOpacity = 0.0, faceSpecular = 0.0;
  int faceMonoColor = 0;
  // edge group
  int edgeModel = 0, edgeStyle = 0, edgeIntersectionColor = 0,
      edgeObscuredColor = 0;
  int edgeObscuredLtype = 0, edgeIntersectionLtype = 0, edgeModifier = 0,
      edgeColor = 0;
  int edgeWidth = 0, edgeOverhang = 0, edgeJitter = 0, edgeSilhouetteColor = 0,
      edgeSilhouetteWidth = 0;
  int edgeHaloGap = 0, edgeIsolines = 0, edgeStyleApply = 0;
  double edgeCreaseAngle = 0.0, edgeOpacity = 0.0;
  bool edgeDoHidePrecision = false;
  // display group
  int displaySettings = 0, displayShadowType = 0;
  double displayBrightness = 0.0;
  // r2010b header
  int extLightingModel = 0;
  bool internalOnly = false;
  // r2013b expansion (>= AC1027)
  bool hasR2013bExpansion = false;
  bool bProp1c = false, bProp1d = false, bProp1e = false, bProp1f = false;
  bool bProp20 = false, bProp21 = false, bProp22 = false, bProp23 = false,
       bProp24 = false;
  int blProp25 = 0;
  double bdProp26 = 0.0, bdProp27 = 0.0;
  int blProp28 = 0, cProp29 = 0, blProp2a = 0, blProp2b = 0, cProp2c = 0;
  bool bProp2d = false;
  int blProp2e = 0, blProp2f = 0, blProp30 = 0;
  bool bProp31 = false;
  int blProp32 = 0, cProp33 = 0;
  double bdProp34 = 0.0;
  int edgeWiggle = 0;
  UTF8STRING strokes;
  bool bProp37 = false;
  double bdProp38 = 0.0, bdProp39 = 0.0;
  // Assign {} explicitly when replacing source metadata with authored values.
  DRW_VisualStyleEncoding m_encoding;
};

/** Primary DXF VISUALSTYLE property values, independent of native CMC encoding.
 * Occurrence counts distinguish absence, explicit zero and repeated fields.
 * Color62/Color63/FaceMonoRgb are separate source roles. No native palette
 * index is inferred from RGB421. Description text stays on DRW_VisualStyle.
 * Extension fields after291 remain on the raw carrier; tailGroups counts
 * decoder-visible groups excluding consumed application groups, not a full
 * physical span inventory or certificate. */
struct DRW_DxfVisualStyleProperties {
  enum class Integer : std::size_t {
    Type,FaceLightingModel,FaceLightingQuality,FaceColorMode,FaceModifiers,
    Color62,Color63,FaceMonoRgb,EdgeModel,EdgeStyle,EdgeIntersectionColor,
    EdgeObscuredColor,EdgeObscuredLinetype,EdgeIntersectionLinetype,EdgeModifiers,
    EdgeColor,EdgeWidth,EdgeOverhang,EdgeJitter,EdgeSilhouetteColor,
    EdgeSilhouetteWidth,EdgeHaloGap,EdgeIsolines,EdgeHidePrecision,EdgeStyleApply,
    DisplaySettings,ShadowType,InternalOnly,Count
  };
  enum class Real : std::size_t {FaceOpacity,FaceSpecular,EdgeCreaseAngle,EdgeOpacity,Brightness,Count};
  static constexpr std::array<int,static_cast<std::size_t>(Integer::Count)> integerCodes{
    70,71,72,73,90,62,63,421,74,91,64,65,75,175,92,66,76,77,78,67,79,170,171,290,174,93,173,291};
  static constexpr std::array<int,static_cast<std::size_t>(Real::Count)> realCodes{40,41,42,43,44};
  std::array<std::int32_t,integerCodes.size()> integers{};
  std::array<double,realCodes.size()> reals{};
  std::array<std::size_t,integerCodes.size()> integerOccurrences{};
  std::array<std::size_t,realCodes.size()> realOccurrences{};
  std::size_t descriptionOccurrences{0},legacyDescriptionOccurrences{0},tailGroups{0};
  bool inVisualStyle{true},propertyTail{false},invalidBoolean{false};
  std::int32_t value(Integer field) const noexcept {const auto i=static_cast<std::size_t>(field);return i<integers.size()?integers[i]:0;}
  double value(Real field) const noexcept {const auto i=static_cast<std::size_t>(field);return i<reals.size()?reals[i]:0.0;}
  std::size_t occurrences(Integer field) const noexcept {const auto i=static_cast<std::size_t>(field);return i<integerOccurrences.size()?integerOccurrences[i]:0u;}
  std::size_t occurrences(Real field) const noexcept {const auto i=static_cast<std::size_t>(field);return i<realOccurrences.size()?realOccurrences[i]:0u;}
};

//! Class to handle VISUALSTYLE (AcDbVisualStyle) — custom-class object
//! Non-fixed class; ODA 5.4.1 lists it but does not specify its complete body.
/*!
 *  Decodes the full AcDbVisualStyle body (legacy < AC1024 / r2010b >= AC1024 /
 *  r2013b expansion >= AC1027) into m_body for structured-metadata parity with
 *  dwgTs. Object is parsed from a size-bounded buffer and the raw shelf carries
 *  the byte image, and the common handle stream is validated before the
 *  reader publishes the typed/raw pair.
 */
class DRW_VisualStyle : public DRW_TableEntry {
  SETOBJFRIENDS
  friend class dxfRendering::Observation;
public:
  static constexpr std::uint16_t kDwgClassNum = 560;

  DRW_VisualStyle() { reset(); }
  void reset() {
    DRW_TableEntry::reset();
    tType = DRW::VISUALSTYLE;
    desc.clear();
    m_dxfRetainedDescBytes = 0u;
    type = 0;
    m_body = DRW_VisualStyleBody{};
    m_bodyDecoded = false;
    m_dxfInVisualStyle = true;
    m_dxfInPropertyTail = false;
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  UTF8STRING desc; /*!< description (TV in DWG) */
  std::int32_t type =
      0; /*!< visual-style type code (BL style_type in DWG, 70) */
  DRW_VisualStyleBody
      m_body; /*!< native body or unambiguous primary DXF values; DXF color62/63/421 and presence use the separate property view */
  bool m_bodyDecoded =
      false; /*!< true only after body, strings, and handles validate */

private:
  // Markerless older DXF records begin in the basic field scope. A real
  // subclass marker scopes description/type;291 latches the opaque property
  // tail until reset, including across repeated subclass markers. These are
  // parser state, not a property-grammar or source-profile certificate.
  bool m_dxfInVisualStyle = true;
  bool m_dxfInPropertyTail = false;
  // Logical bytes retained by DXF-parsed description text only.
  std::size_t m_dxfRetainedDescBytes = 0u;
};

//! Class to handle UNDERLAYDEFINITION (AcDb*Definition) — custom-class object.
/*!
 *  Three flavors share one class: PDF, DGN, DWF (Pdf/Dgn/DwfDefinition).
 *  Lives in the OBJECTS section. Each carries a filename + sheet/layout name
 *  that the matching UNDERLAY entity references via its definitionHandle.
 */
class DRW_UnderlayDefinition : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  enum Kind { PDF, DGN, DWF };
  static constexpr std::uint16_t kDwgClassNumPdf = 526;
  static constexpr std::uint16_t kDwgClassNumDgn = 527;
  static constexpr std::uint16_t kDwgClassNumDwf = 528;
  DRW_UnderlayDefinition() { reset(); }
  void reset() {
    tType = DRW::UNDERLAYDEFINITION;
    kind = PDF;
    filename.clear();
    sheetName.clear();
    m_dxfRetainedFilenameBytes = 0u;
    m_dxfRetainedSheetNameBytes = 0u;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  Kind kind = PDF;
  UTF8STRING filename;
  UTF8STRING sheetName;

private:
  std::size_t m_dxfRetainedFilenameBytes{0u};
  std::size_t m_dxfRetainedSheetNameBytes{0u};
};

//! INDEX (AcDbIndex) — generic index object with a last-updated TIMEBLL.
class DRW_Index : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgType = 1307;

  DRW_Index() { reset(); }
  DRW_Index(const DRW_Index& o): DRW_TableEntry(o),
      timestamp1(o.timestamp1), timestamp2(o.timestamp2),
      m_dxfTimestampSeen(false) {
    tType = o.tType;
  }
  DRW_Index& operator=(const DRW_Index& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      timestamp1 = o.timestamp1;
      timestamp2 = o.timestamp2;
      m_dxfTimestampSeen = false;
      tType = o.tType;
    }
    return *this;
  }
  void reset() {
    tType = DRW::INDEX;
    timestamp1 = 0;
    timestamp2 = 0;
    m_dxfTimestampSeen = false;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;

public:
  std::uint32_t timestamp1 = 0; /*!< TIMEBLL Julian day */
  std::uint32_t timestamp2 = 0; /*!< TIMEBLL milliseconds */

private:
  bool m_dxfTimestampSeen = false;
};

//! IDBUFFER (AcDbIdBuffer) — ODA §20.4.79.
/*!
 *  Holds a list of object handles. Used as an opaque selection-set carrier
 *  by xref-clip, filter chains, etc.
 */
class DRW_IDBuffer : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 509;
  static constexpr std::size_t kMaxObjectIds = 100000;

  DRW_IDBuffer() { reset(); }
  DRW_IDBuffer(const DRW_IDBuffer& o): DRW_TableEntry(o),
      classVersion(o.classVersion), objIds(o.objIds),
      m_dxfBodySeen(false) {
    tType = o.tType;
  }
  DRW_IDBuffer& operator=(const DRW_IDBuffer& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      classVersion = o.classVersion;
      objIds = o.objIds;
      m_dxfBodySeen = false;
      tType = o.tType;
    }
    return *this;
  }
  void reset() {
    tType = DRW::IDBUFFER;
    classVersion = 0;
    objIds.clear();
    m_dxfBodySeen = false;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  int classVersion = 0;              /*!< class_version RC, always 0 */
  std::vector<std::uint32_t> objIds; /*!< object handles (soft pointer) */

private:
  bool m_dxfBodySeen = false;
};

//! LAYER_INDEX (AcDbLayerIndex) — ODA §20.4.83.
/*!
 *  Layer index — for each layer name records the entry handle to the
 *  matching IDBUFFER (containing the entities on that layer).
 */
struct DRW_LayerIndexEntry {
  int indexLong = 0;
  UTF8STRING name;
  std::uint32_t entryHandle =
      0; /*!< populated from the post-body handle stream */
};

class DRW_LayerIndex : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 510;
  static constexpr std::size_t kMaxEntries = 100000;

  DRW_LayerIndex() { reset(); }
  DRW_LayerIndex(const DRW_LayerIndex& o): DRW_TableEntry(o),
      timestamp1(o.timestamp1), timestamp2(o.timestamp2),
      entries(o.entries), m_dxfTimestampSeen(false),
      m_dxfPendingEntry(false), m_dxfPendingIndex(0),
      m_dxfPendingName(),
      m_dxfRetainedEntryNameBytes(o.m_dxfRetainedEntryNameBytes) {
    tType = o.tType;
  }
  DRW_LayerIndex& operator=(const DRW_LayerIndex& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      timestamp1 = o.timestamp1;
      timestamp2 = o.timestamp2;
      entries = o.entries;
      m_dxfTimestampSeen = false;
      m_dxfPendingEntry = false;
      m_dxfPendingIndex = 0;
      m_dxfPendingName.clear();
      m_dxfRetainedEntryNameBytes = o.m_dxfRetainedEntryNameBytes;
      m_dxfRetainedPendingNameBytes = 0u;
      tType = o.tType;
    }
    return *this;
  }
  void reset() {
    tType = DRW::LAYERINDEX;
    timestamp1 = 0;
    timestamp2 = 0;
    entries.clear();
    m_dxfTimestampSeen = false;
    m_dxfPendingEntry = false;
    m_dxfPendingIndex = 0;
    m_dxfPendingName.clear();
    m_dxfRetainedEntryNameBytes = 0u;
    m_dxfRetainedPendingNameBytes = 0u;
    DRW_TableEntry::reset();
  }
  bool finalizeDxf();

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  std::uint32_t timestamp1 = 0; /*!< Julian day BL */
  std::uint32_t timestamp2 = 0; /*!< milliseconds BL */
  std::vector<DRW_LayerIndexEntry> entries;

private:
  bool m_dxfTimestampSeen = false;
  bool m_dxfPendingEntry = false;
  int m_dxfPendingIndex = 0;
  UTF8STRING m_dxfPendingName;
  std::size_t m_dxfRetainedEntryNameBytes{0u};
  std::size_t m_dxfRetainedPendingNameBytes{0u};
};

//! SPATIAL_INDEX (AcDbSpatialIndex) — ODA §20.4.95.
/*!
 *  Spatial index for accelerated entity lookup. Body beyond timestamps is
 *  unspecified ("rest of bits to handles") in the ODA spec; we capture
 *  only the timestamps + common handles. The opaque blob between
 *  timestamps and handles is preserved by bit-accurate replay on write.
 */
class DRW_SpatialIndex : public DRW_TableEntry {
  SETOBJFRIENDS
public:
  static constexpr std::uint16_t kDwgClassNum = 511;

  DRW_SpatialIndex() { reset(); }
  DRW_SpatialIndex(const DRW_SpatialIndex& o): DRW_TableEntry(o),
      timestamp1(o.timestamp1), timestamp2(o.timestamp2),
      m_rawData(o.m_rawData), m_rawDataBitSize(o.m_rawDataBitSize),
      m_rawDataValid(o.m_rawDataValid), m_rawDataVersion(o.m_rawDataVersion),
      m_dxfTimestampSeen(false) {
    tType = o.tType;
  }
  DRW_SpatialIndex& operator=(const DRW_SpatialIndex& o) {
    if (this != &o) {
      DRW_TableEntry::operator=(o);
      timestamp1 = o.timestamp1;
      timestamp2 = o.timestamp2;
      m_rawData = o.m_rawData;
      m_rawDataBitSize = o.m_rawDataBitSize;
      m_rawDataValid = o.m_rawDataValid;
      m_rawDataVersion = o.m_rawDataVersion;
      m_dxfTimestampSeen = false;
      tType = o.tType;
    }
    return *this;
  }
  void reset() {
    tType = DRW::SPATIALINDEX;
    timestamp1 = 0;
    timestamp2 = 0;
    m_rawData.clear();
    m_rawDataBitSize = 0;
    m_rawDataValid = false;
    m_rawDataVersion = DRW::UNKNOWNV;
    m_dxfTimestampSeen = false;
    DRW_TableEntry::reset();
  }

protected:
  bool parseCode(int code, const std::unique_ptr<dxfReader> &reader) override;
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf = nullptr,
                               dwgBufferW *handleBuf = nullptr) const;

public:
  std::uint32_t timestamp1 = 0; /*!< Julian day BL */
  std::uint32_t timestamp2 = 0; /*!< milliseconds BL */

private:
  std::vector<std::uint8_t> m_rawData;
  std::uint32_t m_rawDataBitSize = 0;
  bool m_rawDataValid = false;
  DRW::Version m_rawDataVersion = DRW::UNKNOWNV;
  bool m_dxfTimestampSeen = false;
};

/** Holds per-write-session maps populated during DXF/DWG writing. */
class DRW_WritingContext {
public:
  DRW_WritingContext() = default;
  std::vector<std::pair<std::string, std::uint32_t>>
      lineTypesMap; /*!< DXF: uppercase name -> handle */
  std::map<std::string, std::uint32_t>
      ltypeMap; /*!< DWG: uppercase ltype name -> handle */
  std::map<std::string, std::uint32_t>
      layerMap; /*!< DWG: uppercase layer name -> handle */
  std::map<std::string, std::uint32_t>
      styleMap; /*!< DWG: uppercase style name -> handle */
  std::map<std::string, std::uint32_t>
      ucsMap; /*!< DWG: uppercase UCS name -> handle */
  std::map<std::string, std::uint32_t>
      viewMap; /*!< DWG: uppercase view name -> handle */
  std::map<std::string, std::uint32_t>
      vportMap; /*!< DWG: uppercase vport name -> handle */
  std::map<std::string, std::uint32_t>
      appidMap; /*!< DWG: uppercase appid name -> handle */
  std::map<std::string, std::uint32_t>
      dimstyleMap; /*!< DWG: uppercase dimstyle name -> handle */
  std::map<std::string, std::uint32_t>
      mlineStyleMap; /*!< DWG: uppercase mlinestyle name -> handle */
  /*!< DXF write: source entity handle -> minted code-5 handle, captured in
   * dxfRW::writeEntity. Lets GROUP-emit (F3) resolve a member's SOURCE handle
   * to the handle actually written. emplace-only: a parent re-entering
   * writeEntity (writePolyline/writeInsert reuse ent->handle) keeps the
   * first-seen real source; stale minted-range keys are never queried. */
  std::map<std::uint32_t, std::uint32_t> sourceHandleToMintedMap;
  /*!< DXF write: source table-record handle -> emitted code-5 handle.
   * Kept separate from graphical entities and objects so forward table
   * references (for example complex LTYPE -> STYLE) can be planned before the
   * corresponding table callbacks are serialized. */
  std::map<std::uint32_t, std::uint32_t>
      sourceHandleToWrittenTableMap;
  /*!< DXF write: source data-object handle -> emitted code-5 handle for
   * typed object records that register their identity after successful
   * serialization. Kept separate from graphical entities because object
   * handles are not minted through writeEntity. */
  std::map<std::uint32_t, std::uint32_t>
      sourceHandleToWrittenObjectMap;
  /*!< DXF write: source modeler-entity handle -> minted code-5 handle.
   * Used only to retarget the known ACDSDATA ASM_Data owner key when opaque
   * sections are replayed after entity handles have been allocated. */
  std::map<std::uint32_t, std::uint32_t>
      modelerSourceHandleToMintedMap;
  /*!< Source entity handles emitted during the current DXF write. Used to
   * reject an ACDSDATA modeler key that is duplicated by any written entity. */
  std::set<std::uint32_t> writtenSourceEntityHandles;
  /*!< DXF write: source handles seen on more than one entity. Such a handle
   * cannot identify a GROUP member or other deferred reference, so it is
   * deliberately absent from sourceHandleToMintedMap. */
  std::set<std::uint32_t> ambiguousSourceHandles;
};

namespace DRW {

// Extended color palette:
// The first entry is only for direct indexing starting with [1]
// Color 1 is red (1,0,0)
const unsigned char dxfColors[][3] = {
    {0, 0, 0},       // unused
    {255, 0, 0},     // 1 red
    {255, 255, 0},   // 2 yellow
    {0, 255, 0},     // 3 green
    {0, 255, 255},   // 4 cyan
    {0, 0, 255},     // 5 blue
    {255, 0, 255},   // 6 magenta
    {0, 0, 0},       // 7 black or white
    {128, 128, 128}, // 8 50% gray
    {192, 192, 192}, // 9 75% gray
    {255, 0, 0},     // 10
    {255, 127, 127}, {204, 0, 0},     {204, 102, 102},
    {153, 0, 0},     {153, 76, 76}, // 15
    {127, 0, 0},     {127, 63, 63},   {76, 0, 0},
    {76, 38, 38},    {255, 63, 0}, // 20
    {255, 159, 127}, {204, 51, 0},    {204, 127, 102},
    {153, 38, 0},    {153, 95, 76}, // 25
    {127, 31, 0},    {127, 79, 63},   {76, 19, 0},
    {76, 47, 38},    {255, 127, 0}, // 30
    {255, 191, 127}, {204, 102, 0},   {204, 153, 102},
    {153, 76, 0},    {153, 114, 76}, // 35
    {127, 63, 0},    {127, 95, 63},   {76, 38, 0},
    {76, 57, 38},    {255, 191, 0}, // 40
    {255, 223, 127}, {204, 153, 0},   {204, 178, 102},
    {153, 114, 0},   {153, 133, 76}, // 45
    {127, 95, 0},    {127, 111, 63},  {76, 57, 0},
    {76, 66, 38},    {255, 255, 0}, // 50
    {255, 255, 127}, {204, 204, 0},   {204, 204, 102},
    {153, 153, 0},   {153, 153, 76}, // 55
    {127, 127, 0},   {127, 127, 63},  {76, 76, 0},
    {76, 76, 38},    {191, 255, 0}, // 60
    {223, 255, 127}, {153, 204, 0},   {178, 204, 102},
    {114, 153, 0},   {133, 153, 76}, // 65
    {95, 127, 0},    {111, 127, 63},  {57, 76, 0},
    {66, 76, 38},    {127, 255, 0}, // 70
    {191, 255, 127}, {102, 204, 0},   {153, 204, 102},
    {76, 153, 0},    {114, 153, 76}, // 75
    {63, 127, 0},    {95, 127, 63},   {38, 76, 0},
    {57, 76, 38},    {63, 255, 0}, // 80
    {159, 255, 127}, {51, 204, 0},    {127, 204, 102},
    {38, 153, 0},    {95, 153, 76}, // 85
    {31, 127, 0},    {79, 127, 63},   {19, 76, 0},
    {47, 76, 38},    {0, 255, 0}, // 90
    {127, 255, 127}, {0, 204, 0},     {102, 204, 102},
    {0, 153, 0},     {76, 153, 76}, // 95
    {0, 127, 0},     {63, 127, 63},   {0, 76, 0},
    {38, 76, 38},    {0, 255, 63}, // 100
    {127, 255, 159}, {0, 204, 51},    {102, 204, 127},
    {0, 153, 38},    {76, 153, 95}, // 105
    {0, 127, 31},    {63, 127, 79},   {0, 76, 19},
    {38, 76, 47},    {0, 255, 127}, // 110
    {127, 255, 191}, {0, 204, 102},   {102, 204, 153},
    {0, 153, 76},    {76, 153, 114}, // 115
    {0, 127, 63},    {63, 127, 95},   {0, 76, 38},
    {38, 76, 57},    {0, 255, 191}, // 120
    {127, 255, 223}, {0, 204, 153},   {102, 204, 178},
    {0, 153, 114},   {76, 153, 133}, // 125
    {0, 127, 95},    {63, 127, 111},  {0, 76, 57},
    {38, 76, 66},    {0, 255, 255}, // 130
    {127, 255, 255}, {0, 204, 204},   {102, 204, 204},
    {0, 153, 153},   {76, 153, 153}, // 135
    {0, 127, 127},   {63, 127, 127},  {0, 76, 76},
    {38, 76, 76},    {0, 191, 255}, // 140
    {127, 223, 255}, {0, 153, 204},   {102, 178, 204},
    {0, 114, 153},   {76, 133, 153}, // 145
    {0, 95, 127},    {63, 111, 127},  {0, 57, 76},
    {38, 66, 76},    {0, 127, 255}, // 150
    {127, 191, 255}, {0, 102, 204},   {102, 153, 204},
    {0, 76, 153},    {76, 114, 153}, // 155
    {0, 63, 127},    {63, 95, 127},   {0, 38, 76},
    {38, 57, 76},    {0, 66, 255}, // 160
    {127, 159, 255}, {0, 51, 204},    {102, 127, 204},
    {0, 38, 153},    {76, 95, 153}, // 165
    {0, 31, 127},    {63, 79, 127},   {0, 19, 76},
    {38, 47, 76},    {0, 0, 255}, // 170
    {127, 127, 255}, {0, 0, 204},     {102, 102, 204},
    {0, 0, 153},     {76, 76, 153}, // 175
    {0, 0, 127},     {63, 63, 127},   {0, 0, 76},
    {38, 38, 76},    {63, 0, 255}, // 180
    {159, 127, 255}, {50, 0, 204},    {127, 102, 204},
    {38, 0, 153},    {95, 76, 153}, // 185
    {31, 0, 127},    {79, 63, 127},   {19, 0, 76},
    {47, 38, 76},    {127, 0, 255}, // 190
    {191, 127, 255}, {102, 0, 204},   {153, 102, 204},
    {76, 0, 153},    {114, 76, 153}, // 195
    {63, 0, 127},    {95, 63, 127},   {38, 0, 76},
    {57, 38, 76},    {191, 0, 255}, // 200
    {223, 127, 255}, {153, 0, 204},   {178, 102, 204},
    {114, 0, 153},   {133, 76, 153}, // 205
    {95, 0, 127},    {111, 63, 127},  {57, 0, 76},
    {66, 38, 76},    {255, 0, 255}, // 210
    {255, 127, 255}, {204, 0, 204},   {204, 102, 204},
    {153, 0, 153},   {153, 76, 153}, // 215
    {127, 0, 127},   {127, 63, 127},  {76, 0, 76},
    {76, 38, 76},    {255, 0, 191}, // 220
    {255, 127, 223}, {204, 0, 153},   {204, 102, 178},
    {153, 0, 114},   {153, 76, 133}, // 225
    {127, 0, 95},    {127, 63, 11},   {76, 0, 57},
    {76, 38, 66},    {255, 0, 127}, // 230
    {255, 127, 191}, {204, 0, 102},   {204, 102, 153},
    {153, 0, 76},    {153, 76, 114}, // 235
    {127, 0, 63},    {127, 63, 95},   {76, 0, 38},
    {76, 38, 57},    {255, 0, 63}, // 240
    {255, 127, 159}, {204, 0, 51},    {204, 102, 127},
    {153, 0, 38},    {153, 76, 95}, // 245
    {127, 0, 31},    {127, 63, 79},   {76, 0, 19},
    {76, 38, 47},    {51, 51, 51}, // 250
    {91, 91, 91},    {132, 132, 132}, {173, 173, 173},
    {214, 214, 214}, {255, 255, 255} // 255
};

} // namespace DRW

#ifdef LIBDXFRW_DRW_OBJECTS_RESTORE_NONE
# pragma pop_macro("None")
# undef LIBDXFRW_DRW_OBJECTS_RESTORE_NONE
#endif

#endif

// EOF
