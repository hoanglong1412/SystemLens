/**
 * @file    LinuxProcFs.cpp
 * @brief   Implementation of the /proc and /sys text parsers.
 */
#include "LinuxProcFs.h"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <set>
#include <utility>

namespace systemlens::plugin::procfs {

namespace {

constexpr double kMhzPerGhz = 1000.0;
constexpr std::uint64_t kBytesPerKib = 1024;
constexpr std::uint64_t kBytesPerMib = 1024 * 1024;
constexpr std::uint64_t kBytesPerGib = 1024 * 1024 * 1024;

/**
 * @brief Field indexes of a "cpu" line of /proc/stat after the label (see proc(5)).
 * guest / guest_nice are not read: the kernel already counts them in user / nice.
 */
enum CpuField : std::size_t
{
    kUser = 0,
    kNice,
    kSystem,
    kIdle,
    kIowait,
    kIrq,
    kSoftirq,
    kSteal,
    kCpuFieldCount,
};

/**
 * @brief Index of the fields of /proc/<pid>/stat after "(comm) ", counted from 0 = state (field 3).
 * Field numbers in proc(5): utime 14, stime 15, num_threads 20.
 */
constexpr std::size_t kStatUtimeIndex = 14 - 3;
constexpr std::size_t kStatStimeIndex = 15 - 3;
constexpr std::size_t kStatNumThreadsIndex = 20 - 3;

/**
 * @brief Split text into whitespace-separated tokens.
 * @param text Input.
 * @return Tokens (views into text).
 */
std::vector<std::string_view> splitWhitespace(std::string_view text)
{
    std::vector<std::string_view> tokens;
    std::size_t i = 0;
    while (i < text.size())
    {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r'))
        {
            ++i;
        }
        const std::size_t start = i;
        while (i < text.size() && text[i] != ' ' && text[i] != '\t' && text[i] != '\n' && text[i] != '\r')
        {
            ++i;
        }
        if (i > start)
        {
            tokens.push_back(text.substr(start, i - start));
        }
    }
    return tokens;
}

/**
 * @brief Call fn(line) for every line of text.
 * @param text Input.
 * @param fn Callback taking a std::string_view without the line break.
 */
template <typename Fn>
void forEachLine(std::string_view text, Fn&& fn)
{
    std::size_t start = 0;
    while (start <= text.size())
    {
        const std::size_t end = text.find('\n', start);
        const std::size_t stop = end == std::string_view::npos ? text.size() : end;
        fn(text.substr(start, stop - start));
        if (end == std::string_view::npos)
        {
            break;
        }
        start = end + 1;
    }
}

/**
 * @brief Remove leading / trailing spaces and tabs.
 * @param text Input.
 * @return Trimmed view.
 */
std::string_view trim(std::string_view text)
{
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
    {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

/**
 * @brief Parse a decimal floating point number.
 * @param text Input, e.g. "2304.000".
 * @return Value, or empty when text is not a number.
 */
std::optional<double> parseDouble(std::string_view text)
{
    text = trim(text);
    if (text.empty())
    {
        return std::nullopt;
    }
    // std::from_chars for double is not available on every supported standard library; strtod on a
    // NUL-terminated copy is portable and the strings are short.
    const std::string copy(text);
    char* end = nullptr;
    const double value = std::strtod(copy.c_str(), &end);
    if (end != copy.c_str() + copy.size() || !std::isfinite(value))
    {
        return std::nullopt;
    }
    return value;
}

/**
 * @brief Parse the values of a "cpu" / "cpuN" line of /proc/stat.
 * @param fields Tokens after the label.
 * @return Times split in the provider's disjoint parts, or empty when the line is too short.
 */
std::optional<providers::cpu::CpuTimes> parseCpuFields(const std::vector<std::string_view>& fields)
{
    // Kernels older than 2.6.11 have no steal column; earlier fields are mandatory.
    constexpr std::size_t kMinimumFields = kSoftirq + 1;
    if (fields.size() < kMinimumFields + 1)
    {
        return std::nullopt;
    }
    std::uint64_t values[kCpuFieldCount] = {};
    for (std::size_t i = 0; i < kCpuFieldCount && i + 1 < fields.size(); ++i)
    {
        values[i] = parseUnsigned(fields[i + 1]).value_or(0);
    }
    providers::cpu::CpuTimes times;
    times.user = values[kUser] + values[kNice];
    // Steal = time a hypervisor ran another guest: not idle for this machine, closest to kernel time.
    times.kernel = values[kSystem] + values[kSteal];
    times.dpc = values[kSoftirq];
    times.interrupt = values[kIrq];
    // iowait is idle time while a disk request is pending (Task Manager counts it as idle too).
    times.idle = values[kIdle] + values[kIowait];
    return times;
}

/**
 * @brief Value after the first ':' of a "key : value" line.
 * @param line Input line.
 * @return Trimmed value, empty when there is no ':'.
 */
std::string_view valueAfterColon(std::string_view line)
{
    const auto colon = line.find(':');
    return colon == std::string_view::npos ? std::string_view{} : trim(line.substr(colon + 1));
}

/**
 * @brief Key before the first ':' of a "key : value" line.
 * @param line Input line.
 * @return Trimmed key.
 */
std::string_view keyBeforeColon(std::string_view line)
{
    const auto colon = line.find(':');
    return trim(colon == std::string_view::npos ? line : line.substr(0, colon));
}

} // namespace

/**
 * @brief Parse an unsigned decimal integer.
 * @param text Input; surrounding spaces are ignored.
 * @return Value, or empty when text is not entirely a number.
 */
std::optional<std::uint64_t> parseUnsigned(std::string_view text)
{
    text = trim(text);
    std::uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || text.empty())
    {
        return std::nullopt;
    }
    return value;
}

/**
 * @brief Parse /proc/stat.
 * @param text File content.
 * @return Parsed values; missing lines leave their fields empty / zero.
 */
ProcStat parseProcStat(std::string_view text)
{
    ProcStat stat;
    forEachLine(text, [&stat](std::string_view line) {
        const auto fields = splitWhitespace(line);
        if (fields.empty())
        {
            return;
        }
        const std::string_view label = fields[0];
        if (label == "cpu")
        {
            if (auto times = parseCpuFields(fields))
            {
                stat.total = *times;
            }
        }
        else if (label.size() > 3 && label.substr(0, 3) == "cpu")
        {
            if (auto times = parseCpuFields(fields))
            {
                stat.perCore.push_back(*times);
            }
        }
        else if (label == "ctxt" && fields.size() > 1)
        {
            stat.contextSwitches = parseUnsigned(fields[1]);
        }
        else if (label == "intr" && fields.size() > 1)
        {
            stat.interrupts = parseUnsigned(fields[1]);
        }
        else if (label == "softirq" && fields.size() > 1)
        {
            stat.softirqs = parseUnsigned(fields[1]);
        }
        else if (label == "btime" && fields.size() > 1)
        {
            if (auto value = parseUnsigned(fields[1]))
            {
                stat.bootTime = static_cast<std::int64_t>(*value);
            }
        }
    });
    return stat;
}

/**
 * @brief Parse /proc/cpuinfo.
 * @param text File content.
 * @return Parsed summary.
 */
CpuInfo parseCpuInfo(std::string_view text)
{
    CpuInfo info;
    std::set<std::string> sockets;
    std::set<std::pair<std::string, std::string>> cores;
    std::string currentSocket;
    std::string fallbackModel;
    double mhzSum = 0;
    std::uint32_t mhzCount = 0;

    forEachLine(text, [&](std::string_view line) {
        const std::string_view key = keyBeforeColon(line);
        const std::string_view value = valueAfterColon(line);
        if (key == "processor")
        {
            ++info.processorCount;
            currentSocket.clear();
        }
        else if (key == "model name" && info.modelName.empty())
        {
            info.modelName = std::string(value);
        }
        else if ((key == "Hardware" || key == "Processor") && fallbackModel.empty())
        {
            fallbackModel = std::string(value);
        }
        else if (key == "physical id")
        {
            currentSocket = std::string(value);
            sockets.insert(currentSocket);
        }
        else if (key == "core id")
        {
            cores.insert({currentSocket, std::string(value)});
        }
        else if (key == "cpu MHz")
        {
            if (auto mhz = parseDouble(value))
            {
                mhzSum += *mhz;
                ++mhzCount;
            }
        }
        else if (key == "flags" && !info.hasFlags)
        {
            info.hasFlags = true;
            for (const auto flag : splitWhitespace(value))
            {
                if (flag == "vmx" || flag == "svm")
                {
                    info.hasVirtualizationFlag = true;
                }
            }
        }
    });

    if (info.modelName.empty())
    {
        info.modelName = fallbackModel;
    }
    info.socketCount = static_cast<std::uint32_t>(sockets.size());
    info.coreCount = static_cast<std::uint32_t>(cores.size());
    if (mhzCount > 0)
    {
        info.averageMhz = mhzSum / mhzCount;
    }
    return info;
}

/**
 * @brief Parse /proc/<pid>/stat.
 * @param text File content, e.g. "1234 (my (app)) S 1 ...".
 * @return Fields, or empty when the format is not recognized.
 */
std::optional<ProcessStat> parseProcessStat(std::string_view text)
{
    // comm may contain spaces and parentheses: it spans from the first '(' to the LAST ')'.
    const auto open = text.find('(');
    const auto close = text.rfind(')');
    if (open == std::string_view::npos || close == std::string_view::npos || close < open)
    {
        return std::nullopt;
    }
    const auto pid = parseUnsigned(text.substr(0, open));
    const auto fields = splitWhitespace(text.substr(close + 1));
    if (!pid || fields.size() <= kStatNumThreadsIndex)
    {
        return std::nullopt;
    }

    ProcessStat stat;
    stat.pid = static_cast<std::uint32_t>(*pid);
    stat.comm = std::string(text.substr(open + 1, close - open - 1));
    stat.utime = parseUnsigned(fields[kStatUtimeIndex]).value_or(0);
    stat.stime = parseUnsigned(fields[kStatStimeIndex]).value_or(0);
    stat.numThreads = static_cast<std::uint32_t>(parseUnsigned(fields[kStatNumThreadsIndex]).value_or(0));
    return stat;
}

/**
 * @brief Parse /proc/<pid>/statm.
 * @param text File content: "size resident shared text lib data dt" (pages).
 * @return Resident set size in pages, or empty when malformed.
 */
std::optional<std::uint64_t> parseStatmResidentPages(std::string_view text)
{
    const auto fields = splitWhitespace(text);
    return fields.size() >= 2 ? parseUnsigned(fields[1]) : std::nullopt;
}

/**
 * @brief Parse /proc/uptime.
 * @param text File content: "<uptime seconds> <idle seconds>".
 * @return Uptime in seconds, or empty when malformed.
 */
std::optional<double> parseUptimeSeconds(std::string_view text)
{
    const auto fields = splitWhitespace(text);
    return fields.empty() ? std::nullopt : parseDouble(fields[0]);
}

/**
 * @brief Parse /proc/sys/fs/file-nr.
 * @param text File content: "<allocated> <free> <max>".
 * @return Allocated file handles, or empty when malformed.
 */
std::optional<std::uint64_t> parseFileNrAllocated(std::string_view text)
{
    const auto fields = splitWhitespace(text);
    return fields.empty() ? std::nullopt : parseUnsigned(fields[0]);
}

/**
 * @brief Parse a sysfs cache size.
 * @param text e.g. "32K", "1024K", "8M", "8192" (bytes).
 * @return Size in bytes, or empty when malformed.
 */
std::optional<std::uint64_t> parseCacheSizeBytes(std::string_view text)
{
    text = trim(text);
    if (text.empty())
    {
        return std::nullopt;
    }
    std::uint64_t multiplier = 1;
    switch (text.back())
    {
    case 'K':
        multiplier = kBytesPerKib;
        break;
    case 'M':
        multiplier = kBytesPerMib;
        break;
    case 'G':
        multiplier = kBytesPerGib;
        break;
    default:
        break;
    }
    if (multiplier != 1)
    {
        text.remove_suffix(1);
    }
    const auto value = parseUnsigned(text);
    return value ? std::optional<std::uint64_t>(*value * multiplier) : std::nullopt;
}

/**
 * @brief Count the CPUs of a sysfs CPU list.
 * @param text e.g. "0-3,8,10-11".
 * @return Number of CPUs listed (0 when malformed).
 */
std::uint32_t countCpuList(std::string_view text)
{
    std::uint32_t count = 0;
    std::size_t start = 0;
    text = trim(text);
    while (start < text.size())
    {
        const auto comma = text.find(',', start);
        const auto item = text.substr(start, comma == std::string_view::npos ? text.size() - start : comma - start);
        const auto dash = item.find('-');
        if (dash == std::string_view::npos)
        {
            count += parseUnsigned(item) ? 1 : 0;
        }
        else
        {
            const auto first = parseUnsigned(item.substr(0, dash));
            const auto last = parseUnsigned(item.substr(dash + 1));
            if (first && last && *last >= *first)
            {
                count += static_cast<std::uint32_t>(*last - *first + 1);
            }
        }
        if (comma == std::string_view::npos)
        {
            break;
        }
        start = comma + 1;
    }
    return count;
}

/**
 * @brief Read the nominal clock from an Intel-style model name.
 * @param model e.g. "Intel(R) Core(TM) i5-8300H CPU @ 2.30GHz".
 * @return Speed in MHz (2300), or empty when the model has no "@ x.xxGHz" suffix.
 */
std::optional<double> parseModelSpeedMhz(std::string_view model)
{
    const auto at = model.rfind('@');
    if (at == std::string_view::npos)
    {
        return std::nullopt;
    }
    std::string_view speed = trim(model.substr(at + 1));
    constexpr std::string_view kGhz = "GHz";
    if (speed.size() <= kGhz.size() || speed.substr(speed.size() - kGhz.size()) != kGhz)
    {
        return std::nullopt;
    }
    speed.remove_suffix(kGhz.size());
    const auto ghz = parseDouble(speed);
    return ghz ? std::optional<double>(*ghz * kMhzPerGhz) : std::nullopt;
}

/**
 * @brief Read PRETTY_NAME from /etc/os-release.
 * @param text File content.
 * @return Unquoted PRETTY_NAME, or empty string when absent.
 */
std::string parseOsReleasePrettyName(std::string_view text)
{
    std::string name;
    forEachLine(text, [&name](std::string_view line) {
        constexpr std::string_view kKey = "PRETTY_NAME=";
        if (line.substr(0, kKey.size()) != kKey)
        {
            return;
        }
        std::string_view value = trim(line.substr(kKey.size()));
        if (value.size() >= 2 && (value.front() == '"' || value.front() == '\'') && value.back() == value.front())
        {
            value = value.substr(1, value.size() - 2);
        }
        name = std::string(value);
    });
    return name;
}

} // namespace systemlens::plugin::procfs
