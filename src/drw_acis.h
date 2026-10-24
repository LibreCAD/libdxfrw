/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

//! In-library ACIS SAB (Standard ACIS Binary) wireframe decoder.
/*!
 *  Decodes the byte-true SAB stream captured from DXF/DWG modeler payloads into
 *  a lightweight B-rep wireframe: vertices (3D points), edges (segment endpoints
 *  + analytic curve params) and faces (surface type/params + loop structure).
 *
 *  This is a Qt-free, RS_*-free port of the TypeScript reference parser
 *  (d:/data/dli/dwg-parser/src/dwg/acis/: sab.ts, entities.ts, geometry.ts).
 *  Three layers:
 *    1. DRW_SabReader / drw_parseSab   — SAB tag byte stream -> record list.
 *    2. DRW_AcisModel / drw_buildAcisModel — resolve POINTER tokens into a graph.
 *    3. drw_extractAcisWireframe        — walk the graph into a DRW_AcisBrep.
 *  drw_decodeAcisWireframe() is the convenience entry: locate the SAB signature
 *  in a raw blob, then parse -> build -> extract. It NEVER throws; malformed,
 *  incomplete or non-SAB input yields an empty DRW_AcisBrep and a false return.
 *
 *  Tessellation, SAT text and NURBS evaluation are intentionally out of scope.
 */

#ifndef DRW_ACIS_H
#define DRW_ACIS_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "drw_base.h"

#if defined(__APPLE__) && defined(__MACH__)
#define LIBDXFRW_EXPORT __attribute__((visibility("default")))
#else
#define LIBDXFRW_EXPORT
#endif

// ── SAB low-level token/record model (sab.ts) ──────────────────────────────

//! SAB tag bytes (ezdxf `Tags`).
namespace DRW_SabTag {
    enum Tag {
        NoType = 0x00, Byte = 0x01, Char = 0x02, Short = 0x03, Int = 0x04,
        Float = 0x05, Double = 0x06, Str = 0x07, Str2 = 0x08, Str3 = 0x09,
        BoolTrue = 0x0a, BoolFalse = 0x0b, Pointer = 0x0c, EntityType = 0x0d,
        EntityTypeEx = 0x0e, SubtypeStart = 0x0f, SubtypeEnd = 0x10,
        RecordEnd = 0x11, LiteralStr = 0x12, LocationVec = 0x13,
        DirectionVec = 0x14, Enum = 0x15, Unknown0x17 = 0x17,
    };
}

//! A single decoded SAB field token: a tag plus its typed value.
/*!
 *  Only one of the value slots is meaningful, selected by `tag`:
 *  Double/Unknown0x17 -> dval; Int/Pointer/Enum/SubtypeStart/SubtypeEnd -> ival;
 *  BoolTrue/BoolFalse -> bval; Str/LiteralStr/EntityType/EntityTypeEx -> sval;
 *  LocationVec/DirectionVec -> vec.
 */
struct DRW_SabToken {
    int tag = DRW_SabTag::NoType;
    double dval = 0.0;
    long long ival = 0;
    bool bval = false;
    std::string sval;
    DRW_Coord vec;
};

//! A decoded SAB record: its entity-type identity plus all field tokens.
struct DRW_SabRecord {
    std::string type;                    //!< record identity = first entity-type
    std::vector<DRW_SabToken> tokens;    //!< all field tokens in wire order
};

//! SAB stream header (identity, source-unit metadata, and tolerances).
struct DRW_SabHeader {
    std::string signature;
    int version = 0;
    int numRecords = 0;
    int numEntities = 0;
    int flags = 0;
    std::string productId;
    std::string acisVersion;
    std::string creationDate;
    double unitsInMm = 1.0;
    double resTol = 0.0;
    double norTol = 0.0;
};

//! A fully decoded SAB stream: header + record list.
struct DRW_SabData {
    DRW_SabHeader header;
    std::vector<DRW_SabRecord> records;
};

//! Operational resource ceilings for the optional in-library SAB decoder.
//! These are not SAB format limits. Graph/output items and parser vector
//! capacity ceilings bound logical item slots, not allocator overhead or RSS.
//! Increase them explicitly for larger trusted models; raw carrier
//! preservation is independent of these defaults.
struct DRW_SabParseLimits {
    std::size_t maxInputBytes = 64u * 1024u * 1024u;
    std::size_t maxRecords = 100000u;
    std::size_t maxTags = 250000u;
    std::size_t maxStringBytes = 1024u * 1024u;
    std::size_t maxCumulativeStringBytes = 8u * 1024u * 1024u;
    std::size_t maxSubtypeDepth = 64u;
    std::size_t maxGraphWorkUnits = 1000000u;
    std::size_t maxGraphItems = 1000000u;
    std::size_t maxExtractionWorkUnits = 1000000u;
    std::size_t maxOutputItems = 500000u;
    std::size_t maxTokenCapacityItems = 500000u;
    std::size_t maxTypePartCapacityItems = 250000u;
};

//! Byte-framing and parse/projection-budget outcome, measured from the input
//! supplied to the operation. For standalone graph building, sourceByteCount
//! and framing fields remain zero/false.
struct DRW_SabParseOutcome {
    std::size_t sourceByteCount = 0;
    std::size_t consumedByteCount = 0;
    std::size_t trailingByteCount = 0;
    std::size_t recordsProcessed = 0; //!< budget-admitted record starts
    std::size_t tagsProcessed = 0;    //!< budget-admitted tagged values
    std::size_t stringBytesAccounted = 0; //!< includes temporary name joins
    std::size_t graphWorkUnitsProcessed = 0; //!< graph copy/reference visits
    std::size_t graphItemsAccounted = 0; //!< nodes, token copies, refs, planner slots
    std::size_t extractionWorkUnitsProcessed = 0; //!< projection/revisit work
    std::size_t outputItemsAccounted = 0; //!< projected items and cycle guards
    std::size_t tokenCapacityItemsAccounted = 0; //!< aggregate token-vector slots
    std::size_t typePartCapacityPeakItems = 0; //!< per-record temporary slots
    bool terminalRecordSeen = false;
    bool recordCountKnown = false;
    bool recordCountMatched = false;  //!< meaningful only when countKnown
    bool complete = false;
    bool resourceLimitExceeded = false;
    bool allocationFailure = false;
};

//! Decode a complete, byte-true SAB stream into a header + record list.
//! Returns false for malformed data, count mismatch or unconsumed suffix.
LIBDXFRW_EXPORT bool drw_parseSab(const std::uint8_t* data, std::size_t length, DRW_SabData& out);

//! Parse a structurally valid SAB prefix and report its exact byte boundary.
//! A true return does not imply complete=true when a suffix or required
//! profile terminator is absent; the caller must inspect the outcome. The
//! parsed prefix is retained in `out` on such incomplete-but-valid results.
LIBDXFRW_EXPORT bool drw_parseSabWithOutcome(
    const std::uint8_t* data, std::size_t length, DRW_SabData& out,
    DRW_SabParseOutcome& outcome);

//! Parse with caller-selected operational limits. Limits do not alter the
//! admitted SAB profile or relax the strict legacy wrapper's completeness
//! requirement.
LIBDXFRW_EXPORT bool drw_parseSabWithLimits(
    const std::uint8_t* data, std::size_t length, DRW_SabData& out,
    DRW_SabParseOutcome& outcome, const DRW_SabParseLimits& limits);

// ── B-rep record graph (entities.ts) ───────────────────────────────────────

struct DRW_AcisNode {
    int index = 0;                       //!< position in the record stream
    std::string type;                    //!< entity-type name
    DRW_SabRecord record;                //!< the decoded record (all tokens)
    std::vector<int> refs;               //!< resolved POINTER targets (-1 = null)
};

//! Resolved SAB record graph: nodes indexed by record position.
struct DRW_AcisModel {
    DRW_SabHeader header;
    std::vector<DRW_AcisNode> nodes;
    //! Node indices grouped by entity type (excludes end-of-data marker).
    std::vector<int> nodesOfType(const std::string& type) const;
};

//! Build the resolved B-rep graph from a decoded SAB stream.
DRW_AcisModel drw_buildAcisModel(const DRW_SabData& sab);

//! Build a resolved graph with caller-selected resource limits.
//! Outcome counters report graph admissions; `complete` remains a parser
//! framing field and is not set by this graph-only operation.
LIBDXFRW_EXPORT bool drw_buildAcisModelWithLimits(
    const DRW_SabData& sab, DRW_AcisModel& out,
    DRW_SabParseOutcome& outcome, const DRW_SabParseLimits& limits);

// ── Extracted wireframe geometry (geometry.ts) ─────────────────────────────

enum class DRW_AcisCurve { Straight, Ellipse, Intcurve, Unknown };
enum class DRW_AcisSurface { Plane, Cone, Torus, Sphere, Spline, Unknown };

//! A B-rep vertex; `valid` is false when its point could not be resolved.
struct DRW_AcisVertex {
    int nodeIndex = 0;
    //! Point record used for these coordinates; strict rooted SAB 21800 uses
    //! vertex pointer ordinal 2.
    int pointNodeIndex = -1;
    DRW_Coord point;
    bool valid = false;
};

//! A B-rep edge with resolved endpoints and analytic curve params.
/*!
 *  Curve param slots by curveType:
 *   - Straight: p0 = origin, p1 = direction.
 *   - Ellipse:  p0 = center, p1 = normal, p2 = majorAxis, ratio.
 *   - Intcurve: controlPoints (B-spline control polygon; may be empty).
 */
struct DRW_AcisEdge {
    int nodeIndex = 0;
    //! Curve source node used for these fields; rooted SAB 21800 uses ordinal 4.
    int curveNodeIndex = -1;
    //! Edge's ordinal-3 coedge pointer target in SAB 21800; -1 if unresolved.
    int coedgeNodeIndex = -1;
    //! True only when that coedge's ordinal-4 edge link points back here.
    bool hasReciprocalCoedgeLink = false;
    //! Start vertex node identity; rooted SAB 21800 uses ordinal 1.
    int startVertexNodeIndex = -1;
    //! End vertex node identity; rooted SAB 21800 uses ordinal 2.
    int endVertexNodeIndex = -1;
    DRW_Coord start;
    DRW_Coord end;
    bool hasStart = false;
    bool hasEnd = false;
    DRW_AcisCurve curveType = DRW_AcisCurve::Unknown;
    bool hasCurve = false;               //!< true when curve params were resolved
    DRW_Coord p0;
    DRW_Coord p1;
    DRW_Coord p2;
    double ratio = 0.0;
    std::vector<DRW_Coord> controlPoints;
    //! True when the admitted SAB 21800 profile exposed the partner links.
    bool hasCoedgePartnerRingObservation = false;
    //! True only when the selected coedge's partner walk returns to itself.
    bool coedgePartnerRingClosed = false;
    //! Unique coedge nodes visited from the selected coedge in partner order.
    std::vector<int> coedgePartnerRingNodeIndices;
    //! Exact partner targets aligned with coedgePartnerRingNodeIndices.
    std::vector<int> coedgePartnerRingNextNodeIndices;
    //! True when ordinal-4 coedge->edge references were indexed in SAB 21800.
    bool hasCoedgeEdgeUseIndexObservation = false;
    //! True when every parsed coedge has a well-typed ordinal-4 edge target.
    bool coedgeEdgeUseIndexCoversParsedCoedges = false;
    //! Parsed coedges whose ordinal-4 edge target is absent or not an edge.
    std::size_t coedgeEdgeUseIndexUnclassifiedCoedgeCount = 0u;
    //! First unclassified coedge node index, or -1 when there is none.
    int firstUnclassifiedCoedgeEdgeUseNodeIndex = -1;
    //! True when a complete parsed-model census equals the closed partner walk.
    bool coedgePartnerRingMatchesEdgeUseIndex = false;
    //! Source-ordered coedge nodes whose exact ordinal-4 target is this edge.
    std::vector<int> coedgeEdgeUseNodeIndices;
};

