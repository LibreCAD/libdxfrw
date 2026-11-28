/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DWGAPPINFO_H
#define DWGAPPINFO_H

#include <cstddef>
#include <cstdint>
#include <cctype>
#include <string>

namespace dwgAppInfo {

enum class R21ProducerProfile : std::uint8_t {
    Unknown,
    AutoCAD2007
};

namespace detail {

class Cursor {
public:
    Cursor(const std::uint8_t *data, std::size_t size) noexcept
        : m_data(data), m_size(size) {}

    bool readUInt32(std::uint32_t& value) noexcept {
        if (m_data == nullptr || m_size - m_position < 4u)
            return false;
        value = static_cast<std::uint32_t>(m_data[m_position])
            | (static_cast<std::uint32_t>(m_data[m_position + 1u]) << 8u)
            | (static_cast<std::uint32_t>(m_data[m_position + 2u]) << 16u)
            | (static_cast<std::uint32_t>(m_data[m_position + 3u]) << 24u);
        m_position += 4u;
        return true;
    }

    bool skip(std::size_t count) noexcept {
        if (m_data == nullptr || count > m_size - m_position)
            return false;
        m_position += count;
        return true;
    }

    bool readUtf16(std::string *ascii) {
        std::uint16_t unitCount = 0;
        if (!readUInt16(unitCount) || unitCount == 0u
            || static_cast<std::size_t>(unitCount)
                   > (m_size - m_position) / 2u)
            return false;

        const std::size_t end = m_position
            + static_cast<std::size_t>(unitCount) * 2u;
        while (m_position < end) {
            const std::uint16_t codeUnit = static_cast<std::uint16_t>(
                m_data[m_position]
                | (static_cast<std::uint16_t>(m_data[m_position + 1u]) << 8u));
            const bool terminator = m_position + 2u == end;
            m_position += 2u;
            if (terminator) {
                if (codeUnit != 0u)
                    return false;
            } else if (ascii != nullptr) {
                if (codeUnit > 0x7fu)
                    return false;
                ascii->push_back(static_cast<char>(codeUnit));
            }
        }
        return true;
    }

private:
    bool readUInt16(std::uint16_t& value) noexcept {
        if (m_data == nullptr || m_size - m_position < 2u)
            return false;
        value = static_cast<std::uint16_t>(m_data[m_position])
            | (static_cast<std::uint16_t>(m_data[m_position + 1u]) << 8u);
        m_position += 2u;
        return true;
    }

    const std::uint8_t *m_data{nullptr};
    std::size_t m_size{0u};
    std::size_t m_position{0u};
};

inline bool readAttribute(const std::string& xml, const char *name,
                          std::string& value) {
    const std::string key{name};
    std::size_t position = 0u;
    while ((position = xml.find(key, position)) != std::string::npos) {
        const bool leftBoundary = position == 0u
            || !(std::isalnum(static_cast<unsigned char>(xml[position - 1u]))
                 || xml[position - 1u] == '_');
        std::size_t cursor = position + key.size();
        if (!leftBoundary) {
            position = cursor;
            continue;
        }
        while (cursor < xml.size()
               && std::isspace(static_cast<unsigned char>(xml[cursor])))
            ++cursor;
        if (cursor == xml.size() || xml[cursor] != '=') {
            position = cursor;
            continue;
        }
        ++cursor;
        while (cursor < xml.size()
               && std::isspace(static_cast<unsigned char>(xml[cursor])))
            ++cursor;
        const bool escapedDelimiter = cursor + 1u < xml.size()
            && xml[cursor] == '\\'
            && (xml[cursor + 1u] == '"' || xml[cursor + 1u] == '\'');
        if (escapedDelimiter)
            ++cursor;
        if (cursor == xml.size()
            || (xml[cursor] != '"' && xml[cursor] != '\''))
            return false;
        const char quote = xml[cursor++];
        const std::size_t end = escapedDelimiter
            ? xml.find(std::string("\\") + quote, cursor)
            : xml.find(quote, cursor);
        if (end == std::string::npos)
            return false;
        value.assign(xml, cursor, end - cursor);
        return true;
    }
    return false;
}

inline bool isAutoCad2007Version(const std::string& version) noexcept {
    std::size_t begin = 0u;
    std::size_t component = 0u;
    while (begin < version.size()) {
        if (component >= 4u)
            return false;
        const std::size_t end = version.find('.', begin);
        const std::size_t limit = end == std::string::npos
            ? version.size() : end;
        if (limit == begin)
            return false;
        for (std::size_t i = begin; i < limit; ++i) {
            if (version[i] < '0' || version[i] > '9')
                return false;
        }
        if (component == 0u && version.compare(begin, limit - begin, "17") != 0)
            return false;
        if (component == 1u && version.compare(begin, limit - begin, "0") != 0)
            return false;
        ++component;
        if (end == std::string::npos)
            break;
        begin = end + 1u;
    }
    return component == 4u;
}

} // namespace detail

/// Classify only the explicitly identified AutoCAD 2007 AppInfo profile.
/// Unknown, malformed, contradictory, or later product versions remain
/// unqualified. The caller must separately bound and validate the DWG section.
inline R21ProducerProfile classifyR21Producer(
    const std::uint8_t *data, std::size_t size) noexcept {
    constexpr std::size_t maxAppInfoBytes = 64u * 1024u;
    if (data == nullptr || size == 0u || size > maxAppInfoBytes)
        return R21ProducerProfile::Unknown;

    try {
        detail::Cursor cursor(data, size);
        std::uint32_t field = 0u;
        if (!cursor.readUInt32(field) || field != 2u)
            return R21ProducerProfile::Unknown;

        std::string name;
        if (!cursor.readUtf16(&name) || name != "AppInfoDataList"
            || !cursor.readUInt32(field) || field != 3u
            || !cursor.skip(16u))
            return R21ProducerProfile::Unknown;

        std::string version;
        if (!cursor.readUtf16(&version)
            || !detail::isAutoCad2007Version(version)
            || !cursor.skip(16u) || !cursor.readUtf16(nullptr)
            || !cursor.skip(16u))
            return R21ProducerProfile::Unknown;

        std::string product;
        if (!cursor.readUtf16(&product)
            || product.find("<ProductInformation") == std::string::npos)
            return R21ProducerProfile::Unknown;

        std::string productName;
        std::string registryVersion;
        if (!detail::readAttribute(product, "name", productName)
            || !detail::readAttribute(product, "registry_version",
                                      registryVersion)
            || productName != "AutoCAD" || registryVersion != "17.0")
            return R21ProducerProfile::Unknown;

        return R21ProducerProfile::AutoCAD2007;
    } catch (...) {
        return R21ProducerProfile::Unknown;
    }
}

} // namespace dwgAppInfo

#endif
