#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"

namespace StudioPatchbay
{

// The application window that floats on the desktop: a 2 px frame, a
// striped title bar with the title in a cut-out, the close and zoom boxes
// (drawn, as on the reference window, but without a function) and the
// collapse box, which shades the window onto the desktop. The body holds
// one child, the window's content.
class AppWindow : public juce::Component
{
public:
    static constexpr int titleBarHeight = 25;

    explicit AppWindow (juce::Component& body);

    void setPalette (const Design::Palette&);
    void setUiScale (double);
    void setTitle (const juce::String&);
    void setCollapseTooltip (const juce::String&);

    std::function<void()> onCollapse;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    int sc (int value) const { return juce::roundToInt (value * uiScale); }
    juce::Rectangle<int> titleRow() const;

    juce::Component& body;
    juce::TextButton collapseBox;
    juce::String title;
    Design::Palette pal = Design::darkPalette();
    double uiScale = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppWindow)
};

} // namespace StudioPatchbay
