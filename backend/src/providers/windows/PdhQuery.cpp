/**
 * @file    PdhQuery.cpp
 * @brief   Implementation of PdhQuery.
 */
#include "providers/windows/PdhQuery.h"

#include <windows.h>

#include <pdh.h>
#include <pdhmsg.h>

#include <cstddef>

namespace systemlens::providers::windows_os {

namespace {

/**
 * @brief Tell whether a counter value can be used.
 * @param status CStatus of a raw or formatted value.
 * @return True for valid data (new or unchanged since the previous collection).
 */
bool isValidData(DWORD status)
{
    return status == PDH_CSTATUS_VALID_DATA || status == PDH_CSTATUS_NEW_DATA;
}

} // namespace

/** @brief Open the query. On failure every later call simply reports "no data". */
PdhQuery::PdhQuery()
{
    PDH_HQUERY query = nullptr;
    if (::PdhOpenQueryW(nullptr, 0, &query) == ERROR_SUCCESS)
    {
        m_query = query;
    }
}

/** @brief Close the query and all its counters. */
PdhQuery::~PdhQuery()
{
    if (m_query != nullptr)
    {
        ::PdhCloseQuery(m_query);
    }
}

/**
 * @brief Add a counter by its English path.
 *
 * The English variant is used because counter names are localized: "\Processor(_Total)\..."
 * does not exist under that name on a non-English Windows.
 * @param englishPath Counter path; may contain the instance wildcard "(*)".
 * @return Counter handle, or std::nullopt when the counter does not exist on this machine.
 */
std::optional<PdhQuery::Counter> PdhQuery::addCounter(const wchar_t* englishPath)
{
    PDH_HCOUNTER counter = nullptr;
    if (m_query == nullptr || ::PdhAddEnglishCounterW(m_query, englishPath, 0, &counter) != ERROR_SUCCESS)
    {
        return std::nullopt;
    }
    return counter;
}

/**
 * @brief Read the current values of all counters from the system.
 * @return True on success. Formatted values of rate counters need two collections.
 */
bool PdhQuery::collect()
{
    return m_query != nullptr && ::PdhCollectQueryData(m_query) == ERROR_SUCCESS;
}

/**
 * @brief Get the raw value of a counter as of the last collect().
 *
 * For "per second" counters (PERF_COUNTER_COUNTER) the raw value is the cumulative event count.
 * @param counter Handle from addCounter(), or std::nullopt.
 * @return Raw value, or std::nullopt when there is no counter or no valid data.
 */
std::optional<std::uint64_t> PdhQuery::rawValue(std::optional<Counter> counter) const
{
    PDH_RAW_COUNTER raw{};
    if (!counter || ::PdhGetRawCounterValue(*counter, nullptr, &raw) != ERROR_SUCCESS || !isValidData(raw.CStatus))
    {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(raw.FirstValue);
}

/**
 * @brief Get the calculated value of a counter as of the last collect().
 * @param counter Handle from addCounter(), or std::nullopt.
 * @return Value as a double (not capped at 100), or std::nullopt when there is no counter or the
 *         value is not available yet (first collection of a rate counter).
 */
std::optional<double> PdhQuery::formattedValue(std::optional<Counter> counter) const
{
    PDH_FMT_COUNTERVALUE value{};
    if (!counter
        || ::PdhGetFormattedCounterValue(*counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, nullptr, &value) != ERROR_SUCCESS
        || !isValidData(value.CStatus))
    {
        return std::nullopt;
    }
    return value.doubleValue;
}

/**
 * @brief Get the calculated values of every instance of a wildcard counter.
 * @param counter Handle of a counter added with "(*)", or std::nullopt.
 * @return Instance name and value for each instance with valid data; empty when unavailable.
 */
std::vector<PdhQuery::InstanceValue> PdhQuery::formattedInstances(std::optional<Counter> counter) const
{
    std::vector<InstanceValue> values;
    if (!counter)
    {
        return values;
    }

    // First call asks for the buffer size; the buffer holds the item array followed by the name strings.
    constexpr DWORD kFormat = PDH_FMT_DOUBLE | PDH_FMT_NOCAP100;
    DWORD bufferBytes = 0;
    DWORD itemCount = 0;
    if (::PdhGetFormattedCounterArrayW(*counter, kFormat, &bufferBytes, &itemCount, nullptr)
            != static_cast<PDH_STATUS>(PDH_MORE_DATA)
        || bufferBytes == 0)
    {
        return values;
    }
    // max_align_t storage keeps the item array correctly aligned.
    std::vector<std::max_align_t> buffer(bufferBytes / sizeof(std::max_align_t) + 1);
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
    if (::PdhGetFormattedCounterArrayW(*counter, kFormat, &bufferBytes, &itemCount, items) != ERROR_SUCCESS)
    {
        return values;
    }

    values.reserve(itemCount);
    for (DWORD index = 0; index < itemCount; ++index)
    {
        if (isValidData(items[index].FmtValue.CStatus) && items[index].szName != nullptr)
        {
            values.emplace_back(items[index].szName, items[index].FmtValue.doubleValue);
        }
    }
    return values;
}

} // namespace systemlens::providers::windows_os
