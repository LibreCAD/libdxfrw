// SPDX-License-Identifier: GPL-2.0-or-later
// Scratch-only P6 AC1021 single-LINE producer; never writes repository fixtures.
#include "../tests/lc3_compat_check.cpp"
#include "libdwgr.h"
#include <cstdio>

class Probe final : public LC3CompatCheck, public DRW_DwgDefaultPlotStyleProvider {
public:
    Probe(dwgRW& target, bool opt) : writer(target), optin(opt) {}
    bool wantsDwgDefaultPlotStyleGraph() const override { return optin; }
    void writeEntities() override {
        DRW_Line line;
        line.basePoint = {1., 2., 3.};
        line.secPoint = {4., 5., 6.};
        ok = writer.writeLine(&line);
    }
    bool finalizeDwgWrite() override { return ok; }
    dwgRW& writer;
    bool optin;
    bool ok = false;
};

int main(int argc, char** argv) {
    if (argc != 3 || (argv[2][0] != '0' && argv[2][0] != '1') || argv[2][1]) return 2;
    dwgRW writer(argv[1]);
    Probe probe(writer, argv[2][0] == '1');
    const bool result = writer.write(&probe, DRW::AC1021, true);
    std::printf("write=%d line=%d\n", int(result), int(probe.ok));
    return result && probe.ok ? 0 : 1;
}
