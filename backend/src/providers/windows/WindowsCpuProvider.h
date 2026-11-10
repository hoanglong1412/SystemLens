/**
 * @file    WindowsCpuProvider.h
 * @brief   Windows ICpuProvider: NtQuerySystemInformation for CPU times, PDH for counters, clock and temperature.
 */
#pragma once

#include "providers/ICpuProvider.h"
#include "providers/windows/PdhQuery.h"

namespace systemlens::providers::windows_os {

class WindowsCpuProvider final : public ICpuProvider
{
public:
    WindowsCpuProvider();
    [[nodiscard]] CpuStaticInfo readStaticInfo() const override;
    [[nodiscard]] CpuSample readSample() override;

private:
    PdhQuery m_query;
    std::optional<PdhQuery::Counter> m_contextSwitches;
    std::optional<PdhQuery::Counter> m_systemCalls;
    std::optional<PdhQuery::Counter> m_interrupts;
    std::optional<PdhQuery::Counter> m_dpcs;
    std::optional<PdhQuery::Counter> m_performancePercent; ///< Clock relative to the base speed.
    std::optional<PdhQuery::Counter> m_thermalZones;       ///< Kelvin, one instance per ACPI thermal zone.
    std::optional<double> m_baseSpeedMhz;
};

} // namespace systemlens::providers::windows_os
