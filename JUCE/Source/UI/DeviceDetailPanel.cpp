#include "DeviceDetailPanel.h"
#include "Fonts.h"
#include "PatchbayGridView.h" // for connectorTintForFamily

namespace StudioPatchbay
{

DeviceDetailPanel::DeviceDetailPanel()
{
    closeButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.setColour (juce::TextButton::textColourOffId, pal.ink);
    closeButton.onClick = [this]
    {
        hide();
        if (onClose) onClose();
    };
    addAndMakeVisible (closeButton);

    setVisible (false);
}

int DeviceDetailPanel::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

void DeviceDetailPanel::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;
    closeButton.setColour (juce::TextButton::textColourOffId, pal.ink);
    repaint();
}

void DeviceDetailPanel::setUiScale (double scale)
{
    uiScale = scale;
    resized();
    repaint();
}

void DeviceDetailPanel::showDevice (const ParsedDevice* device,
                                     const std::vector<DetectedPort>& ports)
{
    if (device == nullptr)
    {
        hide();
        return;
    }

    currentDevice = *device;
    currentPorts = ports;
    visible = true;
    setVisible (true);
    repaint();
}

void DeviceDetailPanel::hide()
{
    visible = false;
    setVisible (false);
}

// ============================================================================
// Port lookup
// ============================================================================

juce::String DeviceDetailPanel::portFamilyForChannel (int channel,
                                                       ChannelDirection dir) const
{
    for (auto& port : currentPorts)
    {
        if (port.direction == dir
            && channel >= port.channelStart
            && channel <= port.channelEnd)
        {
            return port.family;
        }
    }
    return {};
}

// ============================================================================
// Paint helpers
// ============================================================================

void DeviceDetailPanel::paintSectionHeader (juce::Graphics& g,
                                             juce::Rectangle<int>& area,
                                             const juce::String& title)
{
    const auto edge = juce::jmax (1, sc (1));
    auto row = area.removeFromTop (sc (22));

    g.setFont (Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::semiBold));
    g.setColour (pal.ink.withAlpha (0.5f));
    g.drawText (title.toUpperCase(), row.reduced (sc (2), 0),
                juce::Justification::centredLeft);

    // Separator.
    g.setColour (pal.shadowEdge.withAlpha (0.5f));
    g.fillRect (row.getX(), row.getBottom() - edge, row.getWidth(), edge);

    area.removeFromTop (sc (4));
}

void DeviceDetailPanel::paintKeyValue (juce::Graphics& g,
                                        juce::Rectangle<int>& area,
                                        const juce::String& key,
                                        const juce::String& value,
                                        juce::Colour valueColour)
{
    auto row = area.removeFromTop (sc (18));
    auto keyArea = row.removeFromLeft (sc (70));
    auto valArea = row;

    g.setFont (Fonts::sans (static_cast<float> (sc (11)), Fonts::Weight::regular));
    g.setColour (pal.ink.withAlpha (0.55f));
    g.drawText (key, keyArea, juce::Justification::centredLeft);

    g.setColour (valueColour.isTransparent() ? pal.ink : valueColour);
    g.drawText (value, valArea, juce::Justification::centredLeft, true);

    area.removeFromTop (sc (2));
}

void DeviceDetailPanel::paintChannelList (juce::Graphics& g,
                                           juce::Rectangle<int>& area,
                                           const std::vector<int>& channels,
                                           ChannelDirection dir)
{
    const auto edge = juce::jmax (1, sc (1));

    for (auto ch : channels)
    {
        auto row = area.removeFromTop (sc (20));
        auto chArea = row.removeFromLeft (sc (30));
        auto portArea = row;

        // Channel number in an inset field.
        auto field = chArea.reduced (sc (2), sc (2));
        g.setColour (pal.field);
        g.fillRect (field);

        // Field bevel.
        g.setColour (pal.shadowEdge);
        g.fillRect (field.getX(), field.getY(), field.getWidth(), edge);
        g.fillRect (field.getX(), field.getY(), edge, field.getHeight());
        g.setColour (pal.highlightEdge);
        g.fillRect (field.getX(), field.getBottom() - edge, field.getWidth(), edge);
        g.fillRect (field.getRight() - edge, field.getY(), edge, field.getHeight());

        g.setFont (Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::medium));
        g.setColour (pal.ink);
        g.drawText (juce::String (ch), field, juce::Justification::centred);

        // Port family name with connector tint.
        auto family = portFamilyForChannel (ch, dir);
        if (family.isNotEmpty())
        {
            auto tint = PatchbayGridView::connectorTintForFamily (family);
            auto colour = tint.isTransparent() ? pal.ink.withAlpha (0.5f)
                                               : tint.withAlpha (0.8f);

            // Small tint dot.
            g.setColour (colour);
            g.fillEllipse (static_cast<float> (portArea.getX() + sc (2)),
                           static_cast<float> (portArea.getCentreY() - sc (3)),
                           static_cast<float> (sc (6)),
                           static_cast<float> (sc (6)));

            g.setFont (Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::regular));
            g.setColour (pal.ink.withAlpha (0.6f));
            g.drawText (family, portArea.withTrimmedLeft (sc (12)),
                        juce::Justification::centredLeft, true);
        }

        area.removeFromTop (sc (1));
    }
}

