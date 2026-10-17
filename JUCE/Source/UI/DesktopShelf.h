#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumDesktopTile.h"
#include "Export/ViewExporter.h"
#include "Layout/DesktopLayout.h"
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
};

// The icons on the desktop: one volume per Cubase installation, the two
// export programs, the file watcher, recently exported documents, and the
// trash (which opens About). A view tab dropped on an export program exports
// that view. Transparent: the desktop pattern behind it is the parent's.
class DesktopShelf : public juce::Component,
                     public juce::DragAndDropTarget,
                     private juce::ChangeListener
{
public:
    explicit DesktopShelf (AppState&);
    ~DesktopShelf() override;

    void setActions (DesktopActions* a) { actions = a; }
    void setRecentExports (const juce::StringArray& paths);
    void setDarkMode (bool);
    void setUiScale (double);

    // Number of icons besides the trash, for the layout.
    int itemCount() const { return static_cast<int> (items.size()); }

    // Places the shelf and its icons; the layout is in the parent's coordinates.
    void place (const DesktopLayout&);

    // A drag description for a view tab, and back.
    static juce::var describeViewDrag (ViewSwitcher::View);
    static std::optional<ViewSwitcher::View> viewFromDrag (const juce::var&);

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragMove (const SourceDetails&) override;
    void itemDragExit (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;

private:
    enum class Kind { installation, exportPdf, exportPng, watcher, recent };

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
    Item* exportItemAt (juce::Point<int>);
    void setDropHighlight (Item*);
    Design::DesktopTile& addTile (Item&&, juce::String title, juce::String detail,
                                  Design::DesktopTile::IconPainter);

    AppState& appState;
    DesktopActions* actions = nullptr;
    juce::StringArray recentExports;
    std::vector<Item> items;
    std::unique_ptr<Design::DesktopTile> trash;
    Item* dropTarget = nullptr;
    juce::String lastRead;
    bool darkMode = true;
    double uiScale = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DesktopShelf)
};

} // namespace StudioPatchbay
