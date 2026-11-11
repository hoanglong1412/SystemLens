/**
 * @file    MetricsCollector.cpp
 * @brief   Implementation of MetricsCollector.
 */
#include "services/monitoring/MetricsCollector.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace systemlens::services::monitoring {

namespace {

constexpr double kMaxPercent = 100.0;
constexpr double kBitsPerByte = 8.0;
/** PID of init on Unix; PIDs up to this value never identify the SystemLens UI. */
constexpr std::uint32_t kInitPid = 1;

/**
 * @brief Express a part as a percentage of a whole.
 * @param part  Numerator.
 * @param whole Denominator.
 * @return part / whole * 100 clamped to 0..100; 0 when whole is not positive.
 */
double toPercent(double part, double whole)
{
    if (whole <= 0.0)
    {
        return 0.0;
    }
    return std::clamp(part / whole * kMaxPercent, 0.0, kMaxPercent);
}

/**
 * @brief Turn two readings of a cumulative counter into a per-second rate.
 * @param previous Earlier reading.
 * @param current  Later reading.
 * @param elapsed  Time between the readings.
 * @return Rate, or std::nullopt when a reading is missing, time did not advance, or the
 *         counter went backwards (reset / wrap-around).
 */
std::optional<double> toRate(const std::optional<std::uint64_t>& previous,
                             const std::optional<std::uint64_t>& current,
                             std::chrono::duration<double> elapsed)
{
    if (!previous || !current || *current < *previous || elapsed.count() <= 0.0)
    {
        return std::nullopt;
    }
    return static_cast<double>(*current - *previous) / elapsed.count();
}

/**
 * @brief CPU usage between two readings of cumulative CPU times.
 * @param before Earlier reading.
 * @param now    Later reading.
 * @return 100 - idle share, in 0..100; 0 when no time was recorded between the readings.
 */
double usageBetween(const providers::CpuTimes& before, const providers::CpuTimes& now)
{
    const double idle = (now.idle - before.idle).count();
    const double total = idle + (now.user - before.user).count() + (now.kernel - before.kernel).count()
                         + (now.dpc - before.dpc).count() + (now.interrupt - before.interrupt).count();
    return total > 0.0 ? kMaxPercent - toPercent(idle, total) : 0.0;
}

} // namespace

/**
 * @brief Create the collector. Nothing is read until start().
 * @param providers      Platform providers; every pointer must be non-null.
 * @param sampleInterval Time between ticks.
 * @throws std::invalid_argument if a provider is null or the interval is not positive.
 */
MetricsCollector::MetricsCollector(providers::PlatformProviders providers, std::chrono::milliseconds sampleInterval)
    : m_providers(std::move(providers))
    , m_sampleInterval(sampleInterval)
{
    if (!m_providers.systemInfo || !m_providers.cpu || !m_providers.memory || !m_providers.disk
        || !m_providers.network || !m_providers.gpu || !m_providers.process)
    {
        throw std::invalid_argument("MetricsCollector: providers must not be null");
    }
    if (m_sampleInterval <= std::chrono::milliseconds::zero())
    {
        throw std::invalid_argument("MetricsCollector: sample interval must be positive");
    }
}

/** @brief Stop the sampling thread. */
MetricsCollector::~MetricsCollector()
{
    stop();
}

/**
 * @brief Take the first sample on the calling thread, then keep sampling on a background thread.
 *
 * Sampling once synchronously guarantees latest() has a result (or an error) as soon as
 * start() returns. Calling start() twice has no effect.
 */
void MetricsCollector::start()
{
    if (m_thread.joinable())
    {
        return;
    }
    tick();
    m_thread = std::jthread([this](std::stop_token stopToken) { run(std::move(stopToken)); });
}

/** @brief Stop the background thread and wait for it. Safe to call more than once. */
void MetricsCollector::stop()
{
    if (m_thread.joinable())
    {
        m_thread.request_stop();
        m_thread.join();
    }
}

