/**
 * @file    PerformanceController.cpp
 * @brief   Implementation of PerformanceController.
 */
#include "api/performance/PerformanceController.h"

#include "api/ApiResponse.h"
#include "api/JsonHelpers.h"

#include <stdexcept>

namespace systemlens::api::performance {

namespace {

using services::performance::CpuPerformance;
using services::performance::ResourceKind;
using services::performance::ResourceSummary;

/**
 * @brief Name a resource kind as in the PerformanceResource.kind enum of the contract.
 * @param kind Resource kind.
 * @return "cpu", "memory", "disk", "ethernet", "wifi" or "gpu".
 */
const char* toString(ResourceKind kind)
{
    switch (kind)
    {
    case ResourceKind::Cpu:
        return "cpu";
    case ResourceKind::Memory:
        return "memory";
    case ResourceKind::Disk:
        return "disk";
    case ResourceKind::Ethernet:
        return "ethernet";
    case ResourceKind::Wifi:
        return "wifi";
    case ResourceKind::Gpu:
        return "gpu";
    }
    throw std::logic_error("Unhandled ResourceKind");
}

/**
 * @brief Name a virtualization state as in the CpuInfo.virtualization enum of the contract.
 * @param state Virtualization state.
 * @return "enabled", "disabled" or "unknown".
 */
const char* toString(providers::VirtualizationState state)
{
    switch (state)
    {
    case providers::VirtualizationState::Enabled:
        return "enabled";
    case providers::VirtualizationState::Disabled:
        return "disabled";
    case providers::VirtualizationState::Unknown:
        return "unknown";
    }
    throw std::logic_error("Unhandled VirtualizationState");
}

/**
 * @brief Serialize one resource as the PerformanceResource schema.
 * @param resource Service result.
 * @return JSON object.
 */
nlohmann::json toJson(const ResourceSummary& resource)
{
    return {
        {"id", resource.id},
        {"kind", toString(resource.kind)},
        {"name", resource.name},
        {"description", nullable(resource.description)},
        {"percent", nullable(resource.percent)},
        {"history", resource.history},
        {"frequencyMhz", nullable(resource.frequencyMhz)},
        {"usedBytes", nullable(resource.usedBytes)},
        {"totalBytes", nullable(resource.totalBytes)},
        {"sendBitsPerSecond", nullable(resource.sendBitsPerSecond)},
        {"receiveBitsPerSecond", nullable(resource.receiveBitsPerSecond)},
        {"temperatureCelsius", nullable(resource.temperatureCelsius)},
    };
}

/**
 * @brief Serialize the CPU screen data as the CpuPerformance schema.
 * @param performance Service result.
 * @return JSON object.
 */
nlohmann::json toJson(const CpuPerformance& performance)
{
    const providers::CpuStaticInfo& info = performance.info;
    const services::monitoring::CpuMetrics& cpu = performance.cpu;

    nlohmann::json topProcesses = nlohmann::json::array();
    for (const services::monitoring::ProcessMetrics& process : performance.topProcesses)
    {
        topProcesses.push_back({
            {"pid", process.pid},
            {"name", process.name},
            {"cpuPercent", process.cpuPercent},
            {"memoryBytes", process.memoryBytes},
            {"isSelf", process.isSelf},
        });
    }

    return {
        {"info",
         {
             {"model", info.model},
             {"sockets", info.sockets},
             {"cores", info.cores},
             {"logicalProcessors", info.logicalProcessors},
             {"baseSpeedMhz", nullable(info.baseSpeedMhz)},
             {"virtualization", toString(info.virtualization)},
             {"l1CacheBytes", nullable(info.l1CacheBytes)},
             {"l2CacheBytes", nullable(info.l2CacheBytes)},
             {"l3CacheBytes", nullable(info.l3CacheBytes)},
         }},
        {"usagePercent", cpu.usagePercent},
        {"frequencyMhz", nullable(cpu.frequencyMhz)},
        {"frequencyPercent", nullable(cpu.frequencyPercent)},
        {"temperatureCelsius", nullable(cpu.temperatureCelsius)},
        {"processCount", performance.processCount},
        {"threadCount", performance.threadCount},
        {"handleCount", nullable(performance.handleCount)},
        {"bootTime", toRfc3339(performance.bootTime)},
        {"uptimeSeconds", performance.uptime.count()},
        {"history",
         {
             {"usagePercent", cpu.usageHistory},
             {"frequencyPercent", nullableArray(cpu.frequencyPercentHistory)},
             {"temperatureCelsius", nullableArray(cpu.temperatureHistory)},
         }},
        {"topProcesses", std::move(topProcesses)},
        {"timeBreakdown",
         {
             {"userPercent", cpu.userPercent},
             {"kernelPercent", cpu.kernelPercent},
             {"dpcPercent", cpu.dpcPercent},
             {"interruptPercent", cpu.interruptPercent},
             {"idlePercent", cpu.idlePercent},
             {"history", {{"userPercent", cpu.userHistory}, {"kernelPercent", cpu.kernelHistory}}},
         }},
        {"scheduler",
         {
             {"contextSwitchesPerSecond", nullable(cpu.contextSwitchesPerSecond)},
             {"interruptsPerSecond", nullable(cpu.interruptsPerSecond)},
             {"systemCallsPerSecond", nullable(cpu.systemCallsPerSecond)},
             {"dpcsPerSecond", nullable(cpu.dpcsPerSecond)},
             {"source", info.activitySource},
         }},
    };
}

} // namespace

/**
 * @brief Create the controller.
 * @param service Provides the performance data. Must not be null.
 * @throws std::invalid_argument if service is null.
 */
PerformanceController::PerformanceController(std::shared_ptr<const services::performance::IPerformanceService> service)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        throw std::invalid_argument("PerformanceController: service must not be null");
    }
}

/**
 * @brief Register GET kResourcesRoute and GET kCpuRoute.
 * @param registry Target registry.
 */
void PerformanceController::registerRoutes(IRouteRegistry& registry)
{
    registry.addRoute(HttpMethod::Get, kResourcesRoute,
                      [this](const Request& request) { return handleGetResources(request); });
    registry.addRoute(HttpMethod::Get, kCpuRoute, [this](const Request& request) { return handleGetCpu(request); });
}

/**
 * @brief Handle GET kResourcesRoute.
 * @param request Incoming request (unused: the endpoint takes no input).
 * @return 200 with a PerformanceResources object.
 * @throws std::runtime_error if no metrics are available (becomes a 500 problem).
 */
Response PerformanceController::handleGetResources(const Request& /*request*/) const
{
    nlohmann::json resources = nlohmann::json::array();
    for (const ResourceSummary& resource : m_service->getResources())
    {
        resources.push_back(toJson(resource));
    }
    return ok({{"resources", std::move(resources)}});
}

/**
 * @brief Handle GET kCpuRoute.
 * @param request Incoming request (unused: the endpoint takes no input).
 * @return 200 with a CpuPerformance object.
 * @throws std::runtime_error if no metrics are available (becomes a 500 problem).
 */
Response PerformanceController::handleGetCpu(const Request& /*request*/) const
{
    return ok(toJson(m_service->getCpuPerformance()));
}

} // namespace systemlens::api::performance
