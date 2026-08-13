/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#include "PositionDisplayComponent.h"
#include "PositionDisplayHelpers.h"

#include <cmath>
#include <cstdlib>
#include <optional>
#include <vector>

struct FieldSegment
{
    juce::Range<int> textRange;
};

namespace PositionDisplayMetrics
{
static constexpr float fieldHorizontalPadding = 8.0f;
static constexpr float fieldVerticalPadding = 3.0f;
static constexpr float leadingLabelGap = 6.0f;
static constexpr float topLabelHeightRatio = 0.34f;
static constexpr float topLabelExtraHeight = 4.0f;
static constexpr float titleFontHeight = 9.0f;
static constexpr float sideValueFontHeight = 14.0f;
static constexpr float timeValueFontHeight = 16.0f;
static constexpr float positionValueFontHeight = 22.0f;
static constexpr int panelOuterPaddingX = 8;
static constexpr int panelOuterPaddingY = 6;
static constexpr int panelColumnGap = 10;
static constexpr int panelRowGap = 4;
static constexpr float minimumCenterWidthRatio = 0.36f;
static constexpr float titleWidthPadding = 10.0f;
}

float measureTextWidth(const juce::Font &font, const juce::String &text)
{
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, 0.0f, 0.0f);
    return glyphs.getBoundingBox(0, -1, true).getWidth();
}

juce::String makeStableWidthReference(juce::String text)
{
    for (int i = 0; i < text.length(); ++i)
        if (juce::CharacterFunctions::isDigit(text[i]))
            text = text.replaceSection(i, 1, "8");

    return text;
}