/**
 * @brief Get the result of the most recent tick (see IMetricsSource).
 * @return Latest snapshot.
 * @throws std::runtime_error if the most recent tick failed or start() was never called.
 */
std::shared_ptr<const MetricsSnapshot> MetricsCollector::latest() const
{
    const std::scoped_lock lock(m_mutex);
    if (!m_lastError.empty())
    {
        throw std::runtime_error("Metrics sampling failed: " + m_lastError);
    }
    if (!m_latest)
    {
        throw std::runtime_error("Metrics sampling has not started");
    }
    return m_latest;
}

/**
 * @brief Body of the sampling thread: tick at a fixed cadence until stop is requested.
 * @param stopToken Stop request of the owning jthread.
 */
void MetricsCollector::run(std::stop_token stopToken)
{
    // Deadlines advance by a fixed step so the duration of a tick does not accumulate as drift.
    auto deadline = std::chrono::steady_clock::now() + m_sampleInterval;
    std::unique_lock lock(m_wakeUpMutex);
    while (!stopToken.stop_requested())
    {
        // The predicate is never true: the wait ends at the deadline or when stop is requested.
        m_wakeUp.wait_until(lock, stopToken, deadline, [] { return false; });
        if (stopToken.stop_requested())
        {
            break;
        }
        tick();
        deadline = std::max(deadline + m_sampleInterval, std::chrono::steady_clock::now());
    }
}

/**
 * @brief Take one sample and publish it; on failure publish the error instead.
 *
 * A failing provider must not take the backend down: the error is logged once per
 * failure streak and reported by latest() until a tick succeeds again.
 */
void MetricsCollector::tick()
{
    try
    {
        auto snapshot = std::make_shared<const MetricsSnapshot>(sample());
        const std::scoped_lock lock(m_mutex);
        m_latest = std::move(snapshot);
        m_lastError.clear();
        m_failureLogged = false;
    }
    catch (const std::exception& e)
    {
        if (!m_failureLogged)
        {
            // stdout is reserved for the READY handshake, so logs go to stderr.
            std::cerr << "[MetricsCollector] sampling failed: " << e.what() << std::endl;
            m_failureLogged = true;
        }
        const std::scoped_lock lock(m_mutex);
        m_lastError = e.what();
    }
}

/**
 * @brief Read every provider once and build the snapshot.
 * @return New snapshot. Rates are zero / nullopt on the very first tick (no previous sample yet).
 * @throws providers::ProviderError (or any std::exception) if a provider fails.
 */
MetricsSnapshot MetricsCollector::sample()
{
    if (!m_static)
    {
        m_static = StaticData{
            .identity = m_providers.systemInfo->readIdentity(),
            .bootTime = m_providers.systemInfo->readBootTime(),
            .cpuInfo = m_providers.cpu->readStaticInfo(),
        };
    }

    m_current = PreviousSample{};
    m_current.takenAt = std::chrono::steady_clock::now();
    m_current.cpu = m_providers.cpu->readSample();
    const Seconds elapsed = m_previous ? Seconds(m_current.takenAt - m_previous->takenAt) : Seconds::zero();

    MetricsSnapshot snapshot;
    snapshot.takenAt = std::chrono::system_clock::now();
    snapshot.sampleInterval = m_sampleInterval;
    snapshot.identity = m_static->identity;
    snapshot.bootTime = m_static->bootTime;
    snapshot.cpuInfo = m_static->cpuInfo;
    snapshot.counts = m_providers.systemInfo->readCounts();

    sampleCpu(m_current.cpu, elapsed, snapshot);
    sampleMemory(snapshot);
    sampleDisks(elapsed, snapshot);
    sampleNetworks(elapsed, snapshot);
    sampleGpus(snapshot);
    sampleProcesses(elapsed, snapshot);

    // Only a fully successful tick becomes the baseline of the next one.
    m_previous = std::move(m_current);
    return snapshot;
}

