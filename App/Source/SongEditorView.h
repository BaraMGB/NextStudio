
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "LassoSelectionTool.h"
#include "LowerRangeComponent.h"
#include "MenuBar.h"
#include "RecordingClipComponent.h"
#include "Utilities.h"

enum class Tool {pointer, draw, range, eraser, knife, lasso, timestretch};

struct SelectableAutomationPoint  : public te::Selectable
{
    SelectableAutomationPoint (int i, te::AutomationCurve& c)  : index (i), m_curve (c) {}
    ~SelectableAutomationPoint() override {notifyListenersOfDeletion();}

    juce::String getSelectableDescription() override {return juce::String("AutomationPoint");}

    int index = 0;
    te::AutomationCurve&        m_curve;
};

class SongEditorView : public juce::Component
                     , public juce::ChangeListener
                     , public juce::DragAndDropTarget
{
public:
    SongEditorView(EditViewState& evs, LowerRangeComponent& lr, MenuBar& toolBar);
    ~SongEditorView() override;

  	void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent &) override;
    void mouseDown (const juce::MouseEvent &) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;


    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDragEnter (const SourceDetails& dragSourceDetails) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    bool shouldDrawDragImageWhenOver() override {return false;};

    void startLasso(const juce::MouseEvent& e, bool fromAutomation, bool selectRange);
    void updateLasso(const juce::MouseEvent& e);
    void stopLasso();


    int getYForTrack (te::Track* track);
    int getTrackHeight(tracktion_engine::Track* track, EditViewState& evs, bool withAutomation=true);

    void updateTrackHeights(EditViewState& evs);

    void setTool (Tool tool) { m_toolMode = tool; }

    juce::Array<te::Track::Ptr> getShowedTracks ();

    void duplicateSelectedClipsOrTimeRange();
    void deleteSelectedTimeRange();
    void renderSelectedTimeRangeToNewTrack();
    void transposeSelectedClips(float pitchChange);
    void reverseSelectedClips();
    juce::Array<te::Track*> getTracksWithSelectedTimeRange();
