/**
 * @file    FeatureUnavailableError.h
 * @brief   Exception thrown by a service whose platform provider is not available
 *          (provider-level plugin failure). Controllers map it to 501 Not Implemented.
 */
#pragma once

#include <stdexcept>
#include <string>

namespace systemlens::services {

class FeatureUnavailableError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

} // namespace systemlens::services
