#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "Model/DataModel.h"
#include "StudioPatchbayTokens.h"

namespace StudioPatchbay
{

// Individual device card rendered in the OS9 Group Box style.
// Shows device name, type, channel assignments, MIDI link, and
// uses subtle colour tinting from StudioPatchbayTokens::DeviceTint.
//
// Hover feedback:
// selection colour @ 22% alpha fill, 1px rounded border, pointer cursor.
class DeviceCardView : public juce::Component
{
public:
    explicit DeviceCardView (const ParsedDevice& device);

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void setPalette (const StudioPatchbay::Design::Palette& newPalette);
    void setUiScale (double scale);

    const ParsedDevice& getDevice() const { return device; }

    // Callback when the card is clicked (for detail panel).
    std::function<void (const ParsedDevice&)> onClick;

private:
    int sc (int value) const;

    // Get the tint colour for this device type.
    juce::Colour deviceTint() const;

    // Format channel list as a compact string (e.g. "1–2" or "5, 6, 10").
    static juce::String formatChannels (const std::vector<int>& channels);

    // Format the device type as a display string.
    static juce::String typeLabel (ParsedDevice::Type type);

    ParsedDevice device;
    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    double uiScale = 1.0;
    bool hovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeviceCardView)
};

} // namespace StudioPatchbay
