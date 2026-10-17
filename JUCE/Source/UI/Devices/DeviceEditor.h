#pragma once

#include "UI/OverlayContent.h"
#include "AppState.h"

namespace StudioPatchbay
{

// Add or edit one converter / interface, as in the Swift app: identity,
// connector and colour, directions, channel count and the two start
// channels (only starts that overlap no other device are offered), with
// the conflicts listed. Save stays off while there is a conflict, the name
// is empty or the count is zero; closing with changes asks first.
class DeviceEditor : public OverlayContent,
                     private juce::ChangeListener
{
public:
    static constexpr int width = 540, height = 452;

    DeviceEditor (AppState&, PhysicalDevice, bool isNew,
                  std::function<void (std::function<void()> discard)> confirmDiscard);
    ~DeviceEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setPalette (const Design::Palette&) override;
    void closeRequested() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    PhysicalDevice pending() const;
    void readFields();
    void refresh();
    void fillStartChannels (juce::ComboBox&, ChannelDirection);
    void save();
    void pickColour();
    juce::Rectangle<int> row (int index) const;

    AppState& appState;
    const PhysicalDevice original;
    PhysicalDevice draft;
    const bool isNew;
    std::function<void (std::function<void()>)> confirmDiscard;
    juce::Colour customColour;

    juce::TextEditor nameField, shortNameField, countField;
    juce::ComboBox typeBox, connectorBox, directionBox, sendStartBox, recvStartBox;
    juce::ToggleButton customColourToggle;
    juce::TextButton swatchButton, countDown, countUp, cancelButton, saveButton;
    juce::StringArray conflictLines;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeviceEditor)
};

} // namespace StudioPatchbay
