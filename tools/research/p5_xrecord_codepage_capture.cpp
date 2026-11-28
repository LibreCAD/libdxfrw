// Capture one selected XRECORD body from a local DWG; see codepage receipt.
#include "dx_iface.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

class Capture final : public dx_iface {
public:
    explicit Capture(std::filesystem::path output) : output_(std::move(output)) {}
    void addUnsupportedObject(const DRW_UnsupportedObject& record) override {
        dx_iface::addUnsupportedObject(record);
        if (record.m_handle != 0x7ffeu) return;
        if (seen_) { good_ = false; return; }
        seen_ = true;
        std::ofstream output(output_, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(record.m_rawBytes.data()),
                     static_cast<std::streamsize>(record.m_rawBytes.size()));
        good_ = static_cast<bool>(output) && record.m_objectType == dwgObjType::XRECORD;
        std::cout << "handle=0x7ffe objectBufferOffset=" << record.m_objectOffset
                  << " bytes=" << record.m_rawBytes.size() << '\n';
    }
    bool good() const { return good_ && seen_; }
private:
    std::filesystem::path output_;
    bool good_{false};
    bool seen_{false};
};
int main(int argc, char** argv) {
    if (argc != 3 || std::filesystem::exists(argv[2])) {
        std::cerr << "usage: capture drawing.dwg new-output-file\n"; return 2;
    }
    auto data = std::make_unique<dx_data>();
    auto capture = std::make_unique<Capture>(argv[2]);
    const bool read = capture->fileImport(argv[1], data.get(), false);
    std::cout << "read=" << read << " captured=" << capture->good() << '\n';
    return read && capture->good() ? 0 : 1;
}
