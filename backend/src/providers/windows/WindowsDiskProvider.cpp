/**
 * @file    WindowsDiskProvider.cpp
 * @brief   Implementation of WindowsDiskProvider.
 */
#include "providers/windows/WindowsDiskProvider.h"

#include "providers/windows/WindowsUtil.h"

#include <windows.h>

#include <winioctl.h>

#include <map>

namespace systemlens::providers::windows_os {

namespace {

/** Physical drive numbers are small and dense; gaps appear only after hot-unplugging. */
constexpr DWORD kMaxPhysicalDrives = 32;
constexpr int kDriveLetterCount = 26;

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
 * @brief Read the cumulative busy time of a physical drive.
 *
 * The disk driver keeps IdleTime (time with an empty request queue) and QueryTime (its clock at
 * the time of the call), both in 100 ns units; their difference grows only while the disk is busy.
 * @param disk Physical drive handle.
 * @return Busy time, or std::nullopt when disk performance counters are disabled.
 */
std::optional<std::chrono::duration<double>> readBusyTime(HANDLE disk)
{
    DISK_PERFORMANCE performance{};
    if (!queryDevice(disk, IOCTL_DISK_PERFORMANCE, nullptr, 0, performance))
    {
        return std::nullopt;
    }
    return fromHundredNanoseconds(performance.QueryTime.QuadPart - performance.IdleTime.QuadPart);
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
        disks.push_back(DiskSample{
            .id = "disk" + std::to_string(number),
            .name = std::move(name),
            .kind = readKind(disk.get()),
            .busyTime = readBusyTime(disk.get()),
        });
    }
    return disks;
}

} // namespace systemlens::providers::windows_os
