/**
 * @file    LinuxNetworkProvider.cpp
 * @brief   Implementation of LinuxNetworkProvider.
 */
#include "providers/linux/LinuxNetworkProvider.h"

#include "providers/linux/ProcFs.h"

#include <algorithm>
#include <filesystem>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kNetDir = "/sys/class/net";
/** ARPHRD_ETHER: Ethernet-framed interfaces (wired and Wi-Fi); loopback is 772. */
constexpr std::uint64_t kArpHardwareEthernet = 1;
constexpr double kBitsPerMegabit = 1'000'000.0;

/** An interface found in /sys/class/net. */
struct Candidate
{
    std::string name;
    bool hasDevice = false; ///< Backed by a (real or paravirtual) device, unlike bridges and veth pairs.
};

/**
 * @brief List the Ethernet-framed interfaces.
 * @return Candidates sorted by name; empty when /sys/class/net is unreadable.
 */
std::vector<Candidate> listCandidates()
{
    std::vector<Candidate> candidates;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(kNetDir, error))
    {
        if (procfs::readUnsigned(entry.path() / "type") == kArpHardwareEthernet)
        {
            candidates.push_back(Candidate{
                .name = entry.path().filename().string(),
                .hasDevice = std::filesystem::exists(entry.path() / "device", error),
            });
        }
    }
    std::ranges::sort(candidates, {}, &Candidate::name);
    return candidates;
}

/**
 * @brief Name the kernel driver of an interface (target of the device/driver symlink).
 * @param interfacePath /sys/class/net/<name>.
 * @return Driver name such as "virtio_net" or "e1000e"; empty when the interface has no device.
 */
std::string readDriverName(const std::filesystem::path& interfacePath)
{
    std::error_code error;
    const auto driver = std::filesystem::read_symlink(interfacePath / "device" / "driver", error);
    return error ? std::string() : driver.filename().string();
}

} // namespace

/**
 * @brief Enumerate network adapters with their byte counters.
 *
 * Only device-backed interfaces are returned, so container hosts do not list every bridge
 * and veth pair. When there is none (e.g. inside a container) all Ethernet-framed interfaces are used.
 * @return Adapters sorted by name; empty when none exists.
 */
std::vector<NetworkSample> LinuxNetworkProvider::readInterfaces()
{
    std::vector<Candidate> candidates = listCandidates();
    if (std::ranges::any_of(candidates, &Candidate::hasDevice))
    {
        std::erase_if(candidates, [](const Candidate& candidate) { return !candidate.hasDevice; });
    }

    std::vector<NetworkSample> interfaces;
    interfaces.reserve(candidates.size());
    for (const Candidate& candidate : candidates)
    {
        const std::filesystem::path path = std::filesystem::path(kNetDir) / candidate.name;
        std::error_code error;
        const bool isWifi = std::filesystem::exists(path / "wireless", error)
                            || std::filesystem::exists(path / "phy80211", error);
        NetworkSample sample{
            .id = "net-" + candidate.name,
            .name = candidate.name,
            .kind = isWifi ? NetworkKind::Wifi : NetworkKind::Ethernet,
            .description = readDriverName(path),
            .macAddress = procfs::readTrimmed(path / "address").value_or(""),
            .linkSpeedBitsPerSecond = std::nullopt,
            .bytesSent = procfs::readUnsigned(path / "statistics" / "tx_bytes").value_or(0),
            .bytesReceived = procfs::readUnsigned(path / "statistics" / "rx_bytes").value_or(0),
        };
        // "speed" holds megabits per second, or -1 / is unreadable when the link has no speed (Wi-Fi, virtual).
        if (const auto megabits = procfs::readUnsigned(path / "speed"))
        {
            sample.linkSpeedBitsPerSecond = static_cast<double>(*megabits) * kBitsPerMegabit;
        }
        interfaces.push_back(std::move(sample));
    }
    return interfaces;
}

} // namespace systemlens::providers::linux_os
