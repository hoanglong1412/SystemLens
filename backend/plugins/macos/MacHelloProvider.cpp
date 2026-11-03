/**
 * @file    MacHelloProvider.cpp
 * @brief   [SAMPLE API] Implementation of MacHelloProvider.
 *          Written but not built in this change (no macOS build machine).
 */
#include "MacHelloProvider.h"

namespace systemlens::plugin {

/**
 * @brief Get the greeting of the macOS platform.
 * @return kGreeting.
 */
std::string MacHelloProvider::getGreeting() const
{
    return kGreeting;
}

} // namespace systemlens::plugin
