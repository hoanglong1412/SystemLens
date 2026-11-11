/**
 * @file    IStartupProvider.h
 * @brief   Provider interface for the programs started at logon.
 */
#pragma once

#include <string>
#include <vector>

namespace systemlens::providers {

/** Whose logon starts the item. */
enum class StartupScope
{
    User,
    AllUsers,
};

/** One startup item. */
struct StartupItem
{
    std::string name;
    std::string command;
    std::string location; ///< Registry key or folder the item comes from.
    StartupScope scope = StartupScope::User;
    bool enabled = true;
};

class IStartupProvider
{
public:
    virtual ~IStartupProvider() = default;

    /**
     * @brief List the startup items. Stateless and safe to call from several threads.
     * @return Items in no particular order.
     * @throws ProviderError if the locations cannot be read at all.
     */
    [[nodiscard]] virtual std::vector<StartupItem> readStartupItems() const = 0;
};

} // namespace systemlens::providers
