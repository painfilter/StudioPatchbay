// Generated file. Do not edit by hand: changes are overwritten when the
// file is regenerated.

#include "PlatinumLookAndFeel.h"

#include <cmath>
#include <functional>

namespace StudioPatchbay::Design
{

namespace
{
int scaledPixel (double scale, int value) { return juce::roundToInt (static_cast<double> (value) * scale); }

void drawPixelFrame (juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour colour, int thickness)
{
    g.setColour (colour);
    for (int inset = 0; inset < thickness; ++inset)
    {
        const auto f = bounds.reduced (inset);
        if (f.isEmpty())
            break;
        g.fillRect (f.getX(), f.getY(), f.getWidth(), 1);
        g.fillRect (f.getX(), f.getBottom() - 1, f.getWidth(), 1);
        g.fillRect (f.getX(), f.getY(), 1, f.getHeight());
        g.fillRect (f.getRight() - 1, f.getY(), 1, f.getHeight());
    }
}

bool flag (const juce::Button& b, const char* name)
{
    return static_cast<bool> (b.getProperties().getWithDefault (name, false));
}
}

PlatinumLookAndFeel::PlatinumLookAndFeel()
{
    pal = lightPalette();
    refreshColours();
}

void PlatinumLookAndFeel::setDarkMode (bool shouldUseDarkMode)
{
    darkMode = shouldUseDarkMode;
    pal = darkMode ? darkPalette() : lightPalette();
    refreshColours();
}

namespace
{
    std::function<juce::Font (double, int)> chromeFontProvider;
}

void setChromeFontProvider (std::function<juce::Font (double, int)> provider)
{
    chromeFontProvider = std::move (provider);
}

juce::Font chromeFont (double height, int styleFlags)
{
    if (chromeFontProvider)
        return chromeFontProvider (height, styleFlags);

    return juce::Font (juce::FontOptions (static_cast<float> (height), styleFlags));
}

int buttonTextStyleFlags (const juce::TextButton& button)
{
    return flag (button, "menuBar") || flag (button, "regularButtonText")
        ? juce::Font::plain : juce::Font::bold;
}

void PlatinumLookAndFeel::setUiScale (double newScale) { uiScale = newScale > 0.0 ? newScale : 1.0; }

int PlatinumLookAndFeel::scaled (int value) const { return scaledPixel (uiScale, value); }

int PlatinumLookAndFeel::edge (int logicalThickness) const { return juce::jmax (1, scaled (logicalThickness)); }

void PlatinumLookAndFeel::drawHardFrame (juce::Graphics& g, juce::Rectangle<int> bounds,
                                    juce::Colour colour, int thickness)
{
    drawPixelFrame (g, bounds, colour, thickness);
}

void drawTitleStripes (juce::Graphics& g, juce::Rectangle<int> area,
                       juce::Colour colour, double uiScale)
{
    if (area.getWidth() <= 0 || area.getHeight() <= 0)
        return;

    const auto line  = juce::jmax (1, juce::roundToInt (uiScale));
    const auto pitch = line * 3;

    g.setColour (colour);
    for (int y = area.getY(); y + line <= area.getBottom(); y += pitch)
        g.fillRect (area.getX(), y, area.getWidth(), line);
}

void PlatinumLookAndFeel::refreshColours()
{
    setColour (juce::ResizableWindow::backgroundColourId, panel());
    setColour (juce::DocumentWindow::textColourId, ink());

    setColour (juce::PopupMenu::backgroundColourId, panel());
    setColour (juce::PopupMenu::textColourId, ink());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, selection());
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::PopupMenu::headerTextColourId, ink());

    setColour (juce::TextEditor::backgroundColourId, field());
    setColour (juce::TextEditor::textColourId, ink());
    setColour (juce::TextEditor::highlightColourId, selection());
    setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextEditor::outlineColourId, ink());
    setColour (juce::TextEditor::focusedOutlineColourId, selection());
    setColour (juce::CaretComponent::caretColourId, ink());

    setColour (juce::TextButton::buttonColourId, surface());
    setColour (juce::TextButton::buttonOnColourId, surface());
    setColour (juce::TextButton::textColourOffId, ink());
    setColour (juce::TextButton::textColourOnId, ink());

    setColour (juce::ComboBox::backgroundColourId, field());
    setColour (juce::ComboBox::textColourId, ink());
    setColour (juce::ComboBox::outlineColourId, ink());
    setColour (juce::ComboBox::arrowColourId, ink());
    setColour (juce::ComboBox::focusedOutlineColourId, selection());

    setColour (juce::Label::textColourId, ink());
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    setColour (juce::AlertWindow::backgroundColourId, panel());
    setColour (juce::AlertWindow::textColourId, ink());
    setColour (juce::AlertWindow::outlineColourId, ink());

    setColour (juce::ScrollBar::thumbColourId, shadow());
    setColour (juce::ScrollBar::trackColourId, panel());

    setColour (juce::ToggleButton::textColourId, ink());
    setColour (juce::ToggleButton::tickColourId, ink());
}

