#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"

namespace StudioPatchbay
{

// The menu bar across the top of the desktop (27 px logical): the product
// mark, the menu heads, the clock (click shows the date for a moment), a
// double separator and the product name ("Hidden" while the window is
// collapsed). The heads are buttons; the rest is painted.
class OS9MenuBar : public juce::Component,
                   private juce::Timer
{
public:
    enum class Menu { file, view, options, help };

    OS9MenuBar();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setPalette (const StudioPatchbay::Design::Palette& newPalette);
    void setUiScale (double scale);
    void setShaded (bool);

    // Re-reads the head titles in the current language.
    void refreshText();

    // Heads that act on the document are off while the window is collapsed.
    void setMenuEnabled (Menu, bool);

    // The head to anchor a menu at; its highlight stays until menuClosed().
    juce::Component& head (Menu);
    void menuClosed();

    std::function<void (Menu)> onMenu;

private:
    // zero-footprint: one tick a second, repainting only the clock strip; the
    // menu bar exists only while the application window does.
    void timerCallback() override;
    int sc (int value) const;
    juce::TextButton& button (Menu);

    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    double uiScale = 1.0;
    bool shaded = false;

    juce::TextButton fileMenuButton, viewMenuButton, optionsMenuButton, helpMenuButton;
    juce::TextButton clockButton;
    juce::Image productMark;

    bool showDate = false;
    double dateDisplayUntilMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OS9MenuBar)
};

} // namespace StudioPatchbay
