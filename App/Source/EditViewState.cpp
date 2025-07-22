#include "EditViewState.h"

float EditViewState::beatsToX(double beats, int width, double x1beats, double x2beats) const
{
    return static_cast<float>(((beats - x1beats) * width) / (x2beats - x1beats));
}

double EditViewState::xToBeats(float x, int width, double x1beats, double x2beats) const
{
    double beats = (static_cast<double>(x) / width) * (x2beats - x1beats) + x1beats;
    return beats;
}

float EditViewState::timeToX(double time, int width, double x1beats, double x2beats) const
{
    double beats = timeToBeat(time);
    return static_cast<float>(((beats - x1beats) * width) / (x2beats - x1beats));
}

double EditViewState::xToTime(float x, int width, double x1beats, double x2beats) const
{
    double beats = (static_cast<double>(x) / width) * (x2beats - x1beats) + x1beats;
    return beatToTime(beats);
}

float EditViewState::beatsToX(double beats, const juce::String& timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return beatsToX(beats, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

double EditViewState::xToBeats(int x, const juce::String& timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return xToBeats(x, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

float EditViewState::timeToX(double time, const juce::String& timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return timeToX(time, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

double EditViewState::xToTime(int x, const juce::String& timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return xToTime(x, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

double EditViewState::beatToTime (double b) const
{
    auto bp = tracktion::core::BeatPosition::fromBeats(b);
    auto& ts = m_edit.tempoSequence;
    return ts.toTime(bp).inSeconds();
}

double EditViewState::timeToBeat (double t) const
{
    auto tp = tracktion::core::TimePosition::fromSeconds(t);
    auto& ts = m_edit.tempoSequence;
    return ts.toBeats(tp).inBeats();
}

void EditViewState::setNewStartAndZoom(juce::String timeLineID, double startBeat, double beatsPerPixel)
{
    startBeat = juce::jmax(0.0, startBeat);

    if (auto* myViewData = componentViewData[timeLineID])
    {
        if (beatsPerPixel != -1)
            myViewData->beatsPerPixel = beatsPerPixel;
        myViewData->viewX = startBeat;
    }
}

void EditViewState::setNewBeatRange(juce::String timeLineID, tracktion::BeatRange beatRange, float width)
{
    if (auto* myViewData = componentViewData[timeLineID])
    {
        auto startBeat = beatRange.getStart().inBeats();
        auto endBeat = beatRange.getEnd().inBeats();
        auto beatsPerPixel = (endBeat - startBeat) / width;

        if (startBeat < 0)
        {
            startBeat = 0;
            endBeat = startBeat + (beatsPerPixel * width);
        }

        myViewData->viewX = startBeat;
        myViewData->beatsPerPixel = beatsPerPixel;
    }
}

void EditViewState::setNewTimeRange(juce::String timeLineID, tracktion::TimeRange timeRange, float width)
{
    if (auto* myViewData = componentViewData[timeLineID])
    {
        auto startBeat = timeToBeat(timeRange.getStart().inSeconds());
        auto endBeat = timeToBeat(timeRange.getEnd().inSeconds());
        auto beatsPerPixel = (endBeat - startBeat) / width;

        if (startBeat < 0)
        {
            startBeat = 0;
            endBeat = startBeat + (beatsPerPixel * width);
        }

        myViewData->viewX = startBeat;
        myViewData->beatsPerPixel = beatsPerPixel;
    }
}

tracktion::BeatRange EditViewState::getVisibleBeatRange(juce::String id, int width)
{
    if (auto* myViewData = componentViewData[id])
    {
        auto startBeat = myViewData->viewX.get();
        auto endBeat = startBeat + (myViewData->beatsPerPixel * width);

        return {tracktion::BeatPosition::fromBeats(startBeat)
                , tracktion::BeatPosition::fromBeats(endBeat)};
    }
    return tracktion::BeatRange();
}

tracktion::TimeRange EditViewState::getVisibleTimeRange(juce::String id, int width)
{
    if (auto* myViewData = componentViewData[id])
    {
        auto startBeat = myViewData->viewX.get();
        auto endBeat = startBeat + (myViewData->beatsPerPixel * width);

        auto t1 = beatToTime(startBeat);
        auto t2 = beatToTime(endBeat);

        return {tracktion::TimePosition::fromSeconds(t1)
                , tracktion::TimePosition::fromSeconds(t2)};
    }
    return tracktion::TimeRange();
}