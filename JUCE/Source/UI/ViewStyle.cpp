#include "ViewStyle.h"

#include <set>

namespace StudioPatchbay
{

juce::Font ViewStyle::mono (float logical) const
{
    return Fonts::mono (logical * k());
}

juce::Colour ViewStyle::typeColour (ParsedDevice::Type type) const
{
    switch (type)
    {
        case ParsedDevice::Type::instrument: return dark ? juce::Colour (0xff6fa8ff) : juce::Colour (0xff2f62c8);
        case ParsedDevice::Type::sendOnly:   return dark ? juce::Colour (0xffbf96ff) : juce::Colour (0xff7a48b8);
        case ParsedDevice::Type::fx:         break;
    }
    return dark ? juce::Colour (0xffffb04d) : juce::Colour (0xffc46f0c);
}

juce::Colour ViewStyle::controlRoom() const { return dark ? juce::Colour (0xff5fd6c8) : juce::Colour (0xff1f8c80); }
juce::Colour ViewStyle::warning() const     { return dark ? juce::Colour (0xffffa94d) : juce::Colour (0xffb35900); }
juce::Colour ViewStyle::midi() const        { return dark ? juce::Colour (0xff7bd88f) : juce::Colour (0xff2e8b45); }

juce::Colour ViewStyle::cellColour (const std::vector<ParsedDevice>& devices) const
{
    std::set<ParsedDevice::Type> types;
    for (const auto& d : devices)
        types.insert (d.type);
    if (types.count (ParsedDevice::Type::instrument) > 0 && types.count (ParsedDevice::Type::fx) == 0)
        return typeColour (ParsedDevice::Type::instrument);
    if (types.size() == 1 && types.count (ParsedDevice::Type::sendOnly) > 0)
        return typeColour (ParsedDevice::Type::sendOnly);
    return typeColour (ParsedDevice::Type::fx);
}

} // namespace StudioPatchbay
