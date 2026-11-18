// SPDX-License-Identifier: GPL-2.0-or-later
// Test-only selected-edge staging: not a complete DWG export or NOD proof.
bool physicalAc1024LayerStatesRealWriter(const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface reader;
    dx_data source;
    if (!reader.fileImport(sourcePath.string(), &source, false)
        || !Access::consistentLayerStateSourceGraph(reader, source))
        return fail("LayerStates real writer source graph is not qualified");
    const auto app = std::find_if(source.appIds.begin(), source.appIds.end(),
        [](const DRW_AppId& row) { return row.handle == 0x12u; });
    const auto companion = std::find_if(source.dictionaries.begin(), source.dictionaries.end(),
        [](const DRW_Dictionary& row) { return row.handle == 0xEDu; });
    const auto dictionary = std::find_if(source.dictionaries.begin(), source.dictionaries.end(),
        [](const DRW_Dictionary& row) { return row.handle == 0x24Au; });
    const auto record = std::find_if(source.xRecords.begin(), source.xRecords.end(),
        [](const DRW_XRecord& row) { return row.handle == 0x24Bu; });
    const auto scale = std::find_if(source.scales.begin(), source.scales.end(),
        [](const DRW_Scale& row) { return row.handle == 0x127u; });
    if (app == source.appIds.end() || companion == source.dictionaries.end()
        || dictionary == source.dictionaries.end() || record == source.xRecords.end()
        || scale == source.scales.end() || record->m_handleValues.size() != 1u)
        return fail("LayerStates real writer selected source is absent");
    class SelectedWriter final : public dx_iface {
    public:
        dx_data blank;
        DRW_AppId app;
        DRW_Dictionary companion, dictionary;
        DRW_XRecord record;
        DRW_Scale scale;
        bool duplicateRecord {false};
        void writeHeader(DRW_Header& header) override { header.vars.clear(); }
        bool wantsDxfLayerTableControl() const override { return false; }
        void writeVports() override {}
        void writeLTypes() override {}
        void writeLayers() override { DRW_Layer layer; layer.name = "0"; layer.handle = 0x10u; (void)dxfW->writeLayer(&layer); }
        void writeTextstyles() override {}
        void writeDimstyles() override {}
        void writeBlockRecords() override {}
        void writeBlocks() override {}
        void writeEntities() override {}
        void writeAppId() override { (void)dxfW->writeAppId(&app); }
        void writeObjects() override {
            (void)dxfW->writeDictionary(&companion);
            (void)dxfW->writeDictionary(&dictionary);
            (void)dxfW->writeXRecord(&record);
            if (duplicateRecord) (void)dxfW->writeXRecord(&record);
            (void)dxfW->writeScale(&scale);
        }
        bool finalizeDxfWrite() override { return true; }
    } selected;
    selected.cData = &selected.blank;
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-p4-selected-ledger-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    } } cleanup{path};
    dxfRW codec(path.string().c_str());
    selected.dxfW = &codec;
    codec.setDxfWriteRecordInventoryEnabled(true);
    codec.setHandleRemap({{0xEDu, 0xA0EDu}, {0xD5u, 0xA0D5u},
        {0x24Au, 0xA24Au}, {0x24Bu, 0xA24Bu}, {0x127u, 0xA127u},
        {0x249u, 0xA249u}, {0x126u, 0xA126u}});
    bool tablesSeen = false, objectsSeen = false;
    const auto map = [&codec](std::uint32_t input, std::uint32_t& output) {
        output = input == 0u ? 0u : codec.remapHandle(input);
        return input == 0u || (output != 0u && output != DRW::NoHandle);
    };
    if (!codec.setDxfTablesWriteInspector([&](const auto& view) {
        tablesSeen = true;
        if (!Access::consistentLayerStateSourceGraph(reader, source)) return false;
        const auto emitted = codec.dxfWriteEmittedSourceTableHandle(app->handle);
        unsigned matches = 0u;
        return emitted != 0u && codec.dxfWriteRecordHandleCount(emitted) == 1u
            && codec.dxfWriteRecordHasIdentity(emitted, "APPID", "TABLES")
            && codec.dxfWriteRecordHasOwner(emitted, 9u)
            && dx_symbol_output::inspect(view, [&](const dx_symbol_output::NameRecord& row) {
                if (row.type == dx_symbol_output::Kind::AppId && row.name == "ACAD"
                    && row.handle == emitted && row.owner == 9u && row.qualified) ++matches;
                return true;
            }) && matches == 1u;
    }) || !codec.setDxfObjectsWriteInspector([&](const auto& view) {
        objectsSeen = true;
        if (!tablesSeen || !Access::consistentLayerStateSourceGraph(reader, source)) return false;
        for (const auto& item : {std::make_pair(0xA0EDu, "DICTIONARY"),
                std::make_pair(0xA24Au, "DICTIONARY"),
                std::make_pair(0xA24Bu, "XRECORD"), std::make_pair(0xA127u, "SCALE")})
            if (codec.dxfWriteRecordHandleCount(item.first) != 1u
                || !codec.dxfWriteRecordHasIdentity(item.first, item.second, "OBJECTS")) return false;
        if (!codec.dxfWriteRecordHasOwner(0xA0EDu, 0xA0D5u)
            || !codec.dxfWriteRecordHasOwner(0xA24Au, 0xA249u)
            || !codec.dxfWriteRecordHasOwner(0xA24Bu, 0xA24Au)
            || !codec.dxfWriteRecordHasOwner(0xA127u, 0xA126u)) return false;
        unsigned seenCompanion = 0u, seenDictionary = 0u, seenRecord = 0u, seenScale = 0u;
        return dx_standard_output::inspectSelected(view,
            [](std::string_view, std::uint32_t h) {
                return h == 0xA0EDu || h == 0xA24Au || h == 0xA24Bu || h == 0xA127u;
            }, [&](const dx_standard_output::Shape& shape, std::string_view body) {
                dx_standard_output::Decoder decoder(view);
                if (shape.handle == 0xA0EDu || shape.handle == 0xA24Au) {
                    const auto& expected = shape.handle == 0xA0EDu ? *companion : *dictionary;
                    auto& seen = shape.handle == 0xA0EDu ? seenCompanion : seenDictionary;
                    if (++seen != 1u || shape.kind != dx_standard_output::Family::Dictionary) return false;
                    return decoder.decodeRecord<DRW_Dictionary>(body, [&](const DRW_Dictionary& actual) {
                        std::vector<std::uint32_t> refs;
                        return dx_standard_output::equal(expected, actual, map, refs)
                            && (shape.handle != 0xA24Au
                                || std::count(refs.begin(), refs.end(), 0xA24Bu) == 1);
                    });
                }
                if (shape.handle == 0xA24Bu) {
                    if (++seenRecord != 1u || shape.kind != dx_standard_output::Family::Xrecord) return false;
                    return decoder.decodeRecord<DRW_XRecord>(body, [&](const DRW_XRecord& actual) {
                        std::vector<std::uint32_t> refs;
                        return dx_standard_output::equal(*record, actual, map, refs)
                            && std::count(refs.begin(), refs.end(), 0xA127u) == 1;
                    });
                }
                if (++seenScale != 1u || shape.kind != dx_standard_output::Family::Other) return false;
                return decoder.decodeRecord<DRW_Scale>(body, [&](const DRW_Scale& actual) {
                    return actual.handle == 0xA127u && actual.parentHandle == 0xA126u
                        && actual.name == scale->name && actual.flag == scale->flag
                        && actual.paperUnits == scale->paperUnits
                        && actual.drawingUnits == scale->drawingUnits
                        && actual.isUnitScale == scale->isUnitScale;
                });
            }) && seenCompanion == 1u && seenDictionary == 1u && seenRecord == 1u && seenScale == 1u;
    })) return fail("LayerStates real writer inspector setup failed");
    const auto restore = [&] { selected.app = *app; selected.companion = *companion;
        selected.dictionary = *dictionary; selected.record = *record; selected.scale = *scale;
        selected.duplicateRecord = false; };
    const std::string sentinel = "selected LayerStates ledger sentinel\n";
    for (const bool binary : {false, true}) {
        for (unsigned fault = 0u; fault != 7u; ++fault) {
            restore(); tablesSeen = false; objectsSeen = false;
            if (fault == 0u) selected.app.name = "OTHER";
            if (fault == 1u) selected.companion.cloning = 2;
            if (fault == 2u) selected.record.parentHandle = 0x249u;
            if (fault == 3u) selected.scale.paperUnits = 2.0;
            if (fault == 4u) {
                auto value = std::find_if(selected.record.m_dataEntries.begin(),
                    selected.record.m_dataEntries.end(),
                    [](const DRW_Variant& entry) { return entry.code() == 330; });
                if (value == selected.record.m_dataEntries.end())
                    return fail("selected LayerStates XRECORD target is absent");
                *value = DRW_Variant(330, std::string("128"));
            }
            if (fault == 5u) {
                auto value = std::find_if(selected.companion.extData.begin(),
                    selected.companion.extData.end(), [](const auto& entry) {
                        return entry && entry->code() == 1000;
                    });
                if (value == selected.companion.extData.end())
                    return fail("selected LayerStates EED string is absent");
                **value = DRW_Variant(1000, std::string("wrong"));
            }
            if (fault == 6u) selected.duplicateRecord = true;
            { std::ofstream existing(path, std::ios::binary | std::ios::trunc);
              if (!existing || !(existing << sentinel)) return fail("cannot seed selected ledger sentinel"); }
            if (codec.write(&selected, DRW::AC1024, binary)) return fail("corrupt selected ledger output succeeded");
            {
                std::ifstream existing(path, std::ios::binary);
                if (std::string{std::istreambuf_iterator<char>(existing), {}} != sentinel
                    || !tablesSeen || (fault != 0u && fault != 6u && !objectsSeen))
                    return fail("selected ledger refusal did not preserve destination or reach its inspector");
            }
            restore(); tablesSeen = false; objectsSeen = false;
            if (!codec.write(&selected, DRW::AC1024, binary) || !tablesSeen || !objectsSeen)
                return fail("selected ledger restored same-writer retry failed");
        }
    }
    if (!Access::consistentLayerStateSourceGraph(reader, source))
        return fail("selected ledger test changed the original source graph");
    std::cerr << "AC1024 selected LayerStates real writer ledger controls passed (ASCII/Binary).\n";
    return true;
}
