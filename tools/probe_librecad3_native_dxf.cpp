// SPDX-License-Identifier: GPL-2.0-or-later
// Build against a scratch LibreCAD_3 persistence/kernel and current libdxfrw.
// LC3_NATIVE_PROBE_DIR must contain an independently authored `authored.dxf`;
// use a fresh directory because this test writes `saved.dxf` there.
#include <gtest/gtest.h>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <cad/primitive/circle.h>
#include <cad/primitive/line.h>
#include <cad/storage/documentimpl.h>
#include <cad/storage/storagemanagerimpl.h>
#include "persistence/file.h"

namespace {
using Document = lc::storage::DocumentImpl;

std::shared_ptr<Document> newDocument() {
    return std::make_shared<Document>(std::make_shared<lc::storage::StorageManagerImpl>());
}

void checkPlanarEntities(const std::shared_ptr<Document>& document) {
    const auto entities = document->entitiesByBlock(nullptr).asVector();
    ASSERT_EQ(2u, entities.size());
    unsigned lines = 0, circles = 0;
    for (const auto& entity : entities) {
        if (auto line = std::dynamic_pointer_cast<const lc::entity::Line>(entity)) {
            ++lines;
            EXPECT_DOUBLE_EQ(1, line->start().x());
            EXPECT_DOUBLE_EQ(2, line->start().y());
            EXPECT_DOUBLE_EQ(0, line->start().z());
            EXPECT_DOUBLE_EQ(4, line->end().x());
            EXPECT_DOUBLE_EQ(5, line->end().y());
            EXPECT_DOUBLE_EQ(0, line->end().z());
        }
        if (auto circle = std::dynamic_pointer_cast<const lc::entity::Circle>(entity)) {
            ++circles;
            EXPECT_DOUBLE_EQ(7, circle->center().x());
            EXPECT_DOUBLE_EQ(8, circle->center().y());
            EXPECT_DOUBLE_EQ(0, circle->center().z());
            EXPECT_DOUBLE_EQ(2.5, circle->radius());
        }
    }
    EXPECT_EQ(1u, lines);
    EXPECT_EQ(1u, circles);
}

TEST(Lc3NativeDxfrw, OpenSaveReopen2dEntities) {
    const char* path = std::getenv("LC3_NATIVE_PROBE_DIR");
    ASSERT_NE(nullptr, path);
    const std::string root = std::string(path) + "/";
    const std::string input = root + "authored.dxf", output = root + "saved.dxf";
    ASSERT_FALSE(std::ifstream(output).good()) << "use a fresh scratch directory";
    auto first = newDocument();
    EXPECT_EQ(lc::persistence::File::LIBDXFRW_DXF_R2000,
              lc::persistence::File::open(first, input, lc::persistence::File::LIBDXFRW));
    checkPlanarEntities(first);
    ASSERT_TRUE(lc::persistence::File::save(first, output, lc::persistence::File::LIBDXFRW_DXF_R2000));
    auto second = newDocument();
    EXPECT_EQ(lc::persistence::File::LIBDXFRW_DXF_R2000,
              lc::persistence::File::open(second, output, lc::persistence::File::LIBDXFRW));
    checkPlanarEntities(second);
    EXPECT_TRUE(lc::persistence::File::getAvailableLibrariesForFormat("dwg").empty());
}
}