class PositionDisplayField : public juce::Component,
                             private juce::Timer
{
public:
    PositionDisplayField()
    {
        setWantsKeyboardFocus(true);
    }

    ~PositionDisplayField() override
    {
        juce::Desktop::getInstance().removeGlobalMouseListener(this);
    }

    enum class TitlePlacement
    {
        top,
        leading
    };

    void setValueJustification(juce::Justification justification)
    {
        m_valueJustification = justification;
        repaint();
    }

    void setLeadingContentJustification(juce::Justification justification)
    {
        m_leadingContentJustification = justification;
        resized();
        repaint();
    }

    void setFixedTitleWidth(float width)
    {
        m_fixedTitleWidth = width;
        resized();
        repaint();
    }

    void setFixedValueWidth(float width)
    {
        m_fixedValueWidth = width;
        resized();
        repaint();
    }

    struct Callbacks
    {
        std::function<bool(const juce::String &)> commitText;
        std::function<void(int segmentIndex)> dragStarted;
        std::function<void(int segmentIndex, int stepDelta, juce::ModifierKeys modifiers)> dragUpdated;
        std::function<void(int segmentIndex, int stepDelta, juce::ModifierKeys modifiers)> stepped;
        std::function<void()> dragEnded;
    };

    void setCallbacks(Callbacks callbacks) { m_callbacks = std::move(callbacks); }

    void setTitle(juce::String title)
    {
        m_title = std::move(title);
        repaint();
    }

    void setTitlePlacement(TitlePlacement placement)
    {
        m_titlePlacement = placement;
        resized();
        repaint();
    }

    void setDisplayText(const juce::String &text)
    {
        m_displayText = text;

        if (!isEditing())
            repaint();
    }

    void setSegments(std::vector<FieldSegment> segments)
    {
        m_segments = std::move(segments);

        if (m_selectedSegment >= static_cast<int>(m_segments.size()))
            m_selectedSegment = m_segments.empty() ? -1 : static_cast<int>(m_segments.size()) - 1;

        if (m_interactionSegment >= static_cast<int>(m_segments.size()))
            m_interactionSegment = -1;

        if (m_temporaryHighlightSegment >= static_cast<int>(m_segments.size()))
            m_temporaryHighlightSegment = -1;

        repaint();
    }

    void setFont(juce::Font font)
    {
        m_font = std::move(font);

        if (m_editor != nullptr)
            m_editor->applyFontToAllText(m_font);

        repaint();
    }

    void setTitleFont(juce::Font font)
    {
        m_titleFont = std::move(font);
        repaint();
    }

    void setColours(juce::Colour textColour, juce::Colour highlightColour, juce::Colour focusColour)
    {
        m_textColour = textColour;
        m_highlightColour = highlightColour;
        m_focusColour = focusColour;

        if (m_editor != nullptr)
        {
            m_editor->setColour(juce::TextEditor::textColourId, m_textColour);
            m_editor->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
            m_editor->setColour(juce::TextEditor::highlightColourId, juce::Colours::transparentBlack);
            m_editor->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
            m_editor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        }

        repaint();
    }

    bool isEditing() const { return m_editor != nullptr; }

    void paint(juce::Graphics &g) override
    {
        auto area = getLocalBounds().toFloat().reduced(2.0f);

        if (hasKeyboardFocus(true))
        {
            g.setColour(m_focusColour.withAlpha(0.6f));
            g.drawRoundedRectangle(area, 4.0f, 1.0f);
        }

        if (m_title.isNotEmpty())
        {
            g.setColour(m_textColour.withAlpha(0.85f));
            g.setFont(m_titleFont);

            const auto justification = m_titlePlacement == TitlePlacement::leading
                                           ? juce::Justification::centredLeft
                                           : juce::Justification::centred;

            g.drawFittedText(m_title, getTitleBounds().toNearestInt(), justification, 1);
        }

        if (!isEditing())
        {
            int highlightedSegment = -1;

            if (m_dragActive)
                highlightedSegment = m_interactionSegment;
            else if (m_temporaryHighlightSegment >= 0)
                highlightedSegment = m_temporaryHighlightSegment;
            else if (hasKeyboardFocus(true))
                highlightedSegment = m_selectedSegment;

            if (highlightedSegment >= 0 && highlightedSegment < static_cast<int>(m_segments.size()))
            {
                g.setColour(m_highlightColour);
                g.fillRoundedRectangle(getSegmentBounds(highlightedSegment).expanded(4.0f, 2.0f), 4.0f);
            }
        }

        if (!isEditing())
        {
            g.setColour(m_textColour);
            g.setFont(m_font);
            g.drawFittedText(m_displayText, getValueBounds().toNearestInt(), m_valueJustification, 1);
        }
    }

    void resized() override
    {
        if (m_editor != nullptr)
            m_editor->setBounds(getEditorBounds().toNearestInt());
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (isEditing())
        {
            if (!m_editor->getScreenBounds().contains(event.getScreenPosition()))
                endEditing(false);

            return;
        }

        if (!event.mods.isLeftButtonDown())
            return;

        grabKeyboardFocus();
        m_selectedSegment = -1;
        m_interactionSegment = findSegmentAt(event.position);
        m_temporaryHighlightSegment = -1;
        stopTimer();

        if (m_interactionSegment < 0)
        {
            repaint();
            return;
        }

        m_dragActive = true;
        m_lastDragStep = 0;

        if (m_callbacks.dragStarted)
            m_callbacks.dragStarted(m_interactionSegment);

        repaint();
    }

    void mouseDrag(const juce::MouseEvent &event) override
    {
        if (!m_dragActive || isEditing())
            return;

        event.source.enableUnboundedMouseMovement(true);

        constexpr int pixelsPerStep = 4;
        const auto stepDelta = -(event.getDistanceFromDragStartY() / pixelsPerStep);

        if (stepDelta == m_lastDragStep)
            return;

        m_lastDragStep = stepDelta;

        if (m_callbacks.dragUpdated)
            m_callbacks.dragUpdated(m_interactionSegment < 0 ? 0 : m_interactionSegment, stepDelta, event.mods);
    }

    void mouseUp(const juce::MouseEvent &) override
    {
        if (!m_dragActive)
            return;

        m_dragActive = false;
        m_lastDragStep = 0;
        showTemporaryHighlight(m_interactionSegment);
        m_interactionSegment = -1;

        if (m_callbacks.dragEnded)
            m_callbacks.dragEnded();
    }

    void mouseDoubleClick(const juce::MouseEvent &) override
    {
        beginEditing();
    }

    void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override
    {
        if (isEditing() || m_segments.empty())
            return;

        grabKeyboardFocus();
        m_selectedSegment = -1;

        const auto segment = findSegmentAt(event.position);
        const auto step = wheel.deltaY > 0.0f ? 1 : (wheel.deltaY < 0.0f ? -1 : 0);

        if (segment >= 0 && step != 0 && m_callbacks.stepped)
        {
            showTemporaryHighlight(segment);
            m_callbacks.stepped(segment, step, juce::ModifierKeys::getCurrentModifiersRealtime());
        }
        else
        {
            showTemporaryHighlight(-1);
            repaint();
        }
    }

    bool keyPressed(const juce::KeyPress &key) override
    {
        if (isEditing())
            return false;

        const auto keyCode = key.getKeyCode();

        if (keyCode == juce::KeyPress::returnKey || keyCode == juce::KeyPress::F2Key)
        {
            beginEditing();
            return true;
        }

        if (!m_segments.empty())
        {
            if (keyCode == juce::KeyPress::leftKey)
            {
                if (m_selectedSegment < 0)
                    m_selectedSegment = 0;
                else
                    m_selectedSegment = juce::jmax(0, m_selectedSegment - 1);

                repaint();
                return true;
            }

            if (keyCode == juce::KeyPress::rightKey)
            {
                if (m_selectedSegment < 0)
                    m_selectedSegment = 0;
                else
                    m_selectedSegment = juce::jmin(static_cast<int>(m_segments.size()) - 1, m_selectedSegment + 1);

                repaint();
                return true;
            }

            if ((keyCode == juce::KeyPress::upKey || keyCode == juce::KeyPress::downKey) && m_callbacks.stepped)
            {
                if (m_selectedSegment < 0)
                    m_selectedSegment = 0;

                m_callbacks.stepped(m_selectedSegment, keyCode == juce::KeyPress::upKey ? 1 : -1, key.getModifiers());
                return true;
            }
        }

        return false;
    }

private:
    void timerCallback() override
    {
        stopTimer();

        if (m_temporaryHighlightSegment >= 0)
        {
            m_temporaryHighlightSegment = -1;
            repaint();
        }
    }

    void showTemporaryHighlight(int segment)
    {
        m_temporaryHighlightSegment = segment;

        if (segment >= 0)
        {
            startTimer(180);
            repaint();
        }
        else
        {
            stopTimer();
        }
    }

    void beginEditing()
    {
        if (m_callbacks.commitText == nullptr || isEditing())
            return;

        stopTimer();
        m_interactionSegment = -1;
        m_temporaryHighlightSegment = -1;

        m_editor = std::make_unique<juce::TextEditor>();
        addAndMakeVisible(*m_editor);
        m_editor->setBounds(getEditorBounds().toNearestInt());
        m_editor->setFont(m_font);
        m_editor->applyFontToAllText(m_font);
        m_editor->setJustification(juce::Justification::centredLeft);
        m_editor->setBorder(juce::BorderSize<int>(0, 4, 0, 4));
        m_editor->setText(m_displayText, false);
        m_editor->setColour(juce::TextEditor::textColourId, m_textColour);
        m_editor->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        m_editor->setColour(juce::TextEditor::highlightColourId, juce::Colours::transparentBlack);
        m_editor->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        m_editor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        m_editor->onReturnKey = [this] { commitEditorText(); };
        m_editor->onEscapeKey = [this] { endEditing(false); };
        m_editor->onFocusLost = [this] { endEditing(false); };
        juce::Desktop::getInstance().addGlobalMouseListener(this);
        m_editor->grabKeyboardFocus();
        m_editor->selectAll();
        repaint();
    }

    void endEditing(bool keepChanges)
    {
        if (m_editor == nullptr)
            return;

        if (!keepChanges)
            m_editor->setText(m_displayText, false);

        juce::Desktop::getInstance().removeGlobalMouseListener(this);
        m_editor.reset();
        repaint();
    }

    void commitEditorText()
    {
        if (m_editor == nullptr)
            return;

        if (m_callbacks.commitText && m_callbacks.commitText(m_editor->getText()))
        {
            endEditing(true);
            return;
        }

        juce::LookAndFeel::getDefaultLookAndFeel().playAlertSound();
        m_editor->selectAll();
    }

    int findSegmentAt(juce::Point<float> position) const
    {
        if (m_segments.empty())
            return -1;

        for (int i = 0; i < static_cast<int>(m_segments.size()); ++i)
            if (getSegmentBounds(i).expanded(4.0f, 2.0f).contains(position))
                return i;

        return -1;
    }

    juce::Rectangle<float> getTitleBounds() const
    {
        auto area = getLocalBounds().toFloat().reduced(PositionDisplayMetrics::fieldHorizontalPadding,
                                                       PositionDisplayMetrics::fieldVerticalPadding);

        if (m_title.isEmpty())
            return {};

        if (m_titlePlacement == TitlePlacement::leading)
        {
            const auto preferredWidth = m_fixedTitleWidth > 0.0f ? m_fixedTitleWidth
                                                                  : measureTextWidth(m_titleFont, m_title) + PositionDisplayMetrics::titleWidthPadding;
            const auto titleWidth = juce::jmin(area.getWidth() * 0.4f, juce::jmax(24.0f, preferredWidth));
            const auto preferredValueWidth = m_fixedValueWidth > 0.0f ? m_fixedValueWidth
                                                                       : measureTextWidth(m_font, m_displayText);
            const auto valueWidth = juce::jmin(juce::jmax(0.0f, area.getWidth() - titleWidth - PositionDisplayMetrics::leadingLabelGap),
                                               preferredValueWidth);
            const auto contentWidth = titleWidth + PositionDisplayMetrics::leadingLabelGap + valueWidth;

            auto contentX = area.getX();
            if (m_leadingContentJustification.testFlags(juce::Justification::horizontallyCentred))
                contentX = area.getCentreX() - (contentWidth * 0.5f);
            else if (m_leadingContentJustification.testFlags(juce::Justification::right))
                contentX = area.getRight() - contentWidth;

            return {contentX, area.getY(), titleWidth, area.getHeight()};
        }

        const auto titleHeight = juce::jmin(area.getHeight() * PositionDisplayMetrics::topLabelHeightRatio,
                                            m_titleFont.getHeight() + PositionDisplayMetrics::topLabelExtraHeight);
        return area.removeFromTop(titleHeight);
    }

    juce::Rectangle<float> getValueBounds() const
    {
        auto area = getLocalBounds().toFloat().reduced(PositionDisplayMetrics::fieldHorizontalPadding,
                                                       PositionDisplayMetrics::fieldVerticalPadding);

        if (m_title.isEmpty())
            return area;

        if (m_titlePlacement == TitlePlacement::leading)
        {
            const auto preferredWidth = m_fixedTitleWidth > 0.0f ? m_fixedTitleWidth
                                                                  : measureTextWidth(m_titleFont, m_title) + PositionDisplayMetrics::titleWidthPadding;
            const auto titleWidth = juce::jmin(area.getWidth() * 0.4f, juce::jmax(24.0f, preferredWidth));
            const auto maximumValueWidth = juce::jmax(0.0f, area.getWidth() - titleWidth - PositionDisplayMetrics::leadingLabelGap);
            const auto preferredValueWidth = m_fixedValueWidth > 0.0f ? m_fixedValueWidth
                                                                       : measureTextWidth(m_font, m_displayText);
            const auto valueWidth = juce::jmin(maximumValueWidth, preferredValueWidth);
            const auto contentWidth = titleWidth + PositionDisplayMetrics::leadingLabelGap + valueWidth;

            auto contentX = area.getX();
            if (m_leadingContentJustification.testFlags(juce::Justification::horizontallyCentred))
                contentX = area.getCentreX() - (contentWidth * 0.5f);
            else if (m_leadingContentJustification.testFlags(juce::Justification::right))
                contentX = area.getRight() - contentWidth;

            area.setX(contentX + titleWidth + PositionDisplayMetrics::leadingLabelGap);
            area.setWidth(valueWidth);
        }
        else
        {
            const auto titleHeight = juce::jmin(area.getHeight() * PositionDisplayMetrics::topLabelHeightRatio,
                                                m_titleFont.getHeight() + PositionDisplayMetrics::topLabelExtraHeight);
            area.removeFromTop(titleHeight);
        }

        return area;
    }

    juce::Rectangle<float> getEditorBounds() const
    {
        return getValueBounds().expanded(6.0f, 2.0f)
                              .getIntersection(getLocalBounds().toFloat().reduced(1.0f));
    }

    juce::Rectangle<float> getSegmentBounds(int index) const
    {
        if (index < 0 || index >= static_cast<int>(m_segments.size()))
            return {};

        const auto area = getValueBounds();
        const auto totalWidth = measureTextWidth(m_font, m_displayText);

        float textStartX = area.getX();

        if (m_valueJustification.testFlags(juce::Justification::horizontallyCentred))
            textStartX = area.getCentreX() - (totalWidth * 0.5f);
        else if (m_valueJustification.testFlags(juce::Justification::right))
            textStartX = area.getRight() - totalWidth;

        const auto textY = area.getCentreY() - (m_font.getHeight() * 0.5f);

        const auto &segment = m_segments[static_cast<size_t>(index)].textRange;
        const auto prefix = m_displayText.substring(0, segment.getStart());
        const auto text = m_displayText.substring(segment.getStart(), segment.getEnd());

        const auto x = textStartX + measureTextWidth(m_font, prefix);
        const auto width = measureTextWidth(m_font, text);

        return {x, textY, width, m_font.getHeight()};
    }

    Callbacks m_callbacks;
    juce::String m_title;
    juce::String m_displayText;
    std::vector<FieldSegment> m_segments;
    juce::Font m_font{juce::FontOptions(16.0f)};
    juce::Font m_titleFont{juce::FontOptions(12.0f)};
    TitlePlacement m_titlePlacement{TitlePlacement::top};
    juce::Justification m_valueJustification{juce::Justification::centred};
    juce::Justification m_leadingContentJustification{juce::Justification::centredLeft};
    float m_fixedTitleWidth{0.0f};
    float m_fixedValueWidth{0.0f};
    juce::Colour m_textColour{juce::Colours::white};
    juce::Colour m_highlightColour{juce::Colours::white.withAlpha(0.15f)};
    juce::Colour m_focusColour{juce::Colours::white.withAlpha(0.8f)};
    std::unique_ptr<juce::TextEditor> m_editor;
    int m_selectedSegment{-1};
    int m_interactionSegment{-1};
    int m_temporaryHighlightSegment{-1};
    bool m_dragActive{false};
    int m_lastDragStep{0};
};

