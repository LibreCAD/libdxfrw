/******************************************************************************
** Deterministic semantic-evidence adapter for libdxfrw.                     **
**                                                                           **
** Adapter operations use the public libdxfrw API and C++17 so this source  **
** builds against standalone and LibreCAD's pinned package. Generated raw   **
** DWG controls share a test-only, spec-shaped body builder.                **
******************************************************************************/

#include "drw_interface.h"
#include "libdwgr.h"
#include "libdxfrw.h"
#include "generated_raw_dwg_object.h"
#include "nonfinite_dwg_line_wire.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef LIBDXFRW_SEMANTIC_ADAPTER_NAME
#define LIBDXFRW_SEMANTIC_ADAPTER_NAME "libdxfrw-semantic-adapter"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_PACKAGE
#define LIBDXFRW_SEMANTIC_ADAPTER_PACKAGE "libdxfrw"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_COMMIT
#define LIBDXFRW_SEMANTIC_ADAPTER_COMMIT "unknown"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_SOURCE_DIGEST
#define LIBDXFRW_SEMANTIC_ADAPTER_SOURCE_DIGEST "unknown"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_CONFIG_DIGEST
#define LIBDXFRW_SEMANTIC_ADAPTER_CONFIG_DIGEST "unknown"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_STATIC_LIBRARY_DIGEST
#define LIBDXFRW_SEMANTIC_ADAPTER_STATIC_LIBRARY_DIGEST "unknown"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_BINARY_DIGEST
#define LIBDXFRW_SEMANTIC_ADAPTER_BINARY_DIGEST "unknown"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_LINKED_CLOSURE_DIGEST
#define LIBDXFRW_SEMANTIC_ADAPTER_LINKED_CLOSURE_DIGEST "unknown"
#endif

namespace {

class Sha256 {
public:
    Sha256() { reset(); }

    void update(const void* data, std::size_t size) {
        const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
        total_ += static_cast<std::uint64_t>(size);
        while (size != 0) {
            const std::size_t take = std::min(size, block_.size() - used_);
            std::memcpy(block_.data() + used_, bytes, take);
            used_ += take;
            bytes += take;
            size -= take;
            if (used_ == block_.size()) {
                transform(block_.data());
                used_ = 0;
            }
        }
    }

    void update(const std::string& value) { update(value.data(), value.size()); }

    std::string finish() {
        const std::uint64_t bitCount = total_ * 8u;
        const std::uint8_t marker = 0x80;
        update(&marker, 1);
        const std::uint8_t zero = 0;
        while (used_ != 56)
            update(&zero, 1);
        std::array<std::uint8_t, 8> length{};
        for (std::size_t i = 0; i < length.size(); ++i)
            length[7 - i] = static_cast<std::uint8_t>(bitCount >> (i * 8));
        update(length.data(), length.size());

        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::hex << std::setfill('0');
        for (std::uint32_t word : state_)
            out << std::setw(8) << word;
        const std::string result = out.str();
        reset();
        return result;
    }

private:
    static std::uint32_t rotate(std::uint32_t value, unsigned count) {
        return (value >> count) | (value << (32u - count));
    }

    void reset() {
        state_ = {{0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                   0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u}};
        block_.fill(0);
        used_ = 0;
        total_ = 0;
    }

    void transform(const std::uint8_t* input) {
        static const std::uint32_t constants[64] = {
            0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
            0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
            0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
            0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
            0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
            0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
            0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
            0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
        };
        std::uint32_t words[64];
        for (std::size_t i = 0; i < 16; ++i) {
            words[i] = (static_cast<std::uint32_t>(input[i * 4]) << 24) |
                       (static_cast<std::uint32_t>(input[i * 4 + 1]) << 16) |
                       (static_cast<std::uint32_t>(input[i * 4 + 2]) << 8) |
                       static_cast<std::uint32_t>(input[i * 4 + 3]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotate(words[i - 15], 7) ^
                                     rotate(words[i - 15], 18) ^
                                     (words[i - 15] >> 3);
            const std::uint32_t s1 = rotate(words[i - 2], 17) ^
                                     rotate(words[i - 2], 19) ^
                                     (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }
        std::uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
        std::uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25);
            const std::uint32_t choose = (e & f) ^ (~e & g);
            const std::uint32_t temp1 = h + s1 + choose + constants[i] + words[i];
            const std::uint32_t s0 = rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + majority;
            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
        state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
    }

    std::array<std::uint32_t, 8> state_{};
    std::array<std::uint8_t, 64> block_{};
    std::size_t used_ = 0;
    std::uint64_t total_ = 0;
};

std::string sha256(const void* data, std::size_t size) {
    Sha256 hash;
    hash.update(data, size);
    return hash.finish();
}

std::string sha256(const std::string& value) {
    return sha256(value.data(), value.size());
}

std::string jsonString(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : value) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20)
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<unsigned>(ch) << std::dec;
            else
                out << static_cast<char>(ch);
        }
    }
    out << '"';
    return out.str();
}

std::string jsonNullable(const std::string& value) {
    return value.empty() ? "null" : jsonString(value);
}

std::string number(double value) {
    if (!std::isfinite(value))
        throw std::domain_error("non-finite semantic number");
    if (value == 0.0)
        value = 0.0; // normalize negative zero
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return out.str();
}

void appendLengthFramed(std::string& destination, const std::string& value) {
    destination += std::to_string(value.size());
    destination.push_back(':');
    destination += value;
}

std::string decimal(std::int64_t value) {
    return std::to_string(value);
}

std::string unsignedDecimal(std::uint64_t value) {
    return std::to_string(value);
}

std::string hexHandle(std::uint32_t value) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::hex << std::nouppercase << value;
    return out.str();
}

std::string indexedId(const char* prefix, std::size_t index) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << prefix << ':' << std::setw(8) << std::setfill('0') << index;
    return out.str();
}

struct Field {
    std::string name;
    std::string type;
    std::string value;
};

Field stringField(const std::string& name, const std::string& value) {
    return Field{name, "string", jsonString(value)};
}
Field handleField(const std::string& name, std::uint32_t value) {
    return Field{name, "handle", jsonString(hexHandle(value))};
}
void appendIdentityHandleFields(std::vector<Field>& fields, std::uint32_t handle,
                                std::uint32_t ownerHandle) {
    if (handle != DRW::NoHandle)
        fields.push_back(handleField("handle", handle));
    if (ownerHandle != DRW::NoHandle)
        fields.push_back(handleField("ownerHandle", ownerHandle));
}
Field intField(const std::string& name, std::int64_t value) {
    return Field{name, "int64", decimal(value)};
}
Field uintField(const std::string& name, std::uint64_t value) {
    return Field{name, "uint64", unsignedDecimal(value)};
}
Field doubleField(const std::string& name, double value) {
    return Field{name, "double", number(value)};
}
Field boolField(const std::string& name, bool value) {
    return Field{name, "bool", value ? "true" : "false"};
}
std::string pointJson(const DRW_Coord& value) {
    return std::string("{\"x\":") + number(value.x) +
           ",\"y\":" + number(value.y) + ",\"z\":" +
           number(value.z) + "}";
}
Field pointField(const std::string& name, const DRW_Coord& value) {
    return Field{name, "point3d", pointJson(value)};
}

template <typename T, typename = void>
struct HasModelerDwgSourceVersion : std::false_type {};
template <typename T>
struct HasModelerDwgSourceVersion<T, std::void_t<decltype(
    std::declval<const T&>().m_dwgSourceVersion)>> : std::true_type {};

template <typename T, typename = void>
struct HasModelerDwgAcisPayload : std::false_type {};
template <typename T>
struct HasModelerDwgAcisPayload<T, std::void_t<decltype(
    std::declval<const T&>().m_dwgAcisPayload)>> : std::true_type {};

template <typename T, typename = void>
struct HasModelerDxfPayloadChunks : std::false_type {};
template <typename T>
struct HasModelerDxfPayloadChunks<T, std::void_t<decltype(
    std::declval<const T&>().m_dxfPayloadChunks)>> : std::true_type {};

template <typename T, typename = void>
struct HasModelerPayloadStatusName : std::false_type {};
template <typename T>
struct HasModelerPayloadStatusName<T, std::void_t<decltype(
    std::declval<const T&>().modelerPayloadStatusName())>> : std::true_type {};

template <typename T, typename = void>
struct HasPayloadRangeBitOffset : std::false_type {};
template <typename T>
struct HasPayloadRangeBitOffset<T, std::void_t<decltype(
    std::declval<const T&>().m_bitOffset)>> : std::true_type {};

template <typename T, typename = void>
struct HasSurfaceDxfPayloadChunks : std::false_type {};
template <typename T>
struct HasSurfaceDxfPayloadChunks<T, std::void_t<decltype(
    std::declval<const T&>().dxfPayloadChunks)>> : std::true_type {};

template <typename T, typename = void>
struct HasDwgReadObservations : std::false_type {};
template <typename T>
struct HasDwgReadObservations<T, std::void_t<decltype(
    std::declval<const T&>().getReadObservations())>> : std::true_type {};

template <typename T, typename ExpectedReturn, typename = void>
struct DwgReadObservationsMethodProfile {
    static constexpr bool present = false;
    static constexpr bool exactConstNoexceptSignature = false;
};
template <typename T, typename ExpectedReturn>
struct DwgReadObservationsMethodProfile<T, ExpectedReturn, std::void_t<
    decltype(&T::getReadObservations)>> {
    static constexpr bool present = true;
    static constexpr bool exactConstNoexceptSignature = std::is_same<
        decltype(&T::getReadObservations),
        ExpectedReturn (T::*)() const noexcept>::value;
};

struct ReadObservationsExactSignatureProbe {
    int getReadObservations() const noexcept;
};
struct ReadObservationsMutableSignatureProbe {
    int getReadObservations() noexcept;
};
struct ReadObservationsMayThrowSignatureProbe {
    int getReadObservations() const;
};
struct ReadObservationsWrongReturnSignatureProbe {
    long getReadObservations() const noexcept;
};
struct ReadObservationsAbsentSignatureProbe {};

static_assert(DwgReadObservationsMethodProfile<
                  ReadObservationsExactSignatureProbe, int>::present
              && DwgReadObservationsMethodProfile<
                  ReadObservationsExactSignatureProbe,
                  int>::exactConstNoexceptSignature,
              "self-test profile must accept the exact accessor signature");
static_assert(DwgReadObservationsMethodProfile<
                  ReadObservationsMutableSignatureProbe, int>::present
              && !DwgReadObservationsMethodProfile<
                  ReadObservationsMutableSignatureProbe,
                  int>::exactConstNoexceptSignature,
              "self-test profile must reject a non-const accessor");
static_assert(DwgReadObservationsMethodProfile<
                  ReadObservationsMayThrowSignatureProbe, int>::present
              && !DwgReadObservationsMethodProfile<
                  ReadObservationsMayThrowSignatureProbe,
                  int>::exactConstNoexceptSignature,
              "self-test profile must reject a potentially-throwing accessor");
static_assert(DwgReadObservationsMethodProfile<
                  ReadObservationsWrongReturnSignatureProbe, int>::present
              && !DwgReadObservationsMethodProfile<
                  ReadObservationsWrongReturnSignatureProbe,
                  int>::exactConstNoexceptSignature,
              "self-test profile must reject a changed accessor return type");
static_assert(!DwgReadObservationsMethodProfile<
                   ReadObservationsAbsentSignatureProbe, int>::present,
              "self-test profile must distinguish an absent accessor");

struct SemanticAdapterCapabilityProfileV1 {
#ifdef LIBDXFRW_SEMANTIC_SIDE_TARGET
    static constexpr const char* name = "semantic-adapter-capabilities-v1";
    static constexpr const char* side = "target";
    static constexpr bool dwgReadObservations = false;
    static constexpr bool modelerDwgSourceVersion = false;
    static constexpr bool modelerDwgAcisPayload = false;
    static constexpr bool modelerDxfPayloadChunks = false;
    static constexpr bool modelerPayloadStatusName = false;
    static constexpr bool payloadRangeBitOffset = false;
    static constexpr bool surfaceDxfPayloadChunks = false;
#else
    static constexpr const char* name = "semantic-adapter-capabilities-v1";
    static constexpr const char* side = "standalone";
    static constexpr bool dwgReadObservations = true;
    static constexpr bool modelerDwgSourceVersion = true;
    static constexpr bool modelerDwgAcisPayload = true;
    static constexpr bool modelerDxfPayloadChunks = true;
    static constexpr bool modelerPayloadStatusName = true;
    static constexpr bool payloadRangeBitOffset = true;
    static constexpr bool surfaceDxfPayloadChunks = true;
#endif
};

void emitCapabilityProfile(std::ostream& out) {
    const auto boolean = [](bool value) {
        return value ? "true" : "false";
    };
    out << "{\"schema\":1,\"name\":"
        << jsonString(SemanticAdapterCapabilityProfileV1::name)
        << ",\"side\":"
        << jsonString(SemanticAdapterCapabilityProfileV1::side)
        << ",\"capabilities\":{"
        << "\"dwgReadObservations\":"
        << boolean(SemanticAdapterCapabilityProfileV1::dwgReadObservations)
        << ",\"modelerDwgSourceVersion\":"
        << boolean(SemanticAdapterCapabilityProfileV1::modelerDwgSourceVersion)
        << ",\"modelerDwgAcisPayload\":"
        << boolean(SemanticAdapterCapabilityProfileV1::modelerDwgAcisPayload)
        << ",\"modelerDxfPayloadChunks\":"
        << boolean(SemanticAdapterCapabilityProfileV1::modelerDxfPayloadChunks)
        << ",\"modelerPayloadStatusName\":"
        << boolean(SemanticAdapterCapabilityProfileV1::modelerPayloadStatusName)
        << ",\"payloadRangeBitOffset\":"
        << boolean(SemanticAdapterCapabilityProfileV1::payloadRangeBitOffset)
        << ",\"surfaceDxfPayloadChunks\":"
        << boolean(SemanticAdapterCapabilityProfileV1::surfaceDxfPayloadChunks)
        << "}}";
}

#ifdef LIBDXFRW_SEMANTIC_SIDE_TARGET
static_assert(DwgReadObservationsMethodProfile<dwgRW, void>::present
                  == SemanticAdapterCapabilityProfileV1::dwgReadObservations,
              "pinned target getReadObservations capability changed");
static_assert(!SemanticAdapterCapabilityProfileV1::dwgReadObservations,
              "target profile must declare no read-observations API");
static_assert(HasModelerDwgSourceVersion<DRW_ModelerGeometry>::value
                  == SemanticAdapterCapabilityProfileV1::modelerDwgSourceVersion,
              "pinned LibreCAD target capability profile changed: "
              "DRW_ModelerGeometry::m_dwgSourceVersion must be absent");
static_assert(HasModelerDwgAcisPayload<DRW_ModelerGeometry>::value
                  == SemanticAdapterCapabilityProfileV1::modelerDwgAcisPayload,
              "pinned LibreCAD target capability profile changed: "
              "DRW_ModelerGeometry::m_dwgAcisPayload must be absent");
static_assert(HasModelerDxfPayloadChunks<DRW_ModelerGeometry>::value
                  == SemanticAdapterCapabilityProfileV1::modelerDxfPayloadChunks,
              "pinned LibreCAD target capability profile changed: "
              "DRW_ModelerGeometry::m_dxfPayloadChunks must be absent");
static_assert(HasModelerPayloadStatusName<DRW_ModelerGeometry>::value
                  == SemanticAdapterCapabilityProfileV1::modelerPayloadStatusName,
              "pinned LibreCAD target capability profile changed: "
              "DRW_ModelerGeometry::modelerPayloadStatusName must be absent");
static_assert(HasPayloadRangeBitOffset<DRW_ModelerPayloadRange>::value
                  == SemanticAdapterCapabilityProfileV1::payloadRangeBitOffset,
              "pinned LibreCAD target capability profile changed: "
              "DRW_ModelerPayloadRange::m_bitOffset must be absent");
static_assert(HasSurfaceDxfPayloadChunks<DRW_Surface>::value
                  == SemanticAdapterCapabilityProfileV1::surfaceDxfPayloadChunks,
              "pinned LibreCAD target capability profile changed: "
              "DRW_Surface::dxfPayloadChunks must be absent");
static_assert(SemanticAdapterCapabilityProfileV1::name[0] != '\0',
              "target capability profile must have a name");
#else
static_assert(DwgReadObservationsMethodProfile<
                  dwgRW, DwgReadObservations>::present
              == SemanticAdapterCapabilityProfileV1::dwgReadObservations
              && DwgReadObservationsMethodProfile<
                  dwgRW, DwgReadObservations>::exactConstNoexceptSignature,
              "standalone capability profile requires "
              "DwgReadObservations getReadObservations() const noexcept");
static_assert(HasModelerDwgSourceVersion<DRW_ModelerGeometry>::value
                  == SemanticAdapterCapabilityProfileV1::modelerDwgSourceVersion
              && std::is_same<decltype(std::declval<DRW_ModelerGeometry&>()
                                           .m_dwgSourceVersion),
                              DRW::Version>::value,
              "standalone capability profile requires the typed DWG source version");
static_assert(HasModelerDwgAcisPayload<DRW_ModelerGeometry>::value
                  == SemanticAdapterCapabilityProfileV1::modelerDwgAcisPayload
              && std::is_same<decltype(std::declval<DRW_ModelerGeometry&>()
                                           .m_dwgAcisPayload),
                              std::vector<std::uint8_t>>::value,
              "standalone capability profile requires the exact DWG ACIS payload vector");
static_assert(HasModelerDxfPayloadChunks<DRW_ModelerGeometry>::value
                  == SemanticAdapterCapabilityProfileV1::modelerDxfPayloadChunks
              && std::is_same<decltype(std::declval<DRW_ModelerGeometry&>()
                                           .m_dxfPayloadChunks),
                              std::vector<DRW_ModelerPayloadChunk>>::value,
              "standalone capability profile requires ordered DXF modeler chunks");
static_assert(HasModelerPayloadStatusName<DRW_ModelerGeometry>::value
                  == SemanticAdapterCapabilityProfileV1::modelerPayloadStatusName
              && std::is_same<
                  decltype(&DRW_ModelerGeometry::modelerPayloadStatusName),
                  const char* (DRW_ModelerGeometry::*)() const noexcept>::value,
              "standalone capability profile requires the exact payload-status accessor");
static_assert(HasPayloadRangeBitOffset<DRW_ModelerPayloadRange>::value
                  == SemanticAdapterCapabilityProfileV1::payloadRangeBitOffset
              && std::is_same<decltype(std::declval<DRW_ModelerPayloadRange&>()
                                           .m_bitOffset),
                              std::uint8_t>::value,
              "standalone capability profile requires bit-precise payload ranges");
static_assert(HasSurfaceDxfPayloadChunks<DRW_Surface>::value
                  == SemanticAdapterCapabilityProfileV1::surfaceDxfPayloadChunks
              && std::is_same<decltype(std::declval<DRW_Surface&>()
                                           .dxfPayloadChunks),
                              std::vector<DRW_ModelerPayloadChunk>>::value,
              "standalone capability profile requires ordered DXF surface chunks");
static_assert(SemanticAdapterCapabilityProfileV1::name[0] != '\0',
              "standalone capability profile must have a name");
#endif

template <typename T>
bool setModelerDwgSourceVersion(T& modeler, DRW::Version version) {
    if constexpr (HasModelerDwgSourceVersion<T>::value) {
        modeler.m_dwgSourceVersion = version;
        return true;
    }
    return false;
}

template <typename T>
bool setModelerDwgAcisPayload(T& modeler,
                              const std::vector<std::uint8_t>& payload) {
    if constexpr (HasModelerDwgAcisPayload<T>::value) {
        modeler.m_dwgAcisPayload = payload;
        return true;
    }
    return false;
}

template <typename T>
bool setModelerDxfPayloadChunk(T& modeler) {
    if constexpr (HasModelerDxfPayloadChunks<T>::value) {
        using Chunks = typename std::decay<decltype(
            modeler.m_dxfPayloadChunks)>::type;
        using Chunk = typename Chunks::value_type;
        Chunk chunk(310, 1, 2);
        modeler.m_dxfPayloadChunks.push_back(chunk);
        return true;
    }
    return false;
}

template <typename T>
bool setSurfaceDxfPayloadChunk(T& surface) {
    if constexpr (HasSurfaceDxfPayloadChunks<T>::value) {
        using Chunks = typename std::decay<decltype(
            surface.dxfPayloadChunks)>::type;
        using Chunk = typename Chunks::value_type;
        Chunk chunk(310, 1, 2);
        surface.dxfPayloadChunks.push_back(chunk);
        return true;
    }
    return false;
}

template <typename T>
bool setPayloadRangeBitOffset(T& range, std::size_t bitOffset) {
    if constexpr (HasPayloadRangeBitOffset<T>::value) {
        range.m_bitOffset = bitOffset;
        return true;
    }
    return false;
}

struct CallbackRow {
    std::string id;
    std::size_t ordinal = 0;
    std::string kind;
    std::string recordId;
    std::string blockContextId;
    std::vector<std::string> carrierIds;
};
struct RecordRow {
    std::string id;
    std::size_t callbackOrdinal = 0;
    std::string kind;
    std::string recordClass;
    std::string entity;
    std::string sourceHandle;
    std::string ownerHandle;
    std::string blockId;
    std::vector<Field> fields;
};
struct NodeRow {
    std::string id;
    std::string kind;
    std::string recordId;
    std::string handle;
};
struct EdgeRow {
    std::string id;
    std::string kind;
    std::string from;
    std::string to;
    std::string disposition;
};
struct CarrierRow {
    std::string id;
    std::string source;
    std::size_t size = 0;
    std::string digest;
    std::string disposition;
    std::string recordId;
    std::string sourceHandle;
};
struct UnsupportedRow {
    std::string id;
    std::string kind;
    std::string source;
    std::string recordId;
    std::string carrierId;
    std::string diagnosticId;
    std::string disposition;
    std::string reason;
};
struct DiagnosticRow {
    std::string id;
    std::size_t index = 0;
    std::string severity;
    std::string stage;
    std::string code;
    std::string path;
    std::string messageDigest;
};

class SemanticSink final : public DRW_Interface {
public:
    explicit SemanticSink(const std::string& facade = std::string(),
                          const std::string& direction = std::string())
        : facade_(facade), direction_(direction) {}

    std::vector<CallbackRow> callbacks;
    std::vector<RecordRow> records;
    std::vector<NodeRow> nodes;
    std::vector<EdgeRow> edges;
    std::vector<CarrierRow> carriers;
    std::vector<UnsupportedRow> unsupported;
    std::vector<DiagnosticRow> diagnostics;

    void configureWrite(dxfRW* writer) {
        dxfWriter_ = writer;
        dwgWriter_ = nullptr;
        writeRecipeSucceeded_ = true;
    }
    void configureWrite(dwgRW* writer) {
        dxfWriter_ = nullptr;
        dwgWriter_ = writer;
        writeRecipeSucceeded_ = true;
    }
    bool writeRecipeSucceeded() const { return writeRecipeSucceeded_; }
    void enable3DConsumerProbe() { threeDConsumerProbe_ = true; }
    void exerciseDefaultMPolygonFallback(const DRW_MPolygon& polygon) {
        DRW_Interface::addMPolygon(&polygon);
    }
    std::vector<DRW_Polyline> meshFallbackPolylines;
    std::vector<DRW_Line> meshFallbackLines;
    std::vector<DRW_Line> faceFallbackLines;
    std::vector<DRW_Line> traceFallbackLines;
    void exerciseDefault3DFaceFallback(const DRW_3Dface& face) {
        struct ResetFlag {
            bool& flag;
            ~ResetFlag() { flag = false; }
        } reset{collect3DFaceFallbackLines_};
        collect3DFaceFallbackLines_ = true;
        DRW_Interface::add3dFace(face);
    }
    void exerciseDefaultTraceFallback(const DRW_Trace& trace) {
        struct ResetFlag {
            bool& flag;
            ~ResetFlag() { flag = false; }
        } reset{collectTraceFallbackLines_};
        collectTraceFallbackLines_ = true;
        DRW_Interface::addTrace(trace);
    }
    void exerciseDefaultSolidFallback(const DRW_Solid& solid) {
        struct ResetFlag {
            bool& flag;
            ~ResetFlag() { flag = false; }
        } reset{collectTraceFallbackLines_};
        collectTraceFallbackLines_ = true;
        DRW_Interface::addSolid(solid);
    }
    void exerciseDefaultMeshFallback(const DRW_Mesh& mesh) {
        struct ResetFlag {
            bool& flag;
            ~ResetFlag() { flag = false; }
        } reset{collectMeshFallbackPolylines_};
        collectMeshFallbackPolylines_ = true;
        DRW_Interface::addMesh(mesh);
    }
    void enableNestedMTextAttributeProbe() {
        nestedMTextAttributeProbe_ = true;
    }
    void enablePlaneSurfaceInlinePayloadProbe() {
        planeSurfaceInlinePayloadProbe_ = true;
    }
    void enableModelerCarrierProbe() { modelerCarrierProbe_ = true; }
    void enablePartialDwgReadbackControl() {
        partialDwgReadbackControl_ = true;
    }
    void enableOpaqueDwgReadbackControl() {
        opaqueDwgReadbackControl_ = true;
        opaqueDwgReadbackObject_ = libdxfrw_test::makeRawCustomObject(
            DRW::AC1027, 500, 0x700u, "SEMANTIC_OPAQUE_TEST",
            "AcDbSemanticOpaqueTest", "LIBDXFRW_SEMANTIC_TEST");
    }

