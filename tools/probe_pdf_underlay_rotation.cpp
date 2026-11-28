#include "dwg2dxf/dx_iface.h"
#include <cmath>
#include <iostream>
int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) return 2;
    dx_data source;
    dx_iface reader;
    if (!reader.fileImport(argv[1], &source, false) || source.sourceVersion != DRW::AC1024 || source.underlays.size() != 1) return 3;
    const dx_ifaceUnderlay* observed = source.underlays.front();
    if (!observed || observed->kind != DRW_Underlay::PDF || !observed->sourceDwgPdfRotationRadians || observed->sourceDwgPdfVersion != DRW::AC1024 || std::abs(observed->rotation - std::acos(-1.) / 4.) > 1e-12) return 4;
    dx_data selected;
    dx_iface author;
    author.cData = &selected;
    author.currentBlock = selected.mBlock;
    selected.sourceFormat = dx_source_format::DWG;
    selected.sourceVersion = DRW::AC1024;
    selected.hasNamedObjectDictionary = true;
    selected.namedObjectDictionary.handle = 0xC700;
    selected.namedObjectDictionary.m_entries = {{"ACAD_PDFDEFINITIONS", 0xC701}};
    DRW_Dictionary dictionary;
    dictionary.handle=0xC701; dictionary.parentHandle=0xC700; dictionary.hardOwner=1; dictionary.cloning=2; dictionary.m_entries={{"SELECTED",0xC710}};
    selected.dictionaries.push_back(dictionary);
    DRW_UnderlayDefinition definition;
    definition.handle=0xC710; definition.parentHandle=0xC701; definition.kind=DRW_UnderlayDefinition::PDF; definition.filename="selected.pdf"; definition.sheetName="1";
    author.linkUnderlay(&definition);
    DRW_Underlay entity(*observed); entity.handle=0xC720; entity.definitionHandle=0xC710;
    author.addUnderlay(&entity);
    selected.underlays.back()->sourceDwgPdfVersion = observed->sourceDwgPdfVersion;
    // This selected graph isolates the recorded source payload; it is not a
    // qualified export of the full producer file.
    selected.sourceFormat = dx_source_format::Unknown;
    if (!author.fileExport(argv[2], DRW::AC1024, false, &selected, false, true)) return 5;
    if (selected.underlays.size()!=1 || std::abs(selected.underlays.front()->rotation-std::acos(-1.)/4.)>1e-12) return 6;
    if (argc == 4) {
        const auto firstGeneration=observed->sourceImportGeneration;
        if (!reader.fileImport(argv[3], &source, false) || source.underlays.size()!=2) return 7;
        const auto* newest=source.underlays.back();
        if (observed->sourceDwgPdfVersion != DRW::AC1024
            || observed->sourceImportGeneration != firstGeneration
            || !newest || newest->sourceDwgPdfRotationRadians
            || newest->sourceImportGeneration == firstGeneration) return 8;
    }
    std::cout << "selected PDF DWG callback exported in degrees\n";
    return 0;
}
