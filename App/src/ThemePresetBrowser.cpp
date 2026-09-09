#include "ThemePresetBrowser.h"

#include <algorithm>

namespace
{
constexpr int titleHeight = 22;
constexpr int searchHeight = 28;
constexpr int rowHeight = 28;
constexpr int gap = 4;
} // namespace

ThemePresetBrowser::ThemePresetBrowser()
{
    m_titleLabel.setJustificationType(juce::Justification::centredLeft);
    m_titleLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    addAndMakeVisible(m_titleLabel);

    m_searchEditor.setMultiLine(false);
    m_searchEditor.setTextToShowWhenEmpty("Search themes", juce::Colours::grey);
    m_searchEditor.setEscapeAndReturnKeysConsumed(true);
    m_searchEditor.onTextChange = [this] { rebuildRows(); };
    addAndMakeVisible(m_searchEditor);

    m_list.setRowHeight(rowHeight);
    m_list.setMultipleSelectionEnabled(false);
    m_list.setClickingTogglesRowSelection(false);
    m_list.setOutlineThickness(1);
    m_list.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    if (auto *viewport = m_list.getViewport())
    {
        viewport->setScrollBarsShown(true, false);
        viewport->addMouseListener(this, false);
    }
    addAndMakeVisible(m_list);
}

ThemePresetBrowser::~ThemePresetBrowser()
{
    if (auto *viewport = m_list.getViewport())
        viewport->removeMouseListener(this);
}

void ThemePresetBrowser::setPresets(const std::vector<ThemePresetModel::Preset> &presets)
{
    m_presets = presets;
    rebuildRows();
}

void ThemePresetBrowser::setActivePreset(const juce::String &name)
{
    m_activePreset = name;
    const auto row = findRowForPreset(name);

    juce::SparseSet<int> selection;
    if (row >= 0)
    {
        selection.addRange(juce::Range<int>(row, row + 1));
        m_list.scrollToEnsureRowIsOnscreen(row);
    }
    m_list.setSelectedRows(selection, juce::dontSendNotification);
    m_list.repaint();
}

void ThemePresetBrowser::setThemeColours(juce::Colour background, juce::Colour text, juce::Colour border, juce::Colour accent)
{
    m_background = background;
    m_text = text;
    m_border = border;
    m_accent = accent;

    m_titleLabel.setColour(juce::Label::textColourId, text.withAlpha(0.82f));
    m_searchEditor.setColour(juce::TextEditor::backgroundColourId, background.darker(0.15f));
    m_searchEditor.setColour(juce::TextEditor::textColourId, text);
    m_searchEditor.setColour(juce::TextEditor::outlineColourId, border);
    m_searchEditor.setColour(juce::TextEditor::focusedOutlineColourId, accent);
    m_searchEditor.setColour(juce::TextEditor::highlightColourId, accent.withAlpha(0.45f));
    m_list.setColour(juce::ListBox::outlineColourId, border);
    repaint();
}