//! One loop of a face with its coedge nodes in source next-link order.
struct DRW_AcisFaceLoop {
    int nodeIndex = 0;
    int coedgeCount = 0;
    //! Exact coedge node identities; not sorted by record index.
    std::vector<int> coedgeNodeIndices;
    //! Next-link targets in coedgeNodeIndices order; -1 is unresolved.
    std::vector<int> coedgeNextNodeIndices;
    //! Previous-link targets in coedgeNodeIndices order; -1 is unresolved.
    std::vector<int> coedgePreviousNodeIndices;
    //! Adjacent-face partner targets in coedgeNodeIndices order; -1 is unresolved.
    std::vector<int> coedgePartnerNodeIndices;
    //! True when the admitted profile exposes links to classify closure/open state.
    bool hasCoedgeRingClosure = false;
    //! True only when the next-link walk returns to its starting coedge.
    bool coedgeRingClosed = false;
    //! True for a null-terminated chain with null outer links and reciprocal
    //! next/previous links between the retained coedges.
    bool coedgeRingOpen = false;
    //! True only when the retained next/previous links reciprocate throughout
    //! this observed closed ring or open chain.
    bool coedgeLinksReciprocal = false;
    //! Edge node per coedge, aligned with coedgeNodeIndices; -1 is unresolved.
    std::vector<int> coedgeEdgeNodeIndices;
    //! Raw edge endpoint vertex nodes, aligned with coedgeNodeIndices.
    //! These remain in edge order; the coedge sense is not applied.
    std::vector<int> coedgeEdgeStartVertexNodeIndices;
    std::vector<int> coedgeEdgeEndVertexNodeIndices;
    //! Coedge-traversal endpoint nodes, aligned with coedgeNodeIndices.
    //! Derived from raw edge endpoints and the coedge's sense relative to its
    //! edge; unknown sense leaves both endpoints unresolved.
    std::vector<int> coedgeOrientedStartVertexNodeIndices;
    std::vector<int> coedgeOrientedEndVertexNodeIndices;
    //! Owning loop link per coedge, aligned with coedgeNodeIndices.
    std::vector<int> coedgeLoopNodeIndices;
    //! Pcurve node per coedge, aligned with coedgeNodeIndices; -1 if absent.
    std::vector<int> coedgePCurveNodeIndices;
    //! Per-coedge orientation: -1 unknown, 0 forward, 1 reversed.
    std::vector<std::int8_t> coedgeSenseFlags;
    //! True when all endpoint joins in the observed reciprocal chain are
    //! classifiable from the retained endpoint identities and coedge senses.
    bool hasCoedgeEndpointChainObservation = false;
    //! True when every consecutive coedge endpoint identity is connected.
    bool coedgeEndpointChainContinuous = false;
    //! Known disconnected joins; the closing join is included for closed rings.
    std::size_t coedgeEndpointDiscontinuityCount = 0u;
    //! Coedge at the first disconnected join, or -1 when there is none.
    int firstCoedgeEndpointDiscontinuityNodeIndex = -1;
    //! True when exact ordinal-5 coedge->loop owners were indexed in SAB 21800.
    bool hasCoedgeUseIndexObservation = false;
    //! True when every parsed coedge has a well-typed ordinal-5 loop target.
    bool coedgeUseIndexCoversParsedCoedges = false;
    //! Parsed coedges whose ordinal-5 loop target is absent or not a loop.
    std::size_t coedgeUseIndexUnclassifiedCoedgeCount = 0u;
    //! First unclassified coedge node index, or -1 when there is none.
    int firstUnclassifiedCoedgeOwnerNodeIndex = -1;
    //! True when a complete parsed-model owner index equals this loop's walk.
    bool coedgeChainMatchesUseIndex = false;
    //! Source-ordered coedge nodes whose exact ordinal-5 target is this loop.
    std::vector<int> coedgeUseNodeIndices;
};

//! A B-rep face: surface type/params + loop structure.
/*!
 *  Surface param slots by surfaceType:
 *   - Plane: p0 = origin, p1 = normal, p2 = uDir (hasUDir).
 *   - Cone:  p0 = origin, p1 = axis, p2 = majorAxis, ratio, sineAngle, cosineAngle.
 *   - Torus: p0 = center, p1 = axis, p2 = uDir (hasUDir),
 *            majorRadius, minorRadius.
 *   - Sphere: p0 = center, p1 = pole axis, p2 = uDir (hasUDir),
 *             majorRadius = signed radius (legacy shared radius slot).
 */
struct DRW_AcisFace {
    int nodeIndex = 0;
    //! Owning shell node in the admitted rooted projection; -1 if unresolved.
    int shellNodeIndex = -1;
    //! Source surface node; -1 if unresolved or absent in this projection.
    int surfaceNodeIndex = -1;
    DRW_AcisSurface surfaceType = DRW_AcisSurface::Unknown;
    bool hasSurface = false;             //!< true when surface params were resolved
    DRW_Coord p0;
    DRW_Coord p1;
    DRW_Coord p2;
    bool hasUDir = false;                //!< whether the surface U direction (p2) was present
    double ratio = 0.0;
    double sineAngle = 0.0;
    double cosineAngle = 0.0;
    double majorRadius = 0.0;
    double minorRadius = 0.0;
    std::vector<DRW_AcisFaceLoop> loops;
    //! True when the admitted SAB profile observed the face's loop chain.
    bool hasLoopChainObservation = false;
    //! True only when the observed chain ends at an explicit null next link.
    bool loopChainNullTerminated = false;
    //! True when traversal revisits a loop before reaching a null next link.
    bool loopChainCycleDetected = false;
    //! First repeated loop node, or -1 when no cycle was observed.
    int firstRepeatedLoopChainNodeIndex = -1;
    //! Unclassifiable next links in the observed face chain.
    std::size_t loopChainUnclassifiedNextLinkCount = 0u;
    //! First loop with a missing/invalid next link, or -1 when none.
    int firstUnclassifiedLoopNextNodeIndex = -1;
    //! Exact next-link targets aligned with `loops`; -1 is null/unavailable.
    std::vector<int> loopNextNodeIndices;
    //! True when parsed-model loop->face references were indexed in SAB 21800.
    bool hasLoopUseIndexObservation = false;
    //! True when every parsed loop has a well-typed ordinal-3 face target.
    bool loopUseIndexCoversParsedLoops = false;
    //! Parsed loops whose ordinal-3 owner target is absent or not a face.
    std::size_t loopUseIndexUnclassifiedLoopCount = 0u;
    //! First unclassified loop node index, or -1 when there is none.
    int firstUnclassifiedLoopOwnerNodeIndex = -1;
    //! True when the observed face loop chain equals the parsed-model use row.
    bool faceLoopChainMatchesUseIndex = false;
    //! Source-ordered loop nodes whose exact ordinal-3 target is this face.
    std::vector<int> loopUseNodeIndices;
    //! Face normal relative to its surface; true means reversed.
    bool hasSense = false;
    bool senseReversed = false;
    //! ACIS double/single-sided state; only double-sided faces use containment.
    bool hasDoubleSided = false;
    bool doubleSided = false;
    //! For a double-sided face, true means the face is inside the body.
    bool hasContainment = false;
    bool containmentIn = false;
};

//! Structural face-chain observation for one shell in rooted SAB 21800.
//! This is parsed-model incidence only, not a claim of shell validity.
struct DRW_AcisShell {
    int nodeIndex = -1;
    //! Exact ordinal-5 parent lump identity, or -1 when not resolved.
    int lumpNodeIndex = -1;
    bool hasFaceChainObservation = false;
    bool faceChainNullTerminated = false;
    bool faceChainCycleDetected = false;
    int firstRepeatedFaceChainNodeIndex = -1;
    std::size_t faceChainUnclassifiedLinkCount = 0u;
    int firstUnclassifiedFaceLinkSourceNodeIndex = -1;
    //! Ordered face identities from the shell's ordinal-3 first-face link.
    std::vector<int> faceNodeIndices;
    //! Exact next-face targets aligned with faceNodeIndices; -1 is null.
    std::vector<int> faceNextNodeIndices;
    bool hasFaceUseIndexObservation = false;
    bool faceUseIndexCoversParsedFaces = false;
    std::size_t faceUseIndexUnclassifiedFaceCount = 0u;
    int firstUnclassifiedFaceOwnerNodeIndex = -1;
    //! True only when the observed chain equals the parsed face->shell index.
    bool faceChainMatchesUseIndex = false;
    //! Source-ordered parsed faces whose exact ordinal-3 owner is this shell.
    std::vector<int> faceUseNodeIndices;
};

//! Structural lump-to-shell census for rooted SAB 21800.
//! This is parsed-model incidence only, not a claim of lump validity.
struct DRW_AcisLump {
    int nodeIndex = -1;
    //! Exact ordinal-3 owning body identity, or -1 when unresolved.
    int bodyNodeIndex = -1;
    bool hasShellChainObservation = false;
    bool shellChainNullTerminated = false;
    bool shellChainCycleDetected = false;
    int firstRepeatedShellChainNodeIndex = -1;
    std::size_t shellChainUnclassifiedLinkCount = 0u;
    int firstUnclassifiedShellLinkSourceNodeIndex = -1;
    //! Ordered shell identities from the lump's ordinal-2 first-shell link.
    std::vector<int> shellNodeIndices;
    //! Exact next-shell targets aligned with shellNodeIndices; -1 is null.
    std::vector<int> shellNextNodeIndices;
    bool hasShellUseIndexObservation = false;
    bool shellUseIndexCoversParsedShells = false;
    std::size_t shellUseIndexUnclassifiedShellCount = 0u;
    int firstUnclassifiedShellOwnerNodeIndex = -1;
    //! True only when the observed chain equals the parsed shell->lump index.
    bool shellChainMatchesUseIndex = false;
    //! Source-ordered parsed shells whose ordinal-5 owner is this lump.
    std::vector<int> shellUseNodeIndices;
};

//! Decoded ACIS wireframe: vertices/edges/faces + bbox + diagnostics.
struct DRW_AcisBrep {
    std::vector<DRW_AcisVertex> vertices;
    std::vector<DRW_AcisEdge> edges;
    std::vector<DRW_AcisFace> faces;
    //! Populated by the rooted SAB 21800 projection; never implies validity.
    std::vector<DRW_AcisLump> lumps;
    //! Populated by the rooted SAB 21800 projection; never implies validity.
    std::vector<DRW_AcisShell> shells;
    bool hasBBox = false;
    DRW_Coord bboxMin;
    DRW_Coord bboxMax;
    std::vector<std::string> diagnostics;

    bool empty() const {
        return vertices.empty() && edges.empty() && faces.empty();
    }
};

//! Coordinate values currently exposed by the legacy SAB wireframe extractor.
//! This is deliberately not a body-local or drawing/WCS placement claim.
enum class DRW_AcisCoordinateSpace {
    RawRecordValues
};

//! Counts from the current record-inventory projection; edge/face counts
//! reflect the corresponding DRW_AcisEdge/DRW_AcisFace flags, not B-rep
//! validity or body-completeness results.
struct DRW_AcisProjectionReport {
    DRW_AcisCoordinateSpace coordinateSpace =
        DRW_AcisCoordinateSpace::RawRecordValues;
    std::size_t modelNodeCount = 0;
    std::size_t vertexCount = 0;
    std::size_t verticesWithoutResolvedPoint = 0;
    std::size_t edgeCount = 0;
    std::size_t edgesWithoutResolvedEndpointPair = 0;
    std::size_t edgesWithoutCurveFlag = 0;
    std::size_t faceCount = 0;
    std::size_t facesWithoutSurfaceFlag = 0;
    std::size_t facesWithoutLoop = 0;
    std::size_t unsupportedPatternNodeCount = 0;
    std::size_t unsupportedWireNodeCount = 0;
    std::size_t unsupportedSubshellNodeCount = 0;
    std::size_t unsupportedPCurveNodeCount = 0;
    std::size_t unsupportedCurveNodeCount = 0;
    std::size_t unsupportedSurfaceNodeCount = 0;
    bool bodyRootSelected = false;
    bool bodyTransformPresent = false;
    bool bodyTransformsApplied = false;
    bool sourceUnitScaleApplied = false;
    bool brepValidityChecked = false;
};

//! Raw-coordinate projection of the geometry reachable from one ACIS body.
struct DRW_AcisBodyGraphLink {
    int sourceNodeIndex = -1;
    std::size_t pointerOrdinal = 0;
    int targetNodeIndex = -1; //!< -1 preserves a null pointer slot.
};

//! Transform-token inventory state. ParsedUnapplied identifies only a bounded
//! twelve-number candidate profile; UnsupportedVariant preserves a different
//! bounded shape without interpreting it. Matrix interpretation remains a
//! later gate.
enum class DRW_AcisTransformState {
    Absent,
    NotParsed,
    ParsedUnapplied,
    Invalid,
    UnsupportedVariant
};

//! Loss-preserving decoded-token observation for a body's transform record.
struct DRW_AcisTransformObservation {
    int sourceNodeIndex = -1;
    DRW_AcisTransformState state = DRW_AcisTransformState::Absent;
    bool pointerResolved = false;
    bool rawLiteralPresent = false;
    std::size_t rawLiteralTokenIndex =
        std::numeric_limits<std::size_t>::max();
    std::string rawLiteral;
    //! Full decoded record token sequence; trailing hints remain uninterpreted.
    std::vector<DRW_SabToken> rawTokens;
    //! Twelve serialized numeric candidates; no row/column semantics assigned.
    bool candidateComponentsPresent = false;
    std::array<double, 12> candidateComponents {};
};

