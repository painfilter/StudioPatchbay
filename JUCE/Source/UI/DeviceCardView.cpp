#include "DeviceCardView.h"
#include "Fonts.h"

namespace StudioPatchbay
{

DeviceCardView::DeviceCardView (const ParsedDevice& dev)
    : device (dev)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

int DeviceCardView::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

void DeviceCardView::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;
    repaint();
}

void DeviceCardView::setUiScale (double scale)
{
    uiScale = scale;
    repaint();
}

// ============================================================================
// Hover feedback
// ============================================================================

void DeviceCardView::mouseEnter (const juce::MouseEvent&)
{
    hovered = true;
    repaint();
}

void DeviceCardView::mouseExit (const juce::MouseEvent&)
{
    hovered = false;
    repaint();
}

void DeviceCardView::mouseUp (const juce::MouseEvent& e)
{
    if (contains (e.getPosition()) && onClick)
        onClick (device);
}

// ============================================================================
// Helpers
// ============================================================================

juce::Colour DeviceCardView::deviceTint() const
{
    switch (device.type)
    {
        case ParsedDevice::Type::instrument: return Tokens::DeviceTint::instrument();
        case ParsedDevice::Type::fx:         return Tokens::DeviceTint::fx();
        case ParsedDevice::Type::sendOnly:   return Tokens::DeviceTint::sendOnly();
    }
    return juce::Colours::transparentBlack;
}

juce::String DeviceCardView::typeLabel (ParsedDevice::Type type)
{
    switch (type)
    {
        case ParsedDevice::Type::instrument: return "Instrument";
        case ParsedDevice::Type::fx:         return "External FX";
        case ParsedDevice::Type::sendOnly:   return "Send Only";
    }
    return {};
}

juce::String DeviceCardView::formatChannels (const std::vector<int>& channels)
{
    if (channels.empty())
        return juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x93")); // em dash

    if (channels.size() == 1)
        return juce::String (channels[0]);

    // Check if contiguous range.
    bool contiguous = true;
    for (size_t i = 1; i < channels.size(); ++i)
    {
        if (channels[i] != channels[i - 1] + 1)
        {
            contiguous = false;
            break;
        }
    }

    if (contiguous)
        return juce::String (channels.front()) + juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x93"))
               + juce::String (channels.back());

    // Non-contiguous: list them.
    juce::String result;
    for (size_t i = 0; i < channels.size(); ++i)
    {
        if (i > 0) result += ", ";
        result += juce::String (channels[i]);
    }
    return result;
}

// ============================================================================
// Paint
// ============================================================================

void DeviceCardView::paint (juce::Graphics& g)
{
    const auto edge = juce::jmax (1, sc (1));
    auto bounds = getLocalBounds();

    // --- Card background with device type tint ---
    auto tint = deviceTint();
    auto baseBg = pal.controlSurface.overlaidWith (tint);
    g.setColour (baseBg);
    g.fillRect (bounds);

    // --- OS9 Group Box bevel ---
    // Top highlight.
    g.setColour (pal.highlightEdge);
    g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), edge);
    g.fillRect (bounds.getX(), bounds.getY(), edge, bounds.getHeight());

    // Bottom shadow.
    g.setColour (pal.shadowEdge);
    g.fillRect (bounds.getX(), bounds.getBottom() - edge, bounds.getWidth(), edge);
    g.fillRect (bounds.getRight() - edge, bounds.getY(), edge, bounds.getHeight());

    // --- Hover feedback ---
    if (hovered)
    {
        g.setColour (pal.selection.withAlpha (0.22f));
        g.fillRect (bounds);
        g.setColour (pal.selection);
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 2.0f,
                                static_cast<float> (edge));
    }

    // --- Content layout ---
    auto content = bounds.reduced (sc (8), sc (6));

    // Device type label (small, uppercase).
    auto typeFont = Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::semiBold);
    g.setFont (typeFont);
    g.setColour (pal.ink.withAlpha (0.5f));
    auto typeArea = content.removeFromTop (sc (14));
    g.drawText (typeLabel (device.type).toUpperCase(), typeArea,
                juce::Justification::centredLeft);

    content.removeFromTop (sc (2));

    // Device name (bold).
    auto nameFont = Fonts::sans (static_cast<float> (sc (13)), Fonts::Weight::semiBold);
    g.setFont (nameFont);
    g.setColour (pal.ink);
    auto nameArea = content.removeFromTop (sc (18));
    g.drawText (device.name, nameArea, juce::Justification::centredLeft);

    content.removeFromTop (sc (6));

    // --- Channels (inset field style) ---
    auto channelFont = Fonts::sans (static_cast<float> (sc (11)), Fonts::Weight::regular);
    g.setFont (channelFont);

    auto drawChannelRow = [&] (const juce::String& label, const juce::String& value,
                               juce::Rectangle<int>& area)
    {
        auto row = area.removeFromTop (sc (20));
        auto labelArea = row.removeFromLeft (sc (42));
        auto fieldArea = row.reduced (0, sc (1));

        // Label.
        g.setColour (pal.ink.withAlpha (0.6f));
        g.drawText (label, labelArea, juce::Justification::centredLeft);

        // Inset field.
        g.setColour (pal.field);
        g.fillRect (fieldArea);

        // Field bevel (inset).
        g.setColour (pal.shadowEdge);
        g.fillRect (fieldArea.getX(), fieldArea.getY(), fieldArea.getWidth(), edge);
        g.fillRect (fieldArea.getX(), fieldArea.getY(), edge, fieldArea.getHeight());
        g.setColour (pal.highlightEdge);
        g.fillRect (fieldArea.getX(), fieldArea.getBottom() - edge, fieldArea.getWidth(), edge);
        g.fillRect (fieldArea.getRight() - edge, fieldArea.getY(), edge, fieldArea.getHeight());

        // Value text.
        g.setColour (pal.ink);
        g.drawText (value, fieldArea.reduced (sc (4), 0), juce::Justification::centredLeft);
    };

    if (! device.sendChannels.empty())
        drawChannelRow ("Send", formatChannels (device.sendChannels), content);

    if (! device.recvChannels.empty())
        drawChannelRow ("Recv", formatChannels (device.recvChannels), content);

    // Mismatch warning.
    if (device.hasMismatchedChannels())
    {
        content.removeFromTop (sc (2));
        g.setFont (Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::medium));
        g.setColour (juce::Colour (0xFFE57373)); // soft red warning
        g.drawText (juce::String (juce::CharPointer_UTF8 ("\xe2\x9a\xa0")) + " Mismatched channels",
                    content.removeFromTop (sc (14)),
                    juce::Justification::centredLeft);
    }

    // MIDI device.
    if (device.midiDevice.isNotEmpty())
    {
        content.removeFromTop (sc (4));
        g.setFont (Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::regular));
        g.setColour (pal.ink.withAlpha (0.5f));
        g.drawText ("MIDI: " + device.midiDevice,
                    content.removeFromTop (sc (14)),
                    juce::Justification::centredLeft);
    }

    // Channel count badge (bottom-right).
    auto badge = bounds.removeFromBottom (sc (20)).removeFromRight (sc (40)).reduced (sc (4), sc (2));
    g.setFont (Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::medium));
    g.setColour (pal.ink.withAlpha (0.4f));
    auto countText = juce::String (device.channelCount()) + " ch";
    if (device.isStereo()) countText = "Stereo";
    if (device.isMono())   countText = "Mono";
    g.drawText (countText, badge, juce::Justification::centredRight);
}

void DeviceCardView::resized()
{
    // Card is sized by the parent grid – no internal layout needed.
}

} // namespace StudioPatchbay
