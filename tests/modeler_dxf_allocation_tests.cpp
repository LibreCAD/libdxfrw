// SPDX-License-Identifier: GPL-2.0-or-later
// Generated whole-write modeler SAT text allocation rollback/retry control.
#include "dx_iface.h"
#include "libdxfrw.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t allocationAttempts = 0u;
bool allocationFailed = false;
bool allocationTracking = false;

bool failAllocation() noexcept {
  if (!allocationTracking || allocationsBeforeFailure < 0)
    return false;
  ++allocationAttempts;
  if (allocationsBeforeFailure == 0) {
    allocationsBeforeFailure = -1;
    allocationFailed = true;
    return true;
  }
  --allocationsBeforeFailure;
  return false;
}

void armAllocationFailure(long offset) noexcept {
  allocationsBeforeFailure = offset;
  allocationAttempts = 0u;
  allocationFailed = false;
  allocationTracking = true;
}

void disarmAllocationFailure() noexcept {
  allocationTracking = false;
  allocationsBeforeFailure = -1;
}

bool readBytes(const std::filesystem::path &path, std::string &bytes) {
  std::ifstream input(path, std::ios::binary);
  if (!input)
    return false;
  bytes.assign(std::istreambuf_iterator<char>{input},
               std::istreambuf_iterator<char>{});
  return !input.bad();
}

bool hasStagedOutput(const std::filesystem::path &target) {
  const std::filesystem::path directory = target.parent_path().empty()
                                              ? std::filesystem::path(".")
                                              : target.parent_path();
  const std::string prefix = target.filename().string() + ".libdxfrw-";
  std::error_code error;
  std::filesystem::directory_iterator entry(directory, error);
  if (error)
    return true;
  const std::filesystem::directory_iterator end;
  while (entry != end) {
    if (entry->path().filename().string().rfind(prefix, 0u) == 0u)
      return true;
    entry.increment(error);
    if (error)
      return true;
  }
  return false;
}

class Export final : public dx_iface {
public:
  Export(dxfRW &output, const std::vector<std::uint8_t> &payload)
      : output(output), payload(payload) {
    cData = &storage;
    currentBlock = storage.mBlock;
    dxfW = &output;
    geometry.eType = DRW::E3DSOLID;
    geometry.handle = 0xFC00u;
    geometry.parentHandle = DRW::DxfModelSpaceBlockRecordHandle;
    geometry.m_modelerVersion = 1u;
    geometry.m_rawBytes = payload;
    geometry.m_dxfPayloadChunks = {
        {1, 0u, 255u}, {3, 255u, 255u}, {3, 510u, payload.size() - 510u}};
  }

  void writeEntities() override {
    written = output.writeModelerGeometry(&geometry);
    if (!written && !allocationTracking)
      std::cerr << "modeler whole-write callback failed\n";
  }

  bool finalizeDxfWrite() override { return written; }

  dxfRW &output;
  dx_data storage;
  std::vector<std::uint8_t> payload;
  DRW_ModelerGeometry geometry;
  bool written = false;
};

bool callerMatches(const Export &actual,
                   const std::vector<std::uint8_t> &payload) {
  const DRW_ModelerGeometry &value = actual.geometry;
  return value.eType == DRW::E3DSOLID && value.handle == 0xFC00u &&
         value.parentHandle == DRW::DxfModelSpaceBlockRecordHandle &&
         value.m_modelerVersion == 1u && value.m_rawBytes == payload &&
         value.m_dxfPayloadChunks.size() == 3u &&
         value.m_dxfPayloadChunks[0].m_groupCode == 1 &&
         value.m_dxfPayloadChunks[0].m_offset == 0u &&
         value.m_dxfPayloadChunks[0].m_length == 255u &&
         value.m_dxfPayloadChunks[1].m_groupCode == 3 &&
         value.m_dxfPayloadChunks[1].m_offset == 255u &&
         value.m_dxfPayloadChunks[1].m_length == 255u &&
         value.m_dxfPayloadChunks[2].m_groupCode == 3 &&
         value.m_dxfPayloadChunks[2].m_offset == 510u &&
         value.m_dxfPayloadChunks[2].m_length == payload.size() - 510u;
}