int PlatinumLookAndFeel::getDefaultScrollbarWidth()
{
    return scaled (14);
}

void PlatinumLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&,
                                    int x, int y, int width, int height, bool vertical,
                                    int thumbStart, int thumbSize, bool over, bool down)
{
    const juce::Rectangle<int> bounds { x, y, width, height };
    const auto border = edge();

    g.setColour (pal.scrollbarTrack);
    g.fillRect (bounds);
    drawHardFrame (g, bounds, darkMode ? highlight() : ink(), border);
    auto track = bounds.reduced (border);
    g.setColour (pal.scrollbarTrack);
    g.fillRect (track);

    if (thumbSize <= 0)
        return;

    auto thumb = vertical ? juce::Rectangle<int> { x + border, thumbStart, width - border * 2, thumbSize }
                          : juce::Rectangle<int> { thumbStart, y + border, thumbSize, height - border * 2 };
    thumb = thumb.getIntersection (track);
    if (thumb.isEmpty())
        return;

    const auto thumbBase = pal.scrollbarThumb;
    const auto thumbFill = down ? thumbBase.darker (0.17f) : (over ? thumbBase.brighter (0.10f) : thumbBase);
    g.setColour (thumbFill);
    g.fillRect (thumb);

    drawHardFrame (g, thumb, ink(), border);
    auto inner = thumb.reduced (border);
    g.setColour (down ? shadow() : highlight());
    g.fillRect (inner.getX(), inner.getY(), inner.getWidth(), border);
    g.fillRect (inner.getX(), inner.getY(), border, inner.getHeight());
    g.setColour (down ? highlight() : shadow());
    g.fillRect (inner.getX(), inner.getBottom() - border, inner.getWidth(), border);
    g.fillRect (inner.getRight() - border, inner.getY(), border, inner.getHeight());

    g.setColour (darkMode ? juce::Colour (0xff363a35) : juce::Colour (0xff676761));
    if (vertical)
    {
        const auto mid = thumb.getCentreY();
        g.fillRect (thumb.getX() + border * 2, mid - border * 2, thumb.getWidth() - border * 4, border);
        g.fillRect (thumb.getX() + border * 2, mid + border, thumb.getWidth() - border * 4, border);
    }
    else
    {
        const auto mid = thumb.getCentreX();
        g.fillRect (mid - border * 2, thumb.getY() + border * 2, border, thumb.getHeight() - border * 4);
        g.fillRect (mid + border, thumb.getY() + border * 2, border, thumb.getHeight() - border * 4);
    }
}

void PlatinumLookAndFeel::drawPlatinumBevel (juce::Graphics& g, juce::Rectangle<int> bounds,
                                        bool down, bool over, bool enabled)
{
    const auto edgeWidth = edge();
    drawHardFrame (g, bounds, ink(), edgeWidth);
    auto inner = bounds.reduced (edgeWidth);
    g.setColour (enabled ? (over && ! down ? surface().brighter (0.08f) : surface())
                         : surface().darker (0.04f));
    g.fillRect (inner);
    g.setColour (down ? shadow() : highlight());
    g.fillRect (inner.getX(), inner.getY(), inner.getWidth(), edgeWidth);
    g.fillRect (inner.getX(), inner.getY(), edgeWidth, inner.getHeight());
    g.setColour (down ? highlight() : shadow());
    g.fillRect (inner.getX(), inner.getBottom() - edgeWidth, inner.getWidth(), edgeWidth);
    g.fillRect (inner.getRight() - edgeWidth, inner.getY(), edgeWidth, inner.getHeight());
}

void PlatinumLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                           const juce::Colour&, bool over, bool down)
{
    const auto bounds = button.getLocalBounds();
    if (flag (button, "clockDisplay"))
        return;

    if (flag (button, "windowClose"))
    {
        g.setColour (down ? shadow() : panel());
        g.fillRect (bounds);
        drawHardFrame (g, bounds, ink(), edge());
        const auto off = down ? scaled (1) : 0;
        drawHardFrame (g, bounds.reduced (scaled (3)).translated (off, off), ink(), edge());
        return;
    }
    if (flag (button, "presetDisplay"))
    {
        g.setColour (field());
        g.fillRect (bounds);
        drawHardFrame (g, bounds, ink(), edge());
        g.setColour (shadow());
        g.fillRect (edge(), edge(), bounds.getWidth() - edge (2), edge());
        g.fillRect (edge(), edge(), edge(), bounds.getHeight() - edge (2));
        const auto arrowWidth = scaled (21);
        drawPlatinumBevel (g, { bounds.getWidth() - arrowWidth, 0, arrowWidth, bounds.getHeight() },
                           down, over, true);
        g.setColour (ink());
        juce::Path arrow;
        arrow.addTriangle (static_cast<float> (bounds.getWidth() - scaled (15)), bounds.getHeight() * 0.40f,
                           static_cast<float> (bounds.getWidth() - scaled (7)),  bounds.getHeight() * 0.40f,
                           static_cast<float> (bounds.getWidth() - scaled (11)), bounds.getHeight() * 0.64f);
        g.fillPath (arrow);
        return;
    }
    if (flag (button, "menuBar"))
    {
        if (over || down || button.getToggleState())
        {
            g.setColour (selection());
            g.fillRect (bounds);
        }
        return;
    }
    if (flag (button, "shadedWindow"))
    {
        g.setColour (panel()); g.fillRect (bounds);
        drawHardFrame (g, bounds, ink(), edge (2));
        for (int y = scaled (4); y < bounds.getHeight() - scaled (3); y += scaled (3))
            g.fillRect (scaled (4), y, bounds.getWidth() - scaled (8), edge());
        g.setColour (panel());
        g.fillRect (bounds.withSizeKeepingCentre (bounds.getWidth() - scaled (48),
                                                  bounds.getHeight() - scaled (4)));
        g.setColour (ink());
        drawHardFrame (g, { scaled (5), scaled (5), scaled (14), scaled (14) }, ink(), edge());
        drawHardFrame (g, { bounds.getWidth() - scaled (19), scaled (5), scaled (14), scaled (14) }, ink(), edge());
        return;
    }
    if (flag (button, "windowShade"))
    {
        g.setColour (down ? shadow() : panel());
        g.fillRect (bounds);
        drawHardFrame (g, bounds, ink(), edge());
        g.setColour (ink());
        const auto off = down ? scaled (1) : 0;
        g.fillRect (bounds.getX() + scaled (3) + off, bounds.getCentreY() + off,
                    bounds.getWidth() - scaled (6), edge());
        return;
    }

    drawPlatinumBevel (g, bounds, down || button.getToggleState(), over, button.isEnabled());
}

void PlatinumLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    if (flag (button, "clockDisplay") || flag (button, "windowClose") || flag (button, "windowShade"))
        return;
    if (flag (button, "presetDisplay"))
    {
        g.setColour (button.isEnabled() ? ink() : disabledInk());
        g.setFont (chromeFont (12.0 * uiScale));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds().withTrimmedLeft (scaled (6)).withTrimmedRight (scaled (25)),
                          juce::Justification::centredLeft, 1);
        return;
    }
    const auto menuBar = flag (button, "menuBar");
    g.setColour ((menuBar && (button.isDown() || button.isMouseOver() || button.getToggleState()))
                     ? juce::Colours::white
                     : (button.isEnabled() ? ink() : disabledInk()));
    if (flag (button, "shadedWindow"))
        g.setColour (ink());
    g.setFont (chromeFont ((menuBar ? 12.0 : 11.5) * uiScale,
                           buttonTextStyleFlags (button)));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (scaled (3)),
                      juce::Justification::centred, 1);
}

void PlatinumLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor&)
{
    g.setColour (field()); g.fillRect (0, 0, width, height);
}

void PlatinumLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    const auto focused = editor.hasKeyboardFocus (true);
    drawHardFrame (g, { 0, 0, width, height }, focused ? selection() : ink(), focused ? edge (2) : edge());
    if (! focused)
    {
        g.setColour (shadow());
        g.fillRect (edge(), edge(), width - edge (2), edge());
        g.fillRect (edge(), edge(), edge(), height - edge (2));
    }
}

void PlatinumLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.setColour (panel()); g.fillRect (0, 0, width, height);
    drawHardFrame (g, { 0, 0, width, height }, ink(), edge());
    g.setColour (highlight());
    g.fillRect (edge(), edge(), width - edge (2), edge());
    g.fillRect (edge(), edge(), edge(), height - edge (2));
    g.setColour (shadow());
    g.fillRect (edge(), height - edge (2), width - edge (2), edge());
    g.fillRect (width - edge (2), edge(), edge(), height - edge (2));
}

void PlatinumLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                        bool separator, bool active, bool highlighted, bool ticked,
                                        bool hasSubMenu, const juce::String& text, const juce::String& shortcut,
                                        const juce::Drawable*, const juce::Colour*)
{
    if (separator)
    {
        g.setColour (shadow());
        g.fillRect (area.getX() + scaled (3), area.getCentreY(), area.getWidth() - scaled (6), edge());
        g.setColour (highlight());
        g.fillRect (area.getX() + scaled (3), area.getCentreY() + edge(), area.getWidth() - scaled (6), edge());
        return;
    }
    if (highlighted && active)
    {
        g.setColour (selection());
        g.fillRect (area.reduced (scaled (2), 0));
    }
    g.setColour (! active ? disabledInk() : (highlighted ? juce::Colours::white : ink()));
    g.setFont (chromeFont (12.0 * uiScale));
    auto row = area;
    if (ticked)
        g.drawText (juce::CharPointer_UTF8 ("\xe2\x9c\x93"), row.removeFromLeft (scaled (20)),
                    juce::Justification::centred);
    auto textArea = area.reduced (scaled (22), 0);
    g.drawText (text, textArea, juce::Justification::centredLeft, true);
    if (shortcut.isNotEmpty())
        g.drawText (shortcut, textArea, juce::Justification::centredRight, true);
    if (hasSubMenu)
        g.drawText (juce::CharPointer_UTF8 ("\xe2\x96\xb6"), row.removeFromRight (scaled (16)),
                    juce::Justification::centred);
}

void PlatinumLookAndFeel::getIdealPopupMenuItemSize (const juce::String& text, bool separator,
                                                int standardMenuItemHeight,
                                                int& idealWidth, int& idealHeight)
{
    if (separator)
    {
        idealWidth = scaled (50);
        idealHeight = standardMenuItemHeight > 0
                          ? juce::jmax (edge (2), standardMenuItemHeight / 10)
                          : scaled (10);
        return;
    }
    const auto font = getPopupMenuFont();
    idealHeight = standardMenuItemHeight > 0
                      ? standardMenuItemHeight
                      : juce::roundToInt (font.getHeight() * 1.3f);
    idealWidth = juce::GlyphArrangement::getStringWidthInt (font, text) + scaled (44);
}

int PlatinumLookAndFeel::getPopupMenuBorderSize() { return scaled (2); }

void PlatinumLookAndFeel::drawAlertBox (juce::Graphics& g, juce::AlertWindow& alert,
                                   const juce::Rectangle<int>& textArea, juce::TextLayout& layout)
{
    auto b = alert.getLocalBounds();
    g.setColour (panel());
    g.fillRect (b);
    g.setColour (highlight());
    g.fillRect (b.getX() + edge(), b.getY() + edge(), b.getWidth() - edge (2), edge());
    g.fillRect (b.getX() + edge(), b.getY() + edge(), edge(), b.getHeight() - edge (2));
    g.setColour (shadow());
    g.fillRect (b.getX() + edge(), b.getBottom() - edge (2), b.getWidth() - edge (2), edge());
    g.fillRect (b.getRight() - edge (2), b.getY() + edge(), edge(), b.getHeight() - edge (2));
    drawHardFrame (g, b, ink(), edge (2));
    g.setColour (ink());
    layout.draw (g, textArea.toFloat());
}

