#pragma once

#include "TimelineSoftSnap.h"
#include <tracktion_engine/tracktion_engine.h>
#include <cstdint>

namespace tracktion_engine = tracktion::engine;

struct TimelineSnapResult
{
    enum class State { disabled, bypassed, free, held, limited, invalid };
    double rawBeat = 0, beat = 0;
    std::optional<double> targetBeat;
    State state = State::invalid;
    TimelineSnapResult withEffectiveBeat(double, bool destinationValid = true) const;
    bool held() const { return state == State::held && targetBeat.has_value(); }
};

// Engine-aware grid adapter. View normalization and hard command snapping remain
// separate. All inputs/outputs are project-global beats (including MIDI offsets).
class TimelineSnapResolver
{
public:
    struct Context
    {
        bool enabled = false;
        double fixedInterval = 0; // zero selects the actual adaptive engine grid
        tracktion_engine::TimecodeSnapType snapType;
        double beatsPerPixel = 0; // logical pixels
        double rasterScale = 1;
        uint64_t revision = 0;
        uint64_t gridRevision = 0;
        TimelineSoftSnap::Profile attraction;
        bool operator==(const Context&) const;
    };

    TimelineSnapResolver(const tracktion_engine::TempoSequence& tempo, Context context)
        : m_tempo(tempo), m_context(context) {}
    const Context& context() const { return m_context; }
    bool valid() const;
    TimelineSoftSnap::Interval adjacentTargets(double globalBeat) const;
    TimelineSnapResult resolveForMouse(double rawGlobalBeat, bool bypass = false) const;
    double snapBeatForMouse(double rawGlobalBeat) const;
    double rawAnchorForMouse(double displayedGlobalBeat) const;
    // Hard initial mouse anchor, using the same corrected targets as the gesture.
    double startAtOrBefore(double requestedGlobalBeat) const;
    double endAtOrAfter(double requestedGlobalBeat, double minimumGlobalBeat) const;
    double beatToTime(double globalBeat) const;
    double timeToBeat(double seconds) const;
private:
    const tracktion_engine::TempoSequence& m_tempo;
    Context m_context;
};

// A relative mouse gesture retains RAW intent independently of its displayed
// result. Grid/view changes re-anchor; Shift alone does not accumulate offsets.
class TimelineMouseGesture
{
public:
    void begin(double displayedGlobalBeat, double pointerX, const TimelineSnapResolver&);
    double update(double pointerX, const TimelineSnapResolver&, bool bypass);
    void setDisplayedBeat(double beat); // after real feasibility constraints
    double displayedBeat() const { return m_displayedBeat; }
    const TimelineSnapResult& feedback() const { return m_feedback; }
    void reset() { m_active = false; m_knownGrid = false; m_feedback = {}; }
    bool active() const { return m_active; }
private:
    bool m_active = false, m_knownGrid = false;
    double m_originBeat = 0, m_rawAnchor = 0, m_pointerAnchor = 0, m_displayedBeat = 0;
    TimelineSnapResolver::Context m_context;
    TimelineSnapResult m_feedback;
};
