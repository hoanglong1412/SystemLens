/**
 * @file    LinuxCpuProvider.h
 * @brief   ICpuProvider of the Linux platform plugin: reads /proc and /sys (sources: design D2).
 *
 * The root directories are constructor parameters so tests can point the provider at a fake tree;
 * production uses "/proc" and "/sys".
 */
#pragma once

#include "cpu/ICpuProvider.h"

#include <filesystem>
#include <optional>

namespace systemlens::plugin {

class LinuxCpuProvider final : public providers::cpu::ICpuProvider
{
public:
    explicit LinuxCpuProvider(std::filesystem::path procRoot = "/proc", std::filesystem::path sysRoot = "/sys");

    providers::cpu::CpuStaticInfo getStaticInfo() const override;
    providers::cpu::CpuCounters readCounters() override;

private:
    std::optional<double> readCurrentSpeedMhz() const;
    std::optional<double> readTemperatureCelsius();
    std::optional<std::filesystem::path> findTemperatureSensor() const;
    void readProcesses(providers::cpu::CpuCounters& counters) const;

    std::filesystem::path m_procRoot;
    std::filesystem::path m_sysRoot;
    long m_clockTicksPerSecond;
    long m_pageSize;
    bool m_sensorSearched = false;                   ///< findTemperatureSensor() runs once.
    std::optional<std::filesystem::path> m_sensor;   ///< File in millidegrees Celsius.
};

} // namespace systemlens::plugin
