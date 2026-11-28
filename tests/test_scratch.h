// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LIBDXFRW_TEST_SCRATCH_H
#define LIBDXFRW_TEST_SCRATCH_H
#include <filesystem>
#include <string>
#include <system_error>

namespace libdxfrw_test {
// Clock values need not be unique across concurrent test processes. Claim a
// directory atomically, retry collisions only, and never reuse another test's
// drawing owners or silently lose the filesystem diagnostic.
inline std::filesystem::path reserveTestScratch(const std::filesystem::path& parent,const std::string& stem,std::error_code& error) {
    for(unsigned attempt=0u;attempt<64u;++attempt) {
        auto candidate=parent/(stem+"-"+std::to_string(attempt));error.clear();
        if(std::filesystem::create_directory(candidate,error)&&!error)return candidate;
        if(error&&error!=std::errc::file_exists)return {};
    }
    error=std::make_error_code(std::errc::file_exists);return {};
}
}
#endif
