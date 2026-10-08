#include "TimelineSnapResolver.h"
#include <algorithm>
#include <cmath>
#include <limits>

TimelineSnapResult TimelineSnapResult::withEffectiveBeat(double effective, bool destinationValid) const
{
    auto result = *this;
    const auto tolerance = std::max(1.0e-10, 16 * std::numeric_limits<double>::epsilon() * std::abs(beat));
    if (state == State::invalid || !destinationValid || !std::isfinite(effective))
        result.state = State::invalid;
    else if (std::abs(effective - beat) > tolerance)
        result.state = State::limited;
    result.beat = effective;
    if (!result.held())
        result.targetBeat.reset();
    return result;
}

bool TimelineSnapResolver::Context::operator==(const Context& other) const
{
    return enabled == other.enabled && fixedInterval == other.fixedInterval
        && snapType.type == other.snapType.type && snapType.level == other.snapType.level
        && beatsPerPixel == other.beatsPerPixel && rasterScale == other.rasterScale
        && revision == other.revision && gridRevision == other.gridRevision
        && attraction.radiusPixels == other.attraction.radiusPixels && attraction.intervalFraction == other.attraction.intervalFraction;
}
bool TimelineSnapResolver::valid() const
{
    return std::isfinite(m_context.beatsPerPixel) && m_context.beatsPerPixel > 0
        && std::isfinite(m_context.rasterScale) && m_context.rasterScale > 0
        && std::isfinite(m_context.fixedInterval) && m_context.fixedInterval >= 0
        && TimelineSoftSnap::isValid(m_context.attraction);
}
double TimelineSnapResolver::beatToTime(double beat) const
{
    return m_tempo.toTime(tracktion::BeatPosition::fromBeats(beat)).inSeconds();
}
double TimelineSnapResolver::timeToBeat(double seconds) const
{
    return m_tempo.toBeats(tracktion::TimePosition::fromSeconds(seconds)).inBeats();
}
TimelineSoftSnap::Interval TimelineSnapResolver::adjacentTargets(double beat) const
{
    if (!std::isfinite(beat))
        return {beat, beat};
    const auto interval = m_context.fixedInterval;
    if (std::isfinite(interval) && interval > 0)
    {
        const double index = std::floor(beat / interval);
        return {index * interval, (index + 1) * interval};
    }
    const auto time = tracktion::TimePosition::fromSeconds(beatToTime(beat));
    double lower = 0, upper = 0;
    const bool musical = m_context.snapType.type == tracktion_engine::TimecodeType::barsBeats;
    const auto& signature = m_tempo.getTimeSigAt(tracktion::BeatPosition::fromBeats(beat));
    const double signatureStart = signature.startBeatNumber.get().inBeats();
    const auto signatureBars = m_tempo.toBarsAndBeats(tracktion::TimePosition::fromSeconds(beatToTime(signatureStart)));
    const bool partialBar = musical && std::abs(signatureBars.beats.inBeats()) > 1.0e-7;
    if (partialBar)
    {
        // Engine BarsAndBeats -> time extrapolation is also invalid in a bar
        // interrupted by a meter change. Reset that segment's grid at the meter
        // boundary using the engine's exact musical fractions, not a duration
        // approximation. The boundary itself is a shared target on both sides.
        static constexpr double straight[] = {1.0/960, 2.0/960, 5.0/960, 1.0/64, 1.0/32, 1.0/16, 1.0/8, 1.0/4, 1.0/2};
        static constexpr double triplet[] = {1.0/960, 2.0/960, 5.0/960, 1.0/48, 1.0/24, 1.0/12, 1.0/9, 1.0/6, 1.0/3};
        static constexpr int multiples[] = {1, 2, 4, 8, 16, 64, 128, 256, 1024, 4096, 16384, 65536};
        const int level = m_context.snapType.level;
        const double spacing = level >= 10 ? std::max(1, signature.numerator.get()) * multiples[std::clamp(level - 10, 0, 11)]
                             : level == 9 ? 1.0 : (signature.triplets.get() ? triplet : straight)[std::clamp(level, 0, 8)];
        const double index = std::floor((beat - signatureStart) / spacing);
        lower = signatureStart + index * spacing;
        upper = signatureStart + (index + 1) * spacing;
    }
    else if (musical && m_context.snapType.level >= 10)
    {
        // Tracktion's bar rounding uses the time signature at the last TEMPO
        // event. A later meter change can make roundTimeDown land in the future.
        // Resolve the same bar multiples using the actual BarsAndBeats numerator.
        static constexpr int multiples[] = {1, 2, 4, 8, 16, 64, 128, 256, 1024, 4096, 16384, 65536};
        const int multiple = multiples[std::clamp(m_context.snapType.level - 10, 0, 11)];
        const auto bars = m_tempo.toBarsAndBeats(time);
        const double index = std::floor((bars.bars + bars.beats.inBeats() / std::max(1, bars.numerator)) / multiple);
        if (!std::isfinite(index) || std::abs(index * multiple) >= std::numeric_limits<int>::max() - multiple)
            return {beat, beat};
        lower = timeToBeat(m_tempo.toTime(tracktion::tempo::BarsAndBeats{int(index * multiple), {}}).inSeconds());
        upper = timeToBeat(m_tempo.toTime(tracktion::tempo::BarsAndBeats{int((index + 1) * multiple), {}}).inSeconds());
    }
    else
    {
        lower = timeToBeat(m_context.snapType.roundTimeDown(time, m_tempo).inSeconds());
        upper = timeToBeat(m_context.snapType.roundTimeUp(time, m_tempo).inSeconds());
    }
    // Engine rounding deliberately treats tiny boundary errors as exact. At an
    // exact target ask for the strict successor, using a bounded beat-domain
    // probe (not an approximate interval at the transport tempo).
    if (upper <= lower + 1.0e-10)
    {
        double probe = 1.0e-7;
        for (int i = 0; i < 12 && upper <= lower + 1.0e-10; ++i, probe *= 10)
            upper = timeToBeat(m_context.snapType.roundTimeUp(
                tracktion::TimePosition::fromSeconds(beatToTime(lower + probe)), m_tempo).inSeconds());
    }
    if (m_context.snapType.type == tracktion_engine::TimecodeType::barsBeats)
    {
        // Treat meter/triplet changes as explicit cell boundaries, so the grid
        // cannot change halfway through a soft interval and introduce a jump.
        for (auto* signature : m_tempo.getTimeSigs())
        {
            const double boundary = signature->startBeatNumber.get().inBeats();
            if (boundary <= beat && boundary > lower)
                lower = boundary;
            else if (boundary > beat && boundary < upper)
                upper = boundary;
        }
    }
    return {lower, upper};
}
TimelineSnapResult TimelineSnapResolver::resolveForMouse(double beat, bool bypass) const
{
    TimelineSnapResult result{beat, beat, {}, TimelineSnapResult::State::invalid};
    if (!valid() || !std::isfinite(beat))
        return result;
    if (!m_context.enabled || bypass)
    {
        result.state = !m_context.enabled ? TimelineSnapResult::State::disabled : TimelineSnapResult::State::bypassed;
        return result;
    }
    const auto targets = adjacentTargets(beat);
    if (!std::isfinite(targets.lower) || !std::isfinite(targets.upper) || targets.upper <= targets.lower)
        return result;
    const double physicalPixelsPerBeat = m_context.rasterScale / m_context.beatsPerPixel;
    if (!std::isfinite(physicalPixelsPerBeat) || physicalPixelsPerBeat <= 0)
        return result;
    // Translate before scaling to retain precision on long/panned timelines.
    const auto mapping = TimelineSoftSnap::mapDetailed((beat - targets.lower) * physicalPixelsPerBeat,
        {0, (targets.upper - targets.lower) * physicalPixelsPerBeat}, m_context.attraction);
    result.beat = targets.lower + mapping.position / physicalPixelsPerBeat;
    if (mapping.target)
        result.targetBeat = *mapping.target == 0 ? targets.lower : targets.upper;
    result.state = result.targetBeat ? TimelineSnapResult::State::held : TimelineSnapResult::State::free;
    return result;
}
double TimelineSnapResolver::snapBeatForMouse(double beat) const
{
    return resolveForMouse(beat).beat;
}
double TimelineSnapResolver::rawAnchorForMouse(double beat) const
{
    if (!m_context.enabled || !valid() || !std::isfinite(beat))
        return beat;
    const auto targets = adjacentTargets(beat);
    if (!std::isfinite(targets.lower) || !std::isfinite(targets.upper) || targets.upper <= targets.lower)
        return beat;
    const double physicalPixelsPerBeat = m_context.rasterScale / m_context.beatsPerPixel;
    if (!std::isfinite(physicalPixelsPerBeat) || physicalPixelsPerBeat <= 0)
        return beat;
    return targets.lower + TimelineSoftSnap::inverseAnchor((beat - targets.lower) * physicalPixelsPerBeat,
                  {0, (targets.upper - targets.lower) * physicalPixelsPerBeat}, m_context.attraction) / physicalPixelsPerBeat;
}
double TimelineSnapResolver::startAtOrBefore(double beat) const
{
    if (!m_context.enabled || !valid() || !std::isfinite(beat))
        return beat;
    const auto targets = adjacentTargets(beat);
    return std::isfinite(targets.lower) && targets.lower <= beat ? targets.lower : beat;
}
double TimelineSnapResolver::endAtOrAfter(double requested, double minimum) const
{
    if (!std::isfinite(requested) || !std::isfinite(minimum))
        return minimum;
    const double beat = std::max(requested, minimum);
    if (!m_context.enabled)
        return beat;
    const auto targets = adjacentTargets(beat);
    const double tolerance = std::max(1.0e-10, 16 * std::numeric_limits<double>::epsilon() * std::abs(beat));
    if (targets.lower >= minimum && std::abs(beat - targets.lower) <= tolerance)
        return targets.lower;
    return std::isfinite(targets.upper) && targets.upper >= beat ? targets.upper : beat;
}
void TimelineMouseGesture::begin(double beat, double pointerX, const TimelineSnapResolver& resolver)
{
    m_active = std::isfinite(beat) && std::isfinite(pointerX) && resolver.valid();
    if (!m_active)
        return;
    m_context = resolver.context();
    m_knownGrid = m_context.enabled;
    m_originBeat = m_displayedBeat = beat;
    m_rawAnchor = resolver.rawAnchorForMouse(beat);
    m_pointerAnchor = pointerX;
    m_feedback = resolver.resolveForMouse(m_rawAnchor).withEffectiveBeat(beat);
}
double TimelineMouseGesture::update(double pointerX, const TimelineSnapResolver& resolver, bool bypass)
{
    if (!m_active || !resolver.valid() || !std::isfinite(pointerX))
        return m_displayedBeat;
    const auto& context = resolver.context();
    const bool viewChanged = m_context.beatsPerPixel != context.beatsPerPixel || m_context.rasterScale != context.rasterScale
                          || m_context.revision != context.revision || m_context.gridRevision != context.gridRevision
                          || m_context.attraction.radiusPixels != context.attraction.radiusPixels
                          || m_context.attraction.intervalFraction != context.attraction.intervalFraction;
    const bool gridChanged = m_knownGrid && context.enabled
                         && (m_context.fixedInterval != context.fixedInterval || m_context.snapType.type != context.snapType.type
                             || m_context.snapType.level != context.snapType.level);
    if (viewChanged || gridChanged)
        begin(m_displayedBeat, pointerX, resolver);
    else
    {
        // Off is a raw bypass, just like Shift: retain the complete displacement
        // from this gesture origin, even when the UI clears fixedInterval.
        if (!m_context.enabled && context.enabled)
            m_rawAnchor = resolver.rawAnchorForMouse(m_originBeat);
        if (!context.enabled && m_knownGrid)
            m_context.enabled = false; // retain the last active grid signature while bypassed
        else
        {
            m_context = context;
            m_knownGrid = context.enabled;
        }
    }
    const double displacement = (pointerX - m_pointerAnchor) * context.beatsPerPixel;
    m_feedback = resolver.resolveForMouse((bypass || !context.enabled) ? m_originBeat + displacement
                                                                     : m_rawAnchor + displacement, bypass);
    const double candidate = m_feedback.beat;
    if (std::isfinite(candidate))
        m_displayedBeat = candidate;
    return m_displayedBeat;
}
void TimelineMouseGesture::setDisplayedBeat(double beat)
{
    if (std::isfinite(beat))
    {
        m_feedback = m_feedback.withEffectiveBeat(beat);
        m_displayedBeat = beat;
    }
}
