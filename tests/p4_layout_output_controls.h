// SPDX-License-Identifier: GPL-2.0-or-later
bool physicalAc1024AerialLayoutSourceBinding(
        const std::filesystem::path& sourcePath) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dx_iface sourceReader;
    dx_data source;
    if (!sourceReader.fileImport(sourcePath.string(), &source, false)
        || source.sourceFormat != dx_source_format::DWG
        || source.sourceVersion != DRW::AC1024
        || source.layouts.size() != 3u
        || source.namedObjectDictionary.handle != 0x0Cu)
        return fail("AC1024 aerial LAYOUT source preconditions are absent");

    const auto layout = [&source](std::uint32_t handle,
                                  const char* name) {
        const auto found = std::find_if(source.layouts.begin(),
            source.layouts.end(), [handle, name](const DRW_Layout& value) {
                return value.handle == handle && value.name == name;
            });
        return found != source.layouts.end()
            && std::count_if(source.layouts.begin(), source.layouts.end(),
                [handle, name](const DRW_Layout& value) {
                    return value.handle == handle && value.name == name;
                }) == 1u;
    };
    if (!layout(0x22u, "Model") || !layout(0x59u, "Layout1")
        || !layout(0x5Eu, "Layout2"))
        return fail("AC1024 aerial LAYOUT identity inventory changed");

    const auto nodEntry = std::find_if(
        source.namedObjectDictionary.m_entries.begin(),
        source.namedObjectDictionary.m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_LAYOUT";
        });
    const DRW_Dictionary* layoutDictionary = nullptr;
    for (const DRW_Dictionary& dictionary : source.dictionaries) {
        if (nodEntry == source.namedObjectDictionary.m_entries.end()
            || dictionary.handle != nodEntry->m_handle)
            continue;
        if (layoutDictionary != nullptr)
            return fail("ACAD_LAYOUT dictionary handle is ambiguous");
        layoutDictionary = &dictionary;
    }
    if (nodEntry == source.namedObjectDictionary.m_entries.end()
        || nodEntry->m_handle != 0x1Au
        || layoutDictionary == nullptr
        || layoutDictionary->parentHandle != source.namedObjectDictionary.handle
        || layoutDictionary->m_entries.size() != 3u
        || !layoutDictionary->hasDwgCommonLinkTail())
        return fail("AC1024 aerial ACAD_LAYOUT owner or common-link profile changed");

    const auto model = std::find_if(source.layouts.begin(), source.layouts.end(),
        [](const DRW_Layout& value) { return value.handle == 0x22u; });
    DRW_Dictionary* thumbnailDictionary = nullptr;
    for (DRW_Dictionary& dictionary : source.dictionaries) {
        if (dictionary.handle != 0x301u)
            continue;
        if (thumbnailDictionary != nullptr)
            return fail("AC1024 aerial layout thumbnail dictionary is ambiguous");
        thumbnailDictionary = &dictionary;
    }
    DRW_XRecord* thumbnail = nullptr;
    for (DRW_XRecord& record : source.xRecords) {
        if (record.handle != 0x302u)
            continue;
        if (thumbnail != nullptr)
            return fail("AC1024 aerial layout thumbnail XRECORD is ambiguous");
        thumbnail = &record;
    }
    if (model == source.layouts.end() || model->xDictHandle != 0x301u
        || thumbnailDictionary == nullptr
        || thumbnailDictionary->parentHandle != model->handle
        || thumbnailDictionary->hardOwner != 1
        || thumbnailDictionary->m_entries.size() != 1u
        || thumbnailDictionary->m_entries.front().m_name
            != "ADSK_XREC_LAYOUTTHUMBNAIL"
        || thumbnailDictionary->m_entries.front().m_handle != 0x302u
        || thumbnailDictionary->m_entries.front().m_dxfHandleCode
            != DRW_Dictionary::Entry::DxfHandleCode::Unspecified
        || thumbnail == nullptr
        || thumbnail->parentHandle != thumbnailDictionary->handle
        || thumbnail->reactorHandles
            != std::vector<std::uint32_t>{thumbnailDictionary->handle}
        || thumbnail->m_rawDataValid
        || !thumbnail->m_handleValues.empty()
        || thumbnail->m_dataEntries.size() != 1073u
        || thumbnail->m_values.size() != 1073u)
        return fail("AC1024 aerial layout thumbnail extension graph changed");

    for (const DRW_Layout& value : source.layouts) {
        const auto entry = std::find_if(layoutDictionary->m_entries.begin(),
            layoutDictionary->m_entries.end(), [&value](
                const DRW_Dictionary::Entry& candidate) {
                    return candidate.m_name == value.name
                        && candidate.m_handle == value.handle;
                });
        const auto block = std::find_if(source.blockRecords.begin(),
            source.blockRecords.end(), [&value](
                const DRW_Block_Record& candidate) {
                    return candidate.handle
                        == value.paperSpaceBlockRecordHandle.ref;
                });
        if (value.parentHandle != layoutDictionary->handle
            || entry == layoutDictionary->m_entries.end()
            || block == source.blockRecords.end()
            || block->layoutHandle != value.handle)
            return fail("AC1024 aerial LAYOUT owner, dictionary edge or BLOCK_RECORD backlink changed");
    }

    if (!Access::prepareDwgLayoutSource(
            sourceReader, source, DRW::AC1024, true)
        || !Access::hasExactAc1024AerialLayoutSource(sourceReader)
        || !Access::hasStagedLayoutExtension(sourceReader)
        || !Access::dwgLayoutSourceUnchanged(sourceReader))
        return fail("exact AC1024 aerial LAYOUT and thumbnail graph was not source-bound");

    auto layout2 = std::find_if(source.layouts.begin(), source.layouts.end(),
        [](const DRW_Layout& value) { return value.handle == 0x5Eu; });
    if (layout2 == source.layouts.end())
        return fail("AC1024 aerial Layout2 disappeared after source capture");
    const std::string originalLayoutName = layout2->name;
    layout2->name = "mutated Layout2";
    const bool layoutMutationRejected = !Access::dwgLayoutSourceUnchanged(
        sourceReader);
    layout2->name = originalLayoutName;
    if (!layoutMutationRejected || !Access::dwgLayoutSourceUnchanged(sourceReader))
        return fail("LAYOUT source mutation was not rejected or restored retry failed");

    const std::string originalKey = thumbnailDictionary->m_entries.front().m_name;
    thumbnailDictionary->m_entries.front().m_name = "mutated thumbnail key";
    const bool extensionMutationRejected =
        !Access::dwgLayoutSourceUnchanged(sourceReader);
    thumbnailDictionary->m_entries.front().m_name = originalKey;
    if (!extensionMutationRejected
        || !Access::dwgLayoutSourceUnchanged(sourceReader))
        return fail("LAYOUT extension mutation was not rejected or restored retry failed");

    const DRW_Variant originalThumbnailValue = thumbnail->m_values.front();
    thumbnail->m_values.front().addString(1, "mutated thumbnail XRECORD value");
    const bool xrecordMutationRejected =
        !Access::dwgLayoutSourceUnchanged(sourceReader);
    thumbnail->m_values.front() = originalThumbnailValue;
    if (!xrecordMutationRejected
        || !Access::dwgLayoutSourceUnchanged(sourceReader))
        return fail("LAYOUT XRECORD value mutation was not rejected or restored retry failed");

    dx_iface wrongVersion;
    (void)Access::prepareDwgLayoutSource(
        wrongVersion, source, DRW::AC1021);
    if (Access::hasExactAc1024AerialLayoutSource(wrongVersion)
        || Access::hasStagedLayoutExtension(wrongVersion))
        return fail("AC1024 aerial LAYOUT graph was selected for AC1021 output");
    if (!Access::prepareDwgLayoutSource(
            sourceReader, source, DRW::AC1024)
        || !Access::hasExactAc1024AerialLayoutSource(sourceReader)
        || !Access::hasStagedLayoutExtension(sourceReader))
        return fail("AC1024 aerial LAYOUT graph did not recover after wrong-version refusal");

    for (const bool binary : {false, true}) {
        const auto nonce = std::chrono::steady_clock::now()
                               .time_since_epoch().count();
        const std::filesystem::path destinationPath =
            std::filesystem::temp_directory_path()
            / ("libdxfrw-ac1024-aerial-layout-source-"
               + std::to_string(nonce)
               + (binary ? "-binary.dxf" : "-ascii.dxf"));
        struct Cleanup {
            std::filesystem::path path;
            ~Cleanup() {
                std::error_code ignored;
                std::filesystem::remove(path, ignored);
            }
        } cleanup{destinationPath};
        const std::string sentinel = "preserve blocked aerial output\n";
        {
            std::ofstream output(destinationPath,
                std::ios::binary | std::ios::trunc);
            if (!output || !(output << sentinel))
                return fail("could not seed the aerial LAYOUT destination");
        }
        const bool exported = sourceReader.fileExport(
            destinationPath.string(), DRW::AC1024, binary, &source,
            false, true, dx_dxfSourceTextPolicy::RequireValidSource,
            DRW::DxfTextWritePolicy::RequireLossless,
            dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,
            dx_dwgReadCompletenessPolicy::AllowPartial);
        std::ifstream output(destinationPath, std::ios::binary);
        const std::string contents{
            std::istreambuf_iterator<char>{output}, {}};
        if (exported || contents != sentinel)
            return fail("the independent SUN graph blocker did not preserve the output destination");
    }
    return true;
}
