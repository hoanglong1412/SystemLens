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

/** Value of ReceiveLinkSpeed when the speed is not known. */
constexpr ULONG64 kUnknownLinkSpeed = ~ULONG64{0};

/** Frees the table returned by GetIfTable2. */
struct MibTableDeleter
{
    void operator()(MIB_IF_TABLE2* table) const noexcept { ::FreeMibTable(table); }
};

/**
 * @brief Format the hardware address of an interface.
 * @param row Interface row.
 * @return "AA:BB:CC:DD:EE:FF"; empty when the interface has no hardware address.
 */
std::string formatMacAddress(const MIB_IF_ROW2& row)
{
    constexpr char kHexDigits[] = "0123456789ABCDEF";
    std::string text;
    for (ULONG index = 0; index < row.PhysicalAddressLength && index < IF_MAX_PHYS_ADDRESS_LENGTH; ++index)
    {
        if (index != 0)
        {
            text += ':';
        }
        text += kHexDigits[row.PhysicalAddress[index] >> 4];
        text += kHexDigits[row.PhysicalAddress[index] & 0x0F];
    }
    return text;
}

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
        NetworkSample sample{
            .id = "net-" + std::to_string(row.InterfaceIndex),
            .name = toUtf8(row.Alias),
            .kind = isWifi ? NetworkKind::Wifi : NetworkKind::Ethernet,
            .description = toUtf8(row.Description),
            .macAddress = formatMacAddress(row),
            .linkSpeedBitsPerSecond = std::nullopt,
            .bytesSent = row.OutOctets,
            .bytesReceived = row.InOctets,
        };
        // The link speed is "unknown" (all bits set) for some virtual adapters.
        if (row.ReceiveLinkSpeed != 0 && row.ReceiveLinkSpeed != kUnknownLinkSpeed)
        {
            sample.linkSpeedBitsPerSecond = static_cast<double>(row.ReceiveLinkSpeed);
        }
        interfaces.push_back(std::move(sample));
    }
    return interfaces;
}

} // namespace systemlens::providers::windows_os
