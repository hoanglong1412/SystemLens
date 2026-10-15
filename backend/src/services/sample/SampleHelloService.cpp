/**
 * @file    SampleHelloService.cpp
 * @brief   [SAMPLE API] Implementation of SampleHelloService.
 */
#include "services/sample/SampleHelloService.h"

namespace systemlens::services::sample {

std::string SampleHelloService::getGreeting() const
{
    return kGreeting;
}

} // namespace systemlens::services::sample
