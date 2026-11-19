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

bool physicalAc1024SelectedFiveAdvMaterialWriter(
        const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface sourceIface;
    dx_data source;
    if (!sourceIface.fileImport(sourcePath.string(), &source, false))
        return fail("AC1024 ADVMATERIAL source import failed");

    const auto findHandle = [](auto& rows, std::uint32_t handle) {
        return std::find_if(rows.begin(), rows.end(), [handle](const auto& row) {
            return row.handle == handle;
        });
    };
    const auto& allProfiles = dx_ac1024_aerial_advmaterial_wire::profiles;
    std::vector<const dx_ac1024_aerial_advmaterial_wire::Profile*> profiles;
    std::vector<DRW_Material*> materials;
    std::vector<DRW_Dictionary> extensions;
    std::vector<DRW_XRecord*> records;
    std::vector<DRW_Dictionary::Entry> rootEntries;
    std::map<std::uint32_t, std::uint32_t> remap{{0x72u, 0xA072u}};
    auto owner = findHandle(source.dictionaries, 0x72u);
    auto group = findHandle(source.dictionaries, 0xDu);
    const auto appId = findHandle(source.appIds, 0x12u);
    if (owner == source.dictionaries.end() || group == source.dictionaries.end()
        || appId == source.appIds.end() || appId->name != "ACAD"
        || source.dictionaryWithDefaults.size() != 1u
        || source.placeholders.size() != 1u)
        return fail("AC1024 selected ADVMATERIAL dependencies are missing");

    for (std::size_t index = 1u; index < allProfiles.size(); ++index) {
        const auto& profile = allProfiles[index];
        if (!Access::qualifiedAerialAdvMaterial(sourceIface, source, profile))
            return fail("one of the five ADVMATERIAL source profiles is unqualified");
        auto material = findHandle(source.materials, profile.materialHandle);
        auto extension = findHandle(source.dictionaries, profile.extensionHandle);
        auto record = findHandle(source.xRecords, profile.recordHandle);
        if (material == source.materials.end()
            || extension == source.dictionaries.end()
            || record == source.xRecords.end()
            || material->parentHandle != owner->handle
            || material->xDictHandle != profile.extensionHandle
            || extension->parentHandle != profile.materialHandle
            || record->parentHandle != profile.extensionHandle
            || record->m_dataEntries.size() != 10u
            || record->m_values.size() != 10u)
            return fail("an ADVMATERIAL source graph differs from its profile");
        const auto ownerEntries = std::count_if(owner->m_entries.begin(),
            owner->m_entries.end(), [&profile](const auto& entry) {
                return entry.m_handle == profile.materialHandle;
            });
        const auto extensionEntries = std::count_if(extension->m_entries.begin(),
            extension->m_entries.end(), [&profile](const auto& entry) {
                return entry.m_name == "ADVMATERIAL"
                    && entry.m_handle == profile.recordHandle;
            });
        if (ownerEntries != 1 || extensionEntries != 1)
            return fail("an ADVMATERIAL source dictionary edge is ambiguous");
        std::copy_if(owner->m_entries.begin(), owner->m_entries.end(),
            std::back_inserter(rootEntries), [&profile](const auto& entry) {
                return entry.m_handle == profile.materialHandle;
            });
        DRW_Dictionary selectedExtension = *extension;
        selectedExtension.m_entries.erase(std::remove_if(
            selectedExtension.m_entries.begin(), selectedExtension.m_entries.end(),
            [&profile](const auto& entry) {
                return entry.m_name != "ADVMATERIAL"
                    || entry.m_handle != profile.recordHandle;
            }), selectedExtension.m_entries.end());
        profiles.push_back(&profile);
        materials.push_back(&*material);
        extensions.push_back(std::move(selectedExtension));
        records.push_back(&*record);
        remap.emplace(profile.materialHandle, 0xA000u + profile.materialHandle);
        remap.emplace(profile.extensionHandle, 0xA000u + profile.extensionHandle);
        remap.emplace(profile.recordHandle, 0xA000u + profile.recordHandle);
    }
    if (profiles.size() != 5u || rootEntries.size() != 5u)
        return fail("five-profile ADVMATERIAL output projection is incomplete");

    class SelectedWriter final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_AppId appId;
        DRW_Dictionary owner;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        std::vector<DRW_Dictionary> extensions;
        std::vector<DRW_Material*> materials;
        std::vector<DRW_XRecord*> records;
        dx_iface* sourceIface{nullptr};
        bool dependenciesOk{false};
        bool allRoutes{false};
        void writeAppId() override { (void)dxfW->writeAppId(&appId); }
        bool finalizeDxfWrite() override { return true; }
        void writeObjects() override {
            dependenciesOk = dxfW->writeDictionaryWithDefault(&plot)
                && dxfW->writeAcDbPlaceholder(&placeholder)
                && dxfW->writeDictionary(&owner);
            for (auto& extension : extensions)
                dependenciesOk = dxfW->writeDictionary(&extension)
                    && dependenciesOk;
            for (auto* material : materials)
                dependenciesOk = dxfW->writeMaterial(material)
                    && dependenciesOk;
            allRoutes = true;
            for (auto* record : records) {
                bool selected = false;
                const bool written = Access::writeXRecordWithSourceCompatibility(
                    *sourceIface, *record, &selected);
                allRoutes = allRoutes && selected;
                dependenciesOk = dependenciesOk && written;
            }
        }
    } writer;
    writer.cData = &writer.blank;
    writer.sourceIface = &sourceIface;
    writer.appId = *appId;
    writer.owner = *owner;
    writer.owner.m_entries = rootEntries;
    writer.extensions = extensions;
    writer.materials = materials;
    writer.records = records;
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u)
        return fail("selected five-profile plot-style dictionary is not singular");
    writer.plot.m_entries.front().m_dxfHandleCode =
        DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();

    const auto output = std::filesystem::temp_directory_path()
        / ("libdxfrw-p4-five-advmaterial-output-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    }} cleanup{output};
    dxfRW codec(output.string().c_str());
    sourceIface.dxfW = &codec;
    writer.dxfW = &codec;
    codec.setHandleRemap(remap);
    codec.setRootDictEntries({{"ACAD_MATERIAL", "A072"},
                              {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not prepare five-profile ADVMATERIAL roots");
    DRW_Class materialClass, plotClass, placeholderClass;
    if (!dxfRW::dxfClassForRecordName("MATERIAL", materialClass)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("could not prepare five-profile MATERIAL classes");
    materialClass.instanceCount = static_cast<int>(materials.size());
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
    const auto allQualified = [&]() {
        return std::all_of(profiles.begin(), profiles.end(),
            [&](const auto* profile) {
                return Access::qualifiedAerialAdvMaterial(
                    sourceIface, source, *profile);
            });
    };
    const auto readback = [&]() {
        dx_iface reopened;
        dx_data actual;
        if (!reopened.fileImport(output.string(), &actual, false)
            || !actual.hasNamedObjectDictionary
            || std::count_if(actual.namedObjectDictionary.m_entries.begin(),
                actual.namedObjectDictionary.m_entries.end(), [](const auto& entry) {
                    return entry.m_name == "ACAD_MATERIAL"
                        && entry.m_handle == 0xA072u;
                }) != 1
            || std::count_if(actual.appIds.begin(), actual.appIds.end(),
                [](const DRW_AppId& row) { return row.name == "ACAD"; }) != 1)
            return false;
        const auto outOwner = findHandle(actual.dictionaries, 0xA072u);
        if (outOwner == actual.dictionaries.end()
            || outOwner->parentHandle != 0xCu
            || outOwner->m_entries.size() != profiles.size())
            return false;
        for (std::size_t index = 0u; index < profiles.size(); ++index) {
            const auto& profile = *profiles[index];
            const std::uint32_t materialOut = 0xA000u + profile.materialHandle;
            const std::uint32_t extensionOut = 0xA000u + profile.extensionHandle;
            const std::uint32_t recordOut = 0xA000u + profile.recordHandle;
            const auto outMaterial = findHandle(actual.materials, materialOut);
            const auto outExtension = findHandle(actual.dictionaries, extensionOut);
            const auto outRecord = findHandle(actual.xRecords, recordOut);
            const auto& expectedMaterial = *materials[index];
            const auto& expectedRecord = *records[index];
            const auto rootEdge = std::count_if(outOwner->m_entries.begin(),
                outOwner->m_entries.end(), [&](const auto& entry) {
                    return entry.m_name == rootEntries[index].m_name
                        && entry.m_handle == materialOut;
                });
            if (outMaterial == actual.materials.end()
                || outExtension == actual.dictionaries.end()
                || outRecord == actual.xRecords.end()
                || rootEdge != 1 || outMaterial->parentHandle != 0xA072u
                || outMaterial->xDictHandle != extensionOut
                || outMaterial->m_name != expectedMaterial.m_name
                || outExtension->parentHandle != materialOut
                || outExtension->m_entries.size() != 1u
                || outExtension->m_entries.front().m_name != "ADVMATERIAL"
                || outExtension->m_entries.front().m_handle != recordOut
                || outRecord->parentHandle != extensionOut
                || outRecord->reactorHandles.size() != 1u
                || outRecord->reactorHandles.front() != extensionOut
                || outRecord->m_dataEntries.size() != 10u
                || outRecord->m_values.size() != 10u)
                return false;
            for (std::size_t value = 0u;
                 value < expectedRecord.m_dataEntries.size(); ++value)
                if (!sameVariant(expectedRecord.m_dataEntries[value],
                                 outRecord->m_dataEntries[value])
                    || !sameVariant(expectedRecord.m_values[value],
                                    outRecord->m_values[value]))
                    return false;
        }
        return true;
    };
    const auto write = [&](DRW::Version version, bool binary) {
        writer.dependenciesOk = false;
        writer.allRoutes = false;
        return codec.write(&writer, version, binary);
    };
    const std::string sentinel = "preserve five-profile ADVMATERIAL destination\n";
    for (const bool binary : {false, true}) {
        if (!write(DRW::AC1024, binary) || !writer.dependenciesOk
            || !writer.allRoutes || !readback())
            return fail("five-profile ADVMATERIAL baseline/readback failed");
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
            return !saved && preserved == sentinel && allQualified()
                && write(DRW::AC1024, binary) && writer.dependenciesOk
                && writer.allRoutes && readback();
        };
        const std::size_t valueProfile = 2u;
        const auto savedValue = records[valueProfile]->m_dataEntries.front();
        if (!refuseAndRetry(
                [&] { records[valueProfile]->m_dataEntries.front()
                    = DRW_Variant(460, 99.0); },
                [&] { records[valueProfile]->m_dataEntries.front() = savedValue; }))
            return fail("five-profile ADVMATERIAL typed-value refusal/retry failed");

        auto& raw = Access::aerialAdvMaterialRawFrame(
            source, profiles.back()->recordHandle).m_rawBytes.front();
        const auto savedByte = raw;
        if (!refuseAndRetry([&] { raw ^= 0x80u; }, [&] { raw = savedByte; }))
            return fail("five-profile ADVMATERIAL source-frame refusal/retry failed");

        if (!refuseAndRetry([] {}, [] {}, DRW::AC1027))
            return fail("five-profile ADVMATERIAL wrong-version refusal/retry failed");
    }
    std::cerr << "AC1024 five-profile ADVMATERIAL batch passed refusal/retry and readback in ASCII and Binary.\n";
    return true;
}

bool physicalAc1024SelectedSixFbxAssetWriter(
        const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface sourceIface;
    dx_data source;
    if (!sourceIface.fileImport(sourcePath.string(), &source, false)
        || !Access::aerialFbxAssetSourceControls(sourceIface, source))
        return fail("AC1024 FBXASSET source profiles are not qualified");

    const auto findHandle = [](auto& rows, std::uint32_t handle) {
        return std::find_if(rows.begin(), rows.end(), [handle](const auto& row) {
            return row.handle == handle;
        });
    };
    const auto& profiles = dx_ac1024_aerial_fbxasset_wire::profiles;
    auto owner = findHandle(source.dictionaries, 0x72u);
    auto group = findHandle(source.dictionaries, 0xDu);
    auto appId = findHandle(source.appIds, 0x12u);
    if (owner == source.dictionaries.end() || group == source.dictionaries.end()
        || appId == source.appIds.end() || appId->name != "ACAD"
        || source.dictionaryWithDefaults.size() != 1u
        || source.placeholders.size() != 1u)
        return fail("AC1024 FBXASSET selected dependencies are missing");

    std::vector<DRW_Dictionary::Entry> rootEntries;
    std::vector<DRW_Material*> materials;
    std::vector<DRW_Dictionary> extensions;
    std::vector<DRW_XRecord*> records;
    std::map<std::uint32_t, std::uint32_t> remap{{0x72u, 0xA072u}};
    for (const auto& profile : profiles) {
        auto material = findHandle(source.materials, profile.materialHandle);
        auto extension = findHandle(source.dictionaries, profile.extensionHandle);
        auto record = findHandle(source.xRecords, profile.recordHandle);
        if (material == source.materials.end()
            || extension == source.dictionaries.end()
            || record == source.xRecords.end()
            || material->parentHandle != owner->handle
            || material->xDictHandle != profile.extensionHandle
            || extension->parentHandle != profile.materialHandle
            || record->parentHandle != profile.extensionHandle
            || record->m_dataEntries.size() != profile.entryCount
            || record->m_values.size() != profile.entryCount
            || material->extData.empty())
            return fail("an FBXASSET selected source graph differs from its profile");
        const auto rootEdge = std::count_if(owner->m_entries.begin(),
            owner->m_entries.end(), [&profile](const auto& entry) {
                return entry.m_handle == profile.materialHandle;
            });
        const auto fbxEdge = std::count_if(extension->m_entries.begin(),
            extension->m_entries.end(), [&profile](const auto& entry) {
                return entry.m_name == "FBXASSET"
                    && entry.m_handle == profile.recordHandle;
            });
        if (rootEdge != 1 || fbxEdge != 1)
            return fail("an FBXASSET source dictionary edge is ambiguous");
        std::copy_if(owner->m_entries.begin(), owner->m_entries.end(),
            std::back_inserter(rootEntries), [&profile](const auto& entry) {
                return entry.m_handle == profile.materialHandle;
            });
        materials.push_back(&*material);
        DRW_Dictionary selectedExtension = *extension;
        selectedExtension.m_entries.erase(std::remove_if(
            selectedExtension.m_entries.begin(), selectedExtension.m_entries.end(),
            [&profile](const auto& entry) {
                return entry.m_name != "FBXASSET"
                    || entry.m_handle != profile.recordHandle;
            }), selectedExtension.m_entries.end());
        extensions.push_back(std::move(selectedExtension));
        records.push_back(&*record);
        remap.emplace(profile.materialHandle, 0xA000u + profile.materialHandle);
        remap.emplace(profile.extensionHandle, 0xA000u + profile.extensionHandle);
        remap.emplace(profile.recordHandle, 0xA000u + profile.recordHandle);
    }
    if (rootEntries.size() != profiles.size())
        return fail("six-profile FBXASSET root projection is incomplete");

    class SelectedWriter final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_AppId appId;
        DRW_Dictionary owner;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        std::vector<DRW_Material*> materials;
        std::vector<DRW_Dictionary> extensions;
        std::vector<DRW_XRecord*> records;
        bool dependenciesOk{false};
        void writeAppId() override { (void)dxfW->writeAppId(&appId); }
        bool finalizeDxfWrite() override { return true; }
        void writeObjects() override {
            dependenciesOk = dxfW->writeDictionaryWithDefault(&plot)
                && dxfW->writeAcDbPlaceholder(&placeholder)
                && dxfW->writeDictionary(&owner);
            for (auto& extension : extensions)
                dependenciesOk = dxfW->writeDictionary(&extension)
                    && dependenciesOk;
            for (auto* material : materials)
                dependenciesOk = dxfW->writeMaterial(material)
                    && dependenciesOk;
            for (auto* record : records)
                dependenciesOk = dxfW->writeXRecord(record)
                    && dependenciesOk;
        }
    } writer;
    writer.cData = &writer.blank;
    writer.appId = *appId;
    writer.owner = *owner;
    writer.owner.m_entries = rootEntries;
    writer.materials = materials;
    writer.extensions = extensions;
    writer.records = records;
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u)
        return fail("selected FBXASSET plot-style dictionary is not singular");
    writer.plot.m_entries.front().m_dxfHandleCode =
        DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();

    const auto output = std::filesystem::temp_directory_path()
        / ("libdxfrw-p4-six-fbxasset-output-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    }} cleanup{output};
    dxfRW codec(output.string().c_str());
    sourceIface.dxfW = &codec;
    writer.dxfW = &codec;
    codec.setDxfWriteRecordInventoryEnabled(true);
    codec.setHandleRemap(remap);
    codec.setRootDictEntries({{"ACAD_MATERIAL", "A072"},
                              {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not prepare six-profile FBXASSET roots");
    DRW_Class materialClass, plotClass, placeholderClass;
    if (!dxfRW::dxfClassForRecordName("MATERIAL", materialClass)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("could not prepare six-profile FBXASSET classes");
    materialClass.instanceCount = static_cast<int>(materials.size());
    plotClass.instanceCount = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({materialClass, plotClass, placeholderClass});

    const auto sameVariant = [](const DRW_Variant& left,
                                const DRW_Variant& right) {
        if (left.code() != right.code() || left.type() != right.type())
            return false;
        switch (left.type()) {
        case DRW_Variant::INTEGER: return left.i_val() == right.i_val();
        case DRW_Variant::INTEGER64: return left.i64_val() == right.i64_val();
        case DRW_Variant::DOUBLE: return left.d_val() == right.d_val();
        case DRW_Variant::STRING:
            return left.content.s != nullptr && right.content.s != nullptr
                && *left.content.s == *right.content.s;
        case DRW_Variant::BINARY:
            return left.binary() != nullptr && right.binary() != nullptr
                && *left.binary() == *right.binary();
        case DRW_Variant::COORD: {
            const auto* a = left.coord();
            const auto* b = right.coord();
            return a != nullptr && b != nullptr && a->x == b->x
                && a->y == b->y && a->z == b->z;
        }
        default: return false;
        }
    };
    const auto allQualified = [&]() {
        return Access::qualifiedAerialFbxAssets(sourceIface, source);
    };
    const auto readback = [&]() {
        const auto report = [](const char* reason) {
            std::cerr << "FBXASSET readback: " << reason << '\n';
            return false;
        };
        dx_iface reopened;
        dx_data actual;
        if (!reopened.fileImport(output.string(), &actual, false)
            || !actual.hasNamedObjectDictionary
            || std::count_if(actual.namedObjectDictionary.m_entries.begin(),
                actual.namedObjectDictionary.m_entries.end(), [](const auto& entry) {
                    return entry.m_name == "ACAD_MATERIAL"
                        && entry.m_handle == 0xA072u;
                }) != 1
            || std::count_if(actual.appIds.begin(), actual.appIds.end(),
                [](const DRW_AppId& row) { return row.name == "ACAD"; }) != 1)
            return report("NOD or ACAD APPID");
        const auto outOwner = findHandle(actual.dictionaries, 0xA072u);
        if (outOwner == actual.dictionaries.end()
            || outOwner->parentHandle != 0xCu
            || outOwner->m_entries.size() != profiles.size())
            return report("selected MATERIAL owner dictionary");
        for (std::size_t index = 0u; index < profiles.size(); ++index) {
            const auto& profile = profiles[index];
            const auto materialOut = 0xA000u + profile.materialHandle;
            const auto extensionOut = 0xA000u + profile.extensionHandle;
            const auto recordOut = 0xA000u + profile.recordHandle;
            const auto outMaterial = findHandle(actual.materials, materialOut);
            const auto outExtension = findHandle(actual.dictionaries, extensionOut);
            const auto outRecord = findHandle(actual.xRecords, recordOut);
            const auto& expectedMaterial = *materials[index];
            const auto& expectedRecord = *records[index];
            const auto rootEdge = std::count_if(outOwner->m_entries.begin(),
                outOwner->m_entries.end(), [&](const auto& entry) {
                    return entry.m_name == rootEntries[index].m_name
                        && entry.m_handle == materialOut;
                });
            if (outMaterial == actual.materials.end()
                || outExtension == actual.dictionaries.end()
                || outRecord == actual.xRecords.end()
                || rootEdge != 1 || outMaterial->parentHandle != 0xA072u
                || outMaterial->xDictHandle != extensionOut
                || outMaterial->m_name != expectedMaterial.m_name
                || outMaterial->extData.size() != expectedMaterial.extData.size()
                || outExtension->parentHandle != materialOut
                || outExtension->m_entries.size() != 1u
                || outExtension->m_entries.front().m_name != "FBXASSET"
                || outExtension->m_entries.front().m_handle != recordOut
                || outRecord->parentHandle != extensionOut
                || outRecord->reactorHandles.size() != 1u
                || outRecord->reactorHandles.front() != extensionOut
                || outRecord->m_cloning != expectedRecord.m_cloning
                || outRecord->m_dataEntries.size() != expectedRecord.m_dataEntries.size()
                || outRecord->m_values.size() != expectedRecord.m_values.size())
            {
                std::cerr << "FBXASSET readback profile=" << index
                          << " material=" << (outMaterial != actual.materials.end())
                          << " extension=" << (outExtension != actual.dictionaries.end())
                          << " record=" << (outRecord != actual.xRecords.end())
                          << " rootEdge=" << rootEdge;
                if (outMaterial != actual.materials.end())
                    std::cerr << " parent=" << outMaterial->parentHandle
                              << " xdict=" << outMaterial->xDictHandle
                              << " eed=" << outMaterial->extData.size();
                if (outExtension != actual.dictionaries.end())
                    std::cerr << " extParent=" << outExtension->parentHandle
                              << " extEntries=" << outExtension->m_entries.size();
                if (outRecord != actual.xRecords.end())
                    std::cerr << " recParent=" << outRecord->parentHandle
                              << " reactors=" << outRecord->reactorHandles.size()
                              << " data=" << outRecord->m_dataEntries.size()
                              << " values=" << outRecord->m_values.size();
                std::cerr << '\n';
                return false;
            }
            for (std::size_t value = 0u;
                 value < expectedMaterial.extData.size(); ++value)
                if (expectedMaterial.extData[value] == nullptr
                    || outMaterial->extData[value] == nullptr
                    || !sameVariant(*expectedMaterial.extData[value],
                                    *outMaterial->extData[value]))
                    return report("MATERIAL EED value");
            for (std::size_t value = 0u;
                 value < expectedRecord.m_dataEntries.size(); ++value)
                if (!sameVariant(expectedRecord.m_dataEntries[value],
                                 outRecord->m_dataEntries[value])
                    || !sameVariant(expectedRecord.m_values[value],
                                    outRecord->m_values[value]))
                    return report("XRECORD value");
        }
        return true;
    };
    const auto write = [&](DRW::Version version, bool binary) {
        writer.dependenciesOk = false;
        if (version != DRW::AC1024 || !allQualified())
            return false;
        return codec.write(&writer, version, binary)
            && writer.dependenciesOk;
    };
    const std::string sentinel = "preserve six-profile FBXASSET destination\n";
    for (const bool binary : {false, true}) {
        const bool wrote = write(DRW::AC1024, binary);
        const bool reopened = wrote && readback();
        if (!wrote || !reopened) {
            std::cerr << "six-profile FBXASSET baseline binary=" << binary
                      << " wrote=" << wrote << " dependencies="
                      << writer.dependenciesOk << " readback=" << reopened << '\n';
            return fail("six-profile FBXASSET baseline/readback failed");
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
            return !saved && preserved == sentinel && allQualified()
                && write(DRW::AC1024, binary) && readback();
        };
        const std::size_t valueProfile = 3u;
        const auto savedValue = records[valueProfile]->m_dataEntries.front();
        if (!refuseAndRetry(
                [&] { records[valueProfile]->m_dataEntries.front()
                    = DRW_Variant(70, records[valueProfile]->m_dataEntries.front().i_val() + 1); },
                [&] { records[valueProfile]->m_dataEntries.front() = savedValue; }))
            return fail("six-profile FBXASSET typed-value refusal/retry failed");

        auto& raw = Access::aerialAdvMaterialRawFrame(
            source, profiles.back().recordHandle).m_rawBytes.front();
        const auto savedByte = raw;
        if (!refuseAndRetry([&] { raw ^= 0x80u; }, [&] { raw = savedByte; }))
            return fail("six-profile FBXASSET source-frame refusal/retry failed");

        if (!refuseAndRetry([] {}, [] {}, DRW::AC1027))
            return fail("six-profile FBXASSET wrong-version refusal/retry failed");
    }
    std::cerr << "AC1024 six-profile FBXASSET batch passed refusal/retry and readback in ASCII and Binary.\n";
    return true;
}

bool physicalAc1024SelectedThirtySixTileWriter(
        const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface sourceIface;
    dx_data source;
    if (!sourceIface.fileImport(sourcePath.string(), &source, false)
        || !Access::qualifiedAerialFbxAssets(sourceIface, source))
        return fail("AC1024 TILE source profiles are not qualified");

    const auto findHandle = [](auto& rows, std::uint32_t handle) {
        return std::find_if(rows.begin(), rows.end(), [handle](const auto& row) {
            return row.handle == handle;
        });
    };
    const auto& materialProfiles = dx_ac1024_aerial_tile_wire::materials;
    const auto& keys = dx_ac1024_aerial_tile_wire::keys;
    auto owner = findHandle(source.dictionaries, 0x72u);
    auto group = findHandle(source.dictionaries, 0xDu);
    auto appId = findHandle(source.appIds, 0x12u);
    if (owner == source.dictionaries.end() || group == source.dictionaries.end()
        || appId == source.appIds.end() || appId->name != "ACAD"
        || source.dictionaryWithDefaults.size() != 1u
        || source.placeholders.size() != 1u)
        return fail("AC1024 TILE selected dependencies are missing");

    std::vector<DRW_Dictionary::Entry> rootEntries;
    std::vector<DRW_Material*> materials;
    std::vector<DRW_Dictionary> extensions;
    std::vector<DRW_XRecord*> records;
    std::vector<dx_ac1024_aerial_tile_wire::TileProfile> tileProfiles;
    std::map<std::uint32_t, std::uint32_t> remap{{0x72u, 0xA072u}};
    for (std::size_t materialIndex = 0u;
         materialIndex < materialProfiles.size(); ++materialIndex) {
        const auto& profile = materialProfiles[materialIndex];
        auto material = findHandle(source.materials, profile.materialHandle);
        auto extension = findHandle(source.dictionaries, profile.extensionHandle);
        if (material == source.materials.end()
            || extension == source.dictionaries.end()
            || material->parentHandle != owner->handle
            || material->xDictHandle != profile.extensionHandle
            || extension->parentHandle != profile.materialHandle
            || material->extData.empty())
            return fail("a TILE MATERIAL source graph differs from its profile");
        const auto rootEdges = std::count_if(owner->m_entries.begin(),
            owner->m_entries.end(), [&profile](const auto& entry) {
                return entry.m_handle == profile.materialHandle;
            });
        if (rootEdges != 1)
            return fail("a TILE MATERIAL root edge is ambiguous");
        std::copy_if(owner->m_entries.begin(), owner->m_entries.end(),
            std::back_inserter(rootEntries), [&profile](const auto& entry) {
                return entry.m_handle == profile.materialHandle;
            });
        materials.push_back(&*material);

        DRW_Dictionary selectedExtension = *extension;
        selectedExtension.m_entries.erase(std::remove_if(
            selectedExtension.m_entries.begin(), selectedExtension.m_entries.end(),
            [&profile](const auto& entry) {
                for (std::size_t keyIndex = 0u;
                     keyIndex < dx_ac1024_aerial_tile_wire::keys.size(); ++keyIndex)
                    if (entry.m_name == dx_ac1024_aerial_tile_wire::keys[keyIndex]
                        && entry.m_handle == profile.tileHandles[keyIndex])
                        return false;
                return true;
            }), selectedExtension.m_entries.end());
        if (selectedExtension.m_entries.size() != keys.size())
            return fail("a selected TILE extension projection is incomplete");
        extensions.push_back(std::move(selectedExtension));
        remap.emplace(profile.materialHandle, 0xA000u + profile.materialHandle);
        remap.emplace(profile.extensionHandle, 0xA000u + profile.extensionHandle);

        for (std::size_t keyIndex = 0u; keyIndex < keys.size(); ++keyIndex) {
            const auto handle = profile.tileHandles[keyIndex];
            auto record = findHandle(source.xRecords, handle);
            dx_ac1024_aerial_tile_wire::TileProfile tile{};
            if (record == source.xRecords.end()
                || !dx_ac1024_aerial_tile_wire::profileForRecord(handle, tile)
                || tile.materialIndex != materialIndex
                || tile.keyIndex != keyIndex
                || record->parentHandle != profile.extensionHandle
                || record->m_dataEntries.size() != 2u
                || record->m_values.size() != 2u
                || record->m_dataEntries[0].code() != 270
                || record->m_dataEntries[0].i_val() != 1
                || record->m_dataEntries[1].code() != 271
                || record->m_dataEntries[1].i_val() != 1)
                return fail("a TILE XRECORD differs from its qualified profile");
            const auto entry = std::count_if(extension->m_entries.begin(),
                extension->m_entries.end(), [&tile](const auto& row) {
                    return row.m_name == keys[tile.keyIndex]
                        && row.m_handle == tile.recordHandle;
                });
            if (entry != 1)
                return fail("a TILE extension edge is ambiguous");
            records.push_back(&*record);
            tileProfiles.push_back(tile);
            remap.emplace(handle, 0xA000u + handle);
        }
    }
    if (rootEntries.size() != materialProfiles.size()
        || records.size() != 36u || tileProfiles.size() != 36u)
        return fail("36-TILE selected root projection is incomplete");

    class SelectedWriter final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_AppId appId;
        DRW_Dictionary owner;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        std::vector<DRW_Material*> materials;
        std::vector<DRW_Dictionary> extensions;
        std::vector<DRW_XRecord*> records;
        bool dependenciesOk{false};
        void writeAppId() override { (void)dxfW->writeAppId(&appId); }
        bool finalizeDxfWrite() override { return true; }
        void writeObjects() override {
            dependenciesOk = dxfW->writeDictionaryWithDefault(&plot)
                && dxfW->writeAcDbPlaceholder(&placeholder)
                && dxfW->writeDictionary(&owner);
            for (auto& extension : extensions)
                dependenciesOk = dxfW->writeDictionary(&extension)
                    && dependenciesOk;
            for (auto* material : materials)
                dependenciesOk = dxfW->writeMaterial(material)
                    && dependenciesOk;
            for (auto* record : records)
                dependenciesOk = dxfW->writeXRecord(record)
                    && dependenciesOk;
        }
    } writer;
    writer.cData = &writer.blank;
    writer.appId = *appId;
    writer.owner = *owner;
    writer.owner.m_entries = rootEntries;
    writer.materials = materials;
    writer.extensions = extensions;
    writer.records = records;
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u)
        return fail("selected TILE plot-style dictionary is not singular");
    writer.plot.m_entries.front().m_dxfHandleCode =
        DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();

    const auto output = std::filesystem::temp_directory_path()
        / ("libdxfrw-p4-thirtysix-tile-output-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    }} cleanup{output};
    dxfRW codec(output.string().c_str());
    sourceIface.dxfW = &codec;
    writer.dxfW = &codec;
    codec.setDxfWriteRecordInventoryEnabled(true);
    codec.setHandleRemap(remap);
    codec.setRootDictEntries({{"ACAD_MATERIAL", "A072"},
                              {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not prepare 36-TILE roots");
    DRW_Class materialClass, plotClass, placeholderClass;
    if (!dxfRW::dxfClassForRecordName("MATERIAL", materialClass)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("could not prepare 36-TILE classes");
    materialClass.instanceCount = static_cast<int>(materials.size());
    plotClass.instanceCount = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({materialClass, plotClass, placeholderClass});

    const auto sameVariant = [](const DRW_Variant& left,
                                const DRW_Variant& right) {
        if (left.code() != right.code() || left.type() != right.type())
            return false;
        if (left.type() == DRW_Variant::INTEGER)
            return left.i_val() == right.i_val();
        if (left.type() == DRW_Variant::STRING)
            return left.content.s != nullptr && right.content.s != nullptr
                && *left.content.s == *right.content.s;
        return false;
    };
    const auto allQualified = [&]() {
        return Access::qualifiedAerialFbxAssets(sourceIface, source);
    };
    const auto readback = [&]() {
        dx_iface reopened;
        dx_data actual;
        if (!reopened.fileImport(output.string(), &actual, false)
            || !actual.hasNamedObjectDictionary
            || std::count_if(actual.namedObjectDictionary.m_entries.begin(),
                actual.namedObjectDictionary.m_entries.end(), [](const auto& entry) {
                    return entry.m_name == "ACAD_MATERIAL"
                        && entry.m_handle == 0xA072u;
                }) != 1
            || std::count_if(actual.appIds.begin(), actual.appIds.end(),
                [](const DRW_AppId& row) { return row.name == "ACAD"; }) != 1)
            return false;
        const auto outOwner = findHandle(actual.dictionaries, 0xA072u);
        if (outOwner == actual.dictionaries.end()
            || outOwner->parentHandle != 0xCu
            || outOwner->m_entries.size() != materialProfiles.size())
            return false;
        for (std::size_t materialIndex = 0u;
             materialIndex < materialProfiles.size(); ++materialIndex) {
            const auto& profile = materialProfiles[materialIndex];
            const auto materialOut = 0xA000u + profile.materialHandle;
            const auto extensionOut = 0xA000u + profile.extensionHandle;
            const auto outMaterial = findHandle(actual.materials, materialOut);
            const auto outExtension = findHandle(actual.dictionaries, extensionOut);
            const auto& expectedMaterial = *materials[materialIndex];
            const auto rootEdge = std::count_if(outOwner->m_entries.begin(),
                outOwner->m_entries.end(), [&](const auto& entry) {
                    return entry.m_name == rootEntries[materialIndex].m_name
                        && entry.m_handle == materialOut;
                });
            if (outMaterial == actual.materials.end()
                || outExtension == actual.dictionaries.end()
                || rootEdge != 1 || outMaterial->parentHandle != 0xA072u
                || outMaterial->xDictHandle != extensionOut
                || outMaterial->m_name != expectedMaterial.m_name
                || outMaterial->extData.size() != expectedMaterial.extData.size()
                || outExtension->parentHandle != materialOut
                || outExtension->m_entries.size() != keys.size())
                return false;
            for (std::size_t value = 0u;
                 value < expectedMaterial.extData.size(); ++value)
                if (expectedMaterial.extData[value] == nullptr
                    || outMaterial->extData[value] == nullptr
                    || !sameVariant(*expectedMaterial.extData[value],
                                    *outMaterial->extData[value]))
                    return false;
            for (std::size_t keyIndex = 0u; keyIndex < keys.size(); ++keyIndex) {
                const auto recordOut = 0xA000u
                    + profile.tileHandles[keyIndex];
                const auto edge = std::count_if(
                    outExtension->m_entries.begin(), outExtension->m_entries.end(),
                    [&](const auto& entry) {
                        return entry.m_name == keys[keyIndex]
                            && entry.m_handle == recordOut;
                    });
                const auto outRecord = findHandle(actual.xRecords, recordOut);
                const auto& expectedRecord = *records[
                    materialIndex * keys.size() + keyIndex];
                if (edge != 1 || outRecord == actual.xRecords.end()
                    || outRecord->parentHandle != extensionOut
                    || outRecord->reactorHandles.size() != 1u
                    || outRecord->reactorHandles.front() != extensionOut
                    || outRecord->m_dataEntries.size() != 2u
                    || outRecord->m_values.size() != 2u
                    || !sameVariant(expectedRecord.m_dataEntries[0],
                                    outRecord->m_dataEntries[0])
                    || !sameVariant(expectedRecord.m_dataEntries[1],
                                    outRecord->m_dataEntries[1])
                    || !sameVariant(expectedRecord.m_values[0],
                                    outRecord->m_values[0])
                    || !sameVariant(expectedRecord.m_values[1],
                                    outRecord->m_values[1]))
                    return false;
            }
        }
        return true;
    };
    const auto write = [&](DRW::Version version, bool binary) {
        writer.dependenciesOk = false;
        if (version != DRW::AC1024 || !allQualified())
            return false;
        return codec.write(&writer, version, binary)
            && writer.dependenciesOk;
    };
    const std::string sentinel = "preserve 36-TILE destination\n";
    for (const bool binary : {false, true}) {
        if (!write(DRW::AC1024, binary) || !readback())
            return fail("36-TILE baseline/readback failed");
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
            return !saved && preserved == sentinel && allQualified()
                && write(DRW::AC1024, binary) && readback();
        };
        const std::size_t valueIndex = 17u;
        const auto savedValue = records[valueIndex]->m_dataEntries.front();
        if (!refuseAndRetry(
                [&] { records[valueIndex]->m_dataEntries.front()
                    = DRW_Variant(270, records[valueIndex]->m_dataEntries.front().i_val() + 1); },
                [&] { records[valueIndex]->m_dataEntries.front() = savedValue; }))
            return fail("36-TILE typed-value refusal/retry failed");

        auto& raw = Access::aerialAdvMaterialRawFrame(
            source, tileProfiles.back().recordHandle).m_rawBytes.front();
        const auto savedByte = raw;
        if (!refuseAndRetry([&] { raw ^= 0x80u; }, [&] { raw = savedByte; }))
            return fail("36-TILE source-frame refusal/retry failed");

        if (!refuseAndRetry([] {}, [] {}, DRW::AC1027))
            return fail("36-TILE wrong-version refusal/retry failed");
    }
    std::cerr << "AC1024 36-TILE batch passed refusal/retry and readback in ASCII and Binary.\n";
    return true;
}

bool physicalAc1024SelectedDiffuseBumpWriter(
        const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface sourceIface;
    dx_data source;
    if (!sourceIface.fileImport(sourcePath.string(), &source, false)
        || !Access::qualifiedAerialDiffuseBump(sourceIface, source))
        return fail("AC1024 DIFFUSE/BUMP source profiles are not qualified");
    const auto findHandle = [](auto& rows, std::uint32_t handle) {
        return std::find_if(rows.begin(), rows.end(), [handle](const auto& row) {
            return row.handle == handle;
        });
    };
    auto root = findHandle(source.dictionaries, 0x72u);
    auto group = findHandle(source.dictionaries, 0xDu);
    auto material = findHandle(source.materials, 0x2EAu);
    auto extension = findHandle(source.dictionaries, 0x2EBu);
    auto appId = findHandle(source.appIds, 0x12u);
    std::array<DRW_XRecord*, 2> records{};
    const auto& profiles = dx_ac1024_aerial_material_wide_wire::profiles;
    for (std::size_t index = 0u; index < profiles.size(); ++index) {
        auto record = findHandle(source.xRecords, profiles[index].recordHandle);
        if (record == source.xRecords.end())
            return fail("AC1024 DIFFUSE/BUMP XRECORD is missing");
        records[index] = &*record;
    }
    if (root == source.dictionaries.end() || group == source.dictionaries.end()
        || material == source.materials.end()
        || extension == source.dictionaries.end()
        || appId == source.appIds.end() || appId->name != "ACAD"
        || source.dictionaryWithDefaults.size() != 1u
        || source.placeholders.size() != 1u
        || material->parentHandle != root->handle
        || material->xDictHandle != extension->handle
        || material->extData.empty() || extension->parentHandle != material->handle)
        return fail("AC1024 DIFFUSE/BUMP output dependencies are missing");
    std::vector<DRW_Dictionary::Entry> rootEntries;
    std::copy_if(root->m_entries.begin(), root->m_entries.end(),
        std::back_inserter(rootEntries), [](const auto& entry) {
            return entry.m_handle == 0x2EAu;
        });
    if (rootEntries.size() != 1u)
        return fail("DIFFUSE/BUMP MATERIAL root entry is ambiguous");
    DRW_Dictionary selectedExtension = *extension;
    selectedExtension.m_entries.erase(std::remove_if(
        selectedExtension.m_entries.begin(), selectedExtension.m_entries.end(),
        [&](const auto& entry) {
            return !std::any_of(profiles.begin(), profiles.end(),
                [&](const auto& profile) {
                    return entry.m_name == profile.dictionaryKey
                        && entry.m_handle == profile.recordHandle;
                });
        }), selectedExtension.m_entries.end());
    if (selectedExtension.m_entries.size() != profiles.size())
        return fail("DIFFUSE/BUMP extension projection is incomplete");

    class SelectedWriter final : public LayerStateTestWriter {
    public:
        dx_data blank;
        DRW_AppId appId;
        DRW_Dictionary owner, extension;
        DRW_DictionaryWithDefault plot;
        DRW_AcDbPlaceholder placeholder;
        DRW_Material material;
        std::array<DRW_XRecord*, 2> records{};
        dx_iface* sourceIface{nullptr};
        bool dependenciesOk{false};
        bool routesOk{false};
        void writeAppId() override { (void)dxfW->writeAppId(&appId); }
        bool finalizeDxfWrite() override { return true; }
        void writeObjects() override {
            dependenciesOk = dxfW->writeDictionaryWithDefault(&plot)
                && dxfW->writeAcDbPlaceholder(&placeholder)
                && dxfW->writeDictionary(&owner)
                && dxfW->writeDictionary(&extension)
                && dxfW->writeMaterial(&material);
            routesOk = true;
            for (auto* record : records) {
                bool wideRoute = false;
                const bool written = Access::writeXRecordWithSourceCompatibility(
                    *sourceIface, *record, nullptr, &wideRoute);
                routesOk = routesOk && wideRoute;
                dependenciesOk = written && dependenciesOk;
            }
        }
    } writer;
    writer.cData = &writer.blank;
    writer.sourceIface = &sourceIface;
    writer.appId = *appId;
    writer.owner = *root;
    writer.owner.m_entries = rootEntries;
    writer.extension = std::move(selectedExtension);
    writer.material = *material;
    writer.records = records;
    writer.plot = source.dictionaryWithDefaults.front();
    if (writer.plot.m_entries.size() != 1u)
        return fail("DIFFUSE/BUMP plot-style dictionary is not singular");
    writer.plot.m_entries.front().m_dxfHandleCode =
        DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;
    writer.placeholder = source.placeholders.front();

    const auto output = std::filesystem::temp_directory_path()
        / ("libdxfrw-p4-diffuse-bump-output-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    }} cleanup{output};
    dxfRW codec(output.string().c_str());
    sourceIface.dxfW = &codec;
    writer.dxfW = &codec;
    codec.setDxfWriteRecordInventoryEnabled(true);
    codec.setHandleRemap({{0x72u, 0xA072u}, {0x2EAu, 0xA2EAu},
                          {0x2EBu, 0xA2EBu}, {0x2EEu, 0xA2EEu},
                          {0x2F0u, 0xA2F0u}});
    codec.setRootDictEntries({{"ACAD_MATERIAL", "A072"},
                              {"ACAD_PLOTSTYLENAME", "E"}});
    if (!codec.setGeneratedDictionarySourceHandles(0xCu, 0xDu)
        || !codec.setGeneratedDictionarySourceRecords(
            std::make_unique<DRW_Dictionary>(source.namedObjectDictionary),
            std::make_unique<DRW_Dictionary>(*group))
        || !codec.setDxfDefaultPlotStyleSourceHandles(0xEu, 0xFu))
        return fail("could not prepare DIFFUSE/BUMP output roots");
    DRW_Class materialClass, plotClass, placeholderClass;
    if (!dxfRW::dxfClassForRecordName("MATERIAL", materialClass)
        || !dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", plotClass)
        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass))
        return fail("could not prepare DIFFUSE/BUMP output classes");
    materialClass.instanceCount = plotClass.instanceCount
        = placeholderClass.instanceCount = 1;
    codec.setDxfClasses({materialClass, plotClass, placeholderClass});

    const auto sameVariant = [](const DRW_Variant& left,
                                const DRW_Variant& right) {
        if (left.code() != right.code() || left.type() != right.type())
            return false;
        switch (left.type()) {
        case DRW_Variant::STRING:
            return left.content.s != nullptr && right.content.s != nullptr
                && *left.content.s == *right.content.s;
        case DRW_Variant::INTEGER: return left.i_val() == right.i_val();
        case DRW_Variant::DOUBLE: return left.d_val() == right.d_val();
        default: return false;
        }
    };
    const auto allQualified = [&]() {
        return Access::qualifiedAerialDiffuseBump(sourceIface, source);
    };
    const auto readback = [&]() {
        dx_iface reopened;
        dx_data actual;
        if (!reopened.fileImport(output.string(), &actual, false)
            || !actual.hasNamedObjectDictionary
            || std::count_if(actual.namedObjectDictionary.m_entries.begin(),
                actual.namedObjectDictionary.m_entries.end(), [](const auto& entry) {
                    return entry.m_name == "ACAD_MATERIAL"
                        && entry.m_handle == 0xA072u;
                }) != 1
            || std::count_if(actual.appIds.begin(), actual.appIds.end(),
                [](const DRW_AppId& row) { return row.name == "ACAD"; }) != 1)
            return false;
        const auto outMaterial = findHandle(actual.materials, 0xA2EAu);
        const auto outExtension = findHandle(actual.dictionaries, 0xA2EBu);
        const auto outRoot = findHandle(actual.dictionaries, 0xA072u);
        if (outMaterial == actual.materials.end()
            || outExtension == actual.dictionaries.end()
            || outRoot == actual.dictionaries.end()
            || outMaterial->m_name != material->m_name
            || outMaterial->parentHandle != 0xA072u
            || outMaterial->xDictHandle != 0xA2EBu
            || outMaterial->extData.size() != material->extData.size()
            || outExtension->parentHandle != 0xA2EAu
            || outExtension->m_entries.size() != profiles.size())
            return false;
        for (std::size_t index = 0u; index < material->extData.size(); ++index)
            if (material->extData[index] == nullptr
                || outMaterial->extData[index] == nullptr
                || !sameVariant(*material->extData[index],
                                *outMaterial->extData[index]))
                return false;
        for (std::size_t index = 0u; index < profiles.size(); ++index) {
            const auto& profile = profiles[index];
            const auto recordHandle = 0xA000u + profile.recordHandle;
            const auto edge = std::count_if(outExtension->m_entries.begin(),
                outExtension->m_entries.end(), [&profile, recordHandle](const auto& row) {
                    return row.m_name == profile.dictionaryKey
                        && row.m_handle == recordHandle;
                });
            const auto outRecord = findHandle(actual.xRecords, recordHandle);
            const auto& expected = *records[index];
            if (edge != 1 || outRecord == actual.xRecords.end()
                || outRecord->parentHandle != 0xA2EBu
                || outRecord->reactorHandles.size() != 1u
                || outRecord->reactorHandles.front() != 0xA2EBu
                || outRecord->m_dataEntries.size() != expected.m_dataEntries.size()
                || outRecord->m_values.size() != expected.m_values.size())
                return false;
            for (std::size_t field = 0u; field < expected.m_dataEntries.size(); ++field)
                if (!sameVariant(expected.m_dataEntries[field],
                                 outRecord->m_dataEntries[field])
                    || !sameVariant(expected.m_values[field],
                                    outRecord->m_values[field]))
                    return false;
        }
        return true;
    };
    const auto write = [&](DRW::Version version, bool binary) {
        writer.dependenciesOk = writer.routesOk = false;
        return codec.write(&writer, version, binary)
            && writer.dependenciesOk && writer.routesOk;
    };
    const std::string sentinel = "preserve DIFFUSE/BUMP destination\n";
    for (const bool binary : {false, true}) {
        if (!write(DRW::AC1024, binary) || !readback())
            return fail("DIFFUSE/BUMP baseline/readback failed");
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
            return !saved && !writer.routesOk && preserved == sentinel
                && allQualified() && write(DRW::AC1024, binary) && readback();
        };
        for (std::size_t index = 0u; index < records.size(); ++index) {
            auto& record = *records[index];
            const auto field = std::find_if(record.m_dataEntries.begin(),
                record.m_dataEntries.end(), [](const auto& value) {
                    return value.code() == 420;
                });
            if (field == record.m_dataEntries.end())
                return fail("DIFFUSE/BUMP Int32 group 420 is missing");
            const auto savedValue = *field;
            if (!refuseAndRetry(
                    [field] {
                        *field = DRW_Variant(420, field->i_val() + 1);
                    },
                    [field, savedValue] { *field = savedValue; }))
                return fail("DIFFUSE/BUMP typed-value refusal/retry failed");
        }
        auto& raw = Access::aerialAdvMaterialRawFrame(
            source, profiles.back().recordHandle).m_rawBytes.front();
        const auto savedByte = raw;
        if (!refuseAndRetry([&] { raw ^= 0x80u; }, [&] { raw = savedByte; }))
            return fail("DIFFUSE/BUMP raw-frame refusal/retry failed");
        if (!refuseAndRetry([] {}, [] {}, DRW::AC1027))
            return fail("DIFFUSE/BUMP target-version refusal/retry failed");
    }
    std::cerr << "AC1024 DIFFUSE/BUMP profiles passed ASCII/Binary output, refusal and retry.\n";
    return true;
}
