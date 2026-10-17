#pragma once

#include "Model/PhysicalDevice.h"
#include "Model/AudioPorts.h"
#include "ViewStyle.h"

#include <optional>
#include <set>

namespace StudioPatchbay
{

// What every content view reads: the parsed data, the filtered devices and
// the selection, plus the actions a view can ask for. Owned by the workspace.
struct ViewData
{
    ParsedDevices devices;
    std::vector<ParsedDevice> filtered;      // after search and filter
    std::set<int> highlightedChannels;       // channels of the filtered devices
    bool searchActive = false;
    std::vector<PhysicalDevice> physical;
    std::vector<AudioPort> ports;
    std::vector<ControlRoomBus> buses;
    std::optional<ParsedDevice> selected;

    std::function<void (std::optional<ParsedDevice>)> select;
    std::function<void (int channel)> jumpToChannel;
    std::function<void()> openSetup;

    bool isSelected (const ParsedDevice& d) const { return selected && *selected == d; }
};

// A content view: a scrolling page that draws ViewData in a ViewStyle.
class ContentView : public juce::Component
{
public:
    explicit ContentView (const ViewData& d) : data (d)
    {
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);
    }

    void setStyle (const ViewStyle& s) { style = s; refresh(); }
    void resized() override { viewport.setBounds (getLocalBounds()); refresh(); }

    // Lays the page out again for the current data, style and width.
    virtual void refresh() = 0;
    // The whole page, for export.
    virtual juce::Component& getExportContent() = 0;

    juce::Viewport& getViewport() { return viewport; }

protected:
    // Always leaves room for the scroll bar, so a page that starts to scroll
    // does not change width under the pointer.
    int pageWidth() const { return juce::jmax (1, viewport.getWidth() - viewport.getScrollBarThickness()); }

    const ViewData& data;
    ViewStyle style;
    juce::Viewport viewport;
};

} // namespace StudioPatchbay
