/**
 * @file    FakeCpuProvider.h
 * @brief   [TEST] ICpuProvider returning scripted values: each readCounters() call advances the
 *          cumulative counters by a fixed step, so the expected percentages are known exactly.
 */
#pragma once

#include "providers/cpu/ICpuProvider.h"

#include <atomic>
#include <cstdint>
#include <stdexcept>

namespace systemlens::tests {

class FakeCpuProvider final : public providers::cpu::ICpuProvider
{
public:
    /** Logical processors reported by getStaticInfo() and readCounters(). */
    static constexpr std::uint32_t kLogical = 2;
    /** Per-read step of each time part, per core: 25 % user, 25 % kernel, 50 % idle. */
    static constexpr std::uint64_t kUserStep = 25;
    static constexpr std::uint64_t kKernelStep = 25;
    static constexpr std::uint64_t kIdleStep = 50;

    bool failStaticInfo = false;  ///< getStaticInfo() throws when true.
    std::atomic<bool> failReads{false}; ///< readCounters() throws when true.

    /**
     * @brief Static facts of a fake 1-socket, 2-core CPU.
     * @return Info.
     * @throws std::runtime_error when failStaticInfo is set.
     */
    providers::cpu::CpuStaticInfo getStaticInfo() const override
    {
        if (failStaticInfo)
        {
            throw std::runtime_error("static info failed");
        }
        providers::cpu::CpuStaticInfo info;
        info.model = "Fake CPU @ 2.00GHz";
        info.sockets = 1;
        info.cores = kLogical;
        info.logicalProcessors = kLogical;
        info.baseSpeedMhz = 2000;
        info.countersSource = "fake";
        return info;
    }

    /**
     * @brief Counters advanced by one step since the previous call.
     * @return Counters.
     * @throws std::runtime_error when failReads is set.
     */
    providers::cpu::CpuCounters readCounters() override
    {
        if (failReads)
        {
            throw std::runtime_error("read failed");
        }
        const std::uint64_t n = ++m_reads;
        providers::cpu::CpuCounters c;
        for (std::uint32_t i = 0; i < kLogical; ++i)
        {
            providers::cpu::CpuTimes t{n * kUserStep, n * kKernelStep, 0, 0, n * kIdleStep};
            c.perCore.push_back(t);
            c.total.user += t.user;
            c.total.kernel += t.kernel;
            c.total.idle += t.idle;
        }
        c.contextSwitches = n * 1000;
        c.processCount = 3;
        c.threadCount = 7;
        c.uptimeSeconds = 100 + n;
        c.processTimeUnitsPerSecond = 100;
        c.processes.push_back({42, "busy", n * 10, 1024});
        return c;
    }

    /**
     * @brief Number of readCounters() calls so far.
     * @return Count.
     */
    std::uint64_t reads() const
    {
        return m_reads;
    }

private:
    std::atomic<std::uint64_t> m_reads{0};
};

} // namespace systemlens::tests
