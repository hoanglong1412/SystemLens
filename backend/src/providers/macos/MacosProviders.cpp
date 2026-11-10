/**
 * @file    MacosProviders.cpp
 * @brief   macOS placeholders: every provider interface is implemented by a class whose
 *          methods throw NotImplementedError, plus the macOS definition of createPlatformProviders().
 *
 * The backend therefore builds and starts on macOS, and the metrics endpoints answer
 * 500 "... is not implemented on this platform" until the classes below are filled in.
 * When implementing one, move it to its own Macos<Name>Provider.h/.cpp like the other platforms
 * (hints: sysctl, host_statistics64, IOKit, libproc).
 */
#include "providers/PlatformProviders.h"

#include "providers/ProviderError.h"

namespace systemlens::providers {

namespace macos {

/** Placeholder ISystemInfoProvider. */
class MacosSystemInfoProvider final : public ISystemInfoProvider
{
public:
    /** @throws NotImplementedError always. */
    [[nodiscard]] SystemIdentity readIdentity() const override
    {
        throw NotImplementedError("MacosSystemInfoProvider::readIdentity");
    }

    /** @throws NotImplementedError always. */
    [[nodiscard]] std::chrono::system_clock::time_point readBootTime() const override
    {
        throw NotImplementedError("MacosSystemInfoProvider::readBootTime");
    }

    /** @throws NotImplementedError always. */
    [[nodiscard]] SystemCounts readCounts() const override
    {
        throw NotImplementedError("MacosSystemInfoProvider::readCounts");
    }
};

/** Placeholder ICpuProvider. */
class MacosCpuProvider final : public ICpuProvider
{
public:
    /** @throws NotImplementedError always. */
    [[nodiscard]] CpuStaticInfo readStaticInfo() const override
    {
        throw NotImplementedError("MacosCpuProvider::readStaticInfo");
    }

    /** @throws NotImplementedError always. */
    [[nodiscard]] CpuSample readSample() override
    {
        throw NotImplementedError("MacosCpuProvider::readSample");
    }
};

/** Placeholder IMemoryProvider. */
class MacosMemoryProvider final : public IMemoryProvider
{
public:
    /** @throws NotImplementedError always. */
    [[nodiscard]] MemoryUsage readUsage() const override
    {
        throw NotImplementedError("MacosMemoryProvider::readUsage");
    }
};

/** Placeholder IDiskProvider. */
class MacosDiskProvider final : public IDiskProvider
{
public:
    /** @throws NotImplementedError always. */
    [[nodiscard]] std::vector<DiskSample> readDisks() override
    {
        throw NotImplementedError("MacosDiskProvider::readDisks");
    }
};

/** Placeholder INetworkProvider. */
class MacosNetworkProvider final : public INetworkProvider
{
public:
    /** @throws NotImplementedError always. */
    [[nodiscard]] std::vector<NetworkSample> readInterfaces() override
    {
        throw NotImplementedError("MacosNetworkProvider::readInterfaces");
    }
};

/** Placeholder IGpuProvider. */
class MacosGpuProvider final : public IGpuProvider
{
public:
    /** @throws NotImplementedError always. */
    [[nodiscard]] std::vector<GpuSample> readGpus() override
    {
        throw NotImplementedError("MacosGpuProvider::readGpus");
    }
};

/** Placeholder IProcessProvider. */
class MacosProcessProvider final : public IProcessProvider
{
public:
    /** @throws NotImplementedError always. */
    [[nodiscard]] std::vector<ProcessSample> readProcesses() override
    {
        throw NotImplementedError("MacosProcessProvider::readProcesses");
    }

    /** @throws NotImplementedError always. */
    [[nodiscard]] std::uint32_t currentProcessId() const override
    {
        throw NotImplementedError("MacosProcessProvider::currentProcessId");
    }
};

} // namespace macos

/**
 * @brief Create the macOS placeholder providers.
 * @return One provider per domain; every method throws NotImplementedError.
 */
PlatformProviders createPlatformProviders()
{
    return PlatformProviders{
        .systemInfo = std::make_shared<macos::MacosSystemInfoProvider>(),
        .cpu = std::make_shared<macos::MacosCpuProvider>(),
        .memory = std::make_shared<macos::MacosMemoryProvider>(),
        .disk = std::make_shared<macos::MacosDiskProvider>(),
        .network = std::make_shared<macos::MacosNetworkProvider>(),
        .gpu = std::make_shared<macos::MacosGpuProvider>(),
        .process = std::make_shared<macos::MacosProcessProvider>(),
    };
}

} // namespace systemlens::providers
