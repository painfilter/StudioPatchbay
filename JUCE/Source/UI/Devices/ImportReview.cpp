#include "ImportReview.h"
#include "UI/DeviceStyle.h"
#include "Localisation/Text.h"
#include "UI/Glyphs.h"

namespace StudioPatchbay
{

namespace
{
using Text::tr;

constexpr int margin = 16, summaryHeight = 76, footerHeight = 48, lineHeight = 16;

const auto amber = juce::Colour (0xffd08a00);
const auto red = juce::Colour (0xffc0392b);
const auto green = juce::Colour (0xff2fa84f);
} // namespace

ImportReview::ImportReview (AppState& state, std::vector<PhysicalDevice> imported)
    : appState (state), devices (std::move (imported))
{
    checks = PhysicalDevices::checkImport (devices, appState.getPhysicalDevices(), appState.getDetectedPorts());
    for (const auto& c : checks)
    {
        unmatched += c.unmatchedDirections.empty() ? 0 : 1;
        addBlocked += c.addConflicts.empty() ? 0 : 1;
        replaceBlocked += c.replaceConflicts.empty() ? 0 : 1;
    }

    viewport.setViewedComponent (&rows, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    cancelButton.setButtonText (tr ("action.cancel"));
    addButton.setButtonText (tr ("devices.importAdd"));
    replaceButton.setButtonText (tr ("devices.importReplace"));
    addButton.setEnabled (addBlocked == 0);
    replaceButton.setEnabled (replaceBlocked == 0);
    cancelButton.onClick = [this] { close(); };
    addButton.onClick = [this] { appState.addPhysicalDevices (devices); close(); };
    replaceButton.onClick = [this] { appState.replaceAllPhysicalDevices (devices); close(); };
    for (auto* b : { &cancelButton, &addButton, &replaceButton })
        addAndMakeVisible (*b);
    setSize (width, height);
}

void ImportReview::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);
    auto area = getLocalBounds().reduced (margin, 10).removeFromTop (summaryHeight);
    g.setFont (Fonts::sans (12.0f, Fonts::Weight::regular));
    g.setColour (pal.ink);

    const auto count = juce::String (static_cast<int> (devices.size()));
    const auto summaryLine = [&] (Glyphs::Kind kind, juce::Colour colour, const juce::String& text)
    {
        g.setColour (pal.ink);
        Glyphs::line (g, kind, colour, pal.ink, text, area.removeFromTop (24), 2);
    };
    if (appState.getDetectedPorts().empty())
        g.drawFittedText (tr ("devices.importNoPorts"), area.removeFromTop (30), juce::Justification::topLeft, 2);
    else if (unmatched > 0)
        summaryLine (Glyphs::Kind::warning, amber,
                     tr ("devices.importMismatch").replace ("%COUNT%", juce::String (unmatched)).replace ("%TOTAL%", count));
    else
        summaryLine (Glyphs::Kind::check, green, tr ("devices.importAllMatch").replace ("%TOTAL%", count));
    if (addBlocked > 0)
        summaryLine (Glyphs::Kind::stop, red, tr ("devices.importAddBlocked").replace ("%COUNT%", juce::String (addBlocked)));
    if (replaceBlocked > 0)
        summaryLine (Glyphs::Kind::stop, red, tr ("devices.importReplaceBlocked").replace ("%COUNT%", juce::String (replaceBlocked)));

    g.setColour (pal.shadowEdge);
    g.fillRect (margin, summaryHeight + 10, width - 2 * margin, 1);
}

void ImportReview::resized()
{
    viewport.setBounds (margin, summaryHeight + 14, width - 2 * margin, height - summaryHeight - 14 - footerHeight);
    rows.setSize (viewport.getMaximumVisibleWidth(), rows.preferredHeight());

    auto buttons = getLocalBounds().reduced (margin, 12).removeFromBottom (24);
    replaceButton.setBounds (buttons.removeFromRight (120));
    buttons.removeFromRight (8);
    addButton.setBounds (buttons.removeFromRight (130));
    buttons.removeFromRight (8);
    cancelButton.setBounds (buttons.removeFromRight (100));
}

int ImportReview::Rows::preferredHeight() const
{
    int h = 0;
    for (const auto& c : owner.checks)
        h += 22 + lineHeight * juce::jmax (1, static_cast<int> (c.addConflicts.size() + c.unmatchedDirections.size())) + 6;
    return h;
}

void ImportReview::Rows::paint (juce::Graphics& g)
{
    const auto& pal = owner.pal;
    int y = 0;
    for (size_t i = 0; i < owner.devices.size(); ++i)
    {
        const auto& d = owner.devices[i];
        const auto& c = owner.checks[i];
        g.setColour (pal.ink);
        g.setFont (Fonts::sans (13.0f, Fonts::Weight::semiBold));
        g.drawText (d.name, 0, y, getWidth(), 20, juce::Justification::centredLeft);
        y += 22;

        g.setFont (Fonts::sans (11.5f, Fonts::Weight::regular));
        if (c.addConflicts.empty() && c.unmatchedDirections.empty())
        {
            g.setColour (pal.inkSoft);
            Glyphs::line (g, Glyphs::Kind::check, green, pal.inkSoft, tr ("devices.importNoIssues"), { 8, y, getWidth() - 8, lineHeight });
            y += lineHeight;
        }
        for (const auto& conflict : c.addConflicts)
        {
            g.setColour (pal.ink);
            Glyphs::line (g, Glyphs::Kind::stop, red, pal.ink, tr ("devices.conflictWith")
                                  .replace ("%DIR%", conflict.direction == ChannelDirection::send ? "OUT" : "IN")
                                  .replace ("%RANGE%", DeviceStyle::rangeOf ({ conflict.first, conflict.last }))
                                  .replace ("%NAME%", conflict.otherName),
                          { 8, y, getWidth() - 8, lineHeight });
            y += lineHeight;
        }
        for (auto direction : c.unmatchedDirections)
        {
            g.setColour (pal.inkSoft);
            Glyphs::line (g, Glyphs::Kind::warning, amber, pal.inkSoft, tr ("devices.importNoPort")
                                  .replace ("%DIR%", direction == ChannelDirection::send ? "OUT" : "IN")
                                  .replace ("%RANGE%", DeviceStyle::rangeOf (d.range (direction))),
                          { 8, y, getWidth() - 8, lineHeight });
            y += lineHeight;
        }
        y += 6;
    }
}

} // namespace StudioPatchbay