//! A bounded per-body projection plus the source-indexed pointer subgraph.
struct DRW_AcisBodyProjection {
    int bodyNodeIndex = -1;
    int bodyTransformNodeIndex = -1;
    bool bodyGraphProjected = false;
    //! Rooted SAB 21800 body ordinal-1 -> lump chain observation.
    bool hasLumpChainObservation = false;
    bool lumpChainNullTerminated = false;
    bool lumpChainCycleDetected = false;
    int firstRepeatedLumpChainNodeIndex = -1;
    std::size_t lumpChainUnclassifiedLinkCount = 0u;
    int firstUnclassifiedLumpLinkSourceNodeIndex = -1;
    //! Ordered lump identities and exact ordinal-1 targets; -1 is null.
    std::vector<int> lumpNodeIndices;
    std::vector<int> lumpNextNodeIndices;
    bool hasLumpUseIndexObservation = false;
    bool lumpUseIndexCoversParsedLumps = false;
    std::size_t lumpUseIndexUnclassifiedLumpCount = 0u;
    int firstUnclassifiedLumpOwnerNodeIndex = -1;
    bool lumpChainMatchesUseIndex = false;
    //! Source-ordered parsed lumps whose ordinal-3 owner is this body.
    std::vector<int> lumpUseNodeIndices;
    //! Unique nodes reached through the reviewed profile, in traversal order.
    std::vector<int> reachableNodeIndices;
    //! Pointer links retain source ordinals, target indices, and null slots.
    std::vector<DRW_AcisBodyGraphLink> graphLinks;
    //! Counts of functional next/partner cycles; these are preserved topology,
    //! not by themselves evidence of a valid B-rep.
    std::size_t nextListCycleCount = 0;
    std::size_t coedgeNextCycleCount = 0;
    std::size_t coedgePartnerCycleCount = 0;
    DRW_AcisTransformObservation transform;
    DRW_AcisBrep wireframe;
    DRW_AcisProjectionReport report;
};

//! Summary of coverage for the bounded rooted projection only.
enum class DRW_AcisProjectionCoverage {
    NotProjected,
    ProjectedSubsetNoKnownGaps,
    ProjectedSubsetWithKnownGaps
};

