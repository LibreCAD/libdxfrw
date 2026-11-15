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

#include "drw_base.h"
#include "intern/drw_dbg.h"
#include "intern/drw_textcodec.h"

class DRW_LegacyCodePageDecoder::Impl {
public:
    DRW_TextCodec codec;
};

DRW_LegacyCodePageDecoder::DRW_LegacyCodePageDecoder(int codePage)
{
    const std::string name = "ANSI_" + std::to_string(codePage);
    auto impl = std::make_unique<Impl>();
    impl->codec.setCodePage(name, false);
    // Unknown pages historically fall back to ANSI_1252; do not report that
    // fallback as the requested mapping.
    if (impl->codec.getCodePage() == name)
        m_impl = std::move(impl);
}

DRW_LegacyCodePageDecoder::~DRW_LegacyCodePageDecoder() = default;
DRW_LegacyCodePageDecoder::DRW_LegacyCodePageDecoder(DRW_LegacyCodePageDecoder&&) noexcept = default;
DRW_LegacyCodePageDecoder& DRW_LegacyCodePageDecoder::operator=(DRW_LegacyCodePageDecoder&&) noexcept = default;

bool DRW_LegacyCodePageDecoder::supported() const noexcept { return bool(m_impl); }

std::optional<std::string> DRW_LegacyCodePageDecoder::toUtf8(std::string_view bytes)
{
    if (!m_impl) return std::nullopt;
    return m_impl->codec.toUtf8(bytes);
}

void DRW::setCustomDebugPrinter(DebugPrinter *printer)
{
  DRW_dbg::getInstance()->setCustomDebugPrinter(std::unique_ptr<DebugPrinter>(printer));
}
