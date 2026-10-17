#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "Model/DataModel.h"
#include "DeviceCardView.h"

namespace StudioPatchbay
{

// Grid layout of DeviceCardViews inside a scrollable viewport.
// Groups devices by type: Instruments first, then External FX.
// Each group has an OS9-style section header.
class CardsView : public juce::Component
{
public:
    CardsView();

    void paint (juce::Graphics& g) override;
    void resized() override;

    // Update the displayed devices (called when AppState changes).
    void setDevices (const ParsedDevices& devices);

    void setPalette (const StudioPatchbay::Design::Palette& newPalette);
    void setUiScale (double scale);

    // Callback when a device card is clicked.
    std::function<void (const ParsedDevice&)> onDeviceClicked;

    // The whole scrolled content, for export.
    juce::Component& getExportContent() { return cardContainer; }

private:
    int sc (int value) const;
    void rebuildCards();
    int layoutCards(); // returns total content height

    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    double uiScale = 1.0;

    ParsedDevices currentDevices;
    juce::OwnedArray<DeviceCardView> cards;

    // Inner component to hold the cards and section headers (scrolled by viewport).
    struct CardContainer : public juce::Component
    {
        struct Header
        {
            juce::String title;
            juce::Rectangle<int> bounds;
        };

        void paint (juce::Graphics& g) override;

        StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
        double uiScale = 1.0;
        std::vector<Header> headers;
    };

    CardContainer cardContainer;
    juce::Viewport viewport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CardsView)
};

} // namespace StudioPatchbay
