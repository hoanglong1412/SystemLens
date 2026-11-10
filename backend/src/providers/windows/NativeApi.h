/**
 * @file    NativeApi.h
 * @brief   Layouts of the NtQuerySystemInformation results used by the Windows providers.
 *
 * winternl.h declares these structures with most fields hidden as "Reserved" (and the SDK and
 * MinGW headers name them differently), so the well-known layouts are spelled out here.
 * They have been stable since Windows Vista on both 32- and 64-bit Windows.
 */
#pragma once

#include <windows.h>

#include <winternl.h>

namespace systemlens::providers::windows_os::native {

/** STATUS_INFO_LENGTH_MISMATCH: the buffer passed to NtQuerySystemInformation is too small. */
inline constexpr NTSTATUS kStatusInfoLengthMismatch = static_cast<NTSTATUS>(0xC0000004L);

/** One element (per logical processor) of SystemProcessorPerformanceInformation. Times are in 100 ns. */
struct ProcessorPerformanceInformation
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime; ///< Includes IdleTime, DpcTime and InterruptTime.
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
};

/**
 * Leading fields of one entry of SystemProcessInformation. The real entry continues with
 * more counters and the thread array; NextEntryOffset leads to the next process.
 */
struct ProcessInformation
{
    ULONG NextEntryOffset; ///< Bytes to the next entry; 0 for the last one.
    ULONG NumberOfThreads;
    LARGE_INTEGER WorkingSetPrivateSize; ///< Bytes; the "Memory" column of Task Manager.
    ULONG HardFaultCount;
    ULONG NumberOfThreadsHighWatermark;
    ULONGLONG CycleTime;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;   ///< 100 ns.
    LARGE_INTEGER KernelTime; ///< 100 ns.
    UNICODE_STRING ImageName; ///< Not NUL-terminated; empty for the idle process.
    LONG BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
};

} // namespace systemlens::providers::windows_os::native
