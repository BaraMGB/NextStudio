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

#include "LowerRangeComponent.h"
#include "Utilities.h"

class LowerRangeCollapsedButton final : public juce::Button
{
public:
    LowerRangeCollapsedButton(const juce::String &label, const char *svgData, ApplicationViewState &appState)
        : juce::Button(label),
          m_svgData(svgData),
          m_appState(appState)
    {
        setButtonText(label);
        refreshIcon();
    }

    void refreshIcon()
    {
        m_icon = GUIHelpers::getDrawableFromSvg(m_svgData, m_appState.getButtonTextColour());
        repaint();
    }

    void paintButton(juce::Graphics &g, bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        auto background = m_appState.getButtonBackgroundColour();

        if (getToggleState())
            background = m_appState.getPrimeColour().withAlpha(0.35f);
        else if (isButtonDown)
            background = background.darker(0.2f);
        else if (isMouseOverButton)
            background = background.brighter(0.12f);

        const auto opacity = isEnabled() ? 1.0f : 0.35f;
        g.setColour(background.withMultipliedAlpha(opacity));
        g.fillRoundedRectangle(bounds, 3.0f);

        auto content = getLocalBounds().reduced(8, 3);
        auto iconArea = content.removeFromLeft(content.getHeight());
        if (m_icon != nullptr)
            m_icon->drawWithin(g, iconArea.toFloat(), juce::RectanglePlacement::centred, opacity);

        content.removeFromLeft(6);
        g.setColour(m_appState.getButtonTextColour().withMultipliedAlpha(opacity));
        g.drawText(getButtonText(), content, juce::Justification::centredLeft, false);
    }

private:
    const char *m_svgData;
    ApplicationViewState &m_appState;
    std::unique_ptr<juce::Drawable> m_icon;
};

//------------------------------------------------------------------------------
LowerRangeComponent::LowerRangeComponent(EditViewState &evs)
    : m_evs(evs),
      m_pluginChainView(evs),
      m_pianoRollEditor(evs),
      m_mixer(evs),
      m_tabBar(evs),
      m_splitter()
{
    m_evs.setLowerRangeView(LowerRangeView::mixer);
    m_collapsedMixerButton = std::make_unique<LowerRangeCollapsedButton>("Mixer", BinaryData::headphonessettings_svg, m_evs.m_applicationState);
    m_collapsedMidiEditorButton = std::make_unique<LowerRangeCollapsedButton>("MidiEditor", BinaryData::piano_svg, m_evs.m_applicationState);
    m_collapsedPluginChainButton = std::make_unique<LowerRangeCollapsedButton>("PluginChain", BinaryData::powerplug_svg, m_evs.m_applicationState);

    addAndMakeVisible(m_tabBar);
    addAndMakeVisible(m_splitter);
    addChildComponent(*m_collapsedMixerButton);
    addChildComponent(*m_collapsedMidiEditorButton);
    addChildComponent(*m_collapsedPluginChainButton);
    addChildComponent(m_pianoRollEditor);
    addAndMakeVisible(m_pluginChainView);
    addAndMakeVisible(m_mixer);
    m_evs.m_edit.state.addListener(this);
    m_evs.m_applicationState.m_applicationStateValueTree.addListener(this);
    m_evs.m_selectionManager.addChangeListener(this);

    m_tabBar.onTabSelected = [this](LowerRangeView view) { m_evs.setLowerRangeView(view); };

    const auto openCollapsedView = [this](LowerRangeView view)
    {
        EngineHelpers::openLowerRangeView(m_evs, static_cast<int>(view), true);
    };
    m_collapsedMixerButton->onClick = [openCollapsedView] { openCollapsedView(LowerRangeView::mixer); };
    m_collapsedMidiEditorButton->onClick = [openCollapsedView] { openCollapsedView(LowerRangeView::midiEditor); };
    m_collapsedPluginChainButton->onClick = [openCollapsedView] { openCollapsedView(LowerRangeView::pluginRack); };

    m_splitter.onMouseDown = [this]() { handleSplitterMouseDown(); };

    m_splitter.onDrag = [this](int dragDistance) { handleSplitterDrag(dragDistance); };

    updateCollapsedButtons();
    updateView();
}

int LowerRangeComponent::getMaximumExpandedHeight() const
{
    if (auto *parent = getParentComponent())
        return LowerRangeLayout::getMaximumExpandedHeight(parent->getHeight(), (int)m_evs.m_timeLineHeight);

    return LowerRangeLayout::defaultExpandedHeight;
}

