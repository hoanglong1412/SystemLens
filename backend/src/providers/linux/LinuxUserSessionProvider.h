/**
 * @file    LinuxUserSessionProvider.h
 * @brief   Linux IUserSessionProvider: utmp login records.
 */
#pragma once

#include "providers/IUserSessionProvider.h"

namespace systemlens::providers::linux_os {

class LinuxUserSessionProvider final : public IUserSessionProvider
{
public:
    [[nodiscard]] std::vector<UserSession> readSessions() const override;
};

} // namespace systemlens::providers::linux_os
