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
