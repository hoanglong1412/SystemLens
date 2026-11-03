/**
 * @file    WindowsHelloProvider.cpp
 * @brief   [SAMPLE API] Implementation of WindowsHelloProvider.
 */
#include "WindowsHelloProvider.h"

namespace systemlens::plugin {

/**
 * @brief Get the greeting of the Windows platform.
 * @return kGreeting.
 */
std::string WindowsHelloProvider::getGreeting() const
{
    return kGreeting;
}

} // namespace systemlens::plugin
