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

namespace MidiPendingPaste
{
    enum class Action
    {
        none,
        cancel,
        commit
    };

    struct Resolution
    {
        Action action{Action::none};
        double beatDelta{};
        int pitchDelta{};
    };

    /** Pure state machine for a provisional in-place MIDI paste. */
    class State
    {
    public:
        void begin();
        bool nudge(double beatDelta, int pitchDelta);

        Resolution confirm();
        Resolution finishOnDeselect();
        Resolution cancel();

        bool isActive() const { return m_active; }
        bool hasMoved() const { return m_hasMoved; }
        double getBeatDelta() const { return m_beatDelta; }
        int getPitchDelta() const { return m_pitchDelta; }

    private:
        Resolution finish(Action);

        bool m_active{false};
        bool m_hasMoved{false};
        double m_beatDelta{};
        int m_pitchDelta{};
    };
} // namespace MidiPendingPaste