//! Classify known unsupported branches and unresolved projected geometry.
//! `ProjectedSubsetNoKnownGaps` is deliberately not a claim that the source
//! body is complete, transformed to WCS, unit-scaled, a valid B-rep, or
//! accepted by a geometry kernel/consumer. Those remain separate evidence
//! gates. This inline helper adds no data members or exported ABI symbols.
inline DRW_AcisProjectionCoverage drw_acisProjectionCoverage(
    const DRW_AcisBodyProjection& projection) noexcept {
    if (!projection.bodyGraphProjected || !projection.report.bodyRootSelected)
        return DRW_AcisProjectionCoverage::NotProjected;

    const DRW_AcisProjectionReport& report = projection.report;
    bool hasKnownGaps = report.unsupportedPatternNodeCount != 0u
        || report.unsupportedWireNodeCount != 0u
        || report.unsupportedSubshellNodeCount != 0u
        || report.unsupportedPCurveNodeCount != 0u
        || report.unsupportedCurveNodeCount != 0u
        || report.unsupportedSurfaceNodeCount != 0u
        || report.verticesWithoutResolvedPoint != 0u
        || report.edgesWithoutResolvedEndpointPair != 0u
        || report.edgesWithoutCurveFlag != 0u
        || report.facesWithoutSurfaceFlag != 0u
        || report.facesWithoutLoop != 0u;
    // A projected snapshot is rooted only when its root identity is valid and
    // present in the retained reachable row. The report count is a
    // cached copy of that row's size; stale or missing root metadata must not
    // be classified as a gap-free projection merely because geometry rows
    // remain populated.
    bool bodyRootReachable = false;
    for (int nodeIndex : projection.reachableNodeIndices) {
        if (nodeIndex < 0)
            hasKnownGaps = true;
        if (nodeIndex == projection.bodyNodeIndex)
            bodyRootReachable = true;
    }
    if (projection.bodyNodeIndex < 0 || !bodyRootReachable
        || projection.reachableNodeIndices.empty()
        || report.modelNodeCount != projection.reachableNodeIndices.size()
        || report.vertexCount != projection.wireframe.vertices.size()
        || report.edgeCount != projection.wireframe.edges.size()
        || report.faceCount != projection.wireframe.faces.size()) {
        hasKnownGaps = true;
    }
    // The three owner/first-child chains define the row cardinalities for
    // rooted lumps, shells, and faces. Cross-check those parent rows against
    // the separately published geometry rows so truncation or duplicate rows
    // cannot hide behind internally consistent per-row flags.
    if (projection.lumpNodeIndices.size()
        != projection.wireframe.lumps.size()) {
        hasKnownGaps = true;
    }
    const std::size_t maxRows = std::numeric_limits<std::size_t>::max();
    std::size_t shellRowsFromLumps = 0u;
    for (const DRW_AcisLump& lump : projection.wireframe.lumps) {
        if (lump.shellNodeIndices.size() > maxRows - shellRowsFromLumps) {
            hasKnownGaps = true;
            break;
        }
        shellRowsFromLumps += lump.shellNodeIndices.size();
    }
    if (shellRowsFromLumps != projection.wireframe.shells.size())
        hasKnownGaps = true;
    std::size_t faceRowsFromShells = 0u;
    for (const DRW_AcisShell& shell : projection.wireframe.shells) {
        if (shell.faceNodeIndices.size() > maxRows - faceRowsFromShells) {
            hasKnownGaps = true;
            break;
        }
        faceRowsFromShells += shell.faceNodeIndices.size();
    }
    if (faceRowsFromShells != projection.wireframe.faces.size())
        hasKnownGaps = true;
    const double maxFiniteValue = std::numeric_limits<double>::max();
    const auto finiteValue = [maxFiniteValue](double value) noexcept {
        return value >= -maxFiniteValue && value <= maxFiniteValue;
    };
    const auto finiteCoord = [&finiteValue](const DRW_Coord& value) noexcept {
        return finiteValue(value.x) && finiteValue(value.y)
            && finiteValue(value.z);
    };
    const auto finiteNonzeroCoord = [&finiteCoord](
        const DRW_Coord& value) noexcept {
        return finiteCoord(value)
            && (value.x != 0.0 || value.y != 0.0 || value.z != 0.0);
    };
    for (const DRW_AcisEdge& edge : projection.wireframe.edges) {
        if (edge.nodeIndex < 0 || edge.startVertexNodeIndex < 0
            || edge.endVertexNodeIndex < 0 || edge.curveNodeIndex < 0
            || !edge.hasStart || !edge.hasEnd || !edge.hasCurve)
            hasKnownGaps = true;
        if (!finiteCoord(edge.start) || !finiteCoord(edge.end))
            hasKnownGaps = true;
        if (edge.hasCurve && edge.curveType == DRW_AcisCurve::Straight
            && (!finiteCoord(edge.p0) || !finiteNonzeroCoord(edge.p1)))
            hasKnownGaps = true;
        if (edge.hasCurve && edge.curveType == DRW_AcisCurve::Ellipse) {
            if (!finiteCoord(edge.p0) || !finiteNonzeroCoord(edge.p1)
                || !finiteNonzeroCoord(edge.p2)
                || !finiteValue(edge.ratio) || edge.ratio <= 0.0)
                hasKnownGaps = true;
        }
        if (edge.hasCurve && edge.curveType == DRW_AcisCurve::Intcurve) {
            if (edge.controlPoints.empty())
                hasKnownGaps = true;
            for (const DRW_Coord& point : edge.controlPoints) {
                if (!finiteCoord(point)) {
                    hasKnownGaps = true;
                    break;
                }
            }
        }
        if (edge.hasCurve && edge.curveType == DRW_AcisCurve::Unknown)
            hasKnownGaps = true;
    }
    for (const DRW_AcisVertex& vertex : projection.wireframe.vertices) {
        if (vertex.nodeIndex < 0 || vertex.pointNodeIndex < 0
            || !vertex.valid || !finiteCoord(vertex.point)) {
            hasKnownGaps = true;
        }
    }
    if (!projection.hasLumpChainObservation
        || projection.lumpNodeIndices.empty()
        || !projection.lumpChainNullTerminated
        || projection.lumpChainCycleDetected
        || projection.lumpChainUnclassifiedLinkCount != 0u
        || projection.firstUnclassifiedLumpLinkSourceNodeIndex != -1
        || projection.firstRepeatedLumpChainNodeIndex != -1
        || projection.lumpNodeIndices.size()
            != projection.lumpNextNodeIndices.size()
        || !projection.hasLumpUseIndexObservation
        || !projection.lumpUseIndexCoversParsedLumps
        || projection.lumpUseIndexUnclassifiedLumpCount != 0u
        || projection.firstUnclassifiedLumpOwnerNodeIndex != -1
        || !projection.lumpChainMatchesUseIndex
        || projection.lumpNodeIndices.size()
            != projection.lumpUseNodeIndices.size()) {
        hasKnownGaps = true;
    }
    for (std::size_t index = 0u;
         index < projection.lumpNodeIndices.size(); ++index) {
        const int expectedNext = index + 1u < projection.lumpNodeIndices.size()
            ? projection.lumpNodeIndices[index + 1u] : -1;
        if (projection.lumpNodeIndices[index] < 0
            || projection.lumpNextNodeIndices.size()
                != projection.lumpNodeIndices.size()
            || projection.lumpNextNodeIndices[index] != expectedNext
            || (index > 0u
                && projection.lumpUseNodeIndices.size() > index
                && projection.lumpUseNodeIndices[index - 1u]
                    >= projection.lumpUseNodeIndices[index])) {
            hasKnownGaps = true;
            break;
        }
    }
    if (projection.lumpChainMatchesUseIndex) {
        if (!projection.lumpUseIndexCoversParsedLumps
            || projection.lumpUseIndexUnclassifiedLumpCount != 0u
            || projection.firstUnclassifiedLumpOwnerNodeIndex != -1
            || projection.lumpNodeIndices.size()
                != projection.lumpUseNodeIndices.size()) {
            hasKnownGaps = true;
        } else {
            for (int lumpIndex : projection.lumpNodeIndices) {
                std::size_t low = 0u;
                std::size_t high = projection.lumpUseNodeIndices.size();
                while (low < high) {
                    const std::size_t middle = low + (high - low) / 2u;
                    if (projection.lumpUseNodeIndices[middle] < lumpIndex)
                        low = middle + 1u;
                    else
                        high = middle;
                }
                if (low >= projection.lumpUseNodeIndices.size()
                    || projection.lumpUseNodeIndices[low] != lumpIndex) {
                    hasKnownGaps = true;
                    break;
                }
            }
        }
    }
    if (projection.wireframe.lumps.empty()
        || projection.wireframe.shells.empty())
        hasKnownGaps = true;
    for (const DRW_AcisEdge& edge : projection.wireframe.edges) {
        if (edge.coedgeNodeIndex < 0 || !edge.hasReciprocalCoedgeLink)
            hasKnownGaps = true;
        if (edge.hasCoedgePartnerRingObservation) {
            const std::size_t ringSize =
                edge.coedgePartnerRingNodeIndices.size();
            if (!edge.coedgePartnerRingClosed || ringSize == 0u
                || edge.coedgePartnerRingNextNodeIndices.size() != ringSize
                || edge.coedgePartnerRingNodeIndices.front()
                    != edge.coedgeNodeIndex) {
                hasKnownGaps = true;
            } else {
                for (std::size_t index = 0u; index < ringSize; ++index) {
                    const int expectedTarget = index + 1u < ringSize
                        ? edge.coedgePartnerRingNodeIndices[index + 1u]
                        : edge.coedgePartnerRingNodeIndices.front();
                    if (edge.coedgePartnerRingNextNodeIndices[index]
                        != expectedTarget) {
                        hasKnownGaps = true;
                        break;
                    }
                }
            }
        } else if (edge.coedgePartnerRingClosed
                   || !edge.coedgePartnerRingNodeIndices.empty()
                   || !edge.coedgePartnerRingNextNodeIndices.empty()) {
            hasKnownGaps = true;
        }
        if (edge.hasCoedgeEdgeUseIndexObservation) {
            if (!edge.coedgePartnerRingMatchesEdgeUseIndex
                || edge.coedgeEdgeUseNodeIndices.empty()
                || !edge.coedgeEdgeUseIndexCoversParsedCoedges
                || edge.coedgeEdgeUseIndexUnclassifiedCoedgeCount != 0u
                || edge.firstUnclassifiedCoedgeEdgeUseNodeIndex != -1) {
                hasKnownGaps = true;
            }
            for (std::size_t index = 0u;
                 index < edge.coedgeEdgeUseNodeIndices.size(); ++index) {
                if (edge.coedgeEdgeUseNodeIndices[index] < 0
                    || (index > 0u
                        && edge.coedgeEdgeUseNodeIndices[index - 1u]
                            >= edge.coedgeEdgeUseNodeIndices[index])) {
                    hasKnownGaps = true;
                    break;
                }
            }
            if (edge.coedgePartnerRingMatchesEdgeUseIndex) {
                if (!edge.coedgeEdgeUseIndexCoversParsedCoedges
                    || edge.coedgeEdgeUseIndexUnclassifiedCoedgeCount != 0u
                    || edge.firstUnclassifiedCoedgeEdgeUseNodeIndex != -1
                    || edge.coedgePartnerRingNodeIndices.size()
                    != edge.coedgeEdgeUseNodeIndices.size()) {
                    hasKnownGaps = true;
                } else {
                    for (int coedgeIndex :
                         edge.coedgePartnerRingNodeIndices) {
                        std::size_t low = 0u;
                        std::size_t high =
                            edge.coedgeEdgeUseNodeIndices.size();
                        while (low < high) {
                            const std::size_t middle =
                                low + (high - low) / 2u;
                            if (edge.coedgeEdgeUseNodeIndices[middle]
                                < coedgeIndex)
                                low = middle + 1u;
                            else
                                high = middle;
                        }
                        if (low >= edge.coedgeEdgeUseNodeIndices.size()
                            || edge.coedgeEdgeUseNodeIndices[low]
                                != coedgeIndex) {
                            hasKnownGaps = true;
                            break;
                        }
                    }
                }
            }
        } else if (edge.coedgePartnerRingMatchesEdgeUseIndex
                   || edge.coedgeEdgeUseIndexCoversParsedCoedges
                   || edge.coedgeEdgeUseIndexUnclassifiedCoedgeCount != 0u
                   || edge.firstUnclassifiedCoedgeEdgeUseNodeIndex != -1
                   || !edge.coedgeEdgeUseNodeIndices.empty()) {
            hasKnownGaps = true;
        }
    }
    for (const DRW_AcisFace& face : projection.wireframe.faces) {
        if (face.shellNodeIndex < 0 || face.surfaceNodeIndex < 0
            || !face.hasSurface || face.loops.empty()
            || !face.hasSense || !face.hasDoubleSided
            || (face.doubleSided && !face.hasContainment)) {
            hasKnownGaps = true;
        }
        if (face.hasSurface && face.surfaceType == DRW_AcisSurface::Plane) {
            if (!finiteCoord(face.p0) || !finiteNonzeroCoord(face.p1))
                hasKnownGaps = true;
        }
        if (face.hasSurface && face.surfaceType == DRW_AcisSurface::Sphere) {
            if (!finiteCoord(face.p0) || !finiteValue(face.majorRadius)
                || face.majorRadius == 0.0)
                hasKnownGaps = true;
        }
        if (face.hasSurface && face.surfaceType == DRW_AcisSurface::Cone) {
            const bool finiteAngle = finiteValue(face.sineAngle)
                && finiteValue(face.cosineAngle);
            if (!finiteCoord(face.p0) || !finiteNonzeroCoord(face.p1)
                || !finiteNonzeroCoord(face.p2)
                || !finiteValue(face.ratio) || face.ratio <= 0.0
                || !finiteAngle
                || (face.sineAngle == 0.0 && face.cosineAngle == 0.0))
                hasKnownGaps = true;
        }
        if (face.hasSurface && face.surfaceType == DRW_AcisSurface::Torus) {
            if (!finiteCoord(face.p0) || !finiteNonzeroCoord(face.p1)
                || !finiteValue(face.majorRadius)
                || !finiteValue(face.minorRadius)
                || face.majorRadius <= 0.0 || face.minorRadius <= 0.0)
                hasKnownGaps = true;
        }
        if (face.hasLoopUseIndexObservation) {
            if (!face.faceLoopChainMatchesUseIndex
                || !face.loopUseIndexCoversParsedLoops
                || face.loopUseIndexUnclassifiedLoopCount != 0u
                || face.firstUnclassifiedLoopOwnerNodeIndex != -1
                || face.loopUseNodeIndices.empty()) {
                hasKnownGaps = true;
            }
            for (std::size_t index = 0u;
                 index < face.loopUseNodeIndices.size(); ++index) {
                if (face.loopUseNodeIndices[index] < 0
                    || (index > 0u
                        && face.loopUseNodeIndices[index - 1u]
                            >= face.loopUseNodeIndices[index])) {
                    hasKnownGaps = true;
                    break;
                }
            }
            if (face.faceLoopChainMatchesUseIndex) {
                if (!face.loopUseIndexCoversParsedLoops
                    || face.loopUseIndexUnclassifiedLoopCount != 0u
                    || face.firstUnclassifiedLoopOwnerNodeIndex != -1
                    || face.loops.size() != face.loopUseNodeIndices.size()) {
                    hasKnownGaps = true;
                } else {
                    for (const DRW_AcisFaceLoop& faceLoop : face.loops) {
                        std::size_t low = 0u;
                        std::size_t high = face.loopUseNodeIndices.size();
                        while (low < high) {
                            const std::size_t middle =
                                low + (high - low) / 2u;
                            if (face.loopUseNodeIndices[middle]
                                < faceLoop.nodeIndex)
                                low = middle + 1u;
                            else
                                high = middle;
                        }
                        if (low >= face.loopUseNodeIndices.size()
                            || face.loopUseNodeIndices[low]
                                != faceLoop.nodeIndex) {
                            hasKnownGaps = true;
                            break;
                        }
                    }
                }
            }
        } else if (face.loopUseIndexCoversParsedLoops
                   || face.loopUseIndexUnclassifiedLoopCount != 0u
                   || face.firstUnclassifiedLoopOwnerNodeIndex != -1
                   || face.faceLoopChainMatchesUseIndex
                   || !face.loopUseNodeIndices.empty()) {
            hasKnownGaps = true;
        }
        if (face.hasLoopChainObservation) {
            if (face.loops.empty() || !face.loopChainNullTerminated
                || face.loopChainCycleDetected
                || face.loopChainUnclassifiedNextLinkCount != 0u
                || face.firstUnclassifiedLoopNextNodeIndex != -1
                || face.firstRepeatedLoopChainNodeIndex != -1
                || face.loopNextNodeIndices.size() != face.loops.size()) {
                hasKnownGaps = true;
            }
            if (face.loopChainCycleDetected
                != (face.firstRepeatedLoopChainNodeIndex >= 0)) {
                hasKnownGaps = true;
            }
            for (std::size_t index = 0u;
                 index < face.loopNextNodeIndices.size(); ++index) {
                const int expectedTarget = index + 1u < face.loops.size()
                    ? face.loops[index + 1u].nodeIndex : -1;
                if (index >= face.loops.size()
                    || face.loopNextNodeIndices[index] != expectedTarget) {
                    hasKnownGaps = true;
                    break;
                }
            }
            if (face.faceLoopChainMatchesUseIndex
                && (!face.loopChainNullTerminated
                    || face.loopChainCycleDetected
                    || face.loopChainUnclassifiedNextLinkCount != 0u
                    || face.firstUnclassifiedLoopNextNodeIndex != -1
                    || face.firstRepeatedLoopChainNodeIndex != -1
                    || face.loopNextNodeIndices.size()
                        != face.loops.size())) {
                hasKnownGaps = true;
            }
        } else if (face.loopChainNullTerminated
                   || face.loopChainCycleDetected
                   || face.firstRepeatedLoopChainNodeIndex != -1
                   || face.loopChainUnclassifiedNextLinkCount != 0u
                   || face.firstUnclassifiedLoopNextNodeIndex != -1
                   || !face.loopNextNodeIndices.empty()) {
            hasKnownGaps = true;
        }
        for (const DRW_AcisFaceLoop& loop : face.loops) {
            if (loop.hasCoedgeRingClosure && !loop.coedgeRingClosed
                && !loop.coedgeRingOpen)
                hasKnownGaps = true;
            if (loop.coedgeRingOpen
                && (!loop.hasCoedgeRingClosure || loop.coedgeRingClosed))
                hasKnownGaps = true;
            if (loop.hasCoedgeRingClosure
                && !loop.coedgeLinksReciprocal)
                hasKnownGaps = true;
            if (loop.coedgeLinksReciprocal
                && (!loop.hasCoedgeRingClosure
                    || loop.coedgeRingClosed == loop.coedgeRingOpen))
                hasKnownGaps = true;
            if (loop.hasCoedgeUseIndexObservation) {
                if (!loop.coedgeUseIndexCoversParsedCoedges
                    || loop.coedgeUseIndexUnclassifiedCoedgeCount != 0u
                    || loop.firstUnclassifiedCoedgeOwnerNodeIndex != -1
                    || !loop.coedgeChainMatchesUseIndex
                    || loop.coedgeUseNodeIndices.empty()) {
                    hasKnownGaps = true;
                }
                for (std::size_t index = 0u;
                     index < loop.coedgeUseNodeIndices.size(); ++index) {
                    if (loop.coedgeUseNodeIndices[index] < 0
                        || (index > 0u
                            && loop.coedgeUseNodeIndices[index - 1u]
                                >= loop.coedgeUseNodeIndices[index])) {
                        hasKnownGaps = true;
                        break;
                    }
                }
                if (loop.coedgeChainMatchesUseIndex) {
                    const std::size_t coedgeCount =
                        loop.coedgeNodeIndices.size();
                    if (!loop.coedgeUseIndexCoversParsedCoedges
                        || loop.coedgeUseIndexUnclassifiedCoedgeCount != 0u
                        || loop.firstUnclassifiedCoedgeOwnerNodeIndex != -1
                        || coedgeCount == 0u
                        || coedgeCount != loop.coedgeUseNodeIndices.size()
                        || coedgeCount != loop.coedgeNextNodeIndices.size()
                        || coedgeCount != loop.coedgeLoopNodeIndices.size()
                        || !loop.hasCoedgeRingClosure
                        || loop.coedgeRingClosed == loop.coedgeRingOpen
                        || !loop.coedgeLinksReciprocal) {
                        hasKnownGaps = true;
                    } else {
                        for (std::size_t index = 0u;
                             index < coedgeCount; ++index) {
                            const int expectedNext = index + 1u < coedgeCount
                                ? loop.coedgeNodeIndices[index + 1u]
                                : (loop.coedgeRingClosed
                                       ? loop.coedgeNodeIndices.front() : -1);
                            if (loop.coedgeNextNodeIndices[index]
                                    != expectedNext
                                || loop.coedgeLoopNodeIndices[index]
                                    != loop.nodeIndex) {
                                hasKnownGaps = true;
                                break;
                            }
                            std::size_t low = 0u;
                            std::size_t high =
                                loop.coedgeUseNodeIndices.size();
                            while (low < high) {
                                const std::size_t middle =
                                    low + (high - low) / 2u;
                                if (loop.coedgeUseNodeIndices[middle]
                                    < loop.coedgeNodeIndices[index])
                                    low = middle + 1u;
                                else
                                    high = middle;
                            }
                            if (low >= loop.coedgeUseNodeIndices.size()
                                || loop.coedgeUseNodeIndices[low]
                                    != loop.coedgeNodeIndices[index]) {
                                hasKnownGaps = true;
                                break;
                            }
                        }
                    }
                }
            } else if (loop.coedgeUseIndexCoversParsedCoedges
                       || loop.coedgeUseIndexUnclassifiedCoedgeCount != 0u
                       || loop.firstUnclassifiedCoedgeOwnerNodeIndex != -1
                       || loop.coedgeChainMatchesUseIndex
                       || !loop.coedgeUseNodeIndices.empty()) {
                hasKnownGaps = true;
            }
            if (loop.coedgeSenseFlags.size()
                    != loop.coedgeNodeIndices.size()
                || loop.coedgeNextNodeIndices.size()
                    != loop.coedgeNodeIndices.size()
                || loop.coedgePreviousNodeIndices.size()
                    != loop.coedgeNodeIndices.size()
                || loop.coedgePartnerNodeIndices.size()
                    != loop.coedgeNodeIndices.size()
                || loop.coedgeEdgeNodeIndices.size()
                    != loop.coedgeNodeIndices.size()
                || loop.coedgeEdgeStartVertexNodeIndices.size()
                    != loop.coedgeNodeIndices.size()
                || loop.coedgeEdgeEndVertexNodeIndices.size()
                    != loop.coedgeNodeIndices.size()
                || loop.coedgeOrientedStartVertexNodeIndices.size()
                    != loop.coedgeNodeIndices.size()
                || loop.coedgeOrientedEndVertexNodeIndices.size()
                    != loop.coedgeNodeIndices.size()
                || loop.coedgeLoopNodeIndices.size()
                    != loop.coedgeNodeIndices.size()
                || loop.coedgePCurveNodeIndices.size()
                    != loop.coedgeNodeIndices.size()) {
                hasKnownGaps = true;
                continue;
            }
            for (std::size_t index = 0u;
                 index < loop.coedgeNodeIndices.size(); ++index) {
                const bool openStart = loop.coedgeRingOpen && index == 0u;
                const bool openEnd = loop.coedgeRingOpen
                    && index + 1u == loop.coedgeNodeIndices.size();
                if (loop.coedgeSenseFlags[index] < 0
                    || loop.coedgeSenseFlags[index] > 1
                    || (openEnd
                        && loop.coedgeNextNodeIndices[index] >= 0)
                    || (openStart
                        && loop.coedgePreviousNodeIndices[index] >= 0)
                    || (loop.coedgeNextNodeIndices[index] < 0 && !openEnd)
                    || (loop.coedgePreviousNodeIndices[index] < 0
                        && !openStart)
                    || loop.coedgePartnerNodeIndices[index] < 0
                    || loop.coedgeEdgeNodeIndices[index] < 0
                    || loop.coedgeEdgeStartVertexNodeIndices[index] < 0
                    || loop.coedgeEdgeEndVertexNodeIndices[index] < 0
                    || loop.coedgeLoopNodeIndices[index] != loop.nodeIndex) {
                    hasKnownGaps = true;
                    break;
                }
                const int expectedOrientedStart =
                    loop.coedgeSenseFlags[index] == 0
                    ? loop.coedgeEdgeStartVertexNodeIndices[index]
                    : loop.coedgeSenseFlags[index] == 1
                        ? loop.coedgeEdgeEndVertexNodeIndices[index] : -1;
                const int expectedOrientedEnd =
                    loop.coedgeSenseFlags[index] == 0
                    ? loop.coedgeEdgeEndVertexNodeIndices[index]
                    : loop.coedgeSenseFlags[index] == 1
                        ? loop.coedgeEdgeStartVertexNodeIndices[index] : -1;
                if (loop.coedgeOrientedStartVertexNodeIndices[index]
                        != expectedOrientedStart
                    || loop.coedgeOrientedEndVertexNodeIndices[index]
                        != expectedOrientedEnd) {
                    hasKnownGaps = true;
                    break;
                }
            }
            bool canObserveEndpointChain =
                !loop.coedgeNodeIndices.empty()
                && (loop.coedgeRingClosed || loop.coedgeRingOpen)
                && loop.coedgeLinksReciprocal;
            for (std::size_t index = 0u;
                 canObserveEndpointChain
                     && index < loop.coedgeNodeIndices.size(); ++index) {
                canObserveEndpointChain =
                    loop.coedgeSenseFlags[index] >= 0
                    && loop.coedgeSenseFlags[index] <= 1
                    && loop.coedgeOrientedStartVertexNodeIndices[index] >= 0
                    && loop.coedgeOrientedEndVertexNodeIndices[index] >= 0;
            }
            if (loop.hasCoedgeEndpointChainObservation
                != canObserveEndpointChain) {
                hasKnownGaps = true;
            } else if (canObserveEndpointChain) {
                const std::size_t joinCount = loop.coedgeRingClosed
                    ? loop.coedgeNodeIndices.size()
                    : loop.coedgeNodeIndices.size() - 1u;
                std::size_t expectedDiscontinuityCount = 0u;
                int expectedFirstDiscontinuityNodeIndex = -1;
                for (std::size_t index = 0u; index < joinCount; ++index) {
                    const std::size_t nextIndex = (index + 1u)
                        % loop.coedgeNodeIndices.size();
                    if (loop.coedgeOrientedEndVertexNodeIndices[index]
                        != loop.coedgeOrientedStartVertexNodeIndices[nextIndex]) {
                        ++expectedDiscontinuityCount;
                        if (expectedFirstDiscontinuityNodeIndex < 0) {
                            expectedFirstDiscontinuityNodeIndex =
                                loop.coedgeNodeIndices[index];
                        }
                    }
                }
                if (expectedDiscontinuityCount != 0u)
                    hasKnownGaps = true;
                if (loop.coedgeEndpointDiscontinuityCount
                        != expectedDiscontinuityCount
                    || loop.firstCoedgeEndpointDiscontinuityNodeIndex
                        != expectedFirstDiscontinuityNodeIndex
                    || loop.coedgeEndpointChainContinuous
                        != (expectedDiscontinuityCount == 0u)) {
                    hasKnownGaps = true;
                }
            } else if (loop.coedgeEndpointChainContinuous
                       || loop.coedgeEndpointDiscontinuityCount != 0u
                       || loop.firstCoedgeEndpointDiscontinuityNodeIndex != -1) {
                hasKnownGaps = true;
            }
        }
    }
    for (const DRW_AcisShell& shell : projection.wireframe.shells) {
        if (shell.nodeIndex < 0 || shell.lumpNodeIndex < 0
            || !shell.hasFaceChainObservation
            || shell.faceNodeIndices.empty()
            || !shell.faceChainNullTerminated
            || shell.faceChainCycleDetected
            || shell.faceChainUnclassifiedLinkCount != 0u
            || shell.firstUnclassifiedFaceLinkSourceNodeIndex != -1
            || shell.firstRepeatedFaceChainNodeIndex != -1
            || shell.faceNodeIndices.size()
                != shell.faceNextNodeIndices.size()
            || !shell.hasFaceUseIndexObservation
            || !shell.faceUseIndexCoversParsedFaces
            || shell.faceUseIndexUnclassifiedFaceCount != 0u
            || shell.firstUnclassifiedFaceOwnerNodeIndex != -1
            || !shell.faceChainMatchesUseIndex
            || shell.faceNodeIndices.size() != shell.faceUseNodeIndices.size()) {
            hasKnownGaps = true;
        }
        for (std::size_t index = 0u;
             index < shell.faceNodeIndices.size(); ++index) {
            const int expectedNext = index + 1u < shell.faceNodeIndices.size()
                ? shell.faceNodeIndices[index + 1u] : -1;
            if (shell.faceNodeIndices[index] < 0
                || shell.faceNextNodeIndices.size()
                    != shell.faceNodeIndices.size()
                || shell.faceNextNodeIndices[index] != expectedNext
                || (index > 0u
                    && shell.faceUseNodeIndices.size() > index
                    && shell.faceUseNodeIndices[index - 1u]
                        >= shell.faceUseNodeIndices[index])) {
                hasKnownGaps = true;
                break;
            }
        }
        if (shell.faceChainMatchesUseIndex) {
            if (!shell.faceUseIndexCoversParsedFaces
                || shell.faceUseIndexUnclassifiedFaceCount != 0u
                || shell.firstUnclassifiedFaceOwnerNodeIndex != -1
                || shell.faceNodeIndices.size()
                    != shell.faceUseNodeIndices.size()) {
                hasKnownGaps = true;
            } else {
                for (int faceIndex : shell.faceNodeIndices) {
                    std::size_t low = 0u;
                    std::size_t high = shell.faceUseNodeIndices.size();
                    while (low < high) {
                        const std::size_t middle =
                            low + (high - low) / 2u;
                        if (shell.faceUseNodeIndices[middle] < faceIndex)
                            low = middle + 1u;
                        else
                            high = middle;
                    }
                    if (low >= shell.faceUseNodeIndices.size()
                        || shell.faceUseNodeIndices[low] != faceIndex) {
                        hasKnownGaps = true;
                        break;
                    }
                }
            }
        }
    }
    for (const DRW_AcisLump& lump : projection.wireframe.lumps) {
        if (lump.nodeIndex < 0 || lump.bodyNodeIndex < 0
            || !lump.hasShellChainObservation
            || lump.shellNodeIndices.empty()
            || !lump.shellChainNullTerminated
            || lump.shellChainCycleDetected
            || lump.shellChainUnclassifiedLinkCount != 0u
            || lump.firstUnclassifiedShellLinkSourceNodeIndex != -1
            || lump.firstRepeatedShellChainNodeIndex != -1
            || lump.shellNodeIndices.size()
                != lump.shellNextNodeIndices.size()
            || !lump.hasShellUseIndexObservation
            || !lump.shellUseIndexCoversParsedShells
            || lump.shellUseIndexUnclassifiedShellCount != 0u
            || lump.firstUnclassifiedShellOwnerNodeIndex != -1
            || !lump.shellChainMatchesUseIndex
            || lump.shellNodeIndices.size() != lump.shellUseNodeIndices.size()) {
            hasKnownGaps = true;
        }
        for (std::size_t index = 0u;
             index < lump.shellNodeIndices.size(); ++index) {
            const int expectedNext = index + 1u < lump.shellNodeIndices.size()
                ? lump.shellNodeIndices[index + 1u] : -1;
            if (lump.shellNodeIndices[index] < 0
                || lump.shellNextNodeIndices.size()
                    != lump.shellNodeIndices.size()
                || lump.shellNextNodeIndices[index] != expectedNext
                || (index > 0u
                    && lump.shellUseNodeIndices.size() > index
                    && lump.shellUseNodeIndices[index - 1u]
                        >= lump.shellUseNodeIndices[index])) {
                hasKnownGaps = true;
                break;
            }
        }
        if (lump.shellChainMatchesUseIndex) {
            if (!lump.shellUseIndexCoversParsedShells
                || lump.shellUseIndexUnclassifiedShellCount != 0u
                || lump.firstUnclassifiedShellOwnerNodeIndex != -1
                || lump.shellNodeIndices.size()
                    != lump.shellUseNodeIndices.size()) {
                hasKnownGaps = true;
            } else {
                for (int shellIndex : lump.shellNodeIndices) {
                    std::size_t low = 0u;
                    std::size_t high = lump.shellUseNodeIndices.size();
                    while (low < high) {
                        const std::size_t middle =
                            low + (high - low) / 2u;
                        if (lump.shellUseNodeIndices[middle] < shellIndex)
                            low = middle + 1u;
                        else
                            high = middle;
                    }
                    if (low >= lump.shellUseNodeIndices.size()
                        || lump.shellUseNodeIndices[low] != shellIndex) {
                        hasKnownGaps = true;
                        break;
                    }
                }
            }
        }
    }
    // Reconcile the identities on the retained child rows with the exact
    // owner rows that were independently indexed from the parsed model. The
    // chain/use-index checks above prove each parent's row, but without these
    // joins a copied or stale child record could still name a different
    // parent and leave the aggregate counts unchanged.
    bool lumpRowsOrdered = true;
    for (std::size_t index = 0u;
         index < projection.wireframe.lumps.size(); ++index) {
        const DRW_AcisLump& lump = projection.wireframe.lumps[index];
        if (lump.nodeIndex < 0
            || (index > 0u
                && projection.wireframe.lumps[index - 1u].nodeIndex
                    >= lump.nodeIndex)) {
            lumpRowsOrdered = false;
            hasKnownGaps = true;
        }
        if (lump.bodyNodeIndex != projection.bodyNodeIndex)
            hasKnownGaps = true;
        if (index >= projection.lumpUseNodeIndices.size()
            || lump.nodeIndex != projection.lumpUseNodeIndices[index]) {
            hasKnownGaps = true;
        }
    }
    bool shellRowsOrdered = true;
    for (std::size_t index = 0u;
         index < projection.wireframe.shells.size(); ++index) {
        const DRW_AcisShell& shell = projection.wireframe.shells[index];
        if (shell.nodeIndex < 0
            || (index > 0u
                && projection.wireframe.shells[index - 1u].nodeIndex
                    >= shell.nodeIndex)) {
            shellRowsOrdered = false;
            hasKnownGaps = true;
        }
    }
    const auto findLumpRow = [&projection, lumpRowsOrdered](int nodeIndex)
        -> const DRW_AcisLump* {
        if (!lumpRowsOrdered || nodeIndex < 0)
            return nullptr;
        std::size_t low = 0u;
        std::size_t high = projection.wireframe.lumps.size();
        while (low < high) {
            const std::size_t middle = low + (high - low) / 2u;
            if (projection.wireframe.lumps[middle].nodeIndex < nodeIndex)
                low = middle + 1u;
            else
                high = middle;
        }
        return low < projection.wireframe.lumps.size()
                && projection.wireframe.lumps[low].nodeIndex == nodeIndex
            ? &projection.wireframe.lumps[low] : nullptr;
    };
    const auto findShellRow = [&projection, shellRowsOrdered](int nodeIndex)
        -> const DRW_AcisShell* {
        if (!shellRowsOrdered || nodeIndex < 0)
            return nullptr;
        std::size_t low = 0u;
        std::size_t high = projection.wireframe.shells.size();
        while (low < high) {
            const std::size_t middle = low + (high - low) / 2u;
            if (projection.wireframe.shells[middle].nodeIndex < nodeIndex)
                low = middle + 1u;
            else
                high = middle;
        }
        return low < projection.wireframe.shells.size()
                && projection.wireframe.shells[low].nodeIndex == nodeIndex
            ? &projection.wireframe.shells[low] : nullptr;
    };
    const auto containsSortedNode = [](const std::vector<int>& indices,
                                       int nodeIndex) noexcept {
        if (nodeIndex < 0)
            return false;
        std::size_t low = 0u;
        std::size_t high = indices.size();
        while (low < high) {
            const std::size_t middle = low + (high - low) / 2u;
            if (indices[middle] < nodeIndex)
                low = middle + 1u;
            else
                high = middle;
        }
        return low < indices.size() && indices[low] == nodeIndex;
    };
    if (lumpRowsOrdered && shellRowsOrdered) {
        for (const DRW_AcisShell& shell : projection.wireframe.shells) {
            const DRW_AcisLump* parent = findLumpRow(shell.lumpNodeIndex);
            if (!parent || !parent->shellChainMatchesUseIndex
                || !parent->shellUseIndexCoversParsedShells
                || parent->shellNodeIndices.size()
                    != parent->shellUseNodeIndices.size()
                || !containsSortedNode(parent->shellUseNodeIndices,
                                       shell.nodeIndex)) {
                hasKnownGaps = true;
            }
        }
        for (const DRW_AcisFace& face : projection.wireframe.faces) {
            const DRW_AcisShell* parent = findShellRow(face.shellNodeIndex);
            if (!parent || !parent->faceChainMatchesUseIndex
                || !parent->faceUseIndexCoversParsedFaces
                || parent->faceNodeIndices.size()
                    != parent->faceUseNodeIndices.size()
                || !containsSortedNode(parent->faceUseNodeIndices,
                                       face.nodeIndex)) {
                hasKnownGaps = true;
            }
        }
    }
    // graphLinks is stored as contiguous source groups with zero-based pointer
    // ordinals. The graph builder admits traversal sources once and appends
    // the non-traversed source rows from a set; confirm the retained census
    // still obeys that framing and has no repeated source group. Build compact
    // sorted identity indexes and fail closed if their scratch cannot be built.
    struct GraphLinkGroupIndex {
        int sourceNodeIndex = -1;
        std::size_t firstLink = 0u;
    };
    std::vector<int> reachableNodeIndexLookup;
    std::vector<GraphLinkGroupIndex> graphLinkGroupIndex;
    bool reachableNodeIndexLookupReady = false;
    bool graphLinkGroupIndexReady = false;
    try {
        reachableNodeIndexLookup = projection.reachableNodeIndices;
        std::sort(reachableNodeIndexLookup.begin(),
                  reachableNodeIndexLookup.end());
        for (std::size_t index = 0u;
             index < reachableNodeIndexLookup.size(); ++index) {
            if (reachableNodeIndexLookup[index] < 0
                || (index > 0u
                    && reachableNodeIndexLookup[index - 1u]
                        == reachableNodeIndexLookup[index])) {
                hasKnownGaps = true;
            }
        }
        reachableNodeIndexLookupReady = true;

        std::size_t graphLinkGroupCount = 0u;
        for (std::size_t index = 0u;
             index < projection.graphLinks.size();) {
            ++graphLinkGroupCount;
            const int sourceNodeIndex =
                projection.graphLinks[index].sourceNodeIndex;
            while (index < projection.graphLinks.size()
                   && projection.graphLinks[index].sourceNodeIndex
                       == sourceNodeIndex) {
                ++index;
            }
        }
        graphLinkGroupIndex.reserve(graphLinkGroupCount);
        for (std::size_t index = 0u;
             index < projection.graphLinks.size();) {
            const std::size_t firstLink = index;
            const int sourceNodeIndex =
                projection.graphLinks[index].sourceNodeIndex;
            if (sourceNodeIndex < 0)
                hasKnownGaps = true;
            if (!std::binary_search(reachableNodeIndexLookup.begin(),
                                    reachableNodeIndexLookup.end(),
                                    sourceNodeIndex)) {
                hasKnownGaps = true;
            }
            graphLinkGroupIndex.push_back({sourceNodeIndex, firstLink});
            std::size_t expectedOrdinal = 0u;
            while (index < projection.graphLinks.size()
                   && projection.graphLinks[index].sourceNodeIndex
                       == sourceNodeIndex) {
                if (projection.graphLinks[index].targetNodeIndex < -1)
                    hasKnownGaps = true;
                if (projection.graphLinks[index].pointerOrdinal
                    != expectedOrdinal) {
                    hasKnownGaps = true;
                }
                ++expectedOrdinal;
                ++index;
            }
        }
        std::sort(graphLinkGroupIndex.begin(), graphLinkGroupIndex.end(),
                  [](const GraphLinkGroupIndex& lhs,
                     const GraphLinkGroupIndex& rhs) {
                      return lhs.sourceNodeIndex < rhs.sourceNodeIndex;
                  });
        graphLinkGroupIndexReady = true;
        for (std::size_t index = 1u;
             index < graphLinkGroupIndex.size(); ++index) {
            if (graphLinkGroupIndex[index - 1u].sourceNodeIndex
                == graphLinkGroupIndex[index].sourceNodeIndex) {
                hasKnownGaps = true;
                break;
            }
        }
    } catch (...) {
        // The classifier is noexcept. An inability to validate this census
        // must not escape or be mistaken for gap-free projection evidence.
        hasKnownGaps = true;
    }
    if (reachableNodeIndexLookupReady) {
        const auto isReachableSource = [&reachableNodeIndexLookup](
                                           int sourceNodeIndex) noexcept {
            return sourceNodeIndex >= 0
                && std::binary_search(reachableNodeIndexLookup.begin(),
                                      reachableNodeIndexLookup.end(),
                                      sourceNodeIndex);
        };
        // Parent/child geometry rows and their ordered incidence rows must
        // only name identities admitted by this body's rooted traversal.
        // This is a projection identity invariant, not proof that traversal
        // discovered every source-file record.
        for (int nodeIndex : projection.lumpNodeIndices) {
            if (!isReachableSource(nodeIndex))
                hasKnownGaps = true;
        }
        for (const DRW_AcisLump& lump : projection.wireframe.lumps) {
            if (!isReachableSource(lump.nodeIndex)
                || !isReachableSource(lump.bodyNodeIndex)) {
                hasKnownGaps = true;
            }
            for (int nodeIndex : lump.shellNodeIndices) {
                if (!isReachableSource(nodeIndex))
                    hasKnownGaps = true;
            }
        }
        for (const DRW_AcisShell& shell : projection.wireframe.shells) {
            if (!isReachableSource(shell.nodeIndex)
                || !isReachableSource(shell.lumpNodeIndex)) {
                hasKnownGaps = true;
            }
            for (int nodeIndex : shell.faceNodeIndices) {
                if (!isReachableSource(nodeIndex))
                    hasKnownGaps = true;
            }
        }
        for (const DRW_AcisFace& face : projection.wireframe.faces) {
            if (!isReachableSource(face.nodeIndex)
                || !isReachableSource(face.shellNodeIndex)
                || !isReachableSource(face.surfaceNodeIndex)) {
                hasKnownGaps = true;
            }
            for (const DRW_AcisFaceLoop& loop : face.loops) {
                if (!isReachableSource(loop.nodeIndex))
                    hasKnownGaps = true;
                for (int nodeIndex : loop.coedgeNodeIndices) {
                    if (!isReachableSource(nodeIndex))
                        hasKnownGaps = true;
                }
            }
        }
    }
    // Reconcile typed rows against the pointer-row census by source identity,
    // even when their traversal order differs from graphLinks' retained order.
    // The compact group index avoids repeatedly scanning all links and avoids
    // copying/sorting the potentially large link vector.
    const auto findGraphLinkGroup = [&projection, &graphLinkGroupIndex,
                                     graphLinkGroupIndexReady](
                                        int sourceNodeIndex,
                                        std::size_t pointerCount) {
        const std::size_t missing = std::numeric_limits<std::size_t>::max();
        if (!graphLinkGroupIndexReady)
            return missing;
        const auto found = std::lower_bound(
            graphLinkGroupIndex.begin(), graphLinkGroupIndex.end(),
            sourceNodeIndex,
            [](const GraphLinkGroupIndex& group, int source) {
                return group.sourceNodeIndex < source;
            });
        if (found == graphLinkGroupIndex.end()
            || found->sourceNodeIndex != sourceNodeIndex)
            return missing;
        const std::size_t first = found->firstLink;
        std::size_t end = first;
        while (end < projection.graphLinks.size()
               && projection.graphLinks[end].sourceNodeIndex
                   == sourceNodeIndex) {
            ++end;
        }
        if (end - first != pointerCount)
            return missing;
        for (std::size_t ordinal = 0u; ordinal < pointerCount; ++ordinal) {
            if (projection.graphLinks[first + ordinal].pointerOrdinal
                != ordinal)
                return missing;
        }
        return first;
    };
    const std::size_t missingGraphLinkGroup =
        std::numeric_limits<std::size_t>::max();
    const std::size_t firstBodyLink = findGraphLinkGroup(
        projection.bodyNodeIndex, 4u);
    const int firstLump = projection.lumpNodeIndices.empty()
        ? -1 : projection.lumpNodeIndices.front();
    if (firstBodyLink == missingGraphLinkGroup
        || projection.graphLinks[firstBodyLink + 1u].targetNodeIndex
            != firstLump
        || projection.graphLinks[firstBodyLink + 3u].targetNodeIndex
            != projection.bodyTransformNodeIndex) {
        hasKnownGaps = true;
    }
    for (std::size_t lumpIndex = 0u;
         lumpIndex < projection.wireframe.lumps.size(); ++lumpIndex) {
        const DRW_AcisLump& lump = projection.wireframe.lumps[lumpIndex];
        const std::size_t lumpFirst = findGraphLinkGroup(
            lump.nodeIndex, 4u);
        const int firstShell = lump.shellNodeIndices.empty()
            ? -1 : lump.shellNodeIndices.front();
        if (lumpFirst == missingGraphLinkGroup
            || lumpIndex >= projection.lumpNextNodeIndices.size()
            || projection.graphLinks[lumpFirst + 1u].targetNodeIndex
                != projection.lumpNextNodeIndices[lumpIndex]
            || projection.graphLinks[lumpFirst + 2u].targetNodeIndex
                != firstShell
            || projection.graphLinks[lumpFirst + 3u].targetNodeIndex
                != lump.bodyNodeIndex) {
            hasKnownGaps = true;
        }
        for (std::size_t shellIndex = 0u;
             shellIndex < lump.shellNodeIndices.size(); ++shellIndex) {
            const int shellNodeIndex = lump.shellNodeIndices[shellIndex];
            const DRW_AcisShell* shell = findShellRow(shellNodeIndex);
            const std::size_t shellFirst = findGraphLinkGroup(
                shellNodeIndex, 6u);
            const int expectedNextShell =
                shellIndex + 1u < lump.shellNodeIndices.size()
                    ? lump.shellNodeIndices[shellIndex + 1u] : -1;
            const int firstFace = !shell || shell->faceNodeIndices.empty()
                ? -1 : shell->faceNodeIndices.front();
            if (!shell || shellFirst == missingGraphLinkGroup
                || shellIndex >= lump.shellNextNodeIndices.size()
                || lump.shellNextNodeIndices[shellIndex] != expectedNextShell
                || projection.graphLinks[shellFirst + 1u].targetNodeIndex
                    != expectedNextShell
                || projection.graphLinks[shellFirst + 3u].targetNodeIndex
                    != firstFace
                || projection.graphLinks[shellFirst + 5u].targetNodeIndex
                    != lump.nodeIndex
                || shell->lumpNodeIndex != lump.nodeIndex) {
                hasKnownGaps = true;
            }
            if (!shell)
                continue;
            for (std::size_t faceIndex = 0u;
                 faceIndex < shell->faceNodeIndices.size(); ++faceIndex) {
                const std::size_t faceFirst = findGraphLinkGroup(
                    shell->faceNodeIndices[faceIndex], 6u);
                if (faceFirst == missingGraphLinkGroup
                    || faceIndex >= shell->faceNextNodeIndices.size()) {
                    hasKnownGaps = true;
                    continue;
                }
                if (projection.graphLinks[faceFirst + 1u].targetNodeIndex
                        != shell->faceNextNodeIndices[faceIndex]
                    || projection.graphLinks[faceFirst + 3u].targetNodeIndex
                        != shell->nodeIndex) {
                    hasKnownGaps = true;
                }
            }
        }
    }
    for (const DRW_AcisVertex& vertex : projection.wireframe.vertices) {
        const std::size_t first = findGraphLinkGroup(
            vertex.nodeIndex, 3u);
        if (first == missingGraphLinkGroup) {
            hasKnownGaps = true;
            continue;
        }
        if (projection.graphLinks[first + 2u].targetNodeIndex
            != vertex.pointNodeIndex) {
            hasKnownGaps = true;
        }
        // A resolved vertex ordinal-1 target is an edge. Reconcile it back
        // against that edge's retained endpoint identities; a null adjacency
        // remains observable without assuming it is semantically invalid.
        const int edgeNodeIndex =
            projection.graphLinks[first + 1u].targetNodeIndex;
        if (edgeNodeIndex >= 0) {
            const std::size_t edgeFirst = findGraphLinkGroup(
                edgeNodeIndex, 5u);
            if (edgeFirst == missingGraphLinkGroup
                || (projection.graphLinks[edgeFirst + 1u].targetNodeIndex
                        != vertex.nodeIndex
                    && projection.graphLinks[edgeFirst + 2u].targetNodeIndex
                        != vertex.nodeIndex)) {
                hasKnownGaps = true;
            }
        }
    }
    for (const DRW_AcisEdge& edge : projection.wireframe.edges) {
        const std::size_t first = findGraphLinkGroup(
            edge.nodeIndex, 5u);
        if (first == missingGraphLinkGroup
            || projection.graphLinks[first + 1u].targetNodeIndex
                != edge.startVertexNodeIndex
            || projection.graphLinks[first + 2u].targetNodeIndex
                != edge.endVertexNodeIndex
            || projection.graphLinks[first + 3u].targetNodeIndex
                != edge.coedgeNodeIndex
            || projection.graphLinks[first + 4u].targetNodeIndex
                != edge.curveNodeIndex) {
            hasKnownGaps = true;
        }
    }
    for (const DRW_AcisFace& face : projection.wireframe.faces) {
        const std::size_t first = findGraphLinkGroup(
            face.nodeIndex, 6u);
        if (first == missingGraphLinkGroup) {
            hasKnownGaps = true;
            continue;
        }
        if (projection.graphLinks[first + 3u].targetNodeIndex
                != face.shellNodeIndex
            || projection.graphLinks[first + 5u].targetNodeIndex
                != face.surfaceNodeIndex
            || face.loops.empty()
            || projection.graphLinks[first + 2u].targetNodeIndex
                != face.loops.front().nodeIndex) {
            hasKnownGaps = true;
        }
        for (std::size_t loopIndex = 0u;
             loopIndex < face.loops.size(); ++loopIndex) {
            const DRW_AcisFaceLoop& loop = face.loops[loopIndex];
            const std::size_t loopFirst = findGraphLinkGroup(
                loop.nodeIndex, 4u);
            const int firstCoedge = loop.coedgeNodeIndices.empty()
                ? -1 : loop.coedgeNodeIndices.front();
            if (loopFirst == missingGraphLinkGroup
                || loopIndex >= face.loopNextNodeIndices.size()
                || projection.graphLinks[loopFirst + 1u].targetNodeIndex
                    != face.loopNextNodeIndices[loopIndex]
                || projection.graphLinks[loopFirst + 2u].targetNodeIndex
                    != firstCoedge
                || projection.graphLinks[loopFirst + 3u].targetNodeIndex
                    != face.nodeIndex) {
                hasKnownGaps = true;
            }
            for (std::size_t coedgeIndex = 0u;
                 coedgeIndex < loop.coedgeNodeIndices.size(); ++coedgeIndex) {
                const std::size_t coedgeFirst = findGraphLinkGroup(
                    loop.coedgeNodeIndices[coedgeIndex], 7u);
                if (coedgeFirst == missingGraphLinkGroup
                    || coedgeIndex >= loop.coedgeNextNodeIndices.size()
                    || coedgeIndex >= loop.coedgePreviousNodeIndices.size()
                    || coedgeIndex >= loop.coedgePartnerNodeIndices.size()
                    || coedgeIndex >= loop.coedgeEdgeNodeIndices.size()
                    || coedgeIndex >= loop.coedgeLoopNodeIndices.size()
                    || coedgeIndex >= loop.coedgePCurveNodeIndices.size()) {
                    hasKnownGaps = true;
                    continue;
                }
                const DRW_AcisBodyGraphLink* links =
                    &projection.graphLinks[coedgeFirst];
                if (links[1].targetNodeIndex
                        != loop.coedgeNextNodeIndices[coedgeIndex]
                    || links[2].targetNodeIndex
                        != loop.coedgePreviousNodeIndices[coedgeIndex]
                    || links[3].targetNodeIndex
                        != loop.coedgePartnerNodeIndices[coedgeIndex]
                    || links[4].targetNodeIndex
                        != loop.coedgeEdgeNodeIndices[coedgeIndex]
                    || links[5].targetNodeIndex
                        != loop.coedgeLoopNodeIndices[coedgeIndex]
                    || links[6].targetNodeIndex
                        != loop.coedgePCurveNodeIndices[coedgeIndex]) {
                    hasKnownGaps = true;
                }
            }
        }
    }
    return hasKnownGaps
        ? DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
        : DRW_AcisProjectionCoverage::ProjectedSubsetNoKnownGaps;
}

