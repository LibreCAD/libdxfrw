// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

void drwThrowNextAc1021PolylineForTest();
void drwThrowNextAc1021PolylineLateForTest();

class DwgCompoundWriteTestAccess {
public:
    static void failNextSeqEnd(dwgRW& writer) { writer.m_failNextCompoundSeqEndForTest = true; }
};

namespace {
enum class Mode { Vertex, Parent, Missing, Wrong, Duplicate, Additional, LateAdd, LateRemove,
                  SeqEnd, Paper, Named, FailedCompound, RolledBackCompound, ChildCollision,
                  GroupMissingReactor, GroupWrongReactor, GroupXdict, GroupDescription,
                  GroupExtraMember, GroupUnnamed, GroupNotSelectable, GroupFlags,
                  OuterRollback, EarlyException, LateException };
class Author final : public LC3CompatCheck, public DRW_DwgDefaultMaterialProvider,
                     public DRW_DwgDefaultPlotStyleProvider {
public:
    explicit Author(dwgRW& output) : out(output) {}
    bool wantsDwgDefaultMaterialGraph() const override { return true; }
    bool wantsDwgDefaultPlotStyleGraph() const override { return true; }
    bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a, std::uint32_t b,
            std::uint32_t c, std::uint32_t d) const override { return a && b && c && d; }
    void writeDwgClasses() override {
        registered = out.registerDwgGroupDictionaryEntry("GROUP_VERTEX", 0x900u);
    }
    void writeBlocks() override {
        if (mode != Mode::Named) return;
        const auto block = out.defineBlock("GROUP_NAMED", DRW_Coord(0., 0., 0.));
        if (!block || !out.beginBlockContent(block)) return;
        emitPolyline();
        if (!out.endBlockContent()) wrotePolyline = false;
    }
    void writeEntities() override { if (mode != Mode::Named) emitPolyline(); }
    void emitPolyline() {
        DRW_Polyline poly;
        poly.handle = 0x451u;
        if (threeD) poly.flags = 8;
        if (mode == Mode::Paper) poly.space = DRW::PaperSpace;
        poly.addVertex(DRW_Vertex(1., 2., 0., 0.));
        poly.addVertex(DRW_Vertex(3., 4., threeD ? 1. : 0., 0.));
        if (threeD) for (auto& vertex : poly.vertlist) vertex->flags = 32;
        poly.vertlist[0]->handle = mode == Mode::ChildCollision ? 0x900u : 0x501u;
        poly.vertlist[1]->handle = 0x502u;
        if (mode != Mode::Missing && mode != Mode::LateAdd)
            poly.vertlist[0]->reactorHandles = {mode == Mode::Wrong ? 0x901u : 0x900u};
        if (mode == Mode::Duplicate) poly.vertlist[0]->reactorHandles.push_back(0x900u);
        if (mode == Mode::Additional) poly.vertlist[0]->reactorHandles.push_back(0x901u);
        if (mode == Mode::FailedCompound) DwgCompoundWriteTestAccess::failNextSeqEnd(out);
        const auto transaction = mode == Mode::OuterRollback
            ? out.beginDwgWriteTransaction(false, true, false) : 0u;
        if (mode == Mode::EarlyException) drwThrowNextAc1021PolylineForTest();
        if (mode == Mode::LateException) drwThrowNextAc1021PolylineLateForTest();
        try { wrotePolyline = out.writePolyline(&poly); }
        catch (const std::bad_alloc&) { exceptionSeen = true; wrotePolyline = false; }
        if (mode == Mode::LateAdd) poly.vertlist[0]->reactorHandles = {0x900u};
        if (mode == Mode::LateRemove) poly.vertlist[0]->reactorHandles.clear();
        if (mode == Mode::RolledBackCompound && wrotePolyline)
            rollbackOk = out.rollbackLastDwgObjectWrite();
        if (mode == Mode::OuterRollback && transaction && wrotePolyline)
            rollbackOk = out.rollbackDwgWriteTransaction(transaction);
        seqend = poly.dwgSeqEndHandle();
    }
    void writeObjects() override {
        DRW_Group group;
        group.handle = 0x900u;
        group.parentHandle = DRW::DwgAcadGroupDictionaryHandle;
        group.reactorHandles = {DRW::DwgAcadGroupDictionaryHandle};
        group.setDwgCommonObjectState(1, 1, false);
        if (mode == Mode::GroupMissingReactor) group.reactorHandles.clear();
        if (mode == Mode::GroupWrongReactor) group.reactorHandles = {0x901u};
        if (mode == Mode::GroupXdict) group.setDwgCommonObjectState(1, 0, false);
        if (mode == Mode::GroupDescription) group.m_description = "not the witnessed profile";
        if (mode == Mode::GroupUnnamed) group.m_isUnnamed = true;
        if (mode == Mode::GroupNotSelectable) group.m_selectable = false;
        if (mode == Mode::GroupFlags) group.flags = 1;
        group.name = "GROUP_VERTEX";
        group.m_entityHandles = {mode == Mode::Parent ? 0x451u
            : mode == Mode::SeqEnd ? seqend : mode == Mode::ChildCollision ? 0x900u : 0x501u};
        if (mode == Mode::GroupExtraMember) group.m_entityHandles.push_back(0x451u);
        wroteGroup = out.writeGroup(&group); // deliberately ignored by finalizeDwgWrite
    }
    bool finalizeDwgWrite() override { return true; }
    dwgRW& out;
    Mode mode {Mode::Vertex};
    bool threeD {false}, registered {false}, wrotePolyline {false}, wroteGroup {false}, rollbackOk {false}, exceptionSeen {false};
    std::uint32_t seqend {0};
};
class Reader final : public LC3CompatCheck {
public:
    void addGroup(const DRW_Group& group) override {
        if (group.handle == 0x900u && group.m_entityHandles == std::vector<std::uint32_t>{expected}) groupSeen = true;
    }
    void addPolyline(const DRW_Polyline& poly) override {
        if (poly.handle == 0x451u && poly.vertlist.size() == 2u
            && poly.vertlist[0]->handle == 0x501u
            && poly.vertlist[0]->parentHandle == poly.handle
            && poly.vertlist[0]->reactorHandles == std::vector<std::uint32_t>{0x900u}) vertexSeen = true;
    }
    std::uint32_t expected {0x501u};
    bool groupSeen {false}, vertexSeen {false};
};
}

