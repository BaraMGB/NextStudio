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

#include "MidiPendingPaste.h"

#include <cmath>

namespace MidiPendingPaste
{
    void State::begin()
    {
        m_active = true;
        m_hasMoved = false;
        m_beatDelta = 0.0;
        m_pitchDelta = 0;
    }

    bool State::nudge(double beatDelta, int pitchDelta)
    {
        if (!m_active || (std::abs(beatDelta) < 1.0e-12 && pitchDelta == 0))
            return false;

        m_beatDelta += beatDelta;
        m_pitchDelta += pitchDelta;
        m_hasMoved = true;
        return true;
    }

    Resolution State::confirm()
    {
        return finish(m_active ? Action::commit : Action::none);
    }

    Resolution State::finishOnDeselect()
    {
        return finish(!m_active ? Action::none : (m_hasMoved ? Action::commit : Action::cancel));
    }

    Resolution State::cancel()
    {
        return finish(m_active ? Action::cancel : Action::none);
    }

    Resolution State::finish(Action action)
    {
        Resolution resolution{action, m_beatDelta, m_pitchDelta};

        if (action != Action::none)
        {
            m_active = false;
            m_hasMoved = false;
            m_beatDelta = 0.0;
            m_pitchDelta = 0;
        }

        return resolution;
    }
} // namespace MidiPendingPaste