//! Why rooted extraction did not produce a complete structural projection.
enum class DRW_AcisProjectionFailure {
    NoFailure,
#ifndef None
    //! Source-compatible spelling when the X11 None macro is not active.
    None = NoFailure,
#endif
    UnsupportedProfile,
    InvalidModel,
    InvalidReference,
    OwnerMismatch,
    ResourceLimit,
    AllocationFailure,
    UnexpectedFailure
};

//! Summary of graph nodes not reachable through the supported body/shell path.
//! These counts are structural; they do not classify nodes as semantic orphans.
struct DRW_AcisRootedProjectionReport {
    DRW_AcisProjectionFailure failure = DRW_AcisProjectionFailure::NoFailure;
    int failedNodeIndex = -1;
    std::size_t failedPointerOrdinal = std::numeric_limits<std::size_t>::max();
    std::size_t bodyCount = 0;
    std::size_t multiplyRootedNodeCount = 0;
    std::size_t verticesOutsideBodyRoots = 0;
    std::size_t edgesOutsideBodyRoots = 0;
    std::size_t facesOutsideBodyRoots = 0;
    std::size_t bodiesWithoutSupportedLump = 0;
};

//! Extract B-rep wireframe geometry from a resolved model. Never throws.
LIBDXFRW_EXPORT bool drw_extractAcisWireframe(const DRW_AcisModel& model, DRW_AcisBrep& out);

