/* Compare the adapter's INSERT-shaped TABLE projection to its immutable,
 * reader-origin TABLE model. No block-membership or output claim is made. */
#ifndef DWG_TABLE_PROJECTION_AC1018_CANDIDATE_H
#define DWG_TABLE_PROJECTION_AC1018_CANDIDATE_H
#include "dwg_table_model_ac1018_candidate.h"
#include "dxfcompound.h"
#include <typeinfo>
namespace DwgTableProjectionAc1018Candidate {
// This exact profile models dx_iface's complete-TABLE projection policy: the
// duplicate proxy bytes are cleared and replay authorization is revoked.
// Allocation failure refuses the match without altering either input.
inline bool matches(const DRW_Table& source,const DRW_Entity& candidate) noexcept {
    if(!DwgTableModelAc1018Candidate::bounded(source)
        || typeid(candidate)!=typeid(DRW_Insert) || candidate.eType!=DRW::INSERT
        || !candidate.extData.empty() || !candidate.appData.empty() || !candidate.reactorHandles.empty()
        || candidate.hasDataStorageRecord || candidate.hasDataStorageBinaryData()
        || !candidate.dataStorageHandleKey.empty() || !candidate.dataStorageData.empty()
        || !candidate.dataStoragePayloadMarkerSection.empty()
        || !candidate.pendingAppIdResolutions.empty() || !candidate.pendingLayerRefResolutions.empty()
        || !dxfAttdef::Decoder::hasNoDxfAccountingLease(candidate)
        || !dxfAttdef::Decoder::hasNoEntityNativeWriteBindings(candidate))return false;
    const auto& actual=static_cast<const DRW_Insert&>(candidate);
    if(!actual.attlist.empty() || actual.seqEndEntity || !actual.attribHandles.empty())return false;
    try {
        DRW_Insert expected(source);expected.numProxyGraph=0;expected.proxyGraphics.clear();
        expected.revokeProxyGraphicsReplayAuthorization();
        if(expected.proxyGraphicsProvenance().resourceIndicesRemappedForDxf
            !=actual.proxyGraphicsProvenance().resourceIndicesRemappedForDxf)return false;
        return dxfCompound::Decoder::sameInsertModel(expected,actual,
            [](const DRW_Variant& a,const DRW_Variant& b) noexcept {
                return dxfEntitiesAuth::VariantAccess::equal(a,b);
            });
    } catch(...) {return false;}
}
}
#endif
