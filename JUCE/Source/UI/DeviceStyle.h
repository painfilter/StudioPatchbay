#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Model/PhysicalDevice.h"

namespace StudioPatchbay::DeviceStyle
{

// The connector colours of the Swift app, solid, for swatches and badges.
juce::Colour connectorColour (ConnectorType, juce::Colour otherColour);
// A device's custom colour, else its connector's.
juce::Colour deviceColour (const PhysicalDevice&, juce::Colour otherColour);
std::optional<juce::Colour> colourFromHex (const juce::String&);   // "#RRGGBB"
juce::String toHex (juce::Colour);

juce::String connectorName (ConnectorType);
juce::String deviceTypeName (DeviceType);
juce::String deviceTypePlural (DeviceType);
juce::String directionalityName (DeviceDirectionality);

// "OUT ch 1–8  ·  IN ch 1–8  ·  8 ch", as in the Swift app's device list.
juce::String rangeText (const PhysicalDevice&);
juce::String rangeOf (PhysicalDevice::Range);   // "1–8"

// Connector types offered for a converter or interface (no analog ones).
const std::vector<ConnectorType>& digitalConnectors();

juce::Font monoFont (float height);

} // namespace StudioPatchbay::DeviceStyle
