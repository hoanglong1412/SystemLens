/**
 * @file    History.h
 * @brief   Fixed-capacity time series: keeps the newest N samples, oldest first.
 */
#pragma once

#include <cstddef>
#include <deque>
#include <vector>

namespace systemlens::services::monitoring {

template <typename T>
class History
{
public:
    /** @param capacity Maximum number of samples kept; older ones are dropped. */
    explicit History(std::size_t capacity)
        : m_capacity(capacity)
    {
    }

    /**
     * @brief Append the newest sample, dropping the oldest when full.
     * @param value Sample.
     */
    void push(T value)
    {
        m_samples.push_back(std::move(value));
        if (m_samples.size() > m_capacity)
        {
            m_samples.pop_front();
        }
    }

    /**
     * @brief Copy the samples out.
     * @return Samples, oldest first.
     */
    [[nodiscard]] std::vector<T> toVector() const
    {
        return std::vector<T>(m_samples.begin(), m_samples.end());
    }

private:
    std::size_t m_capacity;
    std::deque<T> m_samples;
};

} // namespace systemlens::services::monitoring