    void addHeader(const DRW_Header*) override {
        addCallback("addHeader");
        addUnsupported("headerPayload", "header", std::string(), std::string(),
                       std::string(), "excluded");
    }
    void addLType(const DRW_LType& d) override { addTable("addLType", "LTYPE", d); }
    void addLayer(const DRW_Layer& d) override { addTable("addLayer", "LAYER", d); }
    void addDimStyle(const DRW_Dimstyle& d) override { addTable("addDimStyle", "DIMSTYLE", d); }
    void addVport(const DRW_Vport& d) override { addTable("addVport", "VPORT", d); }
    void addTextStyle(const DRW_Textstyle& d) override { addTable("addTextStyle", "STYLE", d); }
    void addAppId(const DRW_AppId& d) override { addTable("addAppId", "APPID", d); }

#define SEMANTIC_OPAQUE_VALUE(method, type, token) \
    void method(const type& value) override { \
        (void)value; \
        addOpaqueTyped(#method, token); \
    }
#define SEMANTIC_OPAQUE_POINTER(method, type, token) \
    void method(const type* value) override { \
        if (value == nullptr) addNullDiagnostic(#method); \
        else addOpaqueTyped(#method, token); \
    }

    SEMANTIC_OPAQUE_VALUE(addViewportEntityHeader, DRW_ViewportEntityHeader, "VPORT_ENTITY_HEADER")
    SEMANTIC_OPAQUE_VALUE(addVxControl, DRW_VxControl, "VX_CONTROL")
    SEMANTIC_OPAQUE_VALUE(addVxTableRecord, DRW_VxTableRecord, "VX_TABLE_RECORD")
    SEMANTIC_OPAQUE_VALUE(addTvDeviceProperties, DRW_TvDeviceProperties, "TVDEVICEPROPERTIES")
    SEMANTIC_OPAQUE_VALUE(addCsacDocumentOptions, DRW_CsacDocumentOptions, "CSACDOCUMENTOPTIONS")
    SEMANTIC_OPAQUE_VALUE(addContextDataManager, DRW_ContextDataManager, "CONTEXTDATAMANAGER")
    SEMANTIC_OPAQUE_VALUE(addUCS, DRW_UCS, "UCS")
    SEMANTIC_OPAQUE_VALUE(addView, DRW_View, "VIEW")
    SEMANTIC_OPAQUE_VALUE(addTolerance, DRW_Tolerance, "TOLERANCE")
    SEMANTIC_OPAQUE_VALUE(addDictionary, DRW_Dictionary, "DICTIONARY")
    SEMANTIC_OPAQUE_VALUE(addDictionaryWithDefault, DRW_DictionaryWithDefault, "DICTIONARYWDFLT")
    SEMANTIC_OPAQUE_VALUE(addDictionaryVar, DRW_DictionaryVar, "DICTIONARYVAR")
    SEMANTIC_OPAQUE_VALUE(addXRecord, DRW_XRecord, "XRECORD")
    SEMANTIC_OPAQUE_VALUE(addField, DRW_Field, "FIELD")
    SEMANTIC_OPAQUE_VALUE(addFieldList, DRW_FieldList, "FIELDLIST")
    SEMANTIC_OPAQUE_VALUE(addDataTable, DRW_DataTable, "DATATABLE")
    SEMANTIC_OPAQUE_VALUE(addDynamicBlockObject, DRW_DynamicBlockObject, "DYNAMIC_BLOCK_OBJECT")
    SEMANTIC_OPAQUE_VALUE(addRasterVariables, DRW_RasterVariables, "RASTERVARIABLES")
    SEMANTIC_OPAQUE_VALUE(addWipeoutVariables, DRW_WipeoutVariables, "WIPEOUTVARIABLES")
    SEMANTIC_OPAQUE_VALUE(addSortEntsTable, DRW_SortEntsTable, "SORTENTSTABLE")
    SEMANTIC_OPAQUE_VALUE(addMaterial, DRW_Material, "MATERIAL")
    SEMANTIC_OPAQUE_VALUE(addTableStyle, DRW_TableStyle, "TABLESTYLE")
    SEMANTIC_OPAQUE_VALUE(addTableContent, DRW_TableContentObject, "TABLECONTENT")
    SEMANTIC_OPAQUE_VALUE(addObjectContextData, DRW_ObjectContextData, "OBJECTCONTEXTDATA")
    SEMANTIC_OPAQUE_VALUE(addCellStyleMap, DRW_CellStyleMap, "CELLSTYLEMAP")
    SEMANTIC_OPAQUE_VALUE(addLayout, DRW_Layout, "LAYOUT")
    SEMANTIC_OPAQUE_VALUE(addMLineStyle, DRW_MLineStyle, "MLINESTYLE")
    SEMANTIC_OPAQUE_VALUE(addDwgFramePublication, DRW_DwgFramePublication, "DWG_FRAME_PUBLICATION")
    SEMANTIC_OPAQUE_VALUE(addDwgDictionaryMembership, DRW_DwgDictionaryMembership, "DWG_DICTIONARY_MEMBERSHIP")
    SEMANTIC_OPAQUE_VALUE(addDwgDictionaryWithDefaultMembership, DRW_DwgDictionaryWithDefaultMembership, "DWG_DICTIONARY_DEFAULT_MEMBERSHIP")
    SEMANTIC_OPAQUE_VALUE(addDwgGroupMembership, DRW_DwgGroupMembership, "DWG_GROUP_MEMBERSHIP")
    SEMANTIC_OPAQUE_VALUE(addDwgSortEntsMembership, DRW_DwgSortEntsMembership, "DWG_SORTENTS_MEMBERSHIP")
    SEMANTIC_OPAQUE_VALUE(addDwgFieldListMembership, DRW_DwgFieldListMembership, "DWG_FIELDLIST_MEMBERSHIP")
    SEMANTIC_OPAQUE_VALUE(addDwgFieldPayloadReceipt, DRW_DwgFieldPayloadReceipt, "DWG_FIELD_PAYLOAD_RECEIPT")
    SEMANTIC_OPAQUE_VALUE(addDwgTypedReference, DRW_DwgTypedReference, "DWG_TYPED_REFERENCE")
    SEMANTIC_OPAQUE_VALUE(addDwgBlockReachability, DRW_DwgBlockReachability, "DWG_BLOCK_REACHABILITY")
    SEMANTIC_OPAQUE_VALUE(addDwgFrameCoverageReport, DRW_DwgFrameCoverageReport, "DWG_FRAME_COVERAGE")
    SEMANTIC_OPAQUE_VALUE(addDwgClassCoverageReport, DRW_DwgClassCoverageReport, "DWG_CLASS_COVERAGE")
    SEMANTIC_OPAQUE_VALUE(addDataStorage, DRW_DataStorageSection, "DATASTORAGE")
    SEMANTIC_OPAQUE_VALUE(addDxfClass, DRW_Class, "DXF_CLASS")

#ifdef LIBDXFRW_SEMANTIC_SIDE_STANDALONE
    void addNamedObjectDictionary(const DRW_Dictionary& d) override {
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", "DICTIONARY"));
        fields.push_back(stringField("entity", "DICTIONARY"));
        appendIdentityHandleFields(fields, d.handle, d.parentHandle);
        fields.push_back(stringField("name", d.name));
        fields.push_back(intField("cloning", d.cloning));
        fields.push_back(intField("hardOwner", d.hardOwner));
        fields.push_back(uintField("entryCount", d.m_entries.size()));
        fields.push_back(boolField(
            "entriesComplete",
            facade_ == "dwgRW" ? d.hasCompleteDwgEntries()
                                : d.hasCompleteDxfEntries()));
        for (std::size_t i = 0; i < d.m_entries.size(); ++i) {
            const std::string prefix = "entry." + std::to_string(i);
            fields.push_back(stringField(prefix + ".name", d.m_entries[i].m_name));
            fields.push_back(uintField(prefix + ".handle", d.m_entries[i].m_handle));
        }
        const std::size_t recordIndex = addRecord(
            "addNamedObjectDictionary", "table", "DICTIONARY", fields);
        addOwnerEdges(records[recordIndex], d.handle, d.parentHandle);
        for (const DRW_Dictionary::Entry& entry : d.m_entries) {
            if (entry.m_handle != 0)
                addEdge("handleReference", "node:" + records[recordIndex].id,
                        ensureHandleNode(entry.m_handle));
        }
        addExcluded(records[recordIndex].id, "dictionaryExtensionsAndRawData");
    }
#endif

    void addBlock(const DRW_Block& d) override {
        std::vector<Field> fields = entityFields(d, "BLOCK", "BLOCK");
        fields.push_back(stringField("name", d.name));
        fields.push_back(intField("flags", d.flags));
        const std::size_t recordIndex = addRecord("addBlock", "block", "BLOCK", fields);
        addOwnerEdges(records[recordIndex], d.handle, d.parentHandle);
        currentBlockId_ = "node:" + records[recordIndex].id;
        for (NodeRow& node : nodes) {
            if (node.id == currentBlockId_) {
                node.kind = "block";
                break;
            }
        }
        addExcluded(records[recordIndex].id, "unmodeledBlockPayload");
    }
#ifdef LIBDXFRW_SEMANTIC_SIDE_STANDALONE
    void addBlockEnd(const DRW_Block& d) override {
        std::vector<Field> fields = entityFields(d, "ENDBLK", "ENDBLK");
        addEntityRecord("addBlockEnd", "ENDBLK", std::move(fields), d, true);
    }

    void addBlockRecord(const DRW_Block_Record& d) override {
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", "BLOCK_RECORD"));
        fields.push_back(stringField("entity", "BLOCK_RECORD"));
        appendIdentityHandleFields(fields, d.handle, d.parentHandle);
        fields.push_back(stringField("name", d.name));
        fields.push_back(intField("flags", d.flags));
        fields.push_back(intField("insertionUnits", d.insUnits));
        fields.push_back(pointField("basePoint", d.basePoint));
        fields.push_back(stringField("xrefPath", d.xrefPath));
        fields.push_back(stringField("description", d.description));
        fields.push_back(boolField("canExplode", d.canExplode));
        fields.push_back(intField("blockScaling", d.blockScaling));
        fields.push_back(uintField("layoutHandle", d.layoutHandle));
        fields.push_back(uintField("insertCount", d.insertCount));
        fields.push_back(uintField("insertHandleCount", d.insertHandles.size()));
        for (std::size_t i = 0; i < d.insertHandles.size(); ++i)
            fields.push_back(uintField("insertHandle." + std::to_string(i),
                                       d.insertHandles[i]));
        const std::size_t recordIndex = addRecord(
            "addBlockRecord", "table", "BLOCK_RECORD", fields);
        addOwnerEdges(records[recordIndex], d.handle, d.parentHandle);
        if (d.layoutHandle != 0)
            addEdge("handleReference", "node:" + records[recordIndex].id,
                    ensureHandleNode(d.layoutHandle));
        if (!d.previewData.empty()) {
            const std::string carrierId = addCarrier(
                "blockRecordPreview", d.previewData.data(), d.previewData.size(),
                "preserved", records[recordIndex].id);
            callbacks[records[recordIndex].callbackOrdinal].carrierIds.push_back(
                carrierId);
        }
        addExcluded(records[recordIndex].id, "unmodeledBlockRecordPayload");
    }
#endif
    void setBlock(int handle) override {
        const std::string handleNode = ensureHandleNode(static_cast<std::uint32_t>(handle));
        for (NodeRow& node : nodes) {
            if (node.id == handleNode) {
                node.kind = "block";
                break;
            }
        }
        addCallback("setBlock", std::string(), handleNode);
        currentBlockId_ = handleNode;
    }
    void endBlock() override {
        addCallback("endBlock", std::string(), currentBlockId_);
        currentBlockId_.clear();
    }

    void addPoint(const DRW_Point& d) override {
        std::vector<Field> fields = entityFields(d, "POINT", "POINT");
        fields.push_back(pointField("basePoint", d.basePoint));
        fields.push_back(pointField("extrusion", d.extPoint));
        fields.push_back(doubleField("thickness", d.thickness));
        fields.push_back(doubleField("xAxisAngle", d.xAxisAngle));
        addEntityRecord("addPoint", "POINT", fields, d, false);
    }
    void addLine(const DRW_Line& d) override {
        if (collectMeshFallbackPolylines_)
            meshFallbackLines.push_back(d);
        if (collect3DFaceFallbackLines_)
            faceFallbackLines.push_back(d);
        if (collectTraceFallbackLines_)
            traceFallbackLines.push_back(d);
        std::vector<Field> fields = entityFields(d, "LINE", "LINE");
        fields.push_back(pointField("start", d.basePoint));
        fields.push_back(pointField("end", d.secPoint));
        fields.push_back(pointField("extrusion", d.extPoint));
        fields.push_back(doubleField("thickness", d.thickness));
        addEntityRecord("addLine", "LINE", fields, d, false);
    }
    void add3DLine(const DRW_3DLine& d) override {
        std::vector<Field> fields = entityFields(d, "3DLINE", "3DLINE");
        fields.push_back(pointField("start", d.basePoint));
        fields.push_back(pointField("end", d.secPoint));
        fields.push_back(pointField("extrusion", d.extPoint));
        fields.push_back(doubleField("thickness", d.thickness));
        addEntityRecord("add3DLine", "3DLINE", fields, d, false);
    }
    void addRay(const DRW_Ray& d) override { addLineLike("addRay", "RAY", d); }
    void addXline(const DRW_Xline& d) override { addLineLike("addXline", "XLINE", d); }
    void addArc(const DRW_Arc& d) override {
        std::vector<Field> fields = entityFields(d, "ARC", "ARC");
        fields.push_back(pointField("center", d.basePoint));
        fields.push_back(doubleField("radius", d.radious));
        fields.push_back(doubleField("thickness", d.thickness));
        fields.push_back(pointField("extrusion", d.extPoint));
        fields.push_back(doubleField("startAngleRadians", d.staangle));
        fields.push_back(doubleField("endAngleRadians", d.endangle));
        addEntityRecord("addArc", "ARC", fields, d, false);
    }
    void addCircle(const DRW_Circle& d) override {
        std::vector<Field> fields = entityFields(d, "CIRCLE", "CIRCLE");
        fields.push_back(pointField("center", d.basePoint));
        fields.push_back(doubleField("radius", d.radious));
        fields.push_back(doubleField("thickness", d.thickness));
        fields.push_back(pointField("extrusion", d.extPoint));
        addEntityRecord("addCircle", "CIRCLE", fields, d, false);
    }
    void addEllipse(const DRW_Ellipse& d) override {
        std::vector<Field> fields = entityFields(d, "ELLIPSE", "ELLIPSE");
        fields.push_back(pointField("center", d.basePoint));
        fields.push_back(pointField("majorAxis", d.secPoint));
        fields.push_back(doubleField("ratio", d.ratio));
        fields.push_back(doubleField("startParameter", d.staparam));
        fields.push_back(doubleField("endParameter", d.endparam));
        fields.push_back(pointField("extrusion", d.extPoint));
        addEntityRecord("addEllipse", "ELLIPSE", fields, d, false);
    }
    void addLWPolyline(const DRW_LWPolyline& d) override {
        std::vector<Field> fields = entityFields(d, "LWPOLYLINE", "LWPOLYLINE");
        fields.push_back(intField("flags", d.flags));
        fields.push_back(doubleField("elevation", d.elevation));
        fields.push_back(pointField("extrusion", d.extPoint));
        fields.push_back(doubleField("thickness", d.thickness));
        fields.push_back(uintField("vertexCount", d.vertlist.size()));
        for (std::size_t i = 0; i < d.vertlist.size(); ++i) {
            const std::string prefix = "vertex." + std::to_string(i);
            const auto& vertex = d.vertlist[i];
            fields.push_back(boolField(prefix + ".present",
                                       static_cast<bool>(vertex)));
            if (!vertex)
                continue;
            fields.push_back(doubleField(prefix + ".x", vertex->x));
            fields.push_back(doubleField(prefix + ".y", vertex->y));
            fields.push_back(doubleField(prefix + ".startWidth", vertex->stawidth));
            fields.push_back(doubleField(prefix + ".endWidth", vertex->endwidth));
            fields.push_back(doubleField(prefix + ".bulge", vertex->bulge));
            fields.push_back(intField(prefix + ".identifier", vertex->identifier));
        }
        addEntityRecord("addLWPolyline", "LWPOLYLINE", fields, d, false);
    }
    SEMANTIC_OPAQUE_POINTER(addMLine, DRW_MLine, "MLINE")
    SEMANTIC_OPAQUE_POINTER(addUnderlay, DRW_Underlay, "UNDERLAY")
    SEMANTIC_OPAQUE_VALUE(addShape, DRW_Shape, "SHAPE")
    SEMANTIC_OPAQUE_VALUE(addOle2Frame, DRW_Ole2Frame, "OLE2FRAME")
    SEMANTIC_OPAQUE_VALUE(addOleFrame, DRW_OleFrame, "OLEFRAME")
    SEMANTIC_OPAQUE_VALUE(addProxyEntity, DRW_ProxyEntity, "PROXY_ENTITY")
    SEMANTIC_OPAQUE_POINTER(linkUnderlay, DRW_UnderlayDefinition, "UNDERLAYDEFINITION")
    void addPolyline(const DRW_Polyline& d) override {
        if (collectMeshFallbackPolylines_)
            meshFallbackPolylines.push_back(d);
        std::vector<Field> fields = entityFields(d, "POLYLINE", "POLYLINE");
        fields.push_back(pointField("basePoint", d.basePoint));
        fields.push_back(pointField("extrusion", d.extPoint));
        fields.push_back(doubleField("thickness", d.thickness));
        fields.push_back(intField("flags", d.flags));
        fields.push_back(doubleField("defaultStartWidth", d.defstawidth));
        fields.push_back(doubleField("defaultEndWidth", d.defendwidth));
        fields.push_back(intField("declaredVertexCount", d.vertexcount));
        fields.push_back(intField("declaredFaceCount", d.facecount));
        fields.push_back(intField("smoothM", d.smoothM));
        fields.push_back(intField("smoothN", d.smoothN));
        fields.push_back(intField("curveType", d.curvetype));
        fields.push_back(uintField("vertexCount", d.vertlist.size()));
        if (d.dwgSeqEndHandle() != DRW::NoHandle)
            fields.push_back(handleField("seqEndHandle", d.dwgSeqEndHandle()));
        for (std::size_t i = 0; i < d.vertlist.size(); ++i)
            appendVertexFields(fields, d.vertlist[i].get(), i);
        addEntityRecord("addPolyline", "POLYLINE", fields, d, false);
    }
    void addSpline(const DRW_Spline* d) override {
        if (d == nullptr) { addNullDiagnostic("addSpline"); return; }
        std::vector<Field> fields = entityFields(*d, "SPLINE", "SPLINE");
        appendSplineFields(fields, *d);
        addEntityRecord("addSpline", "SPLINE", fields, *d, false);
    }
    void addHelix(const DRW_Helix* d) override {
        if (d == nullptr) { addNullDiagnostic("addHelix"); return; }
        std::vector<Field> fields = entityFields(*d, "HELIX", "HELIX");
        appendSplineFields(fields, *d);
        fields.push_back(intField("helixMajorVersion", d->m_majorVersion));
        fields.push_back(intField("helixMaintenanceVersion", d->m_maintVersion));
        fields.push_back(pointField("axisBasePoint", d->axisBasePt));
        fields.push_back(pointField("startPoint", d->startPt));
        fields.push_back(pointField("axisVector", d->axisVector));
        fields.push_back(doubleField("radius", d->radius));
        fields.push_back(doubleField("turns", d->turns));
        fields.push_back(doubleField("turnHeight", d->turnHeight));
        fields.push_back(boolField("handedness", d->handedness));
        fields.push_back(intField("constraintType", d->constraintType));
        addEntityRecord("addHelix", "HELIX", fields, *d, false);
    }
    void addMesh(const DRW_Mesh& d) override {
        std::vector<Field> fields = entityFields(d, "MESH", "MESH");
        fields.push_back(intField("version", d.version));
        fields.push_back(boolField("blendCrease", d.blendCrease));
        fields.push_back(intField("subdivisionLevel", d.subdivisionLevel));
        fields.push_back(intField("unknown", d.unknown));
        fields.push_back(uintField("legacySubdivVertexCount", d.subdivVertices.size()));
        for (std::size_t i = 0; i < d.subdivVertices.size(); ++i)
            fields.push_back(pointField("legacySubdivVertex." + std::to_string(i),
                                        d.subdivVertices[i]));
        fields.push_back(uintField("vertexCount", d.vertices.size()));
        for (std::size_t i = 0; i < d.vertices.size(); ++i)
            fields.push_back(pointField("vertex." + std::to_string(i), d.vertices[i]));
        fields.push_back(uintField("faceCount", d.faces.size()));
        for (std::size_t i = 0; i < d.faces.size(); ++i) {
            const std::string facePrefix = "face." + std::to_string(i);
            fields.push_back(uintField(facePrefix + ".vertexCount", d.faces[i].size()));
            for (std::size_t j = 0; j < d.faces[i].size(); ++j)
                fields.push_back(intField(facePrefix + ".vertex." +
                                          std::to_string(j), d.faces[i][j]));
        }
        fields.push_back(uintField("edgeCount", d.edges.size()));
        for (std::size_t i = 0; i < d.edges.size(); ++i) {
            const std::string edgePrefix = "edge." + std::to_string(i);
            fields.push_back(intField(edgePrefix + ".start", d.edges[i].first));
            fields.push_back(intField(edgePrefix + ".end", d.edges[i].second));
        }
        fields.push_back(uintField("creaseCount", d.creases.size()));
        for (std::size_t i = 0; i < d.creases.size(); ++i)
            fields.push_back(doubleField("crease." + std::to_string(i), d.creases[i]));
        fields.push_back(uintField("propertyOverrideCount", d.propertyOverrides.size()));
        for (std::size_t i = 0; i < d.propertyOverrides.size(); ++i) {
            const std::string prefix = "propertyOverride." + std::to_string(i);
            fields.push_back(intField(prefix + ".subEntityMarker",
                                      d.propertyOverrides[i].subEntityMarker));
            fields.push_back(uintField(prefix + ".propertyTypeCount",
                                      d.propertyOverrides[i].propertyTypes.size()));
            for (std::size_t j = 0; j < d.propertyOverrides[i].propertyTypes.size(); ++j)
                fields.push_back(intField(prefix + ".propertyType." +
                    std::to_string(j), d.propertyOverrides[i].propertyTypes[j]));
        }
        addEntityRecord("addMesh", "MESH", fields, d, false);
    }
    void addKnot(const DRW_Entity& d) override { addBasicEntity("addKnot", "KNOT", d); }
    void addInsert(const DRW_Insert& d) override {
        std::vector<Field> fields = entityFields(d, "INSERT", "INSERT");
        fields.push_back(stringField("blockName", d.name));
        fields.push_back(pointField("insertionPoint", d.basePoint));
        fields.push_back(pointField("extrusion", d.extPoint));
        fields.push_back(doubleField("xScale", d.xscale));
        fields.push_back(doubleField("yScale", d.yscale));
        fields.push_back(doubleField("zScale", d.zscale));
        fields.push_back(doubleField("rotationAngle", d.angle));
        fields.push_back(intField("columnCount", d.colcount));
        fields.push_back(intField("rowCount", d.rowcount));
        fields.push_back(doubleField("columnSpacing", d.colspace));
        fields.push_back(doubleField("rowSpacing", d.rowspace));
        fields.push_back(uintField("attributeCount", d.attlist.size()));
        fields.push_back(boolField("hasSequenceEnd",
                                   static_cast<bool>(d.seqEndEntity)));
        for (std::size_t i = 0; i < d.attlist.size(); ++i) {
            const std::string prefix = "attribute." + std::to_string(i);
            if (!d.attlist[i]) {
                fields.push_back(boolField(prefix + ".present", false));
                continue;
            }
            fields.push_back(boolField(prefix + ".present", true));
            if (d.attlist[i]->handle != DRW::NoHandle)
                fields.push_back(handleField(prefix + ".handle", d.attlist[i]->handle));
            fields.push_back(stringField(prefix + ".tag", d.attlist[i]->tag));
            fields.push_back(stringField(prefix + ".text", d.attlist[i]->text));
            fields.push_back(pointField(prefix + ".insertionPoint", d.attlist[i]->basePoint));
            fields.push_back(pointField(prefix + ".extrusion", d.attlist[i]->extPoint));
            fields.push_back(doubleField(prefix + ".height", d.attlist[i]->height));
            fields.push_back(doubleField(prefix + ".rotationAngle", d.attlist[i]->angle));
            fields.push_back(doubleField(prefix + ".widthScale", d.attlist[i]->widthscale));
            fields.push_back(doubleField(prefix + ".oblique", d.attlist[i]->oblique));
            fields.push_back(intField(prefix + ".textGeneration", d.attlist[i]->textgen));
            fields.push_back(intField(prefix + ".horizontalAlignment",
                                      static_cast<int>(d.attlist[i]->alignH)));
            fields.push_back(intField(prefix + ".verticalAlignment",
                                      static_cast<int>(d.attlist[i]->alignV)));
            fields.push_back(stringField(prefix + ".style", d.attlist[i]->style));
            fields.push_back(intField(prefix + ".flags", d.attlist[i]->attribFlags));
            fields.push_back(intField(prefix + ".fieldLength", d.attlist[i]->m_fieldLength));
            fields.push_back(boolField(prefix + ".lockPosition", d.attlist[i]->lockPosition));
            fields.push_back(intField(prefix + ".attributeVersion", d.attlist[i]->attVersion));
            fields.push_back(intField(prefix + ".attributeType",
                                      d.attlist[i]->m_attributeType));
            fields.push_back(boolField(prefix + ".hasMText",
                                      static_cast<bool>(d.attlist[i]->mtext)));
            if (d.attlist[i]->mtext) {
                fields.push_back(stringField(prefix + ".mtext.text",
                                             d.attlist[i]->mtext->text));
                fields.push_back(doubleField(prefix + ".mtext.height",
                                             d.attlist[i]->mtext->height));
                fields.push_back(pointField(prefix + ".mtext.insertionPoint",
                                            d.attlist[i]->mtext->basePoint));
                fields.push_back(doubleField(prefix + ".mtext.rotationAngle",
                                             d.attlist[i]->mtext->angle));
            }
        }
        addEntityRecord("addInsert", "INSERT", fields, d, false);
    }
    SEMANTIC_OPAQUE_VALUE(addTable, DRW_Table, "TABLE")
    void addTrace(const DRW_Trace& d) override {
        std::vector<Field> fields = entityFields(d, "TRACE", "TRACE");
        fields.push_back(pointField("corner.0", d.basePoint));
        fields.push_back(pointField("corner.1", d.secPoint));
        fields.push_back(pointField("corner.2", d.thirdPoint));
        fields.push_back(pointField("corner.3", d.fourPoint));
        fields.push_back(doubleField("thickness", d.thickness));
        fields.push_back(pointField("extrusion", d.extPoint));
        addEntityRecord("addTrace", "TRACE", fields, d, false);
    }
    void add3dFace(const DRW_3Dface& d) override {
        std::vector<Field> fields = entityFields(d, "3DFACE", "3DFACE");
        fields.push_back(pointField("corner.0", d.basePoint));
        fields.push_back(pointField("corner.1", d.secPoint));
        fields.push_back(pointField("corner.2", d.thirdPoint));
        fields.push_back(pointField("corner.3", d.fourPoint));
        fields.push_back(intField("invisibleEdgeFlags", d.invisibleflag));
        fields.push_back(doubleField("thickness", d.thickness));
        fields.push_back(pointField("extrusion", d.extPoint));
        addEntityRecord("add3dFace", "3DFACE", fields, d, false);
    }
    void addModelerGeometry(const DRW_ModelerGeometry& d) override {
        std::vector<Field> fields = entityFields(d, "MODELER_GEOMETRY",
                                                 "MODELER_GEOMETRY");
        std::vector<std::string> excludedFields;
        fields.push_back(intField("entityType", static_cast<int>(d.eType)));
        fields.push_back(intField("modelerVersion", d.m_modelerVersion));
        if (!appendModelerDwgSourceVersionField(fields, d))
            excludedFields.push_back("dwgSourceVersion");
        fields.push_back(uintField("bodyBitSize", d.m_bodyBitSize));
        fields.push_back(uintField("objectSize", d.m_objectSize));
        fields.push_back(boolField("isEmpty", d.m_isEmpty));
        fields.push_back(boolField("hasModelerData", d.m_hasModelerData));
        if (!appendModelerPayloadStatusField(fields, d))
            excludedFields.push_back("modelerPayloadStatus");
        fields.push_back(boolField("modelerDataUnknownBit", d.m_modelerDataUnknownBit));
        fields.push_back(boolField("hasWireframe", d.m_hasWireframe));
        fields.push_back(handleField("historyHandle", d.m_historyHandle));
        fields.push_back(boolField("hasRawBytes", !d.m_rawBytes.empty()));
        fields.push_back(uintField("rawByteCount", d.m_rawBytes.size()));
        if (!appendModelerDwgAcisPayloadFields(fields, d))
            excludedFields.push_back("dwgInlineAcisPayload");
        if (!appendModelerDxfPayloadChunkFields(fields, d, d.m_rawBytes))
            excludedFields.push_back("dxfPayloadChunks");
        appendDataStorageFields(fields, d);
        fields.push_back(stringField("rawByteCarrierKind",
            facade_ == "dwgRW" && direction_ == "read"
                ? "dwgObjectFrameBody-unclassified-payload"
                : facade_ == "dxfRW"
                    ? "dxfEntityInlineModelerPayload"
                    : "unknown"));
        fields.push_back(boolField("derivedWireframeDecoded", d.m_wireframeDecoded));
        fields.push_back(uintField("derivedWireframeVertexCount", d.m_wireframe.vertices.size()));
        fields.push_back(uintField("derivedWireframeEdgeCount", d.m_wireframe.edges.size()));
        fields.push_back(uintField("derivedWireframeFaceCount", d.m_wireframe.faces.size()));
        fields.push_back(uintField("payloadRangeCount", d.m_payloadRanges.size()));
        for (std::size_t i = 0; i < d.m_payloadRanges.size(); ++i) {
            const std::string prefix = "payloadRange." + std::to_string(i);
            const DRW_ModelerPayloadRange& range = d.m_payloadRanges[i];
            fields.push_back(intField(prefix + ".kind", static_cast<int>(range.m_kind)));
            fields.push_back(intField(prefix + ".section", static_cast<int>(range.m_section)));
            fields.push_back(uintField(prefix + ".offset", range.m_offset));
            if (!appendPayloadRangeBitOffset(fields, prefix, range))
                excludedFields.push_back(prefix + ".bitOffset");
            fields.push_back(uintField(prefix + ".length", range.m_length));
            fields.push_back(uintField(prefix + ".declaredByteSize", range.m_declaredByteSize));
            fields.push_back(intField(prefix + ".consistency", static_cast<int>(range.m_consistency)));
            fields.push_back(intField(prefix + ".confidence", static_cast<int>(range.m_confidence)));
            fields.push_back(stringField(prefix + ".markerText", range.m_markerText));
        }
        const std::size_t recordIndex = addEntityRecord(
            "addModelerGeometry", "MODELER_GEOMETRY", fields, d, false);
        for (const std::string& source : excludedFields)
            addExcluded(records[recordIndex].id, source);
        if (d.m_historyHandle != 0) {
            addEdge("historyReference", "node:" + records[recordIndex].id,
                    ensureHandleNode(d.m_historyHandle));
        }
        if (!d.m_rawBytes.empty()) {
            const std::string carrierName = facade_ == "dwgRW" && direction_ == "read"
                ? "dwgModelerObjectFrameBody-unclassified-payload"
                : facade_ == "dxfRW"
                    ? "dxfModelerEntityInlinePayload"
                    : "unclassifiedModelerBytes";
            attachCarrierToRecord(recordIndex, carrierName,
                                  d.m_rawBytes.data(), d.m_rawBytes.size());
        }
        attachModelerDwgAcisPayloadCarrier(recordIndex, d);
    }
    SEMANTIC_OPAQUE_VALUE(addLight, DRW_Light, "LIGHT")
    SEMANTIC_OPAQUE_VALUE(addCamera, DRW_Camera, "CAMERA")
    SEMANTIC_OPAQUE_VALUE(addGeoPositionMarker, DRW_GeoPositionMarker, "GEOPOSITIONMARKER")
    SEMANTIC_OPAQUE_VALUE(addSectionObject, DRW_SectionObject, "SECTIONOBJECT")
    void addSolid(const DRW_Solid& d) override {
        std::vector<Field> fields = entityFields(d, "SOLID", "SOLID");
        fields.push_back(pointField("corner.0", d.basePoint));
        fields.push_back(pointField("corner.1", d.secPoint));
        fields.push_back(pointField("corner.2", d.thirdPoint));
        fields.push_back(pointField("corner.3", d.fourPoint));
        fields.push_back(doubleField("thickness", d.thickness));
        fields.push_back(pointField("extrusion", d.extPoint));
        addEntityRecord("addSolid", "SOLID", fields, d, false);
    }
    void addMText(const DRW_MText& d) override {
        std::vector<Field> fields = entityFields(d, "MTEXT", "MTEXT");
        fields.push_back(stringField("text", d.text));
        fields.push_back(pointField("insertion", d.basePoint));
        fields.push_back(doubleField("height", d.height));
        fields.push_back(stringField("style", d.style));
        addEntityRecord("addMText", "MTEXT", fields, d, true);
    }
    void addText(const DRW_Text& d) override {
        if (const auto* value = dynamic_cast<const DRW_RText*>(&d)) {
            std::vector<Field> fields = entityFields(*value, "RTEXT", "RTEXT");
            fields.push_back(stringField("text", value->text));
            fields.push_back(intField("flags", value->m_rTextFlags));
            fields.push_back(pointField("insertion", value->basePoint));
            fields.push_back(pointField("extrusion", value->extPoint));
            fields.push_back(doubleField("rotation", value->angle));
            fields.push_back(doubleField("height", value->height));
            fields.push_back(stringField("style", value->style));
            addEntityRecord("addText", "RTEXT", fields, *value, false);
            return;
        }
        if (const auto* value = dynamic_cast<const DRW_ArcAlignedText*>(&d)) {
            std::vector<Field> fields = entityFields(*value, "ARCALIGNEDTEXT", "ARCALIGNEDTEXT");
            fields.push_back(stringField("text", value->text));
            fields.push_back(pointField("center", value->m_center));
            fields.push_back(doubleField("radius", value->m_radius));
            fields.push_back(doubleField("startAngle", value->m_startAngle));
            fields.push_back(doubleField("endAngle", value->m_endAngle));
            fields.push_back(stringField("textSize", value->m_textSize));
            fields.push_back(stringField("xScale", value->m_xScale));
            fields.push_back(stringField("characterSpacing", value->m_charSpacing));
            fields.push_back(stringField("offsetFromArc", value->m_offsetFromArc));
            fields.push_back(stringField("rightOffset", value->m_rightOffset));
            fields.push_back(stringField("leftOffset", value->m_leftOffset));
            fields.push_back(stringField("fontName", value->m_fontName));
            fields.push_back(stringField("bigFontName", value->m_bigFontName));
            fields.push_back(intField("rawColor", value->m_rawColor));
            fields.push_back(handleField("arcHandle", value->m_arcHandle));
            addEntityRecord("addText", "ARCALIGNEDTEXT", fields, *value, true);
            return;
        }
        std::vector<Field> fields = entityFields(d, "TEXT", "TEXT");
        fields.push_back(stringField("text", d.text));
        fields.push_back(pointField("insertion", d.basePoint));
        fields.push_back(doubleField("height", d.height));
        fields.push_back(doubleField("rotation", d.angle));
        fields.push_back(stringField("style", d.style));
        addEntityRecord("addText", "TEXT", fields, d, false);
    }
    SEMANTIC_OPAQUE_VALUE(addAttDef, DRW_Attdef, "ATTDEF")
    void addDimAlign(const DRW_DimAligned* d) override { addDimension("addDimAlign", "DIMALIGNED", d); }
    void addDimLinear(const DRW_DimLinear* d) override { addDimension("addDimLinear", "DIMLINEAR", d); }
    void addDimRadial(const DRW_DimRadial* d) override {
        if (d == nullptr) { addNullDiagnostic("addDimRadial"); return; }
        if (const auto* value = dynamic_cast<const DRW_DimLargeRadial*>(d)) {
            std::vector<Field> fields = dimensionFields(*value, "LARGE_RADIAL");
            fields.push_back(pointField("center", value->getCenterPoint()));
            fields.push_back(pointField("chord", value->getChordPoint()));
            fields.push_back(pointField("overrideCenter", value->overrideCenterPoint));
            fields.push_back(pointField("jogPoint", value->jogPoint));
            fields.push_back(doubleField("jogAngle", value->jogAngle));
            fields.push_back(doubleField("leaderLength", value->getLeaderLength()));
            addEntityRecord("addDimRadial", "LARGE_RADIAL", fields, *value, false);
            return;
        }
        addDimension("addDimRadial", "DIMRADIAL", d);
    }
    void addDimDiametric(const DRW_DimDiametric* d) override { addDimension("addDimDiametric", "DIMDIAMETRIC", d); }
    void addDimAngular(const DRW_DimAngular* d) override { addDimension("addDimAngular", "DIMANGULAR", d); }
    void addDimAngular3P(const DRW_DimAngular3p* d) override { addDimension("addDimAngular3P", "DIMANGULAR3P", d); }
    void addDimOrdinate(const DRW_DimOrdinate* d) override { addDimension("addDimOrdinate", "DIMORDINATE", d); }
    void addDimArc(const DRW_DimArc* d) override { addDimension("addDimArc", "DIMARC", d); }
    void addLeader(const DRW_Leader* d) override { addPointerEntity("addLeader", "LEADER", d); }
    void addHatch(const DRW_Hatch* d) override {
        if (d == nullptr) { addNullDiagnostic("addHatch"); return; }
        if (!threeDConsumerProbe_) {
            addPointerEntity("addHatch", "HATCH", d);
            return;
        }
        std::vector<Field> fields = entityFields(*d, "HATCH", "HATCH");
        fields.push_back(stringField("name", d->name));
        fields.push_back(intField("solid", d->solid));
        fields.push_back(intField("associative", d->associative));
        fields.push_back(doubleField("elevation", d->basePoint.z));
        fields.push_back(pointField("extrusion", d->extPoint));
        fields.push_back(uintField("loopCount", d->looplist.size()));
        for (std::size_t i = 0; i < d->looplist.size(); ++i) {
            const auto& loop = d->looplist[i];
            if (!loop) {
                fields.push_back(boolField("loop." + std::to_string(i)
                                               + ".present", false));
                continue;
            }
            const std::string prefix = "loop." + std::to_string(i) + ".";
            fields.push_back(boolField(prefix + "present", true));
            fields.push_back(intField(prefix + "type", loop->type));
            fields.push_back(uintField(prefix + "objectCount",
                                       loop->objlist.size()));
            for (std::size_t j = 0; j < loop->objlist.size(); ++j) {
                const auto* polyline = dynamic_cast<const DRW_LWPolyline*>(
                    loop->objlist[j].get());
                if (polyline == nullptr)
                    continue;
                fields.push_back(uintField(prefix + "vertexCount",
                                           polyline->vertlist.size()));
                fields.push_back(boolField(prefix + "closed",
                                           (polyline->flags & 1) != 0));
                for (std::size_t k = 0; k < polyline->vertlist.size(); ++k) {
                    const auto& vertex = polyline->vertlist[k];
                    if (!vertex)
                        continue;
                    const std::string vertexPrefix = prefix + "vertex."
                        + std::to_string(k) + ".";
                    const DRW_Coord ocs{vertex->x, vertex->y, 0.0};
                    fields.push_back(pointField(vertexPrefix + "ocs", ocs));
                    fields.push_back(pointField(vertexPrefix + "wcs",
                                                d->boundaryPointToWcs(ocs)));
                }
            }
        }
        addEntityRecord("addHatch", "HATCH", std::move(fields), *d, false);
    }
    void addMPolygon(const DRW_MPolygon* d) override {
        if (d == nullptr) { addNullDiagnostic("addMPolygon"); return; }
        std::vector<Field> fields = entityFields(*d, "MPOLYGON", "MPOLYGON");
        fields.push_back(boolField("solid", d->solid != 0));
        fields.push_back(boolField("associative", d->associative != 0));
        fields.push_back(stringField("name", d->name));
        fields.push_back(intField("hpattern", d->hpattern));
        fields.push_back(doubleField("basePoint.z", d->basePoint.z));
        fields.push_back(doubleField("extPoint.x", d->extPoint.x));
        fields.push_back(doubleField("extPoint.y", d->extPoint.y));
        fields.push_back(doubleField("extPoint.z", d->extPoint.z));
        fields.push_back(intField("fillColorAci", d->fillColorAci));
        fields.push_back(intField("fillColorRgb", d->fillColorRgb));
        fields.push_back(stringField("fillColorName", d->fillColorName));
        fields.push_back(intField("loopCount", d->loopsnum));
        if (threeDConsumerProbe_) {
            fields.push_back(doubleField("elevation", d->basePoint.z));
            fields.push_back(pointField("extrusion", d->extPoint));
            for (std::size_t i = 0; i < d->looplist.size(); ++i) {
                const auto& loop = d->looplist[i];
                if (!loop)
                    continue;
                const std::string prefix = "loop." + std::to_string(i) + ".";
                fields.push_back(intField(prefix + "type", loop->type));
                fields.push_back(uintField(prefix + "objectCount",
                                           loop->objlist.size()));
                for (const auto& boundary : loop->objlist) {
                    const auto* polyline =
                        dynamic_cast<const DRW_LWPolyline*>(boundary.get());
                    if (polyline == nullptr)
                        continue;
                    fields.push_back(uintField(prefix + "vertexCount",
                                               polyline->vertlist.size()));
                    fields.push_back(boolField(prefix + "closed",
                                               (polyline->flags & 1) != 0));
                    for (std::size_t j = 0; j < polyline->vertlist.size(); ++j) {
                        const auto& vertex = polyline->vertlist[j];
                        if (!vertex)
                            continue;
                        const std::string vertexPrefix = prefix + "vertex."
                            + std::to_string(j) + ".";
                        const DRW_Coord ocs{vertex->x, vertex->y, 0.0};
                        fields.push_back(pointField(vertexPrefix + "ocs", ocs));
                        fields.push_back(pointField(vertexPrefix + "wcs",
                            d->boundaryPointToWcs(ocs)));
                    }
                }
            }
        }
        addEntityRecord("addMPolygon", "MPOLYGON", fields, *d, false);
    }
    void addViewport(const DRW_Viewport& d) override { addBasicEntity("addViewport", "VIEWPORT", d); }
    void addImage(const DRW_Image* d) override { addPointerEntity("addImage", "IMAGE", d); }
    void linkImage(const DRW_ImageDef* d) override {
        if (d == nullptr) { addNullDiagnostic("linkImage"); return; }
        addTable("linkImage", "IMAGEDEF", *d);
    }
    SEMANTIC_OPAQUE_POINTER(addWipeout, DRW_Wipeout, "WIPEOUT")
    SEMANTIC_OPAQUE_POINTER(addPointCloud, DRW_PointCloud, "POINTCLOUD")
    SEMANTIC_OPAQUE_POINTER(addPointCloudEx, DRW_PointCloudEx, "POINTCLOUDEX")
    SEMANTIC_OPAQUE_POINTER(addNavisworksModel, DRW_NavisworksModel, "NAVISWORKSMODEL")
    void addSurface(const DRW_Surface* d) override {
        if (d == nullptr) { addNullDiagnostic("addSurface"); return; }
        std::string entity = "SURFACE";
        switch (d->eType) {
        case DRW::PLANESURFACE: entity = "PLANESURFACE"; break;
        case DRW::EXTRUDEDSURFACE: entity = "EXTRUDEDSURFACE"; break;
        case DRW::REVOLVEDSURFACE: entity = "REVOLVEDSURFACE"; break;
        case DRW::SWEPTSURFACE: entity = "SWEPTSURFACE"; break;
        case DRW::LOFTEDSURFACE: entity = "LOFTEDSURFACE"; break;
        case DRW::NURBSURFACE: entity = "NURBSURFACE"; break;
        default: break;
        }
        std::vector<Field> fields = entityFields(*d, entity, entity);
        std::vector<std::string> excludedFields;
        appendSurfaceFields(fields, *d, excludedFields);
        const std::size_t recordIndex = addEntityRecord(
            "addSurface", entity, fields, *d, false);
        for (const std::string& source : excludedFields)
            addExcluded(records[recordIndex].id, source);
        if (!d->rawAcisData.empty()) {
            const std::string carrierName = facade_ == "dwgRW" && direction_ == "read"
                ? "dwgSurfaceReaderPayload-unclassified"
                : facade_ == "dxfRW"
                    ? "dxfSurfaceEntityInlineAcis"
                    : "unclassifiedSurfaceBytes";
            attachCarrierToRecord(recordIndex, carrierName,
                                  d->rawAcisData.data(), d->rawAcisData.size());
        }
        if (const auto* swept = dynamic_cast<const DRW_SweptSurface*>(d)) {
            if (!swept->sweepData.empty())
                attachCarrierToRecord(recordIndex, "surfaceSweepData",
                    swept->sweepData.data(), swept->sweepData.size());
            if (!swept->pathData.empty())
                attachCarrierToRecord(recordIndex, "surfacePathData",
                    swept->pathData.data(), swept->pathData.size());
        }
        if (const auto* lofted = dynamic_cast<const DRW_LoftedSurface*>(d)) {
            for (std::size_t i = 0; i < lofted->dxfReferenceData.size(); ++i) {
                const DRW_Variant& value = lofted->dxfReferenceData[i];
                if (value.type() == DRW_Variant::BINARY && value.binary() != nullptr &&
                    !value.binary()->empty()) {
                    const std::string carrierName = "loftDxfReference." +
                        std::to_string(i) + ".group" + std::to_string(value.code());
                    attachCarrierToRecord(recordIndex, carrierName,
                        value.binary()->data(), value.binary()->size());
                }
            }
        }
    }
    SEMANTIC_OPAQUE_POINTER(addMLeader, DRW_MLeader, "MULTILEADER")
    SEMANTIC_OPAQUE_POINTER(addMLeaderStyle, DRW_MLeaderStyle, "MLEADERSTYLE")
    SEMANTIC_OPAQUE_VALUE(addDbColor, DRW_DbColor, "DBCOLOR")
    SEMANTIC_OPAQUE_VALUE(addVisualStyle, DRW_VisualStyle, "VISUALSTYLE")
    SEMANTIC_OPAQUE_VALUE(addScale, DRW_Scale, "SCALE")
    SEMANTIC_OPAQUE_VALUE(addDimensionAssociation, DRW_DimensionAssociation, "DIMASSOC")
    SEMANTIC_OPAQUE_VALUE(addEvaluationGraph, DRW_EvaluationGraph, "EVALUATION_GRAPH")
    SEMANTIC_OPAQUE_VALUE(addBlockRepresentationData, DRW_BlockRepresentationData, "BLOCKREPRESENTATIONDATA")
    SEMANTIC_OPAQUE_VALUE(addDetailViewStyle, DRW_DetailViewStyle, "DETAILVIEWSTYLE")
    SEMANTIC_OPAQUE_VALUE(addSectionViewStyle, DRW_SectionViewStyle, "SECTIONVIEWSTYLE")
    SEMANTIC_OPAQUE_VALUE(addBreakData, DRW_BreakData, "BREAKDATA")
    SEMANTIC_OPAQUE_VALUE(addBreakPointRef, DRW_BreakPointRef, "BREAKPOINTREF")
    SEMANTIC_OPAQUE_VALUE(addGroup, DRW_Group, "GROUP")
    SEMANTIC_OPAQUE_VALUE(addImageDefinitionReactor, DRW_ImageDefinitionReactor, "IMAGEDEF_REACTOR")
    SEMANTIC_OPAQUE_VALUE(addSpatialFilter, DRW_SpatialFilter, "SPATIAL_FILTER")
    SEMANTIC_OPAQUE_VALUE(addGeoData, DRW_GeoData, "GEODATA")
    SEMANTIC_OPAQUE_VALUE(addTableGeometry, DRW_TableGeometry, "TABLEGEOMETRY")
    SEMANTIC_OPAQUE_VALUE(addAcDbPlaceholder, DRW_AcDbPlaceholder, "ACDBPLACEHOLDER")
    SEMANTIC_OPAQUE_VALUE(addVbaProject, DRW_VbaProject, "VBA_PROJECT")
    SEMANTIC_OPAQUE_VALUE(addProxyObject, DRW_ProxyObject, "PROXY_OBJECT")
    SEMANTIC_OPAQUE_VALUE(addSun, DRW_Sun, "SUN")
    SEMANTIC_OPAQUE_VALUE(addLightList, DRW_LightList, "LIGHTLIST")
    SEMANTIC_OPAQUE_VALUE(addDataLink, DRW_DataLink, "DATALINK")
    SEMANTIC_OPAQUE_VALUE(addGeoMapImage, DRW_GeoMapImage, "GEOMAPIMAGE")
    SEMANTIC_OPAQUE_VALUE(addBackground, DRW_Background, "BACKGROUND")
    SEMANTIC_OPAQUE_VALUE(addPointCloudDef, DRW_PointCloudDef, "POINTCLOUDDEF")
    SEMANTIC_OPAQUE_VALUE(addNavisworksModelDef, DRW_NavisworksModelDef, "NAVISWORKSMODELDEF")
    SEMANTIC_OPAQUE_VALUE(addPointCloudColorMap, DRW_PointCloudColorMap, "POINTCLOUDCOLORMAP")
    SEMANTIC_OPAQUE_VALUE(addSunStudy, DRW_SunStudy, "SUNSTUDY")
    SEMANTIC_OPAQUE_VALUE(addLayerFilter, DRW_LayerFilter, "LAYERFILTER")
    SEMANTIC_OPAQUE_VALUE(addMotionPath, DRW_MotionPath, "MOTIONPATH")
    SEMANTIC_OPAQUE_VALUE(addCurvePath, DRW_CurvePath, "CURVEPATH")
    SEMANTIC_OPAQUE_VALUE(addPointPath, DRW_PointPath, "POINTPATH")
    SEMANTIC_OPAQUE_VALUE(addObjectPtr, DRW_ObjectPtr, "OBJECT_PTR")
    SEMANTIC_OPAQUE_VALUE(addPartialViewingIndex, DRW_PartialViewingIndex, "PARTIAL_VIEWING_INDEX")
    SEMANTIC_OPAQUE_VALUE(addRenderSettings, DRW_RenderSettings, "RENDER_SETTINGS")
    SEMANTIC_OPAQUE_VALUE(addSection, DRW_Section, "SECTION")
    SEMANTIC_OPAQUE_VALUE(addAssociativeObject, DRW_AssociativeObject, "ASSOCIATIVE_OBJECT")
    SEMANTIC_OPAQUE_VALUE(addAcShHistoryObject, DRW_AcShHistoryObject, "ACSH_HISTORY_OBJECT")
    SEMANTIC_OPAQUE_VALUE(addIDBuffer, DRW_IDBuffer, "IDBUFFER")
    SEMANTIC_OPAQUE_VALUE(addIndex, DRW_Index, "INDEX")
    SEMANTIC_OPAQUE_VALUE(addLayerIndex, DRW_LayerIndex, "LAYER_INDEX")
    SEMANTIC_OPAQUE_VALUE(addSpatialIndex, DRW_SpatialIndex, "SPATIAL_INDEX")

#undef SEMANTIC_OPAQUE_POINTER
#undef SEMANTIC_OPAQUE_VALUE

    void addUnsupportedObject(const DRW_UnsupportedObject& d) override {
        const std::string recordClass = !d.m_recordName.empty()
            ? d.m_recordName
            : (!d.m_className.empty() ? d.m_className
                                      : "DWG_TYPE_" + std::to_string(d.m_objectType));
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", recordClass));
        fields.push_back(stringField("entity", recordClass));
        fields.push_back(intField("objectType", d.m_objectType));
        appendIdentityHandleFields(fields, d.m_handle, d.m_parentHandle);
        fields.push_back(boolField("isEntity", d.m_isEntity));
        fields.push_back(uintField("rawByteCount", d.m_rawBytes.size()));
        const std::size_t recordIndex = addRecord("addUnsupportedObject", "unsupported", recordClass, fields);
        addOwnerEdges(records[recordIndex], d.m_handle, d.m_parentHandle);
        const std::size_t callbackIndex = records[recordIndex].callbackOrdinal;
        const std::string carrierId = addCarrier("dwgUnsupportedObject", d.m_rawBytes.data(),
                                                 d.m_rawBytes.size(), "preserved",
                                                 records[recordIndex].id);
        callbacks[callbackIndex].carrierIds.push_back(carrierId);
        addUnsupported("opaqueObject", "dwg", records[recordIndex].id,
                       carrierId, std::string(), "preserved");
    }
    void addRawDwgSection(const DRW_RawDwgSection& d) override {
        const std::size_t callbackIndex = addCallback("addRawDwgSection");
        const std::string carrierId = addCarrier("rawDwgSection:" + d.m_name,
                                                 d.m_data.data(), d.m_data.size(),
                                                 "preserved", std::string());
        callbacks[callbackIndex].carrierIds.push_back(carrierId);
        addUnsupported("opaqueSection", "dwg", std::string(), carrierId,
                       std::string(), "preserved");
    }
    void addRawDxfObject(const DRW_RawDxfObject& d) override { addRawDxf("addRawDxfObject", "rawDxfObject", d); }
    void addRawDxfEntity(const DRW_RawDxfObject& d) override { addRawDxf("addRawDxfEntity", "rawDxfEntity", d); }
    void addRawDxfSection(const DRW_RawDxfSection& d) override {
        const std::string bytes = serializeVariants(d.m_groups);
        const std::size_t callbackIndex = addCallback("addRawDxfSection");
        const std::string carrierId = addCarrier("rawDxfSection:" + d.m_name,
                                                 bytes.data(), bytes.size(),
                                                 "preservedCanonical", std::string());
        callbacks[callbackIndex].carrierIds.push_back(carrierId);
        addUnsupported("opaqueSection", "dxf", std::string(), carrierId,
                       std::string(), "preservedCanonical");
    }

    void addComment(const char* comment) override {
        const std::size_t callbackIndex = addCallback("addComment");
        if (comment != nullptr) {
            const std::string value(comment);
            callbacks[callbackIndex].carrierIds.push_back(
                addCarrier("dxfComment", value.data(), value.size(),
                           "observed", std::string()));
        }
    }
    void addPlotSettings(const DRW_PlotSettings* d) override {
        if (d == nullptr) { addNullDiagnostic("addPlotSettings"); return; }
        addTable("addPlotSettings", "PLOTSETTINGS", *d);
    }

    void writeHeader(DRW_Header& header) override {
        if (partialDwgReadbackControl_ || opaqueDwgReadbackControl_)
            header.vars.clear();
        addCallback("writeHeader");
    }
    void writeDwgClasses() override {
        addCallback("writeDwgClasses");
        if (opaqueDwgReadbackControl_) {
            if (dwgWriter_ == nullptr) {
                writeRecipeSucceeded_ = false;
                return;
            }
            writeRecipeSucceeded_ = dwgWriter_->registerRawDwgObjectClass(
                &opaqueDwgReadbackObject_) && writeRecipeSucceeded_;
        }
    }
    void collectDwgAppIds() override { addCallback("collectDwgAppIds"); }
    void writeBlocks() override {
        addCallback("writeBlocks");
        if (!threeDConsumerProbe_ || dxfWriter_ == nullptr)
            return;

        DRW_Block childBlock;
        childBlock.name = "PROBE_NESTED_CHILD";
        childBlock.basePoint = DRW_Coord{1.0, -2.0, 0.5};
        bool wrote = dxfWriter_->writeBlock(&childBlock);

        DRW_Line childGeometry;
        childGeometry.layer = "probe_nested_geometry";
        childGeometry.basePoint = DRW_Coord{1.0, 2.0, 3.0};
        childGeometry.secPoint = DRW_Coord{4.0, -1.0, 2.0};
        wrote = dxfWriter_->writeLine(&childGeometry) && wrote;

        DRW_Block parentBlock;
        parentBlock.name = "PROBE_NESTED_PARENT";
        parentBlock.basePoint = DRW_Coord{-3.0, 2.0, 1.0};
        wrote = dxfWriter_->writeBlock(&parentBlock) && wrote;

        DRW_Insert nestedArray;
        nestedArray.layer = "probe_nested_inner_insert";
        nestedArray.name = "PROBE_NESTED_CHILD";
        nestedArray.basePoint = DRW_Coord{4.0, 5.0, 6.0};
        nestedArray.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        nestedArray.xscale = 1.5;
        nestedArray.yscale = 0.5;
        nestedArray.zscale = 2.0;
        nestedArray.angle = 0.3;
        nestedArray.colcount = 2;
        nestedArray.rowcount = 2;
        nestedArray.colspace = 7.0;
        nestedArray.rowspace = 11.0;
        wrote = dxfWriter_->writeInsert(&nestedArray) && wrote;
        writeRecipeSucceeded_ = wrote && writeRecipeSucceeded_;
    }
    void writeBlockRecords() override {
        addCallback("writeBlockRecords");
        if (!threeDConsumerProbe_ || dxfWriter_ == nullptr)
            return;
        bool wrote = dxfWriter_->writeBlockRecord("PROBE_NESTED_CHILD", 0);
        wrote = dxfWriter_->writeBlockRecord("PROBE_NESTED_PARENT", 0)
            && wrote;
        writeRecipeSucceeded_ = wrote && writeRecipeSucceeded_;
    }
    void writeEntities() override;
    void writeLTypes() override { addCallback("writeLTypes"); }
    void writeLayers() override { addCallback("writeLayers"); }
    void writeTextstyles() override { addCallback("writeTextstyles"); }
    void writeVports() override { addCallback("writeVports"); }
    void writeDimstyles() override { addCallback("writeDimstyles"); }
    void writeObjects() override {
        addCallback("writeObjects");
        if (opaqueDwgReadbackControl_) {
            if (dwgWriter_ == nullptr) {
                writeRecipeSucceeded_ = false;
                return;
            }
            writeRecipeSucceeded_ = dwgWriter_->writeRawDwgObject(
                &opaqueDwgReadbackObject_) && writeRecipeSucceeded_;
        }
    }
    bool finalizeDwgWrite() override {
        addCallback("finalizeDwgWrite");
        return true;
    }
    bool finalizeDwgWriteStructure() override {
        addCallback("finalizeDwgWriteStructure");
        return true;
    }
#ifdef LIBDXFRW_SEMANTIC_SIDE_STANDALONE
    bool finalizeDxfWrite() override {
        addCallback("finalizeDxfWrite");
        return true;
    }
    void addDwgTemplateDescription(const std::string&) override {
        addCallback("addDwgTemplateDescription");
    }
    bool addProxySubentityMarker(std::uint32_t, std::uint32_t) override {
        addCallback("addProxySubentityMarker");
        return false;
    }
#endif
    void writeAppId() override { addCallback("writeAppId"); }
    void writeViews() override { addCallback("writeViews"); }
    void writeUCSs() override { addCallback("writeUCSs"); }

    void addOperationFailure(int code, const std::string& stage,
                             const std::string& path,
                             const std::string& operation) {
        addDiagnostic("error", stage,
                      operation + "-error-" + std::to_string(code), path,
                      stage + ": libdxfrw " + operation + " failed");
    }

    bool addWriteResult(const std::string& path, const std::string& digest,
                        std::uint64_t size, const std::string& format,
                        const std::string& version) {
        std::ifstream input(path.c_str(), std::ios::binary);
        if (!input)
            return false;
        std::vector<std::uint8_t> bytes;
        std::array<char, 65536> buffer{};
        while (input) {
            input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize count = input.gcount();
            if (count > 0)
                bytes.insert(bytes.end(), buffer.data(), buffer.data() + count);
        }
        if (input.bad())
            return false;
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", "WRITE_RESULT"));
        fields.push_back(stringField("entity", "WRITE_RESULT"));
        fields.push_back(stringField("sha256", digest));
        fields.push_back(uintField("byteSize", size));
        fields.push_back(stringField("format", format));
        fields.push_back(stringField("version", version));
        const std::size_t recordIndex = addRecord("writeResult", "artifact",
                                                  "WRITE_RESULT", fields);
        const std::string carrierId = addCarrier("generatedOutput", bytes.data(),
            bytes.size(), "observed", records[recordIndex].id);
        callbacks[records[recordIndex].callbackOrdinal].carrierIds.push_back(carrierId);
        return true;
    }

    void addIntegrityDiagnostic(const DwgIntegrityDiagnostic& d) {
        std::ostringstream message;
        message.imbue(std::locale::classic());
        message << "schema=" << d.schemaVersion << ";phase=" << static_cast<int>(d.phase)
                << ";kind=" << static_cast<int>(d.kind)
                << ";section=" << d.sectionName
                << ";logicalSection=" << d.logicalSectionId
                << ";descriptor=" << d.sectionDescriptorId;
        if (d.hasPageId) message << ";page=" << d.pageId;
        if (d.hasFileOffset) message << ";offset=" << d.fileOffset;
        if (d.hasLogicalHandle) message << ";handle=" << d.logicalHandle;
        if (d.hasExpected) message << ";expected=" << d.expected;
        if (d.hasObserved) message << ";observed=" << d.observed;
        addDiagnostic(d.severity == DwgIntegritySeverity::Error ? "error" : "warning",
                      "integrity",
                      "dwg-integrity-" + std::to_string(static_cast<int>(d.kind)),
                      "/diagnostics/integrity", message.str());
    }

    std::size_t fieldCount() const {
        std::size_t total = 0;
        for (const RecordRow& row : records)
            total += row.fields.size();
        return total;
    }

private:
    void addOpaqueTyped(const std::string& callback,
                        const std::string& recordClass) {
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", recordClass));
        fields.push_back(stringField("entity", recordClass));
        const std::size_t recordIndex = addRecord(callback, "unsupported",
                                                  recordClass, fields);
        addUnsupported("semanticFields", callback, records[recordIndex].id,
                       std::string(), std::string(), "excluded");
    }

    std::size_t addCallback(const std::string& kind,
                            const std::string& recordId = std::string(),
                            const std::string& explicitBlock = std::string()) {
        const std::size_t ordinal = callbacks.size();
        callbacks.push_back(CallbackRow{indexedId("callback", ordinal), ordinal,
            kind, recordId, explicitBlock.empty() ? currentBlockId_ : explicitBlock, {}});
        return ordinal;
    }

    std::size_t addRecord(const std::string& callbackKind, const std::string& kind,
                          const std::string& recordClass, std::vector<Field> fields) {
        const std::string recordId = indexedId("record", records.size());
        const std::size_t callbackIndex = addCallback(callbackKind, recordId);
        records.push_back(RecordRow{recordId, callbackIndex, kind, recordClass,
                                    recordClass, std::string(), std::string(),
                                    currentBlockId_, std::move(fields)});
        nodes.push_back(NodeRow{"node:" + recordId, "record", recordId, std::string()});
        if (!currentBlockId_.empty())
            addEdge("blockMembership", "node:" + recordId, nodeId(currentBlockId_));
        return records.size() - 1;
    }

    template <typename T>
    void addTable(const std::string& callback, const std::string& recordClass,
                  const T& d) {
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", recordClass));
        fields.push_back(stringField("entity", recordClass));
        appendIdentityHandleFields(fields, d.handle, d.parentHandle);
        fields.push_back(stringField("name", d.name));
        fields.push_back(intField("flags", d.flags));
        const std::size_t recordIndex = addRecord(callback, "table", recordClass, fields);
        addOwnerEdges(records[recordIndex], d.handle, d.parentHandle);
        addExcluded(records[recordIndex].id, "tablePayload");
    }

    std::vector<Field> entityFields(const DRW_Entity& d,
                                    const std::string& recordClass,
                                    const std::string& entity) {
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", recordClass));
        fields.push_back(stringField("entity", entity));
        appendIdentityHandleFields(fields, d.handle, d.parentHandle);
        fields.push_back(stringField("layer", d.layer));
        fields.push_back(stringField("lineType", d.lineType));
        fields.push_back(intField("color", d.color));
        fields.push_back(intField("color24", d.color24));
        fields.push_back(boolField("visible", d.visible));
        fields.push_back(intField("space", static_cast<int>(d.space)));
        return fields;
    }

    std::size_t addEntityRecord(const std::string& callback,
                                const std::string& recordClass,
                                std::vector<Field> fields, const DRW_Entity& d,
                                bool hasExcludedFields) {
        (void)hasExcludedFields;
        const std::size_t recordIndex = addRecord(callback, "entity", recordClass,
                                                  std::move(fields));
        RecordRow& record = records[recordIndex];
        addOwnerEdges(record, d.handle, d.parentHandle);
        const std::size_t callbackIndex = record.callbackOrdinal;
        if (!d.proxyGraphics.empty()) {
            const std::string carrierId = addCarrier("entityProxyGraphics",
                d.proxyGraphics.data(), d.proxyGraphics.size(), "preserved", record.id);
            callbacks[callbackIndex].carrierIds.push_back(carrierId);
        }
        if (!d.dataStorageData.empty()) {
            const std::string carrierId = addCarrier("dwgLinkedDataStoragePayload",
                d.dataStorageData.data(), d.dataStorageData.size(), "preserved", record.id);
            callbacks[callbackIndex].carrierIds.push_back(carrierId);
        }
        addExcluded(record.id, "unmodeledCommonEntityOrSubtypeFields");
        return recordIndex;
    }

    template <typename T>
    void addBasicEntity(const std::string& callback, const std::string& recordClass,
                        const T& d) {
        addEntityRecord(callback, recordClass,
                        entityFields(d, recordClass, recordClass), d, true);
    }

    template <typename T>
    void addPointerEntity(const std::string& callback, const std::string& recordClass,
                          const T* d) {
        if (d == nullptr) { addNullDiagnostic(callback); return; }
        addBasicEntity(callback, recordClass, *d);
    }

    std::vector<Field> dimensionFields(const DRW_Dimension& d,
                                       const std::string& recordClass) {
        auto fields = entityFields(d, recordClass, recordClass);
        fields.push_back(pointField("definitionPointWcs", d.getDefPoint()));
        fields.push_back(pointField("textPointOcs", d.getTextPoint()));
        fields.push_back(pointField("extrusion", d.getExtrusion()));
        fields.push_back(intField("dimensionType", d.type));
        fields.push_back(stringField("text", d.getText()));
        fields.push_back(stringField("style", d.getStyle()));
        if (const auto* aligned = dynamic_cast<const DRW_DimAligned*>(&d)) {
            fields.push_back(pointField("clonePointOcs", aligned->getClonepoint()));
            fields.push_back(pointField("definition1Wcs", aligned->getDef1Point()));
            fields.push_back(pointField("definition2Wcs", aligned->getDef2Point()));
        } else if (const auto* angular = dynamic_cast<const DRW_DimAngular*>(&d)) {
            fields.push_back(pointField("firstLine1Wcs", angular->getFirstLine1()));
            fields.push_back(pointField("firstLine2Wcs", angular->getFirstLine2()));
            fields.push_back(pointField("secondLine1Wcs", angular->getSecondLine1()));
            fields.push_back(pointField("arcPointOcs", angular->getDimPoint()));
        } else if (const auto* angular = dynamic_cast<const DRW_DimAngular3p*>(&d)) {
            fields.push_back(pointField("firstLineWcs", angular->getFirstLine()));
            fields.push_back(pointField("secondLineWcs", angular->getSecondLine()));
            fields.push_back(pointField("vertexWcs", angular->getVertexPoint()));
        } else if (const auto* radial = dynamic_cast<const DRW_DimRadial*>(&d)) {
            if (dynamic_cast<const DRW_DimLargeRadial*>(radial) == nullptr)
                fields.push_back(pointField("diameterPointWcs", radial->getDiameterPoint()));
        } else if (const auto* diametric = dynamic_cast<const DRW_DimDiametric*>(&d)) {
            fields.push_back(pointField("diameter1Wcs", diametric->getDiameter1Point()));
        } else if (const auto* ordinate = dynamic_cast<const DRW_DimOrdinate*>(&d)) {
            fields.push_back(pointField("featureWcs", ordinate->getFirstLine()));
            fields.push_back(pointField("leaderEndWcs", ordinate->getSecondLine()));
        } else if (const auto* arc = dynamic_cast<const DRW_DimArc*>(&d)) {
            fields.push_back(pointField("extension1Wcs", arc->getExtLine1()));
            fields.push_back(pointField("extension2Wcs", arc->getExtLine2()));
            fields.push_back(pointField("arcCenterWcs", arc->getArcCenter()));
            fields.push_back(boolField("isPartial", arc->isPartial));
            fields.push_back(boolField("hasLeader", arc->hasLeader));
            // ARC_DIMENSION's optional leader grammar is distinct from the
            // ordinary angular dimension's OCS group16; preserve raw fields
            // without inferring a leader coordinate conversion.
            fields.push_back(pointField("leader1Stored", arc->getLeaderPt1()));
            fields.push_back(pointField("leader2Stored", arc->leaderPt2));
        }
        return fields;
    }

    void addDimension(const std::string& callback, const std::string& recordClass,
                      const DRW_Dimension* d) {
        if (d == nullptr) { addNullDiagnostic(callback); return; }
        addEntityRecord(callback, recordClass, dimensionFields(*d, recordClass), *d, true);
    }

    void attachCarrierToRecord(std::size_t recordIndex, const std::string& source,
                               const void* bytes, std::size_t size) {
        if (recordIndex >= records.size())
            return;
        const std::string carrierId = addCarrier(source, bytes, size,
                                                  "preserved",
                                                  records[recordIndex].id);
        const std::size_t callbackIndex = records[recordIndex].callbackOrdinal;
        if (callbackIndex < callbacks.size())
            callbacks[callbackIndex].carrierIds.push_back(carrierId);
    }

    void appendDataStorageFields(std::vector<Field>& fields,
                                 const DRW_Entity& entity) const {
        fields.push_back(boolField("hasDataStorageBinaryData",
                                   entity.hasDataStorageBinaryData()));
        fields.push_back(boolField("hasDataStorageRecord",
                                   entity.hasDataStorageRecord));
        fields.push_back(uintField("dataStorageByteCount",
                                   entity.dataStorageData.size()));
        fields.push_back(uintField("dataStorageHandle",
                                   entity.dataStorageHandle));
        fields.push_back(stringField("dataStorageHandleKey",
                                     entity.dataStorageHandleKey));
        fields.push_back(uintField("dataStorageSegmentIndex",
                                   entity.dataStorageSegmentIndex));
        fields.push_back(uintField("dataStorageSchemaIndex",
                                   entity.dataStorageSchemaIndex));
        fields.push_back(boolField("hasDataStoragePayloadMarker",
                                   entity.hasDataStoragePayloadMarker));
        fields.push_back(uintField("dataStoragePayloadMarkerOffset",
                                   entity.dataStoragePayloadMarkerOffset));
        fields.push_back(uintField("dataStoragePayloadMarkerLength",
                                   entity.dataStoragePayloadMarkerLength));
        fields.push_back(stringField("dataStoragePayloadMarkerSection",
                                     entity.dataStoragePayloadMarkerSection));
    }

    template <typename T>
    bool appendModelerDwgSourceVersionField(
        std::vector<Field>& fields, const T& modeler) const {
        if constexpr (HasModelerDwgSourceVersion<T>::value) {
            fields.push_back(intField("dwgSourceVersion",
                                      static_cast<int>(modeler.m_dwgSourceVersion)));
            return true;
        }
        return false;
    }

    template <typename T>
    bool appendModelerDwgAcisPayloadFields(
        std::vector<Field>& fields, const T& modeler) const {
        if constexpr (HasModelerDwgAcisPayload<T>::value) {
            fields.push_back(boolField("hasDwgInlineAcisPayload",
                                       !modeler.m_dwgAcisPayload.empty()));
            fields.push_back(uintField("dwgInlineAcisByteCount",
                                       modeler.m_dwgAcisPayload.size()));
            return true;
        }
        return false;
    }

    template <typename T>
    bool appendModelerDxfPayloadChunkFields(
        std::vector<Field>& fields, const T& modeler,
        const std::vector<std::uint8_t>& bytes) const {
        if constexpr (HasModelerDxfPayloadChunks<T>::value) {
            appendPayloadChunkFields(fields, "dxfPayloadChunk",
                                     modeler.m_dxfPayloadChunks, bytes);
            return true;
        }
        return false;
    }

    template <typename T>
    bool appendModelerPayloadStatusField(
        std::vector<Field>& fields, const T& modeler) const {
        if constexpr (HasModelerPayloadStatusName<T>::value) {
            fields.push_back(stringField("modelerPayloadStatus",
                                         modeler.modelerPayloadStatusName()));
            return true;
        }
        return false;
    }

    template <typename T>
    void attachModelerDwgAcisPayloadCarrier(
        std::size_t recordIndex, const T& modeler) {
        if constexpr (HasModelerDwgAcisPayload<T>::value) {
            if (!modeler.m_dwgAcisPayload.empty()) {
                attachCarrierToRecord(recordIndex,
                                      facade_ == "dwgRW" && direction_ == "read"
                                          ? "dwgModelerInlineAcisPayload"
                                          : "unclassifiedModelerInlineAcisPayload",
                                      modeler.m_dwgAcisPayload.data(),
                                      modeler.m_dwgAcisPayload.size());
            }
        }
    }

    template <typename T>
    bool appendPayloadRangeBitOffset(std::vector<Field>& fields,
                                    const std::string& prefix,
                                    const T& range) const {
        if constexpr (HasPayloadRangeBitOffset<T>::value) {
            fields.push_back(uintField(prefix + ".bitOffset", range.m_bitOffset));
            return true;
        }
        return false;
    }

    template <typename Chunks>
    void appendPayloadChunkFields(
        std::vector<Field>& fields, const std::string& name,
        const Chunks& chunks, const std::vector<std::uint8_t>& bytes) const {
        fields.push_back(uintField(name + ".count", chunks.size()));
        for (std::size_t i = 0; i < chunks.size(); ++i) {
            const auto& chunk = chunks[i];
            const std::string prefix = name + "." + std::to_string(i);
            const bool inBounds = chunk.m_offset <= bytes.size()
                && chunk.m_length <= bytes.size() - chunk.m_offset;
            fields.push_back(intField(prefix + ".groupCode", chunk.m_groupCode));
            fields.push_back(uintField(prefix + ".offset", chunk.m_offset));
            fields.push_back(uintField(prefix + ".length", chunk.m_length));
            fields.push_back(boolField(prefix + ".inBounds", inBounds));
        }
    }

    void appendVertexFields(std::vector<Field>& fields, const DRW_Vertex* vertex,
                            std::size_t index) const {
        const std::string prefix = "vertex." + std::to_string(index);
        if (vertex == nullptr) {
            fields.push_back(boolField(prefix + ".present", false));
            return;
        }
        fields.push_back(boolField(prefix + ".present", true));
        fields.push_back(pointField(prefix + ".position", vertex->basePoint));
        fields.push_back(pointField(prefix + ".extrusion", vertex->extPoint));
        fields.push_back(doubleField(prefix + ".startWidth", vertex->stawidth));
        fields.push_back(doubleField(prefix + ".endWidth", vertex->endwidth));
        fields.push_back(doubleField(prefix + ".bulge", vertex->bulge));
        fields.push_back(intField(prefix + ".flags", vertex->flags));
        fields.push_back(doubleField(prefix + ".tangentDirection", vertex->tgdir));
        fields.push_back(intField(prefix + ".faceIndex1", vertex->vindex1));
        fields.push_back(intField(prefix + ".faceIndex2", vertex->vindex2));
        fields.push_back(intField(prefix + ".faceIndex3", vertex->vindex3));
        fields.push_back(intField(prefix + ".faceIndex4", vertex->vindex4));
        fields.push_back(intField(prefix + ".identifier", vertex->identifier));
        fields.push_back(intField(prefix + ".dwgSubtype",
                                  static_cast<int>(vertex->dwgSubtype())));
        if (vertex->handle != DRW::NoHandle)
            fields.push_back(handleField(prefix + ".handle", vertex->handle));
        if (vertex->parentHandle != DRW::NoHandle)
            fields.push_back(handleField(prefix + ".ownerHandle", vertex->parentHandle));
    }

    void appendSplineFields(std::vector<Field>& fields,
                            const DRW_Spline& spline) const {
        fields.push_back(pointField("normal", spline.normalVec));
        fields.push_back(pointField("startTangent", spline.tgStart));
        fields.push_back(pointField("endTangent", spline.tgEnd));
        fields.push_back(intField("flags", spline.flags));
        fields.push_back(intField("degree", spline.degree));
        fields.push_back(intField("scenario", spline.m_scenario));
        fields.push_back(intField("splineFlags1", spline.m_splineFlags1));
        fields.push_back(intField("knotParameterization", spline.m_knotParam));
        fields.push_back(intField("declaredKnotCount", spline.nknots));
        fields.push_back(intField("declaredControlPointCount", spline.ncontrol));
        fields.push_back(intField("declaredFitPointCount", spline.nfit));
        fields.push_back(doubleField("knotTolerance", spline.tolknot));
        fields.push_back(doubleField("controlTolerance", spline.tolcontrol));
        fields.push_back(doubleField("fitTolerance", spline.tolfit));
        fields.push_back(uintField("knotCount", spline.knotslist.size()));
        for (std::size_t i = 0; i < spline.knotslist.size(); ++i)
            fields.push_back(doubleField("knot." + std::to_string(i),
                                         spline.knotslist[i]));
        fields.push_back(uintField("weightCount", spline.weightlist.size()));
        for (std::size_t i = 0; i < spline.weightlist.size(); ++i)
            fields.push_back(doubleField("weight." + std::to_string(i),
                                         spline.weightlist[i]));
        fields.push_back(uintField("controlPointCount", spline.controllist.size()));
        for (std::size_t i = 0; i < spline.controllist.size(); ++i) {
            const std::string prefix = "controlPoint." + std::to_string(i);
            fields.push_back(boolField(prefix + ".present",
                                       static_cast<bool>(spline.controllist[i])));
            if (spline.controllist[i])
                fields.push_back(pointField(prefix + ".position",
                                            *spline.controllist[i]));
        }
        fields.push_back(uintField("fitPointCount", spline.fitlist.size()));
        for (std::size_t i = 0; i < spline.fitlist.size(); ++i) {
            const std::string prefix = "fitPoint." + std::to_string(i);
            fields.push_back(boolField(prefix + ".present",
                                       static_cast<bool>(spline.fitlist[i])));
            if (spline.fitlist[i])
                fields.push_back(pointField(prefix + ".position", *spline.fitlist[i]));
        }
    }

    template <std::size_t N>
    void appendTransformFields(std::vector<Field>& fields,
                               const std::string& name,
                               const std::array<double, N>& values) const {
        fields.push_back(uintField(name + ".count", values.size()));
        for (std::size_t i = 0; i < values.size(); ++i)
            fields.push_back(doubleField(name + "." + std::to_string(i), values[i]));
    }

    void appendHandleVector(std::vector<Field>& fields, const std::string& name,
                            const std::vector<std::uint32_t>& handles) const {
        fields.push_back(uintField(name + ".count", handles.size()));
        for (std::size_t i = 0; i < handles.size(); ++i)
            fields.push_back(handleField(name + "." + std::to_string(i), handles[i]));
    }

    void appendVariantFields(std::vector<Field>& fields, const std::string& name,
                             const std::vector<DRW_Variant>& values) const {
        fields.push_back(uintField(name + ".count", values.size()));
        for (std::size_t i = 0; i < values.size(); ++i) {
            const DRW_Variant& value = values[i];
            const std::string prefix = name + "." + std::to_string(i);
            fields.push_back(intField(prefix + ".groupCode", value.code()));
            fields.push_back(intField(prefix + ".type", static_cast<int>(value.type())));
            fields.push_back(boolField(prefix + ".layerReference", value.isLayerRef()));
            switch (value.type()) {
            case DRW_Variant::STRING:
                fields.push_back(stringField(prefix + ".value", value.c_str()));
                break;
            case DRW_Variant::INTEGER:
                fields.push_back(intField(prefix + ".value", value.i_val()));
                break;
            case DRW_Variant::INTEGER64:
                fields.push_back(intField(prefix + ".value", value.i64_val()));
                break;
            case DRW_Variant::DOUBLE:
                fields.push_back(doubleField(prefix + ".value", value.d_val()));
                break;
            case DRW_Variant::COORD:
                if (value.coord() != nullptr)
                    fields.push_back(pointField(prefix + ".value", *value.coord()));
                break;
            case DRW_Variant::BINARY:
                fields.push_back(uintField(prefix + ".byteCount",
                    value.binary() == nullptr ? 0 : value.binary()->size()));
                break;
            case DRW_Variant::INVALID:
                break;
            }
        }
    }

    template <typename T>
    bool appendSurfaceDxfPayloadChunkFields(
        std::vector<Field>& fields, const T& surface) const {
        if constexpr (HasSurfaceDxfPayloadChunks<T>::value) {
            appendPayloadChunkFields(fields, "dxfAcisChunk",
                                     surface.dxfPayloadChunks,
                                     surface.rawAcisData);
            return true;
        }
        return false;
    }

    void appendSurfaceFields(std::vector<Field>& fields,
                             const DRW_Surface& surface,
                             std::vector<std::string>& excludedFields) const {
        fields.push_back(intField("uIsolines", surface.uIsolines));
        fields.push_back(intField("vIsolines", surface.vIsolines));
        fields.push_back(intField("modelerFormatVersion", surface.modelerFormatVersion));
        fields.push_back(boolField("acisEmpty", surface.acisEmpty));
        fields.push_back(intField("acisVersion", surface.acisVersion));
        fields.push_back(boolField("hasRawAcisData", !surface.rawAcisData.empty()));
        fields.push_back(uintField("rawAcisByteCount", surface.rawAcisData.size()));
        if (!appendSurfaceDxfPayloadChunkFields(fields, surface))
            excludedFields.push_back("dxfAcisChunks");
        appendDataStorageFields(fields, surface);
        fields.push_back(stringField("rawAcisCarrierKind",
            facade_ == "dwgRW" && direction_ == "read"
                ? "dwgSurfaceReaderPayload-unclassified"
                : facade_ == "dxfRW"
                    ? "dxfEntityInlineAcis"
                    : "unknown"));
        fields.push_back(boolField("hasRawDwgBody", surface.hasRawDwgBody));
        fields.push_back(uintField("rawDwgBodyBitSize", surface.rawDwgBodyBitSize));
        fields.push_back(intField("rawDwgBodyVersion",
                                  static_cast<int>(surface.rawDwgBodyVersion)));
        fields.push_back(uintField("dwgClassNum", surface.dwgClassNum));
        fields.push_back(boolField("dwgPayloadDecoded", surface.dwgPayloadDecoded));
        fields.push_back(boolField("derivedWireframeDecoded", surface.m_wireframeDecoded));
        fields.push_back(uintField("derivedWireframeVertexCount",
                                   surface.m_wireframe.vertices.size()));
        fields.push_back(uintField("derivedWireframeEdgeCount",
                                   surface.m_wireframe.edges.size()));
        fields.push_back(uintField("derivedWireframeFaceCount",
                                   surface.m_wireframe.faces.size()));
        appendHandleVector(fields, "crossSectionHandle", surface.crossSectionHandles);
        appendHandleVector(fields, "guideCurveHandle", surface.guideCurveHandles);

        if (const auto* value = dynamic_cast<const DRW_ExtrudedSurface*>(&surface)) {
            fields.push_back(uintField("classId", value->classId));
            fields.push_back(pointField("sweepVector", value->sweepVector));
            appendTransformFields(fields, "extrudedTransform", value->extrudedTransform);
            appendTransformFields(fields, "sweepEntityTransform", value->sweepEntityTransform);
            appendTransformFields(fields, "pathEntityTransform", value->pathEntityTransform);
            fields.push_back(doubleField("draftAngle", value->draftAngle));
            fields.push_back(doubleField("draftStartDistance", value->draftStartDistance));
            fields.push_back(doubleField("draftEndDistance", value->draftEndDistance));
            fields.push_back(doubleField("twistAngle", value->twistAngle));
            fields.push_back(doubleField("scaleFactor", value->scaleFactor));
            fields.push_back(doubleField("alignAngle", value->alignAngle));
            fields.push_back(boolField("solid", value->solid));
            fields.push_back(intField("sweepAlignmentFlags", value->sweepAlignmentFlags));
            fields.push_back(intField("pathFlags", value->pathFlags));
            fields.push_back(boolField("alignStart", value->alignStart));
            fields.push_back(boolField("bank", value->bank));
            fields.push_back(boolField("basePointSet", value->basePointSet));
            fields.push_back(boolField("sweepEntityTransformComputed",
                                       value->sweepEntityTransformComputed));
            fields.push_back(boolField("pathEntityTransformComputed",
                                       value->pathEntityTransformComputed));
            fields.push_back(pointField("referenceVector", value->referenceVector));
        } else if (const auto* value = dynamic_cast<const DRW_RevolvedSurface*>(&surface)) {
            fields.push_back(uintField("classId", value->classId));
            fields.push_back(uintField("id", value->id));
            fields.push_back(pointField("axisPoint", value->axisPoint));
            fields.push_back(pointField("axisVector", value->axisVector));
            fields.push_back(doubleField("revolveAngle", value->revolveAngle));
            fields.push_back(doubleField("startAngle", value->startAngle));
            appendTransformFields(fields, "transform", value->transform);
            fields.push_back(doubleField("draftAngle", value->draftAngle));
            fields.push_back(doubleField("draftStartDistance", value->draftStartDistance));
            fields.push_back(doubleField("draftEndDistance", value->draftEndDistance));
            fields.push_back(doubleField("twistAngle", value->twistAngle));
            fields.push_back(boolField("solid", value->solid));
            fields.push_back(boolField("closeToAxis", value->closeToAxis));
        } else if (const auto* value = dynamic_cast<const DRW_SweptSurface*>(&surface)) {
            fields.push_back(uintField("classVersion", value->classVersion));
            fields.push_back(uintField("sweepEntityId", value->sweepEntityId));
            fields.push_back(uintField("sweepDataByteCount", value->sweepData.size()));
            fields.push_back(uintField("pathEntityId", value->pathEntityId));
            fields.push_back(uintField("pathDataByteCount", value->pathData.size()));
            appendTransformFields(fields, "sweepEntityTransform", value->sweepEntityTransform);
            appendTransformFields(fields, "pathEntityTransform", value->pathEntityTransform);
            appendTransformFields(fields, "sweepEntityTransformed", value->sweepEntityTransformed);
            appendTransformFields(fields, "pathEntityTransformed", value->pathEntityTransformed);
            fields.push_back(doubleField("draftAngle", value->draftAngle));
            fields.push_back(doubleField("draftStartDistance", value->draftStartDistance));
            fields.push_back(doubleField("draftEndDistance", value->draftEndDistance));
            fields.push_back(doubleField("twistAngle", value->twistAngle));
            fields.push_back(doubleField("scaleFactor", value->scaleFactor));
            fields.push_back(doubleField("alignAngle", value->alignAngle));
            fields.push_back(boolField("solid", value->solid));
            fields.push_back(intField("sweepAlignmentFlags", value->sweepAlignmentFlags));
            fields.push_back(intField("pathFlags", value->pathFlags));
            fields.push_back(boolField("alignStart", value->alignStart));
            fields.push_back(boolField("bank", value->bank));
            fields.push_back(boolField("basePointSet", value->basePointSet));
            fields.push_back(boolField("sweepEntityTransformComputed",
                                       value->sweepEntityTransformComputed));
            fields.push_back(boolField("pathEntityTransformComputed",
                                       value->pathEntityTransformComputed));
            fields.push_back(pointField("referenceVector", value->referenceVector));
        } else if (const auto* value = dynamic_cast<const DRW_LoftedSurface*>(&surface)) {
            appendTransformFields(fields, "loftEntityTransform", value->loftEntityTransform);
            fields.push_back(intField("planeNormalLoftingType", value->planeNormalLoftingType));
            fields.push_back(doubleField("startDraftAngle", value->startDraftAngle));
            fields.push_back(doubleField("endDraftAngle", value->endDraftAngle));
            fields.push_back(doubleField("startDraftMagnitude", value->startDraftMagnitude));
            fields.push_back(doubleField("endDraftMagnitude", value->endDraftMagnitude));
            fields.push_back(boolField("arcLengthParameterization", value->arcLengthParameterization));
            fields.push_back(boolField("noTwist", value->noTwist));
            fields.push_back(boolField("alignDirection", value->alignDirection));
            fields.push_back(boolField("simpleSurfaces", value->simpleSurfaces));
            fields.push_back(boolField("closedSurfaces", value->closedSurfaces));
            fields.push_back(boolField("solid", value->solid));
            fields.push_back(boolField("ruledSurface", value->ruledSurface));
            fields.push_back(boolField("virtualGuide", value->virtualGuide));
            fields.push_back(intField("numCrossSections", value->numCrossSections));
            fields.push_back(intField("numGuideCurves", value->numGuideCurves));
            fields.push_back(handleField("pathCurveHandle", value->pathCurveHandle));
            appendVariantFields(fields, "dxfReferenceData", value->dxfReferenceData);
        } else if (const auto* value = dynamic_cast<const DRW_NurbsSurface*>(&surface)) {
            fields.push_back(intField("short170", value->short170));
            fields.push_back(boolField("cvHullDisplay", value->cvHullDisplay));
            fields.push_back(pointField("uvec1", value->uvec1));
            fields.push_back(pointField("vvec1", value->vvec1));
            fields.push_back(pointField("uvec2", value->uvec2));
            fields.push_back(pointField("vvec2", value->vvec2));
        }
    }

    template <typename T>
    void addLineLike(const std::string& callback, const std::string& recordClass,
                     const T& d) {
        std::vector<Field> fields = entityFields(d, recordClass, recordClass);
        fields.push_back(pointField("start", d.basePoint));
        fields.push_back(pointField("direction", d.secPoint));
        addEntityRecord(callback, recordClass, fields, d, false);
    }

    void addOwnerEdges(RecordRow& record, std::uint32_t handle,
                       std::uint32_t ownerHandle) {
        if (handle != DRW::NoHandle) {
            record.sourceHandle = hexHandle(handle);
            for (NodeRow& node : nodes) {
                if (node.recordId == record.id) {
                    node.handle = hexHandle(handle);
                    break;
                }
            }
            addEdge("handleReference", "node:" + record.id,
                    ensureHandleNode(handle));
        }
        if (ownerHandle != DRW::NoHandle) {
            record.ownerHandle = hexHandle(ownerHandle);
            const std::string owner = ensureHandleNode(ownerHandle);
            addEdge("owner", "node:" + record.id, owner);
        }
    }

    std::string ensureHandleNode(std::uint32_t handle) {
        const std::string id = "node:handle:" + hexHandle(handle);
        if (nodeIds_.insert(std::make_pair(id, true)).second)
            nodes.push_back(NodeRow{id, "handle", std::string(), hexHandle(handle)});
        return id;
    }

    std::string nodeId(const std::string& recordOrNode) const {
        return recordOrNode.compare(0, 5, "node:") == 0
            ? recordOrNode : "node:" + recordOrNode;
    }

    void addEdge(const std::string& kind, const std::string& from,
                 const std::string& to) {
        edges.push_back(EdgeRow{indexedId("edge", edges.size()), kind, from, to,
                                "observed"});
    }

    std::string addCarrier(const std::string& source, const void* bytes,
                           std::size_t size, const std::string& disposition,
                           const std::string& recordId) {
        const std::string id = indexedId("carrier", carriers.size());
        static const std::uint8_t empty = 0;
        std::string sourceHandle;
        for (const RecordRow& record : records) {
            if (record.id == recordId) {
                sourceHandle = record.sourceHandle;
                break;
            }
        }
        const std::string sourcePath = recordId.empty()
            ? "/opaqueCarriers/" + pointerSegment(source)
            : "/records/" + pointerSegment(recordId) + "/" + pointerSegment(source);
        carriers.push_back(CarrierRow{id, sourcePath, size,
            sha256(size == 0 ? static_cast<const void*>(&empty) : bytes, size),
            disposition, recordId, sourceHandle});
        return id;
    }

    void addUnsupported(const std::string& kind, const std::string& source,
                        const std::string& recordId, const std::string& carrierId,
                        const std::string& diagnosticId,
                        const std::string& disposition) {
        const std::string reason = disposition == "excluded"
            ? "adapterFieldCoverageExcluded" : "opaqueBytesPreserved";
        const std::string sourcePath = !recordId.empty()
            ? "/records/" + pointerSegment(recordId) + "/" + pointerSegment(source)
            : (!carrierId.empty()
                ? "/opaqueCarriers/" + pointerSegment(carrierId) + "/" +
                    pointerSegment(source)
                : "/unsupportedContent/" + pointerSegment(source));
        unsupported.push_back(UnsupportedRow{indexedId("unsupported", unsupported.size()),
            kind, sourcePath, recordId, carrierId, diagnosticId, disposition, reason});
    }

    void addExcluded(const std::string& recordId, const std::string& source) {
        addUnsupported("semanticFields", source, recordId, std::string(),
                       std::string(), "excluded");
    }

    void addDiagnostic(const std::string& severity, const std::string& stage,
                       const std::string& code,
                       const std::string& path, const std::string& message) {
        const std::size_t index = diagnostics.size();
        diagnostics.push_back(DiagnosticRow{indexedId("diagnostic", index), index,
            severity, stage, code, path, sha256(message)});
    }

    void addNullDiagnostic(const std::string& callback) {
        addCallback(callback);
        addDiagnostic("error", "callback", "null-callback-payload-1", "/callbacks",
                      callback + " received a null payload");
    }

    static std::string variantValue(const DRW_Variant& value) {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << value.code() << ':' << static_cast<int>(value.type()) << ':';
        switch (value.type()) {
        case DRW_Variant::STRING: out << value.c_str(); break;
        case DRW_Variant::INTEGER: out << value.i_val(); break;
        case DRW_Variant::INTEGER64: out << value.i64_val(); break;
        case DRW_Variant::DOUBLE: out << number(value.d_val()); break;
        case DRW_Variant::COORD:
            if (value.coord() != nullptr)
                out << number(value.coord()->x) << ',' << number(value.coord()->y)
                    << ',' << number(value.coord()->z);
            break;
        case DRW_Variant::BINARY:
            if (value.binary() != nullptr) {
                out << value.binary()->size() << ':';
                for (std::uint8_t byte : *value.binary())
                    out << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<unsigned>(byte);
            }
            break;
        case DRW_Variant::INVALID: out << "invalid"; break;
        }
        return out.str();
    }

    static std::string pointerSegment(const std::string& value) {
        std::string result;
        for (char ch : value) {
            if (ch == '~') result += "~0";
            else if (ch == '/') result += "~1";
            else result.push_back(ch);
        }
        return result;
    }

    static std::string serializeVariants(const std::vector<DRW_Variant>& values) {
        std::string result;
        for (const DRW_Variant& value : values) {
            const std::string encoded = variantValue(value);
            appendLengthFramed(result, encoded);
        }
        return result;
    }

    void addRawDxf(const std::string& callback, const std::string& source,
                   const DRW_RawDxfObject& d) {
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", d.name));
        fields.push_back(stringField("entity", d.name));
        appendIdentityHandleFields(fields, d.handle, d.parentHandle);
        const std::size_t recordIndex = addRecord(callback, "unsupported", d.name, fields);
        addOwnerEdges(records[recordIndex], d.handle, d.parentHandle);
        const std::string bytes = serializeVariants(d.groups);
        const std::string carrierId = addCarrier(source, bytes.data(), bytes.size(),
                                                 "preservedCanonical",
                                                 records[recordIndex].id);
        callbacks[records[recordIndex].callbackOrdinal].carrierIds.push_back(carrierId);
        addUnsupported("opaqueRecord", "dxf", records[recordIndex].id,
                       carrierId, std::string(), "preservedCanonical");
    }

    std::string facade_;
    std::string direction_;
    std::string currentBlockId_;
    std::map<std::string, bool> nodeIds_;
    dxfRW* dxfWriter_ = nullptr;
    dwgRW* dwgWriter_ = nullptr;
    bool writeRecipeSucceeded_ = true;
    bool collectMeshFallbackPolylines_ = false;
    bool collect3DFaceFallbackLines_ = false;
    bool collectTraceFallbackLines_ = false;
    bool threeDConsumerProbe_ = false;
    bool nestedMTextAttributeProbe_ = false;
    bool planeSurfaceInlinePayloadProbe_ = false;
    bool modelerCarrierProbe_ = false;
    bool partialDwgReadbackControl_ = false;
    bool opaqueDwgReadbackControl_ = false;
    DRW_UnsupportedObject opaqueDwgReadbackObject_;
};

void SemanticSink::writeEntities() {
    addCallback("writeEntities");
    if (partialDwgReadbackControl_) {
        if (dwgWriter_ == nullptr) {
            writeRecipeSucceeded_ = false;
            return;
        }

        // The test encoder injects a non-finite start-X into an otherwise
        // valid LINE body. The reader must reject that record and continue.
        NonFiniteDwgLineWire malformed;
        malformed.basePoint = DRW_Coord{
            std::numeric_limits<double>::quiet_NaN(), 12.0, 13.0};
        malformed.secPoint = DRW_Coord{21.0, 22.0, 23.0};
        bool wrote = dwgWriter_->writeLine(&malformed);
        DRW_Line followingLine;
        followingLine.basePoint = DRW_Coord{11.0, 12.0, 13.0};
        followingLine.secPoint = DRW_Coord{21.0, 22.0, 23.0};
        wrote = dwgWriter_->writeLine(&followingLine) && wrote;
        writeRecipeSucceeded_ = wrote && writeRecipeSucceeded_;
        return;
    }
    if (opaqueDwgReadbackControl_) {
        if (dwgWriter_ == nullptr) {
            writeRecipeSucceeded_ = false;
            return;
        }
        DRW_Line line;
        line.basePoint = DRW_Coord{11.0, 12.0, 13.0};
        line.secPoint = DRW_Coord{21.0, 22.0, 23.0};
        writeRecipeSucceeded_ = dwgWriter_->writeLine(&line)
            && writeRecipeSucceeded_;
        return;
    }
#ifdef LIBDXFRW_SEMANTIC_3D_CONSUMER_PROBE
    if (planeSurfaceInlinePayloadProbe_) {
        if (dxfWriter_ == nullptr) {
            writeRecipeSucceeded_ = false;
            return;
        }
        DRW_PlaneSurface plane;
        plane.layer = "probe_plane_surface_inline_payload";
        plane.modelerFormatVersion = 1;
        const std::string acis = "ACIS 7.0\nopaque-plane";
        plane.rawAcisData.assign(acis.begin(), acis.end());
        plane.dxfPayloadChunks.emplace_back(1, 0, 8);
        plane.dxfPayloadChunks.emplace_back(1, 9, acis.size() - 9);
        writeRecipeSucceeded_ = dxfWriter_->writeSurface(&plane)
            && writeRecipeSucceeded_;
        return;
    }
    if (nestedMTextAttributeProbe_) {
        if (dxfWriter_ == nullptr) {
            writeRecipeSucceeded_ = false;
            return;
        }
        DRW_Insert insert;
        insert.layer = "probe_nested_mtext_attribute";
        insert.name = "PROBE_MTEXT_ATTRIBUTE_PARENT";
        insert.basePoint = DRW_Coord{7.0, -3.0, 11.0};
        insert.extPoint = DRW_Coord{0.0, 1.0, 0.0};
        insert.xscale = 1.5;
        insert.yscale = -0.75;
        insert.zscale = 2.0;
        insert.angle = 0.375;
        auto attribute = std::make_shared<DRW_Attrib>();
        attribute->tag = "MTEXT_TAG";
        attribute->text = "single-line fallback";
        attribute->basePoint = DRW_Coord{13.0, 17.0, 19.0};
        attribute->secPoint = DRW_Coord{23.0, 29.0, 31.0};
        attribute->extPoint = DRW_Coord{0.0, 0.0, 1.0};
        attribute->height = 2.25;
        attribute->attVersion = 1;
        attribute->m_attributeType = 2;
        attribute->mtext = std::make_unique<DRW_MText>();
        attribute->mtext->text = "first line\\Psecond line";
        attribute->mtext->basePoint = DRW_Coord{37.0, 41.0, 43.0};
        attribute->mtext->height = 1.75;
        attribute->mtext->angle = 0.625;
        insert.attlist.push_back(std::move(attribute));
        insert.seqEndEntity = std::make_shared<DRW_SeqEnd>();
        writeRecipeSucceeded_ = dxfWriter_->writeInsert(&insert)
            && writeRecipeSucceeded_;
        return;
    }
    if (modelerCarrierProbe_) {
        if (dxfWriter_ == nullptr) {
            writeRecipeSucceeded_ = false;
            return;
        }
        const auto writeModelerCarrier = [&](DRW::ETYPE type,
                                              const std::string& layer,
                                              const std::string& body) {
            DRW_ModelerGeometry modeler(type);
            modeler.layer = layer;
            const std::string firstLine = "ACIS 7.0";
            modeler.m_rawBytes.assign(firstLine.begin(), firstLine.end());
            modeler.m_rawBytes.push_back('\n');
            modeler.m_rawBytes.insert(modeler.m_rawBytes.end(),
                                      body.begin(), body.end());
            modeler.m_dxfPayloadChunks.emplace_back(1, 0,
                                                     firstLine.size());
            modeler.m_dxfPayloadChunks.emplace_back(
                1, firstLine.size() + 1, body.size());
            return dxfWriter_->writeModelerGeometry(&modeler);
        };
        bool wrote = writeModelerCarrier(
            DRW::E3DSOLID, "probe_carrier_3dsolid", "solid-body");
        wrote = writeModelerCarrier(
            DRW::REGION, "probe_carrier_region", "region-body") && wrote;
        wrote = writeModelerCarrier(
            DRW::BODY, "probe_carrier_body", "body-body") && wrote;
        writeRecipeSucceeded_ = wrote && writeRecipeSucceeded_;
        return;
    }
    if (threeDConsumerProbe_) {
        if (dxfWriter_ == nullptr) {
            writeRecipeSucceeded_ = false;
            return;
        }
        bool wrote = true;
        DRW_Circle defaultCircle;
        defaultCircle.layer = "probe_circle_default";
        defaultCircle.basePoint = DRW_Coord{6.0, -3.0, 1.5};
        defaultCircle.radious = 2.5;
        defaultCircle.thickness = 0.75;
        wrote = dxfWriter_->writeCircle(&defaultCircle) && wrote;

        DRW_Circle obliqueCircle;
        obliqueCircle.layer = "probe_circle_oblique";
        obliqueCircle.basePoint = DRW_Coord{2.0, 3.0, 5.0};
        obliqueCircle.radious = 4.0;
        obliqueCircle.thickness = 0.5;
        obliqueCircle.extPoint = DRW_Coord{0.0, 1.0, 0.0};
        wrote = dxfWriter_->writeCircle(&obliqueCircle) && wrote;

        DRW_Arc negativeNormalArc;
        negativeNormalArc.layer = "probe_arc_negative_normal";
        negativeNormalArc.basePoint = DRW_Coord{2.0, 3.0, 4.0};
        negativeNormalArc.radious = 7.0;
        negativeNormalArc.thickness = 0.125;
        negativeNormalArc.extPoint = DRW_Coord{0.0, 0.0, -1.0};
        negativeNormalArc.staangle = 10.0 / ARAD;
        negativeNormalArc.endangle = 70.0 / ARAD;
        wrote = dxfWriter_->writeArc(&negativeNormalArc) && wrote;

        DRW_Ellipse defaultEllipse;
        defaultEllipse.layer = "probe_ellipse_default";
        defaultEllipse.basePoint = DRW_Coord{1.0, 2.0, 3.0};
        defaultEllipse.secPoint = DRW_Coord{5.0, 0.0, 0.0};
        defaultEllipse.ratio = 0.5;
        defaultEllipse.staparam = 0.0;
        defaultEllipse.endparam = 2.0 * M_PI;
        wrote = dxfWriter_->writeEllipse(&defaultEllipse) && wrote;

        DRW_Ellipse tiltedEllipse;
        tiltedEllipse.layer = "probe_ellipse_tilted_partial";
        tiltedEllipse.basePoint = DRW_Coord{-2.0, 4.0, 6.0};
        tiltedEllipse.secPoint = DRW_Coord{2.0, -1.0, 0.0};
        tiltedEllipse.extPoint = DRW_Coord{1.0 / 3.0, 2.0 / 3.0,
                                            2.0 / 3.0};
        tiltedEllipse.ratio = 0.4;
        tiltedEllipse.staparam = 0.25;
        tiltedEllipse.endparam = 1.75;
        wrote = dxfWriter_->writeEllipse(&tiltedEllipse) && wrote;

        DRW_3Dface face;
        face.layer = "probe_3dface_wcs";
        face.lineType = "DASHED";
        face.color = 5;
        face.color24 = 0x2468AC;
        face.visible = false;
        face.space = DRW::PaperSpace;
        face.basePoint = DRW_Coord{1.0, 2.0, 3.0};
        face.secPoint = DRW_Coord{4.0, 5.0, 6.0};
        face.thirdPoint = DRW_Coord{7.0, 8.0, 9.0};
        face.fourPoint = DRW_Coord{10.0, 11.0, 12.0};
        face.invisibleflag = DRW_3Dface::FirstEdge | DRW_3Dface::ThirdEdge;
        wrote = dxfWriter_->write3dface(&face) && wrote;

        // SOLID corner coordinates are OCS values. Keep the four corners at
        // one nonzero OCS elevation and use an oblique normal so the legacy
        // extrusion projection has a nontrivial, independently calculable
        // WCS result.
        DRW_Solid solid;
        solid.layer = "probe_solid_oblique_ocs";
        solid.basePoint = DRW_Coord{1.0, 1.0, 2.0};
        solid.secPoint = DRW_Coord{5.0, 1.0, 2.0};
        solid.thirdPoint = DRW_Coord{5.0, 4.0, 2.0};
        solid.fourPoint = DRW_Coord{1.0, 4.0, 2.0};
        solid.extPoint = DRW_Coord{0.0, 1.0, 0.0};
        solid.thickness = 0.25;
        wrote = dxfWriter_->writeSolid(&solid) && wrote;

        DRW_Trace trace;
        trace.layer = "probe_trace_oblique_ocs";
        trace.basePoint = DRW_Coord{2.0, 1.0, 3.0};
        trace.secPoint = DRW_Coord{6.0, 1.0, 3.0};
        trace.thirdPoint = DRW_Coord{6.0, 4.0, 3.0};
        trace.fourPoint = DRW_Coord{2.0, 4.0, 3.0};
        trace.extPoint = DRW_Coord{0.0, 1.0, 0.0};
        trace.thickness = 0.5;
        wrote = dxfWriter_->writeTrace(&trace) && wrote;

        DRW_Ray ray;
        ray.layer = "probe_ray_wcs";
        ray.basePoint = DRW_Coord{3.5, -2.0, 9.0};
        ray.secPoint = DRW_Coord{0.0, 0.0, 1.0};
        wrote = dxfWriter_->writeRay(&ray) && wrote;

        DRW_Xline xline;
        xline.layer = "probe_xline_wcs";
        xline.basePoint = DRW_Coord{-4.0, 6.5, -8.0};
        xline.secPoint = DRW_Coord{0.0, 1.0, 0.0};
        wrote = dxfWriter_->writeXline(&xline) && wrote;

        DRW_Point point;
        point.layer = "probe_point_wcs";
        point.basePoint = DRW_Coord{8.0, -6.0, 14.0};
        point.extPoint = DRW_Coord{0.0, 1.0, 0.0};
        point.thickness = 0.75;
        point.xAxisAngle = 0.25;
        wrote = dxfWriter_->writePoint(&point) && wrote;

        // 3DLINE is a distinct LibreCAD/libdxfrw entity profile. Its endpoints
        // and extrusion vector are stored as direct 3D coordinates, independent
        // of the legacy OCS-to-WCS option used for planar entities.
        DRW_3DLine line3d;
        line3d.layer = "probe_3dline_wcs";
        line3d.basePoint = DRW_Coord{-3.0, 5.0, 11.0};
        line3d.secPoint = DRW_Coord{7.0, -13.0, 17.0};
        line3d.extPoint = DRW_Coord{1.0, 0.0, 0.0};
        line3d.thickness = 0.625;
        wrote = dxfWriter_->write3DLine(&line3d) && wrote;

        DRW_Polyline polyline;
        polyline.flags = 8;
        polyline.vertexcount = 2;
        polyline.addVertex(DRW_Vertex{13.0, 14.0, 15.0, 0.0});
        polyline.addVertex(DRW_Vertex{16.0, 17.0, 18.0, 0.0});
        for (const auto& vertex : polyline.vertlist)
            vertex->flags = 32;
        wrote = dxfWriter_->writePolyline(&polyline) && wrote;

        DRW_Polyline polyface;
        polyface.layer = "probe_pface";
        polyface.flags = 64;
        polyface.vertexcount = 4;
        polyface.facecount = 1;
        polyface.addVertex(DRW_Vertex{1.0, 2.0, 3.0, 0.0});
        polyface.addVertex(DRW_Vertex{4.0, 5.0, 6.0, 0.0});
        polyface.addVertex(DRW_Vertex{7.0, 8.0, 9.0, 0.0});
        polyface.addVertex(DRW_Vertex{10.0, 11.0, 12.0, 0.0});
        for (std::size_t i = 0; i < 4; ++i)
            polyface.vertlist[i]->flags = 192;
        DRW_Vertex faceRecord;
        faceRecord.setDwgSubtype(DRW_Vertex::DwgSubtype::PolyfaceFace);
        faceRecord.vindex1 = 1;
        faceRecord.vindex2 = -2;
        faceRecord.vindex3 = 3;
        faceRecord.vindex4 = -4;
        polyface.addVertex(faceRecord);
        wrote = dxfWriter_->writePolyline(&polyface) && wrote;

        DRW_Mesh mesh;
        mesh.layer = "probe_mesh_geometry";
        mesh.version = 7;
        mesh.blendCrease = true;
        mesh.subdivisionLevel = 5;
        mesh.vertices = {DRW_Coord{20.0, 21.0, 22.0},
                         DRW_Coord{23.0, 24.0, 25.0},
                         DRW_Coord{26.0, 27.0, 28.0}};
        mesh.faces = {{0, 1, 2}};
        mesh.edges = {{0, 1}, {1, 2}};
        mesh.creases = {0.25, 0.75};
        mesh.propertyOverrides = {{17, {0, 2}}, {29, {1, 3}}};
        wrote = dxfWriter_->writeMesh(&mesh) && wrote;

        DRW_Insert insert;
        insert.layer = "probe_insert_grid";
        insert.name = "PROBE_BLOCK";
        insert.basePoint = DRW_Coord{30.0, 31.0, 32.0};
        insert.extPoint = DRW_Coord{0.0, 1.0, 0.0};
        insert.xscale = -2.0;
        insert.yscale = 3.0;
        insert.zscale = 4.0;
        insert.angle = 0.5;
        insert.colcount = 2;
        insert.rowcount = 3;
        insert.colspace = 7.0;
        insert.rowspace = 8.0;
        wrote = dxfWriter_->writeInsert(&insert) && wrote;

        DRW_Insert outerInsert;
        outerInsert.layer = "probe_nested_outer_insert";
        outerInsert.name = "PROBE_NESTED_PARENT";
        outerInsert.basePoint = DRW_Coord{20.0, -5.0, 3.0};
        outerInsert.extPoint = DRW_Coord{0.0, 1.0, 0.0};
        outerInsert.xscale = 2.0;
        outerInsert.yscale = -1.0;
        outerInsert.zscale = 0.75;
        outerInsert.angle = -0.4;
        auto nestedAttribute = std::make_shared<DRW_Attrib>();
        nestedAttribute->tag = "NESTED_ID";
        nestedAttribute->text = "ATTRIBUTE 3D";
        nestedAttribute->basePoint = DRW_Coord{12.0, -4.0, 9.0};
        nestedAttribute->extPoint = DRW_Coord{0.0, 0.0, 1.0};
        nestedAttribute->height = 2.5;
        nestedAttribute->angle = 0.6;
        nestedAttribute->widthscale = 0.8;
        nestedAttribute->oblique = 0.1;
        nestedAttribute->attribFlags = 0;
        nestedAttribute->m_fieldLength = 12;
        nestedAttribute->lockPosition = true;
        nestedAttribute->style = "Standard";
        outerInsert.attlist.push_back(nestedAttribute);
        auto secondNestedAttribute = std::make_shared<DRW_Attrib>();
        secondNestedAttribute->tag = "NESTED_NOTE";
        secondNestedAttribute->text = "ARRAY PARENT";
        secondNestedAttribute->basePoint = DRW_Coord{-3.0, 15.0, 22.0};
        secondNestedAttribute->extPoint = DRW_Coord{0.0, 0.0, 1.0};
        secondNestedAttribute->height = 1.25;
        secondNestedAttribute->angle = -0.2;
        secondNestedAttribute->widthscale = 1.1;
        secondNestedAttribute->attribFlags = 1;
        secondNestedAttribute->m_fieldLength = 5;
        secondNestedAttribute->style = "Standard";
        outerInsert.attlist.push_back(secondNestedAttribute);
        outerInsert.seqEndEntity = std::make_shared<DRW_SeqEnd>();
        wrote = dxfWriter_->writeInsert(&outerInsert) && wrote;

        DRW_Spline spline;
        spline.degree = 2;
        spline.ncontrol = 3;
        spline.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        spline.normalVec = DRW_Coord{0.0, 0.0, 1.0};
        spline.controllist = {
            std::make_shared<DRW_Coord>(40.0, 41.0, 42.0),
            std::make_shared<DRW_Coord>(43.0, 44.0, 45.0),
            std::make_shared<DRW_Coord>(46.0, 47.0, 48.0)};
        wrote = dxfWriter_->writeSpline(&spline) && wrote;

        DRW_Helix helix;
        helix.layer = "probe_helix";
        helix.degree = 2;
        helix.flags = 4;
        helix.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        helix.weightlist = {1.0, 0.75, 1.5};
        helix.controllist = {
            std::make_shared<DRW_Coord>(50.0, 51.0, 52.0),
            std::make_shared<DRW_Coord>(53.0, 54.0, 55.0),
            std::make_shared<DRW_Coord>(56.0, 57.0, 58.0)};
        helix.m_majorVersion = 1;
        helix.m_maintVersion = 2;
        helix.axisBasePt = DRW_Coord{60.0, 61.0, 62.0};
        helix.startPt = DRW_Coord{63.0, 64.0, 65.0};
        helix.axisVector = DRW_Coord{0.0, 0.0, 1.0};
        helix.radius = 2.5;
        helix.turns = 3.25;
        helix.turnHeight = 4.5;
        helix.handedness = true;
        helix.constraintType = 2;
        wrote = dxfWriter_->writeHelix(&helix) && wrote;

        DRW_NurbsSurface nurbs;
        nurbs.layer = "probe_nurbs_surface";
        nurbs.modelerFormatVersion = 2;
        nurbs.short170 = 7;
        nurbs.cvHullDisplay = true;
        nurbs.uvec1 = DRW_Coord{70.0, 71.0, 72.0};
        nurbs.vvec1 = DRW_Coord{73.0, 74.0, 75.0};
        nurbs.uvec2 = DRW_Coord{76.0, 77.0, 78.0};
        nurbs.vvec2 = DRW_Coord{79.0, 80.0, 81.0};
        wrote = dxfWriter_->writeSurface(&nurbs) && wrote;

        DRW_PlaneSurface plane;
        plane.layer = "probe_plane_surface";
        plane.modelerFormatVersion = 3;
        plane.uIsolines = 5;
        plane.vIsolines = 7;
        wrote = dxfWriter_->writeSurface(&plane) && wrote;

        DRW_ExtrudedSurface extruded;
        extruded.layer = "probe_extruded_surface";
        extruded.modelerFormatVersion = 2;
        extruded.uIsolines = 13;
        extruded.vIsolines = 17;
        extruded.classId = 23;
        extruded.sweepVector = DRW_Coord{0.5, 1.5, 2.5};
        extruded.extrudedTransform[3] = 10.0;
        extruded.sweepEntityTransform[7] = 20.0;
        extruded.pathEntityTransform[11] = 30.0;
        extruded.draftAngle = 0.125;
        extruded.draftStartDistance = 1.25;
        extruded.draftEndDistance = 2.5;
        extruded.twistAngle = 0.75;
        extruded.scaleFactor = 1.5;
        extruded.alignAngle = 0.25;
        extruded.solid = true;
        extruded.sweepAlignmentFlags = 2;
        extruded.pathFlags = 8;
        extruded.alignStart = true;
        extruded.bank = true;
        extruded.basePointSet = true;
        extruded.sweepEntityTransformComputed = true;
        extruded.pathEntityTransformComputed = false;
        extruded.referenceVector = DRW_Coord{3.5, 4.5, 5.5};
        wrote = dxfWriter_->writeSurface(&extruded) && wrote;

        DRW_RevolvedSurface revolved;
        revolved.layer = "probe_revolved_surface";
        revolved.modelerFormatVersion = 2;
        revolved.uIsolines = 19;
        revolved.vIsolines = 23;
        revolved.id = 29;
        revolved.axisPoint = DRW_Coord{31.0, 32.0, 33.0};
        revolved.axisVector = DRW_Coord{0.0, 1.0, 0.0};
        revolved.revolveAngle = 1.25;
        revolved.startAngle = 0.375;
        revolved.transform[5] = 12.0;
        revolved.draftAngle = 0.125;
        revolved.draftStartDistance = 1.5;
        revolved.draftEndDistance = 2.75;
        revolved.twistAngle = 0.625;
        revolved.solid = true;
        revolved.closeToAxis = true;
        wrote = dxfWriter_->writeSurface(&revolved) && wrote;

        DRW_SweptSurface swept;
        swept.layer = "probe_swept_surface";
        swept.modelerFormatVersion = 2;
        swept.uIsolines = 29;
        swept.vIsolines = 31;
        swept.sweepEntityId = 37;
        swept.sweepData = {0x41u, 0x42u};
        swept.pathEntityId = 43;
        swept.pathData = {0x51u, 0x52u, 0x53u};
        swept.sweepEntityTransformed[2] = 14.0;
        swept.pathEntityTransformed[4] = 15.0;
        swept.sweepEntityTransform[6] = 16.0;
        swept.pathEntityTransform[8] = 17.0;
        swept.draftAngle = 0.25;
        swept.draftStartDistance = 1.75;
        swept.draftEndDistance = 2.25;
        swept.twistAngle = 0.5;
        swept.scaleFactor = 1.25;
        swept.alignAngle = 0.875;
        swept.solid = true;
        swept.sweepAlignmentFlags = 1;
        swept.alignStart = true;
        swept.bank = true;
        swept.basePointSet = true;
        swept.sweepEntityTransformComputed = true;
        swept.pathEntityTransformComputed = true;
        swept.referenceVector = DRW_Coord{2.5, 3.5, 4.5};
        wrote = dxfWriter_->writeSurface(&swept) && wrote;

        DRW_LWPolyline ocsPolyline;
        ocsPolyline.elevation = 5.0;
        ocsPolyline.extPoint = DRW_Coord{0.0, 1.0, 0.0};
        ocsPolyline.addVertex(DRW_Vertex2D{2.0, 3.0, 0.25});
        ocsPolyline.addVertex(DRW_Vertex2D{4.0, 5.0, 0.0});
        wrote = dxfWriter_->writeLWPolyline(&ocsPolyline) && wrote;

        DRW_LoftedSurface lofted;
        lofted.modelerFormatVersion = 1;
        lofted.uIsolines = 10;
        lofted.vIsolines = 11;
        lofted.dxfReferenceData.emplace_back(90, std::int32_t{2});
        lofted.dxfReferenceData.emplace_back(310,
            std::vector<std::uint8_t>{0x12u, 0x34u, 0x56u});
        wrote = dxfWriter_->writeSurface(&lofted) && wrote;

        // HATCH boundary vertices are stored in OCS; the callback also
        // exposes the hatch elevation and extrusion needed by a consumer to
        // derive WCS coordinates. Keep a non-default normal and elevation so
        // the probe detects accidental loss or projection of either value.
        DRW_Hatch hatch;
        hatch.layer = "probe_hatch_ocs_boundary";
        hatch.name = "SOLID";
        hatch.solid = 1;
        hatch.associative = 0;
        hatch.loopsnum = 1;
        hatch.basePoint = DRW_Coord{0.0, 0.0, 4.0};
        hatch.extPoint = DRW_Coord{1.0, 0.0, 0.0};
        auto hatchLoop = std::make_shared<DRW_HatchLoop>(2);
        auto hatchBoundary = std::make_shared<DRW_LWPolyline>();
        hatchBoundary->flags = 1;
        hatchBoundary->addVertex(DRW_Vertex2D{2.0, 3.0, 0.0});
        hatchBoundary->addVertex(DRW_Vertex2D{5.0, 3.0, 0.0});
        hatchBoundary->addVertex(DRW_Vertex2D{5.0, 7.0, 0.0});
        hatchLoop->objlist.push_back(hatchBoundary);
        hatchLoop->update();
        hatch.appendLoop(hatchLoop);
        wrote = dxfWriter_->writeHatch(&hatch) && wrote;

        DRW_MPolygon mpolygon;
        mpolygon.layer = "probe_mpolygon_ocs_boundary";
        mpolygon.name = "SOLID";
        mpolygon.solid = 1;
        mpolygon.associative = 0;
        mpolygon.dxfVersion = 1;
        mpolygon.loopsnum = 1;
        mpolygon.basePoint = DRW_Coord{0.0, 0.0, 6.0};
        mpolygon.extPoint = DRW_Coord{0.0, 1.0, 0.0};
        mpolygon.fillColorAci = 5;
        mpolygon.fillColorRgb = 0x345678;
        auto mpolygonLoop = std::make_shared<DRW_HatchLoop>(2);
        auto mpolygonBoundary = std::make_shared<DRW_LWPolyline>();
        mpolygonBoundary->flags = 1;
        mpolygonBoundary->addVertex(DRW_Vertex2D{2.0, 3.0, 0.0});
        mpolygonBoundary->addVertex(DRW_Vertex2D{5.0, 3.0, 0.0});
        mpolygonBoundary->addVertex(DRW_Vertex2D{5.0, 7.0, 0.0});
        mpolygonLoop->objlist.push_back(mpolygonBoundary);
        mpolygonLoop->update();
        mpolygon.appendLoop(mpolygonLoop);
        wrote = dxfWriter_->writeMPolygon(&mpolygon) && wrote;
        writeRecipeSucceeded_ = wrote && writeRecipeSucceeded_;
        return;
    }
#endif
    DRW_Line line;
    line.layer = "0";
    line.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    line.secPoint = DRW_Coord{4.0, 5.0, 6.0};
    line.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    line.thickness = 0.25;
    if (dxfWriter_ != nullptr)
        writeRecipeSucceeded_ = dxfWriter_->writeLine(&line) && writeRecipeSucceeded_;
    else if (dwgWriter_ != nullptr)
        writeRecipeSucceeded_ = dwgWriter_->writeLine(&line) && writeRecipeSucceeded_;
    else
        writeRecipeSucceeded_ = false;
}

struct Options {
    std::string input;
    std::string inputPathHint;
    std::string output;
    std::string recipe;
    std::string facade;
    std::string direction = "read";
    std::string inputId;
    std::string originKind = "localFile";
    std::string repository;
    std::string inputCommit;
    std::string sourcePath;
    std::string sourceBlob;
    std::string registryDigest;
    std::string generatorDigest;
    std::string adapterName = LIBDXFRW_SEMANTIC_ADAPTER_NAME;
#ifdef LIBDXFRW_SEMANTIC_SIDE_TARGET
    std::string adapterSide = "target";
#else
    std::string adapterSide = "standalone";
#endif
    std::string package = LIBDXFRW_SEMANTIC_ADAPTER_PACKAGE;
    std::string adapterCommit = LIBDXFRW_SEMANTIC_ADAPTER_COMMIT;
    std::string sourceDigest = LIBDXFRW_SEMANTIC_ADAPTER_SOURCE_DIGEST;
    std::string configDigest = LIBDXFRW_SEMANTIC_ADAPTER_CONFIG_DIGEST;
    std::string staticLibraryDigest = LIBDXFRW_SEMANTIC_ADAPTER_STATIC_LIBRARY_DIGEST;
    std::string binaryDigest = LIBDXFRW_SEMANTIC_ADAPTER_BINARY_DIGEST;
    std::string linkedClosureDigest = LIBDXFRW_SEMANTIC_ADAPTER_LINKED_CLOSURE_DIGEST;
    bool applyExtrusion = false;
    std::string compatibilityProfile = "default";
    bool selfTest = false;
    bool printCapabilityProfile = false;
};

std::string nextArgument(int& index, int argc, char** argv, const std::string& name) {
    if (index + 1 >= argc)
        throw std::runtime_error("missing value for " + name);
    return argv[++index];
}

Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--input") options.input = nextArgument(i, argc, argv, arg);
        else if (arg == "--input-path-hint") options.inputPathHint = nextArgument(i, argc, argv, arg);
        else if (arg == "--output") options.output = nextArgument(i, argc, argv, arg);
        else if (arg == "--recipe") options.recipe = nextArgument(i, argc, argv, arg);
        else if (arg == "--facade") options.facade = nextArgument(i, argc, argv, arg);
        else if (arg == "--direction") options.direction = nextArgument(i, argc, argv, arg);
        else if (arg == "--input-id") options.inputId = nextArgument(i, argc, argv, arg);
        else if (arg == "--origin-kind") options.originKind = nextArgument(i, argc, argv, arg);
        else if (arg == "--repository") options.repository = nextArgument(i, argc, argv, arg);
        else if (arg == "--input-commit") options.inputCommit = nextArgument(i, argc, argv, arg);
        else if (arg == "--source-path") options.sourcePath = nextArgument(i, argc, argv, arg);
        else if (arg == "--source-blob") options.sourceBlob = nextArgument(i, argc, argv, arg);
        else if (arg == "--registry-digest") options.registryDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--generator-digest" || arg == "--generator-sha256") options.generatorDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--adapter-name") options.adapterName = nextArgument(i, argc, argv, arg);
        else if (arg == "--adapter-side") options.adapterSide = nextArgument(i, argc, argv, arg);
        else if (arg == "--package") options.package = nextArgument(i, argc, argv, arg);
        else if (arg == "--adapter-commit") options.adapterCommit = nextArgument(i, argc, argv, arg);
        else if (arg == "--source-digest" || arg == "--source-sha256") options.sourceDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--config-digest" || arg == "--config-sha256") options.configDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--static-library-digest" || arg == "--static-library-sha256") options.staticLibraryDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--binary-digest" || arg == "--binary-sha256") options.binaryDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--linked-closure-digest" || arg == "--linked-library-closure-sha256") options.linkedClosureDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--apply-extrusion") options.applyExtrusion = true;
        else if (arg == "--compatibility-profile") options.compatibilityProfile = nextArgument(i, argc, argv, arg);
        else if (arg == "--json") {}
        else if (arg == "--self-test") options.selfTest = true;
        else if (arg == "--capability-profile") options.printCapabilityProfile = true;
        else throw std::runtime_error("unknown argument: " + arg);
    }
    return options;
}

