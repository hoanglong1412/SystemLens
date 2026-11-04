/**
 * @file    LinuxSystemInfoProvider.cpp
 * @brief   Implementation of LinuxSystemInfoProvider.
 */
#include "LinuxSystemInfoProvider.h"

#include "LinuxProcFs.h"

#include <pwd.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

namespace systemlens::plugin {

namespace {

constexpr const char* kOsReleasePath = "/etc/os-release";
constexpr const char* kFallbackOsName = "Linux";
/** Initial getpwuid_r buffer size when sysconf gives no hint. */
constexpr long kDefaultPasswdBufferSize = 4096;

/**
 * @brief Name of the user the backend runs as.
 * @return passwd entry of the effective uid, else $USER, else empty.
 */
std::string currentUserName()
{
    long size = sysconf(_SC_GETPW_R_SIZE_MAX);
    std::vector<char> buffer(static_cast<std::size_t>(size > 0 ? size : kDefaultPasswdBufferSize));
    passwd entry{};
    passwd* result = nullptr;
    if (getpwuid_r(geteuid(), &entry, buffer.data(), buffer.size(), &result) == 0 && result != nullptr)
    {
        return result->pw_name;
    }
    const char* user = std::getenv("USER");
    return user != nullptr ? user : "";
}

/**
 * @brief Distribution name from /etc/os-release.
 * @return PRETTY_NAME, or empty when unavailable.
 */
std::string distributionName()
{
    std::ifstream in(kOsReleasePath);
    if (!in)
    {
        return {};
    }
    std::ostringstream content;
    content << in.rdbuf();
    return procfs::parseOsReleasePrettyName(content.str());
}

} // namespace

/**
 * @brief Read host name, distribution, kernel version and user.
 * @return Identity; unknown parts are empty strings.
 */
providers::system::SystemIdentity LinuxSystemInfoProvider::getIdentity() const
{
    providers::system::SystemIdentity identity;
    identity.platform = "linux";
    identity.userName = currentUserName();

    utsname uts{};
    const bool haveUname = uname(&uts) == 0;
    std::string name = distributionName();
    if (name.empty())
    {
        name = kFallbackOsName;
    }
    if (haveUname)
    {
        identity.hostName = uts.nodename;
        identity.osName = name + " " + uts.machine;
        identity.osVersion = std::string("Kernel ") + uts.release;
    }
    else
    {
        identity.osName = name;
    }
    return identity;
}

} // namespace systemlens::plugin
