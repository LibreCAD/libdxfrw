// SPDX-License-Identifier: GPL-2.0-or-later
class LayerStateTestWriter : public dx_iface {
public:
    void writeHeader(DRW_Header& header) override { header.vars.clear(); }
    bool wantsDxfLayerTableControl() const override { return false; }
    void writeVports() override {}
    void writeLTypes() override {}
    void writeLayers() override {
        DRW_Layer layer;
        layer.name = "0";
        layer.handle = 0x10u;
        (void)dxfW->writeLayer(&layer);
    }
    void writeTextstyles() override {}
    void writeDimstyles() override {}
    void writeBlockRecords() override {}
    void writeBlocks() override {}
    void writeEntities() override {}
};

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
    const auto scaleList = std::find_if(source.dictionaries.begin(), source.dictionaries.end(),
        [](const DRW_Dictionary& row) { return row.handle == 0x126u; });
    const auto selectedRootCount = std::count_if(source.namedObjectDictionary.m_entries.begin(),
        source.namedObjectDictionary.m_entries.end(), [](const auto& entry) {
            return entry.m_name == "ACAD_SCALELIST" && entry.m_handle == 0x126u;
        });
    if (app == source.appIds.end() || companion == source.dictionaries.end()
        || dictionary == source.dictionaries.end() || record == source.xRecords.end()
        || scale == source.scales.end() || scaleList == source.dictionaries.end()
        || source.namedObjectDictionary.handle != 0xCu || selectedRootCount != 1
        || source.scales.size() != 32u || scaleList->m_entries.size() != 32u
        || record->m_handleValues.size() != 1u)
        return fail("LayerStates real writer selected source is absent");
    class SelectedWriter final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_AppId app;
        DRW_Dictionary companion, dictionary, scaleList;
        DRW_XRecord record;
        std::vector<DRW_Scale> scales;
        bool duplicateRecord {false};
        void writeAppId() override { (void)dxfW->writeAppId(&app); }
        void writeObjects() override {
            (void)dxfW->writeDictionary(&companion);
            (void)dxfW->writeDictionary(&dictionary);
            (void)dxfW->writeXRecord(&record);
            if (duplicateRecord) (void)dxfW->writeXRecord(&record);
            (void)dxfW->writeDictionary(&scaleList);
            for (auto& item : scales) (void)dxfW->writeScale(&item);
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
    std::map<std::uint32_t, std::uint32_t> mappings{{0xEDu, 0xA0EDu}, {0xD5u, 0xA0D5u},
        {0x24Au, 0xA24Au}, {0x24Bu, 0xA24Bu}, {0x127u, 0xA127u},
        {0x249u, 0xA249u}, {0x126u, 0xA126u}};
    for (const auto& item : source.scales) mappings[item.handle] = 0xA000u + item.handle;
    std::map<std::uint32_t, const DRW_Scale*> scalesByOutput;
    for (const auto& item : source.scales) {
        if (!scalesByOutput.emplace(mappings.at(item.handle), &item).second)
            return fail("LayerStates SCALE output identities are ambiguous");
    }
    codec.setHandleRemap(mappings);
    codec.setRootDictEntries({{"ACAD_SCALELIST", "A126"}});
    if (!codec.setGeneratedDictionarySourceHandles(source.namedObjectDictionary.handle, 0u))
        return fail("selected NOD source handle setup failed");
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
                std::make_pair(0xA24Bu, "XRECORD"), std::make_pair(0xA127u, "SCALE"),
                std::make_pair(0xA126u, "DICTIONARY"), std::make_pair(0xCu, "DICTIONARY")})
            if (codec.dxfWriteRecordHandleCount(item.first) != 1u
                || !codec.dxfWriteRecordHasIdentity(item.first, item.second, "OBJECTS")) return false;
        if (!codec.dxfWriteRecordHasOwner(0xA0EDu, 0xA0D5u)
            || !codec.dxfWriteRecordHasOwner(0xA24Au, 0xA249u)
            || !codec.dxfWriteRecordHasOwner(0xA24Bu, 0xA24Au)
            || !codec.dxfWriteRecordHasOwner(0xA127u, 0xA126u)
            || !codec.dxfWriteRecordHasOwner(0xA126u, 0xCu)) return false;
        for (const auto& item : source.scales) {
            const auto emitted = codec.remapHandle(item.handle);
            if (codec.dxfWriteRecordHandleCount(emitted) != 1u
                || !codec.dxfWriteRecordHasIdentity(emitted, "SCALE", "OBJECTS")
                || !codec.dxfWriteRecordHasOwner(emitted, 0xA126u)) return false;
        }
        unsigned seenCompanion = 0u, seenDictionary = 0u, seenRecord = 0u, seenScale = 0u;
        unsigned seenRoot = 0u, seenScaleList = 0u;
        return dx_standard_output::inspectSelected(view,
            [&](std::string_view, std::uint32_t h) {
                return h == 0xA0EDu || h == 0xA24Au || h == 0xA24Bu
                    || h == 0xCu || h == 0xA126u || scalesByOutput.count(h) != 0u;
            }, [&](const dx_standard_output::Shape& shape, std::string_view body) {
                dx_standard_output::Decoder decoder(view);
                if (shape.handle == 0xCu) {
                    if (++seenRoot != 1u) return false;
                    return decoder.decodeRecord<DRW_Dictionary>(body, [&](const DRW_Dictionary& actual) {
                        return std::count_if(actual.m_entries.begin(), actual.m_entries.end(), [](const auto& entry) {
                            return entry.m_name == "ACAD_SCALELIST" && entry.m_handle == 0xA126u
                                && entry.m_dxfHandleCode == DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
                        }) == 1;
                    });
                }
                if (shape.handle == 0xA126u) {
                    if (++seenScaleList != 1u) return false;
                    return decoder.decodeRecord<DRW_Dictionary>(body, [&](const DRW_Dictionary& actual) {
                        std::vector<std::uint32_t> refs;
                        if (!dx_standard_output::equal(*scaleList, actual, map, refs)) return false;
                        for (const auto& entry : scaleList->m_entries)
                            if (std::count(refs.begin(), refs.end(), codec.remapHandle(entry.m_handle)) != 1)
                                return false;
                        return true;
                    });
                }
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
                ++seenScale;
                if (shape.kind != dx_standard_output::Family::Other) return false;
                const auto expected = scalesByOutput.find(shape.handle);
                if (expected == scalesByOutput.end()) return false;
                return decoder.decodeRecord<DRW_Scale>(body, [&](const DRW_Scale& actual) {
                    return actual.handle == shape.handle && actual.parentHandle == 0xA126u
                        && actual.reactorHandles == std::vector<std::uint32_t>{0xA126u}
                        && actual.name == expected->second->name && actual.flag == expected->second->flag
                        && actual.paperUnits == expected->second->paperUnits
                        && actual.drawingUnits == expected->second->drawingUnits
                        && actual.isUnitScale == expected->second->isUnitScale;
                });
            }) && seenRoot == 1u && seenScaleList == 1u && seenCompanion == 1u
                && seenDictionary == 1u && seenRecord == 1u && seenScale == 32u;
    })) return fail("LayerStates real writer inspector setup failed");
    const auto restore = [&] { selected.app = *app; selected.companion = *companion;
        selected.dictionary = *dictionary; selected.record = *record;
        selected.scaleList = *scaleList;
        selected.scales.assign(source.scales.begin(), source.scales.end());
        codec.setRootDictEntries({{"ACAD_SCALELIST", "A126"}});
        selected.duplicateRecord = false; };
    const std::string sentinel = "selected LayerStates ledger sentinel\n";
    for (const bool binary : {false, true}) {
        for (unsigned fault = 0u; fault != 12u; ++fault) {
            restore(); tablesSeen = false; objectsSeen = false;
            if (fault == 0u) selected.app.name = "OTHER";
            if (fault == 1u) selected.companion.cloning = 2;
            if (fault == 2u) selected.record.parentHandle = 0x249u;
            if (fault == 3u) for (auto& item : selected.scales)
                if (item.handle == 0x127u) item.paperUnits = 2.0;
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
            if (fault == 7u) codec.setRootDictEntries({{"ACAD_SCALELIST", "A128"}});
            if (fault == 8u) selected.scaleList.m_entries.front().m_handle = 0xdeadbeefu;
            if (fault == 9u) selected.scaleList.parentHandle = 0xD5u;
            if (fault == 10u) for (auto& item : selected.scales)
                if (item.handle == 0x145u) item.drawingUnits += 1.0;
            if (fault == 11u) selected.scales.push_back(selected.scales.front());
            { std::ofstream existing(path, std::ios::binary | std::ios::trunc);
              if (!existing || !(existing << sentinel)) return fail("cannot seed selected ledger sentinel"); }
            if (codec.write(&selected, DRW::AC1024, binary)) {
                std::cerr << "LayerStates staged fault " << fault
                          << ", binary=" << binary << '\n';
                return fail("corrupt selected ledger output succeeded");
            }
            {
                std::ifstream existing(path, std::ios::binary);
                if (std::string{std::istreambuf_iterator<char>(existing), {}} != sentinel
                    || !tablesSeen || (fault != 0u && fault != 6u && fault != 11u && !objectsSeen)) {
                    std::cerr << "LayerStates staged fault " << fault
                              << ", binary=" << binary << '\n';
                    return fail("selected ledger refusal did not preserve destination or reach its inspector");
                }
            }
            restore(); tablesSeen = false; objectsSeen = false;
            if (!codec.write(&selected, DRW::AC1024, binary) || !tablesSeen || !objectsSeen) {
                std::cerr << "LayerStates staged fault " << fault
                          << ", binary=" << binary << '\n';
                return fail("selected ledger restored same-writer retry failed");
            }
        }
    }
    if (!Access::consistentLayerStateSourceGraph(reader, source))
        return fail("selected ledger test changed the original source graph");
    std::cerr << "AC1024 selected NOD/SCALELIST and 32 SCALE staged controls passed (ASCII/Binary, 12 faults each).\n";
    return true;
}