namespace
{
using PositionDisplayHelpers::parseStrictInt;
using PositionDisplayHelpers::parseStrictDouble;
using PositionDisplayHelpers::formatBpm;
using PositionDisplayHelpers::formatTimeSignature;
using PositionDisplayHelpers::formatTime;
using PositionDisplayHelpers::formatBarsBeatsTicks;
using PositionDisplayHelpers::parseTimeValue;
using PositionDisplayHelpers::parseBarsBeatsTicks;
using PositionDisplayHelpers::clampPositionToRange;
using PositionDisplayHelpers::parseTimeSignatureValue;
using PositionDisplayHelpers::getDenominatorIndex;
using PositionDisplayHelpers::getDenominatorForIndex;

std::vector<FieldSegment> buildSegmentsFromDelimitedText(const juce::String &text, const juce::String &delimiters)
{
    std::vector<FieldSegment> segments;
    int segmentStart = 0;

    for (int i = 0; i < text.length(); ++i)
    {
        if (delimiters.containsChar(text[i]))
        {
            if (i > segmentStart)
                segments.push_back({juce::Range<int>(segmentStart, i)});

            segmentStart = i + 1;
        }
    }

    if (segmentStart < text.length())
        segments.push_back({juce::Range<int>(segmentStart, text.length())});

    return segments;
}

std::vector<FieldSegment> buildSingleSegment(const juce::String &text)
{
    return {{juce::Range<int>(0, text.length())}};
}

struct BarsBeatsTicksParts
{
    int bar{1};
    int beat{1};
    int tick{0};
};

BarsBeatsTicksParts getBarsBeatsTicksParts(te::Edit &edit, tracktion::TimePosition position)
{
    const auto barsBeats = edit.tempoSequence.toBarsAndBeats(position);

    return {
        barsBeats.bars + 1,
        barsBeats.getWholeBeats() + 1,
        juce::jlimit(0,
                     te::Edit::ticksPerQuarterNote - 1,
                     static_cast<int>(barsBeats.getFractionalBeats().inBeats() * te::Edit::ticksPerQuarterNote))};
}

tracktion::TimePosition clampToValidPosition(tracktion::TimePosition position)
{
    return clampPositionToRange(position, te::Edit::getMaximumEditEnd());
}

tracktion::TimePosition timeFromDuration(const te::TimecodeDuration &duration)
{
    if (!duration.seconds.has_value())
        return {};

    return tracktion::TimePosition::fromSeconds(duration.seconds->inSeconds());
}

} // namespace

