#pragma once

#include "PhysicalDevice.h"

#include <map>

namespace StudioPatchbay
{

// The user's physical devices and port labels on disk, in the files and
// format the Swift app uses (physicalDevices.json, portLabels.json), so both
// apps share one setup.
//
// A file that exists but cannot be read is reported, never replaced by
// defaults, and set aside before the next save; a failed save is reported too (a silent failure looked like a
// successful one in the Swift app).
class DeviceStore
{
public:
    // The first of these that can be created: <app data>/StudioPatchbay,
    // <documents>/StudioPatchbay, <temp>/StudioPatchbay.
    static juce::File defaultDirectory();

    explicit DeviceStore (juce::File directory = defaultDirectory());

    const juce::File& getDirectory() const { return directory; }
    juce::File devicesFile() const { return directory.getChildFile ("physicalDevices.json"); }
    juce::File labelsFile() const { return directory.getChildFile ("portLabels.json"); }

    const std::vector<PhysicalDevice>& getDevices() const { return devices; }
    const std::map<juce::String, juce::String>& getLabels() const { return labels; }
    // Empty when the last load and save worked.
    const juce::String& getError() const { return error; }

    void load();

    void add (const PhysicalDevice&);
    void update (const PhysicalDevice&);     // by id; unknown ids are ignored
    void remove (const juce::Uuid&);
    PhysicalDevice duplicate (const PhysicalDevice&);   // "<name> copy", new id
    void addImported (const std::vector<PhysicalDevice>&); // each with a new id
    void replaceAll (const std::vector<PhysicalDevice>&);

    // Sets or, when blank, clears the label of a detected port.
    void setLabel (const juce::String& key, const juce::String& label);

private:
    void saveDevices();
    void saveLabels();

    juce::File directory;
    std::vector<PhysicalDevice> devices;
    std::map<juce::String, juce::String> labels;
    juce::String error;
    bool unreadable = false;
};

} // namespace StudioPatchbay