void ThemePresetBrowser::paint(juce::Graphics &g)
{
    g.setColour(m_background.withAlpha(0.45f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}

void ThemePresetBrowser::resized()
{
    auto bounds = getLocalBounds();
    m_titleLabel.setBounds(bounds.removeFromTop(titleHeight).reduced(5, 0));
    m_searchEditor.setBounds(bounds.removeFromTop(searchHeight));
    bounds.removeFromTop(gap);
    m_list.setBounds(bounds);
}

void ThemePresetBrowser::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel)
{
    auto *listViewport = m_list.getViewport();
    if (event.eventComponent == listViewport && listViewport != nullptr)
    {
        const auto *viewed = listViewport->getViewedComponent();
        const auto maximumY = viewed != nullptr ? juce::jmax(0, viewed->getHeight() - listViewport->getMaximumVisibleHeight()) : 0;
        const auto positionY = listViewport->getViewPositionY();
        const auto atBoundary = (wheel.deltaY > 0.0f && positionY <= 0)
                                || (wheel.deltaY < 0.0f && positionY >= maximumY);

        if (!atBoundary)
            return;

        for (auto *parent = getParentComponent(); parent != nullptr; parent = parent->getParentComponent())
        {
            if (auto *outerViewport = dynamic_cast<juce::Viewport *>(parent))
            {
                outerViewport->mouseWheelMove(event.getEventRelativeTo(outerViewport), wheel);
                return;
            }
        }
    }

    juce::Component::mouseWheelMove(event, wheel);
}

int ThemePresetBrowser::getNumRows()
{
    return (int)m_rows.size();
}

void ThemePresetBrowser::paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int)m_rows.size())
        return;

    const auto &row = m_rows[(size_t)rowNumber];
    const auto bounds = juce::Rectangle<int>(0, 0, width, height);

    if (row.presetIndex < 0)
    {
        g.setColour(m_background.brighter(0.06f));
        g.fillRect(bounds);
        g.setColour(m_text.withAlpha(0.62f));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(row.heading, bounds.reduced(7, 0), juce::Justification::centredLeft);
        return;
    }

    const auto &preset = m_presets[(size_t)row.presetIndex];
    const auto isActive = preset.name.equalsIgnoreCase(m_activePreset);

    if (rowIsSelected || isActive)
    {
        g.setColour(m_accent.withAlpha(rowIsSelected ? 0.20f : 0.12f));
        g.fillRect(bounds);
    }

    if (isActive)
    {
        g.setColour(m_accent);
        g.fillRect(0, 3, 3, height - 6);
    }

    auto content = bounds.reduced(8, 0);
    if (preset.builtIn)
    {
        auto badge = content.removeFromRight(48);
        g.setColour(m_text.withAlpha(0.58f));
        g.setFont(juce::FontOptions(10.0f));
        g.drawText("Built-in", badge, juce::Justification::centredRight);
    }

    g.setColour(m_text);
    g.setFont(juce::FontOptions(12.5f, isActive ? juce::Font::bold : juce::Font::plain));
    g.drawFittedText(preset.name, content, juce::Justification::centredLeft, 1);
}

void ThemePresetBrowser::listBoxItemClicked(int row, const juce::MouseEvent &)
{
    activateRow(row);
}

void ThemePresetBrowser::returnKeyPressed(int lastRowSelected)
{
    activateRow(lastRowSelected);
}

juce::String ThemePresetBrowser::getNameForRow(int rowNumber)
{
    if (rowNumber < 0 || rowNumber >= (int)m_rows.size())
        return {};

    const auto &row = m_rows[(size_t)rowNumber];
    return row.presetIndex >= 0 ? m_presets[(size_t)row.presetIndex].name : row.heading;
}

void ThemePresetBrowser::rebuildRows()
{
    m_rows.clear();
    const auto search = m_searchEditor.getText().trim();

    const auto addGroup = [this, &search](bool builtIn, const juce::String &heading)
    {
        std::vector<int> matchingPresets;
        for (size_t i = 0; i < m_presets.size(); ++i)
        {
            const auto &preset = m_presets[i];
            if (preset.builtIn == builtIn && (search.isEmpty() || preset.name.containsIgnoreCase(search)))
                matchingPresets.push_back((int)i);
        }

        if (matchingPresets.empty())
            return;

        m_rows.push_back({-1, heading});
        for (const auto index : matchingPresets)
            m_rows.push_back({index, {}});
    };

    addGroup(true, "BUILT-IN");
    addGroup(false, "CUSTOM");

    const auto visiblePresetCount = (int)std::count_if(m_rows.begin(), m_rows.end(), [](const Row &row)
                                                       { return row.presetIndex >= 0; });
    auto title = "All Themes (" + juce::String((int)m_presets.size()) + ")";
    if (search.isNotEmpty())
        title = "Themes (" + juce::String(visiblePresetCount) + " of " + juce::String((int)m_presets.size()) + ")";
    m_titleLabel.setText(title, juce::dontSendNotification);

    m_list.updateContent();
    setActivePreset(m_activePreset);
    m_list.repaint();
}

void ThemePresetBrowser::activateRow(int row)
{
    if (row < 0 || row >= (int)m_rows.size())
        return;

    const auto presetIndex = m_rows[(size_t)row].presetIndex;
    if (presetIndex < 0 || presetIndex >= (int)m_presets.size())
        return;

    if (onPresetSelected)
        onPresetSelected(m_presets[(size_t)presetIndex].name);
}

int ThemePresetBrowser::findRowForPreset(const juce::String &name) const
{
    const auto found = std::find_if(m_rows.begin(), m_rows.end(), [this, &name](const Row &row)
                                    { return row.presetIndex >= 0 && m_presets[(size_t)row.presetIndex].name.equalsIgnoreCase(name); });
    return found == m_rows.end() ? -1 : (int)std::distance(m_rows.begin(), found);
}
