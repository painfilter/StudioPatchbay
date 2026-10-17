// Generated file. Do not edit by hand: changes are overwritten when the
// file is regenerated.

#include "PlatinumOverlayWindow.h"

namespace StudioPatchbay::Design
{

namespace
{
void drawPixelFrame (juce::Graphics& g, juce::Rectangle<int> b, juce::Colour c, int thickness)
{
    g.setColour (c);
    for (int i = 0; i < thickness; ++i)
    {
        const auto f = b.reduced (i);
        if (f.isEmpty()) break;
        g.fillRect (f.getX(), f.getY(), f.getWidth(), 1);
        g.fillRect (f.getX(), f.getBottom() - 1, f.getWidth(), 1);
        g.fillRect (f.getX(), f.getY(), 1, f.getHeight());
        g.fillRect (f.getRight() - 1, f.getY(), 1, f.getHeight());
    }
}
}

OverlayWindow::OverlayWindow (juce::String titleIn, std::unique_ptr<juce::Component> contentIn,
                                    int contentW, int contentH)
    : title (std::move (titleIn)), content (std::move (contentIn)),
      contentWidth (contentW), contentHeight (contentH)
{
    closeBox.getProperties().set ("windowClose", true);
    closeBox.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeBox);
    if (content != nullptr)
        addAndMakeVisible (*content);
    setInterceptsMouseClicks (true, true);
    setWantsKeyboardFocus (true);
}

void OverlayWindow::setDarkMode (bool value) { darkMode = value; repaint(); }

void OverlayWindow::setCloseTooltip (const juce::String& text) { closeBox.setTooltip (text); }

void OverlayWindow::setUiScale (double newScale)
{
    uiScale = juce::jmax (0.5, newScale);
    resized();
    repaint();
}

juce::Rectangle<int> OverlayWindow::dialogBounds() const
{
    const int cw = juce::roundToInt (contentWidth * uiScale);
    const int ch = juce::roundToInt (contentHeight * uiScale);
    const int w = cw + 2 * edge (2);
    const int h = scaled (25) + ch + 2 * edge (2);
    if (! hasCustomPos)
        return getLocalBounds().withSizeKeepingCentre (w, h);
    const int m = scaled (4);
    const int x = juce::jlimit (m, juce::jmax (m, getWidth() - w - m), juce::roundToInt (posLogical.x * uiScale));
    const int y = juce::jlimit (m, juce::jmax (m, getHeight() - h - m), juce::roundToInt (posLogical.y * uiScale));
    return { x, y, w, h };
}

juce::Rectangle<int> OverlayWindow::previewBounds() const
{
    const auto d = dialogBounds();
    return { juce::roundToInt (previewLogical.x * uiScale), juce::roundToInt (previewLogical.y * uiScale),
             d.getWidth(), d.getHeight() };
}

juce::Rectangle<int> OverlayWindow::titleBar (juce::Rectangle<int> dialog) const
{
    return dialog.withHeight (scaled (25));
}

void OverlayWindow::resized()
{
    const auto dialog = dialogBounds();
    closeBox.setBounds (dialog.getX() + scaled (5), dialog.getY() + scaled (5), scaled (16), scaled (16));
    if (content != nullptr)
    {
        const auto s = static_cast<float> (uiScale);
        content->setTransform (juce::AffineTransform::scale (s));
        content->setBounds (juce::roundToInt ((dialog.getX() + edge (2)) / uiScale),
                            juce::roundToInt ((dialog.getY() + scaled (25)) / uiScale),
                            contentWidth, contentHeight);
    }
}

void OverlayWindow::paint (juce::Graphics& g)
{
    const auto pal = darkMode ? darkPalette() : lightPalette();
    g.fillAll (pal.modalScrim);
    auto dialog = dialogBounds();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (dialog.translated (scaled (4), scaled (4)));
    g.setColour (pal.windowPanel);
    g.fillRect (dialog);
    drawPixelFrame (g, dialog, pal.ink, edge (2));

    auto tb = titleBar (dialog).reduced (scaled (2));

    const auto titleFont = chromeFont (13.0 * uiScale, juce::Font::bold);
    const int cutoutW = juce::jlimit (scaled (60), juce::jmax (scaled (60), tb.getWidth() - scaled (52)),
                                      juce::GlyphArrangement::getStringWidthInt (titleFont, title) + scaled (28));
    const auto titleText = tb.withSizeKeepingCentre (cutoutW, tb.getHeight());
    const auto leftStripes = juce::Rectangle<int>::leftTopRightBottom (
        tb.getX() + scaled (23), tb.getY(), titleText.getX() - scaled (4), tb.getBottom());
    const auto rightStripes = juce::Rectangle<int>::leftTopRightBottom (
        titleText.getRight() + scaled (4), tb.getY(), tb.getRight() - scaled (3), tb.getBottom());

    for (const auto stripes : { leftStripes, rightStripes })
        drawTitleStripes (g, stripes.withTrimmedTop (scaled (2))
                                         .withTrimmedBottom (edge()), pal.ink, uiScale);
    g.setColour (pal.windowPanel);
    g.fillRect (titleText);
    g.setColour (pal.ink);
    g.setFont (titleFont);
    g.drawText (title, titleText.reduced (scaled (6), 0), juce::Justification::centred);

    if (dragging)
        drawDragOutline (g, previewBounds());
}

