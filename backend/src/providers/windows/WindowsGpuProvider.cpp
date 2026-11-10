/**
 * @file    WindowsGpuProvider.cpp
 * @brief   Implementation of WindowsGpuProvider.
 */
#include "providers/windows/WindowsGpuProvider.h"

#include "providers/windows/WindowsUtil.h"

#include <dxgi.h>

#include <algorithm>
#include <cwchar>
#include <map>
#include <memory>
#include <string_view>

namespace systemlens::providers::windows_os {

namespace {

constexpr double kMaxPercent = 100.0;
constexpr int kHexBase = 16;
constexpr unsigned kLuidHighShift = 32;
/**
 * Instance names of the "GPU Engine" counter look like
 * "pid_1234_luid_0x00000000_0x0000D6A4_phys_0_eng_3_engtype_3D".
 */
constexpr std::wstring_view kLuidMarker = L"_luid_0x";
constexpr std::wstring_view kLuidLowMarker = L"_0x";
constexpr std::wstring_view kEngineMarker = L"_phys_";

/** Releases a COM interface; deleter of ComPointer. */
struct ComReleaser
{
    void operator()(IUnknown* object) const noexcept { object->Release(); }
};
template <typename T>
using ComPointer = std::unique_ptr<T, ComReleaser>;

/** Adapter and engine named by one counter instance. */
struct EngineInstance
{
    std::uint64_t luid = 0;
    std::wstring engine; ///< "_phys_0_eng_3_engtype_3D": one engine of one adapter.
};

/**
 * @brief Combine the two halves of a LUID into one number.
 * @param high HighPart.
 * @param low  LowPart.
 * @return 64-bit value used as the adapter key.
 */
std::uint64_t makeLuid(std::uint32_t high, std::uint32_t low)
{
    return (static_cast<std::uint64_t>(high) << kLuidHighShift) | low;
}

/**
 * @brief Parse a "GPU Engine" instance name.
 * @param name Instance name.
 * @return Adapter LUID and engine key, or std::nullopt when the name has another shape.
 */
std::optional<EngineInstance> parseEngineInstance(const std::wstring& name)
{
    const auto luidAt = name.find(kLuidMarker);
    if (luidAt == std::wstring::npos)
    {
        return std::nullopt;
    }
    const auto highAt = luidAt + kLuidMarker.size();
    const auto lowMarkerAt = name.find(kLuidLowMarker, highAt);
    const auto engineAt = name.find(kEngineMarker, highAt);
    if (lowMarkerAt == std::wstring::npos || engineAt == std::wstring::npos || lowMarkerAt > engineAt)
    {
        return std::nullopt;
    }
    const auto high = std::wcstoul(name.c_str() + highAt, nullptr, kHexBase);
    const auto low = std::wcstoul(name.c_str() + lowMarkerAt + kLuidLowMarker.size(), nullptr, kHexBase);
    return EngineInstance{
        .luid = makeLuid(static_cast<std::uint32_t>(high), static_cast<std::uint32_t>(low)),
        .engine = name.substr(engineAt),
    };
}

} // namespace

/** @brief List the hardware adapters through DXGI and register the utilization counter. */
WindowsGpuProvider::WindowsGpuProvider()
    : m_engineUtilization(m_query.addCounter(L"\\GPU Engine(*)\\Utilization Percentage"))
{
    IDXGIFactory1* rawFactory = nullptr;
    if (FAILED(::CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&rawFactory))))
    {
        return;
    }
    const ComPointer<IDXGIFactory1> factory(rawFactory);

    IDXGIAdapter1* rawAdapter = nullptr;
    for (UINT index = 0; factory->EnumAdapters1(index, &rawAdapter) != DXGI_ERROR_NOT_FOUND; ++index)
    {
        const ComPointer<IDXGIAdapter1> adapter(rawAdapter);
        DXGI_ADAPTER_DESC1 description{};
        // The software rasterizer ("Microsoft Basic Render Driver") is not a GPU.
        if (FAILED(adapter->GetDesc1(&description)) || (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
        {
            continue;
        }
        m_adapters.push_back(Adapter{
            .model = toUtf8(description.Description),
            .luid = makeLuid(static_cast<std::uint32_t>(description.AdapterLuid.HighPart),
                             description.AdapterLuid.LowPart),
        });
    }
}

/**
 * @brief Read the utilization of every adapter.
 *
 * Each counter instance is one process on one engine (3D, video decode, copy...). As in
 * Task Manager, an adapter's utilization is that of its busiest engine, where an engine's
 * load is the sum over all processes.
 * @return One sample per adapter; utilizationPercent is empty on the first call and when the
 *         counter does not exist (Windows versions before 10 1709). Temperature is never set.
 */
std::vector<GpuSample> WindowsGpuProvider::readGpus()
{
    std::map<std::uint64_t, std::map<std::wstring, double>> engineLoads;
    const bool hasData = m_engineUtilization.has_value() && m_query.collect();
    if (hasData)
    {
        for (const auto& [name, percent] : m_query.formattedInstances(m_engineUtilization))
        {
            if (const auto instance = parseEngineInstance(name))
            {
                engineLoads[instance->luid][instance->engine] += percent;
            }
        }
    }

    std::vector<GpuSample> gpus;
    gpus.reserve(m_adapters.size());
    for (std::size_t index = 0; index < m_adapters.size(); ++index)
    {
        GpuSample sample{
            .id = "gpu" + std::to_string(index),
            .name = "GPU " + std::to_string(index),
            .model = m_adapters[index].model,
            .utilizationPercent = std::nullopt,
            .temperatureCelsius = std::nullopt,
        };
        if (hasData)
        {
            double busiest = 0.0;
            if (const auto loads = engineLoads.find(m_adapters[index].luid); loads != engineLoads.end())
            {
                for (const auto& [engine, percent] : loads->second)
                {
                    busiest = std::max(busiest, percent);
                }
            }
            sample.utilizationPercent = std::min(busiest, kMaxPercent);
        }
        gpus.push_back(std::move(sample));
    }
    return gpus;
}

} // namespace systemlens::providers::windows_os
