#include "MainComponentDebugHost.h"

#include "AgentDebug.h"
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

bool MainComponentDebugHost::hasEditComponent() const { return m_mainComponent.m_editComponent != nullptr; }

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

juce::File MainComponentDebugHost::writeStateDump() const
{
    return NextStudio::AgentDebug::writeStateDump(*this);
}

juce::File MainComponentDebugHost::captureSnapshot(int maxWidth) const
{
    return NextStudio::AgentDebug::captureSnapshot(*this, {}, maxWidth);
}

bool MainComponentDebugHost::play()
{
    if (m_mainComponent.m_editComponent == nullptr)
        return false;
    EngineHelpers::play(m_mainComponent.m_editComponent->getEditViewState());
    return true;
}

bool MainComponentDebugHost::stop()
{
    if (m_mainComponent.m_editComponent == nullptr)
        return false;
    EngineHelpers::stopPlay(m_mainComponent.m_editComponent->getEditViewState());
    return true;
}

bool MainComponentDebugHost::showProjectSaveAs()
{
    if (m_mainComponent.m_sideBarBrowser == nullptr)
        return false;
    m_mainComponent.m_sideBarBrowser->beginProjectSaveAs();
    m_mainComponent.resized();
    return true;
}

te::AudioTrack *MainComponentDebugHost::createAudioTrack(bool midi, const juce::String &name)
{
    if (m_mainComponent.m_editViewState == nullptr)
        return nullptr;
    auto track = EngineHelpers::addAudioTrack(midi, juce::Colour(0xff4f81bd), *m_mainComponent.m_editViewState);
    if (track != nullptr)
        track->setName(name);
    return track;
}

void MainComponentDebugHost::requestQuit()
{
    if (auto *app = juce::JUCEApplication::getInstance())
        app->quit();
}
} // namespace NextStudio::Debug
