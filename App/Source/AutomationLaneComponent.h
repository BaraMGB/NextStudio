
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

// AutomationLaneComponent.h

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "MouseEventThrottler.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class SongEditorView;

class AutomationLaneComponent
    : public juce::Component
    , public juce::ChangeListener
    , private juce::ValueTree::Listener
    , private juce::AsyncUpdater
{
public:
    AutomationLaneComponent(EditViewState &evs, te::AutomatableParameter::Ptr parameter, juce::String timeLineID, SongEditorView &songEditor);
    ~AutomationLaneComponent() override;

    void paint(juce::Graphics &g) override;

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;
    void handleAsyncUpdate() override;

    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;

    void mouseMove(const juce::MouseEvent &e) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseExit(const juce::MouseEvent &e) override;

    void selectPointsInLasso(juce::Rectangle<int> lassoRect, bool addToSelection);

    te::AutomatableParameter::Ptr getAutomatableParameter() const { return m_parameter; }

    void setHoveredCurve(int index)
    {
        m_hoveredCurve = index;
        m_needsRepaint = true;
    }
    void setHoveredPoint(int index)
    {
        m_hoveredPoint = index;
        m_needsRepaint = true;
    }
    void setHoveredRect(juce::Rectangle<float> rect)
    {
        m_hoveredRect = rect;
        m_needsRepaint = true;
    }
    void setIsDragging(bool dragging)
    {
        m_isDragging = dragging;
        m_needsRepaint = true;
    }

    void drawAutomationLane(juce::Graphics &g, tracktion::TimeRange drawRange, juce::Rectangle<float> drawRect);
    juce::Point<float> getPointOnAutomationRect(tracktion::TimePosition t, double v, int w, double x1b, double x2b);
    int getAutomationPointWidth();
    int getYPos(double value);
    double getValue(int y);

    tracktion_engine::AutomatableParameter::Ptr getParameter() { return m_parameter; }
    struct CachedCurvePoint
    {
        int index;
        double time;
        float value;

        juce::Point<float> getScreenPosition(AutomationLaneComponent *lane, int width, double visibleStartBeat, double visibleEndBeat) const { return lane->getPointOnAutomationRect(tracktion::TimePosition::fromSeconds(time), value, width, visibleStartBeat, visibleEndBeat); }
    };

    void updateCurveCache(const tracktion::AutomationCurve &curve);
    int findPointUnderMouse(const juce::Rectangle<float> &area, double visibleStartBeat, double visibleEndBeat, int width) const;
    bool isCurveValid(const tracktion::AutomationCurve &curve) const;
    void invalidateCurveCache();

    juce::Array<CachedCurvePoint> getVisiblePoints(double visibleStartBeat, double visibleEndBeat) const;

private:
    struct CurvePoint
    {
        CurvePoint(tracktion::TimePosition t, double v, int i, te::AutomatableParameter::Ptr p)
            : time(t),
              value(v),
              index(i),
              param(p)
        {
        }
        tracktion::TimePosition time;
        double value;
        int index;
        te::AutomatableParameter::Ptr param;
    };

    // Helpers
    float timeToX(tracktion::TimePosition time);
    tracktion::TimePosition xtoTime(int x);
    tracktion::TimePosition getSnappedTime(tracktion::TimePosition time, bool downwards = false);

    // Selection/Drag State
    juce::OwnedArray<CurvePoint> m_selPointsAtMousedown;
    juce::OwnedArray<SelectableAutomationPoint> m_selectedAutomationPoints;
    tracktion::TimePosition m_timeOfHoveredAutomationPoint;
    double m_curveSteepAtMousedown{0.0};

    void selectAutomationPoint(int index, bool add);
    void deselectAutomationPoint(int index);
    juce::OwnedArray<CurvePoint> getSelectedPoints();
    void addAutomationPointAt(tracktion::TimePosition pos);
    void removeAutomationPoint(int index);

    juce::Array<CachedCurvePoint> m_curvePointCache;

    // Mouse event throttling
    MouseEventThrottler m_mouseThrottler;
    int m_lastMouseMoveNumPoints = -1;

    int m_cachedCurvePointCount = 0;
    int m_cachedCurveVersion = 0;
    bool m_curveValid = false;
    juce::int64 m_lastPaintTime = 0;
    static constexpr int kMinPaintIntervalMs = 16; // ~60 FPS

    bool isAutomationPointSelected(int index);
    int nextIndexAfter(tracktion::TimePosition t, te::AutomatableParameter::Ptr ap) const;
    juce::Point<float> getPointOnAutomation(int index, juce::Rectangle<float> drawRect, double startBeat, double endBeat);
    juce::Point<float> getCurveControlPoint(juce::Point<float> p1, juce::Point<float> p2, float curve);

    EditViewState &m_editViewState;
    te::AutomatableParameter::Ptr m_parameter;
    juce::String m_timeLineID;

    int m_hoveredCurve = -1;
    int m_hoveredPoint = -1;
    juce::Rectangle<float> m_hoveredRect;
    bool m_isDragging = false;
    bool m_needsRepaint = false;

    SongEditorView &m_songEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLaneComponent)
};
