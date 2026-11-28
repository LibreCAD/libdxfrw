#ifndef LIBDXFRW_DWG_ELLIPSE_WCS_TEST_H
#define LIBDXFRW_DWG_ELLIPSE_WCS_TEST_H

// ODA v5.4.1 section 20.4.39: both ELLIPSE center and major axis are WCS.
class EllipseWcsInterface final : public dx_iface {
public:
    explicit EllipseWcsInterface(dwgRW* writer = nullptr) : writer_(writer) {
        cData = &data_;
        currentBlock = data_.mBlock;
    }
    void writeHeader(DRW_Header& header) override { header.vars.clear(); }
    void writeEntities() override {
        DRW_Ellipse ellipse;
        ellipse.basePoint = {1.0, 2.0, 3.0};
        ellipse.secPoint = {4.0, -3.0, 0.0};
        ellipse.extPoint = {0.36, 0.48, -0.8};
        ellipse.ratio = 0.5;
        ellipse.staparam = 0.3;
        ellipse.endparam = 4.1;
        DRW_Line sentinel;
        sentinel.basePoint = {101.0, 102.0, 103.0};
        sentinel.secPoint = {104.0, 105.0, 106.0};
        wrote_ = writer_ && writer_->writeEllipse(&ellipse)
            && writer_->writeLine(&sentinel);
    }
    bool wrote() const { return wrote_; }
    bool matchesOdaSource() const {
        const auto near = [](double a, double b) { return std::abs(a-b) < 1.e-12; };
        unsigned count = 0;
        for (const DRW_Entity* entity : data_.mBlock->ent) {
            const auto* e = dynamic_cast<const DRW_Ellipse*>(entity);
            if (!e) continue;
            if (e->handle != 0x32u) continue;
            ++count;
            if (!near(e->basePoint.x,-10.) || !near(e->basePoint.y,5.)
                || !near(e->basePoint.z,2.) || !near(e->secPoint.x,2.)
                || !near(e->secPoint.y,0.) || !near(e->secPoint.z,-1.5)
                || !near(e->extPoint.x,0.6) || !near(e->extPoint.y,0.)
                || !near(e->extPoint.z,0.8) || !near(e->ratio,0.5)
                || !near(e->staparam,0.25) || !near(e->endparam,1.75))
                return false;
        }
        return count == 1;
    }
    bool matches() const {
        const auto near = [](double a, double b) { return std::abs(a-b) < 1.e-12; };
        const auto same = [&near](const DRW_Coord& a, const DRW_Coord& b) {
            return near(a.x,b.x) && near(a.y,b.y) && near(a.z,b.z);
        };
        unsigned count = 0, lines = 0;
        for (const DRW_Entity* entity : data_.mBlock->ent) {
            if (const auto* ellipse = dynamic_cast<const DRW_Ellipse*>(entity)) {
                if (++count != 1 || !same(ellipse->basePoint, {1.,2.,3.})
                    || !same(ellipse->secPoint, {4.,-3.,0.})
                    || !same(ellipse->extPoint, {0.36,0.48,-0.8})
                    || !near(ellipse->ratio,0.5) || !near(ellipse->staparam,0.3)
                    || !near(ellipse->endparam,4.1)) return false;
            } else if (const auto* line = dynamic_cast<const DRW_Line*>(entity)) {
                if (++lines != 1 || !same(line->basePoint, {101.,102.,103.})
                    || !same(line->secPoint, {104.,105.,106.})) return false;
            } else return false;
        }
        return count == 1 && lines == 1;
    }
private:
    dwgRW* writer_;
    dx_data data_;
    bool wrote_ {false};
};

bool runDwgEllipseWcsExtrusion(const std::filesystem::path& directory) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (const DRW::Version version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                       DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        const auto path = directory / ("ellipse-wcs-" + std::to_string(nonce)
                                       + "-" + std::to_string(version) + ".dwg");
        struct Cleanup { std::filesystem::path path; ~Cleanup() {
            std::error_code error; std::filesystem::remove(path,error);
        }} cleanup{path};
        dwgRW writer(path.string().c_str());
        EllipseWcsInterface source(&writer);
        if (!writer.write(&source, version, true) || !source.wrote()) {
            std::cerr << "ELLIPSE source write failed for " << version << '\n';
            return false;
        }
        for (const bool apply : {false, true}) {
            dwgRW reader(path.string().c_str());
            EllipseWcsInterface capture;
            if (!reader.read(&capture, apply) || !capture.matches()) {
                std::cerr << "ELLIPSE WCS mismatch for " << version
                          << ", applyExtrusion=" << apply << '\n';
                return false;
            }
        }
    }
    return true;
}

bool runDwgEllipseWcsOdaSource(const std::filesystem::path& path) {
    for (const bool apply : {false, true}) {
        dwgRW reader(path.string().c_str());
        EllipseWcsInterface capture;
        if (!reader.read(&capture, apply) || !capture.matchesOdaSource()) {
            std::cerr << "ODA source ELLIPSE WCS mismatch, applyExtrusion="
                      << apply << '\n';
            return false;
        }
    }
    return true;
}

#endif
