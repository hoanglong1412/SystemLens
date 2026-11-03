/**
 * @file    LinuxHelloProvider.h
 * @brief   [SAMPLE API] IHelloProvider of the Linux platform plugin.
 */
#pragma once

#include "sample/IHelloProvider.h"

namespace systemlens::plugin {

class LinuxHelloProvider final : public providers::sample::IHelloProvider
{
public:
    /** Greeting returned by getGreeting(). */
    static constexpr const char* kGreeting = "hello from Linux";

    std::string getGreeting() const override;
};

} // namespace systemlens::plugin
