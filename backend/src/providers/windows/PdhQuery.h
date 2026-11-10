/**
 * @file    PdhQuery.h
 * @brief   RAII wrapper around a PDH (Performance Data Helper) query: the Windows API behind
 *          Performance Monitor counters such as "\System\Context Switches/sec".
 *
 * pdh.h is not included here; PDH handles are plain pointers, so Counter hides them.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace systemlens::providers::windows_os {

class PdhQuery
{
public:
    /** Opaque handle of a counter added to the query. */
    using Counter = void*;
    /** One instance of a wildcard counter: instance name and value. */
    using InstanceValue = std::pair<std::wstring, double>;

    PdhQuery();
    ~PdhQuery();

    PdhQuery(const PdhQuery&) = delete;
    PdhQuery& operator=(const PdhQuery&) = delete;

    std::optional<Counter> addCounter(const wchar_t* englishPath);
    bool collect();
    [[nodiscard]] std::optional<std::uint64_t> rawValue(std::optional<Counter> counter) const;
    [[nodiscard]] std::optional<double> formattedValue(std::optional<Counter> counter) const;
    [[nodiscard]] std::vector<InstanceValue> formattedInstances(std::optional<Counter> counter) const;

private:
    void* m_query = nullptr; ///< PDH_HQUERY; null when the query could not be opened.
};

} // namespace systemlens::providers::windows_os
