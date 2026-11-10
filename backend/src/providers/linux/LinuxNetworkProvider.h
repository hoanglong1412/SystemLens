/**
 * @file    LinuxNetworkProvider.h
 * @brief   Linux INetworkProvider: /sys/class/net.
 */
#pragma once

#include "providers/INetworkProvider.h"

namespace systemlens::providers::linux_os {

class LinuxNetworkProvider final : public INetworkProvider
{
public:
    [[nodiscard]] std::vector<NetworkSample> readInterfaces() override;
};

} // namespace systemlens::providers::linux_os