PositionDisplayComponent::PositionDisplayComponent(te::Edit &edit, ApplicationViewState &appState)
    : m_edit(edit),
      m_appState(appState),
      m_bpmField(std::make_unique<PositionDisplayField>()),
      m_timeSignatureField(std::make_unique<PositionDisplayField>()),
      m_positionField(std::make_unique<PositionDisplayField>()),
      m_timeField(std::make_unique<PositionDisplayField>()),
      m_loopInField(std::make_unique<PositionDisplayField>()),
      m_loopOutField(std::make_unique<PositionDisplayField>())
{
    m_bpmField->setCallbacks({
        [this](const juce::String &text) { return commitBpm(text); },
        [this](int segmentIndex) { beginDrag(FieldId::bpm, segmentIndex); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers) { updateDrag(FieldId::bpm, segmentIndex, stepDelta, modifiers); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers)
        {
            beginDrag(FieldId::bpm, segmentIndex);
            updateDrag(FieldId::bpm, segmentIndex, stepDelta, modifiers);
            endDrag(FieldId::bpm);
        },
        [this] { endDrag(FieldId::bpm); }});

    m_timeSignatureField->setCallbacks({
        [this](const juce::String &text) { return commitTimeSignature(text); },
        [this](int segmentIndex) { beginDrag(FieldId::timeSignature, segmentIndex); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers) { updateDrag(FieldId::timeSignature, segmentIndex, stepDelta, modifiers); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers)
        {
            beginDrag(FieldId::timeSignature, segmentIndex);
            updateDrag(FieldId::timeSignature, segmentIndex, stepDelta, modifiers);
            endDrag(FieldId::timeSignature);
        },
        [this] { endDrag(FieldId::timeSignature); }});

    m_positionField->setCallbacks({
        [this](const juce::String &text) { return commitTransportPositionFromBarsBeats(text); },
        [this](int segmentIndex) { beginDrag(FieldId::position, segmentIndex); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers) { updateDrag(FieldId::position, segmentIndex, stepDelta, modifiers); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers)
        {
            beginDrag(FieldId::position, segmentIndex);
            updateDrag(FieldId::position, segmentIndex, stepDelta, modifiers);
            endDrag(FieldId::position);
        },
        [this] { endDrag(FieldId::position); }});

    m_timeField->setCallbacks({
        [this](const juce::String &text) { return commitTransportPositionFromTime(text); },
        [this](int segmentIndex) { beginDrag(FieldId::time, segmentIndex); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers) { updateDrag(FieldId::time, segmentIndex, stepDelta, modifiers); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers)
        {
            beginDrag(FieldId::time, segmentIndex);
            updateDrag(FieldId::time, segmentIndex, stepDelta, modifiers);
            endDrag(FieldId::time);
        },
        [this] { endDrag(FieldId::time); }});

    m_loopInField->setCallbacks({
        [this](const juce::String &text) { return commitLoopIn(text); },
        [this](int segmentIndex) { beginDrag(FieldId::loopIn, segmentIndex); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers) { updateDrag(FieldId::loopIn, segmentIndex, stepDelta, modifiers); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers)
        {
            beginDrag(FieldId::loopIn, segmentIndex);
            updateDrag(FieldId::loopIn, segmentIndex, stepDelta, modifiers);
            endDrag(FieldId::loopIn);
        },
        [this] { endDrag(FieldId::loopIn); }});

    m_loopOutField->setCallbacks({
        [this](const juce::String &text) { return commitLoopOut(text); },
        [this](int segmentIndex) { beginDrag(FieldId::loopOut, segmentIndex); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers) { updateDrag(FieldId::loopOut, segmentIndex, stepDelta, modifiers); },
        [this](int segmentIndex, int stepDelta, juce::ModifierKeys modifiers)
        {
            beginDrag(FieldId::loopOut, segmentIndex);
            updateDrag(FieldId::loopOut, segmentIndex, stepDelta, modifiers);
            endDrag(FieldId::loopOut);
        },
        [this] { endDrag(FieldId::loopOut); }});

    m_bpmField->setTitle("BPM");
    m_timeSignatureField->setTitle("SIG");
    m_positionField->setTitle("POS");
    m_timeField->setTitle("TIME");
    m_loopInField->setTitle("IN");
    m_loopOutField->setTitle("OUT");

    for (auto *field : {m_bpmField.get(), m_timeSignatureField.get(), m_positionField.get(), m_timeField.get(), m_loopInField.get(), m_loopOutField.get()})
    {
        field->setTitlePlacement(PositionDisplayField::TitlePlacement::leading);
        field->setValueJustification(juce::Justification::centredLeft);
    }

    for (auto *field : {m_positionField.get(), m_timeField.get()})
        field->setLeadingContentJustification(juce::Justification::centred);

    for (auto *field : {m_loopInField.get(), m_loopOutField.get()})
        field->setLeadingContentJustification(juce::Justification::centredRight);

    Helpers::addAndMakeVisible(*this, {m_bpmField.get(), m_timeSignatureField.get(), m_positionField.get(), m_timeField.get(), m_loopInField.get(), m_loopOutField.get()});

    m_edit.state.addListener(this);
    m_edit.getTransport().state.addListener(this);
    m_themeState = m_appState.m_applicationStateValueTree.getChildWithName(IDs::ThemeState);
    if (m_themeState.isValid())
        m_themeState.addListener(this);

    rebuildTempoSequenceListeners();
    refreshFromModel();
}

