/**
 * @file    LinuxGpuProvider.cpp
 * @brief   Implementation of LinuxGpuProvider.
 */
#include "providers/linux/LinuxGpuProvider.h"

#include "providers/linux/ProcFs.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kDrmDir = "/sys/class/drm";
constexpr std::string_view kCardPrefix = "card";
/** Locations of the PCI ID database, when the distribution installs it. */
constexpr std::array<const char*, 2> kPciIdFiles = {"/usr/share/hwdata/pci.ids", "/usr/share/misc/pci.ids"};
constexpr double kMillidegreesPerDegree = 1000.0;
constexpr double kMaxPercent = 100.0;

/** Names for the common GPU vendors, used when pci.ids is not installed. */
struct KnownVendor
{
    std::string_view id;
    std::string_view name;
};
constexpr std::array<KnownVendor, 5> kKnownVendors = {{
    {"8086", "Intel"},
    {"10de", "NVIDIA"},
    {"1002", "AMD"},
    {"1af4", "Virtio"},
    {"15ad", "VMware"},
}};

/**
 * @brief Read a sysfs PCI id file ("0x8086") as four lowercase hex digits.
 * @param path vendor or device file.
 * @return e.g. "8086", or std::nullopt when unreadable (non-PCI GPUs).
 */
std::optional<std::string> readPciId(const std::filesystem::path& path)
{
    auto text = procfs::readTrimmed(path);
    if (!text)
    {
        return std::nullopt;
    }
    if (text->starts_with("0x"))
    {
        text->erase(0, 2);
    }
    std::ranges::transform(*text, text->begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

/**
 * @brief Look a device up in pci.ids ("vvvv  Vendor" lines, each followed by "\tdddd  Device" lines).
 * @param vendorId Four hex digits.
 * @param deviceId Four hex digits.
 * @return "Vendor Device", or std::nullopt when the database or the entry is missing.
 */
std::optional<std::string> lookUpPciName(std::string_view vendorId, std::string_view deviceId)
{
    for (const char* file : kPciIdFiles)
    {
        const auto content = procfs::readFile(file);
        if (!content)
        {
            continue;
        }
        std::string_view vendorName;
        for (const std::string_view line : procfs::splitLines(*content))
        {
            if (line.empty() || line.front() == '#')
            {
                continue;
            }
            if (line.front() != '\t')
            {
                if (!vendorName.empty())
                {
                    // Reached the next vendor without finding the device.
                    return std::nullopt;
                }
                if (line.starts_with(vendorId))
                {
                    vendorName = procfs::trim(line.substr(vendorId.size()));
                }
            }
            else if (!vendorName.empty() && line.size() > 1 && line[1] != '\t'
                     && line.substr(1).starts_with(deviceId))
            {
                return std::string(vendorName) + ' ' + std::string(procfs::trim(line.substr(1 + deviceId.size())));
            }
        }
    }
    return std::nullopt;
}

/**
 * @brief Build a display model for a card.
 * @param devicePath /sys/class/drm/cardN/device.
 * @return Name from pci.ids, else "<Vendor> GPU [vvvv:dddd]", else "Graphics adapter".
 */
std::string describeModel(const std::filesystem::path& devicePath)
{
    const auto vendorId = readPciId(devicePath / "vendor");
    const auto deviceId = readPciId(devicePath / "device");
    if (!vendorId || !deviceId)
    {
        return "Graphics adapter";
    }
    if (auto name = lookUpPciName(*vendorId, *deviceId))
    {
        return std::move(*name);
    }
    const auto known = std::ranges::find(kKnownVendors, std::string_view(*vendorId), &KnownVendor::id);
    const std::string vendor = known != kKnownVendors.end() ? std::string(known->name) : "Unknown";
    return vendor + " GPU [" + *vendorId + ':' + *deviceId + ']';
}

/**
 * @brief Read the temperature of a card from its hwmon node.
 * @param devicePath /sys/class/drm/cardN/device.
 * @return Degrees Celsius, or std::nullopt when the driver exposes no sensor.
 */
std::optional<double> readTemperature(const std::filesystem::path& devicePath)
{
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(devicePath / "hwmon", error))
    {
        if (const auto millidegrees = procfs::readUnsigned(entry.path() / "temp1_input"))
        {
            return static_cast<double>(*millidegrees) / kMillidegreesPerDegree;
        }
    }
    return std::nullopt;
}

} // namespace

/** @brief Find the graphics cards (/sys/class/drm/cardN, not the cardN-<connector> entries). */
LinuxGpuProvider::LinuxGpuProvider()
{
    std::vector<std::filesystem::path> cardPaths;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(kDrmDir, error))
    {
        const std::string name = entry.path().filename().string();
        if (name.starts_with(kCardPrefix) && procfs::isAllDigits(std::string_view(name).substr(kCardPrefix.size())))
        {
            cardPaths.push_back(entry.path());
        }
    }
    std::ranges::sort(cardPaths);

    for (const auto& cardPath : cardPaths)
    {
        const std::filesystem::path devicePath = cardPath / "device";
        m_cards.push_back(Card{.devicePath = devicePath, .model = describeModel(devicePath)});
    }
}

/**
 * @brief Read utilization and temperature of every card.
 * @return One sample per card. utilizationPercent is set only for drivers exporting
 *         gpu_busy_percent (amdgpu and a few others); Intel and NVIDIA do not.
 */
std::vector<GpuSample> LinuxGpuProvider::readGpus()
{
    std::vector<GpuSample> gpus;
    gpus.reserve(m_cards.size());
    for (std::size_t index = 0; index < m_cards.size(); ++index)
    {
        const Card& card = m_cards[index];
        GpuSample sample{
            .id = "gpu" + std::to_string(index),
            .name = "GPU " + std::to_string(index),
            .model = card.model,
            .utilizationPercent = std::nullopt,
            .temperatureCelsius = readTemperature(card.devicePath),
        };
        if (const auto busy = procfs::readUnsigned(card.devicePath / "gpu_busy_percent"))
        {
            sample.utilizationPercent = std::min(static_cast<double>(*busy), kMaxPercent);
        }
        gpus.push_back(std::move(sample));
    }
    return gpus;
}

} // namespace systemlens::providers::linux_os
