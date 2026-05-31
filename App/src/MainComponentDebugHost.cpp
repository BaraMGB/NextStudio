#include "MainComponentDebugHost.h"

#include "DebugSessionEnvironment.h"
#include "MainComponent.h"
#include "Logging.h"

namespace te = tracktion_engine;

namespace NextStudio::Debug
{
MainComponentDebugHost::MainComponentDebugHost(MainComponent &mainComponent)
    : m_mainComponent(mainComponent)
{
}

bool MainComponentDebugHost::isDebugMode() const { return m_mainComponent.m_debugMode; }

const ApplicationViewState &MainComponentDebugHost::getApplicationState() const { return m_mainComponent.m_applicationState; }

te::Edit *MainComponentDebugHost::getCurrentEdit() const { return m_mainComponent.m_edit.get(); }

EditViewState *MainComponentDebugHost::getEditViewState() const { return m_mainComponent.m_editViewState.get(); }

EditComponent *MainComponentDebugHost::getEditComponent() const { return m_mainComponent.m_editComponent.get(); }

bool MainComponentDebugHost::hasHeaderComponent() const { return m_mainComponent.m_header != nullptr; }

bool MainComponentDebugHost::hasLowerRangeComponent() const { return m_mainComponent.m_lowerRange != nullptr; }

juce::Rectangle<int> MainComponentDebugHost::getScreenBounds() const { return m_mainComponent.getScreenBounds(); }

juce::Rectangle<int> MainComponentDebugHost::getLocalBounds() const { return m_mainComponent.getLocalBounds(); }

juce::Image MainComponentDebugHost::createSnapshot(const juce::Rectangle<int> &bounds, float scale) const
{
    return const_cast<MainComponent &>(m_mainComponent).createComponentSnapshot(bounds, false, scale);
}

juce::File MainComponentDebugHost::getDebugArtifactsDirectory() const
{
    return SessionEnvironment::getDebugArtifactsDirectory(m_mainComponent.m_engine.getTemporaryFileManager().getTempDirectory());
}

bool MainComponentDebugHost::selectTrackByName(const juce::String &trackName)
{
    if (m_mainComponent.m_editViewState == nullptr)
        return false;

    for (auto *track : te::getAllTracks(*m_mainComponent.m_edit))
    {
        if (track != nullptr && track->getName().equalsIgnoreCase(trackName.trim()))
        {
            m_mainComponent.m_selectionManager.selectOnly(track);
            NS_LOG_INFO(selection, "agent selected track: " + track->getName());
            return true;
        }
    }

    NS_LOG_WARN(selection, "agent could not find track: " + trackName);
    return false;
}

void MainComponentDebugHost::switchLowerRangeView(LowerRangeView view)
{
    if (m_mainComponent.m_editViewState == nullptr)
        return;

    m_mainComponent.m_editViewState->setLowerRangeView(view);
    m_mainComponent.resized();
    m_mainComponent.repaint();
    NS_LOG_INFO(viewstate, "agent switched lower range view to " + NextStudio::Logging::toLogString(static_cast<int>(view)));
}

void MainComponentDebugHost::requestQuit()
{
    if (auto *app = juce::JUCEApplication::getInstance())
        app->quit();
}
} // namespace NextStudio::Debug