void OverlayWindow::drawDragOutline (juce::Graphics& g, juce::Rectangle<int> b) const
{
    const int dot = juce::jmax (1, scaled (1)), step = dot * 2;
    for (int x = b.getX(); x < b.getRight(); x += step)
    {
        g.setColour (((x - b.getX()) / step) % 2 == 0 ? juce::Colours::black : juce::Colours::white);
        g.fillRect (x, b.getY(), dot, dot);
        g.fillRect (x, b.getBottom() - dot, dot, dot);
    }
    for (int y = b.getY() + dot; y < b.getBottom() - dot; y += step)
    {
        g.setColour (((y - b.getY()) / step) % 2 == 0 ? juce::Colours::black : juce::Colours::white);
        g.fillRect (b.getX(), y, dot, dot);
        g.fillRect (b.getRight() - dot, y, dot, dot);
    }
}

void OverlayWindow::mouseDown (const juce::MouseEvent& e)
{
    const auto dialog = dialogBounds();
    if (titleBar (dialog).contains (e.getPosition()))
    {
        dragging = true;
        dragOffset = e.getPosition() - dialog.getPosition();
        previewLogical = { dialog.getX() / uiScale, dialog.getY() / uiScale };
        repaint (previewBounds().expanded (2));
    }
}

void OverlayWindow::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const auto oldOutline = previewBounds();
    const auto d = dialogBounds();
    const auto m = scaled (4);
    auto p = e.getPosition() - dragOffset;
    p.x = juce::jlimit (m, juce::jmax (m, getWidth() - d.getWidth() - m), p.x);
    p.y = juce::jlimit (m, juce::jmax (m, getHeight() - d.getHeight() - m), p.y);
    previewLogical = { p.x / uiScale, p.y / uiScale };
    repaint (oldOutline.getUnion (previewBounds()).expanded (2));
}

void OverlayWindow::mouseUp (const juce::MouseEvent&)
{
    if (! dragging) return;
    posLogical = previewLogical;
    hasCustomPos = true;
    dragging = false;
    resized();
    repaint();
}

bool OverlayWindow::keyPressed (const juce::KeyPress& k)
{
    if (content != nullptr && content->keyPressed (k))
        return true;

    if (k == juce::KeyPress::escapeKey)
    {
        if (onClose) onClose();
        return true;
    }
    return false;
}

std::vector<juce::Rectangle<int>> drawTabStrip (juce::Graphics& g,
                                                juce::Rectangle<int> strip,
                                                const juce::StringArray& names,
                                                int activeIndex,
                                                const Palette& pal,
                                                double uiScale)
{
    std::vector<juce::Rectangle<int>> hits;
    const int n = names.size();
    if (n <= 0 || strip.isEmpty())
        return hits;

    g.setColour (pal.ink);
    g.fillRect (strip.getX(), strip.getBottom() - 1, strip.getWidth(), 1);

    const float tw = static_cast<float> (strip.getWidth()) / static_cast<float> (n);
    hits.reserve (static_cast<size_t> (n));
    for (int i = 0; i < n; ++i)
        hits.push_back (juce::Rectangle<float> (strip.getX() + i * tw,
                                                static_cast<float> (strip.getY()), tw,
                                                static_cast<float> (strip.getHeight()))
                            .toNearestInt());

    for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < n; ++i)
        {
            const bool sel = (i == activeIndex);
            if (sel != (pass == 1))
                continue;

            const float top = static_cast<float> (strip.getY()) + (sel ? 0.0f : 4.0f);
            const juce::Rectangle<float> r (strip.getX() + i * tw, top, tw,
                                            static_cast<float> (strip.getBottom()) - top);
            const float slant = r.getHeight() * 0.34f;
            juce::Path p;
            p.startNewSubPath (r.getX() + 1.0f, r.getBottom());
            p.lineTo (r.getX() + slant, r.getY() + 1.0f);
            p.lineTo (r.getRight() - slant, r.getY() + 1.0f);
            p.lineTo (r.getRight() - 1.0f, r.getBottom());
            g.setColour (sel ? pal.windowPanel : pal.controlSurface.darker (0.14f));
            g.fillPath (p);
            if (sel)
            {
                g.setColour (pal.windowPanel);
                g.fillRect (r.getX() + 1.5f, r.getBottom() - 1.5f, r.getWidth() - 3.0f, 2.0f);
                g.setColour (juce::Colours::white.withAlpha (0.55f));
                g.drawLine (r.getX() + slant + 1.0f, r.getY() + 2.0f,
                            r.getRight() - slant - 1.0f, r.getY() + 2.0f, 1.0f);
            }
            g.setColour (sel ? pal.ink : pal.ink.withAlpha (0.85f));
            g.strokePath (p, juce::PathStrokeType (sel ? 1.4f : 1.0f));
            g.setColour (sel ? pal.ink : pal.inkSoft);
            g.setFont (chromeFont ((sel ? 11.5 : 10.5) * uiScale));
            g.drawText (names[i], r.withTrimmedBottom (2).toNearestInt(),
                        juce::Justification::centred);
        }

    return hits;
}

} // namespace StudioPatchbay::Design
