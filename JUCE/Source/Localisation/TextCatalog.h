#pragma once

#include <juce_core/juce_core.h>

namespace StudioPatchbay::Text
{

// A dotted key ("menu.file") in a parsed JSON catalogue; empty when missing.
inline juce::String lookup (const juce::var& catalog, const juce::String& key)
{
    const juce::var* current = &catalog;
    for (const auto& part : juce::StringArray::fromTokens (key, ".", ""))
    {
        const auto* object = current->getDynamicObject();
        if (object == nullptr || ! object->hasProperty (part))
            return {};
        current = &object->getProperty (part);
    }
    return current->isString() ? current->toString() : juce::String();
}

// Every dotted key that leads to a string, for completeness checks.
inline void collectKeys (const juce::var& node, const juce::String& prefix, juce::StringArray& keys)
{
    if (node.isString())
    {
        keys.add (prefix);
        return;
    }
    if (const auto* object = node.getDynamicObject())
        for (const auto& property : object->getProperties())
            collectKeys (property.value, prefix.isEmpty() ? property.name.toString()
                                                          : prefix + "." + property.name.toString(), keys);
}

} // namespace StudioPatchbay::Text
