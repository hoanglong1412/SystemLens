/**
 * @file    LinuxUserSessionProvider.cpp
 * @brief   Implementation of LinuxUserSessionProvider.
 */
#include "providers/linux/LinuxUserSessionProvider.h"

#include <pwd.h>
#include <unistd.h>
#include <utmpx.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string_view>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kSessionTypeEnvVar = "XDG_SESSION_TYPE";
constexpr const char* kDefaultSessionName = "console";
constexpr const char* kActiveState = "Active";
constexpr std::size_t kPasswdBufferSize = 4096;

/** getutxent keeps its cursor in static storage, so only one thread may use it at a time. */
std::mutex g_utmpMutex;

/**
 * @brief Name the user the backend runs as.
 * @return Login name, or the numeric uid when it has no passwd entry.
 */
std::string currentUserName()
{
    const uid_t uid = ::geteuid();
    std::array<char, kPasswdBufferSize> buffer{};
    passwd entry{};
    passwd* result = nullptr;
    if (::getpwuid_r(uid, &entry, buffer.data(), buffer.size(), &result) == 0 && result != nullptr)
    {
        return result->pw_name;
    }
    return std::to_string(uid);
}

} // namespace

/**
 * @brief List the user sessions recorded in utmp.
 * @return One entry per login record of type USER_PROCESS. When there is none (containers, servers without a
 *         login manager) a single entry for the user running the backend is returned, so the list is not empty
 *         although somebody is clearly logged on.
 */
std::vector<UserSession> LinuxUserSessionProvider::readSessions() const
{
    std::vector<UserSession> sessions;
    {
        const std::scoped_lock lock(g_utmpMutex);
        ::setutxent();
        while (const utmpx* record = ::getutxent())
        {
            if (record->ut_type != USER_PROCESS)
            {
                continue;
            }
            // Fixed-size char arrays that are not guaranteed to be NUL-terminated.
            UserSession session;
            session.userName = std::string(record->ut_user, ::strnlen(record->ut_user, sizeof(record->ut_user)));
            session.sessionId = std::to_string(record->ut_pid);
            session.sessionName = std::string(record->ut_line, ::strnlen(record->ut_line, sizeof(record->ut_line)));
            session.state = kActiveState;
            session.logonTime = std::chrono::system_clock::time_point(std::chrono::seconds(record->ut_tv.tv_sec));
            if (std::string host(record->ut_host, ::strnlen(record->ut_host, sizeof(record->ut_host))); !host.empty())
            {
                session.host = std::move(host);
            }
            sessions.push_back(std::move(session));
        }
        ::endutxent();
    }

    if (sessions.empty())
    {
        const char* type = std::getenv(kSessionTypeEnvVar);
        sessions.push_back(UserSession{
            .userName = currentUserName(),
            .sessionId = std::nullopt,
            .sessionName = (type != nullptr && *type != '\0') ? type : kDefaultSessionName,
            .state = kActiveState,
            .logonTime = std::nullopt,
            .host = std::nullopt,
        });
    }
    return sessions;
}

} // namespace systemlens::providers::linux_os
