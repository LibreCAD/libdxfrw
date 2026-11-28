// SPDX-License-Identifier: GPL-2.0-or-later
// Explicit selected LIGHT preview transport; not whole-document qualification.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <stdexcept>
#include <vector>

struct DerivedLight final : DRW_Light {};
struct LightReader final : LC3CompatCheck {
    std::vector<DRW_Light> lights;
    void addLight(const DRW_Light& v) override { lights.push_back(v); }
};
struct LightWriter final : LC3CompatCheck {
    dxfRW& out;
    DRW_Light* light;
    bool accepted = false;
    LightWriter(dxfRW& o, DRW_Light* l) : out(o), light(l) {}
    void writeEntities() override { accepted = out.writeLight(light); }
    bool finalizeDxfWrite() override { return accepted; }
};
static bool read(const std::filesystem::path& p, DRW_Light& l) {
    dxfRW input(p.string().c_str()); LightReader r;
    if (!input.read(&r, false) || r.lights.size() != 1) return false;
    l = r.lights[0]; return true;
}
static void sentinel(const std::filesystem::path& p) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc); out << "SENTINEL";
}
static bool unchanged(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {}) == "SENTINEL";
}
int main() {
    const auto dir = std::filesystem::temp_directory_path() / ("light-preview-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    struct Cleanup { std::filesystem::path p; ~Cleanup() { std::error_code e; std::filesystem::remove_all(p,e); } } cleanup{dir};
    const auto source = dir / "source.dxf";
    { std::ofstream f(source); f <<
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n0\nLIGHT\n5\n2F\n100\nAcDbEntity\n8\n0\n160\n8\n310\n0800000000000000\n"
      "100\nAcDbLight\n90\n0\n1\nLOCAL_LIGHT\n70\n2\n290\n1\n63\n148\n421\n1193046\n40\n2.5\n"
      "10\n1.0\n20\n2.0\n30\n3.0\n11\n1.0\n21\n1.0\n31\n1.0\n72\n2\n292\n0\n41\n1.0\n42\n10.0\n50\n45.0\n51\n50.0\n293\n1\n73\n0\n91\n256\n280\n1\n0\nENDSEC\n0\nEOF\n"; }
    DRW_Light original;
    if (!read(source, original) || original.proxyGraphics != std::string("\x08\0\0\0\0\0\0\0", 8)
        || !original.authorizeProxyGraphicsReplay()) return 1;
    for (bool binary : {false,true}) {
        const auto path = dir / (binary ? "binary.dxf" : "ascii.dxf");
        auto light = original; dxfRW writer(path.string().c_str()); LightWriter adapter(writer,&light);
        const auto retry = [&]() {
            adapter.accepted = false;
            if (!writer.write(&adapter, DRW::AC1027, binary) || !adapter.accepted) return false;
            DRW_Light decoded;
            return read(path, decoded) && decoded.proxyGraphics == original.proxyGraphics
                && decoded.numProxyGraph == 8 && decoded.m_colorRgb == original.m_colorRgb;
        };
        const auto reject = [&](DRW::Version v) {
            sentinel(path); adapter.accepted = false;
            return !writer.write(&adapter,v,binary) && !adapter.accepted && unchanged(path);
        };
        if (!retry()) return 2;
        light.proxyGraphics[7] = 1;
        if (!reject(DRW::AC1027)) return 3;
        light = original; light.numProxyGraph = 7;
        if (!reject(DRW::AC1027)) return 4;
        light = original;
        if (!reject(DRW::AC1024)) return 5;
        light.eType = DRW::LINE;
        if (!reject(DRW::AC1027)) return 6;
        light = original;
        DerivedLight derived; static_cast<DRW_Light&>(derived) = original;
        adapter.light = &derived;
        if (!reject(DRW::AC1027)) return 10;
        adapter.light = &light;
        DRW_Light caller;
        caller.proxyGraphics = original.proxyGraphics; caller.numProxyGraph = 8;
        if (!caller.authorizeProxyGraphicsReplay(DRW::AC1027, "ANSI_1252")) return 11;
        adapter.light = &caller;
        if (!reject(DRW::AC1027)) return 12;
        adapter.light = &light;
        if (!retry()) return 7;
        if (binary) {
            DRW_Light binarySource;
            if (!read(path, binarySource) || !binarySource.authorizeProxyGraphicsReplay()) return 13;
            adapter.light = &binarySource;
            if (!retry()) return 14;
            adapter.light = &light;
        }
        // The same writer cannot write another record from a reference
        // resolver, even if a different LIGHT has a fresh source identity.
        for (int nestedKind : {0, 1, 2}) {
            light = original; light.reactorHandles = {123u};
            DRW_Light nested = original; nested.handle = 0xE001u;
            DRW_Line nestedLine; nestedLine.handle = 0xE002u;
            bool called = false, refused = false;
            writer.setReferenceResolver([&](std::uint32_t h) {
                if (h == 123u && !called) {
                    called = true;
                    if (nestedKind == 0) refused = !writer.writeLight(&light);
                    else if (nestedKind == 1) refused = !writer.writeLight(&nested);
                    else refused = !writer.writeLine(&nestedLine);
                }
                return h;
            });
            sentinel(path); adapter.accepted = false;
            if (writer.write(&adapter, DRW::AC1027, binary) || !called || !refused || !unchanged(path)) return 8;
            writer.setReferenceResolver([](std::uint32_t h) { return h; });
            light = original;
            if (!retry()) return 9;
        }
        // A nested write on a different writer remains valid; its scope is
        // restored before the original LIGHT resumes.
        light = original; light.reactorHandles = {123u};
        DRW_Light separate = original; separate.handle = 0xE003u;
        const auto otherPath = dir / "other.dxf";
        dxfRW other(otherPath.string().c_str()); LightWriter otherAdapter(other, &separate);
        bool calledOther = false, otherOk = false;
        writer.setReferenceResolver([&](std::uint32_t h) {
            if (h == 123u && !calledOther) {
                calledOther = true;
                otherOk = other.write(&otherAdapter, DRW::AC1027, binary) && otherAdapter.accepted;
            }
            return h;
        });
        if (!retry() || !calledOther || !otherOk) return 17;
        // A -> B -> A must still find A's active resolver in the scope stack.
        light = original; light.reactorHandles = {123u};
        separate = original; separate.handle = 0xE003u; separate.reactorHandles = {321u};
        bool bounced = false, bounceRefused = false, bounceOtherOk = false;
        DRW_Line bouncedLine; bouncedLine.handle = 0xE004u;
        other.setReferenceResolver([&](std::uint32_t h) {
            if (h == 321u && !bounced) { bounced = true; bounceRefused = !writer.writeLine(&bouncedLine); }
            return h;
        });
        writer.setReferenceResolver([&](std::uint32_t h) {
            if (h == 123u && !bounced) {
                otherAdapter.accepted = false;
                bounceOtherOk = other.write(&otherAdapter, DRW::AC1027, binary) && otherAdapter.accepted;
            }
            return h;
        });
        sentinel(path); sentinel(otherPath); adapter.accepted = false;
        if (writer.write(&adapter, DRW::AC1027, binary) || !bounced || !bounceRefused
            || !bounceOtherOk || !unchanged(path)) return 18;
        writer.setReferenceResolver([](std::uint32_t h) { return h; });
        light = original;
        if (!retry()) return 19;
        // Exception unwinding must restore the resolver guard on this writer.
        light = original; light.reactorHandles = {123u};
        writer.setReferenceResolver([](std::uint32_t h) -> std::uint32_t {
            if (h == 123u) throw std::runtime_error("injected resolver failure");
            return h;
        });
        sentinel(path); adapter.accepted = false;
        if (writer.write(&adapter, DRW::AC1027, binary) || !unchanged(path)) return 20;
        writer.setReferenceResolver([](std::uint32_t h) { return h; });
        light = original;
        if (!retry()) return 21;
        // Authorization and byte changes from a resolver happen after the
        // initial preflight; neither may introduce an unchecked cache.
        for (int mutation : {0, 1, 2, 3}) {
            light = original; light.reactorHandles = {123u};
            if (mutation == 0) light.revokeProxyGraphicsReplayAuthorization();
            bool changed = false;
            writer.setReferenceResolver([&](std::uint32_t h) {
                if (h == 123u && !changed) {
                    changed = true;
                    if (mutation == 0) {
                        if (!light.authorizeProxyGraphicsReplay()) return 0u;
                    } else if (mutation == 1) light.proxyGraphics[7] = 1;
                    else if (mutation == 2) light.revokeProxyGraphicsReplayAuthorization();
                    else { light.proxyGraphics.clear(); light.numProxyGraph = 0; }
                }
                return h;
            });
            sentinel(path); adapter.accepted = false;
            if (writer.write(&adapter, DRW::AC1027, binary) || !changed || !unchanged(path)) return 15;
            writer.setReferenceResolver([](std::uint32_t h) { return h; });
            light = original;
            if (!retry()) return 16;
        }
    }
    return 0;
}
