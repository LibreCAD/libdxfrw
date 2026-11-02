#include "dx_iface.h"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <vector>
static long allocations = -1;
void *operator new(std::size_t n) {
  if (allocations == 0)
    throw std::bad_alloc();
  if (allocations > 0)
    --allocations;
  if (auto p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
using S = DRW_DxfEntitiesSource;
using M = DRW_DxfEntitiesDeliverySource;
using T = dxfRW::DxfEntitiesDeliveryTicketPtr;
class DxIfaceStandardDwgCaptureTestAccess {
public:
  static bool fault(dxfRW &r, const S &s, const M &m, long n, bool manifest,
                    bool &failed) {
    r.beginDxfEntitiesDelivery();
    std::size_t index = (std::numeric_limits<std::size_t>::max)();
    if (manifest) {
      index = r.captureDxfEntitiesSource(s);
      if (r.m_dxfEntitiesDelivery.state.failed)
        return false;
    }
    allocations = n;
    if (manifest)
      r.captureDxfEntitiesManifest(index, m);
    else
      index = r.captureDxfEntitiesSource(s);
    allocations = -1;
    failed = r.m_dxfEntitiesDelivery.state.failed;
    if (failed)
      return !r.m_dxfEntitiesDelivery.state.complete &&
             (manifest ? !r.m_dxfEntitiesDelivery.scopes[index].delivered
                       : r.m_dxfEntitiesDelivery.scopes.empty());
    return index < r.m_dxfEntitiesDelivery.scopes.size() &&
           (!manifest || r.m_dxfEntitiesDelivery.scopes[index].delivered);
  }
};
struct Receiver : dx_iface {
  dxfRW *codec = nullptr;
  bool source = true, delivery = true, reenter = false, rejected = false;
  int phase = 0;
  std::vector<S> physical;
  std::vector<M> manifests;
  std::vector<T> tickets, mtickets;
  std::string order;
  bool wantsDxfEntitiesSource() const override { return source; }
  bool wantsDxfEntitiesDeliverySource() const override { return delivery; }
  void addHeader(const DRW_Header *) override {}
  void addPoint(const DRW_Point &) override {
    order += 'P';
    if (phase == 1)
      throw std::runtime_error("point");
  }
  void addDxfEntitiesSource(const S &v) override {
    order += 'S';
    tickets.push_back(codec->dxfEntitiesSourceTicket(v));
    physical.push_back(v);
    if (reenter) {
      std::string nested;
      rejected = !codec->readAscii(this, false, nested) &&
                 !codec->enableDxfEntitiesDeliveryLedger();
    }
    if (phase == 2)
      throw std::runtime_error("source");
  }
  void addDxfEntitiesDeliverySource(const M &v) override {
    order += 'M';
    mtickets.push_back(codec->dxfEntitiesManifestTicket(v));
    manifests.push_back(v);
    if (phase == 3)
      throw std::runtime_error("manifest");
  }
  void reset() {
    physical.clear();
    manifests.clear();
    tickets.clear();
    mtickets.clear();
    order.clear();
  }
};
const std::string doc =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1018\n9\n$DWGCODEPAGE\n3\nANSI_"
    "1252\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nPOINT\n5\n101\n8\n0\n10\n1."
    "0\n70\n7\n999\ncomment\n20\n2\n0\nENDSEC\n0\nSECTION\n"
    "2\nENTITIES\n0\nPOINT\n5\n102\n8\n0\n10\n3\n20\n4\n0\nENDSEC\n0\nEOF\n";
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      std::cerr << "fail line " << __LINE__ << " " << #x << "\n";              \
      return 1;                                                                \
    }                                                                          \
  } while (0)
