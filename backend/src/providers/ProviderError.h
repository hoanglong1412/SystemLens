/**
 * @file    ProviderError.h
 * @brief   Exceptions thrown by platform providers.
 */
#pragma once

#include <stdexcept>
#include <string>

namespace systemlens::providers {

/** A provider failed to read the operating system (missing file, failed system call...). */
class ProviderError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/** The provider exists only as an interface placeholder on this platform. */
class NotImplementedError : public ProviderError
{
public:
    /** @param what Name of the missing piece, e.g. "MacosCpuProvider::readSample". */
    explicit NotImplementedError(const std::string& what)
        : ProviderError(what + " is not implemented on this platform")
    {
    }
};

} // namespace systemlens::providers
