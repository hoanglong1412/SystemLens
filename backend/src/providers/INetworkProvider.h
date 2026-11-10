/**
 * @file    INetworkProvider.h
 * @brief   Provider interface for network adapters and their cumulative byte counters.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace systemlens::providers {

/** Link technology of an adapter. */
enum class NetworkKind
{
    Ethernet,
    Wifi,
};

/** One network adapter at one instant. */
struct NetworkSample
{
    std::string id;   ///< Stable identifier, e.g. "net-eth0".
    std::string name; ///< Adapter name as the OS shows it, e.g. "vEthernet (WSL)", "eth0".
    NetworkKind kind = NetworkKind::Ethernet;
    std::uint64_t bytesSent = 0;     ///< Cumulative.
    std::uint64_t bytesReceived = 0; ///< Cumulative.
};

class INetworkProvider
{
public:
    virtual ~INetworkProvider() = default;

    /**
     * @brief Enumerate Ethernet and Wi-Fi adapters (no loopback) with their counters.
     * @return Adapters in display order; empty when none is visible.
     * @throws ProviderError if the adapters cannot be enumerated.
     */
    [[nodiscard]] virtual std::vector<NetworkSample> readInterfaces() = 0;
};

} // namespace systemlens::providers
