#include "DeviceEditor.h"
#include "UI/DeviceStyle.h"
#include "Localisation/Text.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace StudioPatchbay
{

namespace
{
using Text::tr;

constexpr int margin = 16, labelWidth = 190, rowHeight = 30, firstRow = 12;

const std::vector<DeviceType> deviceTypes { DeviceType::converter, DeviceType::audioInterface, DeviceType::other };
const std::vector<DeviceDirectionality> directions { DeviceDirectionality::bidirectional,
                                                     DeviceDirectionality::sendOnly, DeviceDirectionality::recvOnly };

template <typename T>
int indexOf (const std::vector<T>& list, T value)
{
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i] == value)
            return static_cast<int> (i);
    return 0;
}

juce::String describe (const ChannelConflict& c, const juce::String& template_)
{
    return template_.replace ("%DIR%", c.direction == ChannelDirection::send ? "OUT" : "IN")
                    .replace ("%RANGE%", DeviceStyle::rangeOf ({ c.first, c.last }))
                    .replace ("%NAME%", c.otherName);
}

// The colour picker in a small window of its own.
class ColourPicker : public juce::Component, private juce::ChangeListener
{
public:
    ColourPicker (juce::Colour initial, std::function<void (juce::Colour)> changed)
        : selector (juce::ColourSelector::showColourspace | juce::ColourSelector::showSliders),
          onChange (std::move (changed))
    {
        selector.setCurrentColour (initial, juce::dontSendNotification);
        selector.addChangeListener (this);
        addAndMakeVisible (selector);
        setSize (260, 260);
    }
    void resized() override { selector.setBounds (getLocalBounds()); }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { onChange (selector.getCurrentColour()); }
    juce::ColourSelector selector;
    std::function<void (juce::Colour)> onChange;
};
} // namespace

DeviceEditor::DeviceEditor (AppState& state, PhysicalDevice device, bool newDevice,
                            std::function<void (std::function<void()>)> confirm)
    : appState (state), original (device), draft (std::move (device)), isNew (newDevice),
      confirmDiscard (std::move (confirm))
{
    const auto other = pal.inkSoft;
    customColour = DeviceStyle::colourFromHex (draft.customColorHex)
                       .value_or (DeviceStyle::connectorColour (draft.portColor, other));

    for (auto* field : { &nameField, &shortNameField, &countField })
    {
        field->onTextChange = [this] { readFields(); refresh(); };
        addAndMakeVisible (*field);
    }
    nameField.setText (draft.name, false);
    shortNameField.setText (draft.shortName, false);
    countField.setInputRestrictions (3, "0123456789");
    countField.setText (juce::String (draft.physicalChannelCount), false);
    nameField.setTextToShowWhenEmpty (tr ("devices.fieldName"), pal.inkSoft);
    shortNameField.setTextToShowWhenEmpty (tr ("devices.fieldShortNameHint"), pal.inkSoft);

    for (auto type : deviceTypes)
        typeBox.addItem (DeviceStyle::deviceTypeName (type), indexOf (deviceTypes, type) + 1);
    typeBox.setSelectedItemIndex (indexOf (deviceTypes, draft.deviceType), juce::dontSendNotification);
    typeBox.setTooltip (tr ("devices.helpType"));

    const auto& connectors = DeviceStyle::digitalConnectors();
    for (size_t i = 0; i < connectors.size(); ++i)
        connectorBox.addItem (DeviceStyle::connectorName (connectors[i]), static_cast<int> (i) + 1);
    connectorBox.setSelectedItemIndex (indexOf (connectors, draft.portColor), juce::dontSendNotification);
    connectorBox.setTooltip (tr ("devices.helpConnector"));

    for (auto d : directions)
        directionBox.addItem (DeviceStyle::directionalityName (d), indexOf (directions, d) + 1);
    directionBox.setSelectedItemIndex (indexOf (directions, draft.directionality), juce::dontSendNotification);
    directionBox.setTooltip (tr ("devices.helpDirections"));

    sendStartBox.setTooltip (tr ("devices.helpChannels"));
    recvStartBox.setTooltip (tr ("devices.helpChannels"));

    for (auto* box : { &typeBox, &connectorBox, &directionBox, &sendStartBox, &recvStartBox })
    {
        box->onChange = [this] { readFields(); refresh(); };
        addAndMakeVisible (*box);
    }

    customColourToggle.setButtonText (tr ("devices.customColour"));
    customColourToggle.setToggleState (draft.customColorHex.isNotEmpty(), juce::dontSendNotification);
    customColourToggle.onClick = [this] { refresh(); };
    swatchButton.onClick = [this] { pickColour(); };
    swatchButton.setTooltip (tr ("devices.pickColour"));
    addAndMakeVisible (customColourToggle);
    addAndMakeVisible (swatchButton);

    countDown.setButtonText (juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92")));
    countUp.setButtonText ("+");
    const auto step = [this] (int delta)
    {
        countField.setText (juce::String (juce::jlimit (1, 128, draft.physicalChannelCount + delta)), true);
    };
    countDown.onClick = [step] { step (-1); };
    countUp.onClick = [step] { step (1); };
    addAndMakeVisible (countDown);
    addAndMakeVisible (countUp);

    cancelButton.setButtonText (tr ("action.cancel"));
    saveButton.setButtonText (tr ("action.save"));
    cancelButton.onClick = [this] { closeRequested(); };
    saveButton.onClick = [this] { save(); };
    addAndMakeVisible (cancelButton);
    addAndMakeVisible (saveButton);

    appState.addChangeListener (this);
    refresh();
    setSize (width, height);
}

DeviceEditor::~DeviceEditor()
{
    appState.removeChangeListener (this);
}

void DeviceEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();   // another device may have changed the free channels
}

