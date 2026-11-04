/**
 * @file    LinuxProcFs.h
 * @brief   Pure parsers of the Linux /proc and /sys text formats used by LinuxCpuProvider.
 *
 * No file access here: every function takes the file content, so the parsing rules are unit tested
 * with fixed strings (backend/tests/LinuxProcFsTest.cpp). File reading lives in LinuxCpuProvider.
 */
#pragma once

#include "cpu/ICpuProvider.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace systemlens::plugin::procfs {

/** Content of /proc/stat that the CPU provider uses. */
struct ProcStat
{
    providers::cpu::CpuTimes total;
    std::vector<providers::cpu::CpuTimes> perCore;
    std::optional<std::uint64_t> contextSwitches; ///< "ctxt"
    std::optional<std::uint64_t> interrupts;      ///< first value of "intr"
    std::optional<std::uint64_t> softirqs;        ///< first value of "softirq"
    std::optional<std::int64_t> bootTime;         ///< "btime", Unix seconds
};

/** Content of /proc/cpuinfo that the CPU provider uses. */
struct CpuInfo
{
    std::string modelName;             ///< "model name" (x86), else "Hardware" / "Processor" (ARM).
    std::uint32_t processorCount = 0;  ///< Number of "processor" entries.
    std::uint32_t socketCount = 0;     ///< Distinct "physical id"; 0 when absent.
    std::uint32_t coreCount = 0;       ///< Distinct ("physical id", "core id"); 0 when absent.
    std::optional<double> averageMhz;  ///< Mean of "cpu MHz".
    bool hasFlags = false;             ///< A "flags" line exists (x86).
    bool hasVirtualizationFlag = false; ///< "vmx" (Intel VT-x) or "svm" (AMD-V) in the flags.
};

/** Fields of /proc/<pid>/stat that the CPU provider uses. */
struct ProcessStat
{
    std::uint32_t pid = 0;
    std::string comm;              ///< Command name, without the parentheses.
    std::uint64_t utime = 0;       ///< Clock ticks in user mode.
    std::uint64_t stime = 0;       ///< Clock ticks in kernel mode.
    std::uint32_t numThreads = 0;
};

ProcStat parseProcStat(std::string_view text);

CpuInfo parseCpuInfo(std::string_view text);

std::optional<ProcessStat> parseProcessStat(std::string_view text);

std::optional<std::uint64_t> parseStatmResidentPages(std::string_view text);

std::optional<double> parseUptimeSeconds(std::string_view text);

std::optional<std::uint64_t> parseFileNrAllocated(std::string_view text);

std::optional<std::uint64_t> parseCacheSizeBytes(std::string_view text);

std::uint32_t countCpuList(std::string_view text);

std::optional<double> parseModelSpeedMhz(std::string_view model);

std::optional<std::uint64_t> parseUnsigned(std::string_view text);

std::string parseOsReleasePrettyName(std::string_view text);

} // namespace systemlens::plugin::procfs
