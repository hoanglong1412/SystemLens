/**
 * @file    CpuMetricsCalculatorTest.cpp
 * @brief   [TEST] CpuMetricsCalculator: time shares, usage, rates, 32-bit wrap, reset, top processes.
 */
#include "services/cpu/CpuMetricsCalculator.h"

#include <gtest/gtest.h>

#include <limits>
#include <string>

namespace {

using systemlens::providers::cpu::CpuCounters;
using systemlens::providers::cpu::CpuTimes;
using systemlens::providers::cpu::ProcessCounters;
namespace cpu = systemlens::services::cpu;

constexpr double kTolerance = 1e-9;

/**
 * @brief Counters with one core equal to the total.
 * @param times Total times.
 * @return Counters.
 */
CpuCounters withTimes(const CpuTimes& times)
{
    CpuCounters c;
    c.total = times;
    c.perCore = {times};
    c.processTimeUnitsPerSecond = 100;
    return c;
}

TEST(CounterDeltaTest, MonotonicIncrease)
{
    EXPECT_EQ(cpu::counterDelta(100, 250), 150u);
    EXPECT_EQ(cpu::counterDelta(7, 7), 0u);
}

TEST(CounterDeltaTest, Wraps32BitCounter)
{
    constexpr std::uint64_t kMax32 = std::numeric_limits<std::uint32_t>::max();
    // 10 before the wrap + 5 after it (0..4) + the step from max to 0.
    EXPECT_EQ(cpu::counterDelta(kMax32 - 9, 5), 15u);
}

TEST(CounterDeltaTest, ResetOf64BitCounterIsUnknown)
{
    EXPECT_FALSE(cpu::counterDelta(std::uint64_t{1} << 40, 10).has_value());
}

TEST(ComputeBreakdownTest, SharesSumTo100)
{
    const CpuTimes previous{100, 100, 10, 10, 780};
    const CpuTimes current{130, 120, 15, 15, 820}; // +30 user, +20 kernel, +5 dpc, +5 irq, +40 idle
    const auto b = cpu::computeBreakdown(previous, current);
    EXPECT_NEAR(b.userPercent, 30, kTolerance);
    EXPECT_NEAR(b.kernelPercent, 20, kTolerance);
    EXPECT_NEAR(b.dpcPercent, 5, kTolerance);
    EXPECT_NEAR(b.interruptPercent, 5, kTolerance);
    EXPECT_NEAR(b.idlePercent, 40, kTolerance);
    EXPECT_NEAR(b.userPercent + b.kernelPercent + b.dpcPercent + b.interruptPercent + b.idlePercent, 100, kTolerance);
}

TEST(ComputeBreakdownTest, NoElapsedTimeIsFullyIdle)
{
    const CpuTimes same{5, 5, 0, 0, 90};
    const auto b = cpu::computeBreakdown(same, same);
    EXPECT_EQ(b.idlePercent, 100);
    EXPECT_EQ(b.userPercent, 0);
}

TEST(ComputeSnapshotTest, UsageIsHundredMinusIdle)
{
    const auto previous = withTimes({0, 0, 0, 0, 0});
    const auto current = withTimes({60, 15, 0, 0, 25});
    const auto s = cpu::computeSnapshot(previous, current, 1.0, 1);
    EXPECT_NEAR(s.usagePercent, 75, kTolerance);
    ASSERT_EQ(s.perCoreUsagePercent.size(), 1u);
    EXPECT_NEAR(s.perCoreUsagePercent[0], 75, kTolerance);
}

TEST(ComputeSnapshotTest, NewCoreWithoutBaselineReportsZero)
{
    auto previous = withTimes({0, 0, 0, 0, 0});
    auto current = withTimes({50, 0, 0, 0, 50});
    current.perCore.push_back({10, 0, 0, 0, 0});
    const auto s = cpu::computeSnapshot(previous, current, 1.0, 2);
    ASSERT_EQ(s.perCoreUsagePercent.size(), 2u);
    EXPECT_EQ(s.perCoreUsagePercent[1], 0);
}

TEST(ComputeSnapshotTest, RatesPerSecond)
{
    auto previous = withTimes({});
    auto current = withTimes({});
    previous.contextSwitches = 1000;
    current.contextSwitches = 3000;
    previous.interrupts = 10;
    current.interrupts = 30;
    // systemCalls missing on both sides (Linux), dpcs only on one side.
    current.dpcs = 5;

    const auto s = cpu::computeSnapshot(previous, current, 2.0, 1);
    ASSERT_TRUE(s.scheduler.contextSwitchesPerSec.has_value());
    EXPECT_NEAR(*s.scheduler.contextSwitchesPerSec, 1000, kTolerance);
    EXPECT_NEAR(*s.scheduler.interruptsPerSec, 10, kTolerance);
    EXPECT_FALSE(s.scheduler.systemCallsPerSec.has_value());
    EXPECT_FALSE(s.scheduler.dpcsPerSec.has_value());
}

TEST(ComputeSnapshotTest, RatesAreUnknownWithoutElapsedTime)
{
    auto previous = withTimes({});
    auto current = withTimes({});
    previous.contextSwitches = 1;
    current.contextSwitches = 2;
    EXPECT_FALSE(cpu::computeSnapshot(previous, current, 0.0, 1).scheduler.contextSwitchesPerSec.has_value());
}

TEST(ComputeSnapshotTest, CopiesInstantaneousValues)
{
    auto previous = withTimes({});
    auto current = withTimes({});
    current.speedMhz = 2610;
    current.temperatureCelsius = 62;
    current.processCount = 253;
    current.threadCount = 3278;
    current.handleCount = 114453;
    current.uptimeSeconds = 3888392;
    current.bootTimeUnixSeconds = 1789373561;

    const auto s = cpu::computeSnapshot(previous, current, 1.0, 1);
    EXPECT_EQ(s.speedMhz, 2610);
    EXPECT_EQ(s.temperatureCelsius, 62);
    EXPECT_EQ(s.processes, 253u);
    EXPECT_EQ(s.threads, 3278u);
    EXPECT_EQ(s.handles, 114453u);
    EXPECT_EQ(s.uptimeSeconds, 3888392u);
    EXPECT_EQ(s.bootTimeUnixSeconds, 1789373561);
}

TEST(ComputeSnapshotTest, ProcessPercentIsShareOfWholeMachine)
{
    auto previous = withTimes({});
    auto current = withTimes({});
    // 100 units/s, 4 logical processors, 1 s: capacity 400 units. +100 units = 25 %.
    previous.processes = {{10, "worker", 1000, 1}};
    current.processes = {{10, "worker", 1100, 1}};
    const auto s = cpu::computeSnapshot(previous, current, 1.0, 4);
    ASSERT_EQ(s.topProcesses.size(), 1u);
    EXPECT_NEAR(s.topProcesses[0].cpuPercent, 25, kTolerance);
}

TEST(ComputeSnapshotTest, TopProcessesSortedAndLimited)
{
    auto previous = withTimes({});
    auto current = withTimes({});
    for (std::uint32_t pid = 1; pid <= 15; ++pid)
    {
        previous.processes.push_back({pid, "p" + std::to_string(pid), 0, pid * 10});
        current.processes.push_back({pid, "p" + std::to_string(pid), pid, pid * 10});
    }
    const auto s = cpu::computeSnapshot(previous, current, 1.0, 1);
    ASSERT_EQ(s.topProcesses.size(), cpu::kTopProcessCount);
    EXPECT_EQ(s.topProcesses.front().pid, 15u);
    EXPECT_EQ(s.topProcesses.back().pid, 6u);
    for (std::size_t i = 1; i < s.topProcesses.size(); ++i)
    {
        EXPECT_GE(s.topProcesses[i - 1].cpuPercent, s.topProcesses[i].cpuPercent);
    }
}

TEST(ComputeSnapshotTest, TiesAreOrderedByPid)
{
    auto previous = withTimes({});
    auto current = withTimes({});
    previous.processes = {{9, "b", 0, 0}, {3, "a", 0, 0}};
    current.processes = {{9, "b", 0, 0}, {3, "a", 0, 0}};
    const auto s = cpu::computeSnapshot(previous, current, 1.0, 1);
    ASSERT_EQ(s.topProcesses.size(), 2u);
    EXPECT_EQ(s.topProcesses[0].pid, 3u);
}

TEST(ComputeSnapshotTest, ReusedPidStartsAtZero)
{
    auto previous = withTimes({});
    auto current = withTimes({});
    previous.processes = {{77, "old.exe", 0, 0}};
    current.processes = {{77, "new.exe", 500, 0}};
    const auto s = cpu::computeSnapshot(previous, current, 1.0, 1);
    ASSERT_EQ(s.topProcesses.size(), 1u);
    EXPECT_EQ(s.topProcesses[0].name, "new.exe");
    EXPECT_EQ(s.topProcesses[0].cpuPercent, 0);
}

TEST(ComputeSnapshotTest, ProcessPercentIsClamped)
{
    auto previous = withTimes({});
    auto current = withTimes({});
    previous.processes = {{1, "x", 0, 0}};
    current.processes = {{1, "x", 1000, 0}}; // 10x the capacity of 1 s on 1 CPU
    EXPECT_EQ(cpu::computeSnapshot(previous, current, 1.0, 1).topProcesses[0].cpuPercent, 100);
}

} // namespace
