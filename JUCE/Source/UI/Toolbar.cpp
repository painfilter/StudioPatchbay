#include "Toolbar.h"
#include "Fonts.h"
#include "Localisation/Text.h"

namespace StudioPatchbay
{

namespace
{
using Text::tr;

juce::String agoText (juce::Time t)
{
    const auto minutes = juce::roundToInt ((juce::Time::getCurrentTime() - t).inMinutes());
    if (minutes < 1) return tr ("toolbar.updatedJustNow");
    if (minutes < 60) return tr ("toolbar.updatedMinutes").replace ("%COUNT%", juce::String (minutes));
    const auto hours = minutes / 60;
    if (hours < 24) return tr ("toolbar.updatedHours").replace ("%COUNT%", juce::String (hours));
    return tr ("toolbar.updatedAt").replace ("%TIME%", t.toString (true, true, false));
}
} // namespace

// ============================================================================
// Toolbar
// ============================================================================

Toolbar::Toolbar()
{
    backButton.setButtonText (juce::String (juce::CharPointer_UTF8 ("\xe2\x97\x80")));
    forwardButton.setButtonText (juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xb6")));
    clearButton.setButtonText (juce::String (juce::CharPointer_UTF8 ("\xc3\x97")));
    backButton.onClick = [this] { if (onBack) onBack(); };
    forwardButton.onClick = [this] { if (onForward) onForward(); };
    matchesButton.onClick = [this] { if (onNextMatch) onNextMatch(); };
    clearButton.onClick = [this] { clearSearch(); };
    search.onTextChange = [this]
    {
        clearButton.setVisible (search.getText().isNotEmpty());
        if (onSearch) onSearch (search.getText());
    };
    search.onEscapeKey = [this] { clearSearch(); };
    search.onReturnKey = [this] { if (onNextMatch) onNextMatch(); };
    for (juce::Component* c : { static_cast<juce::Component*> (&backButton), static_cast<juce::Component*> (&forwardButton),
                                static_cast<juce::Component*> (&search) })
        addAndMakeVisible (*c);
    addChildComponent (matchesButton);
    addChildComponent (clearButton);
    setHistory (false, false);
    refreshText();
}

void Toolbar::refreshText()
{
    backButton.setTooltip (tr ("toolbar.back"));
    forwardButton.setTooltip (tr ("toolbar.forward"));
    matchesButton.setTooltip (tr ("toolbar.nextMatch"));
    clearButton.setTooltip (tr ("toolbar.clearSearch"));
    search.setTextToShowWhenEmpty (tr ("toolbar.search"), pal.inkSoft);
    repaint();
}

void Toolbar::setPalette (const Design::Palette& p)
{
    pal = p;
    refreshText();
}

void Toolbar::setUiScale (double s)
{
    uiScale = s;
    search.setFont (Fonts::sans (static_cast<float> (12.0 * s)));
    search.applyFontToAllText (search.getFont());
    resized();
    repaint();
}

void Toolbar::setHistory (bool back, bool forward)
{
    backButton.setEnabled (back);
    forwardButton.setEnabled (forward);
}

void Toolbar::setCounts (int i, int f, int c, bool data)
{
    instruments = i;
    fxDevices = f;
    channels = c;
    hasData = data;
    repaint (statsArea);
}

void Toolbar::setMatches (int count)
{
    matches = count;
    const auto searching = search.getText().isNotEmpty();
    matchesButton.setVisible (searching);
    matchesButton.setEnabled (count > 0);
    matchesButton.setButtonText (juce::String (count) + (count > 0 ? juce::String (juce::CharPointer_UTF8 (" \xe2\x86\x92")) : juce::String()));
}

void Toolbar::clearSearch()
{
    search.setText ({}, true);
}

void Toolbar::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);
    g.setColour (pal.shadowEdge.withAlpha (0.6f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
    if (! hasData || statsArea.isEmpty())
        return;

    const std::tuple<int, juce::String, juce::Colour> stats[] = {
        { instruments, tr ("toolbar.instruments"), juce::Colour (0xff2f62c8) },
        { fxDevices, tr ("toolbar.fxDevices"), juce::Colour (0xffc46f0c) },
        { channels, tr ("toolbar.channels"), pal.inkSoft } };
    auto area = statsArea;
    const auto w = area.getWidth() / 3;
    for (auto& [value, label, colour] : stats)
    {
        auto cell = area.removeFromLeft (w);
        g.setColour (colour);
        g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), static_cast<float> (14.0 * uiScale), juce::Font::bold)));
        g.drawText (juce::String (value), cell.removeFromTop (cell.getHeight() / 2 + sc (2)), juce::Justification::centredBottom);
        g.setColour (pal.inkSoft);
        g.setFont (Fonts::sans (static_cast<float> (9.5 * uiScale)));
        g.drawText (label, cell, juce::Justification::centredTop, true);
    }
}