PositionDisplayComponent::~PositionDisplayComponent()
{
    cancelPendingUpdate();

    if (m_dragState.active && (m_dragState.field == FieldId::position || m_dragState.field == FieldId::time))
        m_edit.getTransport().setUserDragging(false);

    if (m_dragState.undoTransactionStarted)
        m_edit.getUndoManager().beginNewTransaction();

    for (auto state : m_tempoItemStates)
        state.removeListener(this);

    m_edit.getTransport().state.removeListener(this);
    m_edit.state.removeListener(this);

    if (m_themeState.isValid())
        m_themeState.removeListener(this);
}

void PositionDisplayComponent::paint(juce::Graphics &g)
{
    auto area = getLocalBounds();

    g.setColour(m_appState.getButtonBackgroundColour());
    g.fillRoundedRectangle(area.toFloat(), 5.0f);
    const auto textColour = m_appState.getButtonTextColour();
    g.setColour(textColour);
    g.drawRoundedRectangle(area.reduced(1).toFloat(), 5.0f, 0.5f);

    g.setColour(textColour.withAlpha(0.18f));
    const auto separatorTop = static_cast<float>(PositionDisplayMetrics::panelOuterPaddingY + 3);
    const auto separatorBottom = static_cast<float>(getHeight() - PositionDisplayMetrics::panelOuterPaddingY - 3);
    g.drawLine(static_cast<float>(m_leftGroupSeparatorX), separatorTop,
               static_cast<float>(m_leftGroupSeparatorX), separatorBottom, 1.0f);
    g.drawLine(static_cast<float>(m_rightGroupSeparatorX), separatorTop,
               static_cast<float>(m_rightGroupSeparatorX), separatorBottom, 1.0f);
}

void PositionDisplayComponent::resized()
{
    updateFieldStyles();

    auto area = getLocalBounds().reduced(PositionDisplayMetrics::panelOuterPaddingX,
                                         PositionDisplayMetrics::panelOuterPaddingY);

    const auto titleFont = juce::Font(juce::FontOptions(PositionDisplayMetrics::titleFontHeight));
    const auto sideValueFont = juce::Font(juce::FontOptions(PositionDisplayMetrics::sideValueFontHeight));

    auto sideLabelWidth = measureTextWidth(titleFont, "BPM") + PositionDisplayMetrics::titleWidthPadding;
    sideLabelWidth = juce::jmax(sideLabelWidth, measureTextWidth(titleFont, "SIG") + PositionDisplayMetrics::titleWidthPadding);
    sideLabelWidth = juce::jmax(sideLabelWidth, measureTextWidth(titleFont, "IN") + PositionDisplayMetrics::titleWidthPadding);
    sideLabelWidth = juce::jmax(sideLabelWidth, measureTextWidth(titleFont, "OUT") + PositionDisplayMetrics::titleWidthPadding);

    const auto leftValueWidth = juce::jmax(measureTextWidth(sideValueFont, m_snapshot.bpmText),
                                           measureTextWidth(sideValueFont, m_snapshot.timeSignatureText));
    const auto rightValueWidth = juce::jmax(measureTextWidth(sideValueFont, m_snapshot.loopInText),
                                            measureTextWidth(sideValueFont, m_snapshot.loopOutText));

    const auto leftColumnWidth = juce::roundToInt(sideLabelWidth + PositionDisplayMetrics::leadingLabelGap + leftValueWidth + (PositionDisplayMetrics::fieldHorizontalPadding * 2.0f));
    const auto rightColumnWidth = juce::roundToInt(sideLabelWidth + PositionDisplayMetrics::leadingLabelGap + rightValueWidth + (PositionDisplayMetrics::fieldHorizontalPadding * 2.0f));
    const auto minimumCenterWidth = juce::roundToInt(area.getWidth() * PositionDisplayMetrics::minimumCenterWidthRatio);
    const auto maximumSideWidth = juce::jmax(0, (area.getWidth() - minimumCenterWidth - (PositionDisplayMetrics::panelColumnGap * 2)) / 2);

    auto leftColumn = area.removeFromLeft(juce::jmin(leftColumnWidth, maximumSideWidth));
    m_leftGroupSeparatorX = area.getX() + (PositionDisplayMetrics::panelColumnGap / 2);
    area.removeFromLeft(PositionDisplayMetrics::panelColumnGap);

    auto rightColumn = area.removeFromRight(juce::jmin(rightColumnWidth, maximumSideWidth));
    m_rightGroupSeparatorX = rightColumn.getX() - ((PositionDisplayMetrics::panelColumnGap + 1) / 2);
    area.removeFromRight(PositionDisplayMetrics::panelColumnGap);
    auto centerColumn = area;

    const auto splitColumn = [] (juce::Rectangle<int> column)
    {
        auto top = column.removeFromTop((column.getHeight() - PositionDisplayMetrics::panelRowGap) / 2);
        column.removeFromTop(PositionDisplayMetrics::panelRowGap);
        return std::pair{top, column};
    };

    const auto [bpmBounds, timeSignatureBounds] = splitColumn(leftColumn);
    const auto [loopInBounds, loopOutBounds] = splitColumn(rightColumn);
    const auto [positionBounds, timeBounds] = splitColumn(centerColumn);

    m_bpmField->setBounds(bpmBounds);
    m_timeSignatureField->setBounds(timeSignatureBounds);
    m_positionField->setBounds(positionBounds);
    m_timeField->setBounds(timeBounds);
    m_loopInField->setBounds(loopInBounds);
    m_loopOutField->setBounds(loopOutBounds);
}

