#include <wallet/coincontrol.h>

namespace wallet {

CCoinControl::CCoinControl() = default;

PreselectedInput& CCoinControl::Select(const COutPoint& outpoint)
{
    return m_selected[outpoint];
}

std::vector<COutPoint> CCoinControl::ListSelected() const
{
    std::vector<COutPoint> out;
    out.reserve(m_selected.size());
    for (const auto& [outpoint, input] : m_selected) {
        (void)input;
        out.push_back(outpoint);
    }
    return out;
}

} // namespace wallet
