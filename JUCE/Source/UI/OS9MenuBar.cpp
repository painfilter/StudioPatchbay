#include "OS9MenuBar.h"
#include "Fonts.h"
#include "Localisation/Text.h"
#include "BinaryData.h"

namespace StudioPatchbay
{

namespace
{
using Text::tr;

juce::String clockText (const juce::Time& t, bool german)
{
    if (german)
        return juce::String (t.getHours()).paddedLeft ('0', 2) + ":"
               + juce::String (t.getMinutes()).paddedLeft ('0', 2);
    const auto h24 = t.getHours();
    return juce::String (h24 % 12 == 0 ? 12 : h24 % 12) + ":"
           + juce::String (t.getMinutes()).paddedLeft ('0', 2) + (h24 < 12 ? " AM" : " PM");
}

juce::String dateText (const juce::Time& t, bool german)
{
    const auto year = juce::String (t.getYear() % 100).paddedLeft ('0', 2);
    if (german)
        return juce::String (t.getDayOfMonth()).paddedLeft ('0', 2) + "."
               + juce::String (t.getMonth() + 1).paddedLeft ('0', 2) + "." + year;
    return juce::String (t.getMonth() + 1) + "/" + juce::String (t.getDayOfMonth()) + "/" + year;
}

// Clock, separator and product name: the strip the clock tick repaints.
constexpr int rightStripWidth = 202;
} // namespace

OS9MenuBar::OS9MenuBar()
{
    productMark = juce::ImageCache::getFromMemory (BinaryData::StudioPatchbayIcon_png,
                                                    BinaryData::StudioPatchbayIcon_pngSize);

    const std::pair<juce::TextButton*, Menu> heads[] = {
        { &fileMenuButton, Menu::file }, { &viewMenuButton, Menu::view },
        { &optionsMenuButton, Menu::options }, { &helpMenuButton, Menu::help } };
    for (auto [b, menu] : heads)
    {
        b->getProperties().set ("menuBar", true);
        b->setClickingTogglesState (true);
        b->setTriggeredOnMouseDown (true);
        b->onClick = [this, b, m = menu]
        {
            if (b->getToggleState() && onMenu)
                onMenu (m);
        };
        addAndMakeVisible (*b);
    }

    // The clock is painted; this invisible button only takes the click.
    clockButton.getProperties().set ("clockDisplay", true);
    addAndMakeVisible (clockButton);
    clockButton.onClick = [this]
    {
        showDate = true;
        dateDisplayUntilMs = juce::Time::getMillisecondCounterHiRes()
                             + StudioPatchbay::Design::Interaction::menuClockDateDisplayMs;
        repaint (getWidth() - sc (rightStripWidth), 0, sc (rightStripWidth), getHeight());
    };

    refreshText();
    startTimerHz (1);
}

juce::TextButton& OS9MenuBar::button (Menu menu)
{
    switch (menu)
    {
        case Menu::file:    return fileMenuButton;
        case Menu::view:    return viewMenuButton;
        case Menu::options: return optionsMenuButton;
        case Menu::help:    break;
    }
    return helpMenuButton;
}

juce::Component& OS9MenuBar::head (Menu menu) { return button (menu); }

void OS9MenuBar::menuClosed()
{
    for (auto* b : { &fileMenuButton, &viewMenuButton, &optionsMenuButton, &helpMenuButton })
        b->setToggleState (false, juce::dontSendNotification);
}

void OS9MenuBar::setMenuEnabled (Menu menu, bool enabled)
{
    button (menu).setToggleState (false, juce::dontSendNotification);
    button (menu).setEnabled (enabled);
}

void OS9MenuBar::refreshText()
{
    fileMenuButton.setButtonText (tr ("menu.file"));
    viewMenuButton.setButtonText (tr ("menu.view"));
    optionsMenuButton.setButtonText (tr ("menu.options"));
    helpMenuButton.setButtonText (tr ("menu.help"));
    resized();
    repaint();
}

void OS9MenuBar::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;
    repaint();
}

void OS9MenuBar::setUiScale (double scale)
{
    uiScale = scale;
    resized();
    repaint();
}

void OS9MenuBar::setShaded (bool value)
{
    shaded = value;
    repaint();
}

int OS9MenuBar::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

void OS9MenuBar::paint (juce::Graphics& g)
{
    const auto edge = juce::jmax (1, sc (1));
    const auto w = getWidth();

    g.setColour (pal.windowPanel);
    g.fillRect (getLocalBounds());
    g.setColour (pal.ink);
    g.fillRect (0, getHeight() - edge, w, edge);

    if (productMark.isValid())
        g.drawImageWithin (productMark, sc (5), sc (4), sc (19), sc (19), juce::RectanglePlacement::centred);

    const auto now = juce::Time::getCurrentTime();
    g.setFont (Fonts::sans (static_cast<float> (12.0 * uiScale), Fonts::Weight::regular));
    g.drawText (showDate ? dateText (now, Text::german()) : clockText (now, Text::german()),
                juce::Rectangle<int> (w - sc (198), 0, sc (StudioPatchbay::Design::Metrics::menuClockWidth), sc (26)),
                juce::Justification::centredRight);

    g.setColour (pal.shadowEdge);
    g.fillRect (w - sc (108), sc (3), edge, sc (21));
    g.setColour (pal.highlightEdge);
    g.fillRect (w - sc (107), sc (3), edge, sc (21));

    g.setColour (pal.ink);
    g.drawText (shaded ? tr ("ui.hidden") : juce::String ("StudioPatchbay"),
                juce::Rectangle<int> (w - sc (102), 0, sc (94), sc (26)), juce::Justification::centredRight);
}

void OS9MenuBar::resized()
{
    // Heads sit side by side from x 28, each as wide as its title plus a
    // margin, so a longer translation never clips.
    const auto font = Fonts::sans (12.0f, Fonts::Weight::regular);
    int x = 28;
    for (auto* b : { &fileMenuButton, &viewMenuButton, &optionsMenuButton, &helpMenuButton })
    {
        const auto width = juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, b->getButtonText())) + 22;
        b->setBounds (sc (x), sc (1), sc (width), sc (24));
        x += width + 1;
    }
    clockButton.setBounds (getWidth() - sc (202), 0, sc (98), sc (27));
}

void OS9MenuBar::timerCallback()
{
    if (showDate && juce::Time::getMillisecondCounterHiRes() >= dateDisplayUntilMs)
        showDate = false;
    repaint (getWidth() - sc (rightStripWidth), 0, sc (rightStripWidth), getHeight());
}

} // namespace StudioPatchbay
