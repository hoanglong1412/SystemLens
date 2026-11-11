/**
 * @file    WindowsUserSessionProvider.h
 * @brief   Windows IUserSessionProvider: Remote Desktop Services (WTS) sessions.
 */
#pragma once

#include "providers/IUserSessionProvider.h"

namespace systemlens::providers::windows_os {

class WindowsUserSessionProvider final : public IUserSessionProvider
{
public:
    [[nodiscard]] std::vector<UserSession> readSessions() const override;
};

} // namespace systemlens::providers::windows_os
