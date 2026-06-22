#include "Plugins/SoundFont/SoundFontPluginComponent.h"

#include "LowerRange/PluginChain/PresetHelpers.h"

namespace
{
constexpr auto soundFontTitle = "SOUNDFONT PLAYER";
constexpr int panelInset = 6;
constexpr int contentInset = 12;
constexpr int headerHeight = 20;
constexpr int rowHeight = 24;
constexpr int controlRowHeight = 32;
constexpr int smallGap = 8;
constexpr int mediumGap = 10;
constexpr int largeGap = 12;
constexpr int loadButtonWidth = 110;
constexpr int panicButtonWidth = 90;
constexpr int presetLabelWidth = 54;
constexpr int gainKnobWidth = 120;
} // namespace

SoundFontPluginComponent::SoundFontPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p),
      m_soundFontPlugin(dynamic_cast<SoundFontPlugin *>(p.get()))
{
    jassert(m_soundFontPlugin != nullptr);

    m_fileLabel.setJustificationType(juce::Justification::centredLeft);
    m_fileLabel.setMinimumHorizontalScale(0.8f);

    m_statusLabel.setJustificationType(juce::Justification::centredLeft);
    m_statusLabel.setMinimumHorizontalScale(0.8f);

    m_presetLabel.setText("Preset", juce::dontSendNotification);
    m_presetLabel.setJustificationType(juce::Justification::centredLeft);

    m_presetBox.addListener(this);
    m_loadButton.addListener(this);
    m_panicButton.addListener(this);

    m_gainValue.setValue(m_soundFontPlugin->getOutputGainDb());
    m_gainValue.addListener(this);
    m_gainKnob = std::make_unique<NonAutomatableParameterComponent>(m_gainValue, "Gain", -600, 120);
    m_gainKnob->setSliderRange(-60.0, 12.0, 0.1);

    Helpers::addAndMakeVisible(*this, {&m_fileLabel, &m_statusLabel, &m_presetLabel, &m_presetBox, &m_loadButton, &m_panicButton, m_gainKnob.get()});

    p->state.addListener(this);
    refreshFromPlugin();
}

SoundFontPluginComponent::~SoundFontPluginComponent()
{
    m_gainValue.removeListener(this);

    if (m_soundFontPlugin != nullptr)
        m_soundFontPlugin->state.removeListener(this);
}

void SoundFontPluginComponent::paint(juce::Graphics &g)
{
    g.fillAll(m_editViewState.m_applicationState.getBackgroundColour2());

    GUIHelpers::drawHeaderBox(g, getLocalBounds().reduced(panelInset).toFloat(), getTrackColour(), m_editViewState.m_applicationState.getBorderColour(), m_editViewState.m_applicationState.getBackgroundColour1(), (float)headerHeight, GUIHelpers::HeaderPosition::top, soundFontTitle);
}

void SoundFontPluginComponent::resized()
{
    auto area = getLocalBounds().reduced(contentInset);
    area.removeFromTop(headerHeight + largeGap);

    auto controlRow = area.removeFromTop(controlRowHeight);
    m_loadButton.setBounds(controlRow.removeFromLeft(loadButtonWidth));
    controlRow.removeFromLeft(smallGap);
    m_panicButton.setBounds(controlRow.removeFromLeft(panicButtonWidth));

    area.removeFromTop(mediumGap);

    auto fileRow = area.removeFromTop(rowHeight);
    m_fileLabel.setBounds(fileRow);

    area.removeFromTop(smallGap);

    auto presetRow = area.removeFromTop(rowHeight);
    m_presetLabel.setBounds(presetRow.removeFromLeft(presetLabelWidth));
    presetRow.removeFromLeft(smallGap);
    m_presetBox.setBounds(presetRow);

    area.removeFromTop(smallGap);

    auto statusRow = area.removeFromTop(rowHeight);
    m_statusLabel.setBounds(statusRow);

    area.removeFromTop(largeGap);
    m_gainKnob->setBounds(area.removeFromLeft(gainKnobWidth));
}

juce::ValueTree SoundFontPluginComponent::getPluginState()
{
    auto pluginState = m_soundFontPlugin->state.createCopy();
    pluginState.setProperty("type", getPluginTypeName(), nullptr);
    return pluginState;
}

