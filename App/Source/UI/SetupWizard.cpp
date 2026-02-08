
#include "SetupWizard.h"
#include "BinaryData.h"
#include <array>

bool SetupWizard::ensureDirectory(const juce::File &directory, juce::StringArray &errors)
{
    if (directory.existsAsFile())
    {
        errors.add("Path exists as a file: " + directory.getFullPathName());
        return false;
    }

    if (directory.exists() && directory.isDirectory())
        return true;

    if (!directory.createDirectory() && !(directory.exists() && directory.isDirectory()))
    {
        errors.add("Unable to create directory: " + directory.getFullPathName());
        return false;
    }

    return true;
}

bool SetupWizard::ensureWritable(const juce::File &directory, juce::StringArray &errors)
{
    const auto probeFile = directory.getNonexistentChildFile(".nextstudio_write_test_", ".tmp", false);
    if (!probeFile.replaceWithText("write-test"))
    {
        errors.add("No write access to directory: " + directory.getFullPathName());
        return false;
    }

    if (!probeFile.deleteFile() && probeFile.existsAsFile())
    {
        errors.add("Unable to clean up write-test file in: " + directory.getFullPathName());
        return false;
    }

    return true;
}

bool SetupWizard::ensureContentLayout(const juce::File &root, juce::StringArray &errors)
{
    if (!ensureDirectory(root, errors))
        return false;

    static constexpr std::array<const char *, 5> requiredDirs{"Presets", "Clips", "Renders", "Samples", "Projects"};
    for (const auto *name : requiredDirs)
    {
        if (!ensureDirectory(root.getChildFile(name), errors))
            return false;
    }

    if (!ensureWritable(root, errors))
        return false;

    for (const auto *name : requiredDirs)
    {
        if (!ensureWritable(root.getChildFile(name), errors))
            return false;
    }

    return true;
}

bool SetupWizard::validateAndPrepareContentRoot(const juce::File &root, juce::String &errorMessage) const
{
    juce::StringArray errors;
    if (!ensureContentLayout(root, errors))
    {
        errorMessage = errors.joinIntoString("\n");
        return false;
    }

    errorMessage.clear();
    return true;
}

