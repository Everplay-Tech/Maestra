#pragma once

#include <memory>
#include <atomic>

#ifdef __OBJC__
@class AVAudioEngine;
@class AVAudioInputNode;
@class AVAudioOutputNode;
#endif

class AVAudioEngineManager
{
public:
    struct RenderStatsSnapshot
    {
        bool running = false;
        double lastLatencyMs = 0.0;
    };

    AVAudioEngineManager();
    ~AVAudioEngineManager();

    AVAudioEngineManager (const AVAudioEngineManager&) = delete;
    AVAudioEngineManager& operator= (const AVAudioEngineManager&) = delete;
    AVAudioEngineManager (AVAudioEngineManager&&) = delete;
    AVAudioEngineManager& operator= (AVAudioEngineManager&&) = delete;

    void start();
    void stop();

    RenderStatsSnapshot getSnapshot() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    std::atomic<bool> running { false };
    std::atomic<double> lastLatencyMs { 0.0 };
};
