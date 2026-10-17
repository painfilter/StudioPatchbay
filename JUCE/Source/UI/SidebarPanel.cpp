#include "SidebarPanel.h"
#include "Fonts.h"

namespace StudioPatchbay
{

SidebarPanel::SidebarPanel (AppState& state)
    : appState (state)
{
    listBox.setModel (this);
    listBox.setRowHeight (sc (28));
    listBox.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    listBox.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (listBox);

    // Select the current installation in the list.
    if (appState.getSelectedIndex() >= 0)
        listBox.selectRow (appState.getSelectedIndex());

    appState.addChangeListener (this);
}

SidebarPanel::~SidebarPanel()
{
    appState.removeChangeListener (this);
}

int SidebarPanel::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

void SidebarPanel::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;
    repaint();
    listBox.repaint();
}

void SidebarPanel::setUiScale (double scale)
{
    uiScale = scale;
    listBox.setRowHeight (sc (28));
    resized();
    repaint();
}

// ============================================================================
// Paint
// ============================================================================

void SidebarPanel::paint (juce::Graphics& g)
{
    const auto edge = juce::jmax (1, sc (1));

    // Panel background.
    g.fillAll (pal.windowPanel);

    // OS9 inset border for the sidebar.
    auto bounds = getLocalBounds();

    // Top shadow edge.
    g.setColour (pal.shadowEdge);
    g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), edge);

    // Left shadow edge.
    g.fillRect (bounds.getX(), bounds.getY(), edge, bounds.getHeight());

    // Bottom highlight.
    g.setColour (pal.highlightEdge);
    g.fillRect (bounds.getX(), bounds.getBottom() - edge, bounds.getWidth(), edge);

    // Right highlight.
    g.fillRect (bounds.getRight() - edge, bounds.getY(), edge, bounds.getHeight());

    // Section header: "Cubase Installations"
    g.setFont (Fonts::sans (static_cast<float> (sc (11)), Fonts::Weight::semiBold));
    g.setColour (pal.ink);
    g.drawText ("CUBASE INSTALLATIONS",
                bounds.removeFromTop (sc (24)).reduced (sc (8), 0),
                juce::Justification::centredLeft);

    // Separator under header.
    g.setColour (pal.shadowEdge);
    g.fillRect (sc (6), sc (24), getWidth() - sc (12), edge);
}

// ============================================================================
// Layout
// ============================================================================

void SidebarPanel::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (sc (28)); // header space
    area.reduce (sc (4), sc (4));
    listBox.setBounds (area);
}

// ============================================================================
// ListBoxModel
// ============================================================================

int SidebarPanel::getNumRows()
{
    return static_cast<int> (appState.getInstallations().size());
}

void SidebarPanel::paintListBoxItem (int rowNumber, juce::Graphics& g,
                                      int width, int height, bool rowIsSelected)
{
    auto& installations = appState.getInstallations();
    if (rowNumber < 0 || rowNumber >= static_cast<int> (installations.size()))
        return;

    auto& inst = installations[static_cast<size_t> (rowNumber)];

    // Selection highlight.
    if (rowIsSelected)
    {
        g.setColour (pal.selection);
        g.fillRect (0, 0, width, height);
        g.setColour (juce::Colours::white);
    }
    else
    {
        g.setColour (pal.controlSurface);
        g.fillRect (0, 0, width, height);
        g.setColour (pal.ink);
    }

    // Version name.
    g.setFont (Fonts::sans (static_cast<float> (sc (12)), Fonts::Weight::regular));
    g.drawText (inst.displayName(),
                juce::Rectangle<int> (sc (8), 0, width - sc (16), height),
                juce::Justification::centredLeft);

    // Status indicator – green dot if XML exists, gray if not.
    const auto dotSize = sc (6);
    const auto dotY = (height - dotSize) / 2;
    g.setColour (inst.xmlExists() ? juce::Colour (0xFF4CAF50) : pal.shadowEdge);
    g.fillEllipse (static_cast<float> (width - sc (16)),
                   static_cast<float> (dotY),
                   static_cast<float> (dotSize),
                   static_cast<float> (dotSize));

    // Bottom separator.
    g.setColour (pal.shadowEdge.withAlpha (0.3f));
    g.fillRect (sc (4), height - 1, width - sc (8), 1);
}

void SidebarPanel::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    appState.selectInstallation (row);
}

// ============================================================================
// ChangeListener
// ============================================================================

void SidebarPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    listBox.updateContent();
    listBox.repaint();
}

} // namespace StudioPatchbay