struct InputInfo {
    std::string digest;
    std::uint64_t size = 0;
    std::string format = "unknown";
    std::string version = "unknown";
};

struct DrawingIdentity {
    std::string format = "unknown";
    std::string version = "unknown";
};

bool validAcVersion(const std::string& value) {
    return value.size() == 6 && value.compare(0, 2, "AC") == 0 &&
        std::all_of(value.begin() + 2, value.end(), [](char ch) {
            return std::isdigit(static_cast<unsigned char>(ch)) != 0;
        });
}

std::string trimAscii(const std::string& value) {
    std::size_t begin = 0;
    while (begin < value.size() &&
           std::isspace(static_cast<unsigned char>(value[begin])) != 0)
        ++begin;
    std::size_t end = value.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
        --end;
    return value.substr(begin, end - begin);
}

bool parseDxfGroupCode(const std::string& text, int& code) {
    const std::string normalized = trimAscii(text);
    if (normalized.empty())
        return false;
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(normalized.c_str(), &end, 10);
    if (errno != 0 || end == normalized.c_str() || *end != '\0' ||
        value < 0 || value > 1071)
        return false;
    code = static_cast<int>(value);
    return true;
}

std::string asciiDxfVersion(const std::string& prefix) {
    if (prefix.find('\0') != std::string::npos)
        return std::string();
    std::vector<std::string> lines;
    std::size_t begin = 0;
    while (begin < prefix.size()) {
        const std::size_t newline = prefix.find('\n', begin);
        std::string line = prefix.substr(begin, newline == std::string::npos
            ? std::string::npos : newline - begin);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
        if (newline == std::string::npos)
            break;
        begin = newline + 1;
    }
    bool expectSectionName = false;
    bool inHeader = false;
    bool expectVersion = false;
    for (std::size_t index = 0; index + 1 < lines.size(); index += 2) {
        int code = 0;
        if (!parseDxfGroupCode(lines[index], code))
            return std::string();
        const std::string value = trimAscii(lines[index + 1]);
        if (expectSectionName) {
            inHeader = code == 2 && value == "HEADER";
            expectSectionName = false;
        }
        if (expectVersion) {
            return code == 1 && validAcVersion(value) ? value : std::string();
        }
        if (inHeader && code == 9 && value == "$ACADVER") {
            expectVersion = true;
        } else if (code == 0 && value == "SECTION") {
            expectSectionName = true;
            inHeader = false;
        } else if (code == 0 && value == "ENDSEC") {
            inHeader = false;
        }
    }
    return std::string();
}

