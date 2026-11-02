/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <iostream>
#include <iomanip>
#include <locale>
#include <sstream>
#include "drw_dbg.h"

std::atomic<bool> DRW_dbg::s_enabled{false};

namespace {
// All built-in printers, including a retired one with an active snapshot,
// share this process-lifetime output lock. User callbacks never acquire it.
void emitDebugFragment(const std::string& fragment) {
    static std::mutex* const outputMutex = new std::mutex;
    std::lock_guard<std::mutex> lock(*outputMutex);
    std::cerr.write(fragment.data(), static_cast<std::streamsize>(fragment.size()));
}

std::ostringstream debugFragment() {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    return stream;
}
} // namespace

/*********private clases*************/

class print_debug : public DRW::DebugPrinter {
public:
    void printS(const std::string &s) override;
    void printI(long long int i) override;
    void printUI(long long unsigned int i) override;
    void printD(double d) override;
    void printH(long long int i) override;
    void printB(int i) override;
    void printHL(int c, int s, int h) override;
    void printPT(double x, double y, double z) override;
};

/********* debug class *************/
DRW_dbg *DRW_dbg::getInstance(){
    // Preserve the historical process lifetime. A destructed function-local
    // object would break callers logging from other static destructors.
    static DRW_dbg* const instance = new DRW_dbg;
    return instance;
}

DRW_dbg::DRW_dbg(){
    debugPrinter = std::make_shared<print_debug>();
}

void DRW_dbg::setCustomDebugPrinter(std::unique_ptr<DRW::DebugPrinter> printer)
{
    // Construct ownership before taking the state lock. Replaced printer
    // destruction (including reentrant custom destructors) occurs after it.
    std::shared_ptr<DRW::DebugPrinter> replacement = printer
        ? std::shared_ptr<DRW::DebugPrinter>(std::move(printer))
        : std::make_shared<print_debug>();
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        debugPrinter.swap(replacement);
    }
}

void DRW_dbg::setLevel(Level lvl){
    std::lock_guard<std::mutex> lock(stateMutex);
    level = lvl;
    s_enabled.store(lvl == Level::Debug, std::memory_order_relaxed);
}

DRW_dbg::Level DRW_dbg::getLevel(){
    std::lock_guard<std::mutex> lock(stateMutex);
    return level;
}

bool DRW_dbg::isDebugEnabled() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return level == Level::Debug;
}

std::shared_ptr<DRW::DebugPrinter> DRW_dbg::printerSnapshot() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return level == Level::Debug ? debugPrinter : nullptr;
}

void DRW_dbg::print(const char *s){
    if (auto printer = printerSnapshot()) printer->printS(s);
}

void DRW_dbg::print(const std::string &s){
    if (auto printer = printerSnapshot()) printer->printS(s);
}

void DRW_dbg::print(signed char i){
    if (auto printer = printerSnapshot()) printer->printI(i);
}

void DRW_dbg::print(unsigned char i){
    if (auto printer = printerSnapshot()) printer->printUI(i);
}

void DRW_dbg::print(int i){
    if (auto printer = printerSnapshot()) printer->printI(i);
}

void DRW_dbg::print(unsigned int i){
    if (auto printer = printerSnapshot()) printer->printUI(i);
}

void DRW_dbg::print(long int i){
    if (auto printer = printerSnapshot()) printer->printI(i);
}

void DRW_dbg::print(long long int i){
    if (auto printer = printerSnapshot()) printer->printI(i);
}

void DRW_dbg::print(long unsigned int i){
    if (auto printer = printerSnapshot()) printer->printUI(i);
}

void DRW_dbg::print(long long unsigned int i){
    if (auto printer = printerSnapshot()) printer->printUI(i);
}

void DRW_dbg::print(double d){
    if (auto printer = printerSnapshot()) printer->printD(d);
}

void DRW_dbg::printH(long long int i){
    if (auto printer = printerSnapshot()) printer->printH(i);
}

void DRW_dbg::printB(int i){
    if (auto printer = printerSnapshot()) printer->printB(i);
}
void DRW_dbg::printHL(int c, int s, int h){
    if (auto printer = printerSnapshot()) printer->printHL(c, s, h);
}

void DRW_dbg::printPT(double x, double y, double z){
    if (auto printer = printerSnapshot()) printer->printPT(x, y, z);
}

void print_debug::printS(const std::string &s){
    emitDebugFragment(s);
}

void print_debug::printI(long long int i){
    auto stream = debugFragment();
    stream << i;
    emitDebugFragment(stream.str());
}

void print_debug::printUI(long long unsigned int i){
    auto stream = debugFragment();
    stream << i;
    emitDebugFragment(stream.str());
}

void print_debug::printD(double d){
    auto stream = debugFragment();
    stream << std::fixed << d;
    emitDebugFragment(stream.str());
}

void print_debug::printH(long long  i){
    auto stream = debugFragment();
    stream << "0x" << std::setw(2) << std::setfill('0') << std::hex << i;
    emitDebugFragment(stream.str());
}

void print_debug::printB(int i){
    auto stream = debugFragment();
    // Keep the historical decimal, zero-padded convention. setbase(2) did
    // not request binary formatting from an ostream.
    stream << std::setw(8) << std::setfill('0') << i;
    emitDebugFragment(stream.str());
}

void print_debug::printHL(int c, int s, int h){
    auto stream = debugFragment();
    stream << c << '.' << s << ".0x" << std::setw(2) << std::setfill('0') << std::hex << h;
    emitDebugFragment(stream.str());
}

void print_debug::printPT(double x, double y, double z){
    auto stream = debugFragment();
    stream << std::fixed << "x: " << x << ", y: " << y << ", z: " << z;
    emitDebugFragment(stream.str());
}
