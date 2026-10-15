/**
 * @file    SampleHelloService.h
 * @brief   [SAMPLE API] Default ISampleHelloService: always greets with "@@Hello world@@".
 */
#pragma once

#include "services/sample/ISampleHelloService.h"

namespace systemlens::services::sample {

class SampleHelloService final : public ISampleHelloService
{
public:
    /** Greeting returned by getGreeting(). */
    static constexpr const char* kGreeting = "@@Hello world@@";

    /**
     * @brief Get the fixed greeting.
     * @return kGreeting.
     */
    std::string getGreeting() const override;
};

} // namespace systemlens::services::sample
