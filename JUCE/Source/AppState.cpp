#include "AppState.h"

namespace StudioPatchbay
{

AppState::AppState (int preferredCubaseVersion) : preferredVersion (preferredCubaseVersion)
{
    deviceStore.load();
    rescanInstallations();
}

// ============================================================================
// Physical devices and port labels
// ============================================================================

void AppState::addPhysicalDevice (const PhysicalDevice& d)        { deviceStore.add (d); sendChangeMessage(); }
void AppState::updatePhysicalDevice (const PhysicalDevice& d)     { deviceStore.update (d); sendChangeMessage(); }
void AppState::deletePhysicalDevice (const juce::Uuid& id)        { deviceStore.remove (id); sendChangeMessage(); }
void AppState::replaceAllPhysicalDevices (const std::vector<PhysicalDevice>& d) { deviceStore.replaceAll (d); sendChangeMessage(); }
void AppState::addPhysicalDevices (const std::vector<PhysicalDevice>& d)        { deviceStore.addImported (d); sendChangeMessage(); }

PhysicalDevice AppState::duplicatePhysicalDevice (const PhysicalDevice& d)
{
    auto copy = deviceStore.duplicate (d);
    sendChangeMessage();
    return copy;
}

void AppState::setPortLabel (const AudioPort& port, const juce::String& label)
{
    deviceStore.setLabel (AudioPorts::stableLabelKey (port), label);
    rebuildAudioPorts();
    sendChangeMessage();
}

void AppState::rebuildAudioPorts()
{
    // A Port Setup that reads as empty keeps the last good ports.
    if (detectedPorts.empty())
        return;
    audioPorts = AudioPorts::build (detectedPorts);
    AudioPorts::applyLabels (audioPorts, deviceStore.getLabels());
}

// ============================================================================
// Installation management
// ============================================================================

void AppState::rescanInstallations()
{
    // Keep the user's choice across a rescan; otherwise take the latest.
    const auto previous = getSelectedInstallation() != nullptr ? getSelectedInstallation()->prefsDir
                                                               : juce::File();
    installations = PlatformPaths::detectInstallations();

    // Re-parses the files too: "Reload" means the data, not just the list.
    show (defaultSelection (installations, previous, preferredVersion));
}

int AppState::defaultSelection (const std::vector<CubaseInstallation>& list, const juce::File& previous,
                                int preferredVersion)
{
    const auto find = [&list] (auto&& predicate)
    {
        for (size_t i = 0; i < list.size(); ++i)
            if (predicate (list[i]))
                return static_cast<int> (i);
        return -1;
    };

    // The one shown before a rescan, else the version chosen last time if it
    // has data, else the newest with data, else the newest.
    for (auto index : { find ([&] (const CubaseInstallation& c) { return previous != juce::File() && c.prefsDir == previous; }),
                        find ([&] (const CubaseInstallation& c) { return c.version == preferredVersion && c.xmlExists(); }),
                        find ([] (const CubaseInstallation& c) { return c.xmlExists(); }) })
        if (index >= 0)
            return index;
    return list.empty() ? -1 : 0;
}

void AppState::selectInstallation (int index)
{
    if (index >= 0 && index < static_cast<int> (installations.size()))
        preferredVersion = installations[static_cast<size_t> (index)].version;
    show (index);
}

void AppState::show (int index)
{
    if (index != selectedIndex)
    {
        // Another installation's data does not apply.
        audioPorts.clear();
        parsedDevices = {};
        hasDevices = false;
    }
    if (index < 0 || index >= static_cast<int> (installations.size()))
    {
        selectedIndex = -1;
        parsedDevices = {};
        hasDevices = false;
        detectedPorts.clear();
        controlRoomBuses.clear();
        errorMessage.clear();
        fileWatcher.stopWatching();
        sendChangeMessage();
        return;
    }

    selectedIndex = index;
    parseSelectedInstallation();
    startWatching();
    sendChangeMessage();
}

const CubaseInstallation* AppState::getSelectedInstallation() const
{
    if (selectedIndex >= 0 && selectedIndex < static_cast<int> (installations.size()))
        return &installations[static_cast<size_t> (selectedIndex)];
    return nullptr;
}

// ============================================================================
// Parsing
// ============================================================================

void AppState::parseSelectedInstallation()
{
    auto* inst = getSelectedInstallation();
    if (inst == nullptr)
        return;

    errorMessage.clear();

    // Parse External Plugins. As in the Swift app: a missing file is an
    // error with its path; a file that cannot be read keeps what was shown.
    auto epFile = inst->externalPluginsXml();
    if (! epFile.existsAsFile())
    {
        errorMessage = "External Plugins.xml not found in " + inst->prefsDir.getFullPathName();
        parsedDevices = {};
        hasDevices = false;
    }
    else
    {
        ParsedDevices parsed;
        const auto result = CubaseConfigParser::parseExternalPlugins (epFile, parsed);
        if (result.failed())
            errorMessage = "Failed to parse XML: " + result.getErrorMessage();
        else
        {
            parsedDevices = std::move (parsed);
            hasDevices = true;
            lastReadTime = juce::Time::getCurrentTime();
        }
    }

    // Parse Port Setup.
    auto psFile = inst->portSetupXml();
    if (psFile.existsAsFile())
    {
        detectedPorts = CubaseConfigParser::parsePortSetup (psFile);
    }
    else
    {
        detectedPorts.clear();
    }
    rebuildAudioPorts();

    // Parse Control Room.
    auto crFile = inst->controlRoomXml();
    if (crFile.existsAsFile())
    {
        controlRoomBuses = CubaseConfigParser::parseControlRoom (crFile);
    }
    else
    {
        controlRoomBuses.clear();
    }
}

// ============================================================================
// File watching
// ============================================================================

void AppState::startWatching()
{
    auto* inst = getSelectedInstallation();
    if (inst == nullptr)
        return;

    std::vector<juce::File> filesToWatch;
    filesToWatch.push_back (inst->externalPluginsXml());
    filesToWatch.push_back (inst->portSetupXml());
    filesToWatch.push_back (inst->controlRoomXml());

    fileWatcher.watchFiles (filesToWatch, [this] { onFilesChanged(); });
}

void AppState::stopWatching()
{
    fileWatcher.stopWatching();
}

void AppState::onFilesChanged()
{
    // Re-parse all XML files and notify listeners.
    parseSelectedInstallation();
    sendChangeMessage();
}

} // namespace StudioPatchbay