bool physicalAc1024EmptyNodWriter(const std::filesystem::path& sourcePath,
                                bool withMLineStyle = false) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface reader;
    dx_data source;
    if (!reader.fileImport(sourcePath.string(), &source, false)
        || !Access::consistentLayerStateSourceGraph(reader, source)
        || source.dictionaryWithDefaults.size() != 1u
        || source.placeholders.size() != 1u)
        return fail("could not import the AC1024 empty NOD witness");

    const auto find = [&source](std::uint32_t handle) {
        return std::find_if(source.dictionaries.begin(), source.dictionaries.end(),
            [handle](const DRW_Dictionary& row) { return row.handle == handle; });
    };
    const auto group = find(0xDu);
    const auto color = find(0x73u);
    const auto plotSettings = find(0x19u);
    const auto styleDictionary = find(0x17u);
    if (group == source.dictionaries.end() || color == source.dictionaries.end()
        || plotSettings == source.dictionaries.end()
        || (withMLineStyle && (styleDictionary == source.dictionaries.end()
            || source.mLineStyles.size() != 1u)))
        return fail("missing empty NOD source dictionaries");

    class Writer final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_Dictionary color, plotSettings;
        DRW_Dictionary styleDictionary;
        DRW_MLineStyle style;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        std::function<bool()> finalize;
        bool withMLineStyle{false};
        void writeAppId() override {}
        void writeObjects() override {
            (void)dxfW->writeDictionary(&color);
            (void)dxfW->writeDictionary(&plotSettings);
            (void)dxfW->writeDictionaryWithDefault(&plot);
            (void)dxfW->writeAcDbPlaceholder(&placeholder);
            if (withMLineStyle) {
                (void)dxfW->writeDictionary(&styleDictionary);
                (void)dxfW->writeMLineStyle(&style);
            }
        }
        bool finalizeDxfWrite() override { return finalize(); }
    } writer;
    writer.cData = &writer.blank;
    writer.color = *color;
    writer.plotSettings = *plotSettings;
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u) return fail("missing Normal plot-style entry");
    writer.plot.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();
    writer.withMLineStyle = withMLineStyle;
    if (withMLineStyle) {
        writer.styleDictionary = *styleDictionary;
        writer.style = source.mLineStyles.front();
    }

    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-empty-nod-writer-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    } } cleanup{path};
    dxfRW codec(path.string().c_str());
    dx_iface foreign;
    for (auto* adapter : {&reader, &foreign, static_cast<dx_iface*>(&writer)})
        Access::attachDxfWriter(*adapter, &codec);
    codec.setDxfWriteRecordInventoryEnabled(true);
    std::map<std::uint32_t, std::uint32_t> remap{{0x73u, 0xA073u}, {0x19u, 0xA019u}};
    std::vector<std::pair<std::string, std::string>> roots{
        {"ACAD_COLOR", "A073"}, {"ACAD_PLOTSETTINGS", "A019"}, {"ACAD_PLOTSTYLENAME", "E"}};
    if (withMLineStyle) {
        remap.emplace(0x17u, 0xA017u);
        remap.emplace(0x18u, 0xA018u);
        roots.emplace_back("ACAD_MLINESTYLE", "A017");
    }
    codec.setHandleRemap(std::move(remap));
    codec.setRootDictEntries(roots);
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not set up the full NOD snapshot");
    DRW_Class plotClass, placeholderClass;
    if (!dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("missing plot-style CLASSES rows");
    plotClass.instanceCount = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({plotClass, placeholderClass});

    enum class Fault { None, SourceTail, WrongSession, MissingRoot, MissingBaseline,
                       WrongBody, LateTail, StyleSourceTail, StyleRootTail,
                       WrongStyleBody, LateStyleTail };
    Fault fault = Fault::None;
    bool staged = false, finalizing = false;
    if (!codec.setDxfObjectsWriteInspector([&](const auto& view) {
        staged = true;
        auto& adapter = fault == Fault::WrongSession ? foreign : reader;
        return (!withMLineStyle || Access::inspectSelectedNodMLineStyle(reader, view))
            && Access::inspectEmptyNodChildren(adapter, source, view);
    })) return fail("could not set up selected NOD staging");
    auto& tail = Access::standardFrameTail(source, 0x73u);
    writer.finalize = [&]() {
        finalizing = true;
        if (fault == Fault::LateTail) tail ^= 1u;
        if (fault == Fault::LateStyleTail) Access::mlineStyleFrameTail(source) ^= 1u;
        const bool result = (!withMLineStyle || Access::finalizeSelectedNodMLineStyle(reader))
            && Access::finalizeEmptyNodChildren(reader);
        if (fault == Fault::LateStyleTail) Access::mlineStyleFrameTail(source) ^= 1u;
        if (fault == Fault::LateTail) tail ^= 1u;
        return result;
    };
    const auto run = [&](bool binary) {
        staged = finalizing = false;
        Access::resetRasterOutput(reader);
        Access::resetRasterOutput(foreign);
        return (!withMLineStyle || Access::prepareSelectedNodMLineStyle(reader))
            && codec.write(&writer, DRW::AC1024, binary);
    };
    const auto verified = [&]() {
        return staged && finalizing && Access::emptyNodChildrenSelected(reader)
            && (!withMLineStyle || Access::selectedNodMLineStyleProof(reader));
    };
    std::vector<Fault> faults{Fault::SourceTail, Fault::WrongSession, Fault::MissingRoot,
                              Fault::MissingBaseline, Fault::WrongBody, Fault::LateTail};
    if (withMLineStyle)
        faults.insert(faults.end(), {Fault::StyleSourceTail, Fault::StyleRootTail,
                                     Fault::WrongStyleBody, Fault::LateStyleTail});
    const std::string sentinel = "preserve empty NOD writer destination\n";
    for (const bool binary : {false, true}) {
        if (!run(binary) || !verified())
            return fail("empty NOD production proof rejected the original source");
        for (const auto bad : faults) {
            fault = bad;
            if (fault == Fault::SourceTail) tail ^= 1u;
            if (fault == Fault::MissingRoot) {
                auto incomplete = roots;
                incomplete.erase(incomplete.begin() + 1);
                codec.setRootDictEntries(std::move(incomplete));
            }
            if (fault == Fault::WrongBody) writer.color.cloning = 2;
            if (fault == Fault::StyleSourceTail) Access::mlineStyleFrameTail(source) ^= 1u;
            if (fault == Fault::StyleRootTail) Access::standardFrameTail(source, 0x17u) ^= 1u;
            if (fault == Fault::WrongStyleBody) writer.style.elements.front().offset += 1.0;
            std::shared_ptr<const DRW_TableEntry> rootBaseline;
            if (fault == Fault::MissingBaseline
                && !Access::replaceStandardDictionaryBaseline(source, 0xCu, nullptr, rootBaseline))
                return fail("cannot remove the test NOD baseline");
            {
                std::ofstream existing(path, std::ios::binary | std::ios::trunc);
                if (!(existing << sentinel)) return fail("cannot seed empty NOD destination");
            }
            const bool saved = run(binary);
            if (fault == Fault::SourceTail) tail ^= 1u;
            if (fault == Fault::StyleSourceTail) Access::mlineStyleFrameTail(source) ^= 1u;
            if (fault == Fault::StyleRootTail) Access::standardFrameTail(source, 0x17u) ^= 1u;
            if (fault == Fault::WrongStyleBody) writer.style = source.mLineStyles.front();
            std::shared_ptr<const DRW_TableEntry> removed;
            if (fault == Fault::MissingBaseline
                && !Access::replaceStandardDictionaryBaseline(source, 0xCu, rootBaseline, removed))
                return fail("cannot restore the test NOD baseline");
            codec.setRootDictEntries(roots);
            writer.color = *color;
            const bool shouldStage = fault != Fault::StyleSourceTail
                && fault != Fault::StyleRootTail
                && !(withMLineStyle && fault == Fault::MissingBaseline);
            const bool shouldFinalize = fault == Fault::WrongBody || fault == Fault::LateTail
                || fault == Fault::LateStyleTail;
            fault = Fault::None;
            std::ifstream existing(path, std::ios::binary);
            if (saved || staged != shouldStage || finalizing != shouldFinalize
                || std::string(std::istreambuf_iterator<char>(existing), {}) != sentinel) {
                std::cerr << "selected NOD fault " << static_cast<int>(bad)
                          << ", binary=" << binary << ", staged=" << staged
                          << ", finalizing=" << finalizing << '\n';
                return fail("empty NOD fault did not refuse in the expected stage");
            }
            existing.close();
            if (!run(binary) || !verified())
                return fail("empty NOD corrected retry failed");
        }
    }
    std::cerr << "AC1024 selected NOD production proof passed " << faults.size()
              << " refusal-and-retry cases per encoding"
              << (withMLineStyle ? " with MLINESTYLE.\n" : ".\n");
    return true;
}