void DeviceEditor::readFields()
{
    draft.name = nameField.getText();
    draft.shortName = shortNameField.getText();
    draft.physicalChannelCount = juce::jlimit (0, 128, countField.getText().getIntValue());
    draft.deviceType = deviceTypes[static_cast<size_t> (juce::jmax (0, typeBox.getSelectedItemIndex()))];
    draft.portColor = DeviceStyle::digitalConnectors()[static_cast<size_t> (juce::jmax (0, connectorBox.getSelectedItemIndex()))];
    draft.directionality = directions[static_cast<size_t> (juce::jmax (0, directionBox.getSelectedItemIndex()))];
    if (sendStartBox.getSelectedId() > 0)
        draft.sendChannelStart = sendStartBox.getSelectedId();
    if (recvStartBox.getSelectedId() > 0)
        draft.recvChannelStart = recvStartBox.getSelectedId();
}

PhysicalDevice DeviceEditor::pending() const
{
    auto d = draft;
    d.customColorHex = customColourToggle.getToggleState() ? DeviceStyle::toHex (customColour) : juce::String();
    return d;
}

void DeviceEditor::fillStartChannels (juce::ComboBox& box, ChannelDirection direction)
{
    const auto current = direction == ChannelDirection::send ? draft.sendChannelStart : draft.recvChannelStart;
    box.clear (juce::dontSendNotification);
    for (auto start : PhysicalDevices::freeStartChannels (draft, appState.getPhysicalDevices(), direction))
        box.addItem (juce::String (start), start);
    box.setSelectedId (current, juce::dontSendNotification);
}

void DeviceEditor::refresh()
{
    fillStartChannels (sendStartBox, ChannelDirection::send);
    fillStartChannels (recvStartBox, ChannelDirection::recv);
    sendStartBox.setVisible (draft.supports (ChannelDirection::send));
    recvStartBox.setVisible (draft.supports (ChannelDirection::recv));
    swatchButton.setVisible (customColourToggle.getToggleState());

    conflictLines.clear();
    for (const auto& c : PhysicalDevices::conflicts (draft, appState.getPhysicalDevices()))
        conflictLines.add (describe (c, tr ("devices.conflictOverlaps")));

    saveButton.setEnabled (draft.name.trim().isNotEmpty() && conflictLines.isEmpty() && draft.physicalChannelCount > 0);
    repaint();
}

juce::Rectangle<int> DeviceEditor::row (int index) const
{
    return { margin, firstRow + index * rowHeight, width - 2 * margin, rowHeight - 6 };
}

void DeviceEditor::resized()
{
    const auto field = [this] (int index) { return row (index).withTrimmedLeft (labelWidth); };
    nameField.setBounds (field (0));
    shortNameField.setBounds (field (1));
    typeBox.setBounds (field (2));
    connectorBox.setBounds (field (3));
    customColourToggle.setBounds (row (4).withWidth (labelWidth + 160));
    swatchButton.setBounds (field (4).withTrimmedLeft (170).withWidth (48));
    directionBox.setBounds (field (5));
    countField.setBounds (field (6).withWidth (64));
    countDown.setBounds (field (6).withTrimmedLeft (70).withWidth (24));
    countUp.setBounds (field (6).withTrimmedLeft (96).withWidth (24));
    sendStartBox.setBounds (field (7).withWidth (90));
    recvStartBox.setBounds (field (8).withWidth (90));

    auto buttons = getLocalBounds().reduced (margin, 12).removeFromBottom (24);
    saveButton.setBounds (buttons.removeFromRight (100));
    buttons.removeFromRight (8);
    cancelButton.setBounds (buttons.removeFromRight (100));
}

