// Generated file. Do not edit by hand: changes are overwritten when the
// file is regenerated.

#include "PlatinumAboutDialog.h"

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

AboutDialog::AboutDialog (std::function<void()> closeCallback,
                                std::unique_ptr<juce::Drawable> lightLogoToUse,
                                std::unique_ptr<juce::Drawable> darkLogoToUse)
    : close (std::move (closeCallback)), lightLogo (std::move (lightLogoToUse)),
      darkLogo (std::move (darkLogoToUse)), okButton ("OK"),
      websiteButton ({}, {}), emailButton ({}, {}), contactSeparator ("contactSeparator", "|")
{
    for (auto* child : { static_cast<juce::Component*> (&closeButton),
                         static_cast<juce::Component*> (&okButton),
                         static_cast<juce::Component*> (&websiteButton),
                         static_cast<juce::Component*> (&emailButton),
                         static_cast<juce::Component*> (&contactSeparator) })
        addAndMakeVisible (child);
    closeButton.getProperties().set ("windowClose", true);
    closeButton.onClick = [this] { closeDialog(); };
    okButton.onClick = [this] { closeDialog(); };
    websiteButton.setJustificationType (juce::Justification::centredLeft);
    emailButton.setJustificationType (juce::Justification::centredLeft);
    contactSeparator.setJustificationType (juce::Justification::centred);
    contactSeparator.setInterceptsMouseClicks (false, false);
    setWantsKeyboardFocus (true);
    setInterceptsMouseClicks (true, true);
    applyAppearance();
}

void AboutDialog::setContent (Content newContent)
{
    content = std::move (newContent);
    okButton.setButtonText (content.okText);
    closeButton.setTooltip (content.closeTooltip);
    websiteButton.setButtonText (content.websiteLabel);
    websiteButton.setURL (content.websiteUrl);
    emailButton.setButtonText (content.emailLabel);
    emailButton.setURL (content.emailUrl);
    applyAppearance();
    resized();
    repaint();
}

void AboutDialog::setDarkMode (bool value) { darkMode = value; applyAppearance(); repaint(); }

void AboutDialog::setUiScale (double value)
{
    uiScale = value > 0.0 ? value : 1.0;
    applyAppearance();
    resized();
    repaint();
}

int AboutDialog::scaled (int value) const { return juce::roundToInt (uiScale * static_cast<double> (value)); }

int AboutDialog::logicalDialogHeight() const
{
    return Metrics::aboutDialogHeight + (content.licensingLine.isEmpty() ? 0 : Metrics::aboutLicensingRowHeight);
}

int AboutDialog::edge (int value) const { return juce::jmax (1, scaled (value)); }

void AboutDialog::paint (juce::Graphics& g)
{
    const auto palette = darkMode ? darkPalette() : lightPalette();
    const auto dialog = dialogBounds();
    g.setColour (palette.modalScrim);
    g.fillRect (getLocalBounds());
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRect (dialog.translated (scaled (4), scaled (4)));
    g.setColour (palette.windowPanel);
    g.fillRect (dialog);
    drawPixelFrame (g, dialog, palette.ink, edge (2));

    auto titleBar = dialog.withHeight (scaled (26)).reduced (scaled (2));
    const auto caption = titleBar.withSizeKeepingCentre (scaled (210), titleBar.getHeight());
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
    g.drawText (content.title, caption, juce::Justification::centred);

    const auto body = dialog.reduced (scaled (26), scaled (42));
    g.setColour (palette.field);
    g.fillRect (body);
    drawPixelFrame (g, body, palette.ink, edge());

    auto inner = body.reduced (scaled (12), scaled (12));
    auto brand = inner.removeFromLeft (scaled (Metrics::aboutBrandColumnWidth));
    g.setColour ((darkMode ? palette.highlightEdge : palette.shadowEdge)
                     .withAlpha (darkMode ? Interaction::aboutDividerAlphaDark
                                          : Interaction::aboutDividerAlphaLight));
    g.fillRect (brand.getRight(), brand.getY(), edge(), brand.getHeight());
    inner.removeFromLeft (scaled (14));

    if (auto* logo = darkMode ? darkLogo.get() : lightLogo.get())
    {
        const auto logoBounds = brand.withSizeKeepingCentre (scaled (Metrics::aboutBrandLogoWidth),
                                                              scaled (Metrics::aboutBrandLogoHeight))
                                     .translated (scaled (Metrics::aboutBrandLogoOffsetX), 0);
        logo->drawWithin (g, logoBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }

    g.setColour (palette.ink);
    g.setFont (chromeFont (18.0 * uiScale, juce::Font::bold));
    g.drawFittedText (content.productName, inner.removeFromTop (scaled (24)), juce::Justification::centredLeft, 1);
    g.setFont (chromeFont (10.5 * uiScale));
    g.drawFittedText (content.tagline, inner.removeFromTop (scaled (18)), juce::Justification::centredLeft, 1);
    inner.removeFromTop (scaled (4));
    g.setFont (chromeFont (10.5 * uiScale, juce::Font::bold));
    g.drawFittedText (content.versionLine, inner.removeFromTop (scaled (17)), juce::Justification::centredLeft, 1);
    if (content.licensingLine.isNotEmpty())
        g.setFont (chromeFont (10.5 * uiScale));
    g.drawFittedText (content.architectureLine, inner.removeFromTop (scaled (17)), juce::Justification::centredLeft, 1);
    if (content.licensingLine.isNotEmpty())
        g.drawFittedText (content.licensingLine, inner.removeFromTop (scaled (Metrics::aboutLicensingRowHeight)),
                          juce::Justification::centredLeft, 2, 1.0f);
    inner.removeFromTop (scaled (3));
    g.drawFittedText (content.publisher, inner.removeFromTop (scaled (17)), juce::Justification::centredLeft, 1);
    g.setFont (chromeFont (10.0 * uiScale));
    g.drawFittedText (content.copyright, inner.removeFromTop (scaled (16)), juce::Justification::centredLeft, 1);

    if (draggingWindow)
        drawDragOutline (g, previewDialogBounds());
}

void AboutDialog::resized()
{
    const auto dialog = dialogBounds();
    closeButton.setBounds (dialog.getX() + scaled (5), dialog.getY() + scaled (5), scaled (16), scaled (16));
    okButton.setBounds (dialog.getRight() - scaled (92), dialog.getBottom() - scaled (34), scaled (72), scaled (24));
    const auto body = dialog.reduced (scaled (26), scaled (42));
    const auto linksY = body.getBottom() - scaled (Metrics::aboutContactRowBottomInset);
    const auto linksX = body.getX() + scaled (12 + Metrics::aboutBrandColumnWidth + 15);
    const auto linkFont = chromeFont (10.0 * uiScale, juce::Font::bold);
    const auto websiteWidth = juce::GlyphArrangement::getStringWidthInt (linkFont, content.websiteLabel) + scaled (4);
    const auto emailWidth = juce::GlyphArrangement::getStringWidthInt (linkFont, content.emailLabel) + scaled (4);
    websiteButton.setBounds (linksX, linksY, websiteWidth, scaled (18));
    contactSeparator.setBounds (websiteButton.getRight(), linksY,
                                scaled (Metrics::aboutContactSeparatorWidth), scaled (18));
    emailButton.setBounds (contactSeparator.getRight(), linksY, emailWidth, scaled (18));
}

bool AboutDialog::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey || key == juce::KeyPress::returnKey)
    {
        closeDialog();
        return true;
    }
    return false;
}

