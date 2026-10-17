#pragma once

#include "UI/OverlayContent.h"
#include "AppState.h"

namespace StudioPatchbay
{

// Shown after choosing a file to import, before anything changes. Whether
// the devices match ports detected here is information only; channel
// conflicts are not: "Add to Current" needs none with the saved devices or
// within the file, "Replace All" none within the file. The same rule as the
// edit form, so import is no way around it.
class ImportReview : public OverlayContent
{
public:
    static constexpr int width = 560, height = 440;

    ImportReview (AppState&, std::vector<PhysicalDevice> devices);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AppState& appState;
    std::vector<PhysicalDevice> devices;
    std::vector<ImportCheck> checks;
    int unmatched = 0, addBlocked = 0, replaceBlocked = 0;

    struct Rows : juce::Component
    {
        explicit Rows (ImportReview& o) : owner (o) {}
        void paint (juce::Graphics&) override;
        int preferredHeight() const;
        ImportReview& owner;
    } rows { *this };

    juce::Viewport viewport;
    juce::TextButton cancelButton, addButton, replaceButton;
};

} // namespace StudioPatchbay
