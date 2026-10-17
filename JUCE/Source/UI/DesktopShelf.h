#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumDesktopTile.h"
#include "Export/ViewExporter.h"
#include "AppState.h"
#include "ViewSwitcher.h"

namespace StudioPatchbay
{

// What the desktop asks the application to do. One interface, implemented by
// the main window, instead of a callback per icon.
class DesktopActions
{
public:
    virtual ~DesktopActions() = default;

    virtual ViewSwitcher::View currentView() const = 0;
    virtual void exportView (Export::Format, ViewSwitcher::View) = 0;
    virtual void reload() = 0;
    virtual void showAbout() = 0;
    virtual void forgetRecentExport (const juce::File&) = 0;
    // Opens the window again, zooming out of the given icon (desktop coordinates).
    virtual void restoreWindow (juce::Rectangle<int> fromIcon) = 0;
    virtual void showDeviceManager() = 0;
};

// The icons on the desktop while the window is collapsed. As on the
// reference desktop: StudioPatchbay itself at the bottom left (click opens
// the window again), the trash at the bottom right (opens About), and in a
// grid from the top left one volume per Cubase installation (click shows
// that version), Converters & Interfaces, the two export programs, the file watcher and the recently
// exported documents. Transparent: the desktop pattern is the parent's.
class DesktopShelf : public juce::Component,
                     private juce::ChangeListener
{
public:
    explicit DesktopShelf (AppState&);
    ~DesktopShelf() override;

    void setActions (DesktopActions* a) { actions = a; }
    void setRecentExports (const juce::StringArray& paths);
    void setDarkMode (bool);
    void setUiScale (double);
    void refreshText();

    // The icon of the application tile, in this component's coordinates.
    juce::Rectangle<int> appIconBounds() const;

    void resized() override;

    // Grid placement in logical pixels for a desktop of the given logical
    // size; index 0 is the first tile after the fixed ones.
    static juce::Rectangle<int> gridSlot (int index, int logicalWidth);

private:
    enum class Kind { installation, devices, exportPdf, exportPng, watcher, recent };

    struct Item
    {
        Kind kind;
        int installation = -1;
        juce::File file;
        std::unique_ptr<Design::DesktopTile> tile;
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void rebuild();
    void updateStates();
    void activate (const Item&);
    void showMenu (const Item&);
    void addTile (Item&&, Design::DesktopTile::IconPainter);
    void prepare (Design::DesktopTile&);
    int sc (int value) const { return juce::roundToInt (value * uiScale); }

    AppState& appState;
    DesktopActions* actions = nullptr;
    juce::StringArray recentExports;
    std::vector<Item> items;
    std::unique_ptr<Design::DesktopTile> appTile, trash;
    juce::Image appIcon;
    juce::String lastRead;
    bool darkMode = true;
    double uiScale = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DesktopShelf)
};

} // namespace StudioPatchbay
