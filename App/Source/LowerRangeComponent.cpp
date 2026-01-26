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

//------------------------------------------------------------------------------
LowerRangeComponent::LowerRangeComponent(EditViewState &evs)
    : m_evs(evs), m_rackView(evs), m_pianoRollEditor(evs), m_mixer(evs), m_tabBar(evs), m_splitter()
{
    m_evs.setLowerRangeView(LowerRangeView::mixer);
    addAndMakeVisible(m_tabBar);
    addAndMakeVisible(m_splitter);
    addChildComponent(m_pianoRollEditor);
    addAndMakeVisible(m_rackView);
    addAndMakeVisible(m_mixer);
    m_evs.m_edit.state.addListener(this);

    m_tabBar.onTabSelected = [this](LowerRangeView view) { m_evs.setLowerRangeView(view); };

    m_splitter.onMouseDown = [this]() { handleSplitterMouseDown(); };

    m_splitter.onDrag = [this](int dragDistance) { handleSplitterDrag(dragDistance); };

    updateView();
}

void LowerRangeComponent::handleSplitterMouseDown()
{
    m_pianorollHeightAtMousedown = m_evs.m_midiEditorHeight;
    m_cachedPianoNoteNum = (double)m_evs.getViewYScroll(m_pianoRollEditor.getTimeLineComponent().getTimeLineID());
}

void LowerRangeComponent::handleSplitterDrag(int dragDistance)
{
    if (m_evs.getLowerRangeView() == LowerRangeView::midiEditor) {
        auto newHeight = static_cast<int>(m_pianorollHeightAtMousedown - dragDistance);
        auto noteHeight = (double)m_evs.getViewYScale(m_pianoRollEditor.getTimeLineComponent().getTimeLineID());
        auto noteDist = dragDistance / noteHeight;

        m_evs.setYScroll(m_pianoRollEditor.getTimeLineComponent().getTimeLineID(),
                         juce::jlimit(0.0, 127.0 - (getHeight() / noteHeight), m_cachedPianoNoteNum + noteDist));
        m_evs.m_midiEditorHeight = std::max(20, newHeight);
    }
}

LowerRangeComponent::~LowerRangeComponent()
{
    m_evs.m_edit.state.removeListener(this);
}

void LowerRangeComponent::paint(juce::Graphics &g)
{
    auto rect = getLocalBounds();
    g.setColour(juce::Colour(0xff181818));
    g.fillRect(rect.removeFromBottom(getHeight() - (int)m_splitterHeight).toFloat());
}

void LowerRangeComponent::paintOverChildren(juce::Graphics &g)
{
    auto area = getLocalBounds();
    area.removeFromTop((int)m_splitterHeight);
    GUIHelpers::drawFakeRoundCorners(g, area.toFloat(), m_evs.m_applicationState.getMainFrameColour(),
                                     m_evs.m_applicationState.getBorderColour());
}

void LowerRangeComponent::resized()
{
    auto area = getLocalBounds();
    auto splitter = area.removeFromTop((int)m_splitterHeight);
    splitter.reduce(10, 1);

    m_splitter.setBounds(splitter);

    auto leftArea = area.removeFromLeft(70);
    auto presetArea = leftArea.removeFromTop(100);
    if (m_presetManager) {
        m_presetManager->setBounds(presetArea);
    }
    m_tabBar.setBounds(leftArea);

    m_rackView.setBounds(area);
    m_mixer.setBounds(area);

    if (m_pianoRollEditor.isVisible()) {
        m_pianoRollEditor.setBounds(area);
    }
}

void LowerRangeComponent::updatePresetManager(te::Track *track)
{
    if (auto audioTrack = dynamic_cast<te::AudioTrack *>(track)) {
        // Safety check: If we already have an adapter for THIS track, don't recreate it.
        if (m_presetAdapter != nullptr && &m_presetAdapter->getTrack() == audioTrack)
            return;

        // Strict Ownership Protocol (Lifecycle Management):
        // The PresetManagerComponent holds a reference to the TrackPresetAdapter.
        // Therefore, the Manager MUST be destroyed BEFORE the Adapter to avoid dangling references.
        if (m_presetManager) {
            removeChildComponent(m_presetManager.get());
            m_presetManager.reset();
        }
        m_presetAdapter.reset();

        m_presetAdapter = std::make_unique<TrackPresetAdapter>(*audioTrack, m_evs.m_applicationState);

        m_presetManager = std::make_unique<PresetManagerComponent>(*m_presetAdapter);
        addAndMakeVisible(*m_presetManager);
    }
    else {
        if (m_presetManager) {
            removeChildComponent(m_presetManager.get());
            m_presetManager.reset();
        }
        m_presetAdapter.reset();
    }
    resized();
}

void LowerRangeComponent::updateView()
{
    auto currentView = m_evs.getLowerRangeView();

    m_rackView.setVisible(currentView == LowerRangeView::pluginRack);
    m_pianoRollEditor.setVisible(currentView == LowerRangeView::midiEditor);
    m_mixer.setVisible(currentView == LowerRangeView::mixer);

    resized();
    repaint();
}

void LowerRangeComponent::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
    if (i == IDs::lowerRangeView) {
        updateView();
        return;
    }

    if (v.hasType(te::IDs::TRACK) || v.hasType(te::IDs::FOLDERTRACK) || v.hasType(te::IDs::MASTERTRACK)) {
        if (i == IDs::showLowerRange) {
            te::Track::Ptr track = te::findTrackForState(m_evs.m_edit, v);

            if (track == nullptr && v.hasType(te::IDs::MASTERTRACK))
                track = m_evs.m_edit.getMasterTrack();

            if (track) {
                if ((bool)v.getProperty(IDs::showLowerRange) == true) {
                    updatePresetManager(track);

                    if (m_evs.getLowerRangeView() == LowerRangeView::midiEditor) {
                        m_pianoRollEditor.setTrack(track);
                    }
                    else {
                        m_rackView.setTrack(track);
                    }
                }
            }
        }
    }
    if (v.hasType(tracktion_engine::IDs::MIDICLIP)) {
        resized();
        repaint();
    }
    if (v.hasType(m_pianoRollEditor.getTimeLineComponent().getTimeLineID())) {
        resized();
        repaint();
    }
}

// if a new track is added, make its rackview visible
void LowerRangeComponent::valueTreeChildAdded(juce::ValueTree &v, juce::ValueTree &)
{
    if (m_evs.getLowerRangeView() != LowerRangeView::midiEditor && v.hasType(te::IDs::TRACK)) {
        auto track = te::findTrackForState(m_evs.m_edit, v);
        if (track != nullptr) {
            m_rackView.setTrack(track);
            m_rackView.setVisible(true);
            updatePresetManager(track);
        }
    }
    resized();
    repaint();
}

void LowerRangeComponent::valueTreeChildRemoved(juce::ValueTree &v, juce::ValueTree &i, int)
{
    if (i.getProperty(te::IDs::id).toString() == m_rackView.getCurrentTrackID()) {
        m_rackView.clearTrack();

        if (m_presetManager) {
            removeChildComponent(m_presetManager.get());
            m_presetManager.reset();
        }
        m_presetAdapter.reset();
    }

    resized();
    repaint();
}

void LowerRangeComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int)
{
    resized();
    repaint();
}