void Toolbar::resized()
{
    auto area = getLocalBounds().reduced (sc (8), sc (5)).withTrimmedBottom (1);
    backButton.setBounds (area.removeFromLeft (sc (28)));
    area.removeFromLeft (sc (2));
    forwardButton.setBounds (area.removeFromLeft (sc (28)));
    area.removeFromLeft (sc (10));

    auto searchArea = area.removeFromRight (juce::jmin (sc (260), area.getWidth() / 2));
    clearButton.setBounds (searchArea.removeFromRight (sc (24)));
    searchArea.removeFromRight (sc (4));
    matchesButton.setBounds (searchArea.removeFromRight (sc (52)));
    searchArea.removeFromRight (sc (4));
    search.setBounds (searchArea);
    statsArea = area.withTrimmedRight (sc (10)).withWidth (juce::jmin (area.getWidth() - sc (10), sc (270)));
}

// ============================================================================
// ViewBar
// ============================================================================

ViewBar::ViewBar()
{
    zoomOut.setButtonText (juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92")));
    zoomIn.setButtonText ("+");
    zoomOut.onClick = [this] { if (onContentScale) onContentScale (juce::jmax (0.6, contentScale - 0.1)); };
    zoomIn.onClick = [this] { if (onContentScale) onContentScale (juce::jmin (2.0, contentScale + 0.1)); };
    resetButton.onClick = [this] { if (onContentScale) onContentScale (1.0); };
    reloadButton.onClick = [this] { if (onReload) onReload(); };
    for (auto* b : { &reloadButton, &zoomOut, &zoomIn, &resetButton })
        addAndMakeVisible (*b);
    refreshText();
    startTimer (60 * 1000);
}

void ViewBar::refreshText()
{
    reloadButton.setButtonText (tr ("toolbar.reload"));
    reloadButton.setTooltip (tr ("toolbar.reloadTooltip"));
    resetButton.setButtonText (tr ("toolbar.reset"));
    zoomOut.setTooltip (tr ("toolbar.zoomOut"));
    zoomIn.setTooltip (tr ("toolbar.zoomIn"));
    repaint();
}

void ViewBar::setPalette (const Design::Palette& p) { pal = p; repaint(); }

void ViewBar::setUiScale (double s)
{
    uiScale = s;
    resized();
    repaint();
}

void ViewBar::setWatching (bool w, juce::Time t)
{
    watching = w;
    lastRead = t;
    repaint (statusArea);
}

void ViewBar::setContentScale (double s)
{
    contentScale = s;
    zoomOut.setEnabled (s > 0.6 + 1e-6);
    zoomIn.setEnabled (s < 2.0 - 1e-6);
    resetButton.setEnabled (std::abs (s - 1.0) > 1e-6);
    repaint (percentArea);
}

void ViewBar::paint (juce::Graphics& g)
{
    g.setColour (pal.ink);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), static_cast<float> (11.0 * uiScale), juce::Font::plain)));
    g.drawText (juce::String (juce::roundToInt (contentScale * 100)) + "%", percentArea, juce::Justification::centred);

    if (statusArea.isEmpty())
        return;
    const auto dot = juce::Rectangle<float> (static_cast<float> (statusArea.getX()), statusArea.getCentreY() - 3.5f * static_cast<float> (uiScale),
                                             7.0f * static_cast<float> (uiScale), 7.0f * static_cast<float> (uiScale));
    g.setColour (watching ? juce::Colour (0xff2fa84f) : pal.inkSoft.withAlpha (0.6f));
    g.fillEllipse (dot);
    g.setColour (pal.inkSoft);
    g.setFont (Fonts::sans (static_cast<float> (11.0 * uiScale)));
    const auto text = lastRead != juce::Time() ? agoText (lastRead) : (watching ? tr ("toolbar.watching") : tr ("toolbar.notWatching"));
    g.drawText (text, statusArea.withTrimmedLeft (sc (12)), juce::Justification::centredLeft, true);
}

void ViewBar::resized()
{
    auto area = getLocalBounds().reduced (0, sc (4));
    resetButton.setBounds (area.removeFromRight (sc (50)));
    area.removeFromRight (sc (4));
    zoomIn.setBounds (area.removeFromRight (sc (24)));
    percentArea = area.removeFromRight (sc (44));
    zoomOut.setBounds (area.removeFromRight (sc (24)));
    area.removeFromRight (sc (10));
    reloadButton.setBounds (area.removeFromRight (sc (70)));
    area.removeFromRight (sc (8));
    statusArea = area.getWidth() >= sc (60) ? area : juce::Rectangle<int>();
}

} // namespace StudioPatchbay