/**
 * @brief Compute CPU usage, time split, clock, temperature and scheduler rates.
 * @param current  CPU sample of this tick.
 * @param elapsed  Time since the previous tick (zero on the first tick).
 * @param snapshot Receives snapshot.cpu.
 */
void MetricsCollector::sampleCpu(const providers::CpuSample& current, Seconds elapsed, MetricsSnapshot& snapshot)
{
    CpuMetrics& cpu = snapshot.cpu;

    if (m_previous)
    {
        const providers::CpuTimes& before = m_previous->cpu.times;
        const providers::CpuTimes& now = current.times;
        const double user = (now.user - before.user).count();
        const double kernel = (now.kernel - before.kernel).count();
        const double dpc = (now.dpc - before.dpc).count();
        const double interrupt = (now.interrupt - before.interrupt).count();
        const double idle = (now.idle - before.idle).count();
        const double total = user + kernel + dpc + interrupt + idle;
        if (total > 0.0)
        {
            cpu.userPercent = toPercent(user, total);
            cpu.kernelPercent = toPercent(kernel, total);
            cpu.dpcPercent = toPercent(dpc, total);
            cpu.interruptPercent = toPercent(interrupt, total);
            cpu.idlePercent = toPercent(idle, total);
            cpu.usagePercent = kMaxPercent - cpu.idlePercent;
        }

        const providers::CpuActivityCounters& previous = m_previous->cpu.activity;
        cpu.contextSwitchesPerSecond = toRate(previous.contextSwitches, current.activity.contextSwitches, elapsed);
        cpu.interruptsPerSecond = toRate(previous.interrupts, current.activity.interrupts, elapsed);
        cpu.systemCallsPerSecond = toRate(previous.systemCalls, current.activity.systemCalls, elapsed);
        cpu.dpcsPerSecond = toRate(previous.dpcs, current.activity.dpcs, elapsed);
    }

    // Per-core usage needs the same number of cores in both readings (it changes only with CPU hot-plug).
    cpu.coreUsagePercent.assign(current.perCore.size(), 0.0);
    if (m_previous && m_previous->cpu.perCore.size() == current.perCore.size())
    {
        for (std::size_t core = 0; core < current.perCore.size(); ++core)
        {
            cpu.coreUsagePercent[core] = usageBetween(m_previous->cpu.perCore[core], current.perCore[core]);
        }
    }
    if (m_coreHistories.size() != current.perCore.size())
    {
        m_coreHistories.assign(current.perCore.size(), History<double>(kHistoryCapacity));
    }
    cpu.coreUsageHistory.reserve(m_coreHistories.size());
    for (std::size_t core = 0; core < m_coreHistories.size(); ++core)
    {
        m_coreHistories[core].push(cpu.coreUsagePercent[core]);
        cpu.coreUsageHistory.push_back(m_coreHistories[core].toVector());
    }

    cpu.frequencyMhz = current.frequencyMhz;
    cpu.temperatureCelsius = current.temperatureCelsius;
    if (cpu.frequencyMhz)
    {
        // The OS may not know the turbo ceiling, so the scale also learns from what is observed.
        m_highestFrequencyMhz = std::max({m_highestFrequencyMhz, *cpu.frequencyMhz,
                                          snapshot.cpuInfo.maxSpeedMhz.value_or(0.0),
                                          snapshot.cpuInfo.baseSpeedMhz.value_or(0.0)});
        cpu.frequencyPercent = toPercent(*cpu.frequencyMhz, m_highestFrequencyMhz);
    }

    m_cpuHistories.usage.push(cpu.usagePercent);
    m_cpuHistories.user.push(cpu.userPercent);
    m_cpuHistories.kernel.push(std::min(cpu.kernelPercent + cpu.dpcPercent + cpu.interruptPercent, kMaxPercent));
    m_cpuHistories.frequencyPercent.push(cpu.frequencyPercent);
    m_cpuHistories.temperature.push(cpu.temperatureCelsius);

    cpu.usageHistory = m_cpuHistories.usage.toVector();
    cpu.userHistory = m_cpuHistories.user.toVector();
    cpu.kernelHistory = m_cpuHistories.kernel.toVector();
    cpu.frequencyPercentHistory = m_cpuHistories.frequencyPercent.toVector();
    cpu.temperatureHistory = m_cpuHistories.temperature.toVector();
}

