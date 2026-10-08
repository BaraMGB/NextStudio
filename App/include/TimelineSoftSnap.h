#pragma once
#include <optional>

// Stateless magnetic positioning in physical pixels. This is NOT quantization:
// apply once to a raw request, never to a previously mapped result.
namespace TimelineSoftSnap
{
struct Interval { double lower, upper; };
constexpr double radiusPixels = 18.0;
constexpr double intervalFraction = 0.3;
struct Profile
{
    double radiusPixels = TimelineSoftSnap::radiusPixels;
    double intervalFraction = TimelineSoftSnap::intervalFraction;
};
// Both editors share an 18px radius; arrangement keeps its stronger narrow-grid cap.
constexpr Profile profileForEditor(bool pianoRoll) { return pianoRoll ? Profile{} : Profile{radiusPixels, 0.4}; }
bool isValid(Profile);
struct Mapping { double position; std::optional<double> target; };
Mapping mapDetailed(double rawPosition, Interval, Profile = {});
double map(double rawPosition, Interval, Profile = {});
double inverseAnchor(double displayedPosition, Interval, Profile = {});
}