void PlatinumLookAndFeel::drawDocumentWindowTitleBar (juce::DocumentWindow& window, juce::Graphics& g,
                                                 int w, int h, int, int, const juce::Image*, bool)
{
    const auto edgeW = edge();
    g.setColour (panel());
    g.fillRect (0, 0, w, h);

    const auto name = window.getName();
    const auto font = chromeFont (13.0 * uiScale, juce::Font::bold);
    const auto textW = juce::jlimit (scaled (60), juce::jmax (scaled (60), w - scaled (60)),
                                     juce::GlyphArrangement::getStringWidthInt (font, name) + scaled (24));
    const auto title = juce::Rectangle<int> (0, 0, w, h).reduced (scaled (2));
    const auto titleText = juce::Rectangle<int> (0, 0, textW, title.getHeight())
                               .withCentre ({ w / 2, title.getCentreY() });
    const auto leftStripes = juce::Rectangle<int>::leftTopRightBottom (
        title.getX() + scaled (22), title.getY(), titleText.getX() - scaled (4), title.getBottom());
    const auto rightStripes = juce::Rectangle<int>::leftTopRightBottom (
        titleText.getRight() + scaled (4), title.getY(), title.getRight(), title.getBottom());
    for (const auto stripes : { leftStripes, rightStripes })
        drawTitleStripes (g, stripes.withTrimmedTop (scaled (2)).withTrimmedBottom (edgeW),
                          ink(), uiScale);
    g.setColour (panel());
    g.fillRect (titleText);
    g.setColour (ink());
    g.setFont (font);
    g.drawFittedText (name, titleText.reduced (scaled (6), 0), juce::Justification::centred, 1);
    g.fillRect (0, h - edgeW, w, edgeW);
}

juce::Button* PlatinumLookAndFeel::createDocumentWindowButton (int)
{
    auto* b = new juce::TextButton();
    b->getProperties().set ("windowClose", true);
    return b;
}

void PlatinumLookAndFeel::positionDocumentWindowButtons (juce::DocumentWindow&, int titleBarX, int titleBarY,
                                                    int, int titleBarH, juce::Button* minimise,
                                                    juce::Button* maximise, juce::Button* close, bool)
{
    const auto s = scaled (16);
    const auto m = scaled (4);
    if (close != nullptr)
        close->setBounds (titleBarX + m, titleBarY + (titleBarH - s) / 2, s, s);
    juce::ignoreUnused (minimise, maximise);
}

juce::Font PlatinumLookAndFeel::getLabelFont (juce::Label& label)
{
    const auto asked = label.getFont();

    const auto bold = asked.isBold() || asked.getTypefaceStyle().containsIgnoreCase ("bold");
    return chromeFont (asked.getHeight(), bold ? juce::Font::bold : juce::Font::plain);
}

juce::Font PlatinumLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return chromeFont (juce::jmin (15.0, buttonHeight * 0.6));
}

juce::Font PlatinumLookAndFeel::getPopupMenuFont()
{
    return chromeFont (12.0 * uiScale);
}

juce::Font PlatinumLookAndFeel::getAlertWindowTitleFont()
{
    return chromeFont (13.0 * uiScale, juce::Font::bold);
}

juce::Font PlatinumLookAndFeel::getAlertWindowMessageFont()
{
    return chromeFont (12.0 * uiScale);
}

juce::Font PlatinumLookAndFeel::getAlertWindowFont()
{
    return chromeFont (12.0 * uiScale);
}

void paintDesktopPattern (juce::Graphics& g, juce::Colour dot, const DesktopPattern& p,
                          juce::Rectangle<int> covered)
{
    const auto clip = g.getClipBounds();
    if (covered.contains (clip))
        return;
    const auto step = juce::jmax (1, p.step);
    g.setColour (dot);
    const auto y0 = juce::jmax (p.top, clip.getY() - (clip.getY() - p.top) % step);
    for (int y = y0; y < clip.getBottom(); y += step)
    {
        const auto offset = (y / step) % 2 == 0 ? 0 : p.rowOffset;
        const auto xFrom = clip.getX() - ((clip.getX() - offset) % step + step) % step;
        for (int x = juce::jmax (offset, xFrom); x < clip.getRight(); x += step)
            if (! covered.contains (x, y))
                g.fillRect (x, y, p.dotSize, p.dotSize);
    }
}

} // namespace StudioPatchbay::Design
