/**
 * @file    WindowsNetworkProvider.cpp
 * @brief   Implementation of WindowsNetworkProvider.
 */
#include "providers/windows/WindowsNetworkProvider.h"

#include "providers/ProviderError.h"
#include "providers/windows/WindowsUtil.h"

// winsock2.h must precede the IP Helper headers (they need the socket address types).
#include <winsock2.h>

#include <ws2ipdef.h>

#include <iphlpapi.h>
#include <netioapi.h>

#include <memory>

namespace systemlens::providers::windows_os {

namespace {

/** Frees the table returned by GetIfTable2. */
struct MibTableDeleter
{
    void operator()(MIB_IF_TABLE2* table) const noexcept { ::FreeMibTable(table); }
};

} // namespace

/**
 * @brief Enumerate connected Ethernet and Wi-Fi adapters with their byte counters.
 *
 * GetIfTable2 also lists one "filter" pseudo-interface per network driver layer; those
 * duplicate the counters of the real adapter and are skipped.
 * @return Adapters in interface-index order; empty when none is up.
 * @throws ProviderError if GetIfTable2 fails.
 */
std::vector<NetworkSample> WindowsNetworkProvider::readInterfaces()
{
    MIB_IF_TABLE2* rawTable = nullptr;
    if (::GetIfTable2(&rawTable) != NO_ERROR || rawTable == nullptr)
    {
        throw ProviderError("GetIfTable2 failed");
    }
    const std::unique_ptr<MIB_IF_TABLE2, MibTableDeleter> table(rawTable);

    std::vector<NetworkSample> interfaces;
    for (ULONG index = 0; index < table->NumEntries; ++index)
    {
        const MIB_IF_ROW2& row = table->Table[index];
        const bool isWifi = row.Type == IF_TYPE_IEEE80211;
        if ((!isWifi && row.Type != IF_TYPE_ETHERNET_CSMACD) || row.InterfaceAndOperStatusFlags.FilterInterface
            || row.OperStatus != IfOperStatusUp)
        {
            continue;
        }
        interfaces.push_back(NetworkSample{
            .id = "net-" + std::to_string(row.InterfaceIndex),
            .name = toUtf8(row.Alias),
            .kind = isWifi ? NetworkKind::Wifi : NetworkKind::Ethernet,
            .bytesSent = row.OutOctets,
            .bytesReceived = row.InOctets,
        });
    }
    return interfaces;
}

} // namespace systemlens::providers::windows_os
