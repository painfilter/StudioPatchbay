// Generated file. Do not edit by hand: changes are overwritten when the
// file is regenerated.

#pragma once

#include "PlatinumTheme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace StudioPatchbay::Design
{

struct DesktopPattern
{
    int top = 0;
    int step = 4;
    int rowOffset = 2;
    int dotSize = 1;
};

void paintDesktopPattern (juce::Graphics& g, juce::Colour dot, const DesktopPattern& pattern,
                          juce::Rectangle<int> covered);

void drawTitleStripes (juce::Graphics&, juce::Rectangle<int> area, juce::Colour,
                       double uiScale);

void setChromeFontProvider (std::function<juce::Font (double height, int styleFlags)>);

juce::Font chromeFont (double height, int styleFlags = juce::Font::plain);

int buttonTextStyleFlags (const juce::TextButton&);

class PlatinumLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PlatinumLookAndFeel();
    void setDarkMode (bool shouldUseDarkMode);
    void setUiScale (double newScale);
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void fillTextEditorBackground (juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool separator, bool active, bool highlighted, bool ticked,
                            bool hasSubMenu, const juce::String& text, const juce::String& shortcut,
                            const juce::Drawable*, const juce::Colour*) override;
    void getIdealPopupMenuItemSize (const juce::String& text, bool separator,
                                    int standardMenuItemHeight,
                                    int& idealWidth, int& idealHeight) override;
    int getPopupMenuBorderSize() override;
    int getDefaultScrollbarWidth() override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height,
                        bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override;
    void drawAlertBox (juce::Graphics&, juce::AlertWindow&,
                       const juce::Rectangle<int>& textArea, juce::TextLayout&) override;
    void drawDocumentWindowTitleBar (juce::DocumentWindow&, juce::Graphics&, int w, int h,
                                     int titleSpaceX, int titleSpaceW, const juce::Image* icon,
                                     bool drawTitleTextOnLeft) override;
    juce::Button* createDocumentWindowButton (int buttonType) override;
    void positionDocumentWindowButtons (juce::DocumentWindow&, int titleBarX, int titleBarY,
                                        int titleBarW, int titleBarH, juce::Button* minimise,
                                        juce::Button* maximise, juce::Button* close,
                                        bool positionTitleBarButtonsOnLeft) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getAlertWindowTitleFont() override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowFont() override;
    void drawPlatinumBevel (juce::Graphics&, juce::Rectangle<int> bounds,
                            bool down, bool over, bool enabled);

private:
    void refreshColours();
    Palette pal;
    bool darkMode = false;
    double uiScale = 1.0;
    juce::Colour panel()     const { return pal.windowPanel; }
    juce::Colour surface()   const { return pal.controlSurface; }
    juce::Colour field()     const { return pal.field; }
    juce::Colour ink()       const { return pal.ink; }
    juce::Colour highlight() const { return pal.highlightEdge; }
    juce::Colour shadow()    const { return pal.shadowEdge; }
    juce::Colour selection() const { return pal.selection; }
    juce::Colour disabledInk() const { return ink().interpolatedWith (surface(), 0.45f); }
    int scaled (int value) const;
    int edge (int logicalThickness = 1) const;
    void drawHardFrame (juce::Graphics&, juce::Rectangle<int>, juce::Colour, int thickness = 1);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlatinumLookAndFeel)
};

} // namespace StudioPatchbay::Design
