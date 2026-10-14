#include "dx_cli.h"

#include <iostream>
#include <utility>
#include <vector>

int main(){
    const std::vector<std::pair<DRW::Version, DRW::Version>> cases = {
        {DRW::AC1009, DRW::AC1009},
        {DRW::AC1012, DRW::AC1014},
        {DRW::AC1014, DRW::AC1014},
        {DRW::AC1015, DRW::AC1015},
        {DRW::AC1018, DRW::AC1018},
        {DRW::AC1021, DRW::AC1021},
        {DRW::AC1024, DRW::AC1024},
        {DRW::AC1027, DRW::AC1027},
        {DRW::AC1032, DRW::AC1032},
        {DRW::UNKNOWNV, DRW::UNKNOWNV},
        {DRW::MC00, DRW::UNKNOWNV},
        {DRW::AC1006, DRW::UNKNOWNV}
    };

    for (const auto& testCase : cases) {
        const DRW::Version actual =
            dwg2dxfCli::defaultOutputVersion(testCase.first);
        if (actual != testCase.second) {
            std::cerr << "Unexpected output version for source enum "
                      << static_cast<int>(testCase.first) << ": got "
                      << static_cast<int>(actual) << ", expected "
                      << static_cast<int>(testCase.second) << std::endl;
            return 1;
        }
    }

    for (const std::string token : {"", "-", "--", "-yUnexpected", "-bad",
                                    "-Bextra", "-dextra", "-v2007suffix"}) {
        dwg2dxfCli::Options options;
        if (dwg2dxfCli::applyOption(token, options, true)
            || options.overwrite || options.binary || options.batch
            || options.debug || options.haveVersion) {
            std::cerr << "Invalid option accepted or changed state: " << token << '\n';
            return 1;
        }
    }
    dwg2dxfCli::Options options;
    if (!dwg2dxfCli::applyOption("-Y", options, false) || !options.overwrite
        || !dwg2dxfCli::applyOption("-D", options, false) || !options.debug
        || !dwg2dxfCli::applyOption("-b", options, false) || !options.binary
        || dwg2dxfCli::applyOption("-B", options, false)
        || !dwg2dxfCli::applyOption("-B", options, true) || !options.batch
        || !dwg2dxfCli::applyOption("-v2007", options, true)
        || dwg2dxfCli::applyOption("-v2007", options, true)
        || dwg2dxfCli::applyOption("-v2010", options, true)
        || options.version != DRW::AC1021) {
        std::cerr << "Exact option/version policy changed\n";
        return 1;
    }

    return 0;
}