//! Apply the retained face-sense flag to a caller-provided surface normal.
//! The input is normalized with scale-safe arithmetic; `senseReversed` flips
//! its direction. `face.hasSense` must be true, and the caller is responsible
//! for supplying the normal of the corresponding surface at the desired point.
//! This does not derive surface derivatives, interpret trims/pcurves or
//! containment flags, or validate a B-rep. On failure, `faceNormal` is
//! unchanged.
LIBDXFRW_EXPORT bool drw_orientAcisFaceNormal(
    const DRW_AcisFace& face, const DRW_Coord& surfaceNormal,
    DRW_Coord& faceNormal) noexcept;

//! Evaluate the decoded ACIS straight-curve profile at its raw parameter.
//! The curve is evaluated as p0 + parameter * p1; the direction is not
//! normalized and the parameter is not clamped to an edge's endpoint range.
//! On failure, `point` is unchanged. This does not evaluate edge trims or
//! validate that an edge's endpoints lie on the curve.
LIBDXFRW_EXPORT bool drw_evaluateAcisStraightCurve(
    const DRW_AcisEdge& edge, double parameter, DRW_Coord& point) noexcept;

//! Evaluate the analytic ellipse described by a decoded ellipse-curve edge.
//! `parameter` is an angular parameter in radians. `frameTolerance` is a
//! caller-supplied dimensionless bound on the major-axis component along the
//! normalized normal; the major axis is projected into the normal plane only
//! when that discrepancy is within the supplied bound. The minor direction
//! follows normal cross major-axis (right-handed), and `ratio` scales its
//! radius. This does not clamp to edge endpoints, apply trims, or validate a
//! B-rep; no ACIS/modeler tolerance is inferred. On failure, `point` is
//! unchanged.
LIBDXFRW_EXPORT bool drw_evaluateAcisEllipseCurve(
    const DRW_AcisEdge& edge, double parameter, double frameTolerance,
    DRW_Coord& point) noexcept;

