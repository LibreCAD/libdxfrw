#include <array>
#include <cstdint>
#include <iostream>
#include <string>

#include "drw_base.h"
#include "libdxfrw.h"
#include "libdwgr.h"

namespace {

struct TestContext {
    int failures {0};

    void expect(bool condition, const char* label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

void expectDiagnostic(TestContext& t, const DRW_OperationDiagnostic& d,
                      DRW::OperationKind operation,
                      DRW::OperationPhase phase,
                      DRW::OperationCause cause, const char* code,
                      const char* label) {
    t.expect(d.operation == operation && d.phase == phase
                 && d.cause == cause && d.code == code && d.failed(),
             label);
    t.expect(d.secondary.size() <= DRW_OperationDiagnostic::MaxSecondaryEntries,
             "diagnostic secondary list is bounded");
}

void testDxfDiagnostics(TestContext& t) {
    dxfRW writer(nullptr);
    const DRW_OperationDiagnostic initial = writer.getLastDiagnostic();
    t.expect(initial.operation == DRW::OperationKind::None
                 && !initial.failed(),
             "DXF diagnostic is empty before an operation");
    t.expect(!writer.write(nullptr, DRW::UNKNOWNV, false)
                 && writer.getError() == DRW::BAD_VERSION,
             "DXF unsupported version keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Write,
                     DRW::OperationPhase::Validation,
                     DRW::OperationCause::UnsupportedVersion,
                     "unsupported-version",
                     "DXF unsupported version has structured cause");

    t.expect(!writer.write(nullptr, DRW::AC1015, false)
                 && writer.getError() == DRW::BAD_UNKNOWN,
             "DXF null interface keeps legacy invalid-argument error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Write,
                     DRW::OperationPhase::Argument,
                     DRW::OperationCause::InvalidArgument,
                     "invalid-argument",
                     "DXF null interface has structured cause");

    std::string empty;
    t.expect(!writer.readAscii(nullptr, false, empty)
                 && writer.getError() == DRW::BAD_UNKNOWN,
             "DXF null read interface keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Read,
                     DRW::OperationPhase::Argument,
                     DRW::OperationCause::InvalidArgument,
                     "invalid-argument",
                     "DXF null read interface has structured cause");
}

void testDwgDiagnostics(TestContext& t) {
    dwgRW writer(nullptr);
    const DRW_OperationDiagnostic initial = writer.getLastDiagnostic();
    t.expect(initial.operation == DRW::OperationKind::None
                 && !initial.failed(),
             "DWG diagnostic is empty before an operation");
    t.expect(!writer.write(nullptr, DRW::UNKNOWNV, false)
                 && writer.getError() == DRW::BAD_VERSION,
             "DWG unsupported version keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Write,
                     DRW::OperationPhase::Validation,
                     DRW::OperationCause::UnsupportedVersion,
                     "unsupported-version",
                     "DWG unsupported version has structured cause");

    t.expect(!writer.write(nullptr, DRW::AC1015, false)
                 && writer.getError() == DRW::BAD_UNKNOWN,
             "DWG null write interface keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Write,
                     DRW::OperationPhase::Argument,
                     DRW::OperationCause::InvalidArgument,
                     "invalid-argument",
                     "DWG null write interface has structured cause");

    t.expect(!writer.read(nullptr, false)
                 && writer.getError() == DRW::BAD_UNKNOWN,
             "DWG null read interface keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Read,
                     DRW::OperationPhase::Argument,
                     DRW::OperationCause::InvalidArgument,
                     "invalid-argument",
                     "DWG null read interface has structured cause");

    const std::array<std::uint8_t, 6> badMagic {{'B', 'A', 'D', '0', '0', '0'}};
    t.expect(!writer.readBuffer(badMagic.data(), badMagic.size(), nullptr, false)
                 && writer.getError() == DRW::BAD_UNKNOWN,
             "DWG null buffer consumer keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Read,
                     DRW::OperationPhase::Argument,
                     DRW::OperationCause::InvalidArgument,
                     "invalid-argument",
                     "DWG null buffer consumer has structured cause");

    dwgRW missingFile("");
    t.expect(!missingFile.testReader()
                 && missingFile.getError() == DRW::BAD_OPEN,
             "DWG missing file keeps legacy open error");
    expectDiagnostic(t, missingFile.getLastDiagnostic(),
                     DRW::OperationKind::Read, DRW::OperationPhase::Open,
                     DRW::OperationCause::OpenFailure, "open-failure",
                     "DWG missing file has stage-aware open cause");
}

} // namespace

int main() {
    TestContext context;
    testDxfDiagnostics(context);
    testDwgDiagnostics(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " diagnostic assertion(s) failed\n";
        return 1;
    }
    std::cout << "Operation diagnostics: PASS\n";
    return 0;
}