/**
 * @brief Read memory usage.
 * @param snapshot Receives snapshot.memory.
 */
void MetricsCollector::sampleMemory(MetricsSnapshot& snapshot)
{
    const providers::MemoryUsage usage = m_providers.memory->readUsage();
    MemoryMetrics& memory = snapshot.memory;
    memory.totalBytes = usage.totalBytes;
    memory.usedBytes = usage.usedBytes;
    memory.availableBytes = usage.availableBytes;
    memory.cachedBytes = usage.cachedBytes;
    memory.committedBytes = usage.committedBytes;
    memory.commitLimitBytes = usage.commitLimitBytes;
    memory.usedPercent = toPercent(static_cast<double>(usage.usedBytes), static_cast<double>(usage.totalBytes));
    m_memoryHistory.push(memory.usedPercent);
    memory.usedPercentHistory = m_memoryHistory.toVector();
}

/**
 * @brief Compute the active time and the read / write rates of every disk.
 * @param elapsed  Time since the previous tick.
 * @param snapshot Receives snapshot.disks.
 */
void MetricsCollector::sampleDisks(Seconds elapsed, MetricsSnapshot& snapshot)
{
    for (providers::DiskSample& disk : m_providers.disk->readDisks())
    {
        DiskMetrics metrics;
        metrics.id = disk.id;
        metrics.name = std::move(disk.name);
        metrics.model = std::move(disk.model);
        metrics.kind = disk.kind;
        metrics.capacityBytes = disk.capacityBytes;
        if (disk.busyTime)
        {
            m_current.diskBusyTimes.emplace(disk.id, *disk.busyTime);
            if (m_previous)
            {
                const auto before = m_previous->diskBusyTimes.find(disk.id);
                if (before != m_previous->diskBusyTimes.end())
                {
                    metrics.activePercent = toPercent((*disk.busyTime - before->second).count(), elapsed.count());
                }
            }
        }
        if (disk.bytesRead && disk.bytesWritten)
        {
            m_current.diskIo.emplace(disk.id, DiskIo{*disk.bytesRead, *disk.bytesWritten});
            if (m_previous)
            {
                const auto before = m_previous->diskIo.find(disk.id);
                if (before != m_previous->diskIo.end())
                {
                    metrics.readBytesPerSecond = toRate(before->second.bytesRead, disk.bytesRead, elapsed);
                    metrics.writeBytesPerSecond = toRate(before->second.bytesWritten, disk.bytesWritten, elapsed);
                }
            }
        }
        History<double>& active = historyFor(disk.id);
        active.push(metrics.activePercent.value_or(0.0));
        metrics.activePercentHistory = active.toVector();
        History<double>& reads = historyFor(disk.id + "/read");
        reads.push(metrics.readBytesPerSecond.value_or(0.0));
        metrics.readHistory = reads.toVector();
        History<double>& writes = historyFor(disk.id + "/write");
        writes.push(metrics.writeBytesPerSecond.value_or(0.0));
        metrics.writeHistory = writes.toVector();
        snapshot.disks.push_back(std::move(metrics));
    }
}

/**
 * @brief Compute the send / receive rate of every network adapter.
 * @param elapsed  Time since the previous tick.
 * @param snapshot Receives snapshot.networks.
 */