std::string binaryDxfVersion(const std::string& prefix) {
    std::string sentinel("AutoCAD Binary DXF\r\n");
    sentinel.push_back(static_cast<char>(0x1a));
    sentinel.push_back('\0');
    if (prefix.size() < sentinel.size() || prefix.compare(0, sentinel.size(), sentinel) != 0)
        return std::string();

    std::vector<std::string> markers;
    std::string modern;
    modern.push_back(static_cast<char>(9));
    modern.push_back('\0');
    modern.append("$ACADVER");
    modern.push_back('\0');
    modern.push_back(static_cast<char>(1));
    modern.push_back('\0');
    markers.push_back(modern);
    std::string legacy;
    legacy.push_back(static_cast<char>(9));
    legacy.append("$ACADVER");
    legacy.push_back('\0');
    legacy.push_back(static_cast<char>(1));
    markers.push_back(legacy);
    for (const std::string& marker : markers) {
        const std::size_t found = prefix.find(marker, sentinel.size());
        if (found == std::string::npos)
            continue;
        const std::size_t versionOffset = found + marker.size();
        if (versionOffset + 6 < prefix.size()) {
            const std::string version = prefix.substr(versionOffset, 6);
            if (validAcVersion(version) && prefix[versionOffset + 6] == '\0')
                return version;
        }
    }
    return std::string();
}