// Test-only projection of the aerial NOD variable graph; this is not a
// complete DWG export or a native consumer interoperability test.
bool physicalAc1024SelectedDictionaryVarWriter(const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface reader;
    dx_data source;
    if (!reader.fileImport(sourcePath.string(), &source, false)
        || source.sourceFormat != dx_source_format::DWG
        || source.sourceVersion != DRW::AC1024
        || !source.hasSingleSuccessfulNamedResourceImport()
        || !source.hasNamedObjectDictionary || source.namedObjectDictionary.handle != 0xCu
        || source.dictionaryVars.size() != 7u
        || source.dictionaryWithDefaults.size() != 1u || source.placeholders.size() != 1u)
        return fail("could not import the aerial AC1024 variable dictionary witness");
    const auto findDictionary = [&source](std::uint32_t handle) {
        return std::find_if(source.dictionaries.begin(), source.dictionaries.end(),
            [handle](const DRW_Dictionary& row) { return row.handle == handle; });
    };
    const auto group = findDictionary(0xDu);
    const auto owner = findDictionary(0x66u);
    if (group == source.dictionaries.end() || owner == source.dictionaries.end()
        || owner->parentHandle != 0xCu || owner->m_entries.size() != 7u)
        return fail("aerial variable dictionary owner is missing");
    const std::map<std::string, std::pair<std::uint32_t, std::string>> expected{
        {"CANNOSCALE", {0x162u, "1:1"}}, {"CMLEADERSTYLE", {0x161u, "STANDARD"}},
        {"CTABLESTYLE", {0x89u, "STANDARD"}}, {"DIMASSOC", {0x67u, "2"}},
        {"HIDETEXT", {0x6Bu, "1"}}, {"LAYEREVAL", {0x312u, "1"}},
        {"LAYERNOTIFY", {0x313u, "15"}}};
    for (const auto& item : expected) {
        const auto entry = std::find_if(owner->m_entries.begin(), owner->m_entries.end(),
            [&item](const DRW_Dictionary::Entry& value) {
                return value.m_name == item.first && value.m_handle == item.second.first;
            });
        const auto value = std::find_if(source.dictionaryVars.begin(), source.dictionaryVars.end(),
            [&item](const DRW_DictionaryVar& row) { return row.handle == item.second.first; });
        if (entry == owner->m_entries.end() || value == source.dictionaryVars.end()
            || value->parentHandle != 0x66u || value->m_schema != 0
            || value->m_value != item.second.second)
            return fail("aerial variable dictionary contents differ from the witness");
    }

    class Writer final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_Dictionary owner;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        std::vector<DRW_DictionaryVar> vars;
        std::function<bool()> finalize;
        void writeAppId() override {}
        void writeObjects() override {
            (void)dxfW->writeDictionaryWithDefault(&plot);
            (void)dxfW->writeAcDbPlaceholder(&placeholder);
            (void)dxfW->writeDictionary(&owner);
            for (auto& value : vars) (void)dxfW->writeDictionaryVar(&value);
        }
        bool finalizeDxfWrite() override { return finalize(); }
    } writer;
    writer.cData = &writer.blank;
    writer.owner = *owner;
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u) return fail("missing Normal plot-style entry");
    writer.plot.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();
    for (const auto& value : source.dictionaryVars) writer.vars.push_back(value);

    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-selected-dictionaryvar-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    } } cleanup{path};
    dxfRW codec(path.string().c_str());
    Access::attachDxfWriter(reader, &codec);
    reader.cData = &source;
    Access::attachDxfWriter(writer, &codec);
    codec.setDxfWriteRecordInventoryEnabled(true);
    codec.setHandleRemap({{0x66u, 0xA066u}, {0x67u, 0xA067u}, {0x6Bu, 0xA06Bu},
        {0x89u, 0xA089u}, {0x161u, 0xA161u}, {0x162u, 0xA162u},
        {0x312u, 0xA312u}, {0x313u, 0xA313u}});
    codec.setRootDictEntries({{"AcDbVariableDictionary", "A066"}, {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not set up selected variable dictionary roots");
    DRW_Class varClass, plotClass, placeholderClass;
    if (!dxfRW::dxfClassForRecordName("DICTIONARYVAR", varClass)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("missing selected variable dictionary CLASSES rows");
    varClass.instanceCount = 7; plotClass.instanceCount = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({varClass, plotClass, placeholderClass});

    enum class Fault { None, OwnerEarly, ChildEarly, WrongBody, OwnerLate, ChildLate };
    Fault fault = Fault::None;
    bool staged = false, finalizing = false;
    if (!codec.setDxfObjectsWriteInspector([&](const auto& view) {
        staged = true;
        return Access::inspectSelectedNodDictionaryVars(reader, view);
    })) return fail("could not set selected variable dictionary inspector");
    auto& ownerWire = Access::dictionaryVarOwnerWire(source);
    auto& childWire = Access::dictionaryVarChildWire(source);
    writer.finalize = [&]() {
        finalizing = true;
        if (fault == Fault::OwnerLate) ownerWire ^= 0x80u;
        if (fault == Fault::ChildLate) childWire ^= 0x40u;
        const bool result = Access::finalizeSelectedNodDictionaryVars(reader);
        if (fault == Fault::OwnerLate) ownerWire ^= 0x80u;
        if (fault == Fault::ChildLate) childWire ^= 0x40u;
        return result;
    };
    const auto run = [&](bool binary) {
        staged = finalizing = false;
        Access::resetRasterOutput(reader);
        return Access::prepareSelectedNodDictionaryVars(reader, source, binary)
            && codec.write(&writer, DRW::AC1024, binary);
    };
    const std::string sentinel = "preserve selected variable dictionary destination\n";
    for (const bool binary : {false, true}) {
        if (!run(binary) || !staged || !finalizing)
            return fail("selected variable dictionary baseline was rejected");
        for (const Fault bad : {Fault::OwnerEarly, Fault::ChildEarly, Fault::WrongBody,
                                Fault::OwnerLate, Fault::ChildLate}) {
            fault = bad;
            if (bad == Fault::OwnerEarly) ownerWire ^= 0x80u;
            if (bad == Fault::ChildEarly) childWire ^= 0x40u;
            if (bad == Fault::WrongBody) writer.vars.front().m_value += "!";
            {
                std::ofstream existing(path, std::ios::binary | std::ios::trunc);
                if (!(existing << sentinel)) return fail("cannot seed selected variable destination");
            }
            const bool saved = run(binary);
            if (bad == Fault::OwnerEarly) ownerWire ^= 0x80u;
            if (bad == Fault::ChildEarly) childWire ^= 0x40u;
            if (bad == Fault::WrongBody) writer.vars.front().m_value.pop_back();
            const bool early = bad == Fault::OwnerEarly || bad == Fault::ChildEarly;
            const bool late = bad == Fault::OwnerLate || bad == Fault::ChildLate;
            fault = Fault::None;
            std::ifstream existing(path, std::ios::binary);
            if (saved || staged == early || finalizing != late
                || std::string(std::istreambuf_iterator<char>(existing), {}) != sentinel)
                return fail("selected variable dictionary fault did not refuse atomically");
            existing.close();
            if (!run(binary) || !staged || !finalizing)
                return fail("selected variable dictionary corrected retry failed");
        }
    }
    std::cerr << "AC1024 selected variable dictionary writer passed five refusal/retry controls in both encodings.\n";
    return true;
}

// Direct selected-root transaction using the original importer and source data.
// It does not assert that the complete aerial DWG can be exported.
bool physicalAc1024SelectedRenderGlobalWriter(const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface reader;
    dx_data source;
    if (!reader.fileImport(sourcePath.string(), &source, false)
        || source.sourceFormat != dx_source_format::DWG || source.sourceVersion != DRW::AC1024
        || !source.hasSingleSuccessfulNamedResourceImport() || !source.hasNamedObjectDictionary
        || source.dictionaryWithDefaults.size() != 1u || source.placeholders.size() != 1u)
        return fail("could not import the aerial RENDERGLOBAL witness");
    const auto dictionary = [&source](std::uint32_t handle) {
        return std::find_if(source.dictionaries.begin(), source.dictionaries.end(),
            [handle](const DRW_Dictionary& value) { return value.handle == handle; });
    };
    const auto group = dictionary(0xDu), extension = dictionary(0x251u);
    const auto global = std::find_if(source.renderSettings.begin(), source.renderSettings.end(),
        [](const DRW_RenderSettings& value) { return value.handle == 0x237u; });
    const auto record = std::find_if(source.xRecords.begin(), source.xRecords.end(),
        [](const DRW_XRecord& value) { return value.handle == 0x252u; });
    if (group == source.dictionaries.end() || extension == source.dictionaries.end()
        || global == source.renderSettings.end() || record == source.xRecords.end())
        return fail("aerial RENDERGLOBAL extension graph is absent");
    class Writer final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_RenderSettings global;
        DRW_Dictionary extension;
        DRW_XRecord record;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        std::function<bool()> finalize;
        void writeAppId() override {}
        void writeObjects() override {
            (void)dxfW->writeDictionaryWithDefault(&plot);
            (void)dxfW->writeAcDbPlaceholder(&placeholder);
            (void)dxfW->writeRenderSettings(&global, "RENDERGLOBAL");
            (void)dxfW->writeDictionary(&extension);
            (void)dxfW->writeXRecord(&record);
        }
        bool finalizeDxfWrite() override { return finalize(); }
    } writer;
    writer.cData = &writer.blank;
    writer.global = *global; writer.extension = *extension; writer.record = *record;
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u) return fail("missing Normal plot-style entry");
    writer.plot.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-selected-renderglobal-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    } } cleanup{path};
    dxfRW codec(path.string().c_str());
    Access::attachDxfWriter(reader, &codec); reader.cData = &source;
    Access::attachDxfWriter(writer, &codec);
    codec.setDxfWriteRecordInventoryEnabled(true);
    codec.setHandleRemap({{0x237u, 0xA237u}, {0x251u, 0xA251u}, {0x252u, 0xA252u}});
    codec.setRootDictEntries({{"ACAD_RENDER_GLOBAL", "A237"}, {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not set up selected RENDERGLOBAL roots");
    DRW_Class renderClass, plotClass, placeholderClass;
    if (!Access::selectedRenderGlobalClass(reader, source, renderClass)
        || renderClass.proxyFlag != 1024 || renderClass.instanceCount != 1
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("missing selected RENDERGLOBAL CLASSES rows");
    renderClass.instanceCount = plotClass.instanceCount = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({renderClass, plotClass, placeholderClass});
    enum class Fault { None, Body, Order, Owner, GlobalEarly, DictionaryEarly, RecordEarly,
                       GlobalLate, DictionaryLate, RecordLate };
    Fault fault = Fault::None;
    bool staged = false, finalizing = false;
    if (!codec.setDxfObjectsWriteInspector([&](const auto& view) {
        staged = true;
        return Access::inspectSelectedRenderGlobal(reader, view);
    })) return fail("could not install selected RENDERGLOBAL inspector");
    auto& globalWire = Access::renderGlobalWire(source);
    auto& dictionaryWire = Access::standardFrameTail(source, 0x251u);
    auto& recordWire = Access::standardFrameTail(source, 0x252u);
    writer.finalize = [&]() {
        finalizing = true;
        if (fault == Fault::GlobalLate) globalWire ^= 0x40u;
        if (fault == Fault::DictionaryLate) dictionaryWire ^= 1u;
        if (fault == Fault::RecordLate) recordWire ^= 1u;
        const bool result = Access::finalizeSelectedRenderGlobal(reader);
        if (fault == Fault::GlobalLate) globalWire ^= 0x40u;
        if (fault == Fault::DictionaryLate) dictionaryWire ^= 1u;
        if (fault == Fault::RecordLate) recordWire ^= 1u;
        return result;
    };
    const auto run = [&](bool binary) {
        staged = finalizing = false;
        Access::resetRasterOutput(reader);
        return Access::prepareSelectedRenderGlobal(reader, source)
            && codec.write(&writer, DRW::AC1024, binary);
    };
    const std::string sentinel = "preserve selected RENDERGLOBAL destination\n";
    for (const bool binary : {false, true}) {
        if (!run(binary) || !staged || !finalizing)
            return fail("selected RENDERGLOBAL baseline was rejected");
        dx_iface reopened;
        dx_data output;
        if (!reopened.fileImport(path.string(), &output, false)
            || std::count_if(output.dxfClasses.begin(), output.dxfClasses.end(),
                [&renderClass](const DRW_Class& row) {
                    return row.recName == renderClass.recName
                        && row.className == renderClass.className && row.appName == renderClass.appName
                        && row.proxyFlag == renderClass.proxyFlag && row.instanceCount == renderClass.instanceCount
                        && row.wasaProxyFlag == renderClass.wasaProxyFlag && row.entityFlag == renderClass.entityFlag;
                }) != 1)
            return fail("selected RENDERGLOBAL CLASSES row changed during publication");
        for (const Fault bad : {Fault::Body, Fault::Order, Fault::Owner,
                Fault::GlobalEarly, Fault::DictionaryEarly, Fault::RecordEarly,
                Fault::GlobalLate, Fault::DictionaryLate, Fault::RecordLate}) {
            fault = bad;
            if (bad == Fault::Body) ++writer.global.m_longs.at(3u);
            if (bad == Fault::Order && !writer.global.setDwgRawData({0u}, 8u, DRW::AC1024))
                return fail("cannot construct selected RENDERGLOBAL order fault");
            if (bad == Fault::Owner) writer.global.parentHandle = 0xDu;
            if (bad == Fault::GlobalEarly) globalWire ^= 0x40u;
            if (bad == Fault::DictionaryEarly) dictionaryWire ^= 1u;
            if (bad == Fault::RecordEarly) recordWire ^= 1u;
            {
                std::ofstream existing(path, std::ios::binary | std::ios::trunc);
                if (!(existing << sentinel)) return fail("cannot seed selected RENDERGLOBAL destination");
            }
            const bool saved = run(binary);
            if (bad == Fault::GlobalEarly) globalWire ^= 0x40u;
            if (bad == Fault::DictionaryEarly) dictionaryWire ^= 1u;
            if (bad == Fault::RecordEarly) recordWire ^= 1u;
            writer.global = *global;
            const bool early = bad == Fault::GlobalEarly || bad == Fault::DictionaryEarly
                || bad == Fault::RecordEarly;
            const bool late = bad == Fault::GlobalLate || bad == Fault::DictionaryLate
                || bad == Fault::RecordLate;
            fault = Fault::None;
            std::ifstream existing(path, std::ios::binary);
            if (saved || staged == early || finalizing != late
                || std::string(std::istreambuf_iterator<char>(existing), {}) != sentinel) {
                std::cerr << "RENDERGLOBAL fault=" << static_cast<int>(bad) << " binary=" << binary
                          << " staged=" << staged << " finalizing=" << finalizing << '\n';
                return fail("selected RENDERGLOBAL fault did not refuse atomically");
            }
            existing.close();
            if (!run(binary) || !staged || !finalizing)
                return fail("selected RENDERGLOBAL corrected retry failed");
        }
    }
    std::cerr << "AC1024 selected RENDERGLOBAL passed nine refusal/retry controls in both encodings.\n";
    return true;
}

// The complete selected aerial render graph is staged from the same imported
// data; this does not assert complete aerial DWG export or native rendering.
bool physicalAc1024SelectedThreeRendersWriter(const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface reader;
    dx_data source;
    if (!reader.fileImport(sourcePath.string(), &source, false)
        || source.sourceFormat != dx_source_format::DWG || source.sourceVersion != DRW::AC1024
        || !source.hasSingleSuccessfulNamedResourceImport() || !source.hasNamedObjectDictionary
        || source.renderSettings.size() != 3u || source.dictionaryWithDefaults.size() != 1u
        || source.placeholders.size() != 1u)
        return fail("could not import the selected aerial render witnesses");
    const auto findHandle = [](auto& rows, std::uint32_t handle) {
        return std::find_if(rows.begin(), rows.end(), [handle](const auto& value) {
            return value.handle == handle;
        });
    };
    const auto group = findHandle(source.dictionaries, 0xDu);
    if (group == source.dictionaries.end()) return fail("aerial GROUP dictionary missing");
    class Writer final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        std::vector<DRW_RenderSettings> settings;
        std::vector<DRW_Dictionary> dictionaries;
        std::vector<DRW_XRecord> records;
        std::function<bool()> writeSourceSettings;
        bool writeFromSource{true};
        std::function<bool()> finalize;
        void writeAppId() override {}
        void writeObjects() override {
            (void)dxfW->writeDictionaryWithDefault(&plot);
            (void)dxfW->writeAcDbPlaceholder(&placeholder);
            if (writeFromSource) {
                if (!writeSourceSettings()) return;
            } else {
                for (auto& value : settings)
                    (void)dxfW->writeRenderSettings(&value, value.handle == 0x237u
                        ? "RENDERGLOBAL" : "MENTALRAYRENDERSETTINGS");
            }
            for (auto& value : dictionaries) (void)dxfW->writeDictionary(&value);
            for (auto& value : records) (void)dxfW->writeXRecord(&value);
        }
        bool finalizeDxfWrite() override { return finalize(); }
    } writer;
    writer.cData = &writer.blank;
    writer.settings.assign(source.renderSettings.begin(), source.renderSettings.end());
    for (const auto handle : {0x1CDu, 0x251u, 0x255u}) {
        const auto row = findHandle(source.dictionaries, handle);
        if (row == source.dictionaries.end()) return fail("selected render dictionary is missing");
        writer.dictionaries.push_back(*row);
    }
    for (const auto handle : {0x1CEu, 0x252u}) {
        const auto row = findHandle(source.xRecords, handle);
        if (row == source.xRecords.end()) return fail("selected render XRECORD is missing");
        writer.records.push_back(*row);
    }
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u) return fail("missing Normal plot-style entry");
    writer.plot.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();
    const auto sourceMental = std::find_if(source.renderSettings.begin(), source.renderSettings.end(),
        [](const DRW_RenderSettings& value) { return value.handle == 0x1CCu; });
    const auto writerMental = std::find_if(writer.settings.begin(), writer.settings.end(),
        [](const DRW_RenderSettings& value) { return value.handle == 0x1CCu; });
    const auto writerPreset = std::find_if(writer.settings.begin(), writer.settings.end(),
        [](const DRW_RenderSettings& value) { return value.handle == 0x256u; });
    const auto writerSettingsRoot = findHandle(writer.dictionaries, 0x255u);
    if (sourceMental == source.renderSettings.end() || writerMental == writer.settings.end()
        || writerPreset == writer.settings.end() || writerPreset->m_longs.size() < 4u
        || sourceMental->m_longs.size() < 4u || writerMental->m_longs.size() < 4u
        || sourceMental->m_strings.empty() || sourceMental->m_strings.front().empty()
        || writerMental->m_strings.empty() || writerMental->m_strings.front().empty()
        || sourceMental->m_name.empty() || writerMental->m_name.empty()
        || writerSettingsRoot == writer.dictionaries.end() || writerSettingsRoot->m_entries.size() != 1u)
        return fail("selected MentalRay source graph is incomplete");

    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-selected-three-renders-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    } } cleanup{path};
    dxfRW codec(path.string().c_str());
    Access::attachDxfWriter(reader, &codec); reader.cData = &source;
    Access::attachDxfWriter(writer, &codec);
    codec.setDxfWriteRecordInventoryEnabled(true);
    codec.setHandleRemap({{0x1CCu, 0xA1CCu}, {0x1CDu, 0xA1CDu}, {0x1CEu, 0xA1CEu},
        {0x237u, 0xA237u}, {0x251u, 0xA251u}, {0x252u, 0xA252u},
        {0x255u, 0xA255u}, {0x256u, 0xA256u}});
    codec.setRootDictEntries({{"ACAD_RENDER_ACTIVE_SETTINGS", "A1CC"},
        {"ACAD_RENDER_GLOBAL", "A237"}, {"ACAD_RENDER_SETTINGS", "A255"},
        {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not set up selected render roots");
    DRW_Class globalClass, mentalClass, plotClass, placeholderClass;
    if (!Access::selectedRenderGlobalClass(reader, source, globalClass)
        || !Access::selectedMentalRayClass(reader, source, mentalClass)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("missing same-import selected render CLASSES rows");
    if (globalClass.proxyFlag != 1024 || mentalClass.proxyFlag != 1024
        || globalClass.instanceCount != 1 || mentalClass.instanceCount != 2)
        return fail("selected render CLASSES flags differ from the source");
    plotClass.instanceCount = placeholderClass.instanceCount = 1;
    const std::vector<DRW_Class> classes{globalClass, mentalClass, plotClass, placeholderClass};
    codec.setDxfClasses(classes);
    enum class Fault { None, Body, PresetBody, SameModelBody, PairedName, WrongRole, SwappedRecords, WrongClass,
        ActiveEarly, PresetEarly, DictionaryEarly, RecordEarly, RootEarly,
        ActiveLate, PresetLate, DictionaryLate, RecordLate, RootLate };
    Fault fault = Fault::None;
    bool staged = false, finalizing = false;
    if (!codec.setDxfObjectsWriteInspector([&](const auto& view) {
        staged = true;
        return Access::inspectSelectedRenderGlobal(reader, view);
    })) return fail("could not set selected render inspector");
    auto& activeWire = Access::mentalRayWire(source, 0x1CCu);
    // First UTF-16 character of the captured 1CC name starts at bit 941;
    // this bit changes its low ASCII byte while leaving the length unchanged.
    auto& activeNameWire = Access::mentalRayWireAt(source, 0x1CCu, 118u);
    auto& presetWire = Access::mentalRayWire(source, 0x256u);
    auto& dictionaryWire = Access::standardFrameTail(source, 0x1CDu);
    auto& recordWire = Access::standardFrameTail(source, 0x1CEu);
    auto& rootWire = Access::standardFrameTail(source, 0x255u);
    writer.writeSourceSettings = [&]() { return Access::writeSelectedDwgRenders(reader); };
    writer.finalize = [&]() {
        finalizing = true;
        if (fault == Fault::ActiveLate) activeWire ^= 0x80u;
        if (fault == Fault::PresetLate) presetWire ^= 0x80u;
        if (fault == Fault::DictionaryLate) dictionaryWire ^= 1u;
        if (fault == Fault::RecordLate) recordWire ^= 1u;
        if (fault == Fault::RootLate) rootWire ^= 1u;
        const bool result = Access::finalizeSelectedRenderGlobal(reader);
        if (fault == Fault::ActiveLate) activeWire ^= 0x80u;
        if (fault == Fault::PresetLate) presetWire ^= 0x80u;
        if (fault == Fault::DictionaryLate) dictionaryWire ^= 1u;
        if (fault == Fault::RecordLate) recordWire ^= 1u;
        if (fault == Fault::RootLate) rootWire ^= 1u;
        return result;
    };
    const auto run = [&](bool binary) {
        staged = finalizing = false;
        Access::resetRasterOutput(reader);
        writer.writeFromSource = fault == Fault::None;
        return Access::prepareSelectedThreeRenders(reader, source)
            && codec.write(&writer, DRW::AC1024, binary);
    };
    const std::string sentinel = "preserve selected three-render destination\n";
    for (const bool binary : {false, true}) {
        if (!run(binary) || !staged || !finalizing)
            return fail("selected three-render baseline was rejected");
        for (const Fault bad : {Fault::Body, Fault::PresetBody, Fault::SameModelBody, Fault::PairedName, Fault::WrongRole,
                Fault::SwappedRecords, Fault::WrongClass,
                Fault::ActiveEarly, Fault::PresetEarly, Fault::DictionaryEarly,
                Fault::RecordEarly, Fault::RootEarly,
                Fault::ActiveLate, Fault::PresetLate, Fault::DictionaryLate,
                Fault::RecordLate, Fault::RootLate}) {
            fault = bad;
            if (bad == Fault::Body || bad == Fault::SameModelBody) ++writerMental->m_longs[3];
            if (bad == Fault::PresetBody) ++writerPreset->m_longs[3];
            if (bad == Fault::SameModelBody) ++sourceMental->m_longs[3];
            if (bad == Fault::PairedName) {
                activeNameWire ^= 0x08u;
                sourceMental->m_strings.front().front() ^= 1;
                sourceMental->m_name.front() ^= 1;
                writerMental->m_strings.front().front() ^= 1;
                writerMental->m_name.front() ^= 1;
            }
            if (bad == Fault::WrongRole)
                writerSettingsRoot->m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::HardOwner;
            if (bad == Fault::SwappedRecords) std::swap(writer.records[0].handle, writer.records[1].handle);
            if (bad == Fault::WrongClass) {
                auto wrongClasses = classes; wrongClasses[1].proxyFlag ^= 1;
                codec.setDxfClasses(wrongClasses);
            }
            if (bad == Fault::ActiveEarly) activeWire ^= 0x80u;
            if (bad == Fault::PresetEarly) presetWire ^= 0x80u;
            if (bad == Fault::DictionaryEarly) dictionaryWire ^= 1u;
            if (bad == Fault::RecordEarly) recordWire ^= 1u;
            if (bad == Fault::RootEarly) rootWire ^= 1u;
            {
                std::ofstream existing(path, std::ios::binary | std::ios::trunc);
                if (!(existing << sentinel)) return fail("cannot seed selected render destination");
            }
            const bool saved = run(binary);
            if (bad == Fault::Body || bad == Fault::SameModelBody) --writerMental->m_longs[3];
            if (bad == Fault::PresetBody) --writerPreset->m_longs[3];
            if (bad == Fault::SameModelBody) --sourceMental->m_longs[3];
            if (bad == Fault::PairedName) {
                activeNameWire ^= 0x08u;
                sourceMental->m_strings.front().front() ^= 1;
                sourceMental->m_name.front() ^= 1;
                writerMental->m_strings.front().front() ^= 1;
                writerMental->m_name.front() ^= 1;
            }
            if (bad == Fault::WrongRole)
                writerSettingsRoot->m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::Unspecified;
            if (bad == Fault::SwappedRecords) std::swap(writer.records[0].handle, writer.records[1].handle);
            if (bad == Fault::WrongClass) codec.setDxfClasses(classes);
            if (bad == Fault::ActiveEarly) activeWire ^= 0x80u;
            if (bad == Fault::PresetEarly) presetWire ^= 0x80u;
            if (bad == Fault::DictionaryEarly) dictionaryWire ^= 1u;
            if (bad == Fault::RecordEarly) recordWire ^= 1u;
            if (bad == Fault::RootEarly) rootWire ^= 1u;
            const bool early = bad == Fault::SameModelBody || bad == Fault::PairedName || bad == Fault::ActiveEarly
                || bad == Fault::PresetEarly || bad == Fault::DictionaryEarly
                || bad == Fault::RecordEarly || bad == Fault::RootEarly;
            const bool late = bad == Fault::ActiveLate || bad == Fault::PresetLate
                || bad == Fault::DictionaryLate || bad == Fault::RecordLate || bad == Fault::RootLate;
            const bool stagedFault = bad == Fault::Body || bad == Fault::PresetBody || bad == Fault::WrongRole
                || bad == Fault::SwappedRecords;
            fault = Fault::None;
            std::ifstream existing(path, std::ios::binary);
            if (saved || (early && staged) || (late && (!staged || !finalizing))
                || (stagedFault && !staged)
                || std::string(std::istreambuf_iterator<char>(existing), {}) != sentinel) {
                std::cerr << "three-render fault=" << static_cast<int>(bad) << " binary=" << binary
                          << " staged=" << staged << " finalizing=" << finalizing << '\n';
                return fail("selected three-render fault did not refuse atomically");
            }
            existing.close();
            if (!run(binary) || !staged || !finalizing)
                return fail("selected three-render corrected retry failed");
        }
    }
    std::cerr << "AC1024 selected three-render graph passed seventeen refusal/retry controls in both encodings.\n";
    return true;
}

// Stages only the aerial MLEADERSTYLE root and its required symbol-table
// targets from the original importer. This is not a whole-DWG export test.
bool physicalAc1024SelectedMLeaderStylesWriter(const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface reader;
    dx_data source;
    if (!reader.fileImport(sourcePath.string(), &source, false)
        || source.sourceFormat != dx_source_format::DWG || source.sourceVersion != DRW::AC1024
        || !source.hasSingleSuccessfulNamedResourceImport() || !source.hasNamedObjectDictionary
        || source.namedObjectDictionary.handle != 0xCu || source.mLeaderStyles.size() != 2u
        || source.dictionaryWithDefaults.size() != 1u || source.placeholders.size() != 1u)
        return fail("could not import the aerial MLEADERSTYLE witness");
    const auto findHandle = [](auto& rows, std::uint32_t handle) {
        return std::find_if(rows.begin(), rows.end(), [handle](const auto& value) {
            return value.handle == handle;
        });
    };
    const auto group = findHandle(source.dictionaries, 0xDu);
    const auto owner = findHandle(source.dictionaries, 0x147u);
    const auto lineType = findHandle(source.lineTypes, 0x14u);
    const auto textStyle = findHandle(source.textStyles, 0x11u);
    const auto sourceStyle = findHandle(source.mLeaderStyles, 0x148u);
    if (group == source.dictionaries.end() || owner == source.dictionaries.end()
        || lineType == source.lineTypes.end() || textStyle == source.textStyles.end()
        || sourceStyle == source.mLeaderStyles.end() || lineType->name != "ByBlock"
        || textStyle->name != "Standard" || owner->parentHandle != 0xCu
        || owner->m_entries.size() != 2u)
        return fail("aerial MLEADERSTYLE owner or symbol sources are missing");
    for (const auto& expected : {std::make_pair("Standard", 0x148u),
                                 std::make_pair("Annotative", 0x157u)}) {
        const auto style = findHandle(source.mLeaderStyles, expected.second);
        const auto entry = std::find_if(owner->m_entries.begin(), owner->m_entries.end(),
            [&expected](const DRW_Dictionary::Entry& value) {
                return value.m_name == expected.first && value.m_handle == expected.second;
            });
        if (style == source.mLeaderStyles.end() || entry == owner->m_entries.end()
            || style->parentHandle != 0x147u || style->leaderLineTypeHandle.ref != 0x14u
            || style->textStyleHandle.ref != 0x11u || style->arrowHeadBlockHandle.ref != 0u
            || style->blockHandle.ref != 0u)
            return fail("aerial MLEADERSTYLE membership or references differ from the witness");
    }
    class Writer final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_LType lineType;
        DRW_Textstyle textStyle;
        DRW_Dictionary owner;
        std::vector<DRW_MLeaderStyle> styles;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        std::function<bool()> finalize;
        void writeLTypes() override { (void)dxfW->writeLineType(&lineType); }
        void writeTextstyles() override { (void)dxfW->writeTextstyle(&textStyle); }
        void writeAppId() override {}
        void writeObjects() override {
            (void)dxfW->writeDictionaryWithDefault(&plot);
            (void)dxfW->writeAcDbPlaceholder(&placeholder);
            (void)dxfW->writeDictionary(&owner);
            for (auto& value : styles) (void)dxfW->writeMLeaderStyle(&value);
        }
        bool finalizeDxfWrite() override { return finalize(); }
    } writer;
    writer.cData = &writer.blank;
    writer.lineType = *lineType; writer.textStyle = *textStyle; writer.owner = *owner;
    writer.styles.assign(source.mLeaderStyles.begin(), source.mLeaderStyles.end());
    const auto writerStyle = findHandle(writer.styles, 0x148u);
    if (writerStyle == writer.styles.end()) return fail("selected MLEADERSTYLE output model is missing");
    const double originalWriterHeight = writerStyle->textHeight;
    const double originalSourceHeight = sourceStyle->textHeight;
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u) return fail("missing Normal plot-style entry");
    writer.plot.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-selected-mleaderstyle-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    } } cleanup{path};
    dxfRW codec(path.string().c_str());
    Access::attachDxfWriter(reader, &codec); reader.cData = &source;
    Access::attachDxfWriter(writer, &codec);
    codec.setDxfWriteRecordInventoryEnabled(true);
    codec.setCanonicalLineTypeMetadata({writer.lineType});
    // ByBlock is a mandatory fixed DXF table row and stays at handle 0x14.
    codec.setHandleRemap({{0x11u, 0xA011u}, {0x147u, 0xA147u},
                          {0x148u, 0xA148u}, {0x157u, 0xA157u}});
    codec.setRootDictEntries({{"ACAD_MLEADERSTYLE", "A147"}, {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not set up selected MLEADERSTYLE roots");
    DRW_Class styleClass, plotClass, placeholderClass;
    if (!Access::selectedAerialMLeaderClass(reader, styleClass)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("missing selected MLEADERSTYLE CLASSES rows");
    if (styleClass.proxyFlag != 4095) return fail("selected MLEADERSTYLE CLASS proxy flags changed");
    styleClass.instanceCount = 2; plotClass.instanceCount = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({styleClass, plotClass, placeholderClass});
    enum class Fault { None, Body, SameModelBody, WrongTarget, WrongRole, WrongStyleName, WrongStyleFont,
                       FirstEarly, SecondEarly, OwnerEarly, FirstLate, SecondLate, OwnerLate };
    Fault fault = Fault::None;
    bool tablesStaged = false, staged = false, finalizing = false;
    if (!codec.setDxfTablesWriteInspector([&](const auto& view) {
        tablesStaged = true;
        return Access::inspectSelectedAerialMLeaderTables(reader, view);
    }) || !codec.setDxfObjectsWriteInspector([&](const auto& view) {
        staged = true;
        return codec.dxfWriteRecordHasIdentity(0x14u, "LTYPE", "TABLES")
            && codec.dxfWriteRecordHasIdentity(0xA011u, "STYLE", "TABLES")
            && Access::inspectSelectedAerialMLeaders(reader, view);
    })) return fail("could not install selected MLEADERSTYLE inspector");
    auto& firstWire = Access::aerialMLeaderWire(source, 0x148u);
    auto& secondWire = Access::aerialMLeaderWire(source, 0x157u);
    auto& ownerWire = Access::standardFrameTail(source, 0x147u);
    writer.finalize = [&]() {
        finalizing = true;
        if (fault == Fault::FirstLate) firstWire ^= 0x80u;
        if (fault == Fault::SecondLate) secondWire ^= 0x80u;
        if (fault == Fault::OwnerLate) ownerWire ^= 1u;
        const bool result = Access::finalizeSelectedAerialMLeaders(reader);
        if (fault == Fault::FirstLate) firstWire ^= 0x80u;
        if (fault == Fault::SecondLate) secondWire ^= 0x80u;
        if (fault == Fault::OwnerLate) ownerWire ^= 1u;
        return result;
    };
    const auto run = [&](bool binary) {
        tablesStaged = staged = finalizing = false;
        Access::resetRasterOutput(reader);
        return Access::prepareSelectedAerialMLeaders(reader, binary)
            && codec.write(&writer, DRW::AC1024, binary);
    };
    const std::string sentinel = "preserve selected MLEADERSTYLE destination\n";
    for (const bool binary : {false, true}) {
        if (!run(binary) || !tablesStaged || !staged || !finalizing)
            return fail("selected MLEADERSTYLE baseline was rejected");
        for (const Fault bad : {Fault::Body, Fault::SameModelBody, Fault::WrongTarget, Fault::WrongRole,
                Fault::WrongStyleName, Fault::WrongStyleFont,
                Fault::FirstEarly, Fault::SecondEarly, Fault::OwnerEarly,
                Fault::FirstLate, Fault::SecondLate, Fault::OwnerLate}) {
            fault = bad;
            if (bad == Fault::Body || bad == Fault::SameModelBody) writerStyle->textHeight += 1.0;
            if (bad == Fault::SameModelBody) sourceStyle->textHeight += 1.0;
            if (bad == Fault::WrongTarget) writerStyle->textStyleHandle.ref = 0x14u;
            if (bad == Fault::WrongRole)
                writer.owner.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::HardOwner;
            if (bad == Fault::WrongStyleName) writer.textStyle.name = "AlteredStyle";
            if (bad == Fault::WrongStyleFont) writer.textStyle.font = "altered-font.shx";
            if (bad == Fault::FirstEarly) firstWire ^= 0x80u;
            if (bad == Fault::SecondEarly) secondWire ^= 0x80u;
            if (bad == Fault::OwnerEarly) ownerWire ^= 1u;
            {
                std::ofstream existing(path, std::ios::binary | std::ios::trunc);
                if (!(existing << sentinel)) return fail("cannot seed selected MLEADERSTYLE destination");
            }
            const bool saved = run(binary);
            if (bad == Fault::Body || bad == Fault::SameModelBody) writerStyle->textHeight = originalWriterHeight;
            if (bad == Fault::SameModelBody) sourceStyle->textHeight = originalSourceHeight;
            if (bad == Fault::WrongTarget) writerStyle->textStyleHandle.ref = 0x11u;
            if (bad == Fault::WrongRole)
                writer.owner.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::Unspecified;
            if (bad == Fault::WrongStyleName || bad == Fault::WrongStyleFont) writer.textStyle = *textStyle;
            if (bad == Fault::FirstEarly) firstWire ^= 0x80u;
            if (bad == Fault::SecondEarly) secondWire ^= 0x80u;
            if (bad == Fault::OwnerEarly) ownerWire ^= 1u;
            const bool early = bad == Fault::SameModelBody || bad == Fault::FirstEarly
                || bad == Fault::SecondEarly || bad == Fault::OwnerEarly;
            const bool late = bad == Fault::FirstLate || bad == Fault::SecondLate || bad == Fault::OwnerLate;
            const bool stagedFault = bad == Fault::Body || bad == Fault::WrongTarget || bad == Fault::WrongRole;
            const bool tableFault = bad == Fault::WrongStyleName || bad == Fault::WrongStyleFont;
            fault = Fault::None;
            std::ifstream existing(path, std::ios::binary);
            if (saved || (early && staged) || (late && (!staged || !finalizing))
                || (stagedFault && !staged)
                || (tableFault && (!tablesStaged || staged || finalizing))
                || std::string(std::istreambuf_iterator<char>(existing), {}) != sentinel) {
                std::cerr << "MLEADERSTYLE fault=" << static_cast<int>(bad) << " binary=" << binary
                          << " staged=" << staged << " finalizing=" << finalizing << '\n';
                return fail("selected MLEADERSTYLE fault did not refuse atomically");
            }
            existing.close();
            if (!run(binary) || !tablesStaged || !staged || !finalizing)
                return fail("selected MLEADERSTYLE corrected retry failed");
        }
    }
    std::cerr << "AC1024 selected MLEADERSTYLE passed twelve refusal/retry controls in both encodings.\n";
    return true;
}

// Selected TABLESTYLE staging only; this does not qualify the full DWG.
bool physicalAc1024SelectedTableStyleWriter(const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface reader;
    dx_data source;
    if (!reader.fileImport(sourcePath.string(), &source, false)
        || source.sourceFormat != dx_source_format::DWG
        || source.sourceVersion != DRW::AC1024
        || !source.hasSingleSuccessfulNamedResourceImport()
        || !source.hasNamedObjectDictionary || source.namedObjectDictionary.handle != 0xCu
        || source.tableStyles.size() != 1u || source.dictionaryWithDefaults.size() != 1u
        || source.placeholders.size() != 1u)
        return fail("could not import selected aerial TABLESTYLE witness");
    const auto findHandle = [](auto& rows, std::uint32_t handle) {
        return std::find_if(rows.begin(), rows.end(), [handle](const auto& row) {
            return row.handle == handle;
        });
    };
    const auto group = findHandle(source.dictionaries, 0xDu);
    const auto owner = findHandle(source.dictionaries, 0x86u);
    const auto extension = findHandle(source.dictionaries, 0x238u);
    const auto textStyle = findHandle(source.textStyles, 0x11u);
    const auto style = findHandle(source.tableStyles, 0x87u);
    if (group == source.dictionaries.end() || owner == source.dictionaries.end()
        || extension == source.dictionaries.end() || textStyle == source.textStyles.end()
        || style == source.tableStyles.end() || style->parentHandle != 0x86u
        || style->xDictHandle != 0x238u || extension->parentHandle != 0x87u
        || !extension->m_entries.empty() || style->m_cellStyles.size() != 3u
        || style->m_cellStyles.front().m_borders.empty())
        return fail("aerial TABLESTYLE selected graph differs from its witness");

    class Writer final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_Textstyle text;
        std::list<DRW_Textstyle> textStyles;
        DRW_Dictionary owner, extension;
        DRW_TableStyle style;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        std::function<bool()> finalize;
        void writeTextstyles() override { (void)dxfW->writeTextstyle(&text); }
        void writeAppId() override {}
        void writeObjects() override {
            (void)dxfW->writeDictionaryWithDefault(&plot);
            (void)dxfW->writeAcDbPlaceholder(&placeholder);
            (void)dxfW->writeDictionary(&owner);
            (void)dxfW->writeDictionary(&extension);
            (void)dxfW->writeTableStyle(&style, textStyles);
        }
        bool finalizeDxfWrite() override { return finalize(); }
    } writer;
    writer.cData = &writer.blank;
    writer.text = *textStyle;
    writer.textStyles.push_back(*textStyle);
    writer.owner = *owner;
    writer.extension = *extension;
    writer.style = *style;
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u) return fail("missing Normal plot-style entry");
    writer.plot.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-selected-tablestyle-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    } } cleanup{path};
    dxfRW codec(path.string().c_str());
    reader.cData = &source;
    Access::attachDxfWriter(reader, &codec);
    Access::attachDxfWriter(writer, &codec);
    codec.setDxfWriteRecordInventoryEnabled(true);
    codec.setHandleRemap({{0x11u, 0xA011u}, {0x86u, 0xA086u},
                          {0x87u, 0xA087u}, {0x238u, 0xA238u}});
    codec.setRootDictEntries({{"ACAD_TABLESTYLE", "A086"}, {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not set up selected TABLESTYLE roots");
    DRW_Class styleClass, plotClass, placeholderClass;
    if (!Access::selectedAerialTableStyleClass(reader, source, styleClass)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("missing selected TABLESTYLE classes");
    styleClass.instanceCount = plotClass.instanceCount = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({styleClass, plotClass, placeholderClass});
    enum class Fault { None, Height, Border, MissingStyle, Marker, ExtensionOwner,
                       ExtensionFlag, StyleFont, StyleWireEarly, ExtensionWireEarly,
                       DictionaryWireLate };
    Fault fault = Fault::None;
    bool tables = false, objects = false, finalizing = false, alteredBody = false;
    if (!codec.setDxfTablesWriteInspector([&](const auto& view) {
        tables = true;
        return Access::inspectSelectedAerialTableStyleTables(reader, view);
    }) || !codec.setDxfObjectsWriteInspector([&](const auto& view) {
        objects = true;
        if (fault == Fault::Marker || fault == Fault::MissingStyle) {
            std::string bytes(view.bytes);
            const auto record = bytes.find("TABLESTYLE");
            const std::string needle = fault == Fault::Marker ? "CELLSTYLE_END" : "A011";
            const auto at = record == std::string::npos ? std::string::npos : bytes.find(needle, record);
            if (at == std::string::npos) return false;
            bytes[at] = fault == Fault::Marker ? 'X' : 'B';
            alteredBody = true;
            auto altered = view;
            altered.bytes = bytes;
            return Access::inspectSelectedAerialTableStyle(reader, altered);
        }
        return Access::inspectSelectedAerialTableStyle(reader, view);
    })) return fail("could not install selected TABLESTYLE inspectors");
    auto& styleWire = Access::aerialMLeaderWire(source, 0x87u);
    auto& extensionWire = Access::standardFrameTail(source, 0x238u);
    auto& dictionaryWire = Access::standardFrameTail(source, 0x86u);
    writer.finalize = [&]() {
        finalizing = true;
        if (fault == Fault::DictionaryWireLate) dictionaryWire ^= 1u;
        const bool result = Access::finalizeSelectedAerialTableStyle(reader);
        if (fault == Fault::DictionaryWireLate) dictionaryWire ^= 1u;
        return result;
    };
    const auto run = [&](bool binary) {
        tables = objects = finalizing = alteredBody = false;
        Access::resetRasterOutput(reader);
        return Access::prepareSelectedAerialTableStyle(reader, source, *owner, *style)
            && codec.write(&writer, DRW::AC1024, binary);
    };
    const std::string sentinel = "preserve selected TABLESTYLE destination\n";
    for (const bool binary : {false, true}) {
        if (!run(binary) || !tables || !objects || !finalizing)
            return fail("selected TABLESTYLE baseline was rejected");
        for (const Fault bad : {Fault::Height, Fault::Border, Fault::MissingStyle,
                Fault::Marker, Fault::ExtensionOwner, Fault::ExtensionFlag,
                Fault::StyleFont, Fault::StyleWireEarly, Fault::ExtensionWireEarly,
                Fault::DictionaryWireLate}) {
            fault = bad;
            if (bad == Fault::Height) writer.style.m_cellStyles.front().m_contentFormat.m_textHeight += 1.0;
            if (bad == Fault::Border) writer.style.m_cellStyles.front().m_borders.front().m_lineWeight = -1;
            if (bad == Fault::ExtensionOwner) writer.extension.parentHandle = 0x86u;
            if (bad == Fault::ExtensionFlag) writer.extension.hardOwner = 0;
            if (bad == Fault::StyleFont) writer.text.font += ".altered";
            if (bad == Fault::StyleWireEarly) styleWire ^= 0x80u;
            if (bad == Fault::ExtensionWireEarly) extensionWire ^= 1u;
            {
                std::ofstream existing(path, std::ios::binary | std::ios::trunc);
                if (!(existing << sentinel)) return fail("cannot seed TABLESTYLE destination");
            }
            const bool saved = run(binary);
            writer.style = *style;
            writer.extension = *extension;
            writer.text = *textStyle;
            if (bad == Fault::StyleWireEarly) styleWire ^= 0x80u;
            if (bad == Fault::ExtensionWireEarly) extensionWire ^= 1u;
            const bool early = bad == Fault::StyleWireEarly || bad == Fault::ExtensionWireEarly;
            const bool tableFault = bad == Fault::StyleFont;
            const bool late = bad == Fault::DictionaryWireLate;
            const bool editedBytes = bad == Fault::MissingStyle || bad == Fault::Marker;
            fault = Fault::None;
            std::ifstream existing(path, std::ios::binary);
            if (saved || tables == early || objects != (!early && !tableFault)
                || finalizing != late || (editedBytes && !alteredBody)
                || std::string(std::istreambuf_iterator<char>(existing), {}) != sentinel) {
                std::cerr << "TABLESTYLE fault=" << static_cast<int>(bad)
                          << " binary=" << binary << " tables=" << tables
                          << " objects=" << objects << " finalizing=" << finalizing
                          << " altered=" << alteredBody << '\n';
                return fail("selected TABLESTYLE fault did not refuse atomically at the expected stage");
            }
            existing.close();
            if (!run(binary) || !tables || !objects || !finalizing)
                return fail("selected TABLESTYLE corrected retry failed");
        }
    }
    std::cerr << "AC1024 selected TABLESTYLE passed ten refusal/retry controls in both encodings.\n";
    return true;
}

// Stages the original aerial VISUALSTYLE root; this is not a whole-DWG export.
bool physicalAc1024SelectedVisualStyleWriter(const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface reader;
    dx_data source;
    if (!reader.fileImport(sourcePath.string(), &source, false)
        || source.sourceFormat != dx_source_format::DWG || source.sourceVersion != DRW::AC1024
        || !source.hasSingleSuccessfulNamedResourceImport() || !source.hasNamedObjectDictionary
        || source.namedObjectDictionary.handle != 0xCu || source.visualStyles.size() != 27u
        || source.dictionaryWithDefaults.size() != 1u || source.placeholders.size() != 1u)
        return fail("could not import aerial VISUALSTYLE witness");
    const auto findHandle = [](auto& rows, std::uint32_t handle) {
        return std::find_if(rows.begin(), rows.end(), [handle](const auto& value) {
            return value.handle == handle;
        });
    };
    const auto group = findHandle(source.dictionaries, 0xDu);
    const auto owner = findHandle(source.dictionaries, 0x99u);
    if (group == source.dictionaries.end() || owner == source.dictionaries.end()
        || owner->parentHandle != 0xCu || owner->m_entries.size() != source.visualStyles.size())
        return fail("aerial VISUALSTYLE owner graph is incomplete");
    if (std::count_if(owner->m_entries.begin(), owner->m_entries.end(), [](const auto& entry) {
            return entry.m_name == "*A1" && entry.m_handle == 0x19Au;
        }) != 1)
        return fail("aerial VISUALSTYLE anonymous dictionary key changed");

    class Writer final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_Dictionary owner;
        std::vector<DRW_VisualStyle> styles;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        std::function<bool(const DRW_VisualStyle&)> emit;
        std::function<bool()> finalize;
        void writeAppId() override {}
        void writeObjects() override {
            (void)dxfW->writeDictionaryWithDefault(&plot);
            (void)dxfW->writeAcDbPlaceholder(&placeholder);
            (void)dxfW->writeDictionary(&owner);
            for (const auto& value : styles) {
                if (!emit(value)) {
                    (void)dxfW->writeRawDxfObject(nullptr);
                    return;
                }
            }
        }
        bool finalizeDxfWrite() override { return finalize(); }
    } writer;
    writer.cData = &writer.blank;
    writer.owner = *owner;
    writer.styles.assign(source.visualStyles.begin(), source.visualStyles.end());
    std::set<std::uint32_t> identities;
    for (const auto& value : source.visualStyles) {
        if (value.parentHandle != 0x99u || !identities.insert(value.handle).second
            || std::count_if(owner->m_entries.begin(), owner->m_entries.end(),
                [&value](const auto& entry) { return entry.m_handle == value.handle; }) != 1)
            return fail("aerial VISUALSTYLE identity is not uniquely owned");
        if (value.xDictHandle != 0u)
            return fail("aerial VISUALSTYLE source unexpectedly has an extension dictionary");
    }
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u) return fail("missing Normal plot-style entry");
    writer.plot.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-selected-visualstyle-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    } } cleanup{path};
    dxfRW codec(path.string().c_str());
    Access::attachDxfWriter(reader, &codec); reader.cData = &source;
    Access::attachDxfWriter(writer, &codec);
    writer.emit = [&](const DRW_VisualStyle& value) {
        return Access::writeSelectedAerialVisualStyle(reader, value);
    };
    codec.setDxfWriteRecordInventoryEnabled(true);
    std::map<std::uint32_t, std::uint32_t> remap{{0x99u, 0xA099u}};
    for (const auto& value : writer.styles) remap.emplace(value.handle, 0xA000u + value.handle);
    codec.setHandleRemap(remap);
    codec.setRootDictEntries({{"ACAD_VISUALSTYLE", "A099"}, {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not set up selected VISUALSTYLE roots");
    DRW_Class styleClass, plotClass, placeholderClass;
    if (!Access::selectedAerialVisualStyleClass(reader, source, styleClass)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("missing selected VISUALSTYLE classes");
    styleClass.instanceCount = 27; plotClass.instanceCount = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({styleClass, plotClass, placeholderClass});
    enum class Fault { None, Key, Role, Scalar, StyleEarly, OwnerEarly, StyleLate, OwnerLate };
    Fault fault = Fault::None;
    bool staged = false, finalizing = false, altered = false;
    if (!codec.setDxfObjectsWriteInspector([&](const auto& view) {
        staged = true;
        if (fault == Fault::Scalar) {
            std::string bytes(view.bytes);
            const auto start = bytes.find("AcDbVisualStyle");
            const auto at = start == std::string::npos ? std::string::npos
                : bytes.find("2dWireframe", start);
            if (at == std::string::npos) return false;
            bytes[at] = '3'; altered = true;
            auto changed = view; changed.bytes = bytes;
            return Access::inspectSelectedAerialVisualStyles(reader, changed);
        }
        return Access::inspectSelectedAerialVisualStyles(reader, view);
    })) return fail("could not install selected VISUALSTYLE inspector");
    auto& styleWire = Access::aerialMLeaderWire(source, 0x19Au);
    auto& ownerWire = Access::standardFrameTail(source, 0x99u);
    writer.finalize = [&]() {
        finalizing = true;
        if (fault == Fault::StyleLate) styleWire ^= 0x80u;
        if (fault == Fault::OwnerLate) ownerWire ^= 1u;
        const bool result = Access::finalizeSelectedAerialVisualStyles(reader);
        if (fault == Fault::StyleLate) styleWire ^= 0x80u;
        if (fault == Fault::OwnerLate) ownerWire ^= 1u;
        return result;
    };
    const auto run = [&](bool binary) {
        staged = finalizing = altered = false;
        Access::resetRasterOutput(reader);
        Access::attachDxfWriter(reader, nullptr);
        const bool prepared = Access::prepareSelectedAerialVisualStyles(reader, source, binary);
        Access::attachDxfWriter(reader, &codec);
        return prepared && codec.write(&writer, DRW::AC1024, binary);
    };
    const std::string sentinel = "preserve selected VISUALSTYLE destination\n";
    for (const bool binary : {false, true}) {
        if (!run(binary) || !staged || !finalizing)
            return fail("selected VISUALSTYLE baseline was rejected");
        for (const Fault bad : {Fault::Key, Fault::Role, Fault::Scalar,
                Fault::StyleEarly, Fault::OwnerEarly, Fault::StyleLate, Fault::OwnerLate}) {
            fault = bad;
            if (bad == Fault::Key) writer.owner.m_entries.front().m_name += "_altered";
            if (bad == Fault::Role) writer.owner.m_entries.front().m_dxfHandleCode =
                DRW_Dictionary::Entry::DxfHandleCode::HardOwner;
            if (bad == Fault::StyleEarly) styleWire ^= 0x80u;
            if (bad == Fault::OwnerEarly) ownerWire ^= 1u;
            {
                std::ofstream existing(path, std::ios::binary | std::ios::trunc);
                if (!(existing << sentinel)) return fail("cannot seed VISUALSTYLE destination");
            }
            const bool saved = run(binary);
            writer.owner = *owner;
            if (bad == Fault::StyleEarly) styleWire ^= 0x80u;
            if (bad == Fault::OwnerEarly) ownerWire ^= 1u;
            const bool early = bad == Fault::StyleEarly || bad == Fault::OwnerEarly;
            const bool late = bad == Fault::StyleLate || bad == Fault::OwnerLate;
            const bool stagedFault = bad == Fault::Key || bad == Fault::Role || bad == Fault::Scalar;
            fault = Fault::None;
            std::ifstream existing(path, std::ios::binary);
            if (saved || (early && staged) || (late && (!staged || !finalizing))
                || (stagedFault && !staged) || (bad == Fault::Scalar && !altered)
                || std::string(std::istreambuf_iterator<char>(existing), {}) != sentinel) {
                std::cerr << "VISUALSTYLE fault=" << static_cast<int>(bad) << " binary=" << binary
                          << " staged=" << staged << " finalizing=" << finalizing << '\n';
                return fail("selected VISUALSTYLE fault did not refuse atomically");
            }
            existing.close();
            if (!run(binary) || !staged || !finalizing)
                return fail("selected VISUALSTYLE corrected retry failed");
        }
    }
    std::cerr << "AC1024 selected VISUALSTYLE passed seven refusal/retry controls in both encodings.\n";
    return true;
}

// Explicit one-member output projection; the original nine-member source
// dictionary and all import evidence remain intact on the reader.
bool physicalAc1024SelectedByBlockMaterialWriter(const std::filesystem::path& sourcePath, bool byLayer = false) {
    const auto materialHandle = byLayer ? 0x96u : 0x97u;
    const auto extensionHandle = byLayer ? 0x297u : 0x295u;
    const auto recordHandle = byLayer ? 0x298u : 0x296u;
    const std::string materialName = byLayer ? "ByLayer" : "ByBlock";
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface reader;
    dx_data source;
    if (!reader.fileImport(sourcePath.string(), &source, false)
        || source.sourceFormat != dx_source_format::DWG || source.sourceVersion != DRW::AC1024
        || !source.hasSingleSuccessfulNamedResourceImport() || !source.hasNamedObjectDictionary
        || source.namedObjectDictionary.handle != 0xCu || source.dictionaryWithDefaults.size() != 1u
        || source.placeholders.size() != 1u)
        return fail("could not import aerial ByBlock MATERIAL witness");
    const auto findHandle = [](auto& rows, std::uint32_t handle) {
        return std::find_if(rows.begin(), rows.end(), [handle](const auto& value) {
            return value.handle == handle;
        });
    };
    const auto group = findHandle(source.dictionaries, 0xDu);
    const auto owner = findHandle(source.dictionaries, 0x72u);
    const auto material = findHandle(source.materials, materialHandle);
    const auto extension = findHandle(source.dictionaries, extensionHandle);
    const auto record = findHandle(source.xRecords, recordHandle);
    if (group == source.dictionaries.end() || owner == source.dictionaries.end()
        || material == source.materials.end() || extension == source.dictionaries.end()
        || record == source.xRecords.end() || owner->parentHandle != 0xCu
        || owner->m_entries.size() != 9u || material->parentHandle != 0x72u
        || material->xDictHandle != extensionHandle || extension->parentHandle != materialHandle
        || extension->m_entries.size() != 1u || extension->m_entries.front().m_name != "FBXASSET"
        || extension->m_entries.front().m_handle != recordHandle || record->parentHandle != extensionHandle
        || record->m_dataEntries.size() != 193u
        || std::count_if(owner->m_entries.begin(), owner->m_entries.end(), [&](const auto& entry) {
            return entry.m_name == materialName && entry.m_handle == materialHandle;
        }) != 1)
        return fail("aerial ByBlock selected source graph differs from the witness");
    class Writer final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_Dictionary owner, extension;
        DRW_Material material;
        DRW_XRecord record;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        bool skipRecord{false};
        std::function<bool()> finalize;
        void writeAppId() override {}
        void writeObjects() override {
            (void)dxfW->writeDictionaryWithDefault(&plot);
            (void)dxfW->writeAcDbPlaceholder(&placeholder);
            (void)dxfW->writeDictionary(&owner);
            (void)dxfW->writeMaterial(&material);
            (void)dxfW->writeDictionary(&extension);
            if (!skipRecord) (void)dxfW->writeXRecord(&record);
        }
        bool finalizeDxfWrite() override { return finalize(); }
    } writer;
    writer.cData = &writer.blank;
    writer.owner = *owner;
    writer.owner.m_entries.erase(std::remove_if(writer.owner.m_entries.begin(),
        writer.owner.m_entries.end(), [&](const auto& entry) {
            return entry.m_name != materialName || entry.m_handle != materialHandle;
        }), writer.owner.m_entries.end());
    if (writer.owner.m_entries.size() != 1u) return fail("ByBlock projection is ambiguous");
    writer.material = *material;
    writer.extension = *extension;
    writer.record = *record;
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u) return fail("missing Normal plot-style entry");
    writer.plot.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();
    const auto binaryField = std::find_if(writer.record.m_dataEntries.begin(),
        writer.record.m_dataEntries.end(), [](const DRW_Variant& value) {
            return value.code() == 310 && value.binary() != nullptr && !value.binary()->empty();
        });
    if (binaryField == writer.record.m_dataEntries.end()) return fail("ByBlock binary witness is missing");
    const auto binaryIndex = static_cast<std::size_t>(binaryField - writer.record.m_dataEntries.begin());
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-selected-byblock-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    } } cleanup{path};
    dxfRW codec(path.string().c_str());
    reader.cData = &source;
    Access::attachDxfWriter(reader, &codec);
    Access::attachDxfWriter(writer, &codec);
    codec.setDxfWriteRecordInventoryEnabled(true);
    codec.setHandleRemap({{0x72u, 0xA072u}, {materialHandle, 0xA000u + materialHandle},
                          {extensionHandle, 0xA000u + extensionHandle}, {recordHandle, 0xA000u + recordHandle}});
    codec.setRootDictEntries({{"ACAD_MATERIAL", "A072"}, {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not set up selected ByBlock roots");
    DRW_Class materialClass, plotClass, placeholderClass;
    if (!Access::selectedAerialByBlockClass(reader, source, materialClass, materialHandle)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("missing selected ByBlock classes");
    materialClass.instanceCount = plotClass.instanceCount = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({materialClass, plotClass, placeholderClass});
    enum class Fault { None, RootRole, ExtensionRole, BinaryByte, Truncated, MissingTarget,
                       MaterialEarly, RecordEarly, MaterialLate, RecordLate };
    Fault fault = Fault::None;
    bool staged = false, finalizing = false;
    if (!codec.setDxfObjectsWriteInspector([&](const auto& view) {
        staged = true;
        return Access::inspectSelectedAerialByBlock(reader, view);
    })) return fail("could not install selected ByBlock inspector");
    auto& materialWire = Access::aerialMLeaderWire(source, materialHandle);
    auto& recordWire = Access::standardFrameTail(source, recordHandle);
    writer.finalize = [&]() {
        finalizing = true;
        if (fault == Fault::MaterialLate) materialWire ^= 0x80u;
        if (fault == Fault::RecordLate) recordWire ^= 1u;
        const bool result = Access::finalizeSelectedAerialByBlock(reader);
        if (fault == Fault::MaterialLate) materialWire ^= 0x80u;
        if (fault == Fault::RecordLate) recordWire ^= 1u;
        return result;
    };
    const auto run = [&](bool binary) {
        staged = finalizing = false;
        Access::resetRasterOutput(reader);
        return Access::prepareSelectedAerialByBlock(reader, source, *material, *extension)
            && codec.write(&writer, DRW::AC1024, binary);
    };
    const std::string sentinel = "preserve selected ByBlock destination\n";
    for (const bool binary : {false, true}) {
        if (!run(binary) || !staged || !finalizing)
            return fail("selected MATERIAL baseline was rejected");
        if (const char* capture = std::getenv("LIBDXFRW_MATERIAL_RESEARCH_CAPTURE")) {
            std::error_code error;
            std::filesystem::copy_file(path, std::string(capture) + (binary ? "-binary.dxf" : "-ascii.dxf"), error);
            if (error) return fail("could not save selected material research output");
        }
        for (const Fault bad : {Fault::RootRole, Fault::ExtensionRole, Fault::BinaryByte,
                Fault::Truncated, Fault::MissingTarget, Fault::MaterialEarly, Fault::RecordEarly,
                Fault::MaterialLate, Fault::RecordLate}) {
            fault = bad;
            if (bad == Fault::RootRole) writer.owner.m_entries.front().m_dxfHandleCode =
                DRW_Dictionary::Entry::DxfHandleCode::HardOwner;
            if (bad == Fault::ExtensionRole) writer.extension.m_entries.front().m_dxfHandleCode =
                DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
            if (bad == Fault::BinaryByte) {
                auto bytes = *writer.record.m_dataEntries[binaryIndex].binary();
                bytes.front() ^= 1u;
                writer.record.m_dataEntries[binaryIndex] = DRW_Variant(310, std::move(bytes));
            }
            if (bad == Fault::Truncated) writer.record.m_dataEntries.pop_back();
            if (bad == Fault::MissingTarget) writer.skipRecord = true;
            if (bad == Fault::MaterialEarly) materialWire ^= 0x80u;
            if (bad == Fault::RecordEarly) recordWire ^= 1u;
            {
                std::ofstream existing(path, std::ios::binary | std::ios::trunc);
                if (!(existing << sentinel)) return fail("cannot seed selected MATERIAL destination");
            }
            const bool saved = run(binary);
            writer.owner = *owner;
            writer.owner.m_entries.erase(std::remove_if(writer.owner.m_entries.begin(),
                writer.owner.m_entries.end(), [&](const auto& entry) {
                    return entry.m_name != materialName || entry.m_handle != materialHandle;
                }), writer.owner.m_entries.end());
            writer.extension = *extension;
            writer.record = *record;
            writer.skipRecord = false;
            if (bad == Fault::MaterialEarly) materialWire ^= 0x80u;
            if (bad == Fault::RecordEarly) recordWire ^= 1u;
            const bool early = bad == Fault::MaterialEarly || bad == Fault::RecordEarly;
            const bool late = bad == Fault::MaterialLate || bad == Fault::RecordLate;
            fault = Fault::None;
            std::ifstream existing(path, std::ios::binary);
            if (saved || (early && staged) || (late && (!staged || !finalizing))
                || (!early && !late && !staged)
                || std::string(std::istreambuf_iterator<char>(existing), {}) != sentinel) {
                std::cerr << "ByBlock fault=" << static_cast<int>(bad) << " binary=" << binary
                          << " staged=" << staged << " finalizing=" << finalizing << '\n';
                return fail("selected MATERIAL fault did not refuse atomically");
            }
            existing.close();
            if (!run(binary) || !staged || !finalizing)
                return fail("selected MATERIAL corrected retry failed");
        }
    }
    std::cerr << "AC1024 selected " << materialName << " MATERIAL passed nine refusal/retry controls in both encodings.\n";
    return true;
}
