#include "OS9MenuBar.h"
#include "Fonts.h"

namespace StudioPatchbay
{

OS9MenuBar::OS9MenuBar()
{
    // Configure transparent menu buttons (painted text underneath, buttons on top).
    for (auto* b : { &fileMenuButton, &viewMenuButton,
                     &optionsMenuButton, &helpMenuButton })
    {
        b->getProperties().set ("menuBar", true);
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        b->setColour (juce::TextButton::buttonOnColourId, pal.selection);
        b->setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        b->setColour (juce::TextButton::textColourOffId, pal.ink);
        addAndMakeVisible (*b);
    }

    // Wire up menu button callbacks.
    fileMenuButton.onClick    = [this] { if (onFileMenu)    onFileMenu(); };
    viewMenuButton.onClick    = [this] { if (onViewMenu)    onViewMenu(); };
    optionsMenuButton.onClick = [this] { if (onOptionsMenu) onOptionsMenu(); };
    helpMenuButton.onClick    = [this] { if (onHelpMenu)    onHelpMenu(); };

    // Invisible clock button – clicking shows date for ~3.6s. The look and feel
    // draws nothing for a "clockDisplay" button; the clock is painted below it.
    clockButton.getProperties().set ("clockDisplay", true);
    clockButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    clockButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (clockButton);
    clockButton.onClick = [this]
    {
        showDateInMenuBar = true;
        dateDisplayUntilMs = juce::Time::getMillisecondCounterHiRes()
                             + StudioPatchbay::Design::Interaction::menuClockDateDisplayMs;
        repaint();
    };

    // Start the 1-second clock timer.
    startTimerHz (1);
}

void OS9MenuBar::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;

    for (auto* b : { &fileMenuButton, &viewMenuButton,
                     &optionsMenuButton, &helpMenuButton })
    {
        b->setColour (juce::TextButton::buttonOnColourId, pal.selection);
        b->setColour (juce::TextButton::textColourOffId, pal.ink);
    }

    repaint();
}

void OS9MenuBar::setUiScale (double scale)
{
    uiScale = scale;
    resized();
    repaint();
}

int OS9MenuBar::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

// ============================================================================
// Paint
// ============================================================================

void OS9MenuBar::paint (juce::Graphics& g)
{
    const auto edge = juce::jmax (1, sc (1));
    const auto barH = sc (StudioPatchbay::Design::Metrics::menuBarHeight);
    const auto now = juce::Time::getCurrentTime();
    const auto font = Fonts::sans (static_cast<float> (sc (13)), Fonts::Weight::regular);

    // --- Menu bar background ---
    g.setColour (pal.windowPanel);
    g.fillRect (0, 0, getWidth(), barH);

    // Bottom separator line.
    g.setColour (pal.ink);
    g.fillRect (0, barH - edge, getWidth(), edge);

    // --- Menu item text (painted under the transparent buttons) ---
    g.setFont (font);
    g.setColour (pal.ink);

    // Product icon area (left 26px reserved for icon – placeholder square for now).
    const auto iconRect = juce::Rectangle<int> (sc (6), sc (5), sc (16), sc (16));
    g.setColour (pal.ink.withAlpha (0.3f));
    g.drawRect (iconRect, 1);

    // Menu labels – positioned to match the transparent button overlays.
    // (The buttons handle the clicks; we just paint the text underneath.)

    // --- Clock + Product name (right side) ---
    g.setColour (pal.ink);

    // Clock text.
    const auto clockText = showDateInMenuBar ? os9DateText (now) : os9TimeText (now);
    const auto clockRect = juce::Rectangle<int> (
        getWidth() - sc (200), 0, sc (StudioPatchbay::Design::Metrics::menuClockWidth), sc (26));
    g.drawText (clockText, clockRect, juce::Justification::centredRight);

    // Separator before clock.
    g.fillRect (getWidth() - sc (204), sc (2), edge, sc (22));

    // Product name.
    const auto nameRect = juce::Rectangle<int> (
        getWidth() - sc (110), 0, sc (104), sc (26));
    g.drawText ("StudioPatchbay", nameRect, juce::Justification::centredRight);

    // Separator before product name.
    g.fillRect (getWidth() - sc (114), sc (2), edge, sc (22));
}

// ============================================================================
// Layout
// ============================================================================

void OS9MenuBar::resized()
{
    const auto barH = sc (StudioPatchbay::Design::Metrics::menuBarHeight);

    // Place menu buttons starting after the product icon area.
    int mx = sc (30);
    auto placeMenu = [&] (juce::TextButton& b, int logicalWidth)
    {
        b.setBounds (mx, sc (1), sc (logicalWidth), sc (24));
        mx += sc (logicalWidth) + sc (6);
    };

    placeMenu (fileMenuButton, 46);
    placeMenu (viewMenuButton, 48);
    placeMenu (optionsMenuButton, 70);
    placeMenu (helpMenuButton, 50);

    // Clock button (invisible click target over the clock area).
    clockButton.setBounds (getWidth() - sc (210), 0, sc (100), barH);
}

// ============================================================================
// Clock timer
// ============================================================================

void OS9MenuBar::timerCallback()
{
    if (showDateInMenuBar
        && juce::Time::getMillisecondCounterHiRes() >= dateDisplayUntilMs)
    {
        showDateInMenuBar = false;
    }

    // Repaint only the clock + product name strip (right 220px).
    repaint (getWidth() - sc (220), 0, sc (220),
             sc (StudioPatchbay::Design::Metrics::menuBarHeight));
}

juce::String OS9MenuBar::os9TimeText (const juce::Time& t)
{
    const auto h24 = t.getHours();
    const auto h12 = h24 % 12 == 0 ? 12 : h24 % 12;
    return juce::String (h12) + ":"
           + juce::String (t.getMinutes()).paddedLeft ('0', 2)
           + (h24 < 12 ? " AM" : " PM");
}

juce::String OS9MenuBar::os9DateText (const juce::Time& t)
{
    const auto dayNames = std::array<const char*, 7> {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    const auto monthNames = std::array<const char*, 12> {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    return juce::String (dayNames[static_cast<size_t> (t.getDayOfWeek())]) + " "
           + juce::String (monthNames[static_cast<size_t> (t.getMonth())]) + " "
           + juce::String (t.getDayOfMonth());
}

} // namespace StudioPatchbay