DrawingIdentity classifyDrawingPrefix(const std::string& prefix) {
    if (prefix.size() >= 6) {
        const std::string version = prefix.substr(0, 6);
        if (validAcVersion(version))
            return DrawingIdentity{"dwg", version};
    }
    const std::string binaryVersion = binaryDxfVersion(prefix);
    if (!binaryVersion.empty())
        return DrawingIdentity{"dxf-binary", binaryVersion};
    const std::string asciiVersion = asciiDxfVersion(prefix);
    if (!asciiVersion.empty())
        return DrawingIdentity{"dxf-ascii", asciiVersion};
    return DrawingIdentity{};
}

InputInfo inspectInput(const std::string& path) {
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
        throw std::runtime_error("unable to open input");
    Sha256 hash;
    std::array<char, 65536> buffer{};
    std::string prefix;
    InputInfo info;
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = input.gcount();
        if (count <= 0)
            continue;
        if (prefix.size() < 65536) {
            const std::size_t remaining = 65536 - prefix.size();
            prefix.append(buffer.data(), std::min<std::size_t>(remaining,
                static_cast<std::size_t>(count)));
        }
        hash.update(buffer.data(), static_cast<std::size_t>(count));
        info.size += static_cast<std::uint64_t>(count);
    }
    if (input.bad())
        throw std::runtime_error("error reading input");
    info.digest = hash.finish();
    const DrawingIdentity identity = classifyDrawingPrefix(prefix);
    info.format = identity.format;
    info.version = identity.version;
    return info;
}

std::string logicalBasename(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    const std::string value = slash == std::string::npos ? path : path.substr(slash + 1);
    return value.empty() ? "input" : value;
}

bool validPathHint(const std::string& path) {
    if (path.empty() || path.front() == '/' || path.find('\\') != std::string::npos)
        return false;
    std::size_t begin = 0;
    while (begin <= path.size()) {
        const std::size_t end = path.find('/', begin);
        const std::string part = path.substr(begin, end == std::string::npos
            ? std::string::npos : end - begin);
        if (part.empty() || part == "." || part == "..")
            return false;
        if (end == std::string::npos)
            break;
        begin = end + 1;
    }
    return true;
}

std::string minimalLineRecipe() {
    return "{\"end\":[4,5,6],\"entity\":\"LINE\",\"extrusion\":[0,0,1],"
           "\"id\":\"minimal-line-v1\",\"start\":[1,2,3],\"thickness\":0.25,"
           "\"version\":\"AC1024\"}\n";
}

void verifyMinimalLineRecipe(const std::string& path) {
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
        throw std::runtime_error("unable to open local recipe: " + path);
    std::ostringstream bytes;
    bytes << input.rdbuf();
    if (input.bad())
        throw std::runtime_error("error reading local recipe: " + path);
    if (bytes.str() != minimalLineRecipe())
        throw std::runtime_error("local recipe does not match minimal-line-v1");
}

InputInfo recipeInputInfo(const std::string& facade) {
    const std::string recipe = minimalLineRecipe();
    InputInfo info;
    info.digest = sha256(recipe);
    info.size = recipe.size();
    info.format = facade == "dwgRW" ? "generated-dwg-recipe" : "generated-dxf-recipe";
    info.version = "AC1024";
    return info;
}

std::string failureStage(int error) {
    switch (error) {
    case static_cast<int>(DRW::BAD_NONE): return "none";
    case static_cast<int>(DRW::BAD_UNKNOWN): return "unknown";
    case static_cast<int>(DRW::BAD_OPEN): return "open";
    case static_cast<int>(DRW::BAD_VERSION): return "version";
    case static_cast<int>(DRW::BAD_READ_METADATA): return "metadata";
    case static_cast<int>(DRW::BAD_READ_FILE_HEADER): return "fileHeader";
    case static_cast<int>(DRW::BAD_READ_HEADER): return "header";
    case static_cast<int>(DRW::BAD_READ_HANDLES): return "handles";
    case static_cast<int>(DRW::BAD_READ_CLASSES): return "classes";
    case static_cast<int>(DRW::BAD_READ_TABLES): return "tables";
    case static_cast<int>(DRW::BAD_READ_BLOCKS): return "blocks";
    case static_cast<int>(DRW::BAD_READ_ENTITIES): return "entities";
    case static_cast<int>(DRW::BAD_READ_OBJECTS): return "objects";
    case static_cast<int>(DRW::BAD_READ_SECTION): return "section";
    case static_cast<int>(DRW::BAD_CODE_PARSED): return "parseCode";
    default: return "unknown";
    }
}

template <typename T, typename Writer>
void writeArray(std::ostream& out, const std::vector<T>& values, Writer writer) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) out << ',';
        writer(out, values[i]);
    }
    out << ']';
}

void writeFields(std::ostream& out, const std::vector<Field>& fields) {
    writeArray(out, fields, [](std::ostream& stream, const Field& field) {
        stream << "{\"name\":" << jsonString(field.name)
               << ",\"type\":" << jsonString(field.type)
               << ",\"value\":" << field.value << '}';
    });
}

template <typename Prefix>
void writeBytePrefixFields(std::ostream& out, const Prefix& prefix) {
    if (prefix.length > prefix.bytes.size())
        throw std::runtime_error("DWG observation prefix exceeds its storage");
    out << "\"bytesHex\":\"" << std::hex << std::nouppercase
        << std::setfill('0');
    for (std::size_t i = 0; i < prefix.length; ++i)
        out << std::setw(2) << static_cast<unsigned>(prefix.bytes[i]);
    out << std::dec << "\",\"prefixLength\":" << prefix.length
        << ",\"sourceLength\":" << prefix.sourceBytes
        << ",\"truncated\":"
        << (prefix.truncated() ? "true" : "false");
}

template <typename Prefix>
void writeBytePrefix(std::ostream& out, const Prefix& prefix) {
    out << '{';
    writeBytePrefixFields(out, prefix);
    out << '}';
}

template <typename Summary>
void writeNamedSummary(std::ostream& out, const Summary& summary) {
    if (summary.retained > summary.rows.size())
        throw std::runtime_error("DWG named observation count exceeds its bound");
    out << "{\"rows\":[";
    for (std::size_t i = 0; i < summary.retained; ++i) {
        if (i != 0) out << ',';
        out << '{';
        writeBytePrefixFields(out, summary.rows[i].name);
        out << ",\"instances\":" << summary.rows[i].instances << '}';
    }
    out << "],\"retainedRowCount\":" << summary.retained
        << ",\"sourceNameCount\":" << summary.sourceNames
        << ",\"totalInstanceCount\":" << summary.instances
        << ",\"omittedNameCount\":" << summary.omittedNames
        << ",\"omittedInstanceCount\":" << summary.omittedInstances
        << ",\"truncatedRetainedNameCount\":"
        << summary.truncatedRetainedNames
        << ",\"omittedNameBytes\":" << summary.omittedNameBytes
        << ",\"aggregateSaturated\":"
        << (summary.aggregateSaturated ? "true" : "false") << '}';
}

template <typename Integrity>
void writeIntegrity(std::ostream& out, const Integrity& integrity) {
    if (integrity.retained > integrity.rows.size())
        throw std::runtime_error("DWG integrity observation count exceeds its bound");
    out << "{\"rows\":[";
    for (std::size_t i = 0; i < integrity.retained; ++i) {
        if (i != 0) out << ',';
        const auto& row = integrity.rows[i];
        out << "{\"versionCode\":" << static_cast<unsigned>(row.version)
            << ",\"severityCode\":" << static_cast<unsigned>(row.severity)
            << ",\"offsetSpaceCode\":" << static_cast<unsigned>(row.offsetSpace)
            << ",\"phaseCode\":" << static_cast<unsigned>(row.phase)
            << ",\"kindCode\":" << static_cast<unsigned>(row.kind)
            << ",\"logicalSectionId\":" << row.logicalSectionId
            << ",\"sectionDescriptorId\":" << row.sectionDescriptorId
            << ",\"sectionName\":";
        writeBytePrefix(out, row.sectionName);
        out << ",\"pageId\":" << row.pageId
            << ",\"fileOffset\":" << row.fileOffset
            << ",\"logicalHandle\":" << row.logicalHandle
            << ",\"expected\":" << row.expected
            << ",\"observed\":" << row.observed
            << ",\"hasPageId\":" << (row.hasPageId ? "true" : "false")
            << ",\"hasFileOffset\":" << (row.hasFileOffset ? "true" : "false")
            << ",\"hasLogicalHandle\":"
            << (row.hasLogicalHandle ? "true" : "false")
            << ",\"hasExpected\":" << (row.hasExpected ? "true" : "false")
            << ",\"hasObserved\":" << (row.hasObserved ? "true" : "false")
            << '}';
    }
    out << "],\"retainedRowCount\":" << integrity.retained
        << ",\"sourceRecordedCount\":" << integrity.sourceRecorded
        << ",\"extractionOmittedCount\":" << integrity.extractionOmitted
        << ",\"reportedUpstreamDroppedLowerBound\":"
        << integrity.reportedUpstreamDropped
        << ",\"truncatedRetainedNameCount\":"
        << integrity.truncatedRetainedNames
        << ",\"omittedNameBytes\":" << integrity.omittedNameBytes
        << ",\"aggregateSaturated\":"
        << (integrity.aggregateSaturated ? "true" : "false") << '}';
}

template <typename Frames>
void writeFrameObservations(std::ostream& out, const Frames& frames) {
    const int status = static_cast<int>(frames.status);
    const char* statusName = nullptr;
    switch (status) {
    case 1: statusName = "inProgress"; break;
    case 2: statusName = "finalizedComplete"; break;
    case 3: statusName = "finalizedPartial"; break;
    default: statusName = "notAvailable"; break;
    }
    out << "{\"status\":" << jsonString(statusName);
    if (status <= 0 || status > 3) {
        out << ",\"reason\":\"coverageNotPublished\",\"sourceFrameCount\":null"
            << ",\"dispositions\":{\"pending\":null,\"deferred\":null"
            << ",\"staged\":null,\"published\":null,\"quarantined\":null"
            << ",\"failed\":null,\"unresolved\":null}"
            << ",\"unrecognizedDispositionCount\":null}";
        return;
    }
    out << ",\"reason\":null,\"sourceFrameCount\":" << frames.sourceFrames
        << ",\"dispositions\":{\"pending\":" << frames.dispositions[0]
        << ",\"deferred\":" << frames.dispositions[1]
        << ",\"staged\":" << frames.dispositions[2]
        << ",\"published\":" << frames.dispositions[3]
        << ",\"quarantined\":" << frames.dispositions[4]
        << ",\"failed\":" << frames.dispositions[5]
        << ",\"unresolved\":" << frames.dispositions[6] << '}'
        << ",\"unrecognizedDispositionCount\":"
        << frames.unrecognizedDispositions << '}';
}

template <typename Observations>
std::string serializeAvailableObservations(const Observations& observations,
                                           const std::string& phase,
                                           const std::string& sourceVersion) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "{\"phase\":" << jsonString(phase)
        << ",\"availability\":\"available\",\"reason\":null"
        << ",\"sourceVersion\":"
        << jsonString(sourceVersion.empty() ? "unknown" : sourceVersion)
        << ",\"scalarCounters\":{\"entityParseFailures\":"
        << observations.entityParseFailures
        << ",\"objectParseFailures\":" << observations.objectParseFailures
        << ",\"classesCrcMismatch\":" << observations.classesCrcMismatch
        << ",\"reader21CrcMismatch\":" << observations.reader21CrcMismatch
        << ",\"reader18HeaderCrcMismatch\":"
        << observations.reader18HeaderCrcMismatch
        << ",\"dataStorageLinkFailures\":"
        << observations.dataStorageLinkFailures
        << ",\"dataStorageOrphanRecords\":"
        << observations.dataStorageOrphanRecords
        << ",\"decodedProxyPrimitives\":"
        << observations.decodedProxyPrimitives << '}'
        << ",\"namedSummaries\":{\"skippedCustomClasses\":";
    writeNamedSummary(out, observations.skippedCustomClasses);
    out << ",\"skippedUnsupportedObjects\":";
    writeNamedSummary(out, observations.skippedUnsupportedObjects);
    out << "},\"integrity\":";
    writeIntegrity(out, observations.integrity);
    out << ",\"frames\":";
    writeFrameObservations(out, observations.frames);
    out << '}';
    return out.str();
}

std::string unavailableObservationEnvelope(const std::string& phase,
                                          const std::string& availability,
                                          const std::string& reason,
                                          const std::string& sourceVersion = "") {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "{\"phase\":" << jsonString(phase)
        << ",\"availability\":" << jsonString(availability)
        << ",\"reason\":" << jsonString(reason)
        << ",\"sourceVersion\":" << jsonNullable(sourceVersion)
        << ",\"scalarCounters\":null,\"namedSummaries\":null"
        << ",\"integrity\":null,\"frames\":null}";
    return out.str();
}

template <typename Reader>
std::string serializeDwgObservationEnvelope(const Reader& reader,
                                           const std::string& phase,
                                           const std::string& sourceVersion) {
    if constexpr (HasDwgReadObservations<Reader>::value) {
        const auto observations = reader.getReadObservations();
        return serializeAvailableObservations(observations, phase, sourceVersion);
    }
    return unavailableObservationEnvelope(
        phase, "unavailable", "readerApiNotPresent", sourceVersion);
}

std::string graphEdgeDisposition(const SemanticSink& sink,
                                const EdgeRow& edge) {
    if (edge.kind != "historyReference" && edge.kind != "owner")
        return edge.disposition;
    for (const NodeRow& node : sink.nodes) {
        const bool validTargetKind = edge.kind == "owner"
            ? (node.kind == "handle" || node.kind == "block")
            : node.kind == "handle";
        if (node.id != edge.to || !validTargetKind || node.handle.empty())
            continue;
        for (const RecordRow& record : sink.records) {
            if (record.sourceHandle == node.handle)
                return "resolved";
        }
        return "unresolved";
    }
    return "unresolved";
}

void emitResult(std::ostream& out, const Options& options, const InputInfo& input,
                const SemanticSink& sink, bool succeeded, int error,
                const std::string& stage, const std::string& inputReadObservations,
                const std::string& outputReadbackObservations) {
    const std::string optionsCanonical =
        std::string("{\"applyExtrusion\":") +
        (options.applyExtrusion ? "true" : "false") +
        ",\"compatibilityProfile\":" + jsonString(options.compatibilityProfile) + "}";
    std::string firstDiagnostic;
    std::string firstStage = stage;
    std::string firstPath = "/input";
    int firstCode = error;
    if (!succeeded) {
        for (const DiagnosticRow& diagnostic : sink.diagnostics) {
            if (diagnostic.severity == "error") {
                firstDiagnostic = diagnostic.id;
                firstStage = diagnostic.stage;
                firstPath = diagnostic.path;
                const std::string::size_type separator = diagnostic.code.rfind('-');
                if (separator != std::string::npos && separator + 1 < diagnostic.code.size()) {
                    const char* number = diagnostic.code.c_str() + separator + 1;
                    char* end = nullptr;
                    errno = 0;
                    const long parsed = std::strtol(number, &end, 10);
                    if (errno == 0 && end != number && *end == '\0' &&
                        parsed >= std::numeric_limits<int>::min() &&
                        parsed <= std::numeric_limits<int>::max())
                        firstCode = static_cast<int>(parsed);
                }
                break;
            }
        }
    }
    out.imbue(std::locale::classic());
    out << "{\"schema\":3,\"kind\":\"libdxfrw-semantic-adapter-result\"";
    out << ",\"input\":{\"id\":" << jsonString(options.inputId)
        << ",\"pathHint\":" << jsonString(options.inputPathHint)
        << ",\"originKind\":" << jsonString(options.originKind)
        << ",\"repository\":" << jsonNullable(options.repository)
        << ",\"commit\":" << jsonNullable(options.inputCommit)
        << ",\"sourcePath\":" << jsonNullable(options.sourcePath)
        << ",\"sourceBlob\":" << jsonNullable(options.sourceBlob)
        << ",\"registryDigest\":" << jsonNullable(options.registryDigest)
        << ",\"generatorDigest\":" << jsonNullable(options.generatorDigest)
        << ",\"sha256\":" << jsonString(input.digest)
        << ",\"byteSize\":" << input.size
        << ",\"detectedFormat\":" << jsonString(input.format)
        << ",\"detectedVersion\":" << jsonString(input.version) << '}';
    out << ",\"adapter\":{\"name\":" << jsonString(options.adapterName)
        << ",\"side\":" << jsonString(options.adapterSide)
        << ",\"package\":" << jsonString(options.package)
        << ",\"commit\":" << jsonString(options.adapterCommit)
        << ",\"sourceDigest\":" << jsonString(options.sourceDigest)
        << ",\"configDigest\":" << jsonString(options.configDigest)
        << ",\"staticLibraryDigest\":" << jsonString(options.staticLibraryDigest)
        << ",\"binaryDigest\":" << jsonString(options.binaryDigest)
        << ",\"linkedClosureDigest\":" << jsonString(options.linkedClosureDigest) << '}';
    out << ",\"invocation\":{\"facade\":" << jsonString(options.facade)
        << ",\"direction\":" << jsonString(options.direction)
        << ",\"options\":{\"applyExtrusion\":" << (options.applyExtrusion ? "true" : "false")
        << ",\"compatibilityProfile\":" << jsonString(options.compatibilityProfile) << '}'
        << ",\"optionsDigest\":" << jsonString(sha256(optionsCanonical)) << '}';
    out << ",\"status\":{\"outcome\":" << jsonString(succeeded ? "success" : "failure")
        << ",\"operationSucceeded\":" << (succeeded ? "true" : "false")
        << ",\"exitCode\":" << (succeeded ? 0 : 1)
        << ",\"firstFailure\":{\"stage\":" << (succeeded ? "null" : jsonString(firstStage))
        << ",\"code\":" << (succeeded ? "null" : std::to_string(firstCode))
        << ",\"path\":" << (succeeded ? "null" : jsonString(firstPath))
        << ",\"diagnosticId\":" << jsonNullable(firstDiagnostic) << "}}";

    out << ",\"callbacks\":";
    writeArray(out, sink.callbacks, [](std::ostream& stream, const CallbackRow& row) {
        stream << "{\"id\":" << jsonString(row.id) << ",\"ordinal\":" << row.ordinal
               << ",\"kind\":" << jsonString(row.kind)
               << ",\"recordId\":" << jsonNullable(row.recordId)
               << ",\"blockContextId\":" << jsonNullable(row.blockContextId)
               << ",\"carrierIds\":";
        writeArray(stream, row.carrierIds, [](std::ostream& s, const std::string& id) { s << jsonString(id); });
        stream << '}';
    });
    out << ",\"records\":";
    writeArray(out, sink.records, [](std::ostream& stream, const RecordRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"callbackOrdinal\":" << row.callbackOrdinal
               << ",\"kind\":" << jsonString(row.kind)
               << ",\"recordClass\":" << jsonString(row.recordClass)
               << ",\"entity\":" << jsonString(row.entity)
               << ",\"sourceHandle\":" << jsonNullable(row.sourceHandle)
               << ",\"ownerHandle\":" << jsonNullable(row.ownerHandle)
               << ",\"blockId\":" << jsonNullable(row.blockId)
               << ",\"fields\":";
        writeFields(stream, row.fields);
        stream << '}';
    });
    out << ",\"graphNodes\":";
    writeArray(out, sink.nodes, [](std::ostream& stream, const NodeRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"kind\":" << jsonString(row.kind)
               << ",\"recordId\":" << jsonNullable(row.recordId)
               << ",\"handle\":" << jsonNullable(row.handle) << '}';
    });
    out << ",\"graphEdges\":";
    writeArray(out, sink.edges, [&sink](std::ostream& stream, const EdgeRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"kind\":" << jsonString(row.kind)
               << ",\"from\":" << jsonString(row.from)
               << ",\"to\":" << jsonString(row.to)
               << ",\"disposition\":"
               << jsonString(graphEdgeDisposition(sink, row)) << '}';
    });
    out << ",\"opaqueCarriers\":";
    writeArray(out, sink.carriers, [](std::ostream& stream, const CarrierRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"source\":" << jsonString(row.source)
               << ",\"byteSize\":" << row.size
               << ",\"sha256\":" << jsonString(row.digest)
               << ",\"disposition\":" << jsonString(row.disposition)
               << ",\"recordId\":" << jsonNullable(row.recordId)
               << ",\"sourceHandle\":" << jsonNullable(row.sourceHandle) << '}';
    });
    out << ",\"unsupportedContent\":";
    writeArray(out, sink.unsupported, [](std::ostream& stream, const UnsupportedRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"kind\":" << jsonString(row.kind)
               << ",\"source\":" << jsonString(row.source)
               << ",\"recordId\":" << jsonNullable(row.recordId)
               << ",\"carrierId\":" << jsonNullable(row.carrierId)
               << ",\"diagnosticId\":" << jsonNullable(row.diagnosticId)
               << ",\"disposition\":" << jsonString(row.disposition)
               << ",\"reason\":" << jsonString(row.reason) << '}';
    });
    out << ",\"diagnostics\":";
    writeArray(out, sink.diagnostics, [](std::ostream& stream, const DiagnosticRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"ordinal\":" << row.index
               << ",\"severity\":" << jsonString(row.severity)
               << ",\"stage\":" << jsonString(row.stage)
               << ",\"code\":" << jsonString(row.code)
               << ",\"path\":" << jsonString(row.path)
               << ",\"messageDigest\":" << jsonString(row.messageDigest) << '}';
    });
    out << ",\"cardinality\":{\"callbackCount\":" << sink.callbacks.size()
        << ",\"recordCount\":" << sink.records.size()
        << ",\"fieldCount\":" << sink.fieldCount()
        << ",\"graphNodeCount\":" << sink.nodes.size()
        << ",\"graphEdgeCount\":" << sink.edges.size()
        << ",\"opaqueCarrierCount\":" << sink.carriers.size()
        << ",\"unsupportedContentCount\":" << sink.unsupported.size()
        << ",\"diagnosticCount\":" << sink.diagnostics.size() << '}';
    out << ",\"observations\":{\"inputRead\":"
        << inputReadObservations
        << ",\"outputReadback\":" << outputReadbackObservations << "}}\n";
}

const Field* findEntityField(const SemanticSink& sink, const std::string& entity,
                             const std::string& name) {
    for (const RecordRow& record : sink.records) {
        if (record.entity != entity)
            continue;
        for (const Field& field : record.fields) {
            if (field.name == name)
                return &field;
        }
    }
    return nullptr;
}

bool fieldEquals(const SemanticSink& sink, const std::string& entity,
                 const std::string& name, const std::string& expected) {
    const Field* field = findEntityField(sink, entity, name);
    return field != nullptr && field->value == expected;
}

bool hasExcludedField(const SemanticSink& sink, const std::string& source) {
    return std::any_of(sink.unsupported.begin(), sink.unsupported.end(),
        [&source](const UnsupportedRow& row) {
            return row.disposition == "excluded"
                && row.reason == "adapterFieldCoverageExcluded"
                && row.source.find(source) != std::string::npos;
        });
}

#ifdef LIBDXFRW_SEMANTIC_3D_CONSUMER_PROBE
const RecordRow* findEntityRecordByLayer(const SemanticSink& sink,
                                         const std::string& entity,
                                         const std::string& layer) {
    for (const RecordRow& record : sink.records) {
        if (record.entity != entity)
            continue;
        for (const Field& field : record.fields) {
            if (field.name == "layer" && field.value == jsonString(layer))
                return &record;
        }
    }
    return nullptr;
}

const Field* findRecordField(const RecordRow* record, const std::string& name) {
    if (record == nullptr)
        return nullptr;
    for (const Field& field : record->fields) {
        if (field.name == name)
            return &field;
    }
    return nullptr;
}

bool fieldEqualsForLayer(const SemanticSink& sink, const std::string& entity,
                         const std::string& layer, const std::string& name,
                         const std::string& expected) {
    const Field* field = findRecordField(
        findEntityRecordByLayer(sink, entity, layer), name);
    return field != nullptr && field->value == expected;
}

bool doubleFieldEqualsForLayer(const SemanticSink& sink,
                               const std::string& entity,
                               const std::string& layer,
                               const std::string& name, double expected) {
    const Field* field = findRecordField(
        findEntityRecordByLayer(sink, entity, layer), name);
    if (field == nullptr)
        return false;
    try {
        const double actual = std::stod(field->value);
        return std::isfinite(actual)
            && std::fabs(actual - expected) <= 1.0e-12;
    } catch (const std::exception&) {
        return false;
    }
}