void LowerRangeComponent::handleSplitterMouseDown()
{
    m_pianorollHeightAtMousedown = LowerRangeLayout::clampExpandedHeight((int)m_evs.m_midiEditorHeight, getMaximumExpandedHeight());
    m_cachedPianoNoteNum = (double)m_evs.getViewYScroll(m_pianoRollEditor.getTimeLineComponent().getTimeLineID());

    const auto expandedHeight = m_evs.getLowerRangeView() == LowerRangeView::midiEditor
                                    ? m_pianorollHeightAtMousedown
                                    : defaultExpandedHeight;
    m_splitterCollapseController.beginDrag(m_evs.m_applicationState.m_lowerRangeCollapsed,
                                           LowerRangeLayout::getTransitionDistance(expandedHeight,
                                                                                   (bool)m_evs.m_applicationState.m_lowerRangeCollapsed));
}

void LowerRangeComponent::handleSplitterDrag(int dragDistance)
{
    const bool collapsed = m_evs.m_applicationState.m_lowerRangeCollapsed;
    const bool requestedCollapsed = m_splitterCollapseController.getCollapsedState(dragDistance, collapsed);

    if (requestedCollapsed != collapsed)
    {
        m_evs.m_applicationState.m_lowerRangeCollapsed = requestedCollapsed;
        return;
    }

    if (m_splitterCollapseController.startedCollapsed() || collapsed)
        return;

    if (m_evs.getLowerRangeView() == LowerRangeView::midiEditor)
    {
        const auto maximumExpandedHeight = getMaximumExpandedHeight();
        const auto newHeight = LowerRangeLayout::getResizedHeight(m_pianorollHeightAtMousedown, dragDistance, maximumExpandedHeight);
        const auto appliedDragDistance = LowerRangeLayout::getAppliedDragDistance(m_pianorollHeightAtMousedown, dragDistance, maximumExpandedHeight);
        auto noteHeight = (double)m_evs.getViewYScale(m_pianoRollEditor.getTimeLineComponent().getTimeLineID());
        auto noteDist = appliedDragDistance / noteHeight;

        m_evs.setYScroll(m_pianoRollEditor.getTimeLineComponent().getTimeLineID(),
                         juce::jlimit(0.0, 127.0 - (newHeight / noteHeight), m_cachedPianoNoteNum + noteDist));
        m_evs.m_midiEditorHeight = newHeight;
    }
}

LowerRangeComponent::~LowerRangeComponent()
{
    m_evs.m_selectionManager.removeChangeListener(this);
    m_evs.m_applicationState.m_applicationStateValueTree.removeListener(this);
    m_evs.m_edit.state.removeListener(this);
}

te::Track::Ptr LowerRangeComponent::getTrackMarkedForLowerRange() const
{
    for (auto *track : te::getAllTracks(m_evs.m_edit))
        if (track != nullptr && static_cast<bool>(track->state.getProperty(IDs::showLowerRange, false)))
            return track;

    return {};
}

te::Track::Ptr LowerRangeComponent::getSelectedTrackForLowerRange() const
{
    if (auto *selectedTrack = m_evs.m_selectionManager.getFirstItemOfType<te::Track>())
        return selectedTrack;

    if (auto *selectedClip = m_evs.m_selectionManager.getFirstItemOfType<te::Clip>())
        return selectedClip->getClipTrack();

    return {};
}

void LowerRangeComponent::syncActiveTrack(bool forceRefresh)
{
    auto targetTrack = getSelectedTrackForLowerRange();
    if (targetTrack == nullptr)
        targetTrack = getTrackMarkedForLowerRange();

    switch (m_evs.getLowerRangeView())
    {
    case LowerRangeView::pluginRack:
        if (targetTrack != nullptr)
            m_pluginChainView.setTrack(targetTrack, forceRefresh);
        else
            m_pluginChainView.clearTrack();
        break;

    case LowerRangeView::midiEditor:
        if (targetTrack != nullptr)
            m_pianoRollEditor.setTrack(targetTrack, forceRefresh);
        break;

    case LowerRangeView::mixer:
    case LowerRangeView::none:
        break;
    }
}

void LowerRangeComponent::paint(juce::Graphics &g)
{
    auto area = getLocalBounds();
    area.removeFromTop(splitterHeight);

    g.setColour(m_evs.m_applicationState.getBackgroundColour1());
    g.fillRect(area);

    if (m_evs.m_applicationState.m_lowerRangeCollapsed)
    {
        auto collapsedBar = area.removeFromTop(collapsedBarHeight);
        g.setColour(m_evs.m_applicationState.getBorderColour());
        g.drawHorizontalLine(collapsedBar.getBottom() - 1, collapsedBar.getX(), collapsedBar.getRight());
    }
}

void LowerRangeComponent::paintOverChildren(juce::Graphics &g)
{
    auto area = getLocalBounds();
    area.removeFromTop(splitterHeight);
    GUIHelpers::drawFakeRoundCorners(g, area.toFloat(), m_evs.m_applicationState.getMainFrameColour(), m_evs.m_applicationState.getBorderColour());
}