//! Project onto the full, untrimmed ellipse locus described by a decoded
//! ellipse-curve edge. The result is the closest point in 3D; the query's
//! normal offset is retained only in the distance being minimized. The
//! caller supplies the dimensionless frame tolerance used by
//! `drw_evaluateAcisEllipseCurve`. Queries with non-unique closest points or
//! numerically degenerate axes fail atomically. This does not apply edge
//! trims, ACIS parameter direction, placement, units, or B-rep validity.
LIBDXFRW_EXPORT bool drw_projectPointToAcisEllipseCurve(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double frameTolerance, DRW_Coord& projectedPoint) noexcept;

//! Test the Euclidean distance to the full, untrimmed ellipse locus against
//! an explicit finite nonnegative caller tolerance. Unlike point projection,
//! this distance-only query can accept points whose closest ellipse point is
//! nonunique; it does not publish one of the tied points. No ACIS tolerance,
//! edge trim, placement, units, or B-rep validity is inferred.
LIBDXFRW_EXPORT bool drw_pointWithinToleranceOfAcisEllipseCurve(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double frameTolerance, double linearTolerance) noexcept;

//! Check that both decoded endpoints of an ellipse edge lie on its full,
//! untrimmed analytic curve within an explicit finite nonnegative distance.
//! The frame tolerance has the same meaning as for
//! `drw_evaluateAcisEllipseCurve`. This validates endpoint/curve incidence
//! only; it does not derive a finite parameter interval, apply trims or
//! pcurves, or establish native ACIS/B-rep semantics.
LIBDXFRW_EXPORT bool drw_acisEllipseEdgeEndpointsWithinTolerance(
    const DRW_AcisEdge& edge, double frameTolerance,
    double endpointCurveTolerance) noexcept;

//! Project a point orthogonally onto the infinite line of a decoded ACIS
//! straight curve. `parameter` uses the raw p0 + t * p1 parameterization and
//! is not clamped to edge endpoints. On failure, both outputs are unchanged.
//! The caller chooses any tolerance used to compare the projected point with
//! the input; this helper does not apply ACIS tolerances or validate a B-rep.
LIBDXFRW_EXPORT bool drw_projectPointToAcisStraightCurve(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double& parameter, DRW_Coord& projectedPoint) noexcept;

//! Test whether a point lies within an explicit distance of the infinite line
//! of a decoded ACIS straight curve. `linearTolerance` is caller-supplied and
//! is not an ACIS/modeler default; the parameter uses raw p0 + t * p1 and is
//! not clamped to edge endpoints. On failure or an out-of-tolerance point,
//! `parameter` is unchanged.
LIBDXFRW_EXPORT bool drw_pointWithinToleranceOfAcisStraightCurve(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double linearTolerance, double& parameter) noexcept;

//! Project a point onto the finite interval of a decoded ACIS straight edge.
//! The interval is derived from the edge's two topological endpoints projected
//! onto its raw straight-curve parameterization. `endpointCurveTolerance` is
//! explicit and validates that both endpoints lie on that curve; it is not an
//! ACIS/modeler default. On failure, both outputs are unchanged.
LIBDXFRW_EXPORT bool drw_projectPointToAcisStraightEdge(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double endpointCurveTolerance, double& parameter,
    DRW_Coord& projectedPoint) noexcept;

//! Test distance from a point to a finite decoded straight ACIS edge.
//! `endpointCurveTolerance` validates that both topological endpoints lie on
//! the supporting curve; `linearTolerance` is the query-to-edge distance
//! tolerance. Both are explicit, finite, and nonnegative. The raw curve
//! parameter is published only when the bounded distance is accepted.
LIBDXFRW_EXPORT bool drw_pointWithinToleranceOfAcisStraightEdge(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double endpointCurveTolerance, double linearTolerance,
    double& parameter) noexcept;

//! Evaluate a decoded straight edge by its normalized topological endpoint
//! fraction: 0 is the edge start, 1 is the edge end. Endpoints must lie on the
//! supporting curve within the explicit finite nonnegative tolerance.
//! Fractions outside [0,1] fail; `point` is unchanged on failure.
LIBDXFRW_EXPORT bool drw_evaluateAcisStraightEdge(
    const DRW_AcisEdge& edge, double endpointFraction,
    double endpointCurveTolerance, DRW_Coord& point) noexcept;

//! Evaluate a straight edge in coedge traversal order. `coedgeSense` is 0
//! for raw edge endpoint order and 1 for reversed order, matching the rooted
//! projection's oriented endpoint identities. Unknown sense values fail.
//! The endpoint fraction is measured from the oriented coedge start; endpoint
//! consistency uses the explicit finite nonnegative tolerance. On failure,
//! `point` is unchanged. This does not evaluate pcurves/trims or face surfaces.
LIBDXFRW_EXPORT bool drw_evaluateAcisStraightCoedge(
    const DRW_AcisEdge& edge, std::int8_t coedgeSense,
    double traversalFraction, double endpointCurveTolerance,
    DRW_Coord& point) noexcept;

//! Project onto a finite straight coedge and return its normalized traversal
//! fraction (0 at the oriented coedge start, 1 at its end). `coedgeSense` is
//! 0 for raw edge order or 1 for reversed order. Degenerate parameter ranges,
//! unknown sense, or invalid endpoints fail atomically.
LIBDXFRW_EXPORT bool drw_projectPointToAcisStraightCoedge(
    const DRW_AcisEdge& edge, std::int8_t coedgeSense,
    const DRW_Coord& point, double endpointCurveTolerance,
    double& traversalFraction, DRW_Coord& projectedPoint) noexcept;

//! Test query distance to a finite straight coedge, returning its normalized
//! traversal fraction only when accepted. Endpoint and query tolerances are
//! explicit finite nonnegative values; no ACIS/modeler default is inferred.
//! On failure, `traversalFraction` is unchanged.
LIBDXFRW_EXPORT bool drw_pointWithinToleranceOfAcisStraightCoedge(
    const DRW_AcisEdge& edge, std::int8_t coedgeSense,
    const DRW_Coord& point, double endpointCurveTolerance,
    double linearTolerance, double& traversalFraction) noexcept;

//! Measure signed perpendicular distance from a point to a decoded ACIS plane.
//! The normal is normalized for the calculation but is not changed in `face`;
//! the sign follows its direction. `linearTolerance` is explicit and is not
//! an ACIS/modeler default. This tests the infinite plane, not face trims,
//! placement, units, or B-rep validity. On failure or an out-of-tolerance
//! result, `signedDistance` is unchanged.
LIBDXFRW_EXPORT bool drw_pointWithinToleranceOfAcisPlaneSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double linearTolerance, double& signedDistance) noexcept;

