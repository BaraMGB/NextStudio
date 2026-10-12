#pragma once

// Transient presentation state for the Position Display's save confirmation.
// The owner supplies monotonic time and keeps model/file state authoritative.
class PositionDisplayInfoState
{
public:
    static constexpr double savedDurationMs = 3000.0;

    void clear()
    {
        m_pending = false;
        m_savedVisible = false;
        m_deadlineMs = 0.0;
    }

    void saved(double nowMs)
    {
        clear();
        m_pending = true;
        if (!m_blocked)
            activate(nowMs);
    }

    void setBlocked(bool blocked, double nowMs)
    {
        if (blocked == m_blocked)
            return;

        m_blocked = blocked;
        if (blocked)
            clear();
        else if (m_pending)
            activate(nowMs);
    }

    bool expire(double nowMs)
    {
        if (!m_savedVisible || nowMs < m_deadlineMs)
            return false;

        clear();
        return true;
    }

    bool isSavedVisible() const { return m_savedVisible; }
    bool isPending() const { return m_pending; }

private:
    void activate(double nowMs)
    {
        m_pending = false;
        m_savedVisible = true;
        m_deadlineMs = nowMs + savedDurationMs;
    }

    double m_deadlineMs = 0.0;
    bool m_blocked = false;
    bool m_pending = false;
    bool m_savedVisible = false;
};
