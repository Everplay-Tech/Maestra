#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "Logger.h"

class CrashReporter
{
public:
    struct CrashReport
    {
        juce::String message;
        juce::String stackTrace;
        juce::Time   time;
    };

    explicit CrashReporter (Logger& loggerIn) : logger (loggerIn) {}

    CrashReporter (const CrashReporter&) = delete;
    CrashReporter& operator= (const CrashReporter&) = delete;
    CrashReporter (CrashReporter&&) = delete;
    CrashReporter& operator= (CrashReporter&&) = delete;

    void installGlobalHandler()
    {
        installed.store (true, std::memory_order_release);
    }

    void uninstallGlobalHandler()
    {
        installed.store (false, std::memory_order_release);
    }

    void submitCrashReport (const CrashReporter::CrashReport& report)
    {
        logger.log (Logger::LogLevel::Error, "Crash: " + report.message);
        lastReportMessage = report.message;
    }

    juce::String getLastReportMessage() const { return lastReportMessage; }

private:
    Logger& logger;
    std::atomic<bool> installed { false };
    juce::String lastReportMessage;
};
