#import <TargetConditionals.h>

#if TARGET_OS_OSX

#ifdef __OBJC__
#import <AVFoundation/AVFoundation.h>
#endif

#include "AVAudioEngineManager.h"

struct AVAudioEngineManager::Impl
{
#ifdef __OBJC__
    AVAudioEngine* engine = nil;
    AVAudioInputNode* inputNode = nil;
    AVAudioOutputNode* outputNode = nil;
#endif

    void start()
    {
#ifdef __OBJC__
        if (engine == nil)
            engine = [[AVAudioEngine alloc] init];

        inputNode = [engine inputNode];
        outputNode = [engine outputNode];

        NSError* error = nil;
        if (! [engine startAndReturnError:&error])
        {
            NSLog (@"AVAudioEngine failed to start: %@", error);
        }
#endif
    }

    void stop()
    {
#ifdef __OBJC__
        if (engine != nil)
            [engine stop];
#endif
    }
};

AVAudioEngineManager::AVAudioEngineManager()
    : impl (std::make_unique<Impl>())
{
}

AVAudioEngineManager::~AVAudioEngineManager() = default;

void AVAudioEngineManager::start()
{
    impl->start();
    running.store (true, std::memory_order_release);
}

void AVAudioEngineManager::stop()
{
    impl->stop();
    running.store (false, std::memory_order_release);
}

AVAudioEngineManager::RenderStatsSnapshot AVAudioEngineManager::getSnapshot() const
{
    RenderStatsSnapshot s;
    s.running = running.load (std::memory_order_acquire);
    s.lastLatencyMs = lastLatencyMs.load (std::memory_order_relaxed);
    return s;
}

#else

#include "AVAudioEngineManager.h"

AVAudioEngineManager::AVAudioEngineManager() : impl (nullptr) {}
AVAudioEngineManager::~AVAudioEngineManager() = default;

void AVAudioEngineManager::start() {}
void AVAudioEngineManager::stop() {}

AVAudioEngineManager::RenderStatsSnapshot AVAudioEngineManager::getSnapshot() const
{
    return {};
}

#endif