juce::ValueTree SoundFontPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", SoundFontPlugin::xmlTypeName, nullptr);
    defaultState.setProperty("soundFontPath", {}, nullptr);
    defaultState.setProperty("presetIndex", 0, nullptr);
    defaultState.setProperty("gainDb", 0.0f, nullptr);
    return defaultState;
}

void SoundFontPluginComponent::restorePluginState(const juce::ValueTree &state)
{
    m_soundFontPlugin->restorePluginStateFromValueTree(state);
    refreshFromPlugin();
}

juce::String SoundFontPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_soundFontPlugin); }

juce::String SoundFontPluginComponent::getPluginTypeName() const { return SoundFontPlugin::xmlTypeName; }

ApplicationViewState &SoundFontPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

void SoundFontPluginComponent::buttonClicked(juce::Button *button)
{
    if (button == &m_loadButton)
    {
        chooseSoundFontFile();
        return;
    }

    if (button == &m_panicButton)
        m_soundFontPlugin->midiPanic();
}

void SoundFontPluginComponent::comboBoxChanged(juce::ComboBox *comboBoxThatHasChanged)
{
    if (m_isRefreshingUi || comboBoxThatHasChanged != &m_presetBox)
        return;

    const int selectedIndex = m_presetBox.getSelectedItemIndex();
    if (selectedIndex >= 0)
        m_soundFontPlugin->setCurrentPresetIndex(selectedIndex);
}

void SoundFontPluginComponent::valueChanged(juce::Value &value)
{
    if (m_isRefreshingUi)
        return;

    if (value.refersToSameSourceAs(m_gainValue))
        m_soundFontPlugin->setOutputGainDb(static_cast<float>(m_gainValue.getValue()));
}

void SoundFontPluginComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) { refreshFromPlugin(); }

void SoundFontPluginComponent::refreshFromPlugin()
{
    juce::ScopedValueSetter<bool> refreshGuard(m_isRefreshingUi, true);

    const auto filePath = m_soundFontPlugin->getSoundFontFilePath();
    const auto presetNames = m_soundFontPlugin->getPresetNames();
    const auto currentPresetIndex = m_soundFontPlugin->getCurrentPresetIndex();
    const auto lastError = m_soundFontPlugin->getLastError();

    m_fileLabel.setText(filePath.isNotEmpty() ? juce::File(filePath).getFileName() : "No SoundFont loaded", juce::dontSendNotification);
    m_fileLabel.setTooltip(filePath);

    m_presetBox.clear(juce::dontSendNotification);
    for (int i = 0; i < presetNames.size(); ++i)
        m_presetBox.addItem(presetNames[i], i + 1);

    m_presetBox.setEnabled(!presetNames.isEmpty());
    if (juce::isPositiveAndBelow(currentPresetIndex, presetNames.size()))
        m_presetBox.setSelectedItemIndex(currentPresetIndex, juce::dontSendNotification);

    m_gainValue.setValue(m_soundFontPlugin->getOutputGainDb());

    if (lastError.isNotEmpty())
        m_statusLabel.setText(lastError, juce::dontSendNotification);
    else if (presetNames.isEmpty())
        m_statusLabel.setText("Load an .sf2 file to start playing.", juce::dontSendNotification);
    else
        m_statusLabel.setText("Ready: " + m_soundFontPlugin->getCurrentPresetName(), juce::dontSendNotification);
}

void SoundFontPluginComponent::chooseSoundFontFile()
{
    auto chooser = std::make_shared<juce::FileChooser>("Select a SoundFont", juce::File(), "*.sf2");
    juce::Component::SafePointer<SoundFontPluginComponent> safeThis(this);

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [safeThis, chooser](const juce::FileChooser &fileChooser)
                         {
                             if (safeThis == nullptr)
                                 return;

                             const auto file = fileChooser.getResult();
                             if (!file.existsAsFile())
                                 return;

                             safeThis->m_soundFontPlugin->loadSoundFontFile(file.getFullPathName());
                             safeThis->refreshFromPlugin();
                         });
}
