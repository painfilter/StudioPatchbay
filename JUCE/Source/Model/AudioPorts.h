#pragma once

#include "DataModel.h"

#include <map>

namespace StudioPatchbay
{

// One physical port of the audio interface, built from Port Setup: a family
// ("MADI", "ADAT", ...) and the channel span its send and receive runs cover.
struct AudioPort
{
    int id = 0;
    juce::String name;          // "ADAT" or, with several of a family, "ADAT 2"
    int channelStart = 0;
    int channelEnd = -1;        // inclusive
    ConnectorType color = ConnectorType::other;
    juce::String interfaceName; // "<family> Interface" or the user's label

    bool contains (int channel) const { return channel >= channelStart && channel <= channelEnd; }
};

namespace AudioPorts
{
// Groups detected runs by family and pairs a family's send and receive runs
// by position (sorted by first channel), not by equal numbers: asymmetric
// interfaces number one physical port differently per direction. Sorted by
// first channel. Empty input gives an empty list.
std::vector<AudioPort> build (const std::vector<DetectedPort>&);

// The key a user label is stored under: family without its instance number,
// "@", first channel ("AES@201"). Neither part alone is stable.
juce::String stableLabelKey (const AudioPort&);

// Applies labels from portLabels.json; also reads the two older key forms
// ("ch<first channel>", then the plain port name) until they are saved again.
void applyLabels (std::vector<AudioPort>&, const std::map<juce::String, juce::String>& labels);

const AudioPort* find (const std::vector<AudioPort>&, int channel);
ConnectorType colorForFamily (const juce::String& family);
} // namespace AudioPorts

} // namespace StudioPatchbay