int main(int argc, char** argv) {
    const auto dir = argc == 2 ? std::filesystem::path(argv[1])
        : std::filesystem::temp_directory_path() / ("ac1021-group-vertex-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    struct Cleanup { std::filesystem::path path; bool active; ~Cleanup() { if (active) {std::error_code e; std::filesystem::remove_all(path, e);} } } cleanup{dir, argc != 2};
    for (const bool threeD : {false, true}) {
        const auto path = dir / (threeD ? "vertex3d.dwg" : "vertex2d.dwg");
        dwgRW output(path.string().c_str()); Author author(output); author.threeD = threeD;
        const auto success = [&](Mode mode) {
            author.mode = mode; author.wroteGroup = author.wrotePolyline = author.rollbackOk = false;
            if (!output.write(&author, DRW::AC1021, true) || !author.registered || !author.wrotePolyline || !author.wroteGroup)
                return false;
            dwgRW input(path.string().c_str()); Reader reader;
            reader.expected = mode == Mode::Parent ? 0x451u : 0x501u;
            return input.read(&reader, false) && reader.groupSeen && reader.vertexSeen;
        };
        for (const auto mode : {Mode::Vertex, Mode::LateRemove, Mode::Parent}) {
            if (!success(mode)) { std::cerr << "positive GROUP mode=" << static_cast<int>(mode) << " 3D=" << threeD << '\n'; return 1; }
        }
        for (const auto mode : {Mode::Missing, Mode::Wrong, Mode::Duplicate, Mode::Additional, Mode::LateAdd,
                                 Mode::SeqEnd, Mode::Paper, Mode::Named, Mode::FailedCompound,
                                 Mode::RolledBackCompound, Mode::ChildCollision,
                                 Mode::GroupMissingReactor, Mode::GroupWrongReactor,
                                 Mode::GroupXdict, Mode::GroupDescription,
                                 Mode::GroupExtraMember, Mode::GroupUnnamed,
                                 Mode::GroupNotSelectable, Mode::GroupFlags,
                                 Mode::OuterRollback, Mode::EarlyException, Mode::LateException}) {
            { std::ofstream prior(path, std::ios::binary | std::ios::trunc); prior << "group destination"; }
            author.mode = mode; author.wroteGroup = author.wrotePolyline = author.rollbackOk = author.exceptionSeen = false;
            if (output.write(&author, DRW::AC1021, true) || author.wroteGroup) {
                std::cerr << "accepted GROUP mode=" << static_cast<int>(mode) << '\n'; return 1;
            }
            std::ifstream prior(path, std::ios::binary);
            if (std::string(std::istreambuf_iterator<char>(prior), {}) != "group destination") return 1;
            prior.close();
            if ((mode == Mode::RolledBackCompound || mode == Mode::OuterRollback) && !author.rollbackOk) return 1;
            if ((mode == Mode::EarlyException || mode == Mode::LateException) && !author.exceptionSeen) return 1;
            if (!success(Mode::Vertex)) {std::cerr << "GROUP retry failed mode=" << static_cast<int>(mode) << '\n'; return 1;}
        }
        if (!success(Mode::Vertex)) return 1;
    }
    return 0;
}
