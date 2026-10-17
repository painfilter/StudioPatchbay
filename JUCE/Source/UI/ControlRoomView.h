#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "Model/DataModel.h"

namespace StudioPatchbay
{

// Control Room view showing the configured monitor buses from
// Cubase's ControlRoom/Default.xml. Each bus is displayed as an
// OS9-style inset panel with bus name, type, and channel assignment.
class ControlRoomView : public juce::Component
{
public:
    ControlRoomView();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setBuses (const std::vector<ControlRoomBus>& buses);

    void setPalette (const StudioPatchbay::Design::Palette& newPalette);
    void setUiScale (double scale);

    // The whole scrolled content, for export.
    juce::Component& getExportContent() { return *this; }

private:
    int sc (int value) const;
    static juce::String kindLabel (ControlRoomBus::Kind kind);

    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    double uiScale = 1.0;

    std::vector<ControlRoomBus> currentBuses;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlRoomView)
};

} // namespace StudioPatchbay
