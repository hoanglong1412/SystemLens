/**
 * @file    WindowsNetworkProvider.h
 * @brief   Windows INetworkProvider: GetIfTable2 (IP Helper).
 */
#pragma once

#include "providers/INetworkProvider.h"

namespace systemlens::providers::windows_os {

class WindowsNetworkProvider final : public INetworkProvider
{
public:
    [[nodiscard]] std::vector<NetworkSample> readInterfaces() override;
};

} // namespace systemlens::providers::windows_os
