#pragma once

#include "Model/DataModel.h"
#include "Model/CubaseConfigParser.h"
#include "Model/PlatformPaths.h"
#include "Model/FileWatcher.h"
#include "Model/DeviceStore.h"
#include "Model/AudioPorts.h"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace StudioPatchbay
{

// Central application state. Detects Cubase installations, parses their
// XML configuration files, and watches for changes. Notifies listeners
// via juce::ChangeBroadcaster when the parsed data changes.
//
// This is the C++ equivalent of the Swift AppState @ObservableObject.
class AppState : public juce::ChangeBroadcaster
{
public:
    // The Cubase version chosen in an earlier session, or 0.
    explicit AppState (int preferredCubaseVersion = 0);

    // --- Installations ---

    const std::vector<CubaseInstallation>& getInstallations() const { return installations; }

    // Select an installation (the user's choice, remembered) and parse its XML files.
    void selectInstallation (int index);
    int getPreferredVersion() const { return preferredVersion; }

    // Which installation to show without a choice in this session.
    static int defaultSelection (const std::vector<CubaseInstallation>&, const juce::File& previous,
                                 int preferredVersion);
    int getSelectedIndex() const { return selectedIndex; }
    const CubaseInstallation* getSelectedInstallation() const;

    // Rescan for Cubase installations (e.g. after a new version is installed).
    void rescanInstallations();

    // --- Parsed data ---

    const ParsedDevices& getParsedDevices() const { return parsedDevices; }
    const std::vector<DetectedPort>& getDetectedPorts() const { return detectedPorts; }
    const std::vector<ControlRoomBus>& getControlRoomBuses() const { return controlRoomBuses; }

    // The interface's ports from Port Setup, with the user's labels.
    const std::vector<AudioPort>& getAudioPorts() const { return audioPorts; }

    // --- Physical devices (converters, interfaces), shared with the Swift app ---

    const std::vector<PhysicalDevice>& getPhysicalDevices() const { return deviceStore.getDevices(); }
    const DeviceStore& getDeviceStore() const { return deviceStore; }
    void addPhysicalDevice (const PhysicalDevice&);
    void updatePhysicalDevice (const PhysicalDevice&);
    void deletePhysicalDevice (const juce::Uuid&);
    PhysicalDevice duplicatePhysicalDevice (const PhysicalDevice&);
    void replaceAllPhysicalDevices (const std::vector<PhysicalDevice>&);
    void addPhysicalDevices (const std::vector<PhysicalDevice>&);   // imported, new ids
    void setPortLabel (const AudioPort&, const juce::String& label);

    // --- File watching ---

    bool isWatching() const { return fileWatcher.isWatching(); }
    void startWatching();
    void stopWatching();

    // --- Error state ---

    const juce::String& getErrorMessage() const { return errorMessage; }
    bool hasError() const { return errorMessage.isNotEmpty(); }

private:
    void show (int index);
    void rebuildAudioPorts();
    void parseSelectedInstallation();
    void onFilesChanged();

    std::vector<CubaseInstallation> installations;
    int selectedIndex = -1;
    int preferredVersion = 0;

    ParsedDevices parsedDevices;
    std::vector<DetectedPort> detectedPorts;
    std::vector<ControlRoomBus> controlRoomBuses;
    std::vector<AudioPort> audioPorts;
    DeviceStore deviceStore;

    FileWatcher fileWatcher;
    juce::String errorMessage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppState)
};

} // namespace StudioPatchbay
