/**
 * @file    WindowsServiceProvider.cpp
 * @brief   Implementation of WindowsServiceProvider.
 */
#include "providers/windows/WindowsServiceProvider.h"

#include "providers/ProviderError.h"
#include "providers/windows/WindowsUtil.h"

#include <windows.h>

#include <winsvc.h>

#include <cstddef>
#include <memory>
#include <type_traits>

namespace systemlens::providers::windows_os {

namespace {

/** Closes a Service Control Manager handle. */
struct ServiceHandleCloser
{
    void operator()(SC_HANDLE handle) const noexcept { ::CloseServiceHandle(handle); }
};
using UniqueServiceHandle = std::unique_ptr<std::remove_pointer_t<SC_HANDLE>, ServiceHandleCloser>;

/** QueryServiceConfig needs at most 8 KiB (documented upper bound of the structure). */
constexpr std::size_t kConfigBufferBytes = 8192;

/**
 * @brief Translate the SCM current state.
 * @param state SERVICE_* state constant.
 * @return Matching ServiceState.
 */
ServiceState toState(DWORD state)
{
    switch (state)
    {
    case SERVICE_RUNNING:
        return ServiceState::Running;
    case SERVICE_STOPPED:
        return ServiceState::Stopped;
    case SERVICE_START_PENDING:
    case SERVICE_CONTINUE_PENDING:
        return ServiceState::Starting;
    case SERVICE_STOP_PENDING:
    case SERVICE_PAUSE_PENDING:
        return ServiceState::Stopping;
    case SERVICE_PAUSED:
        return ServiceState::Paused;
    default:
        return ServiceState::Other;
    }
}

/**
 * @brief Translate the SCM start type.
 * @param startType SERVICE_*_START constant.
 * @return Matching ServiceStartType.
 */
ServiceStartType toStartType(DWORD startType)
{
    switch (startType)
    {
    case SERVICE_AUTO_START:
    case SERVICE_BOOT_START:
    case SERVICE_SYSTEM_START:
        return ServiceStartType::Automatic;
    case SERVICE_DEMAND_START:
        return ServiceStartType::Manual;
    case SERVICE_DISABLED:
        return ServiceStartType::Disabled;
    default:
        return ServiceStartType::Other;
    }
}

/**
 * @brief Read the start type of one service.
 * @param manager Open Service Control Manager.
 * @param name    Service name.
 * @return Start type; Other when the service cannot be opened for reading its configuration.
 */
ServiceStartType readStartType(SC_HANDLE manager, const wchar_t* name)
{
    const UniqueServiceHandle service(::OpenServiceW(manager, name, SERVICE_QUERY_CONFIG));
    if (!service)
    {
        return ServiceStartType::Other;
    }
    alignas(QUERY_SERVICE_CONFIGW) std::byte buffer[kConfigBufferBytes];
    DWORD needed = 0;
    if (!::QueryServiceConfigW(service.get(), reinterpret_cast<QUERY_SERVICE_CONFIGW*>(buffer), sizeof(buffer),
                               &needed))
    {
        return ServiceStartType::Other;
    }
    return toStartType(reinterpret_cast<const QUERY_SERVICE_CONFIGW*>(buffer)->dwStartType);
}

} // namespace

/**
 * @brief List the Win32 services (not drivers) with state, start type and PID.
 * @return Services in SCM order.
 * @throws ProviderError if the Service Control Manager cannot be opened or enumerated.
 */
std::vector<SystemServiceInfo> WindowsServiceProvider::readServices() const
{
    const UniqueServiceHandle manager(::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE));
    if (!manager)
    {
        throw ProviderError("OpenSCManager failed");
    }

    std::vector<SystemServiceInfo> services;
    DWORD resumeHandle = 0;
    std::vector<std::max_align_t> buffer(kConfigBufferBytes / sizeof(std::max_align_t));
    for (;;)
    {
        DWORD bytesNeeded = 0;
        DWORD returned = 0;
        const DWORD bufferBytes = static_cast<DWORD>(buffer.size() * sizeof(std::max_align_t));
        const BOOL done = ::EnumServicesStatusExW(manager.get(), SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                                                  SERVICE_STATE_ALL, reinterpret_cast<LPBYTE>(buffer.data()),
                                                  bufferBytes, &bytesNeeded, &returned, &resumeHandle, nullptr);
        if (!done && ::GetLastError() != ERROR_MORE_DATA)
        {
            throw ProviderError("EnumServicesStatusEx failed");
        }
        const auto* entries = reinterpret_cast<const ENUM_SERVICE_STATUS_PROCESSW*>(buffer.data());
        for (DWORD index = 0; index < returned; ++index)
        {
            const ENUM_SERVICE_STATUS_PROCESSW& entry = entries[index];
            SystemServiceInfo info;
            info.name = toUtf8(entry.lpServiceName);
            info.displayName = toUtf8(entry.lpDisplayName);
            info.state = toState(entry.ServiceStatusProcess.dwCurrentState);
            info.startType = readStartType(manager.get(), entry.lpServiceName);
            if (entry.ServiceStatusProcess.dwProcessId != 0)
            {
                info.pid = entry.ServiceStatusProcess.dwProcessId;
            }
            services.push_back(std::move(info));
        }
        if (done)
        {
            return services;
        }
        // The call returned what fitted and advanced resumeHandle; grow the buffer for the rest.
        buffer.resize((bufferBytes + bytesNeeded) / sizeof(std::max_align_t) + 1);
    }
}

} // namespace systemlens::providers::windows_os
