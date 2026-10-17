#include "Glyphs.h"

namespace StudioPatchbay::Glyphs
{

void draw (juce::Graphics& g, Kind kind, juce::Rectangle<float> r, juce::Colour c)
{
    const auto line = juce::jmax (1.0f, r.getHeight() / 9.0f);
    g.setColour (c);
    switch (kind)
    {
        case Kind::warning:
        {
            juce::Path p;
            p.addTriangle (r.getCentreX(), r.getY(), r.getRight(), r.getBottom(), r.getX(), r.getBottom());
            g.fillPath (p);
            g.setColour (juce::Colours::white);
            g.fillRect (r.getCentreX() - line * 0.5f, r.getY() + r.getHeight() * 0.35f, line, r.getHeight() * 0.33f);
            g.fillRect (r.getCentreX() - line * 0.5f, r.getBottom() - line * 2.0f, line, line);
            break;
        }
        case Kind::stop:
        {
            const auto box = r.reduced (line * 0.5f);
            juce::Path p;
            p.startNewSubPath (box.getTopLeft());
            p.lineTo (box.getBottomRight());
            p.startNewSubPath (box.getTopRight());
            p.lineTo (box.getBottomLeft());
            g.strokePath (p, juce::PathStrokeType (line * 1.4f));
            break;
        }
        case Kind::check:
        {
            juce::Path p;
            p.startNewSubPath (r.getX(), r.getY() + r.getHeight() * 0.55f);
            p.lineTo (r.getX() + r.getWidth() * 0.38f, r.getBottom() - line);
            p.lineTo (r.getRight(), r.getY() + line);
            g.strokePath (p, juce::PathStrokeType (line * 1.4f));
            break;
        }
    }
}

void line (juce::Graphics& g, Kind kind, juce::Colour symbolColour, juce::Colour textColour, const juce::String& text,
           juce::Rectangle<int> area, int maximumLines)
{
    const auto size = g.getCurrentFont().getHeight() * 0.85f;
    const auto lineHeight = juce::jmin (static_cast<float> (area.getHeight()),
                                        maximumLines > 1 ? g.getCurrentFont().getHeight() * 1.2f
                                                         : static_cast<float> (area.getHeight()));
    const auto symbol = juce::Rectangle<float> (static_cast<float> (area.getX()), area.getY() + (lineHeight - size) * 0.5f,
                                                size, size);
    draw (g, kind, symbol, symbolColour);
    g.setColour (textColour);
    const auto textArea = area.withTrimmedLeft (juce::roundToInt (size * 1.5f));
    if (maximumLines > 1)
        g.drawFittedText (text, textArea, juce::Justification::topLeft, maximumLines);
    else
        g.drawText (text, textArea, juce::Justification::centredLeft, true);
}

void triangle (juce::Graphics& g, juce::Rectangle<float> r, bool pointsRight, juce::Colour c)
{
    juce::Path p;
    if (pointsRight)
        p.addTriangle (r.getX(), r.getY(), r.getRight(), r.getCentreY(), r.getX(), r.getBottom());
    else
        p.addTriangle (r.getRight(), r.getY(), r.getX(), r.getCentreY(), r.getRight(), r.getBottom());
    g.setColour (c);
    g.fillPath (p);
}

} // namespace StudioPatchbay::Glyphs
