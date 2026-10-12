/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

enum class PianoRollMouseDownAction
{
    forwardToTool,
    ignore,
    deleteHitNote
};

constexpr PianoRollMouseDownAction resolvePianoRollMouseDownAction(bool isPopupMenu,
                                                                   bool noteHit,
                                                                   bool secondaryDeleteTool)
{
    if (!isPopupMenu || !secondaryDeleteTool)
        return PianoRollMouseDownAction::forwardToTool;

    return noteHit ? PianoRollMouseDownAction::deleteHitNote
                   : PianoRollMouseDownAction::ignore;
}
