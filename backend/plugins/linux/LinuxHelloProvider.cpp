/**
 * @file    LinuxHelloProvider.cpp
 * @brief   [SAMPLE API] Implementation of LinuxHelloProvider.
 */
#include "LinuxHelloProvider.h"

namespace systemlens::plugin {

/**
 * @brief Get the greeting of the Linux platform.
 * @return kGreeting.
 */
std::string LinuxHelloProvider::getGreeting() const
{
    return kGreeting;
}

} // namespace systemlens::plugin
