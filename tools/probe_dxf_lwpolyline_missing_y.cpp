// SPDX-License-Identifier: GPL-2.0-or-later
// Check a locally authored missing-Y source and same-reader valid-zero retry.
#include "../tests/lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <chrono>
#include <filesystem>
#include <iostream>

struct Capture : LC3CompatCheck {
    void addLWPolyline(const DRW_LWPolyline& line) override {
        ++count;valid=line.vertlist.size()==3u;
        for(const auto& vertex:line.vertlist)valid=valid&&vertex&&vertex->y==0.;
    }
    int count=0;bool valid=false;
};
int main(int argc,char** argv) {
    if(argc!=3){std::cerr<<"usage: probe_dxf_lwpolyline_missing_y <missing.dxf> <zero.dxf>\n";return 2;}
    const auto path=std::filesystem::temp_directory_path()/
        ("lw-missing-y-retry-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
    struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}}cleanup{path};
    std::filesystem::copy_file(argv[1],path);dxfRW reader(path.string().c_str());Capture capture;
    if(reader.read(&capture,false)||capture.count!=0)return 1;
    std::filesystem::copy_file(argv[2],path,std::filesystem::copy_options::overwrite_existing);
    if(!reader.read(&capture,false)||capture.count!=1||!capture.valid)return 1;
    std::cout<<"libdxfrw missing-Y refusal and explicit-zero same-reader retry PASS\n";return 0;
}
