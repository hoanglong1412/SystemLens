/**
 * @file    IUserSessionProvider.h
 * @brief   Provider interface for the logged-on user sessions.
 */
#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::providers {

/** One logon session. */
struct UserSession
{
    std::string userName;
    std::optional<std::string> sessionId;
    std::string sessionName;  ///< Windows: window station ("Console", "RDP-Tcp#0"); Linux: tty.
    std::string state;        ///< "Active", "Connected", "Disconnected" or "Other".
    std::optional<std::chrono::system_clock::time_point> logonTime;
    std::optional<std::string> host; ///< Remote host / client name for remote sessions.
};

class IUserSessionProvider
{
public:
    virtual ~IUserSessionProvider() = default;

    /**
     * @brief List the logon sessions that have a user. Safe to call from several threads.
     * @return Sessions in no particular order.
     * @throws ProviderError if the sessions cannot be listed.
     */
    [[nodiscard]] virtual std::vector<UserSession> readSessions() const = 0;
};

} // namespace systemlens::providers
