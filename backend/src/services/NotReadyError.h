/**
 * @file    NotReadyError.h
 * @brief   Exception thrown by a service that has no data yet (e.g. before the first sampling
 *          interval). Controllers map it to 503 Service Unavailable.
 */
#pragma once

#include <stdexcept>

namespace systemlens::services {

class NotReadyError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

} // namespace systemlens::services