void AboutDialog::mouseDown (const juce::MouseEvent& event)
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

void AboutDialog::mouseDrag (const juce::MouseEvent& event)
{
    if (! draggingWindow)
        return;
    const auto size = juce::Point<int> (scaled (Metrics::aboutDialogWidth), scaled (logicalDialogHeight()));
    const auto margin = scaled (Metrics::modalWindowDragMargin);
    auto position = event.getPosition() - dragOffset;
    position.x = juce::jlimit (margin, juce::jmax (margin, getWidth() - size.x - margin), position.x);
    position.y = juce::jlimit (margin, juce::jmax (margin, getHeight() - size.y - margin), position.y);
    previewPositionLogical = { static_cast<double> (position.x) / uiScale,
                               static_cast<double> (position.y) / uiScale };
    repaint();
}

void AboutDialog::mouseUp (const juce::MouseEvent&)
{
    if (! draggingWindow)
        return;
    dialogPositionLogical = previewPositionLogical;
    hasCustomDialogPosition = true;
    draggingWindow = false;
    resized();
    repaint();
}

juce::Rectangle<int> AboutDialog::dialogBounds() const
{
    const auto width = scaled (Metrics::aboutDialogWidth);
    const auto height = scaled (logicalDialogHeight());
    if (! hasCustomDialogPosition)
        return getLocalBounds().withSizeKeepingCentre (width, height);
    const auto margin = scaled (Metrics::modalWindowDragMargin);
    return { juce::jlimit (margin, juce::jmax (margin, getWidth() - width - margin), juce::roundToInt (uiScale * dialogPositionLogical.x)),
             juce::jlimit (margin, juce::jmax (margin, getHeight() - height - margin), juce::roundToInt (uiScale * dialogPositionLogical.y)),
             width, height };
}

juce::Rectangle<int> AboutDialog::previewDialogBounds() const
{
    return { juce::roundToInt (uiScale * previewPositionLogical.x), juce::roundToInt (uiScale * previewPositionLogical.y),
             scaled (Metrics::aboutDialogWidth), scaled (logicalDialogHeight()) };
}

void AboutDialog::drawDragOutline (juce::Graphics& g, juce::Rectangle<int> bounds) const
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

void AboutDialog::applyAppearance()
{
    const auto palette = darkMode ? darkPalette() : lightPalette();
    const auto font = chromeFont (10.0 * uiScale, juce::Font::bold);
    for (auto* button : { &websiteButton, &emailButton })
    {
        button->setFont (font, false);
        button->setColour (juce::HyperlinkButton::textColourId,
                           darkMode ? palette.ink : palette.selection);
    }
    contactSeparator.setFont (font);
    contactSeparator.setColour (juce::Label::textColourId, palette.ink.withAlpha (0.72f));
}

void AboutDialog::closeDialog()
{
    if (close)
        close();
}

} // namespace StudioPatchbay::Design
