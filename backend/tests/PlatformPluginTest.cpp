/**
 * @file    PlatformPluginTest.cpp
 * @brief   [TEST] Loads the platform plugin built for this OS (ABI 2) and checks that it provides
 *          the CPU and system info providers with plausible live values.
 *          SYSTEMLENS_TEST_PLUGIN_PATH is set by tests/CMakeLists.txt.
 */
#include "platform/PlatformPlugin.h"

#include <gtest/gtest.h>

namespace {

using systemlens::platform::PlatformPlugin;

TEST(PlatformPluginTest, LoadsAndCreatesCpuProvider)
{
    PlatformPlugin plugin(SYSTEMLENS_TEST_PLUGIN_PATH);
    auto cpu = plugin.createCpuProvider();
#if defined(__APPLE__)
    EXPECT_EQ(cpu, nullptr); // no macOS CPU provider yet
#else
    ASSERT_NE(cpu, nullptr);
    const auto info = cpu->getStaticInfo();
    EXPECT_FALSE(info.model.empty());
    EXPECT_GE(info.logicalProcessors, info.cores);
    EXPECT_GE(info.cores, info.sockets);
    EXPECT_GE(info.sockets, 1u);

    const auto counters = cpu->readCounters();
    EXPECT_FALSE(counters.perCore.empty());
    EXPECT_GT(counters.processCount, 0u);
    EXPECT_GT(counters.threadCount, 0u);
    EXPECT_GT(counters.uptimeSeconds, 0u);
#endif
}

TEST(PlatformPluginTest, CreatesSystemInfoProvider)
{
    PlatformPlugin plugin(SYSTEMLENS_TEST_PLUGIN_PATH);
    const auto system = plugin.createSystemInfoProvider();
#if defined(__APPLE__)
    EXPECT_EQ(system, nullptr);
#else
    ASSERT_NE(system, nullptr);
    const auto identity = system->getIdentity();
    EXPECT_FALSE(identity.hostName.empty());
    EXPECT_FALSE(identity.osName.empty());
#if defined(_WIN32)
    EXPECT_EQ(identity.platform, "windows");
#else
    EXPECT_EQ(identity.platform, "linux");
#endif
#endif
}

TEST(PlatformPluginTest, MissingLibraryThrows)
{
    EXPECT_THROW(PlatformPlugin("/nonexistent/libsystemlens_platform.so"), std::runtime_error);
}

} // namespace