bool run3DConsumerProbe(bool binary) {
    std::error_code error;
    const std::filesystem::path temporaryRoot =
        std::filesystem::temp_directory_path(error);
    if (error)
        return false;
    std::filesystem::path directory;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned attempt = 0; attempt < 32; ++attempt) {
        directory = temporaryRoot / ("libdxfrw-semantic-3d-consumer-"
            + std::to_string(stamp) + "-" + std::to_string(attempt));
        error.clear();
        if (std::filesystem::create_directory(directory, error))
            break;
        if (error && error != std::errc::file_exists)
            return false;
        directory.clear();
    }
    if (directory.empty())
        return false;

    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code cleanupError;
            std::filesystem::remove_all(path, cleanupError);
        }
    } cleanup{directory};

    const std::filesystem::path output = directory
        / (binary ? "consumer-probe-binary.dxf" : "consumer-probe-ascii.dxf");
    const std::string outputName = output.string();
    SemanticSink producer("dxfRW", "write");
    producer.enable3DConsumerProbe();
    dxfRW writer(outputName.c_str());
    producer.configureWrite(&writer);
    const bool writeSucceeded = writer.write(&producer, DRW::AC1027, binary);
    if (!writeSucceeded || !producer.writeRecipeSucceeded()) {
        std::cerr << "3D consumer probe write failed: binary=" << binary
                  << ", writer=" << writeSucceeded
                  << ", recipe=" << producer.writeRecipeSucceeded()
                  << ", error=" << static_cast<int>(writer.getError()) << '\n';
        return false;
    }

    SemanticSink consumer("dxfRW", "read");
    consumer.enable3DConsumerProbe();
    dxfRW reader(outputName.c_str());
    const bool readSucceeded = reader.read(&consumer, false);
    if (!readSucceeded || !consumer.diagnostics.empty()) {
        std::cerr << "3D consumer probe read failed: binary=" << binary
                  << ", reader=" << readSucceeded
                  << ", error=" << static_cast<int>(reader.getError())
                  << ", diagnostics=" << consumer.diagnostics.size() << '\n';
        return false;
    }

    // Preserve the established 2D extrusion option independently of the
    // unprojected callback values checked below. For this LWPOLYLINE,
    // extrusion (0,1,0) gives arbitrary-axis Ax=(-1,0,0), Ay=(0,0,1);
    // local (x,y,elevation) therefore maps to (-x,elevation,y):
    // (2,3,5) -> (-2,5,3) and (4,5,5) -> (-4,5,5).
    SemanticSink legacyConsumer("dxfRW", "read");
    legacyConsumer.enable3DConsumerProbe();
    dxfRW legacyReader(outputName.c_str());
    const bool legacyReadSucceeded = legacyReader.read(&legacyConsumer, true);
    if (!legacyReadSucceeded || !legacyConsumer.diagnostics.empty()) {
        std::cerr << "3D consumer legacy read failed: binary=" << binary
                  << ", reader=" << legacyReadSucceeded
                  << ", error=" << static_cast<int>(legacyReader.getError())
                  << ", diagnostics=" << legacyConsumer.diagnostics.size() << '\n';
        return false;
    }
    const auto expectEntityField = [&](const SemanticSink& sink,
                                       const std::string& entity,
                                       const std::string& layer,
                                       const std::string& name,
                                       const std::string& expected) {
        if (fieldEqualsForLayer(sink, entity, layer, name, expected))
            return true;
        const Field* field = findRecordField(
            findEntityRecordByLayer(sink, entity, layer), name);
        std::cerr << "3D consumer probe mismatch for " << entity
                  << " layer " << layer << " field " << name << ": expected "
                  << expected << ", got "
                  << (field == nullptr ? "<missing>" : field->value) << '\n';
        return false;
    };
    const auto expectEntityDouble = [&](const SemanticSink& sink,
                                        const std::string& entity,
                                        const std::string& layer,
                                        const std::string& name,
                                        double expected) {
        if (doubleFieldEqualsForLayer(sink, entity, layer, name, expected))
            return true;
        const Field* field = findRecordField(
            findEntityRecordByLayer(sink, entity, layer), name);
        std::cerr << "3D consumer probe mismatch for " << entity
                  << " layer " << layer << " field " << name << ": expected "
                  << number(expected) << ", got "
                  << (field == nullptr ? "<missing>" : field->value) << '\n';
        return false;
    };

    // ARC/CIRCLE store planar centers in OCS. The default normal remains a
    // no-op; an oblique normal uses the DXF arbitrary-axis basis; the legacy
    // negative-Z ARC path also mirrors/swaps angles while retaining its normal.
    const std::string defaultCircleLayer = "probe_circle_default";
    const std::string obliqueCircleLayer = "probe_circle_oblique";
    const std::string negativeArcLayer = "probe_arc_negative_normal";
    const double rawArcStart = 10.0 / ARAD;
    const double rawArcEnd = 70.0 / ARAD;
    if (!expectEntityField(consumer, "CIRCLE", defaultCircleLayer, "center",
                           "{\"x\":6,\"y\":-3,\"z\":1.5}")
        || !expectEntityField(consumer, "CIRCLE", defaultCircleLayer, "extrusion",
                              "{\"x\":0,\"y\":0,\"z\":1}")
        || !expectEntityDouble(consumer, "CIRCLE", defaultCircleLayer, "radius", 2.5)
        || !expectEntityDouble(consumer, "CIRCLE", defaultCircleLayer, "thickness", 0.75)
        || !expectEntityField(consumer, "CIRCLE", obliqueCircleLayer, "center",
                              "{\"x\":2,\"y\":3,\"z\":5}")
        || !expectEntityField(consumer, "CIRCLE", obliqueCircleLayer, "extrusion",
                              "{\"x\":0,\"y\":1,\"z\":0}")
        || !expectEntityDouble(consumer, "CIRCLE", obliqueCircleLayer, "radius", 4.0)
        || !expectEntityDouble(consumer, "CIRCLE", obliqueCircleLayer, "thickness", 0.5)
        || !expectEntityField(consumer, "ARC", negativeArcLayer, "center",
                              "{\"x\":2,\"y\":3,\"z\":4}")
        || !expectEntityField(consumer, "ARC", negativeArcLayer, "extrusion",
                              "{\"x\":0,\"y\":0,\"z\":-1}")
        || !expectEntityDouble(consumer, "ARC", negativeArcLayer, "radius", 7.0)
        || !expectEntityDouble(consumer, "ARC", negativeArcLayer, "thickness", 0.125)
        || !expectEntityDouble(consumer, "ARC", negativeArcLayer,
                               "startAngleRadians", rawArcStart)
        || !expectEntityDouble(consumer, "ARC", negativeArcLayer,
                               "endAngleRadians", rawArcEnd))
        return false;

    const auto expectObliqueQuad = [&](const SemanticSink& sink,
                                       const std::string& entity,
                                       const std::string& layer,
                                       const std::array<DRW_Coord, 4>& corners,
                                       double thickness) {
        for (std::size_t i = 0; i < corners.size(); ++i) {
            if (!expectEntityField(sink, entity, layer,
                                   "corner." + std::to_string(i),
                                   pointJson(corners[i])))
                return false;
        }
        return expectEntityField(sink, entity, layer, "extrusion",
                                 pointJson(DRW_Coord{0.0, 1.0, 0.0}))
            && expectEntityDouble(sink, entity, layer, "thickness", thickness);
    };
    if (!expectObliqueQuad(consumer, "SOLID", "probe_solid_oblique_ocs", {{
            DRW_Coord{1.0, 1.0, 2.0}, DRW_Coord{5.0, 1.0, 2.0},
            DRW_Coord{5.0, 4.0, 2.0}, DRW_Coord{1.0, 4.0, 2.0}}}, 0.25)
        || !expectObliqueQuad(consumer, "TRACE", "probe_trace_oblique_ocs", {{
            DRW_Coord{2.0, 1.0, 3.0}, DRW_Coord{6.0, 1.0, 3.0},
            DRW_Coord{6.0, 4.0, 3.0}, DRW_Coord{2.0, 4.0, 3.0}}}, 0.5))
        return false;
    const auto expectRayAndXline = [&](const SemanticSink& sink) {
        return expectEntityField(sink, "RAY", "probe_ray_wcs", "start",
                                 pointJson(DRW_Coord{3.5, -2.0, 9.0}))
            && expectEntityField(sink, "RAY", "probe_ray_wcs", "direction",
                                 pointJson(DRW_Coord{0.0, 0.0, 1.0}))
            && expectEntityField(sink, "XLINE", "probe_xline_wcs", "start",
                                 pointJson(DRW_Coord{-4.0, 6.5, -8.0}))
            && expectEntityField(sink, "XLINE", "probe_xline_wcs", "direction",
                                 pointJson(DRW_Coord{0.0, 1.0, 0.0}));
    };
    if (!expectRayAndXline(consumer))
        return false;
    const auto expectPoint = [&](const SemanticSink& sink) {
        return expectEntityField(sink, "POINT", "probe_point_wcs", "basePoint",
                                 pointJson(DRW_Coord{8.0, -6.0, 14.0}))
            && expectEntityField(sink, "POINT", "probe_point_wcs", "extrusion",
                                 pointJson(DRW_Coord{0.0, 1.0, 0.0}))
            && expectEntityDouble(sink, "POINT", "probe_point_wcs", "thickness", 0.75)
            && expectEntityDouble(sink, "POINT", "probe_point_wcs", "xAxisAngle", 0.25);
    };
    if (!expectPoint(consumer))
        return false;
    const auto expect3DLine = [&](const SemanticSink& sink) {
        return expectEntityField(sink, "3DLINE", "probe_3dline_wcs", "start",
                                 pointJson(DRW_Coord{-3.0, 5.0, 11.0}))
            && expectEntityField(sink, "3DLINE", "probe_3dline_wcs", "end",
                                 pointJson(DRW_Coord{7.0, -13.0, 17.0}))
            && expectEntityField(sink, "3DLINE", "probe_3dline_wcs", "extrusion",
                                 pointJson(DRW_Coord{1.0, 0.0, 0.0}))
            && expectEntityDouble(sink, "3DLINE", "probe_3dline_wcs",
                                  "thickness", 0.625);
    };
    if (!expect3DLine(consumer))
        return false;
    const auto expect3DFace = [&](const SemanticSink& sink) {
        return expectEntityField(sink, "3DFACE", "probe_3dface_wcs", "corner.0",
                                 pointJson(DRW_Coord{1.0, 2.0, 3.0}))
            && expectEntityField(sink, "3DFACE", "probe_3dface_wcs", "corner.1",
                                 pointJson(DRW_Coord{4.0, 5.0, 6.0}))
            && expectEntityField(sink, "3DFACE", "probe_3dface_wcs", "corner.2",
                                 pointJson(DRW_Coord{7.0, 8.0, 9.0}))
            && expectEntityField(sink, "3DFACE", "probe_3dface_wcs", "corner.3",
                                 pointJson(DRW_Coord{10.0, 11.0, 12.0}))
            && expectEntityField(sink, "3DFACE", "probe_3dface_wcs",
                                 "invisibleEdgeFlags", "5")
            && expectEntityField(sink, "3DFACE", "probe_3dface_wcs",
                                 "lineType", jsonString("DASHED"))
            && expectEntityField(sink, "3DFACE", "probe_3dface_wcs",
                                 "color", "5")
            && expectEntityField(sink, "3DFACE", "probe_3dface_wcs",
                                 "color24", "2386092")
            && expectEntityField(sink, "3DFACE", "probe_3dface_wcs",
                                 "visible", "false")
            && expectEntityField(sink, "3DFACE", "probe_3dface_wcs",
                                 "space", "1");
    };
    if (!expect3DFace(consumer) || !expect3DFace(legacyConsumer))
        return false;

    if (!expectEntityField(legacyConsumer, "CIRCLE", defaultCircleLayer, "center",
                           "{\"x\":6,\"y\":-3,\"z\":1.5}")
        || !expectEntityField(legacyConsumer, "CIRCLE", defaultCircleLayer, "extrusion",
                              "{\"x\":0,\"y\":0,\"z\":1}")
        || !expectEntityDouble(legacyConsumer, "CIRCLE", defaultCircleLayer,
                               "radius", 2.5)
        || !expectEntityDouble(legacyConsumer, "CIRCLE", defaultCircleLayer,
                               "thickness", 0.75)
        || !expectEntityField(legacyConsumer, "CIRCLE", obliqueCircleLayer, "center",
                              "{\"x\":-2,\"y\":5,\"z\":3}")
        || !expectEntityField(legacyConsumer, "CIRCLE", obliqueCircleLayer, "extrusion",
                              "{\"x\":0,\"y\":1,\"z\":0}")
        || !expectEntityDouble(legacyConsumer, "CIRCLE", obliqueCircleLayer,
                               "radius", 4.0)
        || !expectEntityDouble(legacyConsumer, "CIRCLE", obliqueCircleLayer,
                               "thickness", 0.5)
        || !expectEntityField(legacyConsumer, "ARC", negativeArcLayer, "center",
                              "{\"x\":-2,\"y\":3,\"z\":-4}")
        || !expectEntityField(legacyConsumer, "ARC", negativeArcLayer, "extrusion",
                              "{\"x\":0,\"y\":0,\"z\":-1}")
        || !expectEntityDouble(legacyConsumer, "ARC", negativeArcLayer,
                               "radius", 7.0)
        || !expectEntityDouble(legacyConsumer, "ARC", negativeArcLayer,
                               "thickness", 0.125)
        || !expectEntityDouble(legacyConsumer, "ARC", negativeArcLayer,
                               "startAngleRadians", M_PI - rawArcEnd)
        || !expectEntityDouble(legacyConsumer, "ARC", negativeArcLayer,
                               "endAngleRadians", M_PI - rawArcStart))
        return false;

    if (!expectObliqueQuad(legacyConsumer, "SOLID", "probe_solid_oblique_ocs", {{
            DRW_Coord{-1.0, 2.0, 1.0}, DRW_Coord{-5.0, 2.0, 1.0},
            DRW_Coord{-5.0, 2.0, 4.0}, DRW_Coord{-1.0, 2.0, 4.0}}}, 0.25)
        || !expectObliqueQuad(legacyConsumer, "TRACE", "probe_trace_oblique_ocs", {{
            DRW_Coord{-2.0, 3.0, 1.0}, DRW_Coord{-6.0, 3.0, 1.0},
            DRW_Coord{-6.0, 3.0, 4.0}, DRW_Coord{-2.0, 3.0, 4.0}}}, 0.5))
        return false;
    if (!expectRayAndXline(legacyConsumer))
        return false;
    if (!expectPoint(legacyConsumer))
        return false;
    if (!expect3DLine(legacyConsumer))
        return false;
    if (!expect3DFace(legacyConsumer))
        return false;

    // DXF ELLIPSE stores its center and major-axis vector in WCS. Unlike ARC
    // and CIRCLE, the oblique-normal vector must not rotate those coordinates
    // when the legacy 2D extrusion option is enabled.
    const auto expectEllipses = [&](const SemanticSink& sink) {
        const std::string defaultEllipseLayer = "probe_ellipse_default";
        const std::string tiltedEllipseLayer = "probe_ellipse_tilted_partial";
        return expectEntityField(
                   sink, "ELLIPSE", defaultEllipseLayer, "center",
                   pointJson(DRW_Coord{1.0, 2.0, 3.0}))
            && expectEntityField(
                   sink, "ELLIPSE", defaultEllipseLayer, "majorAxis",
                   pointJson(DRW_Coord{5.0, 0.0, 0.0}))
            && expectEntityField(
                   sink, "ELLIPSE", defaultEllipseLayer, "extrusion",
                   pointJson(DRW_Coord{0.0, 0.0, 1.0}))
            && expectEntityDouble(
                   sink, "ELLIPSE", defaultEllipseLayer, "ratio", 0.5)
            && expectEntityDouble(
                   sink, "ELLIPSE", defaultEllipseLayer,
                   "startParameter", 0.0)
            && expectEntityDouble(
                   sink, "ELLIPSE", defaultEllipseLayer,
                   "endParameter", 2.0 * M_PI)
            && expectEntityField(
                   sink, "ELLIPSE", tiltedEllipseLayer, "center",
                   pointJson(DRW_Coord{-2.0, 4.0, 6.0}))
            && expectEntityField(
                   sink, "ELLIPSE", tiltedEllipseLayer, "majorAxis",
                   pointJson(DRW_Coord{2.0, -1.0, 0.0}))
            && expectEntityField(
                   sink, "ELLIPSE", tiltedEllipseLayer, "extrusion",
                   pointJson(DRW_Coord{1.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0}))
            && expectEntityDouble(
                   sink, "ELLIPSE", tiltedEllipseLayer, "ratio", 0.4)
            && expectEntityDouble(
                   sink, "ELLIPSE", tiltedEllipseLayer,
                   "startParameter", 0.25)
            && expectEntityDouble(
                   sink, "ELLIPSE", tiltedEllipseLayer,
                   "endParameter", 1.75);
    };
    if (!expectEllipses(consumer) || !expectEllipses(legacyConsumer))
        return false;

    const auto expectHelix = [&](const SemanticSink& sink) {
        const std::string layer = "probe_helix";
        return expectEntityField(sink, "HELIX", layer, "degree", "2")
            && expectEntityField(sink, "HELIX", layer, "flags", "4")
            && expectEntityField(sink, "HELIX", layer, "knotCount", "6")
            && expectEntityDouble(sink, "HELIX", layer, "knot.3", 1.0)
            && expectEntityField(sink, "HELIX", layer, "weightCount", "3")
            && expectEntityDouble(sink, "HELIX", layer, "weight.1", 0.75)
            && expectEntityField(sink, "HELIX", layer,
                                 "controlPoint.2.position",
                                 "{\"x\":56,\"y\":57,\"z\":58}")
            && expectEntityField(sink, "HELIX", layer,
                                 "helixMajorVersion", "1")
            && expectEntityField(sink, "HELIX", layer,
                                 "helixMaintenanceVersion", "2")
            && expectEntityField(sink, "HELIX", layer, "axisBasePoint",
                                 "{\"x\":60,\"y\":61,\"z\":62}")
            && expectEntityField(sink, "HELIX", layer, "startPoint",
                                 "{\"x\":63,\"y\":64,\"z\":65}")
            && expectEntityField(sink, "HELIX", layer, "axisVector",
                                 "{\"x\":0,\"y\":0,\"z\":1}")
            && expectEntityDouble(sink, "HELIX", layer, "radius", 2.5)
            && expectEntityDouble(sink, "HELIX", layer, "turns", 3.25)
            && expectEntityDouble(sink, "HELIX", layer, "turnHeight", 4.5)
            && expectEntityField(sink, "HELIX", layer, "handedness", "true")
            && expectEntityField(sink, "HELIX", layer, "constraintType", "2");
    };
    if (!expectHelix(consumer) || !expectHelix(legacyConsumer))
        return false;

    const auto expectNurbsSurface = [&](const SemanticSink& sink) {
        const std::string layer = "probe_nurbs_surface";
        return expectEntityField(sink, "NURBSURFACE", layer,
                                 "modelerFormatVersion", "2")
            && expectEntityField(sink, "NURBSURFACE", layer, "short170", "7")
            && expectEntityField(sink, "NURBSURFACE", layer,
                                 "cvHullDisplay", "true")
            && expectEntityField(sink, "NURBSURFACE", layer, "uvec1",
                                 "{\"x\":70,\"y\":71,\"z\":72}")
            && expectEntityField(sink, "NURBSURFACE", layer, "vvec1",
                                 "{\"x\":73,\"y\":74,\"z\":75}")
            && expectEntityField(sink, "NURBSURFACE", layer, "uvec2",
                                 "{\"x\":76,\"y\":77,\"z\":78}")
            && expectEntityField(sink, "NURBSURFACE", layer, "vvec2",
                                 "{\"x\":79,\"y\":80,\"z\":81}");
    };
    if (!expectNurbsSurface(consumer) || !expectNurbsSurface(legacyConsumer))
        return false;

    const auto expectPlaneSurface = [&](const SemanticSink& sink) {
        const std::string layer = "probe_plane_surface";
        return expectEntityField(sink, "PLANESURFACE", layer,
                                 "modelerFormatVersion", "3")
            && expectEntityField(sink, "PLANESURFACE", layer,
                                 "uIsolines", "5")
            && expectEntityField(sink, "PLANESURFACE", layer,
                                 "vIsolines", "7")
            && expectEntityField(sink, "PLANESURFACE", layer,
                                 "hasRawAcisData", "false")
            && expectEntityField(sink, "PLANESURFACE", layer,
                                 "rawAcisByteCount", "0");
    };
    if (!expectPlaneSurface(consumer) || !expectPlaneSurface(legacyConsumer))
        return false;

    const auto expectExtrudedSurface = [&](const SemanticSink& sink) {
        const std::string layer = "probe_extruded_surface";
        return expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "modelerFormatVersion", "2")
            && expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "uIsolines", "13")
            && expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "vIsolines", "17")
            && expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "classId", "23")
            && expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "sweepVector",
                                 "{\"x\":0.5,\"y\":1.5,\"z\":2.5}")
            && expectEntityDouble(sink, "EXTRUDEDSURFACE", layer,
                                  "extrudedTransform.3", 10.0)
            && expectEntityDouble(sink, "EXTRUDEDSURFACE", layer,
                                  "sweepEntityTransform.7", 20.0)
            && expectEntityDouble(sink, "EXTRUDEDSURFACE", layer,
                                  "pathEntityTransform.11", 30.0)
            && expectEntityDouble(sink, "EXTRUDEDSURFACE", layer,
                                  "draftAngle", 0.125)
            && expectEntityDouble(sink, "EXTRUDEDSURFACE", layer,
                                  "scaleFactor", 1.5)
            && expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "sweepAlignmentFlags", "2")
            && expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "pathFlags", "8")
            && expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "solid", "true")
            && expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "bank", "true")
            && expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "pathEntityTransformComputed", "false")
            && expectEntityField(sink, "EXTRUDEDSURFACE", layer,
                                 "referenceVector",
                                 "{\"x\":3.5,\"y\":4.5,\"z\":5.5}");
    };
    if (!expectExtrudedSurface(consumer)
        || !expectExtrudedSurface(legacyConsumer))
        return false;

    const auto expectRevolvedSurface = [&](const SemanticSink& sink) {
        const std::string layer = "probe_revolved_surface";
        return expectEntityField(sink, "REVOLVEDSURFACE", layer,
                                 "modelerFormatVersion", "2")
            && expectEntityField(sink, "REVOLVEDSURFACE", layer,
                                 "uIsolines", "19")
            && expectEntityField(sink, "REVOLVEDSURFACE", layer,
                                 "vIsolines", "23")
            && expectEntityField(sink, "REVOLVEDSURFACE", layer, "id", "29")
            && expectEntityField(sink, "REVOLVEDSURFACE", layer, "axisPoint",
                                 "{\"x\":31,\"y\":32,\"z\":33}")
            && expectEntityField(sink, "REVOLVEDSURFACE", layer, "axisVector",
                                 "{\"x\":0,\"y\":1,\"z\":0}")
            && expectEntityDouble(sink, "REVOLVEDSURFACE", layer,
                                  "revolveAngle", 1.25)
            && expectEntityDouble(sink, "REVOLVEDSURFACE", layer,
                                  "startAngle", 0.375)
            && expectEntityDouble(sink, "REVOLVEDSURFACE", layer,
                                  "transform.5", 12.0)
            && expectEntityDouble(sink, "REVOLVEDSURFACE", layer,
                                  "draftAngle", 0.125)
            && expectEntityDouble(sink, "REVOLVEDSURFACE", layer,
                                  "draftStartDistance", 1.5)
            && expectEntityDouble(sink, "REVOLVEDSURFACE", layer,
                                  "draftEndDistance", 2.75)
            && expectEntityDouble(sink, "REVOLVEDSURFACE", layer,
                                  "twistAngle", 0.625)
            && expectEntityField(sink, "REVOLVEDSURFACE", layer,
                                 "solid", "true")
            && expectEntityField(sink, "REVOLVEDSURFACE", layer,
                                 "closeToAxis", "true");
    };
    if (!expectRevolvedSurface(consumer)
        || !expectRevolvedSurface(legacyConsumer))
        return false;

    const auto expectSweptSurface = [&](const SemanticSink& sink) {
        const std::string layer = "probe_swept_surface";
        return expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "modelerFormatVersion", "2")
            && expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "uIsolines", "29")
            && expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "vIsolines", "31")
            && expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "sweepEntityId", "37")
            && expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "sweepDataByteCount", "2")
            && expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "pathEntityId", "43")
            && expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "pathDataByteCount", "3")
            && expectEntityDouble(sink, "SWEPTSURFACE", layer,
                                  "sweepEntityTransformed.2", 14.0)
            && expectEntityDouble(sink, "SWEPTSURFACE", layer,
                                  "pathEntityTransformed.4", 15.0)
            && expectEntityDouble(sink, "SWEPTSURFACE", layer,
                                  "sweepEntityTransform.6", 16.0)
            && expectEntityDouble(sink, "SWEPTSURFACE", layer,
                                  "pathEntityTransform.8", 17.0)
            && expectEntityDouble(sink, "SWEPTSURFACE", layer,
                                  "draftAngle", 0.25)
            && expectEntityDouble(sink, "SWEPTSURFACE", layer,
                                  "scaleFactor", 1.25)
            && expectEntityDouble(sink, "SWEPTSURFACE", layer,
                                  "alignAngle", 0.875)
            && expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "sweepAlignmentFlags", "1")
            && expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "solid", "true")
            && expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "pathEntityTransformComputed", "true")
            && expectEntityField(sink, "SWEPTSURFACE", layer,
                                 "referenceVector",
                                 "{\"x\":2.5,\"y\":3.5,\"z\":4.5}");
    };
    const std::vector<std::uint8_t> expectedSweepData{0x41u, 0x42u};
    const std::vector<std::uint8_t> expectedPathData{0x51u, 0x52u, 0x53u};
    const auto expectSweptCarriers = [&](const SemanticSink& sink) {
        bool foundSweepData = false;
        bool foundPathData = false;
        for (const CarrierRow& carrier : sink.carriers) {
            if (carrier.source.find("surfaceSweepData") != std::string::npos
                && carrier.size == expectedSweepData.size()
                && carrier.digest == sha256(expectedSweepData.data(),
                                            expectedSweepData.size())
                && carrier.disposition == "preserved")
                foundSweepData = true;
            if (carrier.source.find("surfacePathData") != std::string::npos
                && carrier.size == expectedPathData.size()
                && carrier.digest == sha256(expectedPathData.data(),
                                            expectedPathData.size())
                && carrier.disposition == "preserved")
                foundPathData = true;
        }
        return foundSweepData && foundPathData;
    };
    if (!expectSweptSurface(consumer) || !expectSweptSurface(legacyConsumer)
        || !expectSweptCarriers(consumer)
        || !expectSweptCarriers(legacyConsumer))
        return false;

    const auto expectNestedInsertFields = [&](const SemanticSink& sink,
                                              bool legacy) {
        const std::string innerLayer = "probe_nested_inner_insert";
        const std::string outerLayer = "probe_nested_outer_insert";
        return expectEntityField(sink, "INSERT", innerLayer, "blockName",
                                 "\"PROBE_NESTED_CHILD\"")
            && expectEntityField(sink, "INSERT", innerLayer, "insertionPoint",
                                 "{\"x\":4,\"y\":5,\"z\":6}")
            && expectEntityField(sink, "INSERT", innerLayer, "extrusion",
                                 "{\"x\":0,\"y\":0,\"z\":1}")
            && expectEntityDouble(sink, "INSERT", innerLayer, "xScale", 1.5)
            && expectEntityDouble(sink, "INSERT", innerLayer, "yScale", 0.5)
            && expectEntityDouble(sink, "INSERT", innerLayer, "zScale", 2.0)
            && expectEntityDouble(sink, "INSERT", innerLayer,
                                  "rotationAngle", 0.3)
            && expectEntityField(sink, "INSERT", innerLayer, "columnCount", "2")
            && expectEntityField(sink, "INSERT", innerLayer, "rowCount", "2")
            && expectEntityDouble(sink, "INSERT", innerLayer,
                                  "columnSpacing", 7.0)
            && expectEntityDouble(sink, "INSERT", innerLayer, "rowSpacing", 11.0)
            && expectEntityField(sink, "INSERT", outerLayer, "blockName",
                                 "\"PROBE_NESTED_PARENT\"")
            && expectEntityField(sink, "INSERT", outerLayer, "insertionPoint",
                                 legacy ? "{\"x\":-20,\"y\":3,\"z\":-5}"
                                        : "{\"x\":20,\"y\":-5,\"z\":3}")
            && expectEntityField(sink, "INSERT", outerLayer, "extrusion",
                                 "{\"x\":0,\"y\":1,\"z\":0}")
            && expectEntityDouble(sink, "INSERT", outerLayer, "xScale", 2.0)
            && expectEntityDouble(sink, "INSERT", outerLayer, "yScale", -1.0)
            && expectEntityDouble(sink, "INSERT", outerLayer, "zScale", 0.75)
            && expectEntityDouble(sink, "INSERT", outerLayer,
                                  "rotationAngle", -0.4)
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attributeCount", "2")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "hasSequenceEnd", "true")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.0.tag", "\"NESTED_ID\"")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.0.text", "\"ATTRIBUTE 3D\"")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.0.insertionPoint",
                                 "{\"x\":12,\"y\":-4,\"z\":9}")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.0.extrusion",
                                 "{\"x\":0,\"y\":0,\"z\":1}")
            && expectEntityDouble(sink, "INSERT", outerLayer,
                                  "attribute.0.height", 2.5)
            && expectEntityDouble(sink, "INSERT", outerLayer,
                                  "attribute.0.rotationAngle", 0.6)
            && expectEntityDouble(sink, "INSERT", outerLayer,
                                  "attribute.0.widthScale", 0.8)
            && expectEntityDouble(sink, "INSERT", outerLayer,
                                  "attribute.0.oblique", 0.1)
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.0.flags", "0")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.0.fieldLength", "12")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.0.lockPosition", "true")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.0.style", "\"Standard\"")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.1.tag", "\"NESTED_NOTE\"")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.1.text", "\"ARRAY PARENT\"")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.1.insertionPoint",
                                 "{\"x\":-3,\"y\":15,\"z\":22}")
            && expectEntityDouble(sink, "INSERT", outerLayer,
                                  "attribute.1.height", 1.25)
            && expectEntityDouble(sink, "INSERT", outerLayer,
                                  "attribute.1.rotationAngle", -0.2)
            && expectEntityDouble(sink, "INSERT", outerLayer,
                                  "attribute.1.widthScale", 1.1)
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.1.flags", "1")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.1.fieldLength", "5")
            && expectEntityField(sink, "INSERT", outerLayer,
                                 "attribute.1.style", "\"Standard\"");
    };
    if (!expectNestedInsertFields(consumer, false)
        || !expectNestedInsertFields(legacyConsumer, true)
        || !expectEntityField(consumer, "LINE", "probe_nested_geometry", "start",
                              "{\"x\":1,\"y\":2,\"z\":3}")
        || !expectEntityField(consumer, "LINE", "probe_nested_geometry", "end",
                              "{\"x\":4,\"y\":-1,\"z\":2}"))
        return false;

    // Independent homogeneous-matrix oracle for the nested INSERT path:
    // child-local point -> MINSERT in the parent block -> oblique INSERT in
    // model space. This deliberately does not call libdxfrw's OCS helpers.
    using Matrix4 = std::array<double, 16>;
    const auto identityMatrix = [] {
        Matrix4 matrix{};
        for (std::size_t i = 0; i < 4; ++i)
            matrix[i * 4 + i] = 1.0;
        return matrix;
    };
    const auto multiplyMatrix = [](const Matrix4& lhs, const Matrix4& rhs) {
        Matrix4 result{};
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                for (std::size_t k = 0; k < 4; ++k)
                    result[row * 4 + column] +=
                        lhs[row * 4 + k] * rhs[k * 4 + column];
            }
        }
        return result;
    };
    const auto translationMatrix = [&](const DRW_Coord& point) {
        Matrix4 matrix = identityMatrix();
        matrix[3] = point.x;
        matrix[7] = point.y;
        matrix[11] = point.z;
        return matrix;
    };
    const auto scaleMatrix = [&](double x, double y, double z) {
        Matrix4 matrix = identityMatrix();
        matrix[0] = x;
        matrix[5] = y;
        matrix[10] = z;
        return matrix;
    };
    const auto rotationZMatrix = [&](double angle) {
        Matrix4 matrix = identityMatrix();
        matrix[0] = std::cos(angle);
        matrix[1] = -std::sin(angle);
        matrix[4] = std::sin(angle);
        matrix[5] = std::cos(angle);
        return matrix;
    };
    const auto ocsMatrix = [&](DRW_Coord normal) {
        const double normalLength = std::sqrt(normal.x * normal.x
            + normal.y * normal.y + normal.z * normal.z);
        normal.x /= normalLength;
        normal.y /= normalLength;
        normal.z /= normalLength;
        DRW_Coord axisX;
        if (std::fabs(normal.x) < 0.015625
            && std::fabs(normal.y) < 0.015625) {
            axisX = DRW_Coord{normal.z, 0.0, -normal.x};
        } else {
            axisX = DRW_Coord{-normal.y, normal.x, 0.0};
        }
        const double axisLength = std::sqrt(axisX.x * axisX.x
            + axisX.y * axisX.y + axisX.z * axisX.z);
        axisX.x /= axisLength;
        axisX.y /= axisLength;
        axisX.z /= axisLength;
        const DRW_Coord axisY{
            normal.y * axisX.z - axisX.y * normal.z,
            normal.z * axisX.x - axisX.z * normal.x,
            normal.x * axisX.y - axisX.x * normal.y};
        Matrix4 matrix = identityMatrix();
        matrix[0] = axisX.x; matrix[1] = axisY.x; matrix[2] = normal.x;
        matrix[4] = axisX.y; matrix[5] = axisY.y; matrix[6] = normal.y;
        matrix[8] = axisX.z; matrix[9] = axisY.z; matrix[10] = normal.z;
        return matrix;
    };
    const auto makeInsertMatrix = [&](const DRW_Coord& insertion,
                                      const DRW_Coord& normal,
                                      double xScale, double yScale,
                                      double zScale, double rotation,
                                      const DRW_Coord& blockBase,
                                      bool insertionIsWcs) {
        const Matrix4 ocs = ocsMatrix(normal);
        Matrix4 result = insertionIsWcs
            ? translationMatrix(insertion)
            : multiplyMatrix(ocs, translationMatrix(insertion));
        if (insertionIsWcs)
            result = multiplyMatrix(result, ocs);
        result = multiplyMatrix(result, rotationZMatrix(rotation));
        result = multiplyMatrix(result,
                                scaleMatrix(xScale, yScale, zScale));
        result = multiplyMatrix(result, translationMatrix(
            DRW_Coord{-blockBase.x, -blockBase.y, -blockBase.z}));
        return result;
    };
    const auto transformPoint = [](const Matrix4& matrix,
                                   const DRW_Coord& point) {
        const double input[] = {point.x, point.y, point.z, 1.0};
        DRW_Coord output;
        double values[] = {0.0, 0.0, 0.0};
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 4; ++column)
                values[row] += matrix[row * 4 + column] * input[column];
        }
        output.x = values[0];
        output.y = values[1];
        output.z = values[2];
        return output;
    };
    const Matrix4 innerNative = makeInsertMatrix(
        DRW_Coord{4.0, 5.0, 6.0}, DRW_Coord{0.0, 0.0, 1.0},
        1.5, 0.5, 2.0, 0.3, DRW_Coord{1.0, -2.0, 0.5}, false);
    // MINSERT spacing is rotated into OCS but is not multiplied by the
    // referenced block's scale. This mirrors ezdxf's virtual-cell expansion.
    const DRW_Coord rotatedArrayOffset = transformPoint(
        rotationZMatrix(0.3), DRW_Coord{7.0, 11.0, 0.0});
    const Matrix4 innerArrayCellNative = makeInsertMatrix(
        DRW_Coord{4.0 + rotatedArrayOffset.x,
                  5.0 + rotatedArrayOffset.y, 6.0},
        DRW_Coord{0.0, 0.0, 1.0}, 1.5, 0.5, 2.0, 0.3,
        DRW_Coord{1.0, -2.0, 0.5}, false);
    const Matrix4 outerNative = makeInsertMatrix(
        DRW_Coord{20.0, -5.0, 3.0}, DRW_Coord{0.0, 1.0, 0.0},
        2.0, -1.0, 0.75, -0.4, DRW_Coord{-3.0, 2.0, 1.0}, false);
    const Matrix4 outerLegacy = makeInsertMatrix(
        DRW_Coord{-20.0, 3.0, -5.0}, DRW_Coord{0.0, 1.0, 0.0},
        2.0, -1.0, 0.75, -0.4, DRW_Coord{-3.0, 2.0, 1.0}, true);
    const DRW_Coord expectedNativeStart = transformPoint(
        multiplyMatrix(outerNative, innerNative), DRW_Coord{1.0, 2.0, 3.0});
    const DRW_Coord expectedNativeEnd = transformPoint(
        multiplyMatrix(outerNative, innerNative), DRW_Coord{4.0, -1.0, 2.0});
    const DRW_Coord expectedArrayCellNativeStart = transformPoint(
        multiplyMatrix(outerNative, innerArrayCellNative),
        DRW_Coord{1.0, 2.0, 3.0});
    const DRW_Coord expectedArrayCellNativeEnd = transformPoint(
        multiplyMatrix(outerNative, innerArrayCellNative),
        DRW_Coord{4.0, -1.0, 2.0});
    const DRW_Coord expectedLegacyStart = transformPoint(
        multiplyMatrix(outerLegacy, innerNative), DRW_Coord{1.0, 2.0, 3.0});
    const DRW_Coord expectedLegacyEnd = transformPoint(
        multiplyMatrix(outerLegacy, innerNative), DRW_Coord{4.0, -1.0, 2.0});
    const DRW_Coord expectedArrayCellLegacyStart = transformPoint(
        multiplyMatrix(outerLegacy, innerArrayCellNative),
        DRW_Coord{1.0, 2.0, 3.0});
    const DRW_Coord expectedArrayCellLegacyEnd = transformPoint(
        multiplyMatrix(outerLegacy, innerArrayCellNative),
        DRW_Coord{4.0, -1.0, 2.0});
    const auto closePoint = [](const DRW_Coord& actual,
                               const DRW_Coord& expected) {
        return std::fabs(actual.x - expected.x) < 1.0e-11
            && std::fabs(actual.y - expected.y) < 1.0e-11
            && std::fabs(actual.z - expected.z) < 1.0e-11;
    };
    if (!closePoint(expectedNativeStart, DRW_Coord{-29.8937792440482,
                                                   10.5, -14.5145621709052})
        || !closePoint(expectedNativeEnd, DRW_Coord{-38.6698381138937,
                                                    9.0, -18.1130149497834})
        || !closePoint(expectedArrayCellNativeStart,
                       DRW_Coord{-31.3266287411441, 10.5,
                                 -28.7756380263298})
        || !closePoint(expectedArrayCellNativeEnd,
                       DRW_Coord{-40.1026876109896, 9.0,
                                 -32.3740908052080})
        || !closePoint(expectedLegacyStart, expectedNativeStart)
        || !closePoint(expectedLegacyEnd, expectedNativeEnd)
        || !closePoint(expectedArrayCellLegacyStart,
                       expectedArrayCellNativeStart)
        || !closePoint(expectedArrayCellLegacyEnd,
                       expectedArrayCellNativeEnd)) {
        std::cerr << "nested INSERT matrix oracle mismatch: nativeStart={"
                  << number(expectedNativeStart.x) << ','
                  << number(expectedNativeStart.y) << ','
                  << number(expectedNativeStart.z) << "}, nativeEnd={"
                  << number(expectedNativeEnd.x) << ','
                  << number(expectedNativeEnd.y) << ','
                  << number(expectedNativeEnd.z) << "}, legacyStart={"
                  << number(expectedLegacyStart.x) << ','
                  << number(expectedLegacyStart.y) << ','
                  << number(expectedLegacyStart.z) << "}, arrayCellStart={"
                  << number(expectedArrayCellNativeStart.x) << ','
                  << number(expectedArrayCellNativeStart.y) << ','
                  << number(expectedArrayCellNativeStart.z) << "}, arrayCellEnd={"
                  << number(expectedArrayCellNativeEnd.x) << ','
                  << number(expectedArrayCellNativeEnd.y) << ','
                  << number(expectedArrayCellNativeEnd.z) << "}\n";
        return false;
    }

    const auto expectPolyface = [&](const SemanticSink& sink) {
        const std::string entity = "POLYLINE";
        const std::string layer = "probe_pface";
        return expectEntityField(sink, entity, layer, "flags", "64")
            && expectEntityField(sink, entity, layer, "declaredVertexCount", "4")
            && expectEntityField(sink, entity, layer, "declaredFaceCount", "1")
            && expectEntityField(sink, entity, layer, "vertexCount", "5")
            && expectEntityField(sink, entity, layer, "vertex.0.position",
                                 "{\"x\":1,\"y\":2,\"z\":3}")
            && expectEntityField(sink, entity, layer, "vertex.1.position",
                                 "{\"x\":4,\"y\":5,\"z\":6}")
            && expectEntityField(sink, entity, layer, "vertex.2.position",
                                 "{\"x\":7,\"y\":8,\"z\":9}")
            && expectEntityField(sink, entity, layer, "vertex.3.position",
                                 "{\"x\":10,\"y\":11,\"z\":12}")
            && expectEntityField(sink, entity, layer, "vertex.4.flags", "128")
            && expectEntityField(sink, entity, layer, "vertex.4.faceIndex1", "1")
            && expectEntityField(sink, entity, layer, "vertex.4.faceIndex2", "-2")
            && expectEntityField(sink, entity, layer, "vertex.4.faceIndex3", "3")
            && expectEntityField(sink, entity, layer, "vertex.4.faceIndex4", "-4");
    };
    if (!expectPolyface(consumer) || !expectPolyface(legacyConsumer))
        return false;

    const auto expectLegacyField = [&](const std::string& name,
                                       const std::string& expected) {
        const Field* field = findEntityField(legacyConsumer, "LWPOLYLINE", name);
        if (field != nullptr && field->value == expected)
            return true;
        std::cerr << "2D extrusion probe mismatch for " << name << ": expected "
                  << expected << ", got "
                  << (field == nullptr ? "<missing>" : field->value) << '\n';
        return false;
    };
    if (!expectLegacyField("vertex.0.x", "-2")
        || !expectLegacyField("vertex.0.y", "5")
        || !expectLegacyField("vertex.1.x", "-4")
        || !expectLegacyField("vertex.1.y", "5")
        || !expectLegacyField("elevation", "5")
        || !expectLegacyField("extrusion", "{\"x\":0,\"y\":1,\"z\":0}")
        || !fieldEquals(legacyConsumer, "3DFACE", "corner.3",
                        "{\"x\":10,\"y\":11,\"z\":12}"))
        return false;

    const auto expectMesh = [&](const SemanticSink& sink) {
        return expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "version", "7")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "blendCrease", "true")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "subdivisionLevel", "5")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "vertexCount", "3")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "vertex.0", "{\"x\":20,\"y\":21,\"z\":22}")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "vertex.1", "{\"x\":23,\"y\":24,\"z\":25}")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "vertex.2", "{\"x\":26,\"y\":27,\"z\":28}")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "faceCount", "1")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "face.0.vertexCount", "3")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "face.0.vertex.2", "2")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "edgeCount", "2")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "edge.1.start", "1")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "edge.1.end", "2")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "creaseCount", "2")
            && expectEntityDouble(sink, "MESH", "probe_mesh_geometry",
                                  "crease.0", 0.25)
            && expectEntityDouble(sink, "MESH", "probe_mesh_geometry",
                                  "crease.1", 0.75)
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "propertyOverrideCount", "2")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "propertyOverride.0.subEntityMarker", "17")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "propertyOverride.0.propertyTypeCount", "2")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "propertyOverride.0.propertyType.0", "0")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "propertyOverride.0.propertyType.1", "2")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "propertyOverride.1.subEntityMarker", "29")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "propertyOverride.1.propertyTypeCount", "2")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "propertyOverride.1.propertyType.0", "1")
            && expectEntityField(sink, "MESH", "probe_mesh_geometry",
                                 "propertyOverride.1.propertyType.1", "3");
    };
    if (!expectMesh(consumer) || !expectMesh(legacyConsumer))
        return false;

    if (!fieldEquals(consumer, "3DFACE", "corner.3",
                     "{\"x\":10,\"y\":11,\"z\":12}")
        || !fieldEquals(consumer, "3DFACE", "invisibleEdgeFlags", "5")
        || !fieldEquals(consumer, "POLYLINE", "vertex.1.position",
                        "{\"x\":16,\"y\":17,\"z\":18}")
        || !expectEntityField(consumer, "INSERT", "probe_insert_grid",
                              "insertionPoint", "{\"x\":30,\"y\":31,\"z\":32}")
        || !expectEntityField(consumer, "INSERT", "probe_insert_grid",
                              "extrusion", "{\"x\":0,\"y\":1,\"z\":0}")
        || !expectEntityField(consumer, "INSERT", "probe_insert_grid",
                              "xScale", "-2")
        || !expectEntityField(consumer, "INSERT", "probe_insert_grid",
                              "zScale", "4")
        || !expectEntityField(consumer, "INSERT", "probe_insert_grid",
                              "columnCount", "2")
        || !expectEntityField(consumer, "INSERT", "probe_insert_grid",
                              "rowCount", "3")
        || !fieldEquals(consumer, "SPLINE", "controlPoint.2.position",
                        "{\"x\":46,\"y\":47,\"z\":48}")
        || !fieldEquals(consumer, "SPLINE", "knotCount", "6")
        || !fieldEquals(consumer, "LWPOLYLINE", "elevation", "5")
        || !fieldEquals(consumer, "LWPOLYLINE", "extrusion",
                        "{\"x\":0,\"y\":1,\"z\":0}")
        || !fieldEquals(consumer, "LWPOLYLINE", "vertex.0.x", "2")
        || !fieldEquals(consumer, "LWPOLYLINE", "vertex.0.y", "3")
        || !fieldEquals(consumer, "LWPOLYLINE", "vertex.0.bulge", "0.25")
        || !fieldEquals(consumer, "LOFTEDSURFACE", "uIsolines", "10")
        || !fieldEquals(consumer, "LOFTEDSURFACE",
                        "dxfReferenceData.1.byteCount", "3"))
        return false;

    for (const SemanticSink* sink : {&consumer, &legacyConsumer}) {
        if (!expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                               "solid", "1")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "associative", "0")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "elevation", "4")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "extrusion", "{\"x\":1,\"y\":0,\"z\":0}")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loopCount", "1")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loop.0.type", "2")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loop.0.objectCount", "1")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loop.0.closed", "true")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loop.0.vertexCount", "3")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loop.0.vertex.0.ocs",
                                  "{\"x\":2,\"y\":3,\"z\":0}")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loop.0.vertex.0.wcs",
                                  "{\"x\":4,\"y\":2,\"z\":3}")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loop.0.vertex.1.ocs",
                                  "{\"x\":5,\"y\":3,\"z\":0}")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loop.0.vertex.1.wcs",
                                  "{\"x\":4,\"y\":5,\"z\":3}")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loop.0.vertex.2.ocs",
                                  "{\"x\":5,\"y\":7,\"z\":0}")
            || !expectEntityField(*sink, "HATCH", "probe_hatch_ocs_boundary",
                                  "loop.0.vertex.2.wcs",
                                  "{\"x\":4,\"y\":5,\"z\":7}"))
            return false;
    }

    for (const SemanticSink* sink : {&consumer, &legacyConsumer}) {
        if (!expectEntityField(*sink, "MPOLYGON",
                               "probe_mpolygon_ocs_boundary", "solid", "true")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "associative", "false")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "fillColorAci", "5")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "fillColorRgb", "3430008")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "elevation", "6")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "extrusion", "{\"x\":0,\"y\":1,\"z\":0}")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loopCount", "1")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loop.0.type", "2")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loop.0.objectCount", "1")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loop.0.closed", "true")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loop.0.vertexCount", "3")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loop.0.vertex.0.ocs",
                                  "{\"x\":2,\"y\":3,\"z\":0}")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loop.0.vertex.0.wcs",
                                  "{\"x\":-2,\"y\":6,\"z\":3}")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loop.0.vertex.1.ocs",
                                  "{\"x\":5,\"y\":3,\"z\":0}")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loop.0.vertex.1.wcs",
                                  "{\"x\":-5,\"y\":6,\"z\":3}")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loop.0.vertex.2.ocs",
                                  "{\"x\":5,\"y\":7,\"z\":0}")
            || !expectEntityField(*sink, "MPOLYGON",
                                  "probe_mpolygon_ocs_boundary",
                                  "loop.0.vertex.2.wcs",
                                  "{\"x\":-5,\"y\":6,\"z\":7}"))
            return false;
    }

    // A pre-MPOLYGON consumer implements only addHatch. Exercise the actual
    // DRW_Interface default bridge and confirm the derived polygon reaches
    // that callback with its boundary model intact.
    SemanticSink legacyMpolygonConsumer("legacy-2d-consumer", "read");
    legacyMpolygonConsumer.enable3DConsumerProbe();
    DRW_MPolygon legacyMpolygon;
    legacyMpolygon.layer = "probe_mpolygon_legacy_hatch_fallback";
    legacyMpolygon.basePoint = DRW_Coord{0.0, 0.0, 6.0};
    legacyMpolygon.extPoint = DRW_Coord{0.0, 1.0, 0.0};
    auto legacyLoop = std::make_shared<DRW_HatchLoop>(2);
    auto legacyBoundary = std::make_shared<DRW_LWPolyline>();
    legacyBoundary->flags = 1;
    legacyBoundary->addVertex(DRW_Vertex2D{2.0, 3.0, 0.0});
    legacyBoundary->addVertex(DRW_Vertex2D{5.0, 3.0, 0.0});
    legacyBoundary->addVertex(DRW_Vertex2D{5.0, 7.0, 0.0});
    legacyLoop->objlist.push_back(legacyBoundary);
    legacyLoop->update();
    legacyMpolygon.appendLoop(legacyLoop);
    legacyMpolygonConsumer.exerciseDefaultMPolygonFallback(legacyMpolygon);
    if (findEntityRecordByLayer(legacyMpolygonConsumer, "HATCH",
                                "probe_mpolygon_legacy_hatch_fallback") == nullptr
        || std::any_of(legacyMpolygonConsumer.records.begin(),
                       legacyMpolygonConsumer.records.end(),
                       [](const RecordRow& row) {
                           return row.entity == "MPOLYGON";
                       })
        || !fieldEqualsForLayer(legacyMpolygonConsumer, "HATCH",
                                "probe_mpolygon_legacy_hatch_fallback",
                                "loopCount", "1")
        || !fieldEqualsForLayer(legacyMpolygonConsumer, "HATCH",
                                "probe_mpolygon_legacy_hatch_fallback",
                                "loop.0.closed", "true")
        || !fieldEqualsForLayer(legacyMpolygonConsumer, "HATCH",
                                "probe_mpolygon_legacy_hatch_fallback",
                                "loop.0.vertex.2.ocs",
                                "{\"x\":5,\"y\":7,\"z\":0}"))
        return false;

    bool foundBinaryReference = false;
    const std::vector<std::uint8_t> expectedBytes{0x12u, 0x34u, 0x56u};
    for (const CarrierRow& carrier : consumer.carriers) {
        if (carrier.source.find("loftDxfReference.1.group310")
                != std::string::npos
            && carrier.size == expectedBytes.size()
            && carrier.digest == sha256(expectedBytes.data(), expectedBytes.size()))
            foundBinaryReference = true;
    }
    return foundBinaryReference;
}

