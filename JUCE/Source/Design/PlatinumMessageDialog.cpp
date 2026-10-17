// Generated file. Do not edit by hand: changes are overwritten when the
// file is regenerated.

#include "PlatinumMessageDialog.h"

namespace StudioPatchbay::Design
{

namespace
{
void drawPixelFrame (juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour colour, int thickness)
{
    g.setColour (colour);
    for (int inset = 0; inset < thickness; ++inset)
    {
        const auto frame = bounds.reduced (inset);
        if (frame.isEmpty())
            break;
        g.drawRect (frame, 1);
    }
}
}

MessageDialog::MessageDialog (std::function<void()> closeCallback)
    : close (std::move (closeCallback)), okButton ("OK")
{
    addAndMakeVisible (closeButton);
    addAndMakeVisible (okButton);
    closeButton.getProperties().set ("windowClose", true);
    closeButton.onClick = [this] { closeDialog(); };
    okButton.onClick = [this] { closeDialog(); };
    setWantsKeyboardFocus (true);
    setInterceptsMouseClicks (true, true);
}

void MessageDialog::setContent (juce::String newTitle, juce::String newMessage,
                                   juce::String okText, juce::String closeTooltip)
{
    title = std::move (newTitle);
    message = std::move (newMessage);
    okButton.setButtonText (std::move (okText));
    closeButton.setTooltip (std::move (closeTooltip));
    setTitle (title);
    setDescription (message);
    repaint();
}

void MessageDialog::setDarkMode (bool value) { darkMode = value; repaint(); }

void MessageDialog::setUiScale (double value)
{
    uiScale = value > 0.0 ? value : 1.0;
    resized();
    repaint();
}

int MessageDialog::scaled (int value) const
{
    return juce::roundToInt (uiScale * static_cast<double> (value));
}

int MessageDialog::edge (int value) const { return juce::jmax (1, scaled (value)); }

void MessageDialog::paint (juce::Graphics& g)
{
    const auto palette = darkMode ? darkPalette() : lightPalette();
    const auto dialog = dialogBounds();
    g.setColour (palette.modalScrim);
    g.fillRect (getLocalBounds());
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRect (dialog.translated (scaled (4), scaled (4)));
    g.setColour (palette.windowPanel);
    g.fillRect (dialog);
    drawPixelFrame (g, dialog, palette.ink, edge (Metrics::windowStroke));

    const auto titleBar = dialog.withHeight (scaled (26)).reduced (scaled (2));
    const auto caption = titleBar.withSizeKeepingCentre (scaled (240), titleBar.getHeight());
    drawTitleStripes (g, { titleBar.getX() + scaled (23), titleBar.getY() + scaled (2),
                           caption.getX() - titleBar.getX() - scaled (27), titleBar.getHeight() - scaled (4) },
                      palette.ink, uiScale);
    drawTitleStripes (g, { caption.getRight() + scaled (4), titleBar.getY() + scaled (2),
                           titleBar.getRight() - caption.getRight() - scaled (7), titleBar.getHeight() - scaled (4) },
                      palette.ink, uiScale);
    g.setColour (palette.windowPanel);
    g.fillRect (caption);
    g.setColour (palette.ink);
    g.setFont (chromeFont (13.0 * uiScale, juce::Font::bold));
    g.drawFittedText (title, caption.reduced (scaled (4), 0), juce::Justification::centred, 1);

    const auto messageArea = dialog.reduced (scaled (24), 0)
                                   .withTrimmedTop (scaled (48))
                                   .withTrimmedBottom (scaled (45));
    g.setFont (chromeFont (12.0 * uiScale));
    g.drawFittedText (message, messageArea, juce::Justification::topLeft, 6);

    if (draggingWindow)
        drawDragOutline (g, previewDialogBounds());
}

void MessageDialog::resized()
{
    const auto dialog = dialogBounds();
    closeButton.setBounds (dialog.getX() + scaled (5), dialog.getY() + scaled (5),
                           scaled (Metrics::windowControlSize), scaled (Metrics::windowControlSize));
    okButton.setBounds (dialog.getRight() - scaled (96), dialog.getBottom() - scaled (36),
                        scaled (74), scaled (26));
}

bool MessageDialog::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey || key == juce::KeyPress::returnKey)
    {
        closeDialog();
        return true;
    }
    return false;
}

