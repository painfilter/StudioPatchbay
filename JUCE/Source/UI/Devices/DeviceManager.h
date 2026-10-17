#pragma once

#include "UI/OverlayContent.h"
#include "AppState.h"

#include <optional>

namespace StudioPatchbay
{

// Converters & Interfaces: the user's physical devices, as in the Swift
// app's settings window. Left: the list (all, grouped by type, or one type),
// click to edit, context menu to edit, duplicate or delete; Add; Actions for
// export, import, the save file and Remove All. Right: the ports detected in
// Port Setup for reference, the Cubase version they come from, and an
// optional hardware name per port.
class DeviceManager : public OverlayContent,
                      private juce::ChangeListener
{
public:
    static constexpr int width = 736, height = 500;

    DeviceManager (AppState&, DesktopWindows&);
    ~DeviceManager() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void setPalette (const Design::Palette&) override;

private:
    struct Row
    {
        bool header = false;
        DeviceType type = DeviceType::converter;
        size_t device = 0;   // index into the app's device list
    };

    class DeviceList;
    class PortPanel;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void rebuild();
    void openEditor (const PhysicalDevice&, bool isNew);
    void addDevice();
    void showRowMenu (const PhysicalDevice&);
    void showActions();
    void exportDevices();
    void importDevices();

    AppState& appState;
    DesktopWindows& desktop;
    std::optional<DeviceType> filter;
    std::vector<Row> rows;
    std::vector<juce::Rectangle<int>> filterTabs;

    std::unique_ptr<DeviceList> list;
    std::unique_ptr<PortPanel> ports;
    juce::Viewport listViewport;
    juce::TextButton addButton, actionsButton, emptyAddButton, emptyImportButton;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeviceManager)
};

} // namespace StudioPatchbay