void PositionDisplayComponent::handleAsyncUpdate()
{
    if (m_needsTempoListenerRebuild)
        rebuildTempoSequenceListeners();

    refreshFromModel();
}

void PositionDisplayComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &)
{
    scheduleRefresh();
}

void PositionDisplayComponent::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &)
{
    scheduleRefresh(true);
}

void PositionDisplayComponent::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int)
{
    scheduleRefresh(true);
}

void PositionDisplayComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int)
{
    scheduleRefresh(true);
}

void PositionDisplayComponent::valueTreeParentChanged(juce::ValueTree &)
{
    scheduleRefresh(true);
}

void PositionDisplayComponent::valueTreeRedirected(juce::ValueTree &)
{
    scheduleRefresh(true);
}

void PositionDisplayComponent::scheduleRefresh(bool rebuildTempoListeners)
{
    m_needsTempoListenerRebuild = m_needsTempoListenerRebuild || rebuildTempoListeners;
    triggerAsyncUpdate();
}

void PositionDisplayComponent::rebuildTempoSequenceListeners()
{
    for (auto state : m_tempoItemStates)
        state.removeListener(this);

    m_tempoItemStates.clearQuick();

    auto addObservedState = [this](juce::ValueTree state)
    {
        if (!state.isValid())
            return;

        state.addListener(this);
        m_tempoItemStates.add(state);
    };

    addObservedState(m_edit.tempoSequence.getState());

    for (auto *tempo : m_edit.tempoSequence.getTempos())
        addObservedState(tempo->state);

    for (auto *timeSig : m_edit.tempoSequence.getTimeSigs())
        addObservedState(timeSig->state);

    m_needsTempoListenerRebuild = false;
}

void PositionDisplayComponent::refreshFromModel()
{
    const auto position = m_edit.getTransport().getPosition();
    const auto loopRange = m_edit.getTransport().getLoopRange();
    auto &tempo = m_edit.tempoSequence.getTempoAt(position);
    auto &timeSignature = m_edit.tempoSequence.getTimeSigAt(position);

    m_snapshot.bpm = tempo.getBpm();
    m_snapshot.numerator = timeSignature.numerator;
    m_snapshot.denominator = timeSignature.denominator;
    m_snapshot.position = position;
    m_snapshot.loopRange = loopRange;
    m_snapshot.bpmText = formatBpm(m_snapshot.bpm);
    m_snapshot.timeSignatureText = formatTimeSignature(m_snapshot.numerator, m_snapshot.denominator);
    const auto &tempoSequence = m_edit.tempoSequence.getInternalSequence();
    m_snapshot.positionText = formatBarsBeatsTicks(tempoSequence, m_snapshot.position, te::Edit::ticksPerQuarterNote);
    m_snapshot.timeText = formatTime(m_snapshot.position);
    m_snapshot.loopInText = formatBarsBeatsTicks(tempoSequence, loopRange.getStart(), te::Edit::ticksPerQuarterNote);
    m_snapshot.loopOutText = formatBarsBeatsTicks(tempoSequence, loopRange.getEnd(), te::Edit::ticksPerQuarterNote);

    updateFieldStyles();
    repaint();

    m_bpmField->setDisplayText(m_snapshot.bpmText);
    m_bpmField->setSegments(buildSingleSegment(m_snapshot.bpmText));

    m_timeSignatureField->setDisplayText(m_snapshot.timeSignatureText);
    m_timeSignatureField->setSegments(buildSegmentsFromDelimitedText(m_snapshot.timeSignatureText, "/"));

    m_positionField->setDisplayText(m_snapshot.positionText);
    m_positionField->setSegments(buildSegmentsFromDelimitedText(m_snapshot.positionText, "."));

    m_timeField->setDisplayText(m_snapshot.timeText);
    m_timeField->setSegments(buildSegmentsFromDelimitedText(m_snapshot.timeText, ":."));

    m_loopInField->setDisplayText(m_snapshot.loopInText);
    m_loopInField->setSegments(buildSegmentsFromDelimitedText(m_snapshot.loopInText, "."));

    m_loopOutField->setDisplayText(m_snapshot.loopOutText);
    m_loopOutField->setSegments(buildSegmentsFromDelimitedText(m_snapshot.loopOutText, "."));
}

