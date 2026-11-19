// SPDX-License-Identifier: GPL-2.0-or-later

bool physicalAc1024SelectedAdvMaterialWriter(
        const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface sourceIface;
    dx_data source;
    if (!sourceIface.fileImport(sourcePath.string(), &source, false)
        || !Access::qualifiedAerialAdvMaterial(sourceIface, source))
        return fail("AC1024 ADVMATERIAL source profile is not qualified");

    const auto findHandle = [](auto& rows, std::uint32_t handle) {
        return std::find_if(rows.begin(), rows.end(), [handle](const auto& row) {
            return row.handle == handle;
        });
    };
    auto material = findHandle(source.materials, 0x2C2u);
    auto extension = findHandle(source.dictionaries, 0x2C3u);
    auto record = findHandle(source.xRecords, 0x2C4u);
    auto owner = findHandle(source.dictionaries, 0x72u);
    auto group = findHandle(source.dictionaries, 0xDu);
    if (source.dictionaryWithDefaults.size() != 1u
        || source.placeholders.size() != 1u)
        return fail("AC1024 ADVMATERIAL default plot-style dependency is missing");
    if (material == source.materials.end() || extension == source.dictionaries.end()
        || record == source.xRecords.end() || owner == source.dictionaries.end()
        || group == source.dictionaries.end())
        return fail("AC1024 ADVMATERIAL output graph is incomplete");
    std::vector<DRW_Dictionary::Entry> rootEntries;
    std::copy_if(owner->m_entries.begin(), owner->m_entries.end(),
        std::back_inserter(rootEntries), [&record](const auto& entry) {
            return entry.m_handle == record->handle - 2u;
        });
    const auto advMaterialEntries = std::count_if(extension->m_entries.begin(),
        extension->m_entries.end(), [&record](const auto& entry) {
            return entry.m_name == "ADVMATERIAL"
                && entry.m_handle == record->handle;
        });
    if (rootEntries.size() != 1u || advMaterialEntries != 1
        || material->parentHandle != owner->handle
        || material->xDictHandle != extension->handle
        || record->parentHandle != extension->handle
        || record->m_dataEntries.size() != 10u || record->m_values.size() != 10u)
        return fail("AC1024 ADVMATERIAL output edges differ from source profile");
    const std::string selectedMaterialName = rootEntries.front().m_name;
    const auto appId = findHandle(source.appIds, 0x12u);
    if (appId == source.appIds.end() || appId->name != "ACAD")
        return fail("AC1024 ADVMATERIAL APPID dependency is missing");

    class SelectedWriter final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_AppId appId;
        DRW_Dictionary owner, extension;
        DRW_DictionaryWithDefault plot;
        DRW_Material material;
        DRW_XRecord* record{nullptr};
        DRW_AcDbPlaceholder placeholder;
        dx_iface* sourceIface{nullptr};
        bool compatibilityRoute{false};
        bool routeResult{false};
        std::string dependencyResults;
        void writeAppId() override { (void)dxfW->writeAppId(&appId); }
        bool finalizeDxfWrite() override { return true; }
        void writeObjects() override {
            const bool wrotePlot = dxfW->writeDictionaryWithDefault(&plot);
            const bool wrotePlaceholder = dxfW->writeAcDbPlaceholder(&placeholder);
            const bool wroteOwner = dxfW->writeDictionary(&owner);
            const bool wroteExtension = dxfW->writeDictionary(&extension);
            const bool wroteMaterial = dxfW->writeMaterial(&material);
            dependencyResults = std::to_string(wrotePlot) + std::to_string(wrotePlaceholder)
                + std::to_string(wroteOwner) + std::to_string(wroteExtension)
                + std::to_string(wroteMaterial);
            bool selected = false;
            routeResult = Access::writeXRecordWithSourceCompatibility(
                *sourceIface, *record, &selected);
            compatibilityRoute = selected;
        }
    } writer;
    writer.cData = &writer.blank;
    writer.sourceIface = &sourceIface;
    writer.appId = *appId;
    writer.owner = *owner;
    writer.owner.m_entries = std::move(rootEntries);
    writer.extension = *extension;
    writer.extension.m_entries.erase(std::remove_if(
        writer.extension.m_entries.begin(), writer.extension.m_entries.end(),
        [&record](const auto& entry) {
            return entry.m_name != "ADVMATERIAL"
                || entry.m_handle != record->handle;
        }), writer.extension.m_entries.end());
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u)
        return fail("AC1024 ADVMATERIAL plot-style dictionary is not singular");
    writer.plot.m_entries.front().m_dxfHandleCode =
        DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();
    writer.material = *material;
    writer.record = &*record;

    const auto output = std::filesystem::temp_directory_path()
        / ("libdxfrw-p4-advmaterial-output-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    }} cleanup{output};
    dxfRW codec(output.string().c_str());
    sourceIface.dxfW = &codec;
    writer.dxfW = &codec;
    codec.setHandleRemap({{0x72u, 0xA072u}, {0x2C2u, 0xA2C2u},
                          {0x2C3u, 0xA2C3u}, {0x2C4u, 0xA2C4u}});
    codec.setRootDictEntries({{"ACAD_MATERIAL", "A072"},
                              {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group)))
        return fail("could not prepare selected ADVMATERIAL root");
    if (!codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not prepare selected ADVMATERIAL plot-style root");
    DRW_Class materialClass, plotClass, placeholderClass;
    if (!dxfRW::dxfClassForRecordName("MATERIAL", materialClass)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("could not prepare MATERIAL DXF class");
    materialClass.instanceCount = 1;
    plotClass.instanceCount = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({materialClass, plotClass, placeholderClass});

    const auto sameVariant = [](const DRW_Variant& left,
                                const DRW_Variant& right) {
        if (left.code() != right.code() || left.type() != right.type())
            return false;
        switch (left.type()) {
        case DRW_Variant::INTEGER: return left.i_val() == right.i_val();
        case DRW_Variant::DOUBLE: return left.d_val() == right.d_val();
        case DRW_Variant::STRING:
            return left.content.s != nullptr && right.content.s != nullptr
                && *left.content.s == *right.content.s;
        default: return false;
        }
    };
    const auto readback = [&]() {
        dx_iface reopened;
        dx_data actual;
        if (!reopened.fileImport(output.string(), &actual, false)) return false;
        const auto outOwner = findHandle(actual.dictionaries, 0xA072u);
        const auto outExtension = findHandle(actual.dictionaries, 0xA2C3u);
        const auto outMaterial = findHandle(actual.materials, 0xA2C2u);
        const auto outRecord = findHandle(actual.xRecords, 0xA2C4u);
        const auto outAppId = std::find_if(actual.appIds.begin(), actual.appIds.end(),
            [](const DRW_AppId& row) { return row.name == "ACAD"; });
        if (outOwner == actual.dictionaries.end()
            || outExtension == actual.dictionaries.end()
            || outMaterial == actual.materials.end()
            || outRecord == actual.xRecords.end()
            || outAppId == actual.appIds.end()
            || std::count_if(actual.appIds.begin(), actual.appIds.end(),
                [](const DRW_AppId& row) { return row.name == "ACAD"; }) != 1
            || outOwner->parentHandle != 0xCu
            || outOwner->m_entries.size() != 1u
            || outOwner->m_entries.front().m_name != selectedMaterialName
            || outOwner->m_entries.front().m_handle != 0xA2C2u
            || outMaterial->parentHandle != 0xA072u
            || outMaterial->xDictHandle != 0xA2C3u
            || outExtension->parentHandle != 0xA2C2u
            || outExtension->m_entries.size() != 1u
            || outExtension->m_entries.front().m_name != "ADVMATERIAL"
            || outExtension->m_entries.front().m_handle != 0xA2C4u
            || outRecord->parentHandle != 0xA2C3u
            || outRecord->reactorHandles.size() != 1u
            || outRecord->reactorHandles.front() != 0xA2C3u
            || outRecord->m_dataEntries.size() != record->m_dataEntries.size()
            || outRecord->m_values.size() != record->m_values.size())
            return false;
        if (!actual.hasNamedObjectDictionary
            || std::count_if(actual.namedObjectDictionary.m_entries.begin(),
                actual.namedObjectDictionary.m_entries.end(), [](const auto& entry) {
                    return entry.m_name == "ACAD_MATERIAL"
                        && entry.m_handle == 0xA072u;
                }) != 1)
            return false;
        for (std::size_t i = 0u; i < record->m_dataEntries.size(); ++i)
            if (!sameVariant(record->m_dataEntries[i], outRecord->m_dataEntries[i])
                || !sameVariant(record->m_values[i], outRecord->m_values[i]))
                return false;
        return true;
    };
    const auto write = [&](DRW::Version version, bool binary) {
        writer.routeResult = false;
        writer.compatibilityRoute = false;
        return codec.write(&writer, version, binary);
    };
    const std::string sentinel = "preserve ADVMATERIAL output destination\n";
    for (const bool binary : {false, true}) {
        const bool baselineWritten = write(DRW::AC1024, binary);
        const bool baselineReadback = baselineWritten && readback();
        if (!baselineWritten || !writer.routeResult
            || !writer.compatibilityRoute || !baselineReadback) {
            std::cerr << "ADVMATERIAL baseline: write=" << baselineWritten
                      << " route-result=" << writer.routeResult
                      << " compatibility-route=" << writer.compatibilityRoute
                      << " readback=" << baselineReadback
                      << " dependency-results=" << writer.dependencyResults
                      << " binary=" << binary << '\n';
            const auto diagnostic = codec.getLastDiagnostic();
            std::cerr << "ADVMATERIAL diagnostic: error="
                      << static_cast<int>(codec.getError())
                      << " code=" << diagnostic.code
                      << " message=" << diagnostic.message << '\n';
            return fail("selected ADVMATERIAL baseline/readback failed");
        }

        const auto refuseAndRetry = [&](const std::function<void()>& corrupt,
                                        const std::function<void()>& restore,
                                        DRW::Version version = DRW::AC1024) {
            {
                std::ofstream file(output, std::ios::binary | std::ios::trunc);
                if (!(file << sentinel)) return false;
            }
            corrupt();
            const bool saved = write(version, binary);
            restore();
            std::ifstream file(output, std::ios::binary);
            const std::string preserved(std::istreambuf_iterator<char>(file), {});
            file.close();
            return !saved && preserved == sentinel
                && Access::qualifiedAerialAdvMaterial(sourceIface, source)
                && write(DRW::AC1024, binary) && writer.routeResult
                && writer.compatibilityRoute && readback();
        };

        const auto savedValue = writer.record->m_dataEntries.front();
        if (!refuseAndRetry(
                [&] { writer.record->m_dataEntries.front() = DRW_Variant(460, 99.0); },
                [&] { writer.record->m_dataEntries.front() = savedValue; }))
            return fail("ADVMATERIAL typed-value refusal/preserve/retry failed");

        auto& raw = Access::aerialAdvMaterialRawFirstByte(source);
        const auto savedByte = raw;
        if (!refuseAndRetry([&] { raw ^= 0x80u; }, [&] { raw = savedByte; }))
            return fail("ADVMATERIAL source-frame refusal/preserve/retry failed");

        if (!refuseAndRetry([] {}, [] {}, DRW::AC1027))
            return fail("ADVMATERIAL wrong-version refusal/preserve/retry failed");
    }
    std::cerr << "AC1024 ADVMATERIAL 0x2C4 passed refusal/retry and readback in ASCII and Binary.\n";
    return true;
}
