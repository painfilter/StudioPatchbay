#include "DeviceStore.h"

#include <algorithm>

namespace StudioPatchbay
{

juce::File DeviceStore::defaultDirectory()
{
    for (auto location : { juce::File::userApplicationDataDirectory, juce::File::userDocumentsDirectory })
    {
        const auto dir = juce::File::getSpecialLocation (location).getChildFile ("StudioPatchbay");
        if (dir.isDirectory() || dir.createDirectory().wasOk())
            return dir;
    }
    // Always writable, though the system may clear it.
    const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("StudioPatchbay");
    dir.createDirectory();
    return dir;
}

DeviceStore::DeviceStore (juce::File dir) : directory (std::move (dir))
{
}

void DeviceStore::load()
{
    error.clear();

    // An earlier Swift build could leave a folder where the file belongs.
    if (devicesFile().isDirectory())
        devicesFile().deleteRecursively();

    if (! devicesFile().existsAsFile())
    {
        devices.clear();   // first launch: nothing yet, not an error
        saveDevices();
    }
    else
    {
        std::vector<PhysicalDevice> loaded;
        const auto result = PhysicalDevices::fromJson (devicesFile().loadFileAsString(), loaded);
        unreadable = result.failed();
        if (result.wasOk())
            devices = std::move (loaded);
        else
            error = "Couldn't read " + devicesFile().getFullPathName() + ":\n" + result.getErrorMessage();
    }

    labels.clear();
    if (const auto parsed = juce::JSON::parse (labelsFile()); auto* object = parsed.getDynamicObject())
        for (const auto& property : object->getProperties())
            if (property.value.isString())
                labels[property.name.toString()] = property.value.toString();
}

void DeviceStore::saveDevices()
{
    if (devicesFile().isDirectory())
        devicesFile().deleteRecursively();

    // The first save after a failed read keeps the unreadable file beside the
    // new one instead of overwriting what might still be recoverable.
    if (unreadable)
    {
        devicesFile().moveFileTo (directory.getChildFile ("physicalDevices.unreadable-"
                                  + juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S") + ".json"));
        unreadable = false;
    }
    error = devicesFile().replaceWithText (PhysicalDevices::toJson (devices), false, false, "\n")
                ? juce::String()
                : "Couldn't save to " + devicesFile().getFullPathName();
}

void DeviceStore::saveLabels()
{
    // Sorted keys, like the Swift encoder (std::map is already sorted).
    juce::StringArray lines;
    for (const auto& [key, value] : labels)
        lines.add ("  " + juce::JSON::toString (juce::var (key)) + " : " + juce::JSON::toString (juce::var (value)));
    const auto text = lines.isEmpty() ? juce::String ("{\n\n}") : "{\n" + lines.joinIntoString (",\n") + "\n}";
    labelsFile().replaceWithText (text, false, false, "\n");
}

void DeviceStore::add (const PhysicalDevice& device)
{
    devices.push_back (device);
    saveDevices();
}

void DeviceStore::update (const PhysicalDevice& device)
{
    for (auto& d : devices)
        if (d.id == device.id)
        {
            d = device;
            saveDevices();
            return;
        }
}

void DeviceStore::remove (const juce::Uuid& id)
{
    devices.erase (std::remove_if (devices.begin(), devices.end(), [&] (const PhysicalDevice& d) { return d.id == id; }),
                   devices.end());
    saveDevices();
}

PhysicalDevice DeviceStore::duplicate (const PhysicalDevice& device)
{
    auto copy = device.withNewId();
    copy.name = device.name + " copy";
    add (copy);
    return copy;
}

void DeviceStore::addImported (const std::vector<PhysicalDevice>& imported)
{
    for (const auto& d : imported)
        devices.push_back (d.withNewId());
    saveDevices();
}

void DeviceStore::replaceAll (const std::vector<PhysicalDevice>& replacement)
{
    devices = replacement;
    saveDevices();
}

void DeviceStore::setLabel (const juce::String& key, const juce::String& label)
{
    if (label.trim().isEmpty())
        labels.erase (key);
    else
        labels[key] = label.trim();
    saveLabels();
}

} // namespace StudioPatchbay
