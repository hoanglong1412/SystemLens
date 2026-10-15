/**
 * @file    ISampleHelloService.h
 * @brief   [SAMPLE API] Service interface behind GET /api/v1/sample/hello.
 *
 * Exists only to exercise the FE <-> BE plumbing and to serve as the reference
 * example of the Controller -> Service layering. Interface so tests can mock it.
 */
#pragma once

#include <string>

namespace systemlens::services::sample {

class ISampleHelloService
{
public:
    virtual ~ISampleHelloService() = default;

    /**
     * @brief Get the greeting returned to the client.
     * @return The greeting text.
     */
    virtual std::string getGreeting() const = 0;
};

} // namespace systemlens::services::sample
