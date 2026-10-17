#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"

namespace StudioPatchbay
{

// OS9-style tab bar for switching between application views.
// Renders trapezoid-shaped tabs matching the Classic Mac OS 9 aesthetic.
// Each tab has a label and optional shortcut. The active tab appears
// "pushed forward" (elevated) with the panel colour background.
class ViewSwitcher : public juce::Component
{
public:
    enum class View
    {
        cards,
        patchbay,
        controlRoom,
        signalFlow
    };

    ViewSwitcher();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

    static juce::String viewName (View view);

    void setPalette (const StudioPatchbay::Design::Palette& newPalette);
    void setUiScale (double scale);

    View getActiveView() const { return activeView; }
    void setActiveView (View view);

    // Callback when the user clicks a tab.
    std::function<void (View)> onViewChanged;

private:
    int sc (int value) const;

    struct TabInfo
    {
        juce::String label;
        View view;
    };

    // Hit-test: which tab index is at this x position?
    int tabIndexAtX (int x) const;

    // Get the bounds of tab at given index.
    juce::Rectangle<int> tabBounds (int index) const;

    std::vector<TabInfo> tabs;
    View activeView = View::cards;
    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    double uiScale = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViewSwitcher)
};

} // namespace StudioPatchbay