bool readbackMatches(const std::filesystem::path &path,
                     const std::vector<std::uint8_t> &payload,
                     DRW::Version version) {
  dx_data data;
  dx_iface importer;
  if (!importer.fileImport(path.string(), &data, false) ||
      data.mBlock == nullptr || data.sourceVersion != version) {
    std::cerr << "modeler readback fileImport failed\n";
    return false;
  }
  const DRW_ModelerGeometry *modeler = nullptr;
  for (const DRW_Entity *entity : data.mBlock->ent) {
    if (entity == nullptr || entity->eType != DRW::E3DSOLID)
      continue;
    if (modeler != nullptr)
      return false;
    modeler = dynamic_cast<const DRW_ModelerGeometry *>(entity);
  }
  if (modeler == nullptr)
    return false;
  const DRW_ModelerGeometry &value = *modeler;
  std::vector<std::uint8_t> expected = payload;
  expected.push_back(static_cast<std::uint8_t>('\n'));
  std::vector<std::uint8_t> recovered;
  for (const DRW_ModelerPayloadChunk &chunk : value.m_dxfPayloadChunks) {
    if ((chunk.m_groupCode != 1 && chunk.m_groupCode != 3) ||
        chunk.m_offset > value.m_rawBytes.size() ||
        chunk.m_length > value.m_rawBytes.size() - chunk.m_offset)
      return false;
    recovered.insert(
        recovered.end(),
        value.m_rawBytes.begin() + static_cast<std::ptrdiff_t>(chunk.m_offset),
        value.m_rawBytes.begin() +
            static_cast<std::ptrdiff_t>(chunk.m_offset + chunk.m_length));
  }
  const bool matches = value.eType == DRW::E3DSOLID && value.handle != 0u &&
                       value.handle != DRW::NoHandle &&
                       value.m_rawBytes == expected && recovered == payload &&
                       value.m_dxfPayloadChunks.size() == 3u &&
                       value.m_dxfPayloadChunks[0].m_groupCode == 1 &&
                       value.m_dxfPayloadChunks[1].m_groupCode == 3 &&
                       value.m_dxfPayloadChunks[2].m_groupCode == 3;
  if (!matches)
    std::cerr << "modeler readback mismatch type=" << value.eType
              << " handle=" << value.handle
              << " raw=" << value.m_rawBytes.size()
              << " expected=" << expected.size()
              << " recovered=" << recovered.size()
              << " chunks=" << value.m_dxfPayloadChunks.size() << '\n';
  return matches;
}

