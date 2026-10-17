#include "AppState.h"

namespace StudioPatchbay
{

AppState::AppState()
{
    rescanInstallations();
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

    auto index = installations.empty() ? -1 : 0;
    for (size_t i = 0; i < installations.size(); ++i)
        if (installations[i].prefsDir == previous)
            index = static_cast<int> (i);

    // Re-parses the files too: "Reload" means the data, not just the list.
    selectInstallation (index);
}

void AppState::selectInstallation (int index)
{
    if (index < 0 || index >= static_cast<int> (installations.size()))
    {
        selectedIndex = -1;
        parsedDevices = {};
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

    // Parse External Plugins.
    auto epFile = inst->externalPluginsXml();
    if (epFile.existsAsFile())
    {
        parsedDevices = CubaseConfigParser::parseExternalPlugins (epFile);
    }
    else
    {
        parsedDevices = {};
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
