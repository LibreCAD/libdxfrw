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

#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <sys/stat.h>

#include "dx_iface.h"
#include "dx_data.h"
#include "dx_cli.h"
#include "dx_batch_input.h"

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
    std::cout << "     version can be:" << std::endl;
    std::cout << "        -R12   dxf release 12 version" << std::endl;
    std::cout << "        -v2000 dxf version 2000" << std::endl;
    std::cout << "        -v2004 dxf version 2004" << std::endl;
    std::cout << "        -v2007 dxf version 2007" << std::endl;
    std::cout << "        -v2010 dxf version 2010" << std::endl;
}

bool convertFile(std::string inName, std::string outName, DRW::Version ver,
                 bool binary, bool overwrite, bool debug,
                 bool nonInteractive = false){
    bool badState = false;
    //verify if input file exist
    std::ifstream ifs;
    ifs.open (inName.c_str(), std::ifstream::in);
    badState = ifs.fail();
    ifs.close();
    if (badState) {
        std::cout << "Error can't open " << inName << std::endl;
        return false;
    }
    //verify if output file exist
    struct stat outStat;
    badState = stat(outName.c_str(), &outStat) != 0;
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
    badState = input.fileImport( inName, &fData, debug);
    if (!badState) {
        std::cout << "Error reading file " << inName << std::endl;
        return false;
    }

    if (ver == DRW::UNKNOWNV && nonInteractive) {
        ver = dwg2dxfCli::defaultOutputVersion(fData.sourceVersion);
        if (ver == DRW::UNKNOWNV) {
            std::cout << "Error: no supported default DXF version for source revision "
                      << static_cast<int>(fData.sourceVersion) << std::endl;
            return false;
        }
    }

    //And write a dxf file
    dx_iface output;
    // CLI complete-conversion admission is explicit and separate from owner
    // resolution. Ordinary library reads/adapter exports remain tolerant.
    badState = output.fileExport(outName, ver, binary, &fData, debug, true,
                                 dx_dxfSourceTextPolicy::RequireValidSource);

    return badState;
}

int main(int argc, char *argv[]) {
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
                                    options.debug, true);
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
                              options.overwrite, options.debug);
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
            const auto output = directory / source.filename();
            if (source.filename().empty() || !outputNames.insert(normalized(output)).second) {
                std::cout << "Batch mode: invalid or colliding output names\n";
                return 2;
            }
            sourceNames.insert(normalized(source));
            sources.push_back(source);
            outputs.push_back(output);
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
                               options.debug, true);
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
