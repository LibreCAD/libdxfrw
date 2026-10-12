#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct ModelerTextRecord {
    std::vector<std::string> values;
};

bool collectSolidSatValues(const std::string& path,
                           std::vector<ModelerTextRecord>& records) {
    std::ifstream input(path);
    if (!input)
        return false;

    std::string codeLine;
    std::string value;
    bool inSection = false;
    bool inEntities = false;
    bool inSolid = false;
    bool expectSectionName = false;
    while (true) {
        if (!std::getline(input, codeLine))
            return input.eof() && !inSolid && !records.empty();
        if (!std::getline(input, value))
            return false;
        if (!codeLine.empty() && codeLine.back() == '\r')
            codeLine.pop_back();
        if (!value.empty() && value.back() == '\r')
            value.pop_back();

        const std::string code = [&codeLine]() {
            const std::size_t first = codeLine.find_first_not_of(" \t");
            if (first == std::string::npos)
                return std::string{};
            const std::size_t last = codeLine.find_last_not_of(" \t");
            return codeLine.substr(first, last - first + 1u);
        }();

        if (code == "0") {
            if (inSolid) {
                inSolid = false;
            }
            if (value == "SECTION") {
                inSection = true;
                inEntities = false;
                expectSectionName = true;
            } else if (value == "ENDSEC") {
                inSection = false;
                inEntities = false;
                expectSectionName = false;
            } else {
                expectSectionName = false;
                if (inSection && inEntities && value == "3DSOLID") {
                    records.emplace_back();
                    inSolid = true;
                }
            }
        } else if (code == "2" && expectSectionName) {
            expectSectionName = false;
            inEntities = value == "ENTITIES";
        } else if (inSolid && (code == "1" || code == "3")) {
            records.back().values.push_back(code);
            records.back().values.push_back(value);
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0]
                  << " REFERENCE.dxf CONVERTED.dxf\n";
        return 2;
    }

    std::vector<ModelerTextRecord> reference;
    std::vector<ModelerTextRecord> converted;
    if (!collectSolidSatValues(argv[1], reference)) {
        std::cerr << "failed to find 3DSOLID SAT text in reference: "
                  << argv[1] << '\n';
        return 1;
    }
    if (!collectSolidSatValues(argv[2], converted)) {
        std::cerr << "failed to find 3DSOLID SAT text in converted DXF: "
                  << argv[2] << '\n';
        return 1;
    }
    if (reference.size() != converted.size()) {
        std::cerr << "3DSOLID count differs: reference=" << reference.size()
                  << " converted=" << converted.size() << '\n';
        return 1;
    }
    for (std::size_t entity = 0; entity < reference.size(); ++entity) {
        if (reference[entity].values != converted[entity].values) {
            std::cerr << "SAT group-1/3 values differ in 3DSOLID #"
                      << entity << " (reference values="
                      << reference[entity].values.size() / 2u
                      << ", converted values="
                      << converted[entity].values.size() / 2u << ")\n";
            return 1;
        }
    }

    std::size_t pairCount = 0;
    for (const ModelerTextRecord& record : reference)
        pairCount += record.values.size() / 2u;
    std::cout << "Matched " << reference.size()
              << " 3DSOLID SAT carriers across " << pairCount
              << " group-1/3 code/value pairs\n";
    return 0;
}