private:

    tracktion::TimeRange getSelectedTimeRange();

    struct CurvePoint
    {
        CurvePoint(tracktion::TimePosition t, double v, int i, te::AutomatableParameter& p)
                : time(t), value(v), index(i), param(p) {}
        ~CurvePoint() {}
        tracktion::TimePosition   time;
        double                    value;
        int                       index;
        te::AutomatableParameter& param;
    };

    //converting
    int timeToX (double time);
    double xtoTime(int x);
    double xToSnapedBeat (int x);
    double getSnapedTime(double time, bool downwards=false);

    void updateCursor(juce::ModifierKeys);


    te::Track::Ptr getTrackAt(int y);

    int getVerticalOffset(te::Track::Ptr sourceTrack, const juce::Point<int>& dropPos);
            
    //AutomatableParameter 
    te::AutomatableParameter::Ptr getAutomatableParamAt(int y);
    juce::Rectangle<int> getAutomationRect (te::AutomatableParameter::Ptr ap);
    int getHeightOfAutomation (te::AutomatableParameter::Ptr ap);
    int getYForAutomatableParam(te::AutomatableParameter::Ptr ap);

    //AutomationPoint info
    int nextIndexAfter (tracktion::TimePosition t,te::AutomatableParameter::Ptr ap) const;
    juce::Point<float> getPointOnAutomation(te::AutomatableParameter::Ptr ap, int index, juce::Rectangle<int> drawRect, double startBeat, double endBeat);
    juce::Point<float> getPointOnAutomationRect (tracktion::TimePosition t, double v, te::AutomatableParameter::Ptr ap, int w, double x1b, double x2b); 
    juce::Point<float> getCurveControlPoint(juce::Point<float> p1, juce::Point<float> p2, float curve);
    int getAutomationPointWidth (te::AutomatableParameter::Ptr ap);
    int getYPos (double value, te::AutomatableParameter::Ptr ap);
    double getValue (int y, te::AutomatableParameter::Ptr ap);

    //AutomationPoint handle
    void addAutomationPointAt(te::AutomatableParameter::Ptr par, tracktion::TimePosition pos);
    void selectAutomationPoint(te::AutomatableParameter::Ptr ap,int index, bool add);
    SelectableAutomationPoint* createSelectablePoint(te::AutomatableParameter::Ptr ap, int index);
    bool isAutomationPointSelected(te::AutomatableParameter::Ptr ap, int index);
    void deselectAutomationPoint(te::AutomatableParameter::Ptr ap, int index);
    juce::Array<CurvePoint*> getSelectedPoints();

    //LassoSelectionTool
    void updateClipSelection(bool add);
    void updateClipCache();
    void updateAutomationSelection(bool add);
    void updateAutomationCache();
    void updateRangeSelection(); 
    void clearSelectedTimeRange();
    void setSelectedTimeRange(tracktion::TimeRange tr, bool snapDownAtStart, bool snapDownAtEnd);
    void selectCatchedClips(const tracktion_engine::Track* track);

    void moveSelectedTimeRanges(tracktion::TimeDuration td, bool copy);
    void moveSelectedRangeOfTrack(te::Track::Ptr,tracktion::TimeDuration td, bool copy);

    void moveSelectedClips(bool copy, double delta, int verticalOffset);  
    void constrainClipInRange(te::Clip* c, tracktion::TimeRange r);
    void addWaveFileToTrack(te::AudioFile audioFile, double dropTime, te::AudioTrack::Ptr track) const;
    void resizeSelectedClips(bool snap, bool fromLeftEdge=false);
    tracktion_engine::MidiClip::Ptr createNewMidiClip(double beatPos, te::Track::Ptr track);
    void splitClipAt(int x, int y);

    void setNewTempoOfClipByNewLength(te::WaveAudioClip::Ptr wac, double newLegth);

    std::unique_ptr<te::SmartThumbnail>& getOrCreateThumbnail (te::WaveAudioClip::Ptr wac);

    void removeThumbnail(te::WaveAudioClip::Ptr wac);
    struct ClipThumbNail 
    {
        ClipThumbNail (te::WaveAudioClip::Ptr wac, std::unique_ptr<te::SmartThumbnail> sn) : waveAudioClip (wac), smartThumbnail (std::move(sn)) {}

        te::WaveAudioClip::Ptr waveAudioClip;
        std::unique_ptr<te::SmartThumbnail> smartThumbnail;
    };
    
    juce::Rectangle<int> getClipRect (te::Clip::Ptr clip);
    juce::Range<int> getVerticalRangeOfTrack(tracktion_engine::Track::Ptr track, bool withAutomation) ;

    void drawTrack(juce::Graphics& g, juce::Rectangle<int> rect, te::ClipTrack::Ptr clipTrack, tracktion::TimeRange etr, bool forDragging=false);

    void drawClipBody(juce::Graphics& g, juce::String name, juce::Rectangle<int> clipRect,bool isSelected, juce::Colour color, juce::Rectangle<int> displayedRect, double x1Beat, double x2beat);
    void drawClip(juce::Graphics& g, juce::Rectangle<int> rect, te::Clip * clip, juce::Colour color, juce::Rectangle<int> displayedRect, double x1Beat, double x2beat);
    void drawAudioClip(juce::Graphics& g, juce::Rectangle<int> rect, te::WaveAudioClip::Ptr audioClip, juce::Colour color, double x1Beat, double x2beat);
    void drawWaveform(juce::Graphics& g, te::AudioClipBase& c, te::SmartThumbnail& thumb, juce::Colour colour, juce::Rectangle<int>, juce::Rectangle<int> displayedRect, double x1Beat, double x2beat);
    void drawChannels(juce::Graphics& g, te::SmartThumbnail& thumb, juce::Rectangle<int> area, bool useHighRes, tracktion::core::TimeRange time, bool useLeft, bool useRight, float leftGain, float rightGain);
    void drawMidiClip (juce::Graphics& g,te::MidiClip::Ptr clip, juce::Rectangle<int> clipRect, juce::Rectangle<int> displayedRect, juce::Colour color, double x1Beat, double x2beat);
    
    void drawAutomationLane (juce::Graphics& g, tracktion::TimeRange drawRange, juce::Rectangle<int> drawRect, te::AutomatableParameter::Ptr ap, bool forDragging=false);
        
    void buildRecordingClips(te::Track::Ptr track);


    struct TrackHeightInfo
    {
        tracktion_engine::Track* m_track;
        int m_height;
        int m_automationHeight;
        bool m_isMinimized;
    };

    juce::Array<TrackHeightInfo> m_trackInfos;

    struct DragFileItemInfo
    {
        bool visible{false};
        juce::String name;
        juce::Rectangle<int> drawRect;
        juce::Colour colour;
        bool valid{false};
    };

    juce::OwnedArray<RecordingClipComponent>  m_recordingClips;
    //essentials
    EditViewState&                      m_editViewState;
    LowerRangeComponent&                m_lowerRange;
    MenuBar&                            m_toolBar;
    LassoSelectionTool                  m_lassoComponent;
    juce::OwnedArray<ClipThumbNail>     m_thumbnails;

    //flags
    bool                                m_isDragging {false};
    bool                                m_lassoStartsInAutomation{false};
    bool                                m_isSelectingTimeRange{false};
    bool                                m_isDraggingSelectedTimeRange{false};
    bool                                m_leftBorderHovered{false};
    bool                                m_rightBorderHovered{false};
    bool                                m_hoveredTimeRange{false};
    bool                                m_hoveredTimeRangeLeft{false};
    bool                                m_hoveredTimeRangeRight{false};

    Tool                                m_toolMode {Tool::pointer};

    //caches
    juce::Array<te::Clip*>              m_cachedSelectedClips;
    juce::Array<SelectableAutomationPoint*>       m_cachedSelectedAutomation;

    GUIHelpers::SelectedTimeRange                   m_selectedRange;
    juce::Image                         m_timeRangeImage;
    juce::Rectangle<int>                m_hoveredRectOnAutomation;

    juce::Array<CurvePoint*>
                                        m_selPointsAtMousedown;
    double                              m_curveSteepAtMousedown{0.f};
    double                              m_curveAtMousedown{0.f};
    double                              m_clipPosAtMouseDown;

    te::Clip::Ptr                       m_draggedClip;
    double                              m_draggedTimeDelta{0.0};
    int                                 m_draggedVerticalOffset{0};

    DragFileItemInfo                    m_dragItemRect;

    te::Track::Ptr                      m_hoveredTrack {nullptr};
    te::AutomatableParameter::Ptr       m_hoveredAutamatableParam{nullptr};
    int                                 m_hoveredAutomationPoint {-1};
    tracktion::TimePosition             m_timeOfHoveredAutomationPoint;
    tracktion::TimePosition             m_timeAtMouseCursor;
    te::Clip::Ptr                       m_hoveredClip {nullptr};
    int                                 m_hoveredCurve {-1};

    void logMousePositionInfo();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongEditorView)
};

