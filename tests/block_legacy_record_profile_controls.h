// Test helper sketch; include after dx_legacy_block_record_source_profile.h.
// The source is the locally authored 18-group AC1018 BLOCK_RECORD fixture.
bool legacyBlockRecordProfileDirectControls(const DRW_RawDxfObject& clean) {
 using namespace dx_legacy_block_record_source;
 const auto expect=[&](DRW_RawDxfObject raw,Reason reason,std::size_t at){const auto v=audit(raw);
  if(!check(v.reason==reason&&v.rejectedGroup==at&&v.spans.empty(),"legacy BLOCK_RECORD profile negative reason and index")) {
   std::cerr<<"expected="<<static_cast<int>(reason)<<'/'<<at<<" got="<<static_cast<int>(v.reason)<<'/'<<v.rejectedGroup<<'\n';return false;}
  return true;};
 auto bad=clean;bad.m_version=DRW::AC1021;if(!expect(bad,Reason::Revision,0))return false;
 bad=clean;bad.name="BLOCK";if(!expect(bad,Reason::Identity,0))return false;
 bad=clean;bad.groups[0]=DRW_Variant(5,std::string("A2"));if(!expect(bad,Reason::Identity,0))return false;
 bad=clean;bad.groups[2]=DRW_Variant(360,std::string("0"));if(!expect(bad,Reason::Application,2))return false;
 bad=clean;bad.groups[4]=DRW_Variant(330,std::string("B1"));if(!expect(bad,Reason::Identity,4))return false;
 bad=clean;bad.groups[5]=DRW_Variant(100,std::string("AcDbBlockTableRecord"));if(!expect(bad,Reason::Unexpected,5))return false;
 bad=clean;bad.groups[7]=DRW_Variant(2,std::string("caf\xE9",4u));if(!expect(bad,Reason::InvalidValue,7))return false;
 bad=clean;bad.groups.erase(bad.groups.begin()+8);if(!expect(bad,Reason::InvalidValue,8))return false;
 bad=clean;bad.groups[8]=DRW_Variant(340,std::string("1"));if(!expect(bad,Reason::InvalidValue,8))return false;
 bad=clean;bad.groups.insert(bad.groups.begin()+8,DRW_Variant(310,std::string("00")));if(!expect(bad,Reason::InvalidValue,8))return false;
 bad=clean;bad.groups[9]=DRW_Variant(102,std::string("{OTHER"));if(!expect(bad,Reason::Application,9))return false;
 bad=clean;bad.groups[10]=DRW_Variant(331,std::string("0"));if(!expect(bad,Reason::Application,10))return false;
 bad=clean;bad.groups[10]=DRW_Variant(331,std::int32_t(1));if(!expect(bad,Reason::Application,10))return false;
 bad=clean;bad.groups.insert(bad.groups.begin()+11,DRW_Variant(331,std::string("E1")));if(!expect(bad,Reason::Application,11))return false;
 bad=clean;bad.groups[12]=DRW_Variant(1001,std::string(""));if(!expect(bad,Reason::Xdata,12))return false;
 bad=clean;bad.groups[12]=DRW_Variant(1001,std::string(32u,'A'));if(!expect(bad,Reason::Xdata,12))return false;
 bad=clean;bad.groups[13]=DRW_Variant(1000,std::string(256u,'x'));if(!expect(bad,Reason::InvalidValue,13))return false;
 bad=clean;bad.groups[13]=DRW_Variant(1000,std::string("a\0b",3u));if(!expect(bad,Reason::InvalidValue,13))return false;
 bad=clean;bad.groups[13]=DRW_Variant(1000,std::int32_t(1));if(!expect(bad,Reason::InvalidValue,13))return false;
 bad=clean;bad.groups.insert(bad.groups.begin()+14,DRW_Variant(1001,std::string("test_app")));if(!expect(bad,Reason::Duplicate,14))return false;
 bad=clean;bad.groups.insert(bad.groups.begin()+14,DRW_Variant(102,std::string("{OTHER")));if(!expect(bad,Reason::Unexpected,14))return false;
 bad=clean;bad.groups[15]=DRW_Variant(1070,std::int32_t(32768));if(!expect(bad,Reason::InvalidValue,15))return false;
 bad=clean;bad.groups[15]=DRW_Variant(1070,std::string("1"));if(!expect(bad,Reason::InvalidValue,15))return false;
 bad=clean;bad.groups[15]=DRW_Variant(1005,std::string("A"));if(!expect(bad,Reason::Unexpected,15))return false;
 bad=clean;bad.groups.pop_back();if(!expect(bad,Reason::Xdata,17))return false;
 bad=clean;for(unsigned i=0;i<240u;++i)bad.groups.emplace_back(1000,std::string("x"));if(!expect(bad,Reason::Limit,0))return false;
 return true;
}
