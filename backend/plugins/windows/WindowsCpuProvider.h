/**
 * @file    WindowsCpuProvider.h
 * @brief   ICpuProvider of the Windows platform plugin (sources: design D2):
 *          NtQuerySystemInformation for CPU times and processes, PDH for scheduler counters,
 *          clock and temperature, registry and GetLogicalProcessorInformationEx for static facts.
 *
 * Limitation: only processor group 0 is read (at most 64 logical processors).
 */
#pragma once

#include "cpu/ICpuProvider.h"

#include <memory>
#include <optional>

namespace systemlens::plugin {

class WindowsCpuProvider final : public providers::cpu::ICpuProvider
{
public:
    WindowsCpuProvider();
    ~WindowsCpuProvider() override;

    WindowsCpuProvider(const WindowsCpuProvider&) = delete;
    WindowsCpuProvider& operator=(const WindowsCpuProvider&) = delete;

    providers::cpu::CpuStaticInfo getStaticInfo() const override;
    providers::cpu::CpuCounters readCounters() override;

private:
    struct PdhCounters; ///< PDH query and counter handles (keeps <pdh.h> out of this header).

    void readProcessorTimes(providers::cpu::CpuCounters& counters) const;
    void readProcesses(providers::cpu::CpuCounters& counters) const;
    void readPdh(providers::cpu::CpuCounters& counters);

    std::unique_ptr<PdhCounters> m_pdh;
    std::optional<double> m_baseSpeedMhz; ///< Registry ~MHz, scale of "% Processor Performance".
};

} // namespace systemlens::plugin
