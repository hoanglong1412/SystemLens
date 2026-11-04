/**
 * @file    LinuxCpuProviderTest.cpp
 * @brief   [TEST] LinuxCpuProvider against the live /proc and /sys of the build machine, and against
 *          a minimal fake /proc tree written to a temporary directory.
 */
#include "LinuxCpuProvider.h"
#include "LinuxSystemInfoProvider.h"

#include <gtest/gtest.h>

#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {

namespace fs = std::filesystem;
using systemlens::plugin::LinuxCpuProvider;
using systemlens::plugin::LinuxSystemInfoProvider;

/**
 * @brief Write a text file, creating its directories.
 * @param path File.
 * @param content Content.
 */
void writeFile(const fs::path& path, const std::string& content)
{
    fs::create_directories(path.parent_path());
    std::ofstream(path) << content;
}

/** Temporary /proc + /sys tree removed at the end of the test. */
class FakeRootTest : public ::testing::Test
{
protected:
    /** @brief Create a unique empty directory for this test. */
    void SetUp() override
    {
        m_root = fs::temp_directory_path() / ("systemlens-test-" + std::to_string(getpid()));
        fs::remove_all(m_root);
        fs::create_directories(m_root / "proc");
        fs::create_directories(m_root / "sys");
    }

    /** @brief Remove the directory. */
    void TearDown() override
    {
        fs::remove_all(m_root);
    }

    fs::path m_root;
};

TEST(LinuxCpuProviderLiveTest, StaticInfoIsConsistent)
{
    const auto info = LinuxCpuProvider().getStaticInfo();
    EXPECT_FALSE(info.model.empty());
    EXPECT_GE(info.logicalProcessors, 1u);
    EXPECT_GE(info.logicalProcessors, info.cores);
    EXPECT_GE(info.cores, info.sockets);
    EXPECT_EQ(info.logicalProcessors, std::thread::hardware_concurrency());
}

TEST(LinuxCpuProviderLiveTest, CountersIncrease)
{
    LinuxCpuProvider provider;
    const auto first = provider.readCounters();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const auto second = provider.readCounters();

    const auto total = [](const auto& t) { return t.user + t.kernel + t.dpc + t.interrupt + t.idle; };
    EXPECT_GE(total(second.total), total(first.total));
    EXPECT_EQ(second.perCore.size(), std::thread::hardware_concurrency());
    EXPECT_TRUE(second.contextSwitches.has_value());
    EXPECT_FALSE(second.systemCalls.has_value()); // not exposed by Linux
    EXPECT_GT(second.processCount, 0u);
    EXPECT_GT(second.uptimeSeconds, 0u);
    EXPECT_GT(second.bootTimeUnixSeconds, 0);
    EXPECT_EQ(second.processTimeUnitsPerSecond, static_cast<std::uint64_t>(sysconf(_SC_CLK_TCK)));

    // This test process must be in the list.
    bool foundSelf = false;
    for (const auto& p : second.processes)
    {
        foundSelf = foundSelf || p.pid == static_cast<std::uint32_t>(getpid());
    }
    EXPECT_TRUE(foundSelf);
}

TEST_F(FakeRootTest, ReadsFakeTree)
{
    writeFile(m_root / "proc/stat", "cpu 10 0 10 80 0 0 0 0\ncpu0 10 0 10 80 0 0 0 0\nctxt 5\nbtime 100\n");
    writeFile(m_root / "proc/cpuinfo", "processor\t: 0\nmodel name\t: Fake @ 1.50GHz\nflags\t: fpu\n");
    writeFile(m_root / "proc/uptime", "42.5 10.0\n");
    writeFile(m_root / "proc/sys/fs/file-nr", "77 0 100\n");
    writeFile(m_root / "proc/123/stat", "123 (fake app) R 1 1 1 0 -1 0 0 0 0 0 30 20 0 0 20 0 4 0 1 1 1\n");
    writeFile(m_root / "proc/123/statm", "10 3 0 0 0 0 0\n");
    writeFile(m_root / "sys/class/hwmon/hwmon0/name", "coretemp\n");
    writeFile(m_root / "sys/class/hwmon/hwmon0/temp1_input", "61500\n");
    writeFile(m_root / "sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "2400000\n");
    writeFile(m_root / "sys/devices/system/cpu/cpu0/cache/index0/level", "1\n");
    writeFile(m_root / "sys/devices/system/cpu/cpu0/cache/index0/size", "32K\n");
    writeFile(m_root / "sys/devices/system/cpu/cpu0/cache/index0/type", "Data\n");
    writeFile(m_root / "sys/devices/system/cpu/cpu0/cache/index0/shared_cpu_list", "0\n");

    LinuxCpuProvider provider(m_root / "proc", m_root / "sys");
    const auto info = provider.getStaticInfo();
    EXPECT_EQ(info.model, "Fake @ 1.50GHz");
    EXPECT_EQ(info.logicalProcessors, 1u);
    EXPECT_EQ(info.baseSpeedMhz, 1500.0);
    EXPECT_EQ(info.virtualization, systemlens::providers::cpu::Virtualization::Disabled);
    EXPECT_EQ(info.l1CacheBytes, 32u * 1024);
    EXPECT_FALSE(info.l3CacheBytes.has_value());

    const auto c = provider.readCounters();
    EXPECT_EQ(c.contextSwitches, 5u);
    EXPECT_EQ(c.bootTimeUnixSeconds, 100);
    EXPECT_EQ(c.uptimeSeconds, 42u);
    EXPECT_EQ(c.handleCount, 77u);
    EXPECT_EQ(c.speedMhz, 2400.0);
    EXPECT_EQ(c.temperatureCelsius, 61.5);
    ASSERT_EQ(c.processes.size(), 1u);
    EXPECT_EQ(c.processes[0].name, "fake app");
    EXPECT_EQ(c.processes[0].cpuTime, 50u);
    EXPECT_EQ(c.processes[0].memoryBytes, 3u * static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE)));
    EXPECT_EQ(c.threadCount, 4u);
}

TEST_F(FakeRootTest, MissingProcStatThrows)
{
    LinuxCpuProvider provider(m_root / "proc", m_root / "sys");
    EXPECT_THROW(provider.getStaticInfo(), std::runtime_error);
    EXPECT_THROW(provider.readCounters(), std::runtime_error);
}

TEST(LinuxSystemInfoProviderTest, IdentityOfThisMachine)
{
    const auto identity = LinuxSystemInfoProvider().getIdentity();
    EXPECT_EQ(identity.platform, "linux");
    EXPECT_FALSE(identity.hostName.empty());
    EXPECT_FALSE(identity.osName.empty());
    EXPECT_EQ(identity.osVersion.rfind("Kernel ", 0), 0u);
}

} // namespace
