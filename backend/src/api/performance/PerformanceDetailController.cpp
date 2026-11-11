/**
 * @file    PerformanceDetailController.cpp
 * @brief   Implementation of PerformanceDetailController.
 */
#include "api/performance/PerformanceDetailController.h"

#include "api/ApiResponse.h"
#include "api/JsonHelpers.h"

#include <stdexcept>

namespace systemlens::api::performance {

namespace {

using nlohmann::json;
namespace monitoring = services::monitoring;

/**
 * @brief Name a disk kind as in the DiskDetail.kind enum of the contract.
 * @param kind Disk kind.
 * @return "ssd", "hdd" or "unknown".
 */
const char* toString(providers::DiskKind kind)
{
    switch (kind)
    {
    case providers::DiskKind::Ssd:
        return "ssd";
    case providers::DiskKind::Hdd:
        return "hdd";
    case providers::DiskKind::Unknown:
        break;
    }
    return "unknown";
}

/**
 * @param text Possibly empty text.
 * @return The text, or JSON null when empty.
 */
json nullIfEmpty(const std::string& text)
{
    return text.empty() ? json(nullptr) : json(text);
}

/** @brief Serialize a process as the CpuProcess schema. */
json toJson(const monitoring::ProcessMetrics& process)
{
    return {
        {"pid", process.pid},
        {"name", process.name},
        {"cpuPercent", process.cpuPercent},
        {"memoryBytes", process.memoryBytes},
        {"isSelf", process.isSelf},
    };
}

/** @brief Serialize a disk as the DiskDetail schema. */
json toJson(const monitoring::DiskMetrics& disk)
{
    return {
        {"id", disk.id},
        {"name", disk.name},
        {"model", nullIfEmpty(disk.model)},
        {"kind", toString(disk.kind)},
        {"capacityBytes", nullable(disk.capacityBytes)},
        {"activePercent", nullable(disk.activePercent)},
        {"readBytesPerSecond", nullable(disk.readBytesPerSecond)},
        {"writeBytesPerSecond", nullable(disk.writeBytesPerSecond)},
        {"activeHistory", disk.activePercentHistory},
        {"readHistory", disk.readHistory},
        {"writeHistory", disk.writeHistory},
    };
}

/** @brief Serialize an adapter as the NetworkDetail schema. */
json toJson(const monitoring::NetworkMetrics& adapter)
{
    return {
        {"id", adapter.id},
        {"name", adapter.name},
        {"kind", adapter.kind == providers::NetworkKind::Wifi ? "wifi" : "ethernet"},
        {"description", nullIfEmpty(adapter.description)},
        {"macAddress", nullIfEmpty(adapter.macAddress)},
        {"linkSpeedBitsPerSecond", nullable(adapter.linkSpeedBitsPerSecond)},
        {"sendBitsPerSecond", adapter.sendBitsPerSecond},
        {"receiveBitsPerSecond", adapter.receiveBitsPerSecond},
        {"sendHistory", adapter.sendHistory},
        {"receiveHistory", adapter.receiveHistory},
    };
}

/** @brief Serialize a graphics adapter as the GpuDetail schema. */
json toJson(const monitoring::GpuMetrics& gpu)
{
    return {
        {"id", gpu.id},
        {"name", gpu.name},
        {"model", gpu.model},
        {"utilizationPercent", nullable(gpu.utilizationPercent)},
        {"temperatureCelsius", nullable(gpu.temperatureCelsius)},
        {"memoryTotalBytes", nullable(gpu.memoryTotalBytes)},
        {"memoryUsedBytes", nullable(gpu.memoryUsedBytes)},
        {"history", gpu.utilizationHistory},
    };
}

/**
 * @brief Serialize a list as {"<key>": [...]}.
 * @param key   Name of the array property.
 * @param items Items convertible with toJson().
 * @return JSON object.
 */
template <typename Item>
json toListJson(const char* key, const std::vector<Item>& items)
{
    json array = json::array();
    for (const Item& item : items)
    {
        array.push_back(toJson(item));
    }
    return {{key, std::move(array)}};
}

} // namespace

/**
 * @brief Create the controller.
 * @param service Provides the data. Must not be null.
 * @throws std::invalid_argument if service is null.
 */
PerformanceDetailController::PerformanceDetailController(
    std::shared_ptr<const services::performance::IPerformanceDetailService> service)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        throw std::invalid_argument("PerformanceDetailController: service must not be null");
    }
}

/**
 * @brief Register the five GET routes.
 *
 * Every handler takes no input; a service exception (no metrics available) becomes a 500 problem.
 * @param registry Target registry.
 */
void PerformanceDetailController::registerRoutes(IRouteRegistry& registry)
{
    registry.addRoute(HttpMethod::Get, kCoresRoute, [this](const Request&) {
        json cores = json::array();
        for (const auto& core : m_service->getCores())
        {
            cores.push_back({{"index", core.index}, {"usagePercent", core.usagePercent}, {"history", core.history}});
        }
        return ok({{"cores", std::move(cores)}});
    });

    registry.addRoute(HttpMethod::Get, kMemoryRoute, [this](const Request&) {
        const auto detail = m_service->getMemory();
        const monitoring::MemoryMetrics& memory = detail.memory;
        json topProcesses = json::array();
        for (const auto& process : detail.topProcesses)
        {
            topProcesses.push_back(toJson(process));
        }
        return ok({
            {"totalBytes", memory.totalBytes},
            {"usedBytes", memory.usedBytes},
            {"availableBytes", memory.availableBytes},
            {"cachedBytes", nullable(memory.cachedBytes)},
            {"committedBytes", nullable(memory.committedBytes)},
            {"commitLimitBytes", nullable(memory.commitLimitBytes)},
            {"usedPercent", memory.usedPercent},
            {"history", memory.usedPercentHistory},
            {"topProcesses", std::move(topProcesses)},
        });
    });

    registry.addRoute(HttpMethod::Get, kDisksRoute,
                      [this](const Request&) { return ok(toListJson("disks", m_service->getDisks())); });
    registry.addRoute(HttpMethod::Get, kNetworksRoute,
                      [this](const Request&) { return ok(toListJson("adapters", m_service->getNetworks())); });
    registry.addRoute(HttpMethod::Get, kGpusRoute,
                      [this](const Request&) { return ok(toListJson("gpus", m_service->getGpus())); });
}

} // namespace systemlens::api::performance