bool runNestedMTextAttributeProbe(bool binary) {
    std::error_code error;
    const auto root = std::filesystem::temp_directory_path(error);
    if (error)
        return false;
    const auto stamp = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    const auto directory = root / ("libdxfrw-nested-mtext-attribute-"
        + std::to_string(stamp) + (binary ? "-bin" : "-ascii"));
    if (!std::filesystem::create_directory(directory, error) || error)
        return false;
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }
    } cleanup{directory};

    const auto output = directory / "nested-mtext-attribute.dxf";
    const std::string outputName = output.string();
    SemanticSink producer("dxfRW", "write");
    producer.enableNestedMTextAttributeProbe();
    dxfRW writer(outputName.c_str());
    producer.configureWrite(&writer);
    if (!writer.write(&producer, DRW::AC1032, binary)
        || !producer.writeRecipeSucceeded()) {
        std::cerr << "nested MTEXT ATTRIB write failed: binary=" << binary
                  << ", error=" << static_cast<int>(writer.getError()) << '\n';
        return false;
    }

    SemanticSink consumer("dxfRW", "read");
    dxfRW reader(outputName.c_str());
    if (!reader.read(&consumer, false) || !consumer.diagnostics.empty()) {
        std::cerr << "nested MTEXT ATTRIB read failed: binary=" << binary
                  << ", error=" << static_cast<int>(reader.getError())
                  << ", diagnostics=" << consumer.diagnostics.size() << '\n';
        return false;
    }
    const std::string layer = "probe_nested_mtext_attribute";
    if (!fieldEqualsForLayer(consumer, "INSERT", layer, "attributeCount", "1")
        || !fieldEqualsForLayer(consumer, "INSERT", layer, "hasSequenceEnd", "true")
        || !fieldEqualsForLayer(consumer, "INSERT", layer, "attribute.0.tag",
                                "\"MTEXT_TAG\"")
        || !fieldEqualsForLayer(consumer, "INSERT", layer,
                                "attribute.0.text",
                                "\"single-line fallback\"")
        || !fieldEqualsForLayer(consumer, "INSERT", layer,
                                "attribute.0.insertionPoint",
                                "{\"x\":13,\"y\":17,\"z\":19}")
        || !fieldEqualsForLayer(consumer, "INSERT", layer,
                                "attribute.0.attributeVersion", "1")
        || !fieldEqualsForLayer(consumer, "INSERT", layer,
                                "attribute.0.attributeType", "2")
        || !fieldEqualsForLayer(consumer, "INSERT", layer,
                                "attribute.0.hasMText", "true")
        || !fieldEqualsForLayer(consumer, "INSERT", layer,
                                "attribute.0.mtext.text",
                                "\"first line\\\\Psecond line\"")
        || !doubleFieldEqualsForLayer(consumer, "INSERT", layer,
                                      "attribute.0.mtext.height", 1.75)
        || !fieldEqualsForLayer(consumer, "INSERT", layer,
                                "attribute.0.mtext.insertionPoint",
                                "{\"x\":37,\"y\":41,\"z\":43}")
        || !doubleFieldEqualsForLayer(consumer, "INSERT", layer,
                                      "attribute.0.mtext.rotationAngle", 0.625))
        return false;

    SemanticSink legacyConsumer("dxfRW", "read");
    dxfRW legacyReader(outputName.c_str());
    if (!legacyReader.read(&legacyConsumer, true)
        || !legacyConsumer.diagnostics.empty()
        || !fieldEqualsForLayer(legacyConsumer, "INSERT", layer,
                                "attribute.0.mtext.text",
                                "\"first line\\\\Psecond line\"")
        || !fieldEqualsForLayer(legacyConsumer, "INSERT", layer,
                                "attribute.0.mtext.insertionPoint",
                                "{\"x\":37,\"y\":41,\"z\":43}"))
        return false;
    return true;
}

bool runPlaneSurfaceInlinePayloadRefusal(bool binary) {
    std::error_code error;
    const auto root = std::filesystem::temp_directory_path(error);
    if (error)
        return false;
    const auto stamp = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    const auto directory = root / ("libdxfrw-plane-inline-refusal-"
        + std::to_string(stamp) + (binary ? "-bin" : "-ascii"));
    if (!std::filesystem::create_directory(directory, error) || error)
        return false;
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }
    } cleanup{directory};

    const auto output = directory / "plane-inline-refusal.dxf";
    const std::string sentinel = "preserve AC1027 plane payload destination";
    {
        std::ofstream seed(output, std::ios::binary | std::ios::trunc);
        seed << sentinel;
        if (!seed)
            return false;
    }
    const auto destinationMatches = [&](const std::string& expected) {
        std::ifstream stream(output, std::ios::binary);
        const std::string contents(std::istreambuf_iterator<char>{stream},
                                   std::istreambuf_iterator<char>{});
        return contents == expected;
    };

    const std::string outputName = output.string();
    dxfRW writer(outputName.c_str());
    SemanticSink invalid("dxfRW", "write");
    invalid.enablePlaneSurfaceInlinePayloadProbe();
    invalid.configureWrite(&writer);
    if (writer.write(&invalid, DRW::AC1027, binary)
        || invalid.writeRecipeSucceeded() || !destinationMatches(sentinel)) {
        std::cerr << "AC1027 inline plane surface was not refused atomically"
                  << ": binary=" << binary
                  << ", recipe=" << invalid.writeRecipeSucceeded()
                  << ", destinationPreserved="
                  << destinationMatches(sentinel) << '\n';
        return false;
    }

    SemanticSink corrected("dxfRW", "write");
    corrected.configureWrite(&writer);
    if (!writer.write(&corrected, DRW::AC1027, binary)
        || !corrected.writeRecipeSucceeded()) {
        std::cerr << "same-writer corrected plane payload retry failed"
                  << ": binary=" << binary << '\n';
        return false;
    }
    SemanticSink readback("dxfRW", "read");
    dxfRW reader(outputName.c_str());
    if (!reader.read(&readback, false) || !readback.diagnostics.empty()
        || !fieldEquals(readback, "LINE", "start", "{\"x\":1,\"y\":2,\"z\":3}")) {
        std::cerr << "same-writer corrected content failed readback"
                  << ": binary=" << binary << '\n';
        return false;
    }
    return true;
}

bool runModelerCarrierProbe(bool binary) {
    std::error_code error;
    const std::filesystem::path temporaryRoot =
        std::filesystem::temp_directory_path(error);
    if (error)
        return false;
    std::filesystem::path directory;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned attempt = 0; attempt < 32; ++attempt) {
        directory = temporaryRoot / ("libdxfrw-modeler-carrier-probe-"
            + std::to_string(stamp) + "-" + std::to_string(attempt));
        error.clear();
        if (std::filesystem::create_directory(directory, error))
            break;
        if (error && error != std::errc::file_exists)
            return false;
        directory.clear();
    }
    if (directory.empty())
        return false;

    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code cleanupError;
            std::filesystem::remove_all(path, cleanupError);
        }
    } cleanup{directory};

    const std::filesystem::path output = directory
        / (binary ? "modeler-carriers-binary.dxf"
                  : "modeler-carriers-ascii.dxf");
    const std::string outputName = output.string();
    SemanticSink producer("dxfRW", "write");
    producer.enableModelerCarrierProbe();
    dxfRW writer(outputName.c_str());
    producer.configureWrite(&writer);
    const bool writeSucceeded = writer.write(&producer, DRW::AC1024, binary);
    if (!writeSucceeded || !producer.writeRecipeSucceeded()) {
        std::cerr << "modeler carrier probe write failed: binary=" << binary
                  << ", writer=" << writeSucceeded
                  << ", recipe=" << producer.writeRecipeSucceeded()
                  << ", error=" << static_cast<int>(writer.getError()) << '\n';
        return false;
    }

    const auto validate = [&](bool legacyExt) {
        SemanticSink consumer("dxfRW", "read");
        dxfRW reader(outputName.c_str());
        const bool readSucceeded = reader.read(&consumer, legacyExt);
        if (!readSucceeded || !consumer.diagnostics.empty()) {
            std::cerr << "modeler carrier probe read failed: binary=" << binary
                      << ", ext=" << legacyExt
                      << ", reader=" << readSucceeded
                      << ", error=" << static_cast<int>(reader.getError())
                      << ", diagnostics=" << consumer.diagnostics.size() << '\n';
            return false;
        }

        const struct Expected {
            const char* layer;
            DRW::ETYPE type;
            const char* body;
        } expected[] = {
            {"probe_carrier_3dsolid", DRW::E3DSOLID, "solid-body"},
            {"probe_carrier_region", DRW::REGION, "region-body"},
            {"probe_carrier_body", DRW::BODY, "body-body"}};
        for (const Expected& item : expected) {
            const RecordRow* record = findEntityRecordByLayer(
                consumer, "MODELER_GEOMETRY", item.layer);
            const std::string payload = std::string("ACIS 7.0\n")
                + item.body + '\n';
            const std::size_t bodyLength = std::strlen(item.body);
            if (record == nullptr
                || !fieldEqualsForLayer(consumer, "MODELER_GEOMETRY",
                    item.layer, "entityType",
                    std::to_string(static_cast<int>(item.type)))
                || !fieldEqualsForLayer(consumer, "MODELER_GEOMETRY",
                    item.layer, "rawByteCount",
                    std::to_string(payload.size()))
                || !fieldEqualsForLayer(consumer, "MODELER_GEOMETRY",
                    item.layer, "dxfPayloadChunk.count", "2")
                || !fieldEqualsForLayer(consumer, "MODELER_GEOMETRY",
                    item.layer, "dxfPayloadChunk.0.groupCode", "1")
                || !fieldEqualsForLayer(consumer, "MODELER_GEOMETRY",
                    item.layer, "dxfPayloadChunk.0.length", "8")
                || !fieldEqualsForLayer(consumer, "MODELER_GEOMETRY",
                    item.layer, "dxfPayloadChunk.1.groupCode", "1")
                || !fieldEqualsForLayer(consumer, "MODELER_GEOMETRY",
                    item.layer, "dxfPayloadChunk.1.offset", "9")
                || !fieldEqualsForLayer(consumer, "MODELER_GEOMETRY",
                    item.layer, "dxfPayloadChunk.1.length",
                    std::to_string(bodyLength))
                || !fieldEqualsForLayer(consumer, "MODELER_GEOMETRY",
                    item.layer, "hasDwgInlineAcisPayload", "false")
                || !fieldEqualsForLayer(consumer, "MODELER_GEOMETRY",
                    item.layer, "hasDataStorageRecord", "false"))
            {
                std::cerr << "modeler carrier callback mismatch: layer="
                          << item.layer << ", record=" << (record != nullptr)
                          << ", records=" << consumer.records.size()
                          << ", carriers=" << consumer.carriers.size() << '\n';
                return false;
            }

            std::size_t matchingCarriers = 0;
            for (const CarrierRow& carrier : consumer.carriers) {
                if (carrier.recordId == record->id
                    && carrier.source.find("dxfModelerEntityInlinePayload")
                        != std::string::npos
                    && carrier.size == payload.size()
                    && carrier.digest == sha256(payload.data(), payload.size())
                    && carrier.disposition == "preserved")
                    ++matchingCarriers;
            }
            if (matchingCarriers != 1) {
                std::cerr << "modeler carrier identity mismatch: layer="
                          << item.layer << ", matches=" << matchingCarriers
                          << ", payloadBytes=" << payload.size() << '\n';
                return false;
            }
        }
        const auto modelerCarriers = std::count_if(
            consumer.carriers.begin(), consumer.carriers.end(),
            [](const CarrierRow& carrier) {
                return carrier.source.find(
                    "dxfModelerEntityInlinePayload") != std::string::npos;
            });
        if (modelerCarriers != 3) {
            std::cerr << "modeler carrier probe count mismatch: records="
                      << consumer.records.size() << ", carriers="
                      << consumer.carriers.size() << ", inlineModelerCarriers="
                      << modelerCarriers << '\n';
            return false;
        }
        return true;
    };
    const bool currentModePasses = validate(false);
    if (!currentModePasses)
        std::cerr << "modeler carrier probe failed in ext=false mode\n";
    const bool legacyModePasses = validate(true);
    if (!legacyModePasses)
        std::cerr << "modeler carrier probe failed in ext=true mode\n";
    return currentModePasses && legacyModePasses;
}
#endif

template <typename Reader>
bool verifyCleanReadbackObservation(const Reader& reader,
                                    std::string& envelope) {
    envelope = serializeDwgObservationEnvelope(
        reader, "outputReadback", "AC1027");
    if constexpr (HasDwgReadObservations<Reader>::value) {
        const auto observations = reader.getReadObservations();
        return observations.entityParseFailures == 0
            && observations.objectParseFailures == 0
            && static_cast<int>(observations.frames.status) == 2
            && observations.frames.sourceFrames != 0
            && envelope.find("\"availability\":\"available\"")
                != std::string::npos
            && envelope.find("\"phase\":\"outputReadback\"")
                != std::string::npos
            && envelope.find("\"sourceVersion\":\"AC1027\"")
                != std::string::npos
            && envelope.find("\"entityParseFailures\":0")
                != std::string::npos;
    }
    return envelope.find("\"availability\":\"unavailable\"")
            != std::string::npos
        && envelope.find("\"reason\":\"readerApiNotPresent\"")
            != std::string::npos
        && envelope.find("\"sourceVersion\":\"AC1027\"")
            != std::string::npos;
}

bool runCleanDwgReadbackControl() {
    std::error_code error;
    const std::filesystem::path temporaryRoot =
        std::filesystem::temp_directory_path(error);
    if (error)
        return false;
    std::filesystem::path directory;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned attempt = 0; attempt < 32; ++attempt) {
        directory = temporaryRoot / ("libdxfrw-semantic-clean-readback-"
            + std::to_string(stamp) + "-" + std::to_string(attempt));
        error.clear();
        if (std::filesystem::create_directory(directory, error))
            break;
        if (error && error != std::errc::file_exists)
            return false;
        directory.clear();
    }
    if (directory.empty())
        return false;

    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code cleanupError;
            std::filesystem::remove_all(path, cleanupError);
        }
    } cleanup{directory};

    const std::filesystem::path output = directory / "clean-readback.dwg";
    const std::string outputName = output.string();
    SemanticSink producer("dwgRW", "write");
    dwgRW writer(outputName.c_str());
    producer.configureWrite(&writer);
    if (!writer.write(&producer, DRW::AC1027, true)
        || !producer.writeRecipeSucceeded()) {
        std::cerr << "clean DWG readback control generation failed; error="
                  << static_cast<int>(writer.getError()) << '\n';
        return false;
    }

    SemanticSink consumer("dwgRW", "read");
    dwgRW reader(outputName.c_str());
    if (!reader.read(&consumer, false) || reader.getError() != DRW::BAD_NONE
        || !consumer.diagnostics.empty()) {
        std::cerr << "clean DWG readback failed; error="
                  << static_cast<int>(reader.getError())
                  << ", diagnostics=" << consumer.diagnostics.size() << '\n';
        return false;
    }
    const auto lineCount = static_cast<std::size_t>(std::count_if(
        consumer.records.begin(), consumer.records.end(),
        [](const RecordRow& row) { return row.entity == "LINE"; }));
    std::string observations;
    const bool observationsMatch =
        verifyCleanReadbackObservation(reader, observations);
    if (lineCount != 1
        || !fieldEquals(consumer, "LINE", "start", "{\"x\":1,\"y\":2,\"z\":3}")
        || !fieldEquals(consumer, "LINE", "end", "{\"x\":4,\"y\":5,\"z\":6}")
        || !observationsMatch) {
        std::cerr << "clean DWG readback mismatch; lines=" << lineCount
                  << ", observationsMatch=" << observationsMatch
                  << ", observations=" << observations << '\n';
        return false;
    }
    return true;
}

template <typename Summary>
bool namedObservationIsOne(const Summary& summary,
                           const std::string& expectedName) {
    if (summary.instances != 1 || summary.sourceNames != 1
        || summary.retained != 1 || summary.rows[0].instances != 1
        || summary.rows[0].name.length != expectedName.size()
        || summary.rows[0].name.sourceBytes != expectedName.size())
        return false;
    for (std::size_t i = 0; i < expectedName.size(); ++i) {
        if (summary.rows[0].name.bytes[i]
            != static_cast<std::uint8_t>(expectedName[i]))
            return false;
    }
    return true;
}

template <typename Reader>
bool verifyPartialReadbackObservation(const Reader& reader,
                                      std::string& envelope) {
    envelope = serializeDwgObservationEnvelope(
        reader, "outputReadback", "AC1027");
    if constexpr (HasDwgReadObservations<Reader>::value) {
        const auto observations = reader.getReadObservations();
        return observations.entityParseFailures == 1
            && observations.objectParseFailures == 0
            && static_cast<int>(observations.frames.status) == 3
            && observations.frames.sourceFrames != 0
            && observations.frames.dispositions[5] == 1
            && envelope.find("\"entityParseFailures\":1") != std::string::npos
            && envelope.find("\"status\":\"finalizedPartial\"")
                != std::string::npos;
    }
    return envelope.find("\"availability\":\"unavailable\"")
            != std::string::npos
        && envelope.find("\"reason\":\"readerApiNotPresent\"")
            != std::string::npos
        && envelope.find("\"sourceVersion\":\"AC1027\"")
            != std::string::npos;
}

template <typename Reader>
bool verifyOpaqueReadbackObservation(const Reader& reader,
                                     std::string& envelope) {
    envelope = serializeDwgObservationEnvelope(
        reader, "outputReadback", "AC1027");
    if constexpr (HasDwgReadObservations<Reader>::value) {
        const auto observations = reader.getReadObservations();
        return observations.entityParseFailures == 0
            && observations.objectParseFailures == 0
            && namedObservationIsOne(observations.skippedCustomClasses,
                                     "AcDbSemanticOpaqueTest")
            && namedObservationIsOne(observations.skippedUnsupportedObjects,
                                     "SEMANTIC_OPAQUE_TEST")
            && static_cast<int>(observations.frames.status) == 2
            && observations.frames.sourceFrames != 0
            && observations.frames.dispositions[3]
                == observations.frames.sourceFrames;
    }
    return envelope.find("\"availability\":\"unavailable\"")
            != std::string::npos
        && envelope.find("\"reason\":\"readerApiNotPresent\"")
            != std::string::npos
        && envelope.find("\"sourceVersion\":\"AC1027\"")
            != std::string::npos;
}

bool runPartialDwgReadbackControl() {
    std::error_code error;
    const std::filesystem::path temporaryRoot =
        std::filesystem::temp_directory_path(error);
    if (error)
        return false;
    std::filesystem::path directory;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned attempt = 0; attempt < 32; ++attempt) {
        directory = temporaryRoot / ("libdxfrw-semantic-partial-readback-"
            + std::to_string(stamp) + "-" + std::to_string(attempt));
        error.clear();
        if (std::filesystem::create_directory(directory, error))
            break;
        if (error && error != std::errc::file_exists)
            return false;
        directory.clear();
    }
    if (directory.empty())
        return false;

    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code cleanupError;
            std::filesystem::remove_all(path, cleanupError);
        }
    } cleanup{directory};

    const std::filesystem::path output = directory / "partial-readback.dwg";
    const std::string outputName = output.string();

    SemanticSink producer("dwgRW", "write");
    producer.enablePartialDwgReadbackControl();
    dwgRW writer(outputName.c_str());
    producer.configureWrite(&writer);
    if (!writer.write(&producer, DRW::AC1027, true)
        || !producer.writeRecipeSucceeded()) {
        std::cerr << "partial DWG readback control generation failed; error="
                  << static_cast<int>(writer.getError())
                  << ", recipe=" << producer.writeRecipeSucceeded() << '\n';
        return false;
    }

    SemanticSink consumer("dwgRW", "read");
    dwgRW reader(outputName.c_str());
    const bool operationSucceeded = reader.read(&consumer, false);
    std::string observations;
    if constexpr (HasDwgReadObservations<dwgRW>::value) {
        if (!operationSucceeded || !consumer.diagnostics.empty()) {
            std::cerr << "standalone partial DWG readback failed; error="
                      << static_cast<int>(reader.getError())
                      << ", diagnostics=" << consumer.diagnostics.size() << '\n';
            return false;
        }
        const std::size_t lines = static_cast<std::size_t>(std::count_if(
            consumer.records.begin(), consumer.records.end(),
            [](const RecordRow& row) { return row.entity == "LINE"; }));
        const std::size_t hatches = static_cast<std::size_t>(std::count_if(
            consumer.records.begin(), consumer.records.end(),
            [](const RecordRow& row) { return row.entity == "HATCH"; }));
        const bool observationsMatch =
            verifyPartialReadbackObservation(reader, observations);
        if (lines != 1 || hatches != 0 || !observationsMatch) {
            std::cerr << "standalone partial DWG readback mismatch; lines="
                      << lines << ", hatches=" << hatches
                      << ", observationsMatch=" << observationsMatch
                      << ", observations=" << observations << '\n';
            return false;
        }
        return true;
    }

    if (!operationSucceeded || reader.getError() != DRW::BAD_NONE
        || !consumer.diagnostics.empty()
        || !verifyPartialReadbackObservation(reader, observations)) {
        std::cerr << "locked-target partial DWG readback disposition changed; "
                  << "operationSucceeded=" << operationSucceeded
                  << ", error=" << static_cast<int>(reader.getError())
                  << ", diagnostics=" << consumer.diagnostics.size()
                  << ", observations=" << observations << '\n';
        return false;
    }
    return true;
}

bool runOpaqueDwgReadbackControl() {
    std::error_code error;
    const std::filesystem::path temporaryRoot =
        std::filesystem::temp_directory_path(error);
    if (error)
        return false;
    std::filesystem::path directory;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned attempt = 0; attempt < 32; ++attempt) {
        directory = temporaryRoot / ("libdxfrw-semantic-opaque-readback-"
            + std::to_string(stamp) + "-" + std::to_string(attempt));
        error.clear();
        if (std::filesystem::create_directory(directory, error))
            break;
        if (error && error != std::errc::file_exists)
            return false;
        directory.clear();
    }
    if (directory.empty())
        return false;

    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code cleanupError;
            std::filesystem::remove_all(path, cleanupError);
        }
    } cleanup{directory};

    const std::filesystem::path output = directory / "opaque-readback.dwg";
    const std::string outputName = output.string();
    SemanticSink producer("dwgRW", "write");
    producer.enableOpaqueDwgReadbackControl();
    dwgRW writer(outputName.c_str());
    producer.configureWrite(&writer);
    if (!writer.write(&producer, DRW::AC1027, true)
        || !producer.writeRecipeSucceeded()) {
        std::cerr << "opaque DWG readback control generation failed; error="
                  << static_cast<int>(writer.getError()) << '\n';
        return false;
    }

    SemanticSink consumer("dwgRW", "read");
    dwgRW reader(outputName.c_str());
    const bool operationSucceeded = reader.read(&consumer, false);
    std::string observations;
    if (!operationSucceeded || reader.getError() != DRW::BAD_NONE
        || !consumer.diagnostics.empty()) {
        std::cerr << "opaque DWG readback failed; operationSucceeded="
                  << operationSucceeded << ", error="
                  << static_cast<int>(reader.getError())
                  << ", diagnostics=" << consumer.diagnostics.size() << '\n';
        return false;
    }

    const auto lineCount = static_cast<std::size_t>(std::count_if(
        consumer.records.begin(), consumer.records.end(),
        [](const RecordRow& row) { return row.entity == "LINE"; }));
    const RecordRow* opaqueRecord = nullptr;
    std::size_t opaqueRecordCount = 0;
    for (const RecordRow& row : consumer.records) {
        if (row.kind == "unsupported"
            && row.entity == "SEMANTIC_OPAQUE_TEST") {
            opaqueRecord = &row;
            ++opaqueRecordCount;
        }
    }
    const auto hasField = [](const RecordRow* record,
                             const std::string& name,
                             const std::string& expected) {
        if (record == nullptr)
            return false;
        for (const Field& field : record->fields) {
            if (field.name == name)
                return field.value == expected;
        }
        return false;
    };
    std::size_t matchingCarriers = 0;
    const auto hasSourceSuffix = [](const std::string& source,
                                    const std::string& suffix) {
        return source.size() >= suffix.size()
            && source.compare(source.size() - suffix.size(),
                              suffix.size(), suffix) == 0;
    };
    for (const CarrierRow& carrier : consumer.carriers) {
        if (hasSourceSuffix(carrier.source, "/dwgUnsupportedObject")
            && opaqueRecord != nullptr
            && carrier.recordId == opaqueRecord->id && carrier.size != 0)
            ++matchingCarriers;
    }
    std::size_t matchingUnsupportedRows = 0;
    for (const UnsupportedRow& row : consumer.unsupported) {
        if (row.kind == "opaqueObject"
            && hasSourceSuffix(row.source, "/dwg")
            && opaqueRecord != nullptr && row.recordId == opaqueRecord->id
            && row.disposition == "preserved" && !row.carrierId.empty())
            ++matchingUnsupportedRows;
    }
    const bool callbackContract = lineCount == 1
        && fieldEquals(consumer, "LINE", "start", "{\"x\":11,\"y\":12,\"z\":13}")
        && fieldEquals(consumer, "LINE", "end", "{\"x\":21,\"y\":22,\"z\":23}")
        && opaqueRecordCount == 1
        && hasField(opaqueRecord, "handle", jsonString("700"))
        && hasField(opaqueRecord, "isEntity", "false");
    const bool carrierContract = matchingCarriers == 1
        && matchingUnsupportedRows == 1;
    const bool observationsMatch =
        verifyOpaqueReadbackObservation(reader, observations);
    if (!callbackContract || !carrierContract || !observationsMatch) {
        std::cerr << "opaque DWG readback mismatch; lines=" << lineCount
                  << ", opaqueRecords=" << opaqueRecordCount
                  << ", carriers=" << matchingCarriers
                  << ", unsupportedRows=" << matchingUnsupportedRows
                  << ", observationsMatch=" << observationsMatch
                  << ", observations=" << observations << '\n';
        return false;
    }
    return true;
}

