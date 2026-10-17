#include "DeviceStyle.h"
#include "Localisation/Text.h"
#include "Fonts.h"

namespace StudioPatchbay::DeviceStyle
{

using Text::tr;

namespace
{
const juce::String dash (juce::CharPointer_UTF8 ("\xe2\x80\x93"));
const juce::String dot (juce::CharPointer_UTF8 ("  \xc2\xb7  "));
} // namespace

juce::Colour connectorColour (ConnectorType type, juce::Colour otherColour)
{
    switch (type)
    {
        case ConnectorType::madi:        return juce::Colour::fromFloatRGBA (0.35f, 0.65f, 1.0f, 1.0f);
        case ConnectorType::adat:        return juce::Colour::fromFloatRGBA (0.4f, 0.85f, 0.55f, 1.0f);
        case ConnectorType::aes:         return juce::Colour::fromFloatRGBA (1.0f, 0.72f, 0.3f, 1.0f);
        case ConnectorType::spdif:       return juce::Colour::fromFloatRGBA (1.0f, 0.55f, 0.85f, 1.0f);
        case ConnectorType::dante:       return juce::Colour::fromFloatRGBA (0.55f, 0.85f, 1.0f, 1.0f);
        case ConnectorType::usb:         return juce::Colour::fromFloatRGBA (0.7f, 0.7f, 0.75f, 1.0f);
        case ConnectorType::thunderbolt: return juce::Colour::fromFloatRGBA (0.5f, 0.55f, 1.0f, 1.0f);
        case ConnectorType::analogLine:  return juce::Colour::fromFloatRGBA (0.95f, 0.6f, 0.25f, 1.0f);
        case ConnectorType::analogMic:   return juce::Colour::fromFloatRGBA (0.85f, 0.35f, 0.35f, 1.0f);
        case ConnectorType::other:       break;
    }
    return otherColour;
}

std::optional<juce::Colour> colourFromHex (const juce::String& hex)
{
    const auto digits = hex.trim().trimCharactersAtStart ("#");
    if (digits.length() != 6 || ! digits.containsOnly ("0123456789abcdefABCDEF"))
        return std::nullopt;
    return juce::Colour (static_cast<juce::uint32> (0xff000000u | static_cast<juce::uint32> (digits.getHexValue32())));
}

juce::String toHex (juce::Colour c)
{
    return "#" + c.toDisplayString (false).toUpperCase();
}

juce::Colour deviceColour (const PhysicalDevice& d, juce::Colour otherColour)
{
    if (auto custom = colourFromHex (d.customColorHex))
        return *custom;
    return connectorColour (d.portColor, otherColour);
}

juce::String connectorName (ConnectorType type)
{
    switch (type)
    {
        case ConnectorType::madi:        return "MADI";
        case ConnectorType::adat:        return "ADAT";
        case ConnectorType::aes:         return "AES/EBU";
        case ConnectorType::spdif:       return "S/PDIF";
        case ConnectorType::dante:       return "Dante";
        case ConnectorType::usb:         return "USB";
        case ConnectorType::thunderbolt: return "Thunderbolt";
        case ConnectorType::analogLine:  return tr ("devices.connectorAnalogLine");
        case ConnectorType::analogMic:   return tr ("devices.connectorAnalogMic");
        case ConnectorType::other:       break;
    }
    return tr ("devices.typeOther");
}

juce::String deviceTypeName (DeviceType type)
{
    switch (type)
    {
        case DeviceType::converter:      return tr ("devices.typeConverter");
        case DeviceType::audioInterface: return tr ("devices.typeAudioInterface");
        case DeviceType::other:          break;
    }
    return tr ("devices.typeOther");
}

juce::String deviceTypePlural (DeviceType type)
{
    switch (type)
    {
        case DeviceType::converter:      return tr ("devices.sectionConverters");
        case DeviceType::audioInterface: return tr ("devices.sectionAudioInterfaces");
        case DeviceType::other:          break;
    }
    return tr ("devices.sectionOther");
}

juce::String directionalityName (DeviceDirectionality d)
{
    switch (d)
    {
        case DeviceDirectionality::sendOnly:      return tr ("devices.directionSendOnly");
        case DeviceDirectionality::recvOnly:      return tr ("devices.directionRecvOnly");
        case DeviceDirectionality::bidirectional: break;
    }
    return tr ("devices.directionBoth");
}

juce::String rangeOf (PhysicalDevice::Range r)
{
    return juce::String (r.first) + dash + juce::String (r.last);
}

juce::String rangeText (const PhysicalDevice& d)
{
    const auto out = "OUT ch " + rangeOf (d.range (ChannelDirection::send));
    const auto in = "IN ch " + rangeOf (d.range (ChannelDirection::recv));
    const auto count = juce::String (d.physicalChannelCount) + " ch";
    switch (d.directionality)
    {
        case DeviceDirectionality::sendOnly: return out + dot + count + dot + tr ("devices.outputOnly");
        case DeviceDirectionality::recvOnly: return in + dot + count + dot + tr ("devices.inputOnly");
        case DeviceDirectionality::bidirectional: break;
    }
    return out + dot + in + dot + count;
}

const std::vector<ConnectorType>& digitalConnectors()
{
    static const std::vector<ConnectorType> list { ConnectorType::madi, ConnectorType::adat, ConnectorType::aes,
                                                   ConnectorType::spdif, ConnectorType::dante, ConnectorType::usb,
                                                   ConnectorType::thunderbolt, ConnectorType::other };
    return list;
}

juce::Font monoFont (float height)
{
    return Fonts::mono (height);
}

} // namespace StudioPatchbay::DeviceStyle