void SetupWizard::showValidationError(const juce::String &message) const { juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Invalid Content Folder", message); }

SetupWizard::SetupWizard(ApplicationViewState &avs, tracktion::Engine &engine)
    : m_avs(avs),
      m_engine(engine)
{
    addAndMakeVisible(m_titleLabel);
    m_titleLabel.setText("Welcome to NextStudio", juce::dontSendNotification);
    m_titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    m_titleLabel.setJustificationType(juce::Justification::centred);

    m_logoDrawable = juce::Drawable::createFromImageData(BinaryData::logo_svg, BinaryData::logo_svgSize);

    addAndMakeVisible(m_instructionLabel);
    m_instructionLabel.setText("Please take a moment to configure your initial settings.", juce::dontSendNotification);
    m_instructionLabel.setJustificationType(juce::Justification::centred);

    // Path Group
    addAndMakeVisible(m_pathGroup);
    m_pathGroup.setText("User Content Folder");

    addAndMakeVisible(m_currentPathLabel);
    updatePathLabel();

    addAndMakeVisible(m_selectPathButton);
    m_selectPathButton.setButtonText("Change Folder...");
    m_selectPathButton.onClick = [this]
    {
        juce::FileChooser chooser("Select NextStudio User Folder...", juce::File(m_avs.m_workDir.get()), "*");

        if (chooser.browseForDirectory())
        {
            const auto selectedRoot = chooser.getResult();
            juce::String validationError;
            if (!validateAndPrepareContentRoot(selectedRoot, validationError))
            {
                showValidationError(validationError);
                return;
            }

            m_avs.setRootFolder(selectedRoot);
            updatePathLabel();
        }
    };

    // Interface Group
    addAndMakeVisible(m_interfaceGroup);
    m_interfaceGroup.setText("Interface");

    addAndMakeVisible(m_guiScaleLabel);
    m_guiScaleLabel.setText("GUI Scale:", juce::dontSendNotification);
    m_guiScaleLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(m_guiScaleSlider);
    m_guiScaleSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_guiScaleSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 22);
    m_guiScaleSlider.setTextValueSuffix("x");
    m_guiScaleSlider.setRange(0.2, 3.0, 0.01);
    m_guiScaleSlider.setNumDecimalPlacesToDisplay(2);
    m_guiScaleSlider.setSliderSnapsToMousePosition(false);
    m_guiScaleSlider.setMouseDragSensitivity(800);
    m_guiScaleSlider.setValue(juce::jlimit(0.2f, 3.0f, (float)m_avs.m_appScale.get()), juce::dontSendNotification);
    m_guiScaleSlider.onValueChange = [this]() { updateGuiScale(); };

    // Audio Group
    addAndMakeVisible(m_audioGroup);
    m_audioGroup.setText("Audio Device Settings");

    m_audioSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(m_engine.getDeviceManager().deviceManager, 0, 256, 0, 256, true, true, true, false);
    m_audioViewport = std::make_unique<juce::Viewport>();
    m_audioViewport->setViewedComponent(m_audioSelector.get(), false);
    m_audioViewport->setScrollBarsShown(true, false);
    addAndMakeVisible(*m_audioViewport);

    // Finish Button
    addAndMakeVisible(m_finishButton);
    m_finishButton.setButtonText("Start NextStudio");
    m_finishButton.setColour(juce::TextButton::buttonColourId, m_avs.getButtonBackgroundColour());
    m_finishButton.setColour(juce::TextButton::textColourOffId, m_avs.getButtonTextColour());
    m_finishButton.onClick = [this]
    {
        const auto selectedRoot = juce::File(m_avs.m_workDir.get());
        juce::String validationError;
        if (!validateAndPrepareContentRoot(selectedRoot, validationError))
        {
            showValidationError(validationError);
            return;
        }

        m_avs.setRootFolder(selectedRoot);
        m_avs.m_setupComplete = true;
        m_avs.saveState();
        m_finished = true;

        if (auto *dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState(1);
    };
}

SetupWizard::~SetupWizard() {}

void SetupWizard::updatePathLabel() { m_currentPathLabel.setText("Current: " + m_avs.m_workDir.get(), juce::dontSendNotification); }

void SetupWizard::updateGuiScale()
{
    const auto guiScale = (float)m_guiScaleSlider.getValue();
    juce::Desktop::getInstance().setGlobalScaleFactor(guiScale);
    m_avs.m_appScale = guiScale;
}

void SetupWizard::paint(juce::Graphics &g)
{
    g.fillAll(m_avs.getBackgroundColour1());

    if (m_logoDrawable != nullptr)
        m_logoDrawable->drawWithin(g, m_logoBounds.toFloat(), juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
}

void SetupWizard::resized()
{
    auto area = getLocalBounds().reduced(20);
    constexpr int sectionSpacing = 20;
    constexpr int columnGap = 20;

    // Header stays centered across the full wizard width.
    m_titleLabel.setBounds(area.removeFromTop(40));
    auto logoArea = area.removeFromTop(80);
    m_logoBounds = logoArea.withSizeKeepingCentre(220, 70);
    m_instructionLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(sectionSpacing);

    auto finishArea = area.removeFromBottom(40);
    area.removeFromBottom(10);

    auto contentArea = area;
    auto leftColumn = contentArea.removeFromLeft((contentArea.getWidth() - columnGap) / 2);
    contentArea.removeFromLeft(columnGap);
    auto rightColumn = contentArea;

    // Left column: non-audio setup sections.
    auto pathArea = leftColumn.removeFromTop(110);
    m_pathGroup.setBounds(pathArea);
    auto pathContent = pathArea.reduced(10, 20);
    pathContent.removeFromTop(10); // Group title space
    m_currentPathLabel.setBounds(pathContent.removeFromTop(30));
    m_selectPathButton.setBounds(pathContent.removeFromLeft(150).withHeight(30));

    leftColumn.removeFromTop(sectionSpacing);

    auto interfaceArea = leftColumn.removeFromTop(90);
    m_interfaceGroup.setBounds(interfaceArea);
    auto interfaceContent = interfaceArea.reduced(10, 20);
    interfaceContent.removeFromTop(10);
    m_guiScaleLabel.setBounds(interfaceContent.removeFromLeft(120));
    m_guiScaleSlider.setBounds(interfaceContent);

    // Right column: audio settings.
    m_audioGroup.setBounds(rightColumn);
    auto audioContent = rightColumn.reduced(10, 20);
    audioContent.removeFromTop(10);
    m_audioViewport->setBounds(audioContent);

    const auto contentWidth = juce::jmax(100, audioContent.getWidth() - m_audioViewport->getScrollBarThickness());
    const auto contentHeight = juce::jmax(audioContent.getHeight(), m_audioSelector->getHeight());
    m_audioSelector->setTopLeftPosition(0, 0);
    m_audioSelector->setSize(contentWidth, juce::jmax(1, contentHeight));

    m_finishButton.setBounds(finishArea.withSizeKeepingCentre(200, 40));
}
