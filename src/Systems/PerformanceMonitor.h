#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "Logger.h"

class PerformanceMonitor
{
public:
    struct BlockStatsSnapshot
    {
        double lastBlockMs = 0.0;
        double averageBlockMs = 0.0;
    };

    explicit PerformanceMonitor (Logger& loggerIn) : logger (loggerIn) {}

    PerformanceMonitor (const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator= (const PerformanceMonitor&) = delete;
    PerformanceMonitor (PerformanceMonitor&&) = delete;
    PerformanceMonitor& operator= (PerformanceMonitor&&) = delete;

    void beginSession()
    {
        running.store (true, std::memory_order_release);
        blockCount.store (0, std::memory_order_release);
        avgBlockMs.store (0.0, std::memory_order_release);
    }

    void endSession()
    {
        running.store (false, std::memory_order_release);
    }

    void beginBlock()
    {
        blockStartTime = juce::Time::getMillisecondCounterHiRes();
    }

    void endBlock (int /*samples*/)
    {
        const auto now = juce::Time::getMillisecondCounterHiRes();
        const auto ms = now - blockStartTime;

        lastBlockMs.store (ms, std::memory_order_relaxed);

        auto n = blockCount.fetch_add (1, std::memory_order_relaxed) + 1;
        const auto prevAvg = avgBlockMs.load (std::memory_order_relaxed);
        const auto newAvg = prevAvg + (ms - prevAvg) / (double) n;
        avgBlockMs.store (newAvg, std::memory_order_relaxed);
    }

    BlockStatsSnapshot getSnapshot() const
    {
        BlockStatsSnapshot s;
        s.lastBlockMs = lastBlockMs.load (std::memory_order_relaxed);
        s.averageBlockMs = avgBlockMs.load (std::memory_order_relaxed);
        return s;
    }

private:
    Logger& logger;
    std::atomic<bool> running { false };
    std::atomic<double> lastBlockMs { 0.0 };
    std::atomic<double> avgBlockMs { 0.0 };
    std::atomic<int> blockCount { 0 };
    double blockStartTime = 0.0;
};
