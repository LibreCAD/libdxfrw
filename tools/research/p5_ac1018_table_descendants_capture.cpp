// Capture exact raw callbacks for one pinned source, without embedding fixtures.
// Build against the existing dwg2dxf adapter object and libdxfrw; see receipt.
#include "dx_iface.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>

class Capture final : public dx_iface {
public:
    explicit Capture(std::filesystem::path directory) : directory_(std::move(directory)) {}
    void addUnsupportedObject(const DRW_UnsupportedObject& record) override {
        dx_iface::addUnsupportedObject(record);
        const auto handle = record.m_handle;
        if (handle != 0xb9u && handle != 0xbau && handle != 0xbdu
            && (handle < 0xbeu || handle > 0xc6u)) return;
        if (!seen_.insert(handle).second) { ok_ = false; return; }
        const auto path = directory_ / ("p5-desc-body-" + std::to_string(handle) + ".bin");
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(record.m_rawBytes.data()),
                     static_cast<std::streamsize>(record.m_rawBytes.size()));
        if (!output) ok_ = false;
        std::cout << "handle=" << std::hex << handle << std::dec
                  << " objectType=" << record.m_objectType
                  << " objectBufferOffset=" << record.m_objectOffset
                  << " bytes=" << record.m_rawBytes.size() << '\n';
    }
    bool complete() const { return ok_ && seen_.size() == 12u; }
private:
    std::filesystem::path directory_;
    std::set<std::uint32_t> seen_;
    bool ok_{true};
};
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "usage: capture drawing.dwg new-output-directory\n"; return 2; }
    std::error_code error;
    const std::filesystem::path directory(argv[2]);
    if (!std::filesystem::create_directory(directory, error)) {
        std::cerr << "capture directory must be new: " << error.message() << '\n'; return 2;
    }
    auto data = std::make_unique<dx_data>();
    auto capture = std::make_unique<Capture>(directory);
    const bool read = capture->fileImport(argv[1], data.get(), false);
    std::cout << "read=" << read << " completeCapture=" << capture->complete() << '\n';
    return read && capture->complete() ? 0 : 1;
}
