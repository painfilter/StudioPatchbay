#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"

namespace StudioPatchbay
{

// The toolbar row of the Swift app above the views: Back and Forward for
// jumps, the counts (instruments, FX devices, channels in use) and the
// search field with its number of matches (click: next match in the
// patchbay; Escape clears).
class Toolbar : public juce::Component
{
    // A Platinum button with a drawn arrow; faint while there is nowhere to go.
    class ArrowButton : public juce::TextButton
    {
    public:
        explicit ArrowButton (bool right) : pointsRight (right) {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
        juce::Colour ink, disabledInk;
    private:
        bool pointsRight;
    };

public:
    Toolbar();

    void setPalette (const Design::Palette&);
    void setUiScale (double);
    void refreshText();

    void setHistory (bool canGoBack, bool canGoForward);
    void setCounts (int instruments, int fxDevices, int channels, bool hasData);
    void setMatches (int count);   // shown only while searching
    juce::String getSearch() const { return search.getText(); }
    void clearSearch();
    void focusSearch() { search.grabKeyboardFocus(); }

    std::function<void()> onBack, onForward, onNextMatch;
    std::function<void (const juce::String&)> onSearch;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    int sc (int v) const { return juce::roundToInt (v * uiScale); }

    Design::Palette pal = Design::lightPalette();
    double uiScale = 1.0;
    ArrowButton backButton { false }, forwardButton { true };
    juce::TextButton matchesButton, clearButton;
    juce::TextEditor search;
    int instruments = 0, fxDevices = 0, channels = 0, matches = 0;
    bool hasData = false;
    juce::Rectangle<int> statsArea;
};

// To the right of the view tabs: whether the files are watched and when they
// were last read, Reload, and the content zoom (60–200 % in 10 % steps).
class ViewBar : public juce::Component, private juce::Timer
{
public:
    ViewBar();

    void setPalette (const Design::Palette&);
    void setUiScale (double);
    void refreshText();
    void setWatching (bool watching, juce::Time lastRead);
    void setContentScale (double);

    std::function<void()> onReload;
    std::function<void (double)> onContentScale;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // zero-footprint: once a minute, only for the "updated ... ago" text,
    // only while the window exists.
    void timerCallback() override { repaint (statusArea); }
    int sc (int v) const { return juce::roundToInt (v * uiScale); }

    Design::Palette pal = Design::lightPalette();
    double uiScale = 1.0, contentScale = 1.0;
    bool watching = false;
    juce::Time lastRead;
    juce::TextButton reloadButton, zoomOut, zoomIn, resetButton;
    juce::Rectangle<int> statusArea, percentArea;
};

} // namespace StudioPatchbay
