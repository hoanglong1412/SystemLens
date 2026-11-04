/**
 * @file    PlatformPlugin.h
 * @brief   Host side of the platform plugin: loads the library, checks its ABI version and creates
 *          the providers it exports. OS independent (OS specifics live in DynamicLibrary).
 *
 * Failure levels (design D5):
 *  - library level (missing file, load error, no ABI entry point, ABI mismatch): the constructor
 *    throws, which stops the backend before READY;
 *  - provider level (create entry point missing or returning nullptr): create*Provider() logs a
 *    warning and returns an empty pointer; only the features of that provider are unavailable.
 */
#pragma once

#include "platform/DynamicLibrary.h"
#include "providers/cpu/ICpuProvider.h"
#include "providers/sample/IHelloProvider.h"
#include "providers/system/ISystemInfoProvider.h"

#include <filesystem>
#include <memory>

namespace systemlens::platform {

class PlatformPlugin
{
public:
    explicit PlatformPlugin(const std::filesystem::path& libraryPath);

    std::shared_ptr<const providers::sample::IHelloProvider> createHelloProvider() const;
    std::shared_ptr<providers::cpu::ICpuProvider> createCpuProvider() const;
    std::shared_ptr<const providers::system::ISystemInfoProvider> createSystemInfoProvider() const;

private:
    // shared_ptr: every provider deleter keeps a copy, so the library is unloaded only after the
    // last provider is destroyed (its vtable and destroy function live in the library).
    std::shared_ptr<DynamicLibrary> m_library;
};

} // namespace systemlens::platform