bool run(const std::filesystem::path &directory) {
  const auto fail = [](const char *message) {
    std::cerr << "modeler DXF allocation transaction: " << message << '\n';
    return false;
  };
  if (!std::filesystem::create_directories(directory) &&
      !std::filesystem::is_directory(directory))
    return fail("could not create scratch directory");
  const std::string sentinel = "prior modeler destination\n";
  const std::string satPayload =
      "ACIS SAT transaction profile with a long multi-chunk text carrier: ";
  std::vector<std::uint8_t> payload;
  payload.reserve(600u);
  while (payload.size() < 600u)
    payload.insert(payload.end(), satPayload.begin(), satPayload.end());
  payload.resize(600u);

  const std::vector<std::pair<DRW::Version, const char *>> versions = {
      {DRW::AC1014, "AC1014"},
      {DRW::AC1015, "AC1015"},
      {DRW::AC1018, "AC1018"},
      {DRW::AC1021, "AC1021"},
      {DRW::AC1024, "AC1024"}};
  for (const auto &[version, versionName] : versions) {
    for (const bool binary : {false, true}) {
      const auto output =
          directory / (std::string("modeler-sat-") + versionName +
                       (binary ? "-allocation-transaction-binary.dxf"
                               : "-allocation-transaction-ascii.dxf"));
      {
        dxfRW codec(output.string().c_str());
        Export adapter(codec, payload);
        if (!codec.write(&adapter, version, binary) || !adapter.written ||
            !readbackMatches(output, payload, version))
          return fail("warm-up write or typed readback failed");
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
      }

      long allocationCount = 0;
      std::string expectedBytes;
      {
        dxfRW codec(output.string().c_str());
        Export adapter(codec, payload);
        armAllocationFailure(1000000000L);
        const bool wrote = codec.write(&adapter, version, binary);
        allocationCount = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
        const bool callerState = callerMatches(adapter, payload);
        const bool bytesAvailable = readBytes(output, expectedBytes);
        const bool typedReadback =
            bytesAvailable && readbackMatches(output, payload, version);
        if (!wrote || !adapter.written || !callerState || !typedReadback) {
          std::cerr << "modeler calibration wrote=" << wrote
                    << " callback=" << adapter.written
                    << " caller=" << callerState << " bytes=" << bytesAvailable
                    << " readback=" << typedReadback
                    << " attempts=" << allocationAttempts
                    << " diagnostic=" << codec.getLastDiagnostic().code << ": "
                    << codec.getLastDiagnostic().message << '\n';
          return fail("calibration write or typed readback failed");
        }
      }
      if (allocationCount <= 0)
        return fail("calibration observed no allocations");

      for (long offset = 0; offset < allocationCount; ++offset) {
        {
          std::ofstream prior(output, std::ios::binary | std::ios::trunc);
          prior << sentinel;
          if (!prior.good())
            return fail("could not seed prior destination");
        }
        dxfRW codec(output.string().c_str());
        Export adapter(codec, payload);
        armAllocationFailure(offset);
        bool wrote = false;
        try {
          wrote = codec.write(&adapter, version, binary);
        } catch (const std::bad_alloc &) {
          wrote = false;
        }
        const bool injected = allocationFailed;
        disarmAllocationFailure();
        std::string afterFailure;
        const bool bytesRead = readBytes(output, afterFailure);
        const auto &context = *codec.getWritingContext();
        const bool identitiesRolledBack =
            context.sourceHandleToMintedMap.empty() &&
            context.sourceHandleToWrittenObjectMap.empty() &&
            context.writtenSourceEntityHandles.empty() &&
            context.ambiguousSourceHandles.empty();
        const bool callerState = callerMatches(adapter, payload);
        const bool destinationState = bytesRead && afterFailure == sentinel;
        const bool stagingState = hasStagedOutput(output);
        if (wrote || !injected || !destinationState || !callerState ||
            !identitiesRolledBack || stagingState) {
          std::cerr << "modeler failure offset=" << offset << " wrote=" << wrote
                    << " injected=" << injected
                    << " destination=" << destinationState
                    << " caller=" << callerState
                    << " identities=" << identitiesRolledBack
                    << " staging=" << stagingState
                    << " callback=" << adapter.written
                    << " attempts=" << allocationAttempts << '\n';
          return fail("injected failure changed caller/destination state");
        }

        const bool retried = codec.write(&adapter, version, binary);
        std::string retryBytes;
        if (!retried || !adapter.written || !callerMatches(adapter, payload) ||
            !readBytes(output, retryBytes) || retryBytes != expectedBytes ||
            !readbackMatches(output, payload, version) ||
            hasStagedOutput(output))
          return fail("same-codec retry or typed readback failed");
      }
      std::cout << "Modeler SAT text " << versionName << ' '
                << (binary ? "Binary" : "ASCII")
                << ": allocation offsets=" << allocationCount
                << ", caller/destination rollback, retry/readback=PASS\n";
    }
  }
  return true;
}
} // namespace

void *operator new(std::size_t size) {
  if (failAllocation())
    throw std::bad_alloc();
  if (void *value = std::malloc(size == 0u ? 1u : size))
    return value;
  throw std::bad_alloc();
}

void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *value) noexcept { std::free(value); }
void operator delete[](void *value) noexcept { std::free(value); }
void operator delete(void *value, std::size_t) noexcept { std::free(value); }
void operator delete[](void *value, std::size_t) noexcept { std::free(value); }

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: modeler_dxf_allocation_tests SCRATCH_DIR\n";
    return 2;
  }
  return run(argv[1]) ? 0 : 1;
}
