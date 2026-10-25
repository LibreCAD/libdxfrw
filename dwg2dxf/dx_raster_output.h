#ifndef DX_RASTER_OUTPUT_H
#define DX_RASTER_OUTPUT_H

// Independent DXF token observer. No dxfReader, parseCode, writer, codec or
// production classifier. Borrowed spans expire with the staged section view.
// Text comparison qualifies literal ASCII (without escape-looking slashes)
// and modern UTF-8 bytes only; it is not an independent text decoder.
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <locale>
#include <sstream>
#include <string_view>

namespace dx_raster_output {
enum class Kind { Text, Double, Integer, Bytes, Invalid };
inline Kind kind(int c) {
    // The standalone/LibreCAD compatible 100..109 string transport includes
    // ordinary XRECORD application strings, not only the named markers.
    // This independent width table must not call the production classifier.
    if ((c >= 0 && c <= 9) || (c >= 100 && c <= 109)
        || (c >= 300 && c <= 309) || (c >= 320 && c <= 369)
        || (c >= 390 && c <= 399) || (c >= 410 && c <= 419)
        || (c >= 430 && c <= 439) || (c >= 470 && c <= 481)
        || c == 999 || (c >= 1000 && c <= 1003) || c == 1005) return Kind::Text;
    if ((c >= 10 && c <= 59) || (c >= 110 && c <= 149)
        || (c >= 210 && c <= 239) || (c >= 460 && c <= 469)
        || (c >= 1010 && c <= 1059)) return Kind::Double;
    if ((c >= 310 && c <= 319) || c == 1004) return Kind::Bytes;
    if ((c >= 60 && c <= 79) || (c >= 90 && c <= 99) || (c >= 160 && c <= 179)
        || (c >= 270 && c <= 299) || (c >= 370 && c <= 389)
        || (c >= 400 && c <= 409) || (c >= 420 && c <= 429)
        || (c >= 440 && c <= 459) || (c >= 1060 && c <= 1071)) return Kind::Integer;
    return Kind::Invalid;
}
inline std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
    return s;
}
template<class I> bool integer(std::string_view s, I& value, int base = 10) {
    s = trim(s);
    const auto p = std::from_chars(s.data(), s.data() + s.size(), value, base);
    return !s.empty() && p.ec == std::errc{} && p.ptr == s.data() + s.size();
}
struct Token {
    int code{-1};
    Kind type{Kind::Invalid};
    std::string_view text;
    double real{0};
    std::int64_t number{0};
};
class Cursor {
public:
    Cursor(std::string_view bytes, bool binary) : bytes(bytes), binary(binary) {}
    std::size_t position() const { return at; }
    bool done() const { return at == bytes.size(); }
    bool next(Token& t) {
        t = {};
        if (++tokens > 4000000u) return false;
        std::uint64_t raw = 0;
        if (binary) {
            if (!word(2, raw)) return false;
            t.code = static_cast<int>(raw);
        } else {
            std::string_view line;
            if (!readLine(line) || !integer(line, t.code)) return false;
        }
        t.type = kind(t.code);
        // ASCII can carry opaque application codes whose binary widths are
        // deliberately unqualified. They remain visible to exact matching.
        if (!binary && t.type == Kind::Invalid && t.code >= 0 && t.code <= 1071) t.type = Kind::Text;
        if (t.type == Kind::Invalid) return false;
        if (!binary) {
            if (!readLine(t.text)) return false;
            if (t.type == Kind::Integer) return integer(t.text, t.number);
            if (t.type != Kind::Double) return true;
            if (t.text.size() > 128u) return false;
            std::istringstream input{std::string(t.text)};
            input.imbue(std::locale::classic());
            input >> t.real;
            return !input.fail() && input.peek() == std::char_traits<char>::eof() && std::isfinite(t.real);
        }
        if (t.type == Kind::Text) {
            const auto end = bytes.find('\0', at);
            if (end == std::string_view::npos) return false;
            t.text = bytes.substr(at, end - at); at = end + 1u; return true;
        }
        if (t.type == Kind::Bytes) {
            if (!word(1, raw) || raw > bytes.size() - at) return false;
            t.text = bytes.substr(at, static_cast<std::size_t>(raw)); at += raw; return true;
        }
        const unsigned width = t.type == Kind::Double || (t.code >= 160 && t.code <= 169) ? 8u
            : (t.code >= 290 && t.code <= 299) ? 1u
            : (t.code >= 90 && t.code <= 99) || (t.code >= 420 && t.code <= 459) || t.code == 1071 ? 4u : 2u;
        if (!word(width, raw)) return false;
        if (t.type == Kind::Double) { std::memcpy(&t.real, &raw, 8); return std::isfinite(t.real); }
        if (width == 8u) std::memcpy(&t.number, &raw, 8);
        else if (width == 4u) t.number = static_cast<std::int32_t>(raw);
        else if (width == 2u) t.number = static_cast<std::int16_t>(raw);
        else t.number = static_cast<std::int64_t>(raw);
        return true;
    }
private:
    bool readLine(std::string_view& line) {
        const auto end = bytes.find('\n', at);
        if (end == std::string_view::npos) return false;
        line = bytes.substr(at, end - at); at = end + 1;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        return true;
    }
    bool word(unsigned width, std::uint64_t& v) {
        if (width > bytes.size() - at) return false;
        v = 0;
        for (unsigned i = 0; i < width; ++i) v |= std::uint64_t(static_cast<unsigned char>(bytes[at++])) << (i * 8);
        return true;
    }
    std::string_view bytes;
    bool binary;
    std::size_t at{0}, tokens{0};
};

