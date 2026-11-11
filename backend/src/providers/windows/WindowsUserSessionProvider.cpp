/**
 * @file    WindowsUserSessionProvider.cpp
 * @brief   Implementation of WindowsUserSessionProvider.
 */
#include "providers/windows/WindowsUserSessionProvider.h"

#include "providers/ProviderError.h"
#include "providers/windows/WindowsUtil.h"

#include <windows.h>

#include <wtsapi32.h>

#include <memory>
#include <string_view>

namespace systemlens::providers::windows_os {

namespace {

/** Seconds between 1601-01-01 (FILETIME epoch) and 1970-01-01 (system_clock epoch). */
constexpr std::int64_t kFileTimeToUnixEpochSeconds = 11'644'473'600;
constexpr double kHundredNanosecondsPerSecond = 10'000'000.0;

/** Frees memory returned by the WTS API. */
struct WtsDeleter
{
    void operator()(void* memory) const noexcept { ::WTSFreeMemory(memory); }
};
template <typename T>
using WtsPointer = std::unique_ptr<T, WtsDeleter>;

/**
 * @brief Read one string property of a session.
 * @param sessionId Session id.
 * @param info      WTS_INFO_CLASS of a string property.
 * @return The text; empty when unavailable.
 */
std::string queryString(DWORD sessionId, WTS_INFO_CLASS info)
{
    LPWSTR raw = nullptr;
    DWORD bytes = 0;
    if (!::WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, sessionId, info, &raw, &bytes) || raw == nullptr)
    {
        return {};
    }
    const WtsPointer<wchar_t> text(raw);
    return toUtf8(text.get());
}

/**
 * @brief Read the logon time of a session.
 * @param sessionId Session id.
 * @return Logon time, or std::nullopt when unavailable or never logged on.
 */
std::optional<std::chrono::system_clock::time_point> queryLogonTime(DWORD sessionId)
{
    LPWSTR raw = nullptr;
    DWORD bytes = 0;
    if (!::WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, sessionId, WTSSessionInfo, &raw, &bytes)
        || raw == nullptr)
    {
        return std::nullopt;
    }
    const WtsPointer<wchar_t> buffer(raw);
    const auto* info = reinterpret_cast<const WTSINFOW*>(buffer.get());
    if (info->LogonTime.QuadPart == 0)
    {
        return std::nullopt;
    }
    const double seconds = static_cast<double>(info->LogonTime.QuadPart) / kHundredNanosecondsPerSecond
                           - static_cast<double>(kFileTimeToUnixEpochSeconds);
    return std::chrono::system_clock::time_point(std::chrono::seconds(static_cast<std::int64_t>(seconds)));
}

/**
 * @brief Translate a WTS connection state.
 * @param state WTS_CONNECTSTATE_CLASS value.
 * @return "Active", "Connected", "Disconnected" or "Other".
 */
const char* toStateText(WTS_CONNECTSTATE_CLASS state)
{
    switch (state)
    {
    case WTSActive:
        return "Active";
    case WTSConnected:
        return "Connected";
    case WTSDisconnected:
        return "Disconnected";
    default:
        return "Other";
    }
}

} // namespace

/**
 * @brief List the sessions that have a user (the services session 0 and the listener have none).
 * @return Sessions.
 * @throws ProviderError if the sessions cannot be enumerated.
 */
std::vector<UserSession> WindowsUserSessionProvider::readSessions() const
{
    WTS_SESSION_INFOW* rawSessions = nullptr;
    DWORD count = 0;
    if (!::WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &rawSessions, &count))
    {
        throw ProviderError("WTSEnumerateSessions failed");
    }
    const WtsPointer<WTS_SESSION_INFOW> sessions(rawSessions);

    std::vector<UserSession> result;
    for (DWORD index = 0; index < count; ++index)
    {
        const WTS_SESSION_INFOW& session = sessions.get()[index];
        std::string user = queryString(session.SessionId, WTSUserName);
        if (user.empty())
        {
            continue;
        }
        std::string domain = queryString(session.SessionId, WTSDomainName);
        std::string client = queryString(session.SessionId, WTSClientName);
        UserSession entry;
        entry.userName = domain.empty() ? std::move(user) : domain + "\\" + user;
        entry.sessionId = std::to_string(session.SessionId);
        entry.sessionName = session.pWinStationName != nullptr ? toUtf8(session.pWinStationName) : std::string();
        entry.state = toStateText(session.State);
        entry.logonTime = queryLogonTime(session.SessionId);
        if (!client.empty())
        {
            entry.host = std::move(client);
        }
        result.push_back(std::move(entry));
    }
    return result;
}

} // namespace systemlens::providers::windows_os
