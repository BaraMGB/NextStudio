#include "ClipGainEdit.h"
#include <cmath>

namespace te = tracktion_engine;

std::optional<float> ClipGainEdit::parse(const juce::String &input)
{
    auto text = input.trim();
    if (text.endsWithIgnoreCase("dB"))
        text = text.dropLastCharacters(2).trimEnd();
    int i = (text.startsWithChar('+') || text.startsWithChar('-')) ? 1 : 0;
    bool digit = false, dot = false;
    for (; i < text.length(); ++i)
    {
        if (text[i] >= '0' && text[i] <= '9') digit = true;
        else if (text[i] == '.' && !dot) dot = true;
        else return {};
    }
    if (!digit) return {};
    const auto value = text.getDoubleValue();
    if (!std::isfinite(value) || value < minimum || value > maximum) return {};
    return static_cast<float>(value);
}

juce::String ClipGainEdit::format(float db)
{
    return juce::String(std::abs(db) < 0.05f ? 0.0f : db, 1) + " dB";
}

bool ClipGainEdit::audioOnly(const juce::Array<te::Clip *> &selection)
{
    if (selection.isEmpty()) return false;
    for (auto *clip : selection)
        if (dynamic_cast<te::AudioClipBase *>(clip) == nullptr) return false;
    return true;
}

ClipGainEdit::ClipGainEdit(te::Edit &edit, const juce::Array<te::Clip *> &selection) : m_edit(edit)
{
    for (auto *clip : selection)
        if (auto *audio = dynamic_cast<te::AudioClipBase *>(clip))
        {
            bool duplicate = false;
            for (const auto &entry : m_entries) duplicate |= entry.clip.get() == audio;
            if (!duplicate)
                m_entries.push_back({audio, audio->getGainDB(), audio->getGainDB(),
                                     audio->state.hasProperty(te::IDs::gain), audio->state[te::IDs::gain]});
        }
}

ClipGainEdit::~ClipGainEdit() { cancel(); }

bool ClipGainEdit::isValid() const
{
    if (m_finished || m_entries.empty()) return false;
    for (const auto &entry : m_entries)
        if (!entry.clip->state.isAChildOf(m_edit.state)
            || !std::isfinite(entry.before)
            || (!m_updating && std::abs(entry.clip->getGainDB() - entry.after) > 0.0001f))
            return false;
    return true;
}

bool ClipGainEdit::matches(const juce::Array<te::Clip *> &selection) const
{
    size_t count = 0;
    for (auto *clip : selection)
        if (dynamic_cast<te::AudioClipBase *>(clip)) ++count;
    if (count != m_entries.size()) return false;
    for (const auto &entry : m_entries)
        if (!selection.contains(entry.clip.get())) return false;
    return isValid();
}

bool ClipGainEdit::previewRelative(float delta)
{
    if (!isValid() || !std::isfinite(delta)) return false;
    float low = minimum - m_entries.front().before, high = maximum - m_entries.front().before;
    for (const auto &entry : m_entries)
    {
        low = juce::jmax(low, minimum - entry.before);
        high = juce::jmin(high, maximum - entry.before);
    }
    delta = juce::jlimit(low, high, delta);
    const juce::ScopedValueSetter<bool> updating(m_updating, true);
    // Assign all expected values before synchronous ValueTree callbacks run.
    for (auto &entry : m_entries) entry.after = entry.before + delta;
    for (auto &entry : m_entries)
        if (entry.clip->getGainDB() != entry.after)
            entry.clip->state.setProperty(te::IDs::gain, entry.after, nullptr);
    return true;
}

bool ClipGainEdit::previewAbsolute(float db)
{
    if (!isValid() || !std::isfinite(db) || db < minimum || db > maximum) return false;
    const juce::ScopedValueSetter<bool> updating(m_updating, true);
    for (auto &entry : m_entries) entry.after = db;
    for (auto &entry : m_entries)
        if (entry.clip->getGainDB() != db)
            entry.clip->state.setProperty(te::IDs::gain, db, nullptr);
    return true;
}

void ClipGainEdit::restore()
{
    for (auto &entry : m_entries)
    {
        // Do not overwrite a competing edit/undo that invalidated this gesture.
        if (std::abs(entry.clip->getGainDB() - entry.after) > 0.0001f) continue;
        if (entry.hadGain) entry.clip->state.setProperty(te::IDs::gain, entry.original, nullptr);
        else entry.clip->state.removeProperty(te::IDs::gain, nullptr);
    }
}

void ClipGainEdit::cancel()
{
    if (m_finished) return;
    m_finished = true;
    restore();
}

bool ClipGainEdit::commit()
{
    if (!isValid()) { cancel(); return false; }
    bool changed = false;
    for (const auto &entry : m_entries) changed |= std::abs(entry.before - entry.after) > 0.0001f;
    m_finished = true;
    restore();
    if (changed)
    {
        te::Edit::UndoTransactionInhibitor undoInhibitor(m_edit);
        auto &undo = m_edit.getUndoManager();
        undo.beginNewTransaction("Change Audio Clip Gain");
        for (const auto &entry : m_entries)
            if (std::abs(entry.before - entry.after) > 0.0001f)
                entry.clip->setGainDB(entry.after);
        undo.beginNewTransaction();
    }
    return true;
}