class Match {
public:
    Match(std::string_view body, bool binary) : cursor(body, binary), binary(binary) {}
    bool text(int code, std::string_view value) {
        Token t; return cursor.next(t) && t.code == code && t.type == Kind::Text && t.text == value;
    }
    bool number(int code, std::int64_t value) {
        Token t; return cursor.next(t) && t.code == code && t.type == Kind::Integer && t.number == value;
    }
    bool real(int code, double value) {
        Token t; return cursor.next(t) && t.code == code && t.type == Kind::Double
            && std::memcmp(&t.real, &value, sizeof(value)) == 0;
    }
    bool handle(int code, std::uint32_t value) {
        Token t; std::uint32_t actual = 0;
        return cursor.next(t) && t.code == code && t.type == Kind::Text
            && integer(t.text, actual, 16) && actual == value;
    }
    bool bytes(int code, std::string_view value) {
        Token t;
        if (!cursor.next(t) || t.code != code || t.type != Kind::Bytes) return false;
        if (binary) return t.text == value;
        if (t.text.size() != value.size() * 2u) return false;
        for (std::size_t i = 0; i < value.size(); ++i) {
            unsigned byte = 0;
            if (!integer(t.text.substr(2u * i, 2u), byte, 16)
                || byte != static_cast<unsigned char>(value[i])) return false;
        }
        return true;
    }
    bool done() const { return cursor.done(); }
private:
    Cursor cursor;
    bool binary;
};

// Scan all records without keeping a section-sized copy/model. Body includes
// the self/owner fields but excludes the opening group0. Visitors choose roots.
template<class Visitor> bool inspect(std::string_view bytes, bool binary, Visitor&& visit) {
    if (bytes.size() > 256u * 1024u * 1024u) return false;
    Cursor input(bytes, binary);
    Token t;
    std::string_view name;
    std::size_t begin = 0, records = 0;
    std::uint32_t handle = 0;
    unsigned self = 0, depth = 0;
    bool body = false, xrecordXdata = false;
    const auto complete = [&]() {
        if (name == "XRECORD" && depth != 0u) return false;
        return (name != "IMAGEDEF" && name != "IMAGEDEF_REACTOR" && name != "IMAGE")
            || (self == 1u && handle != 0u);
    };
    while (!input.done()) {
        const auto start = input.position();
        if (!input.next(t)) return false;
        if (t.code == 0) {
            if (!name.empty() && (!complete() || !visit(name, handle, bytes.substr(begin, start - begin)))) return false;
            if (t.text.empty() || ++records > 1000000u) return false;
            name = t.text; begin = input.position(); handle = 0; self = 0; depth = 0; body = false; xrecordXdata = false;
        } else if (name.empty()) return false;
        else if (name == "XRECORD" && body && t.code == 1001) xrecordXdata = true;
        else if (t.code == 102) {
            // After the outer XRECORD subclass, ordinary102 values belong to
            // application data, not common groups. Trailing XDATA ends that
            // application channel; it cannot be reopened by another102.
            if (name == "XRECORD" && body) {
                if (xrecordXdata) return false;
                continue;
            }
            if (!t.text.empty() && t.text.front() == '{') ++depth;
            else if (t.text == "}" && depth != 0u) --depth;
            else return false;
        } else if (depth == 0u && t.code == 100) body = true;
        else if (depth == 0u && !body && t.code == 5) {
            if (++self != 1u || !integer(t.text, handle, 16) || handle == 0u) return false;
        }
    }
    return (name.empty() && bytes.empty()) || (!name.empty() && complete() && visit(name, handle, bytes.substr(begin)));
}
} // namespace dx_raster_output
#endif
