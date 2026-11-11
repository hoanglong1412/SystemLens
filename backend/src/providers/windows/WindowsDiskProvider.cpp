/**
 * @file    WindowsDiskProvider.cpp
 * @brief   Implementation of WindowsDiskProvider.
 */
#include "providers/windows/WindowsDiskProvider.h"

#include "providers/windows/WindowsUtil.h"

#include <windows.h>

#include <winioctl.h>

#include <array>
#include <cstring>
#include <map>
#include <string_view>

namespace systemlens::providers::windows_os {

namespace {

/** Physical drive numbers are small and dense; gaps appear only after hot-unplugging. */
constexpr DWORD kMaxPhysicalDrives = 32;
constexpr int kDriveLetterCount = 26;
/** Room for the storage descriptor and the vendor / product / serial strings that follow it. */
constexpr std::size_t kDescriptorBufferBytes = 1024;

/**
 * @brief Send a query IOCTL that takes an optional input and returns one fixed-size structure.
 * @param device  Device handle.
 * @param code    IOCTL code.
 * @param input   Input structure, or nullptr.
 * @param inputBytes Size of input.
 * @param output  Receives the result.
 * @return True when the device answered with at least sizeof(Output) bytes.
 */
template <typename Output>
bool queryDevice(HANDLE device, DWORD code, void* input, DWORD inputBytes, Output& output)
{
    DWORD returned = 0;
    return ::DeviceIoControl(device, code, input, inputBytes, &output, sizeof(output), &returned, nullptr)
           && returned >= sizeof(output);
}

/**
 * @brief Map each physical drive number to the drive letters of its volumes.
 * @return e.g. {0: "C: E:", 1: "D:"}. Only fixed drives are probed, so empty card readers and
 *         network drives are not touched.
 */
std::map<DWORD, std::string> readDriveLetters()
{
    std::map<DWORD, std::string> letters;
    const DWORD present = ::GetLogicalDrives();
    for (int index = 0; index < kDriveLetterCount; ++index)
    {
        if ((present & (1u << index)) == 0)
        {
            continue;
        }
        const wchar_t letter = static_cast<wchar_t>(L'A' + index);
        const std::wstring root = {letter, L':', L'\\'};
        if (::GetDriveTypeW(root.c_str()) != DRIVE_FIXED)
        {
            continue;
        }
        const UniqueHandle volume = openForQuery(std::wstring(L"\\\\.\\") + letter + L':');
        STORAGE_DEVICE_NUMBER number{};
        if (volume && queryDevice(volume.get(), IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0, number)
            && number.DeviceType == FILE_DEVICE_DISK)
        {
            std::string& text = letters[number.DeviceNumber];
            if (!text.empty())
            {
                text += ' ';
            }
            text += static_cast<char>('A' + index);
            text += ':';
        }
    }
    return letters;
}

/**
 * @brief Tell SSD from HDD.
 * @param disk Physical drive handle.
 * @return Hdd when the drive reports a seek penalty, Ssd when it reports none or supports TRIM,
 *         Unknown when the driver answers neither query.
 */
DiskKind readKind(HANDLE disk)
{
    STORAGE_PROPERTY_QUERY query{};
    query.QueryType = PropertyStandardQuery;

    query.PropertyId = StorageDeviceSeekPenaltyProperty;
    DEVICE_SEEK_PENALTY_DESCRIPTOR seekPenalty{};
    if (queryDevice(disk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), seekPenalty))
    {
        return seekPenalty.IncursSeekPenalty ? DiskKind::Hdd : DiskKind::Ssd;
    }

    // Some NVMe drivers do not implement the seek penalty property; TRIM support implies flash.
    query.PropertyId = StorageDeviceTrimProperty;
    DEVICE_TRIM_DESCRIPTOR trim{};
    if (queryDevice(disk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), trim) && trim.TrimEnabled)
    {
        return DiskKind::Ssd;
    }
    return DiskKind::Unknown;
}

