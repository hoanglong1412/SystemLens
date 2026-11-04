/**
 * @file    LinuxProcFsTest.cpp
 * @brief   [TEST] Parsers of /proc and /sys text (plugins/linux/LinuxProcFs) on fixed samples.
 */
#include "LinuxProcFs.h"

#include <gtest/gtest.h>

namespace {

namespace procfs = systemlens::plugin::procfs;

constexpr const char* kProcStat =
    "cpu  1000 50 300 8000 100 20 30 5 0 0\n"
    "cpu0 500 25 150 4000 50 10 15 2 0 0\n"
    "cpu1 500 25 150 4000 50 10 15 3 0 0\n"
    "intr 123456 1 2 3\n"
    "ctxt 987654\n"
    "btime 1789373561\n"
    "processes 4321\n"
    "procs_running 2\n"
    "softirq 55555 1 2 3\n";

TEST(LinuxProcFsTest, ProcStatTotalsAndCores)
{
    const auto stat = procfs::parseProcStat(kProcStat);
    EXPECT_EQ(stat.total.user, 1050u);   // user + nice
    EXPECT_EQ(stat.total.kernel, 305u);  // system + steal
    EXPECT_EQ(stat.total.interrupt, 20u);
    EXPECT_EQ(stat.total.dpc, 30u);      // softirq
    EXPECT_EQ(stat.total.idle, 8100u);   // idle + iowait
    ASSERT_EQ(stat.perCore.size(), 2u);
    EXPECT_EQ(stat.perCore[1].kernel, 153u);
    EXPECT_EQ(stat.contextSwitches, 987654u);
    EXPECT_EQ(stat.interrupts, 123456u);
    EXPECT_EQ(stat.softirqs, 55555u);
    EXPECT_EQ(stat.bootTime, 1789373561);
}

TEST(LinuxProcFsTest, ProcStatWithoutStealColumn)
{
    const auto stat = procfs::parseProcStat("cpu 1 2 3 4 5 6 7\n");
    EXPECT_EQ(stat.total.kernel, 3u);
    EXPECT_TRUE(stat.perCore.empty());
    EXPECT_FALSE(stat.contextSwitches.has_value());
}

TEST(LinuxProcFsTest, CpuInfoX86)
{
    constexpr const char* kCpuInfo =
        "processor\t: 0\nmodel name\t: Intel(R) Core(TM) i5-8300H CPU @ 2.30GHz\nphysical id\t: 0\n"
        "core id\t\t: 0\ncpu MHz\t\t: 2000.000\nflags\t\t: fpu vme vmx sse\n\n"
        "processor\t: 1\nmodel name\t: Intel(R) Core(TM) i5-8300H CPU @ 2.30GHz\nphysical id\t: 0\n"
        "core id\t\t: 0\ncpu MHz\t\t: 3000.000\nflags\t\t: fpu vme vmx sse\n\n"
        "processor\t: 2\nmodel name\t: Intel(R) Core(TM) i5-8300H CPU @ 2.30GHz\nphysical id\t: 0\n"
        "core id\t\t: 1\ncpu MHz\t\t: 2500.000\nflags\t\t: fpu vme vmx sse\n";
    const auto info = procfs::parseCpuInfo(kCpuInfo);
    EXPECT_EQ(info.modelName, "Intel(R) Core(TM) i5-8300H CPU @ 2.30GHz");
    EXPECT_EQ(info.processorCount, 3u);
    EXPECT_EQ(info.socketCount, 1u);
    EXPECT_EQ(info.coreCount, 2u);
    ASSERT_TRUE(info.averageMhz.has_value());
    EXPECT_DOUBLE_EQ(*info.averageMhz, 2500.0);
    EXPECT_TRUE(info.hasFlags);
    EXPECT_TRUE(info.hasVirtualizationFlag);
}

TEST(LinuxProcFsTest, CpuInfoArmFallsBackToHardware)
{
    const auto info = procfs::parseCpuInfo("processor\t: 0\nBogoMIPS\t: 108.00\n\nHardware\t: BCM2835\n");
    EXPECT_EQ(info.modelName, "BCM2835");
    EXPECT_FALSE(info.hasFlags);
    EXPECT_EQ(info.socketCount, 0u);
}

TEST(LinuxProcFsTest, ProcessStatWithSpacesAndParenthesesInComm)
{
    const auto stat = procfs::parseProcessStat(
        "1234 (Web (Content) 1) S 1 1234 1234 0 -1 4194304 100 0 0 0 250 75 0 0 20 0 17 0 12345 1000 200 18446744073709551615\n");
    ASSERT_TRUE(stat.has_value());
    EXPECT_EQ(stat->pid, 1234u);
    EXPECT_EQ(stat->comm, "Web (Content) 1");
    EXPECT_EQ(stat->utime, 250u);
    EXPECT_EQ(stat->stime, 75u);
    EXPECT_EQ(stat->numThreads, 17u);
}

TEST(LinuxProcFsTest, ProcessStatMalformed)
{
    EXPECT_FALSE(procfs::parseProcessStat("garbage").has_value());
    EXPECT_FALSE(procfs::parseProcessStat("12 (short) S 1 2").has_value());
}

TEST(LinuxProcFsTest, SmallFiles)
{
    EXPECT_EQ(procfs::parseStatmResidentPages("5000 1200 300 10 0 900 0\n"), 1200u);
    EXPECT_DOUBLE_EQ(procfs::parseUptimeSeconds("3888392.57 1234.00\n").value(), 3888392.57);
    EXPECT_EQ(procfs::parseFileNrAllocated("11616\t0\t9223372036854775807\n"), 11616u);
    EXPECT_FALSE(procfs::parseUnsigned("12a").has_value());
    EXPECT_FALSE(procfs::parseUnsigned("").has_value());
}

TEST(LinuxProcFsTest, CacheSizes)
{
    EXPECT_EQ(procfs::parseCacheSizeBytes("32K\n"), 32u * 1024);
    EXPECT_EQ(procfs::parseCacheSizeBytes("8M"), 8u * 1024 * 1024);
    EXPECT_EQ(procfs::parseCacheSizeBytes("4096"), 4096u);
    EXPECT_FALSE(procfs::parseCacheSizeBytes("").has_value());
}

TEST(LinuxProcFsTest, CpuLists)
{
    EXPECT_EQ(procfs::countCpuList("0-3,8,10-11\n"), 7u);
    EXPECT_EQ(procfs::countCpuList("5"), 1u);
    EXPECT_EQ(procfs::countCpuList(""), 0u);
}

TEST(LinuxProcFsTest, ModelSpeed)
{
    EXPECT_DOUBLE_EQ(procfs::parseModelSpeedMhz("Intel(R) Core(TM) i5-8300H CPU @ 2.30GHz").value(), 2300.0);
    EXPECT_FALSE(procfs::parseModelSpeedMhz("AMD Ryzen 7 5800X 8-Core Processor").has_value());
}

TEST(LinuxProcFsTest, OsReleasePrettyName)
{
    EXPECT_EQ(procfs::parseOsReleasePrettyName("NAME=\"Ubuntu\"\nPRETTY_NAME=\"Ubuntu 24.04.1 LTS\"\nID=ubuntu\n"),
              "Ubuntu 24.04.1 LTS");
    EXPECT_EQ(procfs::parseOsReleasePrettyName("ID=alpine\n"), "");
}

} // namespace
