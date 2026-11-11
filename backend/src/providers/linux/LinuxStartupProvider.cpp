/**
 * @file    LinuxStartupProvider.cpp
 * @brief   Implementation of LinuxStartupProvider.
 */
#include "providers/linux/LinuxStartupProvider.h"

#include "providers/linux/ProcFs.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <string_view>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kSystemAutostartDir = "/etc/xdg/autostart";
constexpr const char* kConfigHomeEnvVar = "XDG_CONFIG_HOME";
constexpr const char* kHomeEnvVar = "HOME";
constexpr const char* kDefaultConfigDir = ".config";
constexpr const char* kAutostartSubDir = "autostart";
constexpr std::string_view kDesktopSuffix = ".desktop";
constexpr std::string_view kMainGroup = "[Desktop Entry]";

/**
 * @brief Find the autostart folder of the current user.
 * @return $XDG_CONFIG_HOME/autostart, else $HOME/.config/autostart; empty path when neither variable is set.
 */
std::filesystem::path userAutostartDir()
{
    if (const char* config = std::getenv(kConfigHomeEnvVar); config != nullptr && *config != '\0')
    {
        return std::filesystem::path(config) / kAutostartSubDir;
    }
    if (const char* home = std::getenv(kHomeEnvVar); home != nullptr && *home != '\0')
    {
        return std::filesystem::path(home) / kDefaultConfigDir / kAutostartSubDir;
    }
    return {};
}

/**
 * @brief Read one autostart entry.
 * @param file     .desktop file.
 * @param scope    Whose logon starts it.
 * @param location Folder shown in the UI.
 * @return The item, or std::nullopt when the file has no Exec line.
 */
std::optional<StartupItem> parseDesktopFile(const std::filesystem::path& file, StartupScope scope,
                                            const std::string& location)
{
    const auto content = procfs::readFile(file);
    if (!content)
    {
        return std::nullopt;
    }
    StartupItem item;
    item.name = file.stem().string();
    item.location = location;
    item.scope = scope;
    bool inMainGroup = false;
    bool hasExec = false;
    for (std::string_view line : procfs::splitLines(*content))
    {
        line = procfs::trim(line);
        if (line.starts_with('['))
        {
            inMainGroup = line == kMainGroup;
            continue;
        }
        const auto equals = line.find('=');
        if (!inMainGroup || equals == std::string_view::npos)
        {
            continue;
        }
        const std::string_view key = line.substr(0, equals);
        const std::string_view value = procfs::trim(line.substr(equals + 1));
        // Unlocalized "Name=" only: "Name[de]=" has the locale in the key.
        if (key == "Name")
        {
            item.name = value;
        }
        else if (key == "Exec")
        {
            item.command = value;
            hasExec = true;
        }
        else if (key == "Hidden" && value == "true")
        {
            item.enabled = false;
        }
        else if (key == "X-GNOME-Autostart-enabled" && value == "false")
        {
            item.enabled = false;
        }
    }
    return hasExec ? std::optional(std::move(item)) : std::nullopt;
}

/**
 * @brief Add the entries of one folder.
 * @param directory Autostart folder; missing folders are ignored.
 * @param scope     Whose logon starts the entries.
 * @param seen      File names already taken from a more specific folder; updated.
 * @param items     Receives the entries.
 */
void collect(const std::filesystem::path& directory, StartupScope scope, std::set<std::string>& seen,
             std::vector<StartupItem>& items)
{
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(directory, error))
    {
        const std::string fileName = entry.path().filename().string();
        if (!fileName.ends_with(kDesktopSuffix) || !seen.insert(fileName).second)
        {
            continue;
        }
        if (auto item = parseDesktopFile(entry.path(), scope, directory.string()))
        {
            items.push_back(std::move(*item));
        }
    }
}

} // namespace

/**
 * @brief List the XDG autostart entries.
 * @return User entries first, then system-wide ones; an entry of the user hides the system entry of the
 *         same file name (that is how the user overrides or disables it).
 */
std::vector<StartupItem> LinuxStartupProvider::readStartupItems() const
{
    std::set<std::string> seen;
    std::vector<StartupItem> items;
    if (const auto userDir = userAutostartDir(); !userDir.empty())
    {
        collect(userDir, StartupScope::User, seen, items);
    }
    collect(kSystemAutostartDir, StartupScope::AllUsers, seen, items);
    return items;
}

} // namespace systemlens::providers::linux_os
