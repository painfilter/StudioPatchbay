#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "Model/Browsing.h"
#include "ViewSwitcher.h"
#include "AppState.h"

namespace StudioPatchbay
{

// The sidebar of the Swift app: Cubase version (only installations with an
// External Plugins file can be chosen), filter, view, and the way to the
// converter/interface setup.
class Sidebar : public juce::Component
{
public:
    explicit Sidebar (AppState&);

    void setPalette (const Design::Palette&);
    void setUiScale (double);
    void setFilter (FilterMode);
    void setView (ViewSwitcher::View);

    std::function<void (FilterMode)> onFilter;
    std::function<void (ViewSwitcher::View)> onView;
    std::function<void()> onSetup;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    struct Row
    {
        enum class Kind { heading, installation, filter, view, setup, note } kind;
        int value = 0;
        juce::Rectangle<int> bounds;
    };

    std::vector<Row> layoutRows() const;
    int rowAt (juce::Point<int>) const;
    int sc (int v) const { return juce::roundToInt (v * uiScale); }

    AppState& appState;
    Design::Palette pal = Design::lightPalette();
    double uiScale = 1.0;
    FilterMode filter = FilterMode::all;
    ViewSwitcher::View view = ViewSwitcher::View::cards;
    int hovered = -1;
};

} // namespace StudioPatchbay
