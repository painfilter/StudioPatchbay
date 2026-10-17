#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "Model/DataModel.h"
#include "StudioPatchbayTokens.h"

namespace StudioPatchbay
{

// Slide-in inspector panel on the right side showing detailed information
// about a selected ParsedDevice. Appears when a device card is clicked,
// dismissed via close button or clicking elsewhere.
//
// Shows: device name, type, all channel assignments with port context,
// MIDI device, channel count, and mismatch warnings.
class DeviceDetailPanel : public juce::Component
{
public:
    DeviceDetailPanel();

    void paint (juce::Graphics& g) override;
    void resized() override;

    // Show the panel for a device. Pass nullptr to hide.
    void showDevice (const ParsedDevice* device,
                     const std::vector<DetectedPort>& ports);
    void hide();

    bool isShowing() const { return visible; }

    void setPalette (const StudioPatchbay::Design::Palette& newPalette);
    void setUiScale (double scale);

    // Callback when the panel is closed.
    std::function<void()> onClose;

    // Logical width of the panel (used by parent for layout).
    static constexpr int panelWidth = 240;

private:
    int sc (int value) const;

    // Paint a labelled section header.
    void paintSectionHeader (juce::Graphics& g, juce::Rectangle<int>& area,
                             const juce::String& title);

    // Paint a key-value row.
    void paintKeyValue (juce::Graphics& g, juce::Rectangle<int>& area,
                        const juce::String& key, const juce::String& value,
                        juce::Colour valueColour = juce::Colour());

    // Paint channel list with port family context.
    void paintChannelList (juce::Graphics& g, juce::Rectangle<int>& area,
                           const std::vector<int>& channels,
                           ChannelDirection dir);

    // Find the port family for a given channel and direction.
    juce::String portFamilyForChannel (int channel, ChannelDirection dir) const;

    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    double uiScale = 1.0;
    bool visible = false;

    ParsedDevice currentDevice;
    std::vector<DetectedPort> currentPorts;

    juce::TextButton closeButton { juce::String (juce::CharPointer_UTF8 ("\xc3\x97")) }; // × symbol

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeviceDetailPanel)
};

} // namespace StudioPatchbay
