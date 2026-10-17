#pragma once

#include "DataModel.h"
#include <juce_events/juce_events.h>

namespace StudioPatchbay
{

// Cross-platform file watcher using juce::Timer for polling.
//
// The Swift original used kqueue (DispatchSource.makeFileSystemObjectSource) which
// is macOS-only. This JUCE implementation uses periodic polling with
// juce::File::getLastModificationTime() – works identically on macOS and Windows.
//
// Cubase writes files atomically (via rename/replacement), so we use a short
// polling interval (500ms) and a debounce delay (400ms) to match the Swift
// implementation's 0.4s delay after detecting a change.
//
// zero-footprint: three modification-time reads every 500 ms and nothing else;
// no repaint and no parse unless a file actually changed. Polling is the
// portable alternative to kqueue / ReadDirectoryChangesW.
class FileWatcher : private juce::Timer
{
public:
    FileWatcher() = default;
    ~FileWatcher() override { stopTimer(); debounce.stopTimer(); }

    // Start watching a set of files. The onChange callback fires on the message
    // thread when any watched file changes. Multiple rapid changes are debounced.
    void watchFiles (const std::vector<juce::File>& filesToWatch,
                     std::function<void()> onChangeCallback)
    {
        files = filesToWatch;
        onChange = std::move (onChangeCallback);

        // Snapshot current modification times
        lastModTimes.clear();
        for (auto& f : files)
            lastModTimes.push_back (f.getLastModificationTime());

        startTimer (pollIntervalMs);
    }

    void stopWatching()
    {
        stopTimer();
        debounce.stopTimer();
        files.clear();
        lastModTimes.clear();
        onChange = nullptr;
    }

    bool isWatching() const { return isTimerRunning(); }

private:
    void timerCallback() override
    {
        bool changed = false;

        for (size_t i = 0; i < files.size(); ++i)
        {
            auto currentMod = files[i].getLastModificationTime();

            if (currentMod != lastModTimes[i])
            {
                lastModTimes[i] = currentMod;
                changed = true;
            }
        }

        // Debounce: delay the callback slightly to let Cubase finish atomic
        // writes (matching the Swift 0.4s delay). A member timer, not
        // callAfterDelay: it dies with the watcher, so a late callback can
        // never reach an owner that is already gone.
        if (changed && onChange)
            debounce.startTimer (debounceMs);
    }

    struct Debounce : juce::Timer
    {
        explicit Debounce (FileWatcher& w) : owner (w) {}
        void timerCallback() override
        {
            stopTimer();
            if (owner.onChange)
                owner.onChange();
        }
        FileWatcher& owner;
    };

    static constexpr int pollIntervalMs = 500;
    static constexpr int debounceMs = 400;

    std::vector<juce::File> files;
    std::vector<juce::Time> lastModTimes;
    std::function<void()> onChange;
    Debounce debounce { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileWatcher)
};

} // namespace StudioPatchbay
