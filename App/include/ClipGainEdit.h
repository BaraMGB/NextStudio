#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <optional>
#include <vector>

// Live, non-undo preview followed by one normal Engine gain transaction.
// Owners must prevent saving during a gesture and cancel on selection changes.
class ClipGainEdit
{
public:
    static constexpr float minimum = -100.0f, maximum = 24.0f;
    static std::optional<float> parse(const juce::String &);
    static juce::String format(float);
    static bool audioOnly(const juce::Array<tracktion_engine::Clip *> &);

    ClipGainEdit(tracktion_engine::Edit &, const juce::Array<tracktion_engine::Clip *> &);
    ~ClipGainEdit();
    bool isValid() const;
    bool matches(const juce::Array<tracktion_engine::Clip *> &) const;
    bool previewRelative(float deltaDb);
    bool previewAbsolute(float db);
    bool commit();
    void cancel();

private:
    struct Entry
    {
        tracktion_engine::AudioClipBase::Ptr clip;
        float before{}, after{};
        bool hadGain{};
        juce::var original;
    };
    void restore();
    tracktion_engine::Edit &m_edit;
    std::vector<Entry> m_entries;
    bool m_finished{false};
    bool m_updating{false};
    JUCE_DECLARE_NON_COPYABLE(ClipGainEdit)
};