void MetricsCollector::sampleNetworks(Seconds elapsed, MetricsSnapshot& snapshot)
{
    for (providers::NetworkSample& adapter : m_providers.network->readInterfaces())
    {
        NetworkMetrics metrics;
        metrics.id = adapter.id;
        metrics.name = adapter.name;
        metrics.description = adapter.description;
        metrics.macAddress = adapter.macAddress;
        metrics.kind = adapter.kind;
        metrics.linkSpeedBitsPerSecond = adapter.linkSpeedBitsPerSecond;
        if (m_previous)
        {
            const auto before = m_previous->networks.find(adapter.id);
            if (before != m_previous->networks.end())
            {
                const auto bitRate = [&](std::uint64_t previousBytes, std::uint64_t currentBytes) {
                    return toRate(previousBytes, currentBytes, elapsed).value_or(0.0) * kBitsPerByte;
                };
                metrics.sendBitsPerSecond = bitRate(before->second.bytesSent, adapter.bytesSent);
                metrics.receiveBitsPerSecond = bitRate(before->second.bytesReceived, adapter.bytesReceived);
            }
        }
        History<double>& total = historyFor(adapter.id);
        total.push(metrics.sendBitsPerSecond + metrics.receiveBitsPerSecond);
        metrics.throughputHistory = total.toVector();
        History<double>& sent = historyFor(adapter.id + "/send");
        sent.push(metrics.sendBitsPerSecond);
        metrics.sendHistory = sent.toVector();
        History<double>& received = historyFor(adapter.id + "/receive");
        received.push(metrics.receiveBitsPerSecond);
        metrics.receiveHistory = received.toVector();
        snapshot.networks.push_back(std::move(metrics));
        std::string id = adapter.id;
        m_current.networks.emplace(std::move(id), std::move(adapter));
    }
}

/**
 * @brief Read the graphics adapters.
 * @param snapshot Receives snapshot.gpus.
 */
void MetricsCollector::sampleGpus(MetricsSnapshot& snapshot)
{
    for (providers::GpuSample& gpu : m_providers.gpu->readGpus())
    {
        GpuMetrics metrics;
        metrics.id = std::move(gpu.id);
        metrics.name = std::move(gpu.name);
        metrics.model = std::move(gpu.model);
        metrics.utilizationPercent = gpu.utilizationPercent;
        metrics.temperatureCelsius = gpu.temperatureCelsius;
        metrics.memoryTotalBytes = gpu.memoryTotalBytes;
        metrics.memoryUsedBytes = gpu.memoryUsedBytes;
        History<double>& history = historyFor(metrics.id);
        history.push(metrics.utilizationPercent.value_or(0.0));
        metrics.utilizationHistory = history.toVector();
        snapshot.gpus.push_back(std::move(metrics));
    }
}

/**
 * @brief Compute per-process CPU usage, the process lists and the per-application totals.
 * @param elapsed  Time since the previous tick.
 * @param snapshot Receives processes, topCpuProcesses, topMemoryProcess and appHistory.
 */
