#include "DesktopIcons.h"
#include "Design/PlatinumLookAndFeel.h"

namespace StudioPatchbay::DesktopIcons
{

namespace
{
// The icon area in logical units: every shape below is placed on this grid
// and scaled with it, so all zoom levels keep the same drawing.
struct Grid
{
    juce::Rectangle<int> area;
    float unit;

    juce::Rectangle<float> rect (float x, float y, float w, float h) const
    {
        return { area.getX() + x * unit, area.getY() + y * unit, w * unit, h * unit };
    }
    float line() const { return juce::jmax (1.0f, std::round (unit)); }
};

Grid gridFor (juce::Rectangle<int> area) { return { area, area.getWidth() / 32.0f }; }

Design::Palette palette (bool dark) { return dark ? Design::darkPalette() : Design::lightPalette(); }

// A raised Platinum body: surface fill, light top/left, dark bottom/right, ink outline.
void bevelBox (juce::Graphics& g, const Grid& grid, juce::Rectangle<float> r, const Design::Palette& p)
{
    const auto l = grid.line();
    g.setColour (p.controlSurface);
    g.fillRect (r);
    g.setColour (p.highlightEdge);
    g.fillRect (r.getX() + l, r.getY() + l, r.getWidth() - 2 * l, l);
    g.fillRect (r.getX() + l, r.getY() + l, l, r.getHeight() - 2 * l);
    g.setColour (p.shadowEdge);
    g.fillRect (r.getX() + l, r.getBottom() - 2 * l, r.getWidth() - 2 * l, l);
    g.fillRect (r.getRight() - 2 * l, r.getY() + l, l, r.getHeight() - 2 * l);
    g.setColour (p.ink);
    g.drawRect (r, l);
}

void shadowUnder (juce::Graphics& g, const Grid& grid, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (r.translated (grid.line(), grid.line()));
}
} // namespace

void volume (juce::Graphics& g, juce::Rectangle<int> area, bool dark)
{
    const auto grid = gridFor (area);
    const auto p = palette (dark);
    const auto body = grid.rect (2, 9, 28, 14);
    shadowUnder (g, grid, body);
    bevelBox (g, grid, body, p);
    // Drive slot and activity light.
    g.setColour (p.ink);
    g.fillRect (grid.rect (6, 15, 14, 1));
    g.setColour (p.selection);
    g.fillRect (grid.rect (24, 14, 3, 3));
}

void printer (juce::Graphics& g, juce::Rectangle<int> area, bool dark)
{
    const auto grid = gridFor (area);
    const auto p = palette (dark);
    // The sheet coming out of the top.
    const auto sheet = grid.rect (9, 1, 14, 12);
    g.setColour (p.field);
    g.fillRect (sheet);
    g.setColour (p.ink);
    g.drawRect (sheet, grid.line());
    for (int i = 0; i < 3; ++i)
        g.fillRect (grid.rect (11, 4.0f + 2.5f * i, 10, 1));
    const auto body = grid.rect (3, 11, 26, 12);
    shadowUnder (g, grid, body);
    bevelBox (g, grid, body, p);
    g.setColour (p.ink);
    g.fillRect (grid.rect (7, 18, 18, 1));
    g.setColour (p.selection);
    g.fillRect (grid.rect (24, 14, 2, 2));
}

void camera (juce::Graphics& g, juce::Rectangle<int> area, bool dark)
{
    const auto grid = gridFor (area);
    const auto p = palette (dark);
    const auto body = grid.rect (2, 7, 28, 17);
    shadowUnder (g, grid, body);
    bevelBox (g, grid, grid.rect (20, 4, 7, 4), p);   // flash
    bevelBox (g, grid, body, p);
    const auto lens = grid.rect (10, 9, 12, 12);
    g.setColour (p.ink);
    g.fillEllipse (lens);
    g.setColour (p.selection);
    g.fillEllipse (lens.reduced (2.5f * grid.unit));
    g.setColour (p.highlightEdge);
    g.fillEllipse (grid.rect (13, 12, 2.5f, 2.5f));
}

void watcher (juce::Graphics& g, juce::Rectangle<int> area, bool dark, bool watching)
{
    const auto grid = gridFor (area);
    const auto p = palette (dark);
    const auto l = grid.line();
    juce::Path eye;
    const auto e = grid.rect (2, 6, 28, 15);
    eye.startNewSubPath (e.getX(), e.getCentreY());
    eye.quadraticTo (e.getCentreX(), e.getY() - 4 * grid.unit, e.getRight(), e.getCentreY());
    eye.quadraticTo (e.getCentreX(), e.getBottom() + 4 * grid.unit, e.getX(), e.getCentreY());
    eye.closeSubPath();
    g.setColour (p.field);
    g.fillPath (eye);
    g.setColour (p.ink);
    g.strokePath (eye, juce::PathStrokeType (l));
    if (watching)
    {
        const auto iris = grid.rect (11, 8, 10, 11);
        g.setColour (p.selection);
        g.fillEllipse (iris);
        g.setColour (p.ink);
        g.fillEllipse (iris.reduced (3 * grid.unit));
        g.setColour (p.highlightEdge);
        g.fillEllipse (grid.rect (13, 10, 2, 2));
    }
    else
    {
        // Closed: the lid line across the middle.
        g.fillRect (e.getX() + 3 * grid.unit, e.getCentreY(), e.getWidth() - 6 * grid.unit, l);
    }
}

void document (juce::Graphics& g, juce::Rectangle<int> area, bool dark, const juce::String& kind)
{
    const auto grid = gridFor (area);
    const auto p = palette (dark);
    const auto l = grid.line();
    const auto page = grid.rect (8, 1, 17, 24);
    const auto ear = 5 * grid.unit;

    juce::Path outline;
    outline.startNewSubPath (page.getX(), page.getY());
    outline.lineTo (page.getRight() - ear, page.getY());
    outline.lineTo (page.getRight(), page.getY() + ear);
    outline.lineTo (page.getRight(), page.getBottom());
    outline.lineTo (page.getX(), page.getBottom());
    outline.closeSubPath();

    shadowUnder (g, grid, page);
    g.setColour (p.field);
    g.fillPath (outline);
    g.setColour (p.ink);
    g.strokePath (outline, juce::PathStrokeType (l));
    g.drawLine (page.getRight() - ear, page.getY(), page.getRight() - ear, page.getY() + ear, l);
    g.drawLine (page.getRight() - ear, page.getY() + ear, page.getRight(), page.getY() + ear, l);

    // The format as a small label band.
    const auto band = grid.rect (5, 14, 17, 7);
    g.setColour (p.selection);
    g.fillRect (band);
    g.setColour (juce::Colours::white);
    g.setFont (Design::chromeFont (6.0 * grid.unit, juce::Font::bold));
    g.drawText (kind.toUpperCase(), band.toNearestInt(), juce::Justification::centred, false);
}

void trash (juce::Graphics& g, juce::Rectangle<int> area, bool dark)
{
    const auto grid = gridFor (area);
    const auto p = palette (dark);
    const auto l = grid.line();
    // A discarded patch sheet behind the lid, then lid and handle.
    juce::Path sheet;
    sheet.addRectangle (grid.rect (17, -1, 9, 7));
    sheet.applyTransform (juce::AffineTransform::rotation (0.35f, grid.rect (21, 6, 0, 0).getX(),
                                                           grid.rect (21, 6, 0, 0).getY()));
    g.setColour (p.field);
    g.fillPath (sheet);
    g.setColour (p.ink);
    g.strokePath (sheet, juce::PathStrokeType (l));
    bevelBox (g, grid, grid.rect (7, 3, 18, 3), p);
    g.setColour (p.ink);
    g.drawRect (grid.rect (13, 1, 6, 2.5f), l);

    // Tapered can.
    juce::Path can;
    can.startNewSubPath (grid.rect (9, 6, 0, 0).getPosition());
    can.lineTo (grid.rect (23, 6, 0, 0).getPosition());
    can.lineTo (grid.rect (21.5f, 26, 0, 0).getPosition());
    can.lineTo (grid.rect (10.5f, 26, 0, 0).getPosition());
    can.closeSubPath();
    g.setColour (p.controlSurface);
    g.fillPath (can);
    g.setColour (p.highlightEdge);
    g.fillRect (grid.rect (10, 7, 1, 17));
    g.setColour (p.ink);
    g.strokePath (can, juce::PathStrokeType (l));
    // Ribs.
    g.setColour (p.shadowEdge);
    for (float x : { 13.0f, 16.0f, 19.0f })
        g.fillRect (grid.rect (x, 9, 1, 14));
}

} // namespace StudioPatchbay::DesktopIcons
