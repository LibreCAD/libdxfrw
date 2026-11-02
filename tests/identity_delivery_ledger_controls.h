// SPDX-License-Identifier: GPL-2.0-or-later
// Codec-owned delivery authentication is not namespace or graph qualification.
bool identityDeliveryLedgerTests(const std::filesystem::path& path) {
    using Source=DRW_DxfIdentitySource;
    struct Facet final:dx_iface {
        dxfRW* reader=nullptr;Source captured;dxfRW::DxfIdentityDeliveryTicketPtr ticket;bool delivered=false,throwSource=false,reenter=false,activeSafe=true,drop=false,selected=true;
        bool wantsDxfIdentitySource()const override{return selected;}
        void addHeader(const DRW_Header*)override{}
        void addLayer(const DRW_Layer&)override{}
        void addDxfIdentitySource(const Source& source)override {
            const auto received=reader->dxfIdentityDeliveryTicket(source);
            if(reader->dxfIdentityDeliveryState().generation||reader->dxfIdentityDeliveryMatches(source,received)
                ||reader->enableDxfIdentityDeliveryLedger())activeSafe=false;
            if(reenter){std::string nested="0\nEOF\n";if(reader->readAscii(this,false,nested))activeSafe=false;}
            if(!drop){captured=source;ticket=received;delivered=true;}
            if(throwSource)throw std::runtime_error("identity ledger callback");
        }
    };
    const Tags doc{{0,"SECTION"},{2,"HEADER"},{999,"ignored"},{9,"$ACADVER"},{1,"AC1018"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"LAYER"},{5,"10"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"LAYER"},{5,"11"},{330,"10"},{100,"AcDbSymbolTableRecord"},{2,"0"},{70,"0"},
        {100,"AcDbLayerTableRecord"},{62,"7"},{6,"CONTINUOUS"},{0,"ENDTAB"},{0,"ENDSEC"},{0,"EOF"}};
    unsigned positive=0u,quotas=0u,fieldFaults=0u,retries=0u,contexts=0u,allocations=0u;
    for(bool binary:{false,true}) {
        if(!write(path,doc,binary))return false;
        dxfRW reader(path.string().c_str());Facet facet;facet.reader=&reader;
        if(!reader.enableDxfIdentityDeliveryLedger()||!reader.read(&facet,false)||!facet.delivered||!facet.activeSafe)return false;
        const auto state=reader.dxfIdentityDeliveryState();const auto original=facet.captured;const auto firstTicket=facet.ticket;
        if(!state.enabled||!state.captured||!state.complete||state.failed||!state.generation
            ||state.sections!=original.sections.size()||state.records!=original.records.size()||state.candidates!=original.candidates.size()
            ||!reader.dxfIdentityDeliveryMatches(original,firstTicket)||reader.dxfIdentityDeliveryTicket(original)
            ||reader.dxfIdentityDeliveryMatches(original,{}))return false;
        if(original.sections.empty()||original.records.empty()||original.candidates.empty())return false;
        // Same address with an unrelated shared owner is not the codec ticket.
        dxfRW::DxfIdentityDeliveryTicketPtr forged(firstTicket.get(),[](const auto*){});
        if(reader.dxfIdentityDeliveryMatches(original,forged))return false;
        dxfRW other(path.string().c_str());Facet otherFacet;otherFacet.reader=&other;
        if(!other.enableDxfIdentityDeliveryLedger()||!other.read(&otherFacet,false)
            ||!other.dxfIdentityDeliveryMatches(otherFacet.captured,otherFacet.ticket)
            ||other.dxfIdentityDeliveryMatches(original,firstTicket)
            ||reader.dxfIdentityDeliveryMatches(original,otherFacet.ticket))return false;
        ++positive;
        // Exercise every scalar, textual and span field, including comment flags.
        for(unsigned field=0u;field<38u;++field){auto changed=original;
            switch(field){
            case 0:changed.status=Source::Status::Quota;break;case 1:changed.version=DRW::AC1021;break;case 2:changed.binary=!changed.binary;break;
            case 3:++changed.bytes;break;case 4:++changed.work;break;case 5:++changed.scalars;break;case 6:++changed.ignoredComments;break;case 7:++changed.unmatchedAdmissions;break;
            case 8:++changed.eofMarker.ordinal;break;case 9:++changed.eofMarker.beginOffset;break;case 10:++changed.eofMarker.endOffset;break;case 11:changed.eofMarker.ignoredComment=!changed.eofMarker.ignoredComment;break;
            case 12:changed.sections[0].kind=Source::Section::Other;break;case 13:changed.sections[0].name+="x";break;case 14:++changed.sections[0].marker.ordinal;break;case 15:changed.sections[0].endMarker.ignoredComment=true;break;
            case 16:changed.records[0].kind=Source::Kind::Unclassified;break;case 17:changed.records[0].name+="x";break;case 18:changed.records[0].tableName+="x";break;
            case 19:++changed.records[0].section;break;case 20:++changed.records[0].table;break;case 21:++changed.records[0].firstCandidate;break;case 22:++changed.records[0].candidates;break;
            case 23:++changed.records[0].marker.beginOffset;break;case 24:changed.records[0].marker.ignoredComment=true;break;case 25:++changed.records[0].endMarker.endOffset;break;case 26:changed.records[0].endMarker.ignoredComment=true;break;
            case 27:++changed.candidates[0].record;break;case 28:++changed.candidates[0].section;break;case 29:++changed.candidates[0].code;break;
            case 30:changed.candidates[0].scope=Source::Scope::Application;break;case 31:changed.candidates[0].admission=Source::Admission::Invalid;break;
            case 32:++changed.candidates[0].repeatedAdmissions;break;case 33:changed.candidates[0].lexeme+="0";break;case 34:changed.candidates[0].rawValue+="0";break;
            case 35:++changed.candidates[0].span.ordinal;break;case 36:++changed.candidates[0].span.beginOffset;break;case 37:changed.candidates[0].span.ignoredComment=true;break;
            }
            if(reader.dxfIdentityDeliveryMatches(changed,firstTicket))return false;++fieldFaults;
        }
        // Every component of each physical span, including ignored-comment
        // flags, is independently authenticated; cardinality changes cannot
        // hide a missing or duplicated row.
        for(unsigned which=0u;which<6u;++which)for(unsigned component=0u;component<4u;++component) {
            auto changed=original;
            std::array<DRW_DxfPhysicalRecordSpan*,6> spans{{&changed.eofMarker,&changed.sections[0].marker,&changed.sections[0].endMarker,
                &changed.records[0].marker,&changed.records[0].endMarker,&changed.candidates[0].span}};
            auto& span=*spans[which];
            if(component==0u)++span.ordinal;else if(component==1u)++span.beginOffset;
            else if(component==2u)++span.endOffset;else span.ignoredComment=!span.ignoredComment;
            if(reader.dxfIdentityDeliveryMatches(changed,firstTicket))return false;++fieldFaults;
        }
        for(unsigned which=0u;which<3u;++which)for(bool remove:{false,true}) {
            auto changed=original;
            if(which==0u){if(remove)changed.sections.pop_back();else changed.sections.push_back(changed.sections.back());}
            else if(which==1u){if(remove)changed.records.pop_back();else changed.records.push_back(changed.records.back());}
            else {if(remove)changed.candidates.pop_back();else changed.candidates.push_back(changed.candidates.back());}
            if(reader.dxfIdentityDeliveryMatches(changed,firstTicket))return false;++fieldFaults;
        }
        const std::array<std::size_t,5> usage{{state.sections,state.records,state.candidates,state.bytes,state.work}};
        for(unsigned which=0u;which<usage.size();++which)for(bool exact:{false,true}) {
            if(!usage[which])return false;auto limits=usage;limits[which]-=exact?0u:1u;
            dxfRW bounded(path.string().c_str());Facet receiver;receiver.reader=&bounded;
            if(!bounded.enableDxfIdentityDeliveryLedger(limits[0],limits[1],limits[2],limits[3],limits[4])
                ||!bounded.read(&receiver,false)||!receiver.delivered)return false;
            const auto outcome=bounded.dxfIdentityDeliveryState();
            if(outcome.complete!=exact||outcome.failed==exact||bounded.dxfIdentityDeliveryMatches(receiver.captured,receiver.ticket)!=exact)return false;++quotas;
            if(!exact){if(!bounded.enableDxfIdentityDeliveryLedger()||!bounded.read(&receiver,false))return false;
                const auto retry=bounded.dxfIdentityDeliveryState();if(!retry.complete||!bounded.dxfIdentityDeliveryMatches(receiver.captured,receiver.ticket))return false;++retries;}
        }
        {
            dxfRW isolated("unused");if(!isolated.enableDxfIdentityDeliveryLedger())return false;
            bool eventually=false;
            for(long failAfter=0;failAfter<64&&!eventually;++failAfter) {
                if(!DxIfaceStandardDwgCaptureTestAccess::identityCaptureAllocation(isolated,original,failAfter))return false;
                const auto outcome=isolated.dxfIdentityDeliveryState();
                if(outcome.failed)++allocations;else eventually=outcome.captured;
            }
            if(!eventually)return false;
        }
        // Same reader, identical source, old token, reentrant attempt and thrown
        // callback: no previous-generation or partial operation can authenticate.
        facet.reenter=true;facet.delivered=false;if(!reader.read(&facet,false)||!facet.delivered||!facet.activeSafe)return false;
        const auto later=reader.dxfIdentityDeliveryState();if(!later.complete||later.generation<=state.generation
            ||reader.dxfIdentityDeliveryMatches(original,firstTicket)||!reader.dxfIdentityDeliveryMatches(facet.captured,facet.ticket))return false;
        ++contexts;
        const auto secondTicket=facet.ticket;
        facet.throwSource=true;bool threw=false;try{(void)reader.read(&facet,false);}catch(const std::runtime_error&){threw=true;}
        if(!threw||reader.dxfIdentityDeliveryState().complete||reader.dxfIdentityDeliveryMatches(original,secondTicket))return false;
        facet.throwSource=false;
        std::string partial="0\nSECTION\n2\nENTITIES\n0\nPOINT\n5\n30\n10\n";
        if(reader.readAscii(&facet,false,partial)||reader.dxfIdentityDeliveryState().complete
            ||reader.dxfIdentityDeliveryMatches(original,secondTicket))return false;
        facet.drop=true;facet.delivered=false;if(!reader.read(&facet,false)||facet.delivered)return false;
        const auto dropped=reader.dxfIdentityDeliveryState();if(!dropped.complete||reader.dxfIdentityDeliveryMatches(original,secondTicket))return false;
        ++contexts;
        facet.drop=false;facet.reenter=false;if(!reader.read(&facet,false))return false;const auto recovered=reader.dxfIdentityDeliveryState();
        if(!recovered.complete||!reader.dxfIdentityDeliveryMatches(facet.captured,facet.ticket))return false;++retries;
        dxfRW unselected(path.string().c_str());Facet noCallback;noCallback.reader=&unselected;noCallback.selected=false;
        if(!unselected.enableDxfIdentityDeliveryLedger()||!unselected.read(&noCallback,false)||noCallback.delivered
            ||unselected.dxfIdentityDeliveryState().complete||unselected.dxfIdentityDeliveryMatches(original,firstTicket))return false;
        ++contexts;
        dxfRW ordinary(path.string().c_str());Facet disabled;disabled.reader=&ordinary;
        if(!ordinary.read(&disabled,false)||!disabled.delivered||ordinary.dxfIdentityDeliveryState().enabled
            ||ordinary.dxfIdentityDeliveryMatches(disabled.captured,disabled.ticket))return false;
        ++contexts;
    }
    std::cout<<"identity ledger profiles="<<positive<<" field-faults="<<fieldFaults<<" quotas="<<quotas<<" retries="<<retries<<" contexts="<<contexts<<" allocations="<<allocations<<'\n';
    return positive==2u&&fieldFaults==136u&&quotas==20u&&retries==12u&&contexts==8u&&allocations>0u;
}