// ============================================================================
// Paint
// ============================================================================

void DeviceDetailPanel::paint (juce::Graphics& g)
{
    const auto edge = juce::jmax (1, sc (1));

    // Panel background.
    g.setColour (pal.windowPanel);
    g.fillRect (getLocalBounds());

    // Left border (inset shadow).
    g.setColour (pal.shadowEdge);
    g.fillRect (0, 0, edge, getHeight());

    auto area = getLocalBounds().reduced (sc (10), sc (8));

    // Close button is positioned in resized().
    area.removeFromTop (sc (4));

    // --- Device type ---
    juce::String typeStr;
    switch (currentDevice.type)
    {
        case ParsedDevice::Type::instrument: typeStr = "INSTRUMENT"; break;
        case ParsedDevice::Type::fx:         typeStr = "EXTERNAL FX"; break;
        case ParsedDevice::Type::sendOnly:   typeStr = "SEND ONLY"; break;
    }
    g.setFont (Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::semiBold));
    g.setColour (pal.ink.withAlpha (0.5f));
    g.drawText (typeStr, area.removeFromTop (sc (16)), juce::Justification::centredLeft);
    area.removeFromTop (sc (2));

    // --- Device name ---
    g.setFont (Fonts::sans (static_cast<float> (sc (15)), Fonts::Weight::semiBold));
    g.setColour (pal.ink);
    g.drawText (currentDevice.name, area.removeFromTop (sc (22)),
                juce::Justification::centredLeft, true);

    area.removeFromTop (sc (10));

    // --- Summary ---
    paintSectionHeader (g, area, "Summary");

    juce::String channelInfo;
    if (currentDevice.isMono())        channelInfo = "Mono";
    else if (currentDevice.isStereo()) channelInfo = "Stereo";
    else                               channelInfo = juce::String (currentDevice.channelCount()) + " channels";
    paintKeyValue (g, area, "Channels", channelInfo);

    paintKeyValue (g, area, "Send",
                   currentDevice.sendChannels.empty() ? juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x93"))
                       : juce::String (currentDevice.sendChannels.size()) + " ch");
    paintKeyValue (g, area, "Return",
                   currentDevice.recvChannels.empty() ? juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x93"))
                       : juce::String (currentDevice.recvChannels.size()) + " ch");

    if (currentDevice.midiDevice.isNotEmpty())
        paintKeyValue (g, area, "MIDI", currentDevice.midiDevice);

    if (currentDevice.hasMismatchedChannels())
    {
        area.removeFromTop (sc (4));
        g.setFont (Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::medium));
        g.setColour (juce::Colour (0xFFE57373));
        g.drawText (juce::String (juce::CharPointer_UTF8 ("\xe2\x9a\xa0")) + " Send/Return channel mismatch",
                    area.removeFromTop (sc (16)),
                    juce::Justification::centredLeft);
    }

    area.removeFromTop (sc (8));

    // --- Send channels ---
    if (! currentDevice.sendChannels.empty())
    {
        paintSectionHeader (g, area, "Send Channels");
        paintChannelList (g, area, currentDevice.sendChannels, ChannelDirection::send);
        area.removeFromTop (sc (8));
    }

    // --- Recv channels ---
    if (! currentDevice.recvChannels.empty())
    {
        paintSectionHeader (g, area, "Return Channels");
        paintChannelList (g, area, currentDevice.recvChannels, ChannelDirection::recv);
    }
}

// ============================================================================
// Layout
// ============================================================================

void DeviceDetailPanel::resized()
{
    auto area = getLocalBounds().reduced (sc (10), sc (8));
    auto topRow = area.removeFromTop (sc (20));
    closeButton.setBounds (topRow.removeFromRight (sc (22)));
}

} // namespace StudioPatchbay