void MetricsCollector::sampleProcesses(Seconds elapsed, MetricsSnapshot& snapshot)
{
    std::vector<providers::ProcessSample> processes = m_providers.process->readProcesses();
    const std::uint32_t selfPid = m_providers.process->currentProcessId();
    const auto self = std::ranges::find(processes, selfPid, &providers::ProcessSample::pid);
    // The parent is the SystemLens UI when it started us. An orphaned backend is re-parented to
    // init (PID 1) or has no parent (PID 0); neither of those is "self".
    const std::optional<std::uint32_t> parentPid = (self != processes.end() && self->parentPid > kInitPid)
                                                       ? std::optional(self->parentPid)
                                                       : std::nullopt;
    // CPU time is summed over all processors, so 100% means every logical processor fully busy.
    const double capacity = elapsed.count() * std::max<std::uint32_t>(snapshot.cpuInfo.logicalProcessors, 1);

    // Instance counts and memory are "now" values: reset them before the processes are counted again.
    for (auto& [name, app] : m_apps)
    {
        app.runningInstances = 0;
        app.memoryBytes = 0;
    }

    std::vector<ProcessMetrics>& all = snapshot.processes;
    all.reserve(processes.size());
    m_current.processCpuTimes.reserve(processes.size());
    for (providers::ProcessSample& process : processes)
    {
        m_current.processCpuTimes.emplace(process.pid, process.cpuTime);
        ProcessMetrics metrics;
        metrics.pid = process.pid;
        metrics.parentPid = process.parentPid;
        metrics.cpuTimeSeconds = process.cpuTime.count();
        metrics.memoryBytes = process.memoryBytes;
        metrics.threadCount = process.threadCount;
        metrics.isSelf = process.pid == selfPid || (parentPid && process.pid == *parentPid);

        AppAccumulator& app = m_apps[process.name];
        ++app.runningInstances;
        app.memoryBytes += process.memoryBytes;
        if (m_previous)
        {
            // A PID missing from the previous tick is a new process: no baseline, so 0 for now.
            const auto before = m_previous->processCpuTimes.find(process.pid);
            if (before != m_previous->processCpuTimes.end())
            {
                const double delta = (process.cpuTime - before->second).count();
                metrics.cpuPercent = toPercent(delta, capacity);
                // A negative delta means the PID was reused by another process; ignore it.
                app.cpuTimeSeconds += std::max(delta, 0.0);
            }
        }

        metrics.name = std::move(process.name);
        metrics.userName = std::move(process.userName);
        metrics.status = std::move(process.status);
        metrics.commandLine = std::move(process.commandLine);
        all.push_back(std::move(metrics));
    }
    for (auto& [name, app] : m_apps)
    {
        app.peakMemoryBytes = std::max(app.peakMemoryBytes, app.memoryBytes);
    }

    if (!all.empty())
    {
        snapshot.topMemoryProcess = *std::ranges::max_element(all, {}, &ProcessMetrics::memoryBytes);
        // Ties (typically many processes at 0%) are ordered by memory so the table does not shuffle.
        std::ranges::sort(all, [](const ProcessMetrics& a, const ProcessMetrics& b) {
            return a.cpuPercent != b.cpuPercent ? a.cpuPercent > b.cpuPercent : a.memoryBytes > b.memoryBytes;
        });
        const auto topCount = static_cast<std::ptrdiff_t>(std::min(kTopProcessCount, all.size()));
        snapshot.topCpuProcesses.assign(all.begin(), all.begin() + topCount);
    }

    snapshot.appHistorySince = m_appsSince;
    snapshot.appHistory.reserve(m_apps.size());
    for (const auto& [name, app] : m_apps)
    {
        snapshot.appHistory.push_back(AppUsage{
            .name = name,
            .cpuTimeSeconds = app.cpuTimeSeconds,
            .peakMemoryBytes = app.peakMemoryBytes,
            .memoryBytes = app.memoryBytes,
            .runningInstances = app.runningInstances,
        });
    }
    std::ranges::sort(snapshot.appHistory, [](const AppUsage& a, const AppUsage& b) {
        return a.cpuTimeSeconds != b.cpuTimeSeconds ? a.cpuTimeSeconds > b.cpuTimeSeconds : a.name < b.name;
    });
    if (snapshot.appHistory.size() > kAppHistoryLimit)
    {
        snapshot.appHistory.resize(kAppHistoryLimit);
    }
}

/**
 * @brief Get (creating on first use) the history of a disk, adapter or GPU.
 * @param id Device identifier.
 * @return History that persists across ticks.
 */
History<double>& MetricsCollector::historyFor(const std::string& id)
{
    return m_deviceHistories.try_emplace(id, kHistoryCapacity).first->second;
}

} // namespace systemlens::services::monitoring