void PositionDisplayComponent::updateFieldStyles()
{
    const auto textColour = m_appState.getButtonTextColour();
    const auto highlightColour = textColour.withAlpha(0.15f);
    const auto focusColour = textColour.withAlpha(0.7f);

    const auto titleFont = juce::Font(juce::FontOptions(PositionDisplayMetrics::titleFontHeight));
    const auto sideValueFont = juce::Font(juce::FontOptions(PositionDisplayMetrics::sideValueFontHeight));
    const auto timeFont = juce::Font(juce::FontOptions(PositionDisplayMetrics::timeValueFontHeight));
    const auto positionFont = juce::Font(juce::FontOptions(PositionDisplayMetrics::positionValueFontHeight));

    auto titleWidth = measureTextWidth(titleFont, "BPM") + PositionDisplayMetrics::titleWidthPadding;
    titleWidth = juce::jmax(titleWidth, measureTextWidth(titleFont, "SIG") + PositionDisplayMetrics::titleWidthPadding);
    titleWidth = juce::jmax(titleWidth, measureTextWidth(titleFont, "IN") + PositionDisplayMetrics::titleWidthPadding);
    titleWidth = juce::jmax(titleWidth, measureTextWidth(titleFont, "OUT") + PositionDisplayMetrics::titleWidthPadding);

    for (auto *field : {m_bpmField.get(), m_timeSignatureField.get(), m_positionField.get(), m_timeField.get(), m_loopInField.get(), m_loopOutField.get()})
    {
        field->setColours(textColour, highlightColour, focusColour);
        field->setTitleFont(titleFont);
    }

    for (auto *field : {m_bpmField.get(), m_timeSignatureField.get(), m_loopInField.get(), m_loopOutField.get()})
        field->setFixedTitleWidth(titleWidth);

    const auto centerTitleWidth = juce::jmax(measureTextWidth(titleFont, "POS"),
                                                measureTextWidth(titleFont, "TIME"))
                                  + PositionDisplayMetrics::titleWidthPadding;
    m_positionField->setFixedTitleWidth(centerTitleWidth);
    m_timeField->setFixedTitleWidth(centerTitleWidth);

    m_bpmField->setFixedValueWidth(measureTextWidth(sideValueFont, makeStableWidthReference(m_snapshot.bpmText.isEmpty() ? "888.88" : m_snapshot.bpmText)));
    m_timeSignatureField->setFixedValueWidth(measureTextWidth(sideValueFont, makeStableWidthReference(m_snapshot.timeSignatureText.isEmpty() ? "8 / 8" : m_snapshot.timeSignatureText)));
    m_positionField->setFixedValueWidth(measureTextWidth(positionFont, makeStableWidthReference(m_snapshot.positionText.isEmpty() ? "8.8.888" : m_snapshot.positionText)));
    m_timeField->setFixedValueWidth(measureTextWidth(timeFont, makeStableWidthReference(m_snapshot.timeText.isEmpty() ? "0:00.000" : m_snapshot.timeText)));
    m_loopInField->setFixedValueWidth(measureTextWidth(sideValueFont, makeStableWidthReference(m_snapshot.loopInText.isEmpty() ? "8.8.888" : m_snapshot.loopInText)));
    m_loopOutField->setFixedValueWidth(measureTextWidth(sideValueFont, makeStableWidthReference(m_snapshot.loopOutText.isEmpty() ? "8.8.888" : m_snapshot.loopOutText)));

    m_bpmField->setFont(sideValueFont);
    m_timeSignatureField->setFont(sideValueFont);
    m_positionField->setFont(positionFont);
    m_timeField->setFont(timeFont);
    m_loopInField->setFont(sideValueFont);
    m_loopOutField->setFont(sideValueFont);
}

PositionDisplayField &PositionDisplayComponent::fieldForId(FieldId field)
{
    switch (field)
    {
        case FieldId::bpm: return *m_bpmField;
        case FieldId::timeSignature: return *m_timeSignatureField;
        case FieldId::position: return *m_positionField;
        case FieldId::time: return *m_timeField;
        case FieldId::loopIn: return *m_loopInField;
        case FieldId::loopOut: return *m_loopOutField;
    }

    return *m_bpmField;
}

const PositionDisplayField &PositionDisplayComponent::fieldForId(FieldId field) const
{
    switch (field)
    {
        case FieldId::bpm: return *m_bpmField;
        case FieldId::timeSignature: return *m_timeSignatureField;
        case FieldId::position: return *m_positionField;
        case FieldId::time: return *m_timeField;
        case FieldId::loopIn: return *m_loopInField;
        case FieldId::loopOut: return *m_loopOutField;
    }

    return *m_bpmField;
}

void PositionDisplayComponent::beginDrag(FieldId field, int segmentIndex)
{
    m_dragState.active = true;
    m_dragState.field = field;
    m_dragState.segment = segmentIndex;
    m_dragState.anchor = m_snapshot;

    if (field == FieldId::position || field == FieldId::time)
        m_edit.getTransport().setUserDragging(true);
}

void PositionDisplayComponent::updateDrag(FieldId field, int segmentIndex, int stepDelta, juce::ModifierKeys modifiers)
{
    if (!m_dragState.active || m_dragState.field != field)
        return;

    switch (field)
    {
        case FieldId::bpm:
        {
            const auto stepSize = modifiers.isAltDown() ? 0.01 : (modifiers.isShiftDown() ? 0.1 : 1.0);
            const auto bpm = juce::jlimit(te::TempoSetting::minBPM,
                                         te::TempoSetting::maxBPM,
                                         m_dragState.anchor.bpm + (stepDelta * stepSize));
            const auto currentBpm = m_edit.tempoSequence.getTempoAt(m_edit.getTransport().getPosition()).getBpm();

            if (bpm != currentBpm)
            {
                if (!m_dragState.undoTransactionStarted)
                {
                    m_edit.getUndoManager().beginNewTransaction("Adjust Tempo");
                    m_dragState.undoTransactionStarted = true;
                }

                applyTempo(bpm);
            }
            break;
        }

        case FieldId::timeSignature:
        {
            auto numerator = m_dragState.anchor.numerator;
            auto denominator = m_dragState.anchor.denominator;

            if (segmentIndex == 0)
            {
                const auto stepSize = modifiers.isShiftDown() ? 1 : 4;
                numerator = juce::jlimit(1, 64, numerator + (stepDelta * stepSize));
            }
            else
            {
                const auto startIndex = getDenominatorIndex(denominator);
                denominator = getDenominatorForIndex(startIndex + stepDelta);
            }

            const auto &current = m_edit.tempoSequence.getTimeSigAt(m_edit.getTransport().getPosition());
            if (numerator != current.numerator || denominator != current.denominator)
            {
                if (!m_dragState.undoTransactionStarted)
                {
                    m_edit.getUndoManager().beginNewTransaction("Adjust Time Signature");
                    m_dragState.undoTransactionStarted = true;
                }

                applyTimeSignature(numerator, denominator);
            }
            break;
        }

        case FieldId::position:
        case FieldId::loopIn:
        case FieldId::loopOut:
        {
            const auto anchorTime = field == FieldId::position ? m_dragState.anchor.position
                                                               : (field == FieldId::loopIn ? m_dragState.anchor.loopRange.getStart()
                                                                                           : m_dragState.anchor.loopRange.getEnd());

            const auto partValues = getBarsBeatsTicksParts(m_edit, anchorTime);
            const auto multiplier = modifiers.isShiftDown() ? 1 : (segmentIndex == 2 ? 10 : 4);

            int newValue = 0;
            int formatPart = 0;

            if (segmentIndex == 0)
            {
                newValue = partValues.bar + (stepDelta * multiplier);
                formatPart = 2;
            }
            else if (segmentIndex == 1)
            {
                newValue = partValues.beat + (stepDelta * multiplier);
                formatPart = 1;
            }
            else
            {
                newValue = partValues.tick + (stepDelta * multiplier);
                formatPart = 0;
            }

            te::TimecodeDisplayFormat format(te::TimecodeType::barsBeats);
            const auto newDuration = format.getNewTimeWithPartValue(te::TimecodeDuration::fromSecondsOnly(tracktion::TimeDuration::fromSeconds(anchorTime.inSeconds())),
                                                                    m_edit.tempoSequence,
                                                                    formatPart,
                                                                    newValue,
                                                                    false);
            const auto newTime = clampToValidPosition(timeFromDuration(newDuration));

            if (field == FieldId::position)
                applyTransportPosition(newTime);
            else if (field == FieldId::loopIn)
                applyLoopIn(newTime);
            else
                applyLoopOut(newTime);

            break;
        }

        case FieldId::time:
        {
            const auto anchorTime = m_dragState.anchor.position;
            const auto showHours = tracktion::abs(anchorTime).inSeconds() >= 3600.0;
            const auto isMilliseconds = segmentIndex == (showHours ? 3 : 2);
            const auto unitScale = modifiers.isShiftDown() ? 1.0 : 10.0;

            double secondsPerStep = 0.0;

            if (showHours)
            {
                if (segmentIndex == 0)
                    secondsPerStep = 3600.0;
                else if (segmentIndex == 1)
                    secondsPerStep = 60.0;
                else if (segmentIndex == 2)
                    secondsPerStep = 1.0;
            }
            else
            {
                if (segmentIndex == 0)
                    secondsPerStep = 60.0;
                else if (segmentIndex == 1)
                    secondsPerStep = 1.0;
            }

            if (isMilliseconds)
                secondsPerStep = modifiers.isAltDown() || modifiers.isShiftDown() ? 0.001 : 0.01;
            else
                secondsPerStep *= unitScale;

            applyTransportPosition(clampToValidPosition(anchorTime + tracktion::TimeDuration::fromSeconds(stepDelta * secondsPerStep)));
            break;
        }
    }
}

