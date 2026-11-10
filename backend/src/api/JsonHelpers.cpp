/**
 * @file    JsonHelpers.cpp
 * @brief   Implementation of the non-template helpers of JsonHelpers.h.
 */
#include "api/JsonHelpers.h"

#include <iomanip>
#include <sstream>

namespace systemlens::api {

/**
 * @brief Format a time point as an RFC 3339 UTC timestamp with second precision.
 * @param time Time to format.
 * @return e.g. "2026-09-14T08:12:41Z".
 */
std::string toRfc3339(std::chrono::system_clock::time_point time)
{
    using namespace std::chrono;
    // C++20 calendar types split the time point without the non-thread-safe gmtime().
    const auto seconds = floor<std::chrono::seconds>(time);
    const auto day = floor<days>(seconds);
    const year_month_day date{day};
    const hh_mm_ss clock{seconds - day};

    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << static_cast<int>(date.year()) << '-' << std::setw(2)
        << static_cast<unsigned>(date.month()) << '-' << std::setw(2) << static_cast<unsigned>(date.day()) << 'T'
        << std::setw(2) << clock.hours().count() << ':' << std::setw(2) << clock.minutes().count() << ':'
        << std::setw(2) << clock.seconds().count() << 'Z';
    return out.str();
}

} // namespace systemlens::api
