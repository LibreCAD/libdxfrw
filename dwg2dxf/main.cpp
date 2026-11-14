/******************************************************************************
**  dwg2dxf - Program to convert dwg/dxf to dxf(ascii & binary)              **
**                                                                           **
**  Copyright (C) 2015 José F. Soriano, rallazz@gmail.com                    **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <system_error>
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "dx_iface.h"
#include "dx_data.h"
#include "dx_cli.h"
#include "dx_batch_input.h"
#include "dx_batch_output_names.h"

void usage(){
    std::cout << "Usage: " << std::endl;
    std::cout << "   dwg2dxf <input> [-b] <-version> <output>" << std::endl << std::endl;
    std::cout << "   dwg2dxf <input> -o <output> [-version] [-b] [-y]" << std::endl << std::endl;
    std::cout << "   input      existing file to convert" << std::endl;
    std::cout << "   -b         optional, sets output as binary dxf" << std::endl;
    std::cout << "   -B         optional, batch mode reads a text file whit a list of full path input" << std::endl;
    std::cout << "               files and saves with the same name in the indicated folder as output" << std::endl;
    std::cout << "               LF/CRLF lists; 128 KiB filename, 16 MiB list, 10000 inputs maximum" << std::endl;
    std::cout << "               requires -version, never prompts, returns failure if any item fails" << std::endl;
    std::cout << "   -y -Y      optional, Warning! if output dxf exist overwrite without ask" << std::endl;
    std::cout << "   -d -D      optional, enable debug trace" << std::endl;
    std::cout << "   -version   version output of dxf file" << std::endl;
    std::cout << "   output     output file name" << std::endl << std::endl;
    std::cout << "   -o         FreeCAD-compatible output path; defaults to the input revision" << std::endl;
    std::cout << "              and never prompts. Use -y to replace an existing output." << std::endl;
    std::cout << "   DXF input text is checked before output: observed corruption, ambiguous" << std::endl;
    std::cout << "   decoding and unsupported legacy Unicode profiles fail without publication." << std::endl;
    std::cout << "   --dxf-codepage-policy=compatible|exact-named selects DXF input mapping." << std::endl;
    std::cout << "   Default is compatible; exact input still requires an eligible source profile." << std::endl;
    std::cout << "   --observe-whole-dxf-source observes DXF scalars during import for" << std::endl;
    std::cout << "   eligible lossless profiles, including in batch mode. It is off by default" << std::endl;
    std::cout << "   and retains up to 64 MiB of source data with a separate work limit;" << std::endl;
    std::cout << "   total memory can be higher and strict checks remain in force." << std::endl;
    std::cout << "   An emitted DXF may not itself qualify for another conversion." << std::endl;
    std::cout << "   --allow-partial-dwg-read permits bounded terminal DWG frame losses;" << std::endl;
    std::cout << "   strict is the default; owner/reference checks remain strict and" << std::endl;
    std::cout << "   partial output is not a complete DWG semantics/support claim." << std::endl;
    std::cout << "     version can be:" << std::endl;
    std::cout << "        -R12   dxf release 12 version" << std::endl;
    std::cout << "        -v2000 dxf version 2000" << std::endl;
    std::cout << "        -v2004 dxf version 2004" << std::endl;
    std::cout << "        -v2007 dxf version 2007" << std::endl;
    std::cout << "        -v2010 dxf version 2010" << std::endl;
    std::cout << "        -v2013 dxf version 2013 (AC1027)" << std::endl;
    std::cout << "        -v2018 dxf version 2018 (AC1032)" << std::endl;
}

namespace {

const char* cliVersionName(DRW::Version version) noexcept {
    switch (version) {
    case DRW::MC00: return "MC0.0";
    case DRW::AC12: return "AC1.2";
    case DRW::AC14: return "AC1.40";
    case DRW::AC150: return "AC1.50";
    case DRW::AC210: return "AC2.10";
    case DRW::AC1002: return "AC1002";
    case DRW::AC1003: return "AC1003";
    case DRW::AC1004: return "AC1004";
    case DRW::AC1006: return "AC1006";
    case DRW::AC1009: return "AC1009";
    case DRW::AC1012: return "AC1012";
    case DRW::AC1014: return "AC1014";
    case DRW::AC1015: return "AC1015";
    case DRW::AC1018: return "AC1018";
    case DRW::AC1021: return "AC1021";
    case DRW::AC1024: return "AC1024";
    case DRW::AC1027: return "AC1027";
    case DRW::AC1032: return "AC1032";
    default: return "unknown";
    }
}

const char* cliSourceFormatName(dx_source_format format) noexcept {
    switch (format) {
    case dx_source_format::DXF: return "DXF";
    case dx_source_format::DWG: return "DWG";
    case dx_source_format::Unknown: return "unknown";
    }
    return "unknown";
}

const char* cliCodePagePolicyName(DRW::DxfCodePagePolicy policy) noexcept {
    return policy == DRW::DxfCodePagePolicy::ExactNamed
        ? "exact-named" : "compatible";
}

const char* cliDwgFrameCoverageName(
        DRW_DwgFrameCoverageStatus status) noexcept {
    switch (status) {
    case DRW_DwgFrameCoverageStatus::NotAvailable: return "not-available";
    case DRW_DwgFrameCoverageStatus::InProgress: return "in-progress";
    case DRW_DwgFrameCoverageStatus::FinalizedComplete: return "complete";
    case DRW_DwgFrameCoverageStatus::FinalizedPartial: return "partial";
    }
    return "unknown";
}

void writeCliHex64(std::ostream& output, std::uint64_t value) {
    static constexpr char digits[] = "0123456789abcdef";
    std::array<char, 16> text{};
    for (std::size_t index = text.size(); index > 0; --index) {
        text[index - 1] = digits[value & 0x0fu];
        value >>= 4u;
    }
    output << "0x";
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
}

void reportCliConversionProjection(const dx_data& data,
                                   DRW::Version targetVersion,
                                   bool sourceDefaultTarget,
                                   bool binary,
                                   bool allowPartialDwgRead,
                                   DRW::DxfCodePagePolicy codePagePolicy) {
    const char* readCompleteness = data.sourceFormat != dx_source_format::DWG
        ? "not-applicable"
        : allowPartialDwgRead ? "explicit-partial" : "strict";
    const char* targetSelection = sourceDefaultTarget
        ? targetVersion == DRW::UNKNOWNV
            ? "source-default-unavailable" : "source-default"
        : targetVersion == DRW::UNKNOWNV ? "unselected" : "explicit";
    std::cerr << "CLI conversion projection: schema=1 sourceFormat="
              << cliSourceFormatName(data.sourceFormat)
              << " sourceVersion=" << cliVersionName(data.sourceVersion)
              << " targetVersion=" << cliVersionName(targetVersion)
              << " targetSelection=" << targetSelection
              << " outputEncoding=" << (binary ? "Binary" : "ASCII")
              << " readCompleteness=" << readCompleteness
              << " stage=before-output-preflight\n";

    const bool isDxf = data.sourceFormat == dx_source_format::DXF;
    const bool isDwg = data.sourceFormat == dx_source_format::DWG;
    const char* headerStructure = !isDxf ? "not-applicable"
        : data.headerC.canWriteDxfHeaderStructure() ? "ready" : "repair-required";
    const char* headerInitialPreflight = !isDxf ? "not-applicable"
        : targetVersion == DRW::UNKNOWNV ? "unknown-target"
        : data.headerC.canWriteDxfHeaderInitialPreflight(targetVersion)
            ? "admitted" : "refused";
    const auto requiredVersions = data.headerC.vars.find("$REQUIREDVERSIONS");
    const bool hasRequiredVersions = requiredVersions != data.headerC.vars.end();
    const DRW_Variant* requiredValue = hasRequiredVersions
        ? requiredVersions->second : nullptr;
    const bool wellTypedMask = requiredValue != nullptr
        && requiredValue->type() == DRW_Variant::INTEGER64
        && requiredValue->code() == 160
        // The variant's final value alone cannot reveal a repeated group 160
        // or duplicate variable: parsing keeps a usable-looking value while
        // separately marking the source field malformed. Probe the newest
        // supported target to report source shape independently of the
        // user's selected output version.
        && data.headerC.canWriteDxfRequiredVersions(DRW::AC1032);
    const DwgReadObservations& dwgObservations = data.dwgReadOutcome.observations;
    std::size_t retainedIntegrityErrors = 0u;
    std::size_t retainedIntegrityWarnings = 0u;
    const std::size_t retainedIntegrityRows = std::min(
        dwgObservations.integrity.retained,
        dwgObservations.integrity.rows.size());
    for (std::size_t index = 0; index < retainedIntegrityRows; ++index) {
        if (dwgObservations.integrity.rows[index].severity
                == DwgIntegritySeverity::Error)
            ++retainedIntegrityErrors;
        else
            ++retainedIntegrityWarnings;
    }
    std::cerr << "CLI source disposition projection: schema=1 "
              << "sourceRepairAction=none "
              << "sourceHeaderStructure=" << headerStructure << ' '
              << "sourceHeaderInitialPreflight=" << headerInitialPreflight << ' '
              << "inputCodePagePolicy=" << cliCodePagePolicyName(codePagePolicy)
              << " sourceDxfTextIssueMask=";
    if (isDxf)
        writeCliHex64(std::cerr, data.dxfReadOutcome.observations.issueMask);
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceRequiredVersionsStatus=";
    if (!isDxf) {
        std::cerr << "not-applicable sourceRequiredVersionsMask=not-applicable";
    } else if (!hasRequiredVersions) {
        std::cerr << "absent sourceRequiredVersionsMask=absent";
    } else if (!wellTypedMask) {
        std::cerr << "malformed sourceRequiredVersionsMask=unknown";
    } else {
        std::cerr << "present sourceRequiredVersionsMask=";
        writeCliHex64(std::cerr,
            static_cast<std::uint64_t>(requiredValue->i64_val()));
    }
    std::cerr << " sourceRequiredVersionsOutput=";
    if (!isDxf)
        std::cerr << "not-applicable";
    else if (targetVersion == DRW::UNKNOWNV)
        std::cerr << "unknown-target";
    else
        std::cerr << (data.headerC.canWriteDxfRequiredVersions(targetVersion)
            ? "admitted" : "refused");
    std::cerr << " sourceDwgSkippedCustomClassInstances=";
    if (isDwg)
        std::cerr << dwgObservations.skippedCustomClasses.instances;
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgSkippedUnsupportedObjectInstances=";
    if (isDwg)
        std::cerr << dwgObservations.skippedUnsupportedObjects.instances;
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgEntityParseFailures=";
    if (isDwg)
        std::cerr << dwgObservations.entityParseFailures;
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgObjectParseFailures=";
    if (isDwg)
        std::cerr << dwgObservations.objectParseFailures;
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgFrameCoverage=";
    if (isDwg)
        std::cerr << cliDwgFrameCoverageName(dwgObservations.frames.status);
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgSourceFrames=";
    if (isDwg)
        std::cerr << dwgObservations.frames.sourceFrames;
    else
        std::cerr << "not-applicable";
    static constexpr std::array<const char*, 7> frameDispositionFields{{
        " sourceDwgFramesPending=", " sourceDwgFramesDeferred=",
        " sourceDwgFramesStaged=", " sourceDwgFramesPublished=",
        " sourceDwgFramesQuarantined=", " sourceDwgFramesFailed=",
        " sourceDwgFramesUnresolved="}};
    for (std::size_t index = 0; index < frameDispositionFields.size(); ++index) {
        std::cerr << frameDispositionFields[index];
        if (isDwg)
            std::cerr << dwgObservations.frames.dispositions[index];
        else
            std::cerr << "not-applicable";
    }
    std::cerr << " sourceDwgFramesUnrecognized=";
    if (isDwg)
        std::cerr << dwgObservations.frames.unrecognizedDispositions;
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgIntegrityRetainedRows=";
    if (isDwg)
        std::cerr << retainedIntegrityRows;
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgIntegrityErrorsAmongRetained=";
    if (isDwg)
        std::cerr << retainedIntegrityErrors;
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgIntegrityWarningsAmongRetained=";
    if (isDwg)
        std::cerr << retainedIntegrityWarnings;
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgIntegrityExtractionOmitted=";
    if (isDwg)
        std::cerr << dwgObservations.integrity.extractionOmitted;
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgIntegrityUpstreamDroppedLowerBound=";
    if (isDwg)
        std::cerr << dwgObservations.integrity.reportedUpstreamDropped;
    else
        std::cerr << "not-applicable";
    std::cerr << " sourceDwgIntegrityAggregateSaturated=";
    if (!isDwg)
        std::cerr << "not-applicable";
    else
        std::cerr << (dwgObservations.integrity.aggregateSaturated
            ? "true" : "false");
    std::cerr << "\n";
}

} // namespace

bool convertFile(std::string inName, std::string outName, DRW::Version ver,
                 bool binary, bool overwrite, bool debug,
                 bool nonInteractive = false,
                 DRW::DxfCodePagePolicy codePagePolicy = DRW::DxfCodePagePolicy::Compatible,
                 bool allowPartialDwgRead = false,
                 bool observeWholeDxfSource = false){
    bool badState = false;
    namespace fs = std::filesystem;
    const fs::path inputPath = fs::u8path(inName);
    const fs::path outputPath = fs::u8path(outName);
    //verify if input file exist
    std::ifstream ifs;
    ifs.open(inputPath, std::ifstream::in);
    badState = ifs.fail();
    ifs.close();
    if (badState) {
        std::cout << "Error can't open " << inName << std::endl;
        return false;
    }
    // A same-file conversion truncates the source when the DXF writer opens
    // its destination. Check filesystem identity before prompting or granting
    // overwrite authority; equivalent() also catches symlink and hard-link
    // aliases that lexical path comparisons miss.
    std::error_code identityError;
    const bool outputExists = fs::exists(outputPath, identityError);
    if (identityError) {
        std::cout << "Error: cannot safely inspect output identity for "
                  << outName << ": " << identityError.message() << std::endl;
        return false;
    }
    if (outputExists) {
        const bool aliasesInput = fs::equivalent(inputPath, outputPath, identityError);
        if (identityError) {
            std::cout << "Error: cannot safely compare input/output identity for "
                      << inName << " and " << outName << ": "
                      << identityError.message() << std::endl;
            return false;
        }
        if (aliasesInput) {
            std::cout << "Error: output aliases input; refusing conversion" << std::endl;
            return false;
        }
    }
    //verify if output file exist
    badState = !outputExists;
    if (!badState) {
        if (!overwrite){
            if (nonInteractive) {
                std::cout << "File " << outName
                          << " already exists; refusing to overwrite" << std::endl;
                return false;
            }
            std::cout << "File " << outName << " already exist, overwrite Y/N ?" << std::endl;
            int c = getchar();
            if (! ('y' == c || 'Y' == c)) {
                std::cout << "Cancelled.";
                return false;
            }
        }
    }
    //All ok proceed whit conversion
    //class to store file read:
    dx_data fData;
    //First read a dwg or dxf file
    dx_iface input;
    if (observeWholeDxfSource && !input.enableWholeDxfSourceObservation()) {
        std::cerr << "Error: cannot enable DXF source observation" << std::endl;
        return false;
    }
    badState = input.fileImport(inName, &fData, debug, codePagePolicy);
    if (!badState) {
        std::cout << "Error reading file " << inName << std::endl;
        return false;
    }

    const bool sourceDefaultTarget = ver == DRW::UNKNOWNV && nonInteractive;
    if (sourceDefaultTarget) {
        ver = dwg2dxfCli::defaultOutputVersion(fData.sourceVersion);
        if (ver == DRW::UNKNOWNV) {
            if (debug)
                reportCliConversionProjection(fData, ver, sourceDefaultTarget,
                                               binary, allowPartialDwgRead,
                                               codePagePolicy);
            std::cout << "Error: no supported default DXF version for source revision "
                      << static_cast<int>(fData.sourceVersion) << std::endl;
            return false;
        }
    }

    if (debug)
        reportCliConversionProjection(fData, ver, sourceDefaultTarget, binary,
                                       allowPartialDwgRead, codePagePolicy);

    if (allowPartialDwgRead && fData.sourceFormat == dx_source_format::DWG) {
        std::cerr << "WARNING: explicit partial DWG read mode; output may omit "
                     "failed or unmodeled source content and is not a "
                     "completeness/support claim. Owner/reference checks remain strict.\n";
    }

    //And write a dxf file
    dx_iface output;
    // CLI complete-conversion admission is explicit and separate from owner
    // resolution and proxy-cache replay. Ordinary library exports retain the
    // compatibility policy for both.
    badState = output.fileExport(outName, ver, binary, &fData, debug, true,
                                 dx_dxfSourceTextPolicy::RequireValidSource,
                                 DRW::DxfTextWritePolicy::RequireLossless,
                                 dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,
                                 allowPartialDwgRead
                                     ? dx_dwgReadCompletenessPolicy::AllowPartial
                                     : dx_dwgReadCompletenessPolicy::RequireComplete);

    return badState;
}

int runCli(int argc, char *argv[]) {
    bool badState = false;
    dwg2dxfCli::Options options;
    std::string outName;
    if (argc < 3) {
        usage();
        return 1;
    }

    bool freeCadMode = false;
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "-o") {
            freeCadMode = true;
            break;
        }
    }

    if (freeCadMode) {
        const std::string fileName = argv[1];
        bool haveOutput = false;
        bool invalidOption = false;

        for (int i = 2; i < argc; ++i) {
            const std::string param = argv[i];
            if (param == "-o") {
                if (haveOutput || i + 1 >= argc) {
                    invalidOption = true;
                    break;
                }
                outName = argv[++i];
                haveOutput = true;
            } else if (!dwg2dxfCli::applyOption(param, options, false)) {
                invalidOption = true;
                break;
            }
        }

        if (invalidOption || !haveOutput || fileName.empty() || outName.empty()) {
            std::cout << "Bad options." << std::endl;
            usage();
            return 1;
        }

        const bool ok = convertFile(fileName, outName, options.version,
                                    options.binary, options.overwrite,
                                    options.debug, true, options.codePagePolicy,
                                    options.allowPartialDwgRead,
                                    options.observeWholeDxfSource);
        if (ok) {
            std::cout << "Success: converted " << fileName << " to " << outName << std::endl;
            return 0;
        }
        std::cout << "\nConversion failed" << std::endl;
        return 1;
    }

//parse params.
    std::string fileName = argv[1];
    for (int i= 2; i < argc; ++i){
        std::string param = argv[i];
        if (i == (argc - 1) )
            outName = param;
        else if (!dwg2dxfCli::applyOption(param, options, true))
            badState = true;
    }

    if (badState || fileName.empty() || outName.empty()
        || (options.batch && !options.haveVersion)) {
        std::cout << "Bad options." << std::endl;
        usage();
        return 1;
    }

    if (!options.batch){ //no batch mode, only one file
        bool ok = convertFile(fileName, outName, options.version, options.binary,
                              options.overwrite, options.debug, false, options.codePagePolicy,
                              options.allowPartialDwgRead, options.observeWholeDxfSource);
        if (ok) {
            std::cout << "Success: converted " << fileName << " to " << outName << std::endl;
            return 0;
        }
        else {
            std::cout << "\nConversion failed" << std::endl;
            return 1;
        }

    }

    // List acquisition and name/identity preflight complete before any output.
    // File conversions remain independent transactions, not a batch rollback.
    try {
        namespace fs = std::filesystem;
        const auto listPath = fs::u8path(fileName);
        std::error_code ec;
        if (!fs::is_regular_file(listPath, ec) || ec) {
            std::cout << "Batch mode: input list must be a regular file\n";
            return 2;
        }
        std::ifstream list(listPath, std::ios::binary);
        std::vector<std::string> inputs;
        std::string error;
        if (!list.is_open() || !dwg2dxfCli::readBatchList(list, inputs, error)) {
            std::cout << "Batch mode: "
                      << (list.is_open() ? error : "cannot open input list") << '\n';
            return 2;
        }
        const auto directory = fs::u8path(outName);
        ec.clear();
        if (!fs::is_directory(directory, ec) || ec) {
            std::cout << "Batch mode: output must be an existing directory\n";
            return 4;
        }
        std::vector<fs::path> sources;
        std::vector<fs::path> outputs;
        std::set<fs::path> sourceNames;
        std::set<fs::path> outputNames;
        const auto normalized = [](const fs::path& path) {
            return fs::weakly_canonical(fs::absolute(path));
        };
        sourceNames.insert(normalized(fs::u8path(fileName)));
        for (const auto& name : inputs) {
            const auto source = fs::u8path(name);
            auto outputName = source.filename();
            outputName.replace_extension(".dxf");
            const auto output = directory / outputName;
            if (source.filename().empty() || !outputNames.insert(normalized(output)).second) {
                std::cout << "Batch mode: invalid or colliding output names\n";
                return 2;
            }
            sourceNames.insert(normalized(source));
            sources.push_back(source);
            outputs.push_back(output);
        }
        const auto outputNameCheck = dwg2dxfCli::checkBatchOutputNameCollisions(
            directory, outputs, error);
        if (outputNameCheck == dwg2dxfCli::BatchOutputNameCheck::Collision) {
            std::cout << "Batch mode: invalid or colliding output names\n";
            return 2;
        }
        if (outputNameCheck == dwg2dxfCli::BatchOutputNameCheck::Error) {
            std::cout << "Batch mode: cannot safely validate output names: "
                      << error << '\n';
            return 2;
        }
        for (const auto& output : outputs) {
            if (sourceNames.count(normalized(output))) {
                std::cout << "Batch mode: output aliases an input/list\n";
                return 2;
            }
            ec.clear();
            const bool exists = fs::exists(output, ec);
            if (ec) {
                std::cout << "Batch mode: cannot inspect output identity\n";
                return 2;
            }
            if (!exists)
                continue;
            for (const auto& source : sources) {
                ec.clear();
                const bool same = fs::equivalent(source, output, ec);
                // A nonexistent source is an independent conversion failure.
                if (same) {
                    std::cout << "Batch mode: output aliases an input\n";
                    return 2;
                }
                if (ec && ec != std::errc::no_such_file_or_directory) {
                    std::cout << "Batch mode: cannot inspect input identity\n";
                    return 2;
                }
            }
            ec.clear();
            if (fs::equivalent(fs::u8path(fileName), output, ec) || ec) {
                std::cout << "Batch mode: output aliases list or identity check failed\n";
                return 2;
            }
        }
        bool allOk = true;
        for (std::size_t index = 0; index < inputs.size(); ++index) {
            const std::string output = outputs[index].u8string();
            ec.clear();
            const bool regular = fs::is_regular_file(sources[index], ec);
            const bool ok = regular && !ec
                && convertFile(inputs[index], output, options.version,
                               options.binary, options.overwrite,
                               options.debug, true, options.codePagePolicy,
                               options.allowPartialDwgRead, options.observeWholeDxfSource);
            std::cout << (ok ? "Success: " : "Failed: ") << inputs[index]
                      << " -> " << output << '\n';
            allOk = ok && allOk;
        }
        return allOk ? 0 : 1;
    } catch (const std::exception& exception) {
        std::cout << "Batch mode: preflight/conversion error: " << exception.what() << '\n';
        return 2;
    }
}

#ifdef _WIN32
int wmain(int argc, wchar_t *wideArgv[]) {
    std::vector<std::string> utf8Arguments;
    std::vector<char *> utf8Argv;
    try {
        utf8Arguments.reserve(static_cast<std::size_t>(argc));
        utf8Argv.reserve(static_cast<std::size_t>(argc) + 1u);
        for (int index = 0; index < argc; ++index) {
            const wchar_t *argument = wideArgv[index];
            if (argument == nullptr) {
                std::cerr << "Error: invalid command-line argument\n";
                return 2;
            }
            const int required = WideCharToMultiByte(
                CP_UTF8, WC_ERR_INVALID_CHARS, argument, -1, nullptr, 0,
                nullptr, nullptr);
            if (required <= 0) {
                std::cerr << "Error: command-line arguments must be valid Unicode\n";
                return 2;
            }
            std::string encoded(static_cast<std::size_t>(required), '\0');
            const int written = WideCharToMultiByte(
                CP_UTF8, WC_ERR_INVALID_CHARS, argument, -1, &encoded[0],
                required, nullptr, nullptr);
            if (written != required) {
                std::cerr << "Error: could not encode command-line argument as UTF-8\n";
                return 2;
            }
            encoded.resize(static_cast<std::size_t>(required - 1));
            utf8Arguments.push_back(std::move(encoded));
        }
        for (std::string &argument : utf8Arguments)
            utf8Argv.push_back(const_cast<char *>(argument.c_str()));
        utf8Argv.push_back(nullptr);
    } catch (...) {
        std::cerr << "Error: could not prepare command-line arguments\n";
        return 2;
    }
    return runCli(argc, utf8Argv.data());
}
#else
int main(int argc, char *argv[]) {
    return runCli(argc, argv);
}
#endif