void DeviceEditor::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);
    const auto label = [&] (int index, const juce::String& text)
    {
        g.setColour (pal.ink);
        g.setFont (Fonts::sans (12.0f, Fonts::Weight::regular));
        g.drawText (text, row (index).withWidth (labelWidth - 8), juce::Justification::centredLeft);
    };
    label (0, tr ("devices.fieldName"));
    label (1, tr ("devices.fieldShortName"));
    label (2, tr ("devices.fieldType"));
    label (3, tr ("devices.fieldConnector"));
    label (5, tr ("devices.fieldDirections"));
    label (6, tr ("devices.fieldCount"));

    if (customColourToggle.getToggleState())
    {
        g.setColour (customColour);
        g.fillRect (swatchButton.getBounds().reduced (4));
    }

    if (draft.supports (ChannelDirection::send))
    {
        label (7, tr ("devices.fieldSendStart"));
        g.setColour (pal.inkSoft);
        g.setFont (DeviceStyle::monoFont (12.0f));
        g.drawText (tr ("devices.resultingRange") + "  " + DeviceStyle::rangeOf (draft.range (ChannelDirection::send)),
                    row (7).withTrimmedLeft (labelWidth + 100), juce::Justification::centredLeft);
    }
    if (draft.supports (ChannelDirection::recv))
    {
        label (8, tr ("devices.fieldRecvStart"));
        g.setColour (pal.inkSoft);
        g.setFont (DeviceStyle::monoFont (12.0f));
        g.drawText (tr ("devices.resultingRange") + "  " + DeviceStyle::rangeOf (draft.range (ChannelDirection::recv)),
                    row (8).withTrimmedLeft (labelWidth + 100), juce::Justification::centredLeft);
    }

    auto notes = juce::Rectangle<int> (margin, row (9).getY() + 4, width - 2 * margin, height - row (9).getY() - 52);
    g.setFont (Fonts::sans (11.5f, Fonts::Weight::regular));
    if (conflictLines.isEmpty())
    {
        g.setColour (pal.inkSoft);
        g.drawFittedText (tr ("devices.helpChannels"), notes, juce::Justification::topLeft, 4);
    }
    else
    {
        g.setColour (pal.ink);
        g.setFont (Fonts::sans (12.0f, Fonts::Weight::semiBold));
        g.drawText (tr ("devices.conflictsHeading"), notes.removeFromTop (18), juce::Justification::centredLeft);
        g.setFont (Fonts::sans (11.5f, Fonts::Weight::regular));
        for (const auto& line : conflictLines)
            g.drawText (juce::String (juce::CharPointer_UTF8 ("\xe2\x9a\xa0 ")) + line, notes.removeFromTop (16),
                        juce::Justification::centredLeft);
    }
}

void DeviceEditor::setPalette (const Design::Palette& p)
{
    OverlayContent::setPalette (p);
    nameField.setTextToShowWhenEmpty (tr ("devices.fieldName"), pal.inkSoft);
    shortNameField.setTextToShowWhenEmpty (tr ("devices.fieldShortNameHint"), pal.inkSoft);
    customColourToggle.setColour (juce::ToggleButton::textColourId, pal.ink);
}

void DeviceEditor::pickColour()
{
    auto picker = std::make_unique<ColourPicker> (customColour, [safe = juce::Component::SafePointer<DeviceEditor> (this)] (juce::Colour c)
    {
        if (safe != nullptr)
        {
            safe->customColour = c;
            safe->repaint();
        }
    });
    juce::CallOutBox::launchAsynchronously (std::move (picker), swatchButton.getBounds(), this);
}

void DeviceEditor::save()
{
    readFields();
    if (! saveButton.isEnabled())
        return;
    if (isNew)
        appState.addPhysicalDevice (pending());
    else
        appState.updatePhysicalDevice (pending());
    close();
}

void DeviceEditor::closeRequested()
{
    readFields();
    if (pending() == original)
    {
        close();
        return;
    }
    confirmDiscard ([safe = juce::Component::SafePointer<DeviceEditor> (this)]
    {
        if (safe != nullptr)
            safe->close();
    });
}

} // namespace StudioPatchbay
