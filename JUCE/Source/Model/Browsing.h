#pragma once

#include "PhysicalDevice.h"

#include <optional>

namespace StudioPatchbay
{

// The sidebar filter of the Swift app.
enum class FilterMode { all, instruments, fx, sendOnly, stereo, mono, midi };

namespace Browsing
{
// Search matches a device's name, MIDI device, any channel number (also
// partly, "19" finds 195) and the names of physical devices on its channels;
// case does not matter. Then the filter applies. Order is kept.
bool matchesSearch (const ParsedDevice&, const juce::String& query, const std::vector<PhysicalDevice>&);
std::vector<ParsedDevice> filter (const ParsedDevices&, FilterMode, const juce::String& query,
                                  const std::vector<PhysicalDevice>&);

// The matches in the order they appear in the patchbay: by first channel,
// then name. "Next match" walks this list and wraps.
std::vector<ParsedDevice> inPatchbayOrder (std::vector<ParsedDevice>);
} // namespace Browsing

// Back and forward for jumps to a channel, as in a browser. A point is a
// view and the channel last jumped to, if any.
class NavigationHistory
{
public:
    struct Point
    {
        int view = 0;
        std::optional<int> channel;
        bool operator== (const Point&) const = default;
    };

    // A new jump from where we are: remembered for Back; Forward is cleared.
    void jumped (Point from);
    // Where Back/Forward lead, recording `current` on the other side.
    std::optional<Point> back (Point current);
    std::optional<Point> forward (Point current);

    bool canGoBack() const { return ! backStack.empty(); }
    bool canGoForward() const { return ! forwardStack.empty(); }
    void clear() { backStack.clear(); forwardStack.clear(); }

private:
    std::vector<Point> backStack, forwardStack;
};

} // namespace StudioPatchbay