int main() {
  dxfRW reader("/tmp/unused.dxf");
  Receiver h;
  h.codec = &reader;
  CHECK(reader.enableDxfEntitiesDeliveryLedger());
  std::string in = doc;
  CHECK(reader.readAscii(&h, false, in));
  auto state = reader.dxfEntitiesDeliveryState();
  CHECK(state.complete && !state.failed && state.scopes == 2 &&
        state.deliveryScopes == 2 && h.order == "PSMPSM");
  for (int i = 0; i < 2; ++i) {
    CHECK(h.tickets[i] && h.mtickets[i] && h.tickets[i] == h.mtickets[i]);
    CHECK(reader.dxfEntitiesSourceMatches(h.physical[i], h.tickets[i]));
    CHECK(reader.dxfEntitiesManifestMatches(h.manifests[i], h.tickets[i]));
    CHECK(!reader.dxfEntitiesSourceMatches(h.physical[i], h.tickets[1 - i]));
    CHECK(!reader.dxfEntitiesManifestMatches(h.manifests[i], h.tickets[1 - i]));
  }
  // An aliased pointer need not point to a ticket at all. Neither match
  // method may dereference it before establishing the codec-owned identity.
  const auto fakePointer =
      reinterpret_cast<const dxfRW::DxfEntitiesDeliveryTicket *>(
          static_cast<std::uintptr_t>(1));
  const T badPointer(h.tickets[0], fakePointer);
  const T badOwner(std::make_shared<int>(7), h.tickets[0].get());
  const T emptyOwner(std::shared_ptr<void>{}, h.tickets[0].get());
  const T emptyOwnerBadPointer(std::shared_ptr<void>{}, fakePointer);
  CHECK(!reader.dxfEntitiesSourceMatches(h.physical[0], badPointer));
  CHECK(!reader.dxfEntitiesManifestMatches(h.manifests[0], badPointer));
  CHECK(!reader.dxfEntitiesSourceMatches(h.physical[0], badOwner));
  CHECK(!reader.dxfEntitiesManifestMatches(h.manifests[0], badOwner));
  CHECK(!reader.dxfEntitiesSourceMatches(h.physical[0], emptyOwner));
  CHECK(!reader.dxfEntitiesManifestMatches(h.manifests[0], emptyOwner));
  CHECK(!reader.dxfEntitiesSourceMatches(h.physical[0], emptyOwnerBadPointer));
  CHECK(
      !reader.dxfEntitiesManifestMatches(h.manifests[0], emptyOwnerBadPointer));
  auto old = h.tickets[0];
  {
    dxfRW other("/tmp/other.dxf");
    Receiver v;
    v.codec = &other;
    CHECK(other.enableDxfEntitiesDeliveryLedger());
    std::string x = doc;
    CHECK(other.readAscii(&v, false, x));
    CHECK(!other.dxfEntitiesSourceMatches(h.physical[0], old));
    CHECK(!reader.dxfEntitiesSourceMatches(v.physical[0], v.tickets[0]));
  }
  unsigned mutations = 0;
  const auto phys = [&](const std::function<void(S &)> &f) {
    auto v = h.physical[0];
    f(v);
    ++mutations;
    return !reader.dxfEntitiesSourceMatches(v, h.tickets[0]);
  };
  CHECK(phys([](S &v) { v.status = S::Status::Quota; }));
  CHECK(phys([](S &v) { v.version = DRW::AC1015; }));
  CHECK(phys([](S &v) { v.binary = !v.binary; }));
  CHECK(phys([](S &v) { ++v.scopeId; }));
  CHECK(phys([](S &v) { ++v.groups; }));
  CHECK(phys([](S &v) { ++v.bytes; }));
  CHECK(phys([](S &v) { ++v.work; }));
  CHECK(phys([](S &v) { ++v.ignoredComments; }));
  for (int where = 0; where < 4; ++where)
    for (int field = 0; field < 4; ++field)
      CHECK(phys([=](S &v) {
        auto *p = where == 0   ? &v.beginMarker
                  : where == 1 ? &v.endMarker
                  : where == 2 ? &v.records[0].marker
                               : &v.records[0].spans[0];
        if (field == 0)
          ++p->ordinal;
        else if (field == 1)
          ++p->beginOffset;
        else if (field == 2)
          ++p->endOffset;
        else
          p->ignoredComment = !p->ignoredComment;
      }));
  CHECK(phys([](S &v) { v.records[0].name += 'X'; }));
  CHECK(phys([](S &v) { ++v.records[0].endOrdinal; }));
  CHECK(phys([](S &v) { ++v.records[0].endOffset; }));
  CHECK(phys([](S &v) { v.records[0].rawValues[0] += '0'; }));
  CHECK(phys([](S &v) { v.records[0].groups[0].addString(5, "other"); }));
  CHECK(phys([](S &v) {
    auto &g = v.records[0].groups[0];
    g.addString(g.code() + 1, *g.content.s);
  }));
  CHECK(phys([](S &v) {
    for (auto &g : v.records[0].groups)
      if (g.type() == DRW_Variant::DOUBLE) {
        g.addDouble(g.code(), g.content.d + 1.);
        return;
      }
  }));
  CHECK(phys([](S &v) {
    for (auto &g : v.records[0].groups)
      if (g.type() == DRW_Variant::INTEGER) {
        g.addInt(g.code(), g.content.i + 1);
        return;
      }
  }));
  CHECK(phys([](S &v) {
    auto &g = v.records[0].groups[0];
    g.setLayerRefName(*g.content.s);
  }));
  CHECK(phys([](S &v) {
    for (auto &g : v.records[0].groups)
      if (g.type() == DRW_Variant::DOUBLE) {
        int c = g.code();
        double d = g.content.d;
        g.addString(c, "hidden");
        g.addDouble(c, d);
        return;
      }
  }));
  CHECK(phys([](S &v) {
    for (auto &g : v.records[0].groups)
      if (g.type() == DRW_Variant::DOUBLE) {
        int c = g.code();
        double d = g.content.d;
        g.addBinary(c, {1u});
        g.addDouble(c, d);
        return;
      }
  }));
  CHECK(phys([](S &v) {
    for (auto &g : v.records[0].groups)
      if (g.type() == DRW_Variant::DOUBLE) {
        int c = g.code();
        double d = g.content.d;
        DRW_Coord x;
        x.x = 1.;
        g.addCoord(c, x);
        g.addDouble(c, d);
        return;
      }
  }));
  CHECK(phys([](S &v) { v.records[0].groups.pop_back(); }));
  CHECK(phys([](S &v) { v.records[0].rawValues.pop_back(); }));
  CHECK(phys([](S &v) { v.records[0].spans.pop_back(); }));
  CHECK(phys([](S &v) { v.records.clear(); }));
  CHECK(phys([](S &v) { v.records.push_back(v.records[0]); }));
  CHECK(phys(
      [](S &v) { v.records[0].groups.push_back(v.records[0].groups[0]); }));
  CHECK(phys([](S &v) {
    v.records[0].rawValues.push_back(v.records[0].rawValues[0]);
  }));
  CHECK(
      phys([](S &v) { v.records[0].spans.push_back(v.records[0].spans[0]); }));
  const auto mani = [&](const std::function<void(M &)> &f) {
    auto v = h.manifests[0];
    f(v);
    ++mutations;
    return !reader.dxfEntitiesManifestMatches(v, h.tickets[0]);
  };
  CHECK(mani([](M &v) { v.status = M::Status::Quota; }));
  CHECK(mani([](M &v) { v.version = DRW::AC1015; }));
  CHECK(mani([](M &v) { v.binary = !v.binary; }));
  CHECK(mani([](M &v) { ++v.scopeId; }));
  CHECK(mani([](M &v) { ++v.bytes; }));
  CHECK(mani([](M &v) { ++v.work; }));
  CHECK(mani([](M &v) { ++v.completedEvents; }));
  for (int where = 0; where < 4; ++where)
    for (int field = 0; field < 4; ++field)
      CHECK(mani([=](M &v) {
        auto *p = where == 0   ? &v.beginMarker
                  : where == 1 ? &v.endMarker
                  : where == 2 ? &v.dispatches[0].begin
                               : &v.dispatches[0].end;
        if (field == 0)
          ++p->ordinal;
        else if (field == 1)
          ++p->beginOffset;
        else if (field == 2)
          ++p->endOffset;
        else
          p->ignoredComment = !p->ignoredComment;
      }));
  CHECK(mani([](M &v) { ++v.dispatches[0].firstEvent; }));
  CHECK(mani([](M &v) { ++v.dispatches[0].events; }));
  CHECK(mani([](M &v) { ++v.events[0].dispatch; }));
  CHECK(mani(
      [](M &v) { v.events[0].callback = DRW_DxfEntitiesCallback::Count; }));
  CHECK(mani([](M &v) {
    v.events[0].origin = DRW_DxfEntitiesDeliveryEvent::Origin::ProxyRendering;
  }));
  CHECK(mani([](M &v) { ++v.events[0].entityType; }));
  CHECK(mani([](M &v) { ++v.events[0].handle; }));
  CHECK(mani([](M &v) { ++v.events[0].owner; }));
  CHECK(mani([](M &v) { v.events[0].hasValue = !v.events[0].hasValue; }));
  CHECK(mani([](M &v) { v.dispatches.clear(); }));
  CHECK(mani([](M &v) { v.events.clear(); }));
  CHECK(mani([](M &v) { v.dispatches.push_back(v.dispatches[0]); }));
  CHECK(mani([](M &v) { v.events.push_back(v.events[0]); }));
  for (int phase = 1; phase <= 3; ++phase) {
    h.reset();
    h.phase = phase;
    in = doc;
    bool threw = false;
    try {
      (void)reader.readAscii(&h, false, in);
    } catch (const std::runtime_error &) {
      threw = true;
    }
    CHECK(threw);
    CHECK(!reader.dxfEntitiesDeliveryState().complete);
    h.reset();
    h.phase = 0;
    in = doc;
    CHECK(reader.readAscii(&h, false, in));
    CHECK(reader.dxfEntitiesSourceMatches(h.physical[0], h.tickets[0]));
    CHECK(!reader.dxfEntitiesSourceMatches(h.physical[0], old));
  }
  h.reset();
  h.reenter = true;
  h.rejected = false;
  in = doc;
  CHECK(reader.readAscii(&h, false, in));
  CHECK(h.rejected &&
        reader.dxfEntitiesSourceMatches(h.physical[0], h.tickets[0]));
  h.reenter = false;
  h.reset();
  in = doc.substr(0, doc.find("0\nSECTION\n2\nENTITIES",
                              doc.find("0\nSECTION\n2\nENTITIES") + 1));
  CHECK(!reader.readAscii(&h, false, in));
  CHECK(!reader.dxfEntitiesDeliveryState().complete);
  CHECK(!reader.dxfEntitiesSourceMatches(h.physical[0], h.tickets[0]));
  h.reset();
  in = doc;
  CHECK(reader.readAscii(&h, false, in));
  CHECK(reader.dxfEntitiesSourceMatches(h.physical[0], h.tickets[0]));
  unsigned faults = 0;
  for (bool manifest : {false, true}) {
    bool success = false;
    for (long n = 0; n < 100 && !success; ++n) {
      bool failed = false;
      CHECK(DxIfaceStandardDwgCaptureTestAccess::fault(
          reader, h.physical[0], h.manifests[0], n, manifest, failed));
      if (failed)
        ++faults;
      else
        success = true;
    }
    CHECK(success);
  }
  CHECK(faults > 2);
  // Calibrate exact aggregate limits, then test each independently one below.
  h.reset();
  h.phase = 0;
  in = doc;
  CHECK(reader.readAscii(&h, false, in));
  auto exact = reader.dxfEntitiesDeliveryState();
  for (int dim = 0; dim < 7; ++dim) {
    const std::size_t used[] = {exact.scopes,     exact.records, exact.groups,
                                exact.dispatches, exact.events,  exact.bytes,
                                exact.work};
    std::size_t limits[] = {1024u,
                            100000u,
                            4000000u,
                            100000u,
                            4000000u,
                            64u * 1024u * 1024u,
                            128u * 1024u * 1024u};
    CHECK(used[dim] > 0);
    limits[dim] = used[dim] - 1u;
    CHECK(reader.enableDxfEntitiesDeliveryLedger(
        limits[0], limits[1], limits[2], limits[3], limits[4], limits[5],
        limits[6]));
    h.reset();
    in = doc;
    CHECK(reader.readAscii(&h, false, in));
    CHECK(reader.dxfEntitiesDeliveryState().failed &&
          !reader.dxfEntitiesDeliveryState().complete);
    CHECK(reader.enableDxfEntitiesDeliveryLedger());
    h.reset();
    in = doc;
    CHECK(reader.readAscii(&h, false, in));
    CHECK(reader.dxfEntitiesSourceMatches(h.physical[0], h.tickets[0]));
  }
  CHECK(reader.enableDxfEntitiesDeliveryLedger(
      1024u, exact.records, exact.groups, exact.dispatches, exact.events,
      exact.bytes, exact.work));
  h.reset();
  in = doc;
  CHECK(reader.readAscii(&h, false, in));
  CHECK(reader.dxfEntitiesDeliveryState().complete);
  h.reset();
  h.delivery = false;
  in = doc;
  CHECK(reader.readAscii(&h, false, in));
  state = reader.dxfEntitiesDeliveryState();
  CHECK(state.complete && state.scopes == 2 && state.deliveryScopes == 0 &&
        h.order == "PSPS" &&
        reader.dxfEntitiesSourceMatches(h.physical[0], h.tickets[0]));
  h.reset();
  h.source = false;
  in = doc;
  CHECK(reader.readAscii(&h, false, in));
  state = reader.dxfEntitiesDeliveryState();
  CHECK(state.complete && state.scopes == 0 && state.deliveryScopes == 0 &&
        h.order == "PP");
  std::cout
      << "mutations=" << mutations
      << " sections=2 forged_tickets=8 cross_reader=1 reentrant=1 "
         "partial_retry=1 "
         "exception_retries=3 allocation_failures="
      << faults
      << " quota_retries=7 source_only=1 default_opt_out=1 order=PSMPSM\n";
  return 0;
}