/**
 * @brief Read the performance counters of a physical drive.
 *
 * The disk driver keeps IdleTime (time with an empty request queue) and QueryTime (its clock at
 * the time of the call), both in 100 ns units; their difference grows only while the disk is busy.
 * @param disk Physical drive handle.
 * @return The counters, or std::nullopt when disk performance counters are disabled.
 */
std::optional<DISK_PERFORMANCE> readPerformance(HANDLE disk)
{
    DISK_PERFORMANCE performance{};
    if (!queryDevice(disk, IOCTL_DISK_PERFORMANCE, nullptr, 0, performance))
    {
        return std::nullopt;
    }
    return performance;
}

/**
 * @brief Read the size of a physical drive.
 * @param disk Physical drive handle.
 * @return Capacity in bytes, or std::nullopt when the drive does not answer.
 */
std::optional<std::uint64_t> readCapacity(HANDLE disk)
{
    GET_LENGTH_INFORMATION length{};
    if (!queryDevice(disk, IOCTL_DISK_GET_LENGTH_INFO, nullptr, 0, length))
    {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(length.Length.QuadPart);
}

/**
 * @brief Read the product name (vendor + model) of a physical drive.
 * @param disk Physical drive handle.
 * @return Product name, or an empty string when the driver reports none.
 */
std::string readModel(HANDLE disk)
{
    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    // The descriptor is followed by its strings; ProductIdOffset is relative to the start of the buffer.
    std::array<char, kDescriptorBufferBytes> buffer{};
    DWORD returned = 0;
    if (!::DeviceIoControl(disk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer.data(),
                           static_cast<DWORD>(buffer.size()), &returned, nullptr)
        || returned < sizeof(STORAGE_DEVICE_DESCRIPTOR))
    {
        return {};
    }
    const auto* descriptor = reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR*>(buffer.data());
    if (descriptor->ProductIdOffset == 0 || descriptor->ProductIdOffset >= returned)
    {
        return {};
    }
    const char* text = buffer.data() + descriptor->ProductIdOffset;
    const std::size_t length = ::strnlen(text, returned - descriptor->ProductIdOffset);
    std::string_view product(text, length);
    while (!product.empty() && product.back() == ' ')
    {
        product.remove_suffix(1);
    }
    return std::string(product);
}

} // namespace

/**
 * @brief Enumerate the physical drives.
 * @return Disks in drive-number order, named "Disk N (letters)"; empty when none can be opened.
 */
std::vector<DiskSample> WindowsDiskProvider::readDisks()
{
    const std::map<DWORD, std::string> letters = readDriveLetters();

    std::vector<DiskSample> disks;
    for (DWORD number = 0; number < kMaxPhysicalDrives; ++number)
    {
        const UniqueHandle disk = openForQuery(L"\\\\.\\PhysicalDrive" + std::to_wstring(number));
        if (!disk)
        {
            continue;
        }
        std::string name = "Disk " + std::to_string(number);
        if (const auto found = letters.find(number); found != letters.end())
        {
            name += " (" + found->second + ")";
        }
        DiskSample sample;
        sample.id = "disk" + std::to_string(number);
        sample.name = std::move(name);
        sample.kind = readKind(disk.get());
        sample.model = readModel(disk.get());
        sample.capacityBytes = readCapacity(disk.get());
        if (const auto performance = readPerformance(disk.get()))
        {
            sample.busyTime = fromHundredNanoseconds(performance->QueryTime.QuadPart - performance->IdleTime.QuadPart);
            sample.bytesRead = static_cast<std::uint64_t>(performance->BytesRead.QuadPart);
            sample.bytesWritten = static_cast<std::uint64_t>(performance->BytesWritten.QuadPart);
        }
        disks.push_back(std::move(sample));
    }
    return disks;
}

} // namespace systemlens::providers::windows_os
