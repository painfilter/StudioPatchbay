#include "Sidebar.h"
#include "Fonts.h"
#include "Localisation/Text.h"
#include "Glyphs.h"

namespace StudioPatchbay
{

namespace
{
using Text::tr;

constexpr FilterMode filters[] = { FilterMode::all, FilterMode::instruments, FilterMode::fx, FilterMode::sendOnly,
                                   FilterMode::stereo, FilterMode::mono, FilterMode::midi };
constexpr ViewSwitcher::View views[] = { ViewSwitcher::View::cards, ViewSwitcher::View::patchbay,
                                         ViewSwitcher::View::controlRoom };

juce::String filterName (FilterMode m)
{
    switch (m)
    {
        case FilterMode::all:         return tr ("filter.all");
        case FilterMode::instruments: return tr ("filter.instruments");
        case FilterMode::fx:          return tr ("filter.fx");
        case FilterMode::sendOnly:    return tr ("filter.sendOnly");
        case FilterMode::stereo:      return tr ("filter.stereo");
        case FilterMode::mono:        return tr ("filter.mono");
        case FilterMode::midi:        break;
    }
    return tr ("filter.midi");
}
} // namespace

Sidebar::Sidebar (AppState& state) : appState (state) {}

void Sidebar::setPalette (const Design::Palette& p) { pal = p; repaint(); }
void Sidebar::setUiScale (double s) { uiScale = s; repaint(); }
void Sidebar::setFilter (FilterMode m) { filter = m; repaint(); }
void Sidebar::setView (ViewSwitcher::View v) { view = v; repaint(); }

std::vector<Sidebar::Row> Sidebar::layoutRows() const
{
    std::vector<Row> rows;
    int y = sc (6);
    const auto w = getWidth();
    const auto add = [&] (Row::Kind kind, int value, int height)
    {
        rows.push_back ({ kind, value, { 0, y, w, sc (height) } });
        y += sc (height);
    };

    add (Row::Kind::heading, 0, 22);
    if (appState.getInstallations().empty())
        add (Row::Kind::note, 0, 22);
    for (size_t i = 0; i < appState.getInstallations().size(); ++i)
        add (Row::Kind::installation, static_cast<int> (i), 34);
    y += sc (6);
    add (Row::Kind::heading, 1, 22);
    for (int i = 0; i < 7; ++i)
        add (Row::Kind::filter, i, 21);
    y += sc (6);
    add (Row::Kind::heading, 2, 22);
    for (int i = 0; i < 3; ++i)
        add (Row::Kind::view, i, 21);
    y += sc (10);
    add (Row::Kind::setup, 0, 24);
    return rows;
}

int Sidebar::rowAt (juce::Point<int> p) const
{
    const auto rows = layoutRows();
    for (size_t i = 0; i < rows.size(); ++i)
        if (rows[i].bounds.contains (p))
        {
            const auto& r = rows[i];
            if (r.kind == Row::Kind::heading || r.kind == Row::Kind::note)
                return -1;
            if (r.kind == Row::Kind::installation
                && ! appState.getInstallations()[static_cast<size_t> (r.value)].xmlExists())
                return -1;
            return static_cast<int> (i);
        }
    return -1;
}

void Sidebar::paint (juce::Graphics& g)
{
    const auto edge = juce::jmax (1, sc (1));
    g.fillAll (pal.windowPanel);
    g.setColour (pal.shadowEdge);
    g.fillRect (getWidth() - edge, 0, edge, getHeight());

    const auto rows = layoutRows();
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const auto& r = rows[i];
        auto area = r.bounds.reduced (sc (8), 0);
        const auto isHover = static_cast<int> (i) == hovered;

        switch (r.kind)
        {
            case Row::Kind::heading:
            {
                static const char* keys[] = { "sidebar.cubaseVersion", "sidebar.filter", "sidebar.view" };
                g.setColour (pal.inkSoft);
                g.setFont (Fonts::sans (static_cast<float> (10.5 * uiScale), Fonts::Weight::semiBold));
                g.drawText (tr (keys[r.value]).toUpperCase(), area.withTrimmedTop (sc (6)), juce::Justification::centredLeft);
                break;
            }
            case Row::Kind::note:
                g.setColour (pal.inkSoft);
                g.setFont (Fonts::sans (static_cast<float> (11.0 * uiScale)));
                Glyphs::line (g, Glyphs::Kind::warning, juce::Colour (0xffd08a00), pal.inkSoft, tr ("sidebar.noInstallations"), area);
                break;
            case Row::Kind::installation:
            {
                const auto& inst = appState.getInstallations()[static_cast<size_t> (r.value)];
                const auto selected = r.value == appState.getSelectedIndex();
                if (isHover)
                {
                    g.setColour (pal.selection.withAlpha (0.12f));
                    g.fillRect (r.bounds);
                }
                const auto dot = juce::Rectangle<float> (static_cast<float> (area.getX()), area.getCentreY() - sc (4) * 0.5f - sc (5),
                                                         static_cast<float> (sc (8)), static_cast<float> (sc (8)));
                g.setColour (selected ? juce::Colour (0xff2fa84f) : pal.inkSoft.withAlpha (0.5f));
                if (selected) g.fillEllipse (dot); else g.drawEllipse (dot, static_cast<float> (edge));
                auto text = area.withTrimmedLeft (sc (16));
                g.setColour (inst.xmlExists() ? pal.ink : pal.inkSoft);
                g.setFont (Fonts::sans (static_cast<float> (12.5 * uiScale), selected ? Fonts::Weight::semiBold : Fonts::Weight::regular));
                g.drawText (inst.displayName(), text.removeFromTop (sc (18)), juce::Justification::bottomLeft);
                g.setColour (inst.xmlExists() ? pal.inkSoft : juce::Colour (0xffc0392b));
                g.setFont (Fonts::sans (static_cast<float> (10.0 * uiScale)));
                g.drawText (inst.xmlExists() ? tr ("sidebar.xmlFound") : tr ("sidebar.xmlMissing"), text,
                            juce::Justification::topLeft);
                break;
            }
            case Row::Kind::filter:
            case Row::Kind::view:
            {
                const auto active = r.kind == Row::Kind::filter ? filters[r.value] == filter : views[r.value] == view;
                if (active && r.kind == Row::Kind::filter)
                {
                    g.setColour (pal.selection);
                    g.fillRect (r.bounds.reduced (sc (4), 0));
                }
                else if (isHover)
                {
                    g.setColour (pal.selection.withAlpha (0.12f));
                    g.fillRect (r.bounds.reduced (sc (4), 0));
                }
                g.setColour (active && r.kind == Row::Kind::filter ? juce::Colours::white
                                                                    : (active || r.kind == Row::Kind::filter ? pal.ink : pal.inkSoft));
                g.setFont (Fonts::sans (static_cast<float> (12.0 * uiScale),
                                        active && r.kind == Row::Kind::view ? Fonts::Weight::semiBold : Fonts::Weight::regular));
                g.drawText (r.kind == Row::Kind::filter ? filterName (filters[r.value]) : ViewSwitcher::viewName (views[r.value]),
                            area.withTrimmedLeft (sc (6)), juce::Justification::centredLeft);
                break;
            }
            case Row::Kind::setup:
                g.setColour (isHover ? pal.ink : pal.inkSoft);
                g.setFont (Fonts::sans (static_cast<float> (11.5 * uiScale)));
                g.drawFittedText (tr ("sidebar.setup"), area, juce::Justification::centredLeft, 2);
                break;
        }
    }
}

void Sidebar::mouseMove (const juce::MouseEvent& e)
{
    const auto index = rowAt (e.getPosition());
    if (index == hovered)
        return;
    hovered = index;
    setMouseCursor (index >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    repaint();
}

void Sidebar::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void Sidebar::mouseUp (const juce::MouseEvent& e)
{
    const auto index = rowAt (e.getPosition());
    if (index < 0)
        return;
    const auto row = layoutRows()[static_cast<size_t> (index)];
    switch (row.kind)
    {
        case Row::Kind::installation: appState.selectInstallation (row.value); break;
        case Row::Kind::filter:       setFilter (filters[row.value]); if (onFilter) onFilter (filter); break;
        case Row::Kind::view:         if (onView) onView (views[row.value]); break;
        case Row::Kind::setup:        if (onSetup) onSetup(); break;
        case Row::Kind::heading:
        case Row::Kind::note:         break;
    }
    repaint();
}

} // namespace StudioPatchbay
