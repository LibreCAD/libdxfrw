#include "dx_cli.h"
#include "dx_batch_input.h"

#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

class FailingBatchBuffer final : public std::streambuf {
public:
    FailingBatchBuffer() { setg(prefix, prefix, prefix + sizeof(prefix)); }
private:
    int_type underflow() override { throw std::ios_base::failure("injected read error"); }
    char prefix[2] {'a', '\n'};
};

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

    const dwg2dxfCli::BatchInputLimits limits {4u, 2u, 8u};
    for (const auto& test : std::vector<std::pair<std::string, std::vector<std::string>>>{
             {"abcd", {"abcd"}}, {"abcd\r\n", {"abcd"}},
             {"abcd\nab\n", {"abcd", "ab"}},
             {"a\r\nb\n", {"a", "b"}},
             {" a \n", {" a "}}, {"a\r", {"a\r"}},
             {"\na\n", {"a"}}}) {
        std::istringstream stream(test.first);
        std::vector<std::string> parsed {"previous"};
        std::string error;
        if (!dwg2dxfCli::readBatchList(stream, parsed, error, limits)
            || parsed != test.second || !error.empty()) {
            std::cerr << "Bounded batch positive failed\n";
            return 1;
        }
    }
    for (const auto& text : {std::string(), std::string("\n\r\n"),
                             std::string("abcde\n"), std::string("a\nb\nc\n"),
                             std::string("a\0b\n", 4), std::string("a\nb\nccc\n\n")}) {
        std::istringstream stream(text);
        std::vector<std::string> parsed {"previous"};
        std::string error;
        if (dwg2dxfCli::readBatchList(stream, parsed, error, limits)
            || parsed != std::vector<std::string>{"previous"} || error.empty()) {
            std::cerr << "Bounded batch negative changed output\n";
            return 1;
        }
    }
    std::istringstream broken("a\n");
    broken.setstate(std::ios::badbit);
    std::vector<std::string> retained {"previous"};
    std::string error;
    if (dwg2dxfCli::readBatchList(broken, retained, error, limits)
        || retained != std::vector<std::string>{"previous"}
        || error != "batch list read failed") {
        std::cerr << "Batch I/O error was accepted\n";
        return 1;
    }
    FailingBatchBuffer buffer;
    std::istream partial(&buffer);
    if (dwg2dxfCli::readBatchList(partial, retained, error, limits)
        || retained != std::vector<std::string>{"previous"}
        || error != "batch list read failed") {
        std::cerr << "Partial batch I/O failure published the parsed prefix\n";
        return 1;
    }
    for (const auto& cap : std::vector<dwg2dxfCli::BatchInputLimits>{
             {4u, 100u, 8u}, {4u, 0u, 8u}, {0u, 2u, 8u}, {4u, 2u, 0u}}) {
        const std::string text = cap.totalBytes == 8u && cap.items == 100u
            ? "\n\n\n\n\n\n\n\n\n" : "a";
        std::istringstream stream(text);
        if (dwg2dxfCli::readBatchList(stream, retained, error, cap)
            || retained != std::vector<std::string>{"previous"}
            || (cap.items == 100u && error != "batch byte limit exceeded")) {
            std::cerr << "Independent batch quota boundary was not enforced\n";
            return 1;
        }
    }

    return 0;
}