//! Project a point orthogonally onto the infinite plane of a decoded ACIS face.
//! Face trims, placement, units, and B-rep validity are not applied. On failure
//! or an unrepresentable projected coordinate, `projectedPoint` is unchanged.
LIBDXFRW_EXPORT bool drw_projectPointToAcisPlaneSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    DRW_Coord& projectedPoint) noexcept;

//! Return the unit normal of a decoded plane, adjusted by its retained
//! face-sense flag. The plane normal is constant over its untrimmed surface;
//! this does not apply trims, placement, units, containment, or B-rep checks.
//! `face.hasSense` must be true. On failure, `faceNormal` is unchanged.
LIBDXFRW_EXPORT bool drw_evaluateAcisPlaneFaceNormal(
    const DRW_AcisFace& face, DRW_Coord& faceNormal) noexcept;

//! Evaluate a local untrimmed plane parameterization from decoded fields:
//! origin + u*unitU + v*unitV, where unitU is the normalized projection of
//! the decoded U direction into the plane and unitV = unitNormal cross unitU.
//! `uParameter` and `vParameter` use drawing-coordinate units. The caller
//! supplies a dimensionless tolerance for U-direction orthogonality to the
//! normalized plane normal. This is an explicit local utility contract, not
//! an independently qualified ACIS parameter or trim contract. Placement,
//! units, and B-rep validity are not applied. On failure, `point` is unchanged.
LIBDXFRW_EXPORT bool drw_evaluateAcisPlaneSurface(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& point) noexcept;

//! Evaluate a local untrimmed cone-surface parameterization from decoded
//! fields: origin + (R + v*sinAngle) * (cos(u)*majorUnit +
//! ratio*sin(u)*minorUnit) + v*cosAngle*axisUnit, where R is the magnitude
//! of the major axis after projection into the axis-normal plane and
//! minorUnit = normalize(axisUnit cross majorUnit). `uParameter` is radians;
//! `vParameter` is the signed surface-generator parameter (not necessarily arc
//! length when ratio differs from one). The caller supplies a dimensionless
//! tolerance for axis/major orthogonality and sin/cos
//! normalization. A positive finite ratio is accepted, but this local formula
//! is not an independently qualified ACIS semantic contract. Trims,
//! placement, units, and B-rep validity are not applied. On failure, `point`
//! is unchanged.
LIBDXFRW_EXPORT bool drw_evaluateAcisConeSurface(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& point) noexcept;

//! Evaluate the oriented unit normal implied by the local cone parameterization
//! `dP/du cross dP/dv`, then apply the retained face-sense flag. `uParameter`
//! is radians; `vParameter` is the signed generator parameter used by
//! `drw_evaluateAcisConeSurface`. The apex (`R + v*sinAngle == 0`) is
//! singular and fails. This is the normal of the documented local utility
//! parameterization, not an independently qualified native ACIS contract; no
//! trims, placement, units, or B-rep validity are applied. On failure,
//! `faceNormal` is unchanged.
LIBDXFRW_EXPORT bool drw_evaluateAcisConeFaceNormal(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& faceNormal) noexcept;

//! Project onto an untrimmed cone surface represented by decoded fields.
//! Circular profiles use the exact meridian solve; non-circular positive
//! ratios use the stationary points of the local elliptic-generator distance
//! function. Both nappes are considered. The explicit frame tolerance has the
//! same meaning as for `drw_evaluateAcisConeSurface`. Refuse non-unique closest
//! points (including symmetric axial queries), trims, and invalid or
//! numerically unresolvable inputs atomically. Non-circular cylinder limits
//! are projected as ellipses in their normal planes. An axial
//! circular-cone query within a 16-epsilon scaled residual of the analytic
//! apex is treated as the apex to absorb floating-point rounding. Placement,
//! units, and B-rep validity are not applied.
LIBDXFRW_EXPORT bool drw_projectPointToAcisConeSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double frameTolerance, DRW_Coord& projectedPoint) noexcept;

//! Test unsigned distance to the same untrimmed cone locus against an explicit
//! finite nonnegative linear tolerance. Unlike projection, this distance-only
//! predicate accepts non-unique closest points. No modeler tolerance, trim,
//! placement, unit scale, or B-rep validity is inferred.
LIBDXFRW_EXPORT bool drw_pointWithinToleranceOfAcisConeSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double frameTolerance, double linearTolerance) noexcept;

//! Project onto the untrimmed circular-cylinder locus represented by the
//! canonical ACIS cone-surface profile (ratio=1, sinAngle=0, cosAngle=1).
//! The cylinder axis/origin come from the decoded surface and its radius is
//! the magnitude of the decoded major-axis vector. Other cone profiles,
//! trims, placement, units, and B-rep validity are not handled. Points on the
//! axis have no unique closest point. On failure or an unrepresentable result,
//! `projectedPoint` is unchanged.
LIBDXFRW_EXPORT bool drw_projectPointToAcisCylinderSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    DRW_Coord& projectedPoint) noexcept;

//! Test signed radial distance to the canonical untrimmed ACIS cylinder
//! locus (the same exact cone-surface profile accepted by
//! `drw_projectPointToAcisCylinderSurface`). Negative means inside. The
//! caller supplies a finite nonnegative tolerance; no modeler default, trims,
//! placement, units, or B-rep validity are applied. On failure or an
//! out-of-tolerance result, `signedDistance` is unchanged.
LIBDXFRW_EXPORT bool drw_pointWithinToleranceOfAcisCylinderSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double linearTolerance, double& signedDistance) noexcept;

//! Test signed distance to the untrimmed locus of a regular ring torus.
//! This accepts only finite faces with majorRadius > minorRadius > 0 and a
//! nonzero axis. The axis is normalized for calculation; the stored frame and
//! U direction are unchanged. `linearTolerance` is explicit, not an
//! ACIS/modeler default. This does not apply trims, placement, units, or
//! validate a B-rep. On failure or an out-of-tolerance result,
//! `signedDistance` is unchanged.
LIBDXFRW_EXPORT bool drw_pointWithinToleranceOfAcisTorusSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double linearTolerance, double& signedDistance) noexcept;

//! Project a point to the untrimmed torus locus. Only finite faces with
//! majorRadius > 0, minorRadius > 0, and a nonzero axis are supported, including
//! horn/spindle profiles. Refuse any point whose closest locus point is
//! nonunique (commonly on the axis or major circle). Frame U direction and
//! trims are not applied. On failure or an unrepresentable result,
//! `projectedPoint` is unchanged.
LIBDXFRW_EXPORT bool drw_projectPointToAcisTorusSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    DRW_Coord& projectedPoint) noexcept;

//! Evaluate an untrimmed local torus parameterization from decoded fields:
//! center + (majorRadius + minorRadius*cos(v)) *
//! (cos(u)*uDir + sin(u)*(axis cross uDir)) +
//! minorRadius*sin(v)*axis. Parameters are radians. The caller supplies a
//! dimensionless tolerance for U-direction orthogonality to the normalized
//! axis. This is a local utility contract, not an independently qualified
//! ACIS parameter-direction or trim contract. Placement, units, and B-rep
//! validity are not applied. On failure, `point` is unchanged.
LIBDXFRW_EXPORT bool drw_evaluateAcisTorusSurface(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& point) noexcept;

//! Evaluate the oriented unit normal implied by the local torus
//! parameterization `dP/du cross dP/dv`, then apply the retained face-sense
//! flag. `uParameter` and `vParameter` are radians. A horn/spindle pinch where
//! `majorRadius + minorRadius*cos(v) == 0` is singular and fails. This local
//! utility does not establish native ACIS parameter/face-sense semantics or
//! apply trims, placement, units, or B-rep validation. On failure,
//! `faceNormal` is unchanged.
LIBDXFRW_EXPORT bool drw_evaluateAcisTorusFaceNormal(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& faceNormal) noexcept;

//! Test whether a point lies within an explicit distance of a decoded sphere
//! locus. The signed result is radial distance minus the magnitude of the
//! stored radius (negative inside); a zero-radius locus is its center point.
//! Unlike projection onto a nondegenerate sphere, the center is valid. This
//! uses no ACIS/modeler tolerance and leaves `signedDistance` unchanged on
//! invalid input or an out-of-tolerance result.
LIBDXFRW_EXPORT bool drw_pointWithinToleranceOfAcisSphereSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double linearTolerance, double& signedDistance) noexcept;

//! Project a point onto the sphere locus of a decoded ACIS sphere face.
//! The signed radius is used by magnitude; face orientation/frame and trims do
//! not affect the locus. The center of a nonzero-radius sphere has no unique
//! projection and fails; a zero-radius locus projects uniquely to its center.
//! On failure or an unrepresentable point, `projectedPoint` is unchanged.
LIBDXFRW_EXPORT bool drw_projectPointToAcisSphereSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    DRW_Coord& projectedPoint) noexcept;

//! Return the unit outward radial normal of a decoded sphere, adjusted by its
//! retained face-sense flag. `point` must be within the explicit finite,
//! nonnegative `linearTolerance` of the untrimmed sphere locus. The signed
//! radius contributes by magnitude; its sign does not reverse the radial
//! direction. A zero-radius sphere or its ambiguous center normal fails.
//! Placement, trims, containment, and B-rep validity are not applied. On
//! failure, `faceNormal` is unchanged.
LIBDXFRW_EXPORT bool drw_evaluateAcisSphereFaceNormal(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double linearTolerance, DRW_Coord& faceNormal) noexcept;

//! Evaluate a local sphere parameterization from decoded center, pole axis,
//! U direction, and signed radius:
//! center + radius*(cos(v)*(cos(u)*uDir +
//! sin(u)*(poleAxis cross uDir)) + sin(v)*poleAxis). Parameters are radians.
//! The caller supplies a dimensionless tolerance for U-direction
//! orthogonality to the normalized pole axis. This is a local utility contract,
//! not independently qualified ACIS parameter direction or trim semantics.
//! Placement, units, and B-rep validity are not applied. On failure, `point`
//! is unchanged.
LIBDXFRW_EXPORT bool drw_evaluateAcisSphereSurface(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& point) noexcept;

//! Extract the same legacy raw-record inventory and report unresolved fields.
//! A successful return only means this bounded projection ran; it does not
//! select a body root, apply placement/units, or validate a closed B-rep.
LIBDXFRW_EXPORT bool drw_extractAcisWireframeWithReport(
    const DRW_AcisModel& model, DRW_AcisBrep& out,
    DRW_AcisProjectionReport& report);

//! Project ACIS BinaryFile 21800 body graphs independently.
//! The supported links follow the pinned ezdxf 1.4.4 source profile; this is
//! implementation corroboration, not a normative ACIS schema. Body transforms
//! and source-unit scaling remain unapplied; unrelated geometry is counted in
//! the summary rather than merged into a body projection. Never throws.
//! On false, `out` is empty and `report.failure` explains the refusal;
//! node/pointer context is populated when the model error is localized.
LIBDXFRW_EXPORT bool drw_extractAcisBodyWireframes(
    const DRW_AcisModel& model, std::vector<DRW_AcisBodyProjection>& out,
    DRW_AcisRootedProjectionReport& report,
    const DRW_SabParseLimits& limits = DRW_SabParseLimits());

//! Convenience entry: locate the SAB signature in `raw`, parse -> build -> extract.
/*!
 *  Returns false (and leaves `out` empty) for non-SAB or malformed input.
 *  NEVER throws.
 */
LIBDXFRW_EXPORT bool drw_decodeAcisWireframe(const std::vector<unsigned char>& raw, DRW_AcisBrep& out);

//! Decode a SAB projection while reporting the candidate stream's exact span.
//! The offsets are relative to the signature-aligned candidate buffer.
LIBDXFRW_EXPORT bool drw_decodeAcisWireframeWithOutcome(
    const std::vector<unsigned char>& raw, DRW_AcisBrep& out,
    DRW_SabParseOutcome& outcome);

//! Convenience decoder with caller-selected parser limits.
LIBDXFRW_EXPORT bool drw_decodeAcisWireframeWithLimits(
    const std::vector<unsigned char>& raw, DRW_AcisBrep& out,
    DRW_SabParseOutcome& outcome, const DRW_SabParseLimits& limits);

//! Decode a wireframe and return the parsed source SAB header separately.
/*! Coordinates in `out` are not scaled by `header.unitsInMm`; that field is
 *  returned as source metadata only. The return/completeness contract matches
 *  drw_decodeAcisWireframeWithLimits. On false, `out` and `header` are reset.
 */
LIBDXFRW_EXPORT bool drw_decodeAcisWireframeWithHeader(
    const std::vector<unsigned char>& raw, DRW_AcisBrep& out,
    DRW_SabHeader& header, DRW_SabParseOutcome& outcome,
    const DRW_SabParseLimits& limits = DRW_SabParseLimits());

#endif // DRW_ACIS_H
