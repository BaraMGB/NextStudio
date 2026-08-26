/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

class SplitterCollapseController
{
public:
    void beginDrag(bool collapsed, int transitionDistance)
    {
        m_startedCollapsed = collapsed;
        m_transitionDistance = transitionDistance > 0 ? transitionDistance : 0;
    }

    [[nodiscard]] bool getCollapsedState(int distanceTowardsCollapsed, bool collapsed) const
    {
        if (m_startedCollapsed)
        {
            if (collapsed && distanceTowardsCollapsed <= -m_transitionDistance)
                return false;
            if (!collapsed && distanceTowardsCollapsed >= 0)
                return true;
            return collapsed;
        }

        if (!collapsed && distanceTowardsCollapsed >= m_transitionDistance)
            return true;
        if (collapsed && distanceTowardsCollapsed <= 0)
            return false;
        return collapsed;
    }

    [[nodiscard]] bool startedCollapsed() const { return m_startedCollapsed; }

private:
    int m_transitionDistance{};
    bool m_startedCollapsed{};
};
