
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


#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "LevelMeterComponent.h"
#include "LowerRangeComponent.h"
#include "RackView.h"
#include "AutomatableSliderComponent.h"

namespace te = tracktion_engine;

class ColourGridComponent : public juce::PopupMenu::CustomComponent
{
public:
    ColourGridComponent(tracktion::Track::Ptr track,
                        const juce::Array<juce::Colour>& colours, 
                        juce::Colour currentColour, 
                        int cols, int rows)
        : juce::PopupMenu::CustomComponent(false),
        m_track (track),
        m_colours(colours),
        m_currentColour(currentColour),
        m_cols(cols),
        m_rows(rows),
        m_selectedIndex(-1)
    {
        // Aktuellen Farbindex bestimmen
        for (size_t i = 0; i < m_colours.size(); ++i)
            if (m_colours[i] == m_currentColour)
                m_selectedIndex = static_cast<int>(i);
    }

    void getIdealSize(int& idealWidth, int& idealHeight) override
    {
        // Ideale Größe für das Farbraster
        int cellSize = 24;
        int padding = 4;
        idealWidth = m_cols * cellSize + (m_cols + 1) * padding;
        idealHeight = m_rows * cellSize + (m_rows + 1) * padding;
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::PopupMenu::backgroundColourId));

        int cellSize = 24;
        int padding = 4;
        int index = 0;

        // Raster zeichnen
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                if (index < (int)m_colours.size())
                {
                    int x = padding + col * (cellSize + padding);
                    int y = padding + row * (cellSize + padding);

                    // Farbfeld zeichnen
                    g.setColour(m_colours[index]);
                    g.fillRect(x, y, cellSize, cellSize);

                    // Rahmen zeichnen
                    g.setColour(juce::Colours::black);
                    g.drawRect(x, y, cellSize, cellSize, 1);

                    // Aktuelle Farbe markieren
                    if (index == m_selectedIndex)
                    {
                        g.setColour(juce::Colours::white);
                        g.drawRect(x - 2, y - 2, cellSize + 4, cellSize + 4, 2);
                    }

                    // Hover-Effekt
                    if (index == m_hoverIndex)
                    {
                        g.setColour(juce::Colours::white.withAlpha(0.3f));
                        g.drawRect(x - 1, y - 1, cellSize + 2, cellSize + 2, 1);
                    }

                    index++;
                }
            }
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        updateHoverIndex(e.getPosition());
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (m_hoverIndex != -1)
        {
            m_hoverIndex = -1;
            repaint();
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int index = getIndexAtPosition(e.getPosition());
        if (index >= 0 && index < (int)m_colours.size())
        {
            std::cout << "FARBINEDX: " << index << std::endl;
            m_track->setColour(m_colours[index]);
            m_selectedIndex = index;
            getParentComponent()->exitModalState(3000 + index);
            triggerMenuItem();
        }
    }

private:
    void updateHoverIndex(juce::Point<int> position)
    {
        int newIndex = getIndexAtPosition(position);
        if (newIndex != m_hoverIndex)
        {
            m_hoverIndex = newIndex;
            repaint();
        }
    }

    int getIndexAtPosition(juce::Point<int> position)
    {
        int cellSize = 24;
        int padding = 4;

        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                int index = row * m_cols + col;
                if (index < (int)m_colours.size())
                {
                    int x = padding + col * (cellSize + padding);
                    int y = padding + row * (cellSize + padding);

                    if (position.x >= x && position.x < x + cellSize &&
                        position.y >= y && position.y < y + cellSize)
                    {
                        return index;
                    }
                }
            }
        }

        return -1;
    }

    tracktion::Track::Ptr m_track;
    juce::Array<juce::Colour> m_colours;
    juce::Colour m_currentColour;
    int m_cols;
    int m_rows;
    int m_selectedIndex;
    int m_hoverIndex = -1;
};


//=================================================================================================================


class AutomationLaneHeaderComponent : public juce::Component
{
public:
    explicit AutomationLaneHeaderComponent(te::AutomatableParameter::Ptr ap, EditViewState& evs);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label m_parameterName;
    juce::Label m_pluginName;
    te::AutomatableParameter::Ptr m_automatableParameter;
    EditViewState& m_evs;
    int m_heightAtMouseDown = 0, m_mouseDownY = 0;
    bool m_resizing = false, m_hovering = false;
    AutomatableSliderComponent m_slider;

    // MouseListener interface
public:
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag (const juce::MouseEvent &event) override;
    void mouseMove (const juce::MouseEvent &event) override;
    void mouseExit (const juce::MouseEvent &) override;
    [[nodiscard]] te::AutomatableParameter::Ptr automatableParameter() const;
};

class TrackHeaderComponent : public juce::Component
                           , private te::ValueTreeAllEventListener
                           , private FlaggedAsyncUpdater
                           , public juce::DragAndDropTarget
                           , public juce::Label::Listener
                           , public juce::ChangeBroadcaster
{
public:
    TrackHeaderComponent (EditViewState&, te::Track::Ptr);
    ~TrackHeaderComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void mouseMove(const juce::MouseEvent & e) override;
    void mouseExit(const juce::MouseEvent &event) override;
    juce::Colour getTrackColour();

    [[nodiscard]] te::Track::Ptr getTrack() const;
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& details) override;

    void labelTextChanged(juce::Label *labelThatHasChanged) override;
    void childrenSetVisible(bool v);

    bool isFolderTrack(){ return m_track->isFolderTrack(); }

    void collapseTrack(bool minimize);


private:
    void handleAsyncUpdate() override;
    void valueTreeChanged() override {}
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeChildAdded(
            juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(
            juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved
            , int indexFromWhichChildWasRemoved) override;

    void showPopupMenu(te::Track* at);
    void deleteTrackFromEdit();

    EditViewState& m_editViewState;
    te::Track::Ptr m_track;
    int m_trackHeightATMouseDown{};
    int m_yPosAtMouseDown{};
    juce::ValueTree inputsState;
    juce::Label m_trackName;
    juce::ToggleButton m_armButton,
                 m_muteButton,
                 m_soloButton;

    std::unique_ptr<AutomatableSliderComponent> m_volumeKnob;
    std::unique_ptr<LevelMeterComponent> levelMeterComp;
    juce::Image m_dragImage;
    bool m_isResizing {false},
         m_isHover {false},
         m_contentIsOver {false},
         m_trackIsOver {false},
         m_isDragging {false},
         m_isAudioTrack {false},
         m_updateAutomationLanes {false},
         m_updateTrackHeight {false};
    void buildAutomationHeader();
    juce::OwnedArray<AutomationLaneHeaderComponent> m_automationHeaders;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackHeaderComponent)
};
