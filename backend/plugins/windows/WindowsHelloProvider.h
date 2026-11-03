/**
 * @file    WindowsHelloProvider.h
 * @brief   [SAMPLE API] IHelloProvider of the Windows platform plugin.
 */
#pragma once

#include "sample/IHelloProvider.h"

namespace systemlens::plugin {

class WindowsHelloProvider final : public providers::sample::IHelloProvider
{
public:
    /** Greeting returned by getGreeting(). */
    static constexpr const char* kGreeting = "hello from Windows";

    std::string getGreeting() const override;
};

} // namespace systemlens::plugin