void PositionDisplayComponent::endDrag(FieldId field)
{
    if (!m_dragState.active || m_dragState.field != field)
        return;

    if (field == FieldId::position || field == FieldId::time)
        m_edit.getTransport().setUserDragging(false);

    if (m_dragState.undoTransactionStarted)
        m_edit.getUndoManager().beginNewTransaction();

    m_dragState = {};
}

bool PositionDisplayComponent::commitBpm(const juce::String &text)
{
    const auto bpm = parseStrictDouble(text);

    if (!bpm.has_value())
        return false;

    const auto targetBpm = juce::jlimit(te::TempoSetting::minBPM, te::TempoSetting::maxBPM, *bpm);
    const auto currentBpm = m_edit.tempoSequence.getTempoAt(m_edit.getTransport().getPosition()).getBpm();

    if (targetBpm != currentBpm)
    {
        auto &undoManager = m_edit.getUndoManager();
        undoManager.beginNewTransaction("Set Tempo");
        applyTempo(targetBpm);
        undoManager.beginNewTransaction();
    }

    return true;
}

bool PositionDisplayComponent::commitTimeSignature(const juce::String &text)
{
    const auto timeSignature = parseTimeSignatureValue(text);

    if (!timeSignature.has_value())
        return false;

    const auto &current = m_edit.tempoSequence.getTimeSigAt(m_edit.getTransport().getPosition());
    if (timeSignature->first != current.numerator || timeSignature->second != current.denominator)
    {
        auto &undoManager = m_edit.getUndoManager();
        undoManager.beginNewTransaction("Set Time Signature");
        applyTimeSignature(timeSignature->first, timeSignature->second);
        undoManager.beginNewTransaction();
    }

    return true;
}

bool PositionDisplayComponent::commitTransportPositionFromBarsBeats(const juce::String &text)
{
    const auto position = parseBarsBeatsTicks(m_edit.tempoSequence.getInternalSequence(), text, te::Edit::ticksPerQuarterNote);

    if (!position.has_value())
        return false;

    applyTransportPosition(*position);
    return true;
}

bool PositionDisplayComponent::commitTransportPositionFromTime(const juce::String &text)
{
    const auto position = parseTimeValue(text);

    if (!position.has_value())
        return false;

    applyTransportPosition(*position);
    return true;
}

bool PositionDisplayComponent::commitLoopIn(const juce::String &text)
{
    const auto position = parseBarsBeatsTicks(m_edit.tempoSequence.getInternalSequence(), text, te::Edit::ticksPerQuarterNote);

    if (!position.has_value())
        return false;

    applyLoopIn(*position);
    return true;
}

bool PositionDisplayComponent::commitLoopOut(const juce::String &text)
{
    const auto position = parseBarsBeatsTicks(m_edit.tempoSequence.getInternalSequence(), text, te::Edit::ticksPerQuarterNote);

    if (!position.has_value())
        return false;

    applyLoopOut(*position);
    return true;
}

void PositionDisplayComponent::applyTempo(double bpm)
{
    auto &transport = m_edit.getTransport();
    const auto beatPosition = m_edit.tempoSequence.toBeats(transport.getPosition());

    auto &tempo = m_edit.tempoSequence.getTempoAt(transport.getPosition());
    tempo.setBpm(juce::jlimit(te::TempoSetting::minBPM, te::TempoSetting::maxBPM, bpm));

    transport.setPosition(clampToValidPosition(m_edit.tempoSequence.toTime(beatPosition)));
}

void PositionDisplayComponent::applyTimeSignature(int numerator, int denominator)
{
    auto &timeSignature = m_edit.tempoSequence.getTimeSigAt(m_edit.getTransport().getPosition());
    timeSignature.numerator = juce::jlimit(1, 64, numerator);
    timeSignature.denominator = juce::jlimit(1, 64, denominator);
}

void PositionDisplayComponent::applyTransportPosition(tracktion::TimePosition position)
{
    m_edit.getTransport().setPosition(clampToValidPosition(position));
}

void PositionDisplayComponent::applyLoopIn(tracktion::TimePosition position)
{
    const auto loopOut = m_edit.getTransport().getLoopRange().getEnd();
    m_edit.getTransport().setLoopIn(juce::jmin(clampToValidPosition(position), loopOut));
}

void PositionDisplayComponent::applyLoopOut(tracktion::TimePosition position)
{
    const auto loopIn = m_edit.getTransport().getLoopRange().getStart();
    m_edit.getTransport().setLoopOut(juce::jmax(clampToValidPosition(position), loopIn));
}