void MessageDialog::mouseDown (const juce::MouseEvent& event)
{
    const auto dialog = dialogBounds();
    if (dialog.withHeight (scaled (26)).contains (event.getPosition()))
    {
        draggingWindow = true;
        dragOffset = event.getPosition() - dialog.getPosition();
        previewPositionLogical = { static_cast<double> (dialog.getX()) / uiScale,
                                   static_cast<double> (dialog.getY()) / uiScale };
        repaint();
    }
}

void MessageDialog::mouseDrag (const juce::MouseEvent& event)
{
    if (! draggingWindow)
        return;
    const auto size = juce::Point<int> (scaled (Metrics::messageDialogWidth),
                                        scaled (Metrics::messageDialogHeight));
    const auto margin = scaled (Metrics::modalWindowDragMargin);
    auto position = event.getPosition() - dragOffset;
    position.x = juce::jlimit (margin, juce::jmax (margin, getWidth() - size.x - margin), position.x);
    position.y = juce::jlimit (margin, juce::jmax (margin, getHeight() - size.y - margin), position.y);
    previewPositionLogical = { static_cast<double> (position.x) / uiScale,
                               static_cast<double> (position.y) / uiScale };
    repaint();
}

void MessageDialog::mouseUp (const juce::MouseEvent&)
{
    if (! draggingWindow)
        return;
    dialogPositionLogical = previewPositionLogical;
    hasCustomDialogPosition = true;
    draggingWindow = false;
    resized();
    repaint();
}

juce::Rectangle<int> MessageDialog::dialogBounds() const
{
    const auto width = scaled (Metrics::messageDialogWidth);
    const auto height = scaled (Metrics::messageDialogHeight);
    if (! hasCustomDialogPosition)
        return getLocalBounds().withSizeKeepingCentre (width, height);
    const auto margin = scaled (Metrics::modalWindowDragMargin);
    return { juce::jlimit (margin, juce::jmax (margin, getWidth() - width - margin),
                           juce::roundToInt (uiScale * dialogPositionLogical.x)),
             juce::jlimit (margin, juce::jmax (margin, getHeight() - height - margin),
                           juce::roundToInt (uiScale * dialogPositionLogical.y)), width, height };
}

juce::Rectangle<int> MessageDialog::previewDialogBounds() const
{
    return { juce::roundToInt (uiScale * previewPositionLogical.x),
             juce::roundToInt (uiScale * previewPositionLogical.y),
             scaled (Metrics::messageDialogWidth), scaled (Metrics::messageDialogHeight) };
}

void MessageDialog::drawDragOutline (juce::Graphics& g, juce::Rectangle<int> bounds) const
{
    const auto palette = darkMode ? darkPalette() : lightPalette();
    const auto dot = edge (Metrics::windowDragDotSize);
    const auto step = dot * 2;
    for (int x = bounds.getX(); x < bounds.getRight(); x += step)
    {
        g.setColour (((x - bounds.getX()) / step) % 2 == 0 ? palette.ink : palette.field);
        g.fillRect (x, bounds.getY(), juce::jmin (dot, bounds.getRight() - x), dot);
        g.fillRect (x, bounds.getBottom() - dot, juce::jmin (dot, bounds.getRight() - x), dot);
    }
    for (int y = bounds.getY() + dot; y < bounds.getBottom() - dot; y += step)
    {
        g.setColour (((y - bounds.getY()) / step) % 2 == 0 ? palette.ink : palette.field);
        g.fillRect (bounds.getX(), y, dot, juce::jmin (dot, bounds.getBottom() - dot - y));
        g.fillRect (bounds.getRight() - dot, y, dot, juce::jmin (dot, bounds.getBottom() - dot - y));
    }
}

void MessageDialog::closeDialog()
{
    if (close)
        close();
}

} // namespace StudioPatchbay::Design