int selfTest() {
    const auto fail = [](int line) {
        std::cerr << "semantic adapter self-test failed at line " << line
                  << '\n';
        return 1;
    };
    if (!runCleanDwgReadbackControl())
        return fail(__LINE__);
    if (!runPartialDwgReadbackControl())
        return fail(__LINE__);
    if (!runOpaqueDwgReadbackControl())
        return fail(__LINE__);
    // Older consumers implement addInsert but do not override the newer
    // addTable callback. Keep the default TABLE-to-INSERT projection intact,
    // including for a useful but semantically incomplete TABLE prefix.
    SemanticSink legacyTableSink("DRW_Interface", "read");
    DRW_Table partialTable;
    partialTable.handle = 0x715u;
    partialTable.parentHandle = 0x30u;
    partialTable.name = "PARTIAL_TABLE_BLOCK";
    partialTable.basePoint = DRW_Coord{12.0, 34.0, 5.0};
    partialTable.angle = 0.25;
    partialTable.proxyGraphics = {0x41u, 0x00u, 0x42u};
    partialTable.m_hasSemanticContent = true;
    partialTable.m_semanticContentComplete = false;
    DRW_Interface& legacyCallbacks = legacyTableSink;
    legacyCallbacks.DRW_Interface::addTable(partialTable);
    if (legacyTableSink.records.size() != 1u
        || legacyTableSink.callbacks.size() != 1u
        || legacyTableSink.callbacks.front().kind != "addInsert"
        || legacyTableSink.records.front().entity != "INSERT"
        || !fieldEquals(legacyTableSink, "INSERT", "handle", jsonString("715"))
        || !fieldEquals(legacyTableSink, "INSERT", "blockName",
                        jsonString("PARTIAL_TABLE_BLOCK"))
        || !fieldEquals(legacyTableSink, "INSERT", "insertionPoint",
                        "{\"x\":12,\"y\":34,\"z\":5}")
        || legacyTableSink.carriers.size() != 1u
        || legacyTableSink.carriers.front().source.find(
               "/entityProxyGraphics") == std::string::npos
        || legacyTableSink.carriers.front().size != 3u
        || legacyTableSink.carriers.front().disposition != "preserved"
        || legacyTableSink.carriers.front().recordId
            != legacyTableSink.records.front().id)
        return fail(__LINE__);

    // Consumers predating addMesh inherit a wireframe projection through
    // their established addPolyline/addLine callbacks. Preserve WCS
    // coordinates and presentation attributes, deduplicate explicit edges
    // already drawn by face boundaries, skip malformed topology, and avoid
    // copying MESH identity/ownership onto synthesized records.
    DRW_Mesh fallbackMesh;
    fallbackMesh.handle = 0x716u;
    fallbackMesh.parentHandle = 0x30u;
    fallbackMesh.layer = "MESH_FALLBACK";
    fallbackMesh.lineType = "CONTINUOUS";
    fallbackMesh.color = 3;
    fallbackMesh.color24 = 0x123456;
    fallbackMesh.colorName = "fallback-color";
    fallbackMesh.material = 0x123u;
    fallbackMesh.transparency = 0x5A000000;
    fallbackMesh.plotStyle = 0x456u;
    fallbackMesh.shadow = static_cast<DRW::ShadowMode>(3);
    fallbackMesh.lWeight = DRW_LW_Conv::width11;
    fallbackMesh.ltypeScale = 2.5;
    fallbackMesh.visible = false;
    fallbackMesh.space = DRW::PaperSpace;
    fallbackMesh.vertices = {DRW_Coord{1.0, 2.0, 3.0},
                             DRW_Coord{4.0, 5.0, 6.0},
                             DRW_Coord{7.0, 8.0, 9.0},
                             DRW_Coord{10.0, 11.0, 12.0}};
    fallbackMesh.faces = {{2, 0, 1}, {0, 1}, {0, -1, 2}, {0, 1, 4}};
    fallbackMesh.edges = {{0, 1}, {1, 2}, {0, 3}, {3, 0},
                          {-1, 0}, {0, 9}, {0, 0}};
    SemanticSink legacyMeshSink("DRW_Interface", "read");
    legacyMeshSink.exerciseDefaultMeshFallback(fallbackMesh);
    if (legacyMeshSink.meshFallbackPolylines.size() != 1u
        || legacyMeshSink.meshFallbackLines.size() != 1u)
        return fail(__LINE__);
    const DRW_Polyline& fallbackPolyline =
        legacyMeshSink.meshFallbackPolylines.front();
    if (fallbackPolyline.flags != (1 | 8)
        || fallbackPolyline.vertlist.size() != 3u
        || fallbackPolyline.vertlist[0]->basePoint.x != 7.0
        || fallbackPolyline.vertlist[0]->basePoint.y != 8.0
        || fallbackPolyline.vertlist[0]->basePoint.z != 9.0
        || fallbackPolyline.vertlist[1]->basePoint.x != 1.0
        || fallbackPolyline.vertlist[2]->basePoint.z != 6.0
        || fallbackPolyline.vertlist[0]->flags != 32
        || fallbackPolyline.layer != fallbackMesh.layer
        || fallbackPolyline.lineType != fallbackMesh.lineType
        || fallbackPolyline.color != fallbackMesh.color
        || fallbackPolyline.color24 != fallbackMesh.color24
        || fallbackPolyline.colorName != fallbackMesh.colorName
        || fallbackPolyline.material != fallbackMesh.material
        || fallbackPolyline.transparency != fallbackMesh.transparency
        || fallbackPolyline.plotStyle != fallbackMesh.plotStyle
        || fallbackPolyline.shadow != fallbackMesh.shadow
        || fallbackPolyline.lWeight != fallbackMesh.lWeight
        || fallbackPolyline.ltypeScale != fallbackMesh.ltypeScale
        || fallbackPolyline.visible != fallbackMesh.visible
        || fallbackPolyline.space != fallbackMesh.space
        || fallbackPolyline.handle != DRW::NoHandle
        || fallbackPolyline.parentHandle != DRW::NoHandle)
        return fail(__LINE__);
    const DRW_Line& fallbackLine = legacyMeshSink.meshFallbackLines.front();
    if (fallbackLine.basePoint.x != 1.0
        || fallbackLine.basePoint.y != 2.0
        || fallbackLine.basePoint.z != 3.0
        || fallbackLine.secPoint.x != 10.0
        || fallbackLine.secPoint.y != 11.0
        || fallbackLine.secPoint.z != 12.0
        || fallbackLine.layer != fallbackMesh.layer
        || fallbackLine.color != fallbackMesh.color
        || fallbackLine.material != fallbackMesh.material
        || fallbackLine.transparency != fallbackMesh.transparency
        || fallbackLine.plotStyle != fallbackMesh.plotStyle
        || fallbackLine.shadow != fallbackMesh.shadow
        || fallbackLine.handle != DRW::NoHandle
        || fallbackLine.parentHandle != DRW::NoHandle)
        return fail(__LINE__);

    DRW_3Dface fallbackFace;
    fallbackFace.layer = "face-fallback-layer";
    fallbackFace.lineType = "DASHED";
    fallbackFace.color = 5;
    fallbackFace.color24 = 0x2468AC;
    fallbackFace.colorName = "face-fallback-color";
    fallbackFace.material = 0x234u;
    fallbackFace.transparency = 0x44000000;
    fallbackFace.plotStyle = 0x567u;
    fallbackFace.shadow = static_cast<DRW::ShadowMode>(2);
    fallbackFace.lWeight = DRW_LW_Conv::width13;
    fallbackFace.ltypeScale = 1.75;
    fallbackFace.visible = false;
    fallbackFace.space = DRW::PaperSpace;
    fallbackFace.invisibleflag = DRW_3Dface::SecodEdge;
    fallbackFace.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    fallbackFace.secPoint = DRW_Coord{4.0, 5.0, 6.0};
    fallbackFace.thirdPoint = DRW_Coord{7.0, 8.0, 9.0};
    fallbackFace.fourPoint = DRW_Coord{10.0, 11.0, 12.0};
    SemanticSink legacyFaceSink("DRW_Interface", "read");
    legacyFaceSink.exerciseDefault3DFaceFallback(fallbackFace);
    if (legacyFaceSink.faceFallbackLines.size() != 3u)
        return fail(__LINE__);
    const DRW_Line& firstFaceEdge = legacyFaceSink.faceFallbackLines[0];
    const DRW_Line& thirdFaceEdge = legacyFaceSink.faceFallbackLines[1];
    const DRW_Line& fourthFaceEdge = legacyFaceSink.faceFallbackLines[2];
    if (firstFaceEdge.basePoint.x != 1.0
        || firstFaceEdge.basePoint.y != 2.0
        || firstFaceEdge.basePoint.z != 3.0
        || firstFaceEdge.secPoint.x != 4.0
        || firstFaceEdge.secPoint.y != 5.0
        || firstFaceEdge.secPoint.z != 6.0
        || thirdFaceEdge.basePoint.x != 7.0
        || thirdFaceEdge.secPoint.x != 10.0
        || fourthFaceEdge.basePoint.x != 10.0
        || fourthFaceEdge.secPoint.x != 1.0
        || firstFaceEdge.layer != fallbackFace.layer
        || firstFaceEdge.lineType != fallbackFace.lineType
        || firstFaceEdge.color != fallbackFace.color
        || firstFaceEdge.color24 != fallbackFace.color24
        || firstFaceEdge.colorName != fallbackFace.colorName
        || firstFaceEdge.material != fallbackFace.material
        || firstFaceEdge.transparency != fallbackFace.transparency
        || firstFaceEdge.plotStyle != fallbackFace.plotStyle
        || firstFaceEdge.shadow != fallbackFace.shadow
        || firstFaceEdge.lWeight != fallbackFace.lWeight
        || firstFaceEdge.ltypeScale != fallbackFace.ltypeScale
        || firstFaceEdge.visible != fallbackFace.visible
        || firstFaceEdge.space != fallbackFace.space
        || firstFaceEdge.handle != DRW::NoHandle
        || firstFaceEdge.parentHandle != DRW::NoHandle)
        return fail(__LINE__);
    fallbackFace.invisibleflag = DRW_3Dface::NoEdge;
    fallbackFace.fourPoint = fallbackFace.thirdPoint;
    legacyFaceSink.faceFallbackLines.clear();
    legacyFaceSink.exerciseDefault3DFaceFallback(fallbackFace);
    if (legacyFaceSink.faceFallbackLines.size() != 3u)
        return fail(__LINE__);

    // SOLID/TRACE store the last two raw OCS slots in reverse perimeter
    // order. The legacy fallback applies Autodesk's arbitrary-axis OCS basis
    // before delivering WCS line callbacks.
    DRW_Trace fallbackTrace;
    fallbackTrace.layer = "trace-fallback-layer";
    fallbackTrace.lineType = "DASHED";
    fallbackTrace.color = 6;
    fallbackTrace.color24 = 0x13579B;
    fallbackTrace.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    fallbackTrace.secPoint = DRW_Coord{4.0, 5.0, 3.0};
    fallbackTrace.thirdPoint = DRW_Coord{7.0, 8.0, 3.0};
    fallbackTrace.fourPoint = DRW_Coord{10.0, 11.0, 3.0};
    SemanticSink legacyTraceSink("DRW_Interface", "read");
    legacyTraceSink.exerciseDefaultTraceFallback(fallbackTrace);
    if (legacyTraceSink.traceFallbackLines.size() != 4u)
        return fail(__LINE__);
    const auto& traceEdges = legacyTraceSink.traceFallbackLines;
    if (traceEdges[0].basePoint.x != 1.0
        || traceEdges[0].secPoint.x != 4.0
        || traceEdges[1].basePoint.x != 4.0
        || traceEdges[1].secPoint.x != 10.0
        || traceEdges[2].basePoint.x != 10.0
        || traceEdges[2].secPoint.x != 7.0
        || traceEdges[3].basePoint.x != 7.0
        || traceEdges[3].secPoint.x != 1.0
        || traceEdges[0].layer != fallbackTrace.layer
        || traceEdges[0].lineType != fallbackTrace.lineType
        || traceEdges[0].color != fallbackTrace.color
        || traceEdges[0].color24 != fallbackTrace.color24
        || traceEdges[0].handle != DRW::NoHandle
        || traceEdges[0].parentHandle != DRW::NoHandle)
        return fail(__LINE__);

    fallbackTrace.thickness = 2.0;
    legacyTraceSink.traceFallbackLines.clear();
    legacyTraceSink.exerciseDefaultTraceFallback(fallbackTrace);
    if (legacyTraceSink.traceFallbackLines.size() != 12u)
        return fail(__LINE__);
    const auto& thickTraceEdges = legacyTraceSink.traceFallbackLines;
    if (thickTraceEdges[4].basePoint.x != 1.0
        || thickTraceEdges[4].basePoint.z != 5.0
        || thickTraceEdges[4].secPoint.x != 4.0
        || thickTraceEdges[4].secPoint.z != 5.0
        || thickTraceEdges[5].basePoint.x != 1.0
        || thickTraceEdges[5].basePoint.z != 3.0
        || thickTraceEdges[5].secPoint.x != 1.0
        || thickTraceEdges[5].secPoint.z != 5.0
        || thickTraceEdges[4].layer != fallbackTrace.layer
        || thickTraceEdges[4].handle != DRW::NoHandle
        || thickTraceEdges[4].parentHandle != DRW::NoHandle)
        return fail(__LINE__);

    fallbackTrace.thickness = -1.5;
    legacyTraceSink.traceFallbackLines.clear();
    legacyTraceSink.exerciseDefaultTraceFallback(fallbackTrace);
    if (legacyTraceSink.traceFallbackLines.size() != 12u
        || legacyTraceSink.traceFallbackLines[4].basePoint.z != 1.5
        || legacyTraceSink.traceFallbackLines[5].secPoint.z != 1.5)
        return fail(__LINE__);

    fallbackTrace.thickness = std::numeric_limits<double>::infinity();
    legacyTraceSink.traceFallbackLines.clear();
    legacyTraceSink.exerciseDefaultTraceFallback(fallbackTrace);
    if (legacyTraceSink.traceFallbackLines.size() != 4u)
        return fail(__LINE__);

    DRW_Solid fallbackSolid;
    fallbackSolid.basePoint = DRW_Coord{0.0, 0.0, 2.0};
    fallbackSolid.secPoint = DRW_Coord{3.0, 0.0, 2.0};
    fallbackSolid.thirdPoint = DRW_Coord{3.0, 2.0, 2.0};
    fallbackSolid.fourPoint = fallbackSolid.thirdPoint;
    SemanticSink legacySolidSink("DRW_Interface", "read");
    legacySolidSink.exerciseDefaultSolidFallback(fallbackSolid);
    if (legacySolidSink.traceFallbackLines.size() != 3u)
        return fail(__LINE__);
    fallbackSolid.thickness = 1.25;
    legacySolidSink.traceFallbackLines.clear();
    legacySolidSink.exerciseDefaultSolidFallback(fallbackSolid);
    if (legacySolidSink.traceFallbackLines.size() != 9u
        || legacySolidSink.traceFallbackLines[3].basePoint.z != 3.25
        || legacySolidSink.traceFallbackLines[6].basePoint.z != 2.0
        || legacySolidSink.traceFallbackLines[6].secPoint.z != 3.25)
        return fail(__LINE__);

    fallbackTrace.extPoint = DRW_Coord{0.0, 2.0, 0.0};
    fallbackTrace.thickness = 2.0;
    legacyTraceSink.traceFallbackLines.clear();
    legacyTraceSink.exerciseDefaultTraceFallback(fallbackTrace);
    if (legacyTraceSink.traceFallbackLines.size() != 12u
        || legacyTraceSink.traceFallbackLines[0].basePoint.x != -1.0
        || legacyTraceSink.traceFallbackLines[0].basePoint.y != 3.0
        || legacyTraceSink.traceFallbackLines[0].basePoint.z != 2.0
        || legacyTraceSink.traceFallbackLines[0].secPoint.x != -4.0
        || legacyTraceSink.traceFallbackLines[0].secPoint.y != 3.0
        || legacyTraceSink.traceFallbackLines[0].secPoint.z != 5.0
        || legacyTraceSink.traceFallbackLines[4].basePoint.y != 5.0
        || legacyTraceSink.traceFallbackLines[5].basePoint.y != 3.0
        || legacyTraceSink.traceFallbackLines[5].secPoint.y != 5.0)
        return fail(__LINE__);

    fallbackTrace.extPoint = DRW_Coord{0.0, 0.0, 0.0};
    legacyTraceSink.traceFallbackLines.clear();
    legacyTraceSink.exerciseDefaultTraceFallback(fallbackTrace);
    if (!legacyTraceSink.traceFallbackLines.empty())
        return fail(__LINE__);

    SemanticSink dimensionSink("dxfRW", "read");
    DRW_DimAligned dimension;
    dimension.setDefPoint({-3, -4, 8});
    dimension.setTextPoint({2, 3, 4});
    dimension.setClonePoint({1, -2, 4});
    dimension.setExtrusion({0, 1, 0});
    dimensionSink.addDimAlign(&dimension);
    if (!fieldEquals(dimensionSink, "DIMALIGNED", "definitionPointWcs", "{\"x\":-3,\"y\":-4,\"z\":8}")
        || !fieldEquals(dimensionSink, "DIMALIGNED", "textPointOcs", "{\"x\":2,\"y\":3,\"z\":4}")
        || !fieldEquals(dimensionSink, "DIMALIGNED", "clonePointOcs", "{\"x\":1,\"y\":-2,\"z\":4}")
        || !fieldEquals(dimensionSink, "DIMALIGNED", "extrusion", "{\"x\":0,\"y\":1,\"z\":0}"))
        return fail(__LINE__);
    if (sha256("abc") != "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
        return fail(__LINE__);
    if (jsonString("a\n\"b") != "\"a\\n\\\"b\"")
        return fail(__LINE__);
    std::string oneRecord;
    appendLengthFramed(oneRecord, "1:0:a\n2:0:b");
    std::string twoRecords;
    appendLengthFramed(twoRecords, "1:0:a");
    appendLengthFramed(twoRecords, "2:0:b");
    if (oneRecord == twoRecords)
        return fail(__LINE__);
    try {
        (void)number(std::numeric_limits<double>::infinity());
        return fail(__LINE__);
    } catch (const std::domain_error&) {
    }
    const DrawingIdentity dwgIdentity = classifyDrawingPrefix("AC1024payload");
    if (dwgIdentity.format != "dwg" || dwgIdentity.version != "AC1024")
        return fail(__LINE__);
    const DrawingIdentity asciiIdentity = classifyDrawingPrefix(
        "  0\nSECTION\n  2\nHEADER\n  9\n$ACADVER\n  1\nAC1021\n  0\nENDSEC\n");
    if (asciiIdentity.format != "dxf-ascii" || asciiIdentity.version != "AC1021")
        return fail(__LINE__);
    std::string binary("AutoCAD Binary DXF\r\n");
    binary.push_back(static_cast<char>(0x1a));
    binary.push_back('\0');
    binary.push_back(static_cast<char>(9));
    binary.push_back('\0');
    binary.append("$ACADVER");
    binary.push_back('\0');
    binary.push_back(static_cast<char>(1));
    binary.push_back('\0');
    binary.append("AC1027");
    binary.push_back('\0');
    const DrawingIdentity binaryIdentity = classifyDrawingPrefix(binary);
    if (binaryIdentity.format != "dxf-binary" || binaryIdentity.version != "AC1027")
        return fail(__LINE__);
    if (classifyDrawingPrefix("{\"version\":\"AC1024\"}\n").format != "unknown")
        return fail(__LINE__);
    if (classifyDrawingPrefix("AutoCAD Binary DXFAC1024").format != "unknown")
        return fail(__LINE__);
    if (failureStage(static_cast<int>(DRW::BAD_READ_SECTION)) != "section" ||
        failureStage(static_cast<int>(DRW::BAD_CODE_PARSED)) != "parseCode")
        return fail(__LINE__);

#ifdef LIBDXFRW_SEMANTIC_3D_CONSUMER_PROBE
    if (!run3DConsumerProbe(false) || !run3DConsumerProbe(true)
        || !runNestedMTextAttributeProbe(false)
        || !runNestedMTextAttributeProbe(true)
        || !runPlaneSurfaceInlinePayloadRefusal(false)
        || !runPlaneSurfaceInlinePayloadRefusal(true)
        || !runModelerCarrierProbe(false) || !runModelerCarrierProbe(true))
        return fail(__LINE__);
#endif

    const auto fieldValue = [](const SemanticSink& sink, const std::string& name) {
        if (sink.records.empty())
            return std::string();
        for (const Field& field : sink.records.front().fields) {
            if (field.name == name)
                return field.value;
        }
        return std::string();
    };
    DRW_Mesh firstMesh;
    firstMesh.vertices.push_back(DRW_Coord{1.0, 2.0, 3.0});
    firstMesh.faces.push_back({0, 0, 0});
    SemanticSink firstMeshSink("dxfRW", "read");
    firstMeshSink.addMesh(firstMesh);
    DRW_Mesh changedMesh = firstMesh;
    changedMesh.vertices[0].z = 4.0;
    SemanticSink changedMeshSink("dxfRW", "read");
    changedMeshSink.addMesh(changedMesh);
    if (fieldValue(firstMeshSink, "vertex.0") == std::string()
        || fieldValue(firstMeshSink, "vertex.0")
            == fieldValue(changedMeshSink, "vertex.0")
        || fieldValue(firstMeshSink, "face.0.vertex.2")
            != "0")
        return fail(__LINE__);

    DRW_Insert insert;
    insert.name = "block";
    insert.basePoint = DRW_Coord{2.0, 3.0, 4.0};
    insert.extPoint = DRW_Coord{0.0, 1.0, 0.0};
    insert.xscale = -2.0;
    auto attribute = std::make_shared<DRW_Attrib>();
    attribute->tag = "TAG";
    attribute->text = "value";
    attribute->basePoint = DRW_Coord{5.0, 6.0, 7.0};
    attribute->extPoint = DRW_Coord{0.0, 0.0, 1.0};
    attribute->attribFlags = 4;
    attribute->m_fieldLength = 12;
    insert.attlist.push_back(attribute);
    SemanticSink insertSink("dxfRW", "read");
    insertSink.addInsert(insert);
    if (fieldValue(insertSink, "insertionPoint") != "{\"x\":2,\"y\":3,\"z\":4}"
        || fieldValue(insertSink, "xScale") != "-2"
        || fieldValue(insertSink, "attribute.0.tag") != jsonString("TAG")
        || fieldValue(insertSink, "attribute.0.extrusion")
            != "{\"x\":0,\"y\":0,\"z\":1}"
        || fieldValue(insertSink, "attribute.0.flags") != "4"
        || fieldValue(insertSink, "attribute.0.fieldLength") != "12")
        return fail(__LINE__);

    DRW_LoftedSurface lofted;
    lofted.rawAcisData = {0x11, 0x22, 0x33};
    const bool hasSurfaceDxfPayloadChunks = setSurfaceDxfPayloadChunk(lofted);
    lofted.dxfReferenceData.emplace_back(90, std::int32_t{3});
    lofted.dxfReferenceData.emplace_back(310,
        std::vector<std::uint8_t>{0x41, 0x42, 0x43});
    SemanticSink loftedSink("dxfRW", "read");
    loftedSink.addSurface(&lofted);
    bool foundLoftedBinaryReference = false;
    for (const CarrierRow& carrier : loftedSink.carriers) {
        if (carrier.source.find("loftDxfReference.1.group310")
                != std::string::npos
            && carrier.size == 3
            && carrier.digest == sha256(lofted.dxfReferenceData[1].binary()->data(), 3))
            foundLoftedBinaryReference = true;
    }
    if (fieldValue(loftedSink, "dxfReferenceData.0.value") != "3"
        || fieldValue(loftedSink, "dxfReferenceData.1.byteCount") != "3"
        || loftedSink.carriers.size() != 2
        || !foundLoftedBinaryReference)
        return fail(__LINE__);
    if ((hasSurfaceDxfPayloadChunks
            && (!fieldEquals(loftedSink, "LOFTEDSURFACE", "dxfAcisChunk.count", "1")
                || !fieldEquals(loftedSink, "LOFTEDSURFACE",
                                "dxfAcisChunk.0.groupCode", "310")
                || !fieldEquals(loftedSink, "LOFTEDSURFACE",
                                "dxfAcisChunk.0.inBounds", "true")))
        || (!hasSurfaceDxfPayloadChunks
            && !hasExcludedField(loftedSink, "dxfAcisChunks")))
        return fail(__LINE__);

    DRW_ModelerGeometry modeler(DRW::E3DSOLID);
    modeler.handle = 0x101u;
    modeler.m_historyHandle = 0x202u;
    modeler.m_rawBytes = {0x41, 0x42, 0x43};
    const std::vector<std::uint8_t> inlineAcisPayload{0x46, 0x47, 0x48};
    const bool hasDwgSourceVersion =
        setModelerDwgSourceVersion(modeler, DRW::AC1024);
    const bool hasDwgAcisPayload =
        setModelerDwgAcisPayload(modeler, inlineAcisPayload);
    const bool hasModelerDxfPayloadChunks =
        setModelerDxfPayloadChunk(modeler);
    DRW_ModelerPayloadRange payloadRange;
    payloadRange.m_offset = 1;
    payloadRange.m_length = 2;
    const bool hasPayloadRangeBitOffset =
        setPayloadRangeBitOffset(payloadRange, 13);
    modeler.m_payloadRanges.push_back(payloadRange);
    modeler.hasDataStorageRecord = true;
    modeler.setHasDataStorageBinaryData(true);
    modeler.dataStorageHandle = 0x123456789u;
    modeler.dataStorageHandleKey = "123456789";
    modeler.dataStorageData = {0x44, 0x45};
    SemanticSink modelerSink("dwgRW", "read");
    modelerSink.addModelerGeometry(modeler);
    bool foundFrameCarrier = false;
    bool foundInlineAcisCarrier = false;
    bool foundDataStorageCarrier = false;
    bool foundHistoryReference = false;
    for (const EdgeRow& edge : modelerSink.edges) {
        if (edge.kind == "historyReference"
            && edge.from == "node:" + modelerSink.records.front().id
            && edge.to == "node:handle:202"
            && graphEdgeDisposition(modelerSink, edge) == "unresolved") {
            foundHistoryReference = true;
        }
    }
    for (const CarrierRow& carrier : modelerSink.carriers) {
        if (carrier.source.find("dwgModelerObjectFrameBody-unclassified-payload")
            != std::string::npos) {
            foundFrameCarrier = carrier.size == modeler.m_rawBytes.size()
                && carrier.digest == sha256(modeler.m_rawBytes.data(),
                                             modeler.m_rawBytes.size());
        } else if (carrier.source.find("dwgModelerInlineAcisPayload")
                   != std::string::npos) {
            foundInlineAcisCarrier = carrier.size == inlineAcisPayload.size()
                && carrier.digest == sha256(inlineAcisPayload.data(),
                                             inlineAcisPayload.size());
        } else if (carrier.source.find("dwgLinkedDataStoragePayload")
                   != std::string::npos) {
            foundDataStorageCarrier = carrier.size == modeler.dataStorageData.size()
                && carrier.digest == sha256(modeler.dataStorageData.data(),
                                             modeler.dataStorageData.size());
        }
    }
    if (fieldValue(modelerSink, "rawByteCarrierKind")
            != jsonString("dwgObjectFrameBody-unclassified-payload")
        || fieldValue(modelerSink, "hasRawBytes") != "true"
        || fieldValue(modelerSink, "rawByteCount") != "3"
        || fieldValue(modelerSink, "hasDataStorageBinaryData") != "true"
        || fieldValue(modelerSink, "hasDataStorageRecord") != "true"
        || fieldValue(modelerSink, "historyHandle") != jsonString("202")
        || fieldValue(modelerSink, "dataStorageByteCount") != "2"
        || fieldValue(modelerSink, "dataStorageHandle") != "4886718345"
        || modelerSink.carriers.size() != (hasDwgAcisPayload ? 3u : 2u)
        || !foundFrameCarrier || foundInlineAcisCarrier != hasDwgAcisPayload
        || !foundDataStorageCarrier || !foundHistoryReference) {
        std::cerr << "modeler semantic self-test mismatch; rawCarrier="
                  << fieldValue(modelerSink, "rawByteCarrierKind")
                  << ", hasRawBytes="
                  << fieldValue(modelerSink, "hasRawBytes")
                  << ", rawByteCount="
                  << fieldValue(modelerSink, "rawByteCount")
                  << ", hasDataStorageRecord="
                  << fieldValue(modelerSink, "hasDataStorageRecord")
                  << ", dataStorageByteCount="
                  << fieldValue(modelerSink, "dataStorageByteCount")
                  << ", dataStorageHandle="
                  << fieldValue(modelerSink, "dataStorageHandle")
                  << ", carriers=" << modelerSink.carriers.size()
                  << ", foundFrame=" << foundFrameCarrier
                  << ", foundInlineAcis=" << foundInlineAcisCarrier
                  << ", expectsInlineAcis=" << hasDwgAcisPayload
                  << ", foundDataStorage=" << foundDataStorageCarrier
                  << ", foundHistoryReference=" << foundHistoryReference << '\n';
        return fail(__LINE__);
    }
    if ((hasDwgSourceVersion
            && !fieldEquals(modelerSink, "MODELER_GEOMETRY", "dwgSourceVersion",
                            std::to_string(static_cast<int>(DRW::AC1024))))
        || (!hasDwgSourceVersion
            && !hasExcludedField(modelerSink, "dwgSourceVersion"))
        || (hasDwgAcisPayload
            && (fieldValue(modelerSink, "hasDwgInlineAcisPayload") != "true"
                || !fieldEquals(modelerSink, "MODELER_GEOMETRY",
                                "dwgInlineAcisByteCount", "3")))
        || (!hasDwgAcisPayload
            && !hasExcludedField(modelerSink, "dwgInlineAcisPayload"))
        || (hasModelerDxfPayloadChunks
            && (!fieldEquals(modelerSink, "MODELER_GEOMETRY",
                             "dxfPayloadChunk.count", "1")
                || !fieldEquals(modelerSink, "MODELER_GEOMETRY",
                                "dxfPayloadChunk.0.groupCode", "310")
                || !fieldEquals(modelerSink, "MODELER_GEOMETRY",
                                "dxfPayloadChunk.0.inBounds", "true")))
        || (!hasModelerDxfPayloadChunks
            && !hasExcludedField(modelerSink, "dxfPayloadChunks"))
        || (hasPayloadRangeBitOffset
            && !fieldEquals(modelerSink, "MODELER_GEOMETRY",
                            "payloadRange.0.bitOffset", "13"))
        || (!hasPayloadRangeBitOffset
            && !hasExcludedField(modelerSink, "payloadRange.0.bitOffset")))
        return fail(__LINE__);
    if ((HasModelerPayloadStatusName<DRW_ModelerGeometry>::value
            && fieldValue(modelerSink, "modelerPayloadStatus")
                != jsonString("carrier-bytes-present"))
        || (!HasModelerPayloadStatusName<DRW_ModelerGeometry>::value
            && !hasExcludedField(modelerSink, "modelerPayloadStatus")))
        return fail(__LINE__);

    // Keep a supported-but-empty optional payload distinct from a capability
    // that this adapter build cannot observe. In particular, empty data must
    // not create a carrier or imply that an ACIS body was present.
    DRW_ModelerGeometry emptyModeler(DRW::E3DSOLID);
    emptyModeler.m_isEmpty = true;
    SemanticSink emptyModelerSink("dwgRW", "read");
    emptyModelerSink.addModelerGeometry(emptyModeler);
    if (fieldValue(emptyModelerSink, "hasRawBytes") != "false"
        || fieldValue(emptyModelerSink, "rawByteCount") != "0"
        || fieldValue(emptyModelerSink, "hasDataStorageBinaryData") != "false"
        || fieldValue(emptyModelerSink, "hasDataStorageRecord") != "false"
        || fieldValue(emptyModelerSink, "dataStorageByteCount") != "0"
        || fieldValue(emptyModelerSink, "payloadRangeCount") != "0"
        || !emptyModelerSink.carriers.empty())
        return fail(__LINE__);

    DRW_ModelerGeometry resolvedModeler(DRW::E3DSOLID);
    resolvedModeler.handle = 0x303u;
    resolvedModeler.m_historyHandle = 0x404u;
    SemanticSink resolvedHistorySink("dxfRW", "read");
    resolvedHistorySink.addModelerGeometry(resolvedModeler);
    DRW_Line capturedHistoryRecord;
    capturedHistoryRecord.handle = 0x404u;
    resolvedHistorySink.addLine(capturedHistoryRecord);
    bool historyTargetResolved = false;
    for (const EdgeRow& edge : resolvedHistorySink.edges) {
        if (edge.kind == "historyReference"
            && edge.to == "node:handle:404"
            && graphEdgeDisposition(resolvedHistorySink, edge) == "resolved") {
            historyTargetResolved = true;
        }
    }
    if (!historyTargetResolved)
        return fail(__LINE__);

    // A zero parent handle is the adapter's null-owner representation: it
    // emits neither ownerHandle nor an owner edge. Nonzero owner edges are
    // resolved only against the complete captured record set, independent of
    // callback order.
    DRW_Line nullOwnerLine;
    nullOwnerLine.handle = 0x505u;
    SemanticSink nullOwnerSink("dxfRW", "read");
    nullOwnerSink.addLine(nullOwnerLine);
    if (!nullOwnerSink.records.front().ownerHandle.empty()
        || std::any_of(nullOwnerSink.edges.begin(), nullOwnerSink.edges.end(),
                       [](const EdgeRow& edge) { return edge.kind == "owner"; }))
        return fail(__LINE__);

    DRW_Line unresolvedOwnerLine;
    unresolvedOwnerLine.handle = 0x606u;
    unresolvedOwnerLine.parentHandle = 0x707u;
    SemanticSink unresolvedOwnerSink("dxfRW", "read");
    unresolvedOwnerSink.addLine(unresolvedOwnerLine);
    bool ownerTargetUnresolved = false;
    for (const EdgeRow& edge : unresolvedOwnerSink.edges) {
        if (edge.kind == "owner" && edge.to == "node:handle:707"
            && graphEdgeDisposition(unresolvedOwnerSink, edge) == "unresolved") {
            ownerTargetUnresolved = true;
        }
    }
    if (!ownerTargetUnresolved)
        return fail(__LINE__);

    DRW_Line resolvedOwnerLine;
    resolvedOwnerLine.handle = 0x808u;
    resolvedOwnerLine.parentHandle = 0x909u;
    SemanticSink resolvedOwnerSink("dxfRW", "read");
    resolvedOwnerSink.addLine(resolvedOwnerLine);
    DRW_Line capturedOwnerRecord;
    capturedOwnerRecord.handle = 0x909u;
    resolvedOwnerSink.addLine(capturedOwnerRecord);
    bool ownerTargetResolved = false;
    for (const EdgeRow& edge : resolvedOwnerSink.edges) {
        if (edge.kind == "owner" && edge.to == "node:handle:909"
            && graphEdgeDisposition(resolvedOwnerSink, edge) == "resolved") {
            ownerTargetResolved = true;
        }
    }
    if (!ownerTargetResolved)
        return fail(__LINE__);

    SemanticSink blockOwnerSink = resolvedOwnerSink;
    for (NodeRow& node : blockOwnerSink.nodes) {
        if (node.id == "node:handle:909")
            node.kind = "block";
    }
    bool blockOwnerTargetResolved = false;
    for (const EdgeRow& edge : blockOwnerSink.edges) {
        if (edge.kind == "owner" && edge.to == "node:handle:909"
            && graphEdgeDisposition(blockOwnerSink, edge) == "resolved") {
            blockOwnerTargetResolved = true;
        }
    }
    if (!blockOwnerTargetResolved)
        return fail(__LINE__);

    if ((hasDwgAcisPayload
            && (fieldValue(emptyModelerSink, "hasDwgInlineAcisPayload") != "false"
                || fieldValue(emptyModelerSink, "dwgInlineAcisByteCount") != "0"))
        || (!hasDwgAcisPayload
            && !hasExcludedField(emptyModelerSink, "dwgInlineAcisPayload"))
        || (hasModelerDxfPayloadChunks
            && !fieldEquals(emptyModelerSink, "MODELER_GEOMETRY",
                            "dxfPayloadChunk.count", "0"))
        || (!hasModelerDxfPayloadChunks
            && !hasExcludedField(emptyModelerSink, "dxfPayloadChunks")))
        return fail(__LINE__);
    if ((HasModelerPayloadStatusName<DRW_ModelerGeometry>::value
            && fieldValue(emptyModelerSink, "modelerPayloadStatus")
                != jsonString("explicitly-empty"))
        || (!HasModelerPayloadStatusName<DRW_ModelerGeometry>::value
            && !hasExcludedField(emptyModelerSink, "modelerPayloadStatus")))
        return fail(__LINE__);

    DRW_ModelerGeometry missingPayloadModeler(DRW::E3DSOLID);
    missingPayloadModeler.m_isEmpty = false;
    missingPayloadModeler.m_hasModelerData = true;
    missingPayloadModeler.m_bodyBitSize = 24;
    missingPayloadModeler.m_objectSize = 3;
    missingPayloadModeler.m_rawBytes = {0x44, 0x57, 0x47};
    SemanticSink missingPayloadSink("dwgRW", "read");
    missingPayloadSink.addModelerGeometry(missingPayloadModeler);
    if ((HasModelerPayloadStatusName<DRW_ModelerGeometry>::value
            && fieldValue(missingPayloadSink, "modelerPayloadStatus")
                != jsonString("expected-but-unavailable"))
        || (!HasModelerPayloadStatusName<DRW_ModelerGeometry>::value
            && !hasExcludedField(missingPayloadSink, "modelerPayloadStatus")))
        return fail(__LINE__);

    SemanticSink failureSink;
    failureSink.addOperationFailure(2, "write", "/output", "write");
    if (failureSink.diagnostics.size() != 1 ||
        failureSink.diagnostics[0].code != "write-error-2" ||
        failureSink.diagnostics[0].path != "/output")
        return fail(__LINE__);
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        Options options = parseOptions(argc, argv);
        if (options.printCapabilityProfile) {
            if (argc != 2 || std::string(argv[1]) != "--capability-profile")
                throw std::runtime_error(
                    "--capability-profile must be used alone");
            emitCapabilityProfile(std::cout);
            std::cout << '\n';
            return 0;
        }
        if (options.selfTest)
            return selfTest();
        if (options.input.empty() && options.recipe.empty())
            throw std::runtime_error("--input is required");
        if (options.facade != "dxfRW" && options.facade != "dwgRW")
            throw std::runtime_error("--facade must be dxfRW or dwgRW");
        if (options.direction != "read" && options.direction != "write")
            throw std::runtime_error("--direction must be read or write");
        if (options.compatibilityProfile != "default")
            throw std::runtime_error(
                "only the cross-package default compatibility profile is supported");

        const bool writeMode = options.direction == "write";
        if (writeMode) {
            if (options.recipe.empty())
                options.recipe = options.input == "minimal-line-v1"
                    ? "minimal-line-v1" : "minimal-line-v1";
            if (options.recipe != "minimal-line-v1")
                throw std::runtime_error("only the minimal-line-v1 recipe is supported");
            if (options.output.empty())
                throw std::runtime_error("--output is required for write direction");
            if (options.originKind == "localFile")
                options.originKind = "localFromScratch";
            if (options.inputId.empty())
                options.inputId = "minimal-line-v1";
            if (options.inputPathHint.empty())
                options.inputPathHint = "local/minimal-line-v1";
            if (options.generatorDigest.empty())
                options.generatorDigest = sha256(minimalLineRecipe());
        } else {
            if (options.inputId.empty())
                options.inputId = logicalBasename(options.input);
            if (options.inputPathHint.empty())
                options.inputPathHint = logicalBasename(options.input);
        }
        if (!validPathHint(options.inputPathHint))
            throw std::runtime_error("--input-path-hint must be a normalized relative POSIX path");

        InputInfo input;
        if (writeMode) {
            if (options.input != "minimal-line-v1" && !options.input.empty()) {
                verifyMinimalLineRecipe(options.input);
                input = inspectInput(options.input);
                input.format = "recipe-json";
                input.version = "AC1024";
            } else {
                input = recipeInputInfo(options.facade);
                input.format = "recipe-json";
            }
        } else {
            input = inspectInput(options.input);
        }
        SemanticSink sink(options.facade, options.direction);
        const bool dwgFacade = options.facade == "dwgRW";
        std::string inputReadObservations = unavailableObservationEnvelope(
            "inputRead", dwgFacade && !writeMode ? "unavailable" : "notApplicable",
            dwgFacade && !writeMode ? "readerApiNotPresent" :
                (writeMode ? "writeRecipeNotDwg" : "facadeIsDxf"),
            dwgFacade && !writeMode ? input.version : "");
        std::string outputReadbackObservations = unavailableObservationEnvelope(
            "outputReadback",
            dwgFacade && writeMode ? "notRun" : "notApplicable",
            dwgFacade && writeMode ? "writeFailedBeforeReadback" :
                (writeMode ? "facadeIsDxf" : "readOnlyNoOutputReadback"),
            "");
        bool succeeded = false;
        int error = 1;
        std::string stage = "invocation";
        if (writeMode) {
            bool wrote = false;
            if (options.facade == "dxfRW") {
                dxfRW writer(options.output.c_str());
                sink.configureWrite(&writer);
                wrote = writer.write(&sink, DRW::AC1024, false);
                error = static_cast<int>(writer.getError());
            } else {
                dwgRW writer(options.output.c_str());
                sink.configureWrite(&writer);
                wrote = writer.write(&sink, DRW::AC1024, false);
                error = static_cast<int>(writer.getError());
            }
            wrote = wrote && sink.writeRecipeSucceeded();
            if (!wrote) {
                stage = "write";
                sink.addOperationFailure(error, stage, "/output", "write");
            } else {
                const InputInfo generated = inspectInput(options.output);
                if (!sink.addWriteResult(options.output, generated.digest,
                                         generated.size, generated.format,
                                         generated.version)) {
                    error = 1;
                    stage = "writeResult";
                    sink.addOperationFailure(error, stage, "/output", "write-result");
                } else if (options.facade == "dxfRW") {
                    dxfRW reader(options.output.c_str());
                    succeeded = reader.read(&sink, options.applyExtrusion);
                    error = static_cast<int>(reader.getError());
                    stage = succeeded ? "none" : "readback";
                    if (!succeeded)
                        sink.addOperationFailure(error, stage, "/output", "readback");
                } else {
                    dwgRW reader(options.output.c_str());
                    succeeded = reader.read(&sink, options.applyExtrusion);
                    error = static_cast<int>(reader.getError());
                    outputReadbackObservations = serializeDwgObservationEnvelope(
                        reader, "outputReadback", generated.version);
                    stage = succeeded ? "none" : "readback";
                    if (!succeeded)
                        sink.addOperationFailure(error, stage, "/output", "readback");
                    for (const DwgIntegrityDiagnostic& diagnostic :
                         reader.getIntegrityDiagnostics())
                        sink.addIntegrityDiagnostic(diagnostic);
                }
            }
        } else if (options.facade == "dxfRW") {
            dxfRW reader(options.input.c_str());
            succeeded = reader.read(&sink, options.applyExtrusion);
            error = static_cast<int>(reader.getError());
            stage = failureStage(error);
            if (!succeeded)
                sink.addOperationFailure(error, stage, "/input", "read");
        } else {
            dwgRW reader(options.input.c_str());
            succeeded = reader.read(&sink, options.applyExtrusion);
            error = static_cast<int>(reader.getError());
            stage = failureStage(error);
            if (!succeeded)
                sink.addOperationFailure(error, stage, "/input", "read");
            const std::vector<DwgIntegrityDiagnostic> diagnostics =
                reader.getIntegrityDiagnostics();
            for (const DwgIntegrityDiagnostic& diagnostic : diagnostics)
                sink.addIntegrityDiagnostic(diagnostic);
            inputReadObservations = serializeDwgObservationEnvelope(
                reader, "inputRead", input.version);
        }
        emitResult(std::cout, options, input, sink, succeeded, error, stage,
                   inputReadObservations, outputReadbackObservations);
        return succeeded ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "semantic adapter: " << error.what() << '\n';
        return 2;
    }
}
