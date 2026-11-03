/**
 * @file    MacHelloProvider.h
 * @brief   [SAMPLE API] IHelloProvider of the macOS platform plugin.
 *          Written but not built in this change (no macOS build machine).
 */
#pragma once

#include "sample/IHelloProvider.h"

namespace systemlens::plugin {

class MacHelloProvider final : public providers::sample::IHelloProvider
{
public:
    /** Greeting returned by getGreeting(). */
    static constexpr const char* kGreeting = "hello from MAC";

    std::string getGreeting() const override;
};

} // namespace systemlens::plugin