void LowerRangeComponent::resized()
{
    auto area = getLocalBounds();
    auto splitter = area.removeFromTop(splitterHeight);
    splitter.reduce(10, 1);
    m_splitter.setBounds(splitter);

    if (m_evs.m_applicationState.m_lowerRangeCollapsed)
    {
        auto collapsedBar = area.removeFromTop(collapsedBarHeight).reduced(4, 2);
        constexpr int gap = 4;
        const auto buttonWidth = juce::jmin(150, juce::jmax(1, (collapsedBar.getWidth() - (2 * gap)) / 3));

        m_collapsedMixerButton->setBounds(collapsedBar.removeFromLeft(buttonWidth));
        collapsedBar.removeFromLeft(gap);
        m_collapsedMidiEditorButton->setBounds(collapsedBar.removeFromLeft(buttonWidth));
        collapsedBar.removeFromLeft(gap);
        m_collapsedPluginChainButton->setBounds(collapsedBar.removeFromLeft(buttonWidth));
        return;
    }

    auto leftArea = area.removeFromLeft(menuBarWidth);
    m_tabBar.setBounds(leftArea.reduced(10, 0));

    m_pluginChainView.setBounds(area);
    m_mixer.setBounds(area);
    m_pianoRollEditor.setBounds(area);
}

void LowerRangeComponent::updateView()
{
    const auto currentView = m_evs.getLowerRangeView();
    const bool expanded = !m_evs.m_applicationState.m_lowerRangeCollapsed;

    m_tabBar.setVisible(expanded);
    m_collapsedMixerButton->setVisible(!expanded);
    m_collapsedMidiEditorButton->setVisible(!expanded);
    m_collapsedPluginChainButton->setVisible(!expanded);
    m_pluginChainView.setVisible(expanded && currentView == LowerRangeView::pluginRack);
    m_pianoRollEditor.setVisible(expanded && currentView == LowerRangeView::midiEditor);
    m_mixer.setVisible(expanded && currentView == LowerRangeView::mixer);

    updateCollapsedButtons();
    if (expanded)
        syncActiveTrack(true);

    resized();
    repaint();
}

void LowerRangeComponent::updateCollapsedButtons()
{
    const auto currentView = m_evs.getLowerRangeView();
    m_collapsedMixerButton->setToggleState(currentView == LowerRangeView::mixer, juce::dontSendNotification);
    m_collapsedMidiEditorButton->setToggleState(currentView == LowerRangeView::midiEditor, juce::dontSendNotification);
    m_collapsedPluginChainButton->setToggleState(currentView == LowerRangeView::pluginRack, juce::dontSendNotification);

    bool midiEditorEnabled = false;
    if (auto *clip = dynamic_cast<te::Clip *>(m_evs.m_selectionManager.getSelectedObject(0)))
        midiEditorEnabled = clip->isMidi();
    m_collapsedMidiEditorButton->setEnabled(midiEditorEnabled);
}

void LowerRangeComponent::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
    if (i == IDs::LowerRangeCollapsed)
    {
        updateView();
        return;
    }

    if (v.hasType(IDs::ThemeState))
    {
        m_collapsedMixerButton->refreshIcon();
        m_collapsedMidiEditorButton->refreshIcon();
        m_collapsedPluginChainButton->refreshIcon();
    }

    if (i == IDs::lowerRangeView)
    {
        updateView();
        return;
    }

    if (v.hasType(te::IDs::TRACK) || v.hasType(te::IDs::FOLDERTRACK) || v.hasType(te::IDs::MASTERTRACK))
    {
        if (i == IDs::showLowerRange)
        {
            NS_LOG_DEBUG(viewstate, "lower range track visibility changed");
            if (auto track = te::findTrackForState(m_evs.m_edit, v))
            {
                NS_LOG_DEBUG(viewstate, "lower range active track candidate: " + track->getName());
                if ((bool)v.getProperty(IDs::showLowerRange) == true)
                    syncActiveTrack(true);
            }
        }
    }
    if (v.hasType(tracktion_engine::IDs::MIDICLIP))
    {
        resized();
        repaint();
    }
    if (v.hasType(m_pianoRollEditor.getTimeLineComponent().getTimeLineID()))
    {
        resized();
        repaint();
    }
}

// if a new track is added, make its rackview visible
void LowerRangeComponent::valueTreeChildAdded(juce::ValueTree &v, juce::ValueTree &)
{
    juce::ignoreUnused(v);
    resized();
    repaint();
}

void LowerRangeComponent::valueTreeChildRemoved(juce::ValueTree &v, juce::ValueTree &i, int)
{
    if (i.getProperty(te::IDs::id).toString() == m_pluginChainView.getCurrentTrackID())
    {
        m_pluginChainView.clearTrack();
    }

    resized();
    repaint();
}

void LowerRangeComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int)
{
    resized();
    repaint();
}

void LowerRangeComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (source == &m_evs.m_selectionManager)
    {
        syncActiveTrack(false);
        updateCollapsedButtons();
    }
}
