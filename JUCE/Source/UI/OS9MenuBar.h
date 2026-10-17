#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"

namespace StudioPatchbay
{

// OS 9 Menu bar (27 px) painted in the Classic Mac OS style.
// Implements: product icon area, menu items as transparent click targets,
// a live clock (h:mm AM/PM), and the product name on the far right.
//
// The menu bar is painted directly (no juce::MenuBarComponent)
// for pixel-exact rendering at every zoom level.
// Transparent juce::TextButtons are laid over the painted text to handle clicks.
class OS9MenuBar : public juce::Component,
                   private juce::Timer
{
public:
    OS9MenuBar();

    void paint (juce::Graphics& g) override;
    void resized() override;

    // Called by the parent when the palette changes (light/dark mode switch).
    void setPalette (const StudioPatchbay::Design::Palette& newPalette);

    // Called by the parent to apply the current UI scale.
    void setUiScale (double scale);

    // Menu callbacks – set these to wire up actual menu logic.
    std::function<void()> onFileMenu;
    std::function<void()> onViewMenu;
    std::function<void()> onOptionsMenu;
    std::function<void()> onHelpMenu;

private:
    void timerCallback() override;

    // Scale a logical pixel value by the current UI scale.
    int sc (int value) const;

    // Format the current time as OS9 clock text (h:mm AM/PM).
    static juce::String os9TimeText (const juce::Time& t);
    // Format the current date as OS9 date text (Mon DD, YYYY).
    static juce::String os9DateText (const juce::Time& t);

    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    double uiScale = 1.0;

    // Menu buttons (transparent, overlaid on the painted text).
    juce::TextButton fileMenuButton    { "File" };
    juce::TextButton viewMenuButton    { "View" };
    juce::TextButton optionsMenuButton { "Options" };
    juce::TextButton helpMenuButton    { "Help" };
    juce::TextButton clockButton;

    // Clock state.
    bool showDateInMenuBar = false;
    double dateDisplayUntilMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OS9MenuBar)
};

} // namespace StudioPatchbay
