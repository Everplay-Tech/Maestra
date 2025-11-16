#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <mutex>
#include <vector>

class Logger
{
public:
    enum class LogLevel { Debug, Info, Warning, Error };

    struct LogEntry
    {
        juce::Time time;
        LogLevel level;
        juce::String message;
    };

    Logger() = default;

    Logger (const Logger&) = delete;
    Logger& operator= (const Logger&) = delete;
    Logger (Logger&&) = delete;
    Logger& operator= (Logger&&) = delete;

    void log (LogLevel level, const juce::String& message)
    {
        std::lock_guard<std::mutex> lock (mutex);
        entries.push_back ({ juce::Time::getCurrentTime(), level, message });
        totalCount.fetch_add (1, std::memory_order_relaxed);

        juce::Logger::outputDebugString (message);
    }

    std::vector<Logger::LogEntry> getSnapshot() const
    {
        std::lock_guard<std::mutex> lock (mutex);
        return entries;
    }

    int getTotalCount() const
    {
        return totalCount.load (std::memory_order_relaxed);
    }

private:
    mutable std::mutex mutex;
    std::vector<Logger::LogEntry> entries;
    std::atomic<int> totalCount { 0 };
};
