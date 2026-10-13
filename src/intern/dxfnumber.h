// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFNUMBER_H
#define DXFNUMBER_H

#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <locale>
#include <locale.h>
#include <sstream>
#include <string>
#if defined(__APPLE__)
#include <xlocale.h>
#endif

// Private numeric conversions for drawing syntax. No operation changes the
// process/thread locale or a caller stream's locale. Callers retain their
// existing full-token, finite and ERANGE acceptance policies.
namespace DxfNumeric {

class ClassicCLocale final {
public:
#if defined(_WIN32)
    ClassicCLocale() : m_locale(_create_locale(LC_NUMERIC, "C")) {}
    ~ClassicCLocale() { if (m_locale != nullptr) _free_locale(m_locale); }
#else
    ClassicCLocale() : m_locale(newlocale(LC_NUMERIC_MASK, "C", nullptr)) {}
    ~ClassicCLocale() { if (m_locale != nullptr) freelocale(m_locale); }
#endif
    ClassicCLocale(const ClassicCLocale&) = delete;
    ClassicCLocale& operator=(const ClassicCLocale&) = delete;

    double parse(const char* text, char** end) const {
        if (!prepare(text, end)) return 0.0;
#if defined(_WIN32)
        return _strtod_l(text, end, m_locale);
#else
        return strtod_l(text, end, m_locale);
#endif
    }
    long parseLong(const char* text, char** end) const {
        if (!prepare(text, end)) return 0;
#if defined(_WIN32)
        return _strtol_l(text, end, 10, m_locale);
#else
        return strtol_l(text, end, 10, m_locale);
#endif
    }
    long long parseLongLong(const char* text, char** end) const {
        if (!prepare(text, end)) return 0;
#if defined(_WIN32)
        return _strtoll_l(text, end, 10, m_locale);
#else
        return strtoll_l(text, end, 10, m_locale);
#endif
    }
private:
    bool prepare(const char* text, char** end) const {
        errno = 0;
        if (m_locale == nullptr) {
            *end = const_cast<char*>(text);
            errno = ENOMEM;
            return false;
        }
        return true;
    }
#if defined(_WIN32)
    _locale_t m_locale;
#else
    locale_t m_locale;
#endif
};

inline const ClassicCLocale& classicCLocale() {
    static const ClassicCLocale locale;
    return locale;
}

inline double strtodC(const char* text, char** end) {
    return classicCLocale().parse(text, end);
}

inline long strtolC(const char* text, char** end) {
    return classicCLocale().parseLong(text, end);
}

inline long long strtollC(const char* text, char** end) {
    return classicCLocale().parseLongLong(text, end);
}

inline bool isSpace(char value) {
    return value == ' ' || value == '\t' || value == '\r'
        || value == '\n' || value == '\v' || value == '\f';
}

// Match std::stod's former prefix/range behavior for legacy string-backed
// ARCALIGNEDTEXT fields, without its dependence on LC_NUMERIC.
inline bool parsePrefix(const std::string& text, double& value) {
    char* end = nullptr;
    value = strtodC(text.c_str(), &end);
    return end != text.c_str() && errno != ERANGE && errno != ENOMEM;
}

inline std::string formatDouble(double value, int precision = 16) {
    struct Formatter {
        Formatter() { stream.imbue(std::locale::classic()); }
        std::ostringstream stream;
    };
    // Reuse the classic facet setup per thread, not per coordinate. The
    // returned spelling is owned by the caller before the next conversion.
    static thread_local Formatter formatter;
    formatter.stream.str(std::string{});
    formatter.stream.clear();
    formatter.stream.precision(precision);
    formatter.stream << value;
    return formatter.stream.str();
}

inline std::string formatHex(std::uint64_t value) {
    char buffer[16];
    const auto result = std::to_chars(buffer, buffer + sizeof buffer, value, 16);
    for (char* current = buffer; current != result.ptr; ++current) {
        if (*current >= 'a' && *current <= 'f')
            *current = static_cast<char>(*current - 'a' + 'A');
    }
    return std::string(buffer, result.ptr);
}

} // namespace DxfNumeric

#endif // DXFNUMBER_H
