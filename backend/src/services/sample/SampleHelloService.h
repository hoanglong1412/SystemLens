/**
 * @file    SampleHelloService.h
 * @brief   [SAMPLE API] Default ISampleHelloService: always greets with "hello client".
 */
#pragma once

#include "services/sample/ISampleHelloService.h"

namespace systemlens::services::sample {

class SampleHelloService final : public ISampleHelloService
{
public:
    /** Greeting returned by getGreeting(). */
    static constexpr const char* kGreeting = "hello client 2222";

    /**
     * @brief Get the fixed greeting.
     * @return kGreeting.
     */
    std::string getGreeting() const override;
};

} // namespace systemlens::services::sample
