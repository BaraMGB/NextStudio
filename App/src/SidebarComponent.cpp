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

#include "SidebarComponent.h"
#include "BinaryData.h"
#include "MainComponent.h"
#include "RenderDialog.h"
#include "EditViewState.h"
#include "Utilities.h"

SidebarComponent::SidebarComponent(EditViewState &evs, juce::ApplicationCommandManager &commandManager)
    : m_evs(evs),
      m_appState(evs.m_applicationState),
      m_engine(evs.m_edit.engine),
      m_edit(evs.m_edit),
      m_commandManager(commandManager),
      m_menu(m_appState),
      m_settingsView(m_engine, m_commandManager, m_appState),
      m_instrumentList(m_engine, true, m_appState),
      m_effectList(m_engine, false, m_appState),
      m_samplePreview(m_engine, m_edit, m_appState),
      m_sampleBrowser(m_appState, m_samplePreview),
      m_fileListBrowser(m_appState, m_engine, m_samplePreview),
      m_projectsBrowser(m_evs, m_appState)
{
    addAndMakeVisible(m_menu);
    addChildComponent(m_settingsView);
    addChildComponent(m_instrumentList);
    addChildComponent(m_effectList);
    addChildComponent(m_sampleBrowser);
    addChildComponent(m_samplePreview);
    addChildComponent(m_fileListBrowser);
    addChildComponent(m_projectsBrowser);
    for (auto b : m_menu.getButtons())
        b->addListener(this);

    m_settingsView.setIndent(10);
    m_settingsView.setOnContentPathChanged(
        [this]
        {
            if (auto parent = dynamic_cast<MainComponent *>(getParentComponent()))
                parent->handleContentPathChangedFromSettings();
        });
    refreshBrowsersFromAppState();

    setAllVisibleOff();
    m_projectsBrowser.setVisible(true);
}

SidebarComponent::~SidebarComponent()
{
    if (auto parent = dynamic_cast<MainComponent *>(getParentComponent()))
    {
        parent->setProjectSaveAsInteractionBlocked(false);
        m_fileListBrowser.removeChangeListener(parent);
        m_projectsBrowser.removeChangeListener(parent);
    }
    for (auto b : m_menu.getButtons())
        b->removeListener(this);
}

void SidebarComponent::paint(juce::Graphics &g)
{
    auto sideMenu = m_menu.getBounds();

    g.setColour(m_appState.getBackgroundColour1());
    g.fillRect(sideMenu);
    g.setColour(m_appState.getBorderColour());
    g.drawVerticalLine(sideMenu.getRight() - 1, 0, getHeight());

    if (m_appState.m_sidebarCollapsed)
        return;

    auto headerRect = getLocalBounds().removeFromTop(CONTENT_HEADER_HEIGHT).withLeft(sideMenu.getWidth());
    auto footerRect = getLocalBounds().removeFromBottom(CONTENT_HEADER_HEIGHT).withLeft(sideMenu.getWidth());
    auto colourBulbH = headerRect.removeFromRight(10);
    auto colourBulbF = footerRect.removeFromRight(10);

    g.setColour(m_appState.getBackgroundColour1());
    g.fillRect(headerRect);
    g.fillRect(footerRect);
    g.setColour(m_headerColour);
    g.fillRect(colourBulbH);
    g.fillRect(colourBulbF);

    g.setColour(m_appState.getBorderColour());
    g.drawHorizontalLine(CONTENT_HEADER_HEIGHT - 1, sideMenu.getRight(), getWidth());
    g.drawHorizontalLine(getHeight() - CONTENT_HEADER_HEIGHT, sideMenu.getRight(), getWidth());

    g.setColour(m_appState.getTextColour());
    headerRect.reduce(10, 0);
    g.drawText(m_headerName, headerRect, juce::Justification::centredLeft, false);

    auto iconRect = headerRect.removeFromRight(CONTENT_HEADER_HEIGHT);
    iconRect.reduce(2, 2);

    if (m_instrumentList.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::presetsButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_projectsBrowser.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::projectsButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_sampleBrowser.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::samplesButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_effectList.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::pluginsButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_fileListBrowser.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::homeButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_settingsView.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::settingsButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_renderComponent != nullptr)
        GUIHelpers::drawFromSvg(g, BinaryData::renderButton_svg, m_headerColour, iconRect.toFloat());
}
void SidebarComponent::paintOverChildren(juce::Graphics &g)
{
    GUIHelpers::drawFakeRoundCorners(g, getLocalBounds().toFloat(), m_appState.getMainFrameColour(), m_appState.getBorderColour());

    if (!m_projectsBrowser.isSaveAsWorkflowActive())
        return;

    g.setColour(juce::Colours::black.withAlpha(0.58f));
    g.fillRect(m_menu.getBounds());

    auto shell = getLocalBounds().withLeft(m_menu.getRight());
    g.fillRect(shell.removeFromTop(CONTENT_HEADER_HEIGHT));
    g.fillRect(shell.removeFromBottom(CONTENT_HEADER_HEIGHT));
}

void SidebarComponent::resized()
{
    auto area = getLocalBounds();

    m_menu.setBounds(area.removeFromLeft(SidebarLayout::collapsedWidth));

    if (m_appState.m_sidebarCollapsed == false)
    {
        area.removeFromTop(CONTENT_HEADER_HEIGHT);
        area.removeFromBottom(CONTENT_HEADER_HEIGHT);

        if (m_settingsView.isVisible())
        {
            m_settingsView.setBounds(area);
            m_headerName = "Settings";
            m_headerColour = m_appState.getSettingsColour();
        }
        else if (m_instrumentList.isVisible())
        {
            m_instrumentList.setBounds(area);
            m_headerName = "Instrument Plugins";
            m_headerColour = m_appState.getInstrumentsColour();
        }
        else if (m_effectList.isVisible())
        {
            m_effectList.setBounds(area);
            m_headerName = "Effect Plugins";
            m_headerColour = m_appState.getEffectsColour();
        }
        else if (m_fileListBrowser.isVisible())
        {
            auto bounds = area;
            if (m_samplePreview.isVisible())
            {
                auto preview = area.removeFromBottom(180);
                m_samplePreview.setBounds(preview);
            }
            m_fileListBrowser.setBounds(area);
            m_headerName = "Home Folder";
            m_headerColour = m_appState.getHomeColour();
        }
        else if (m_sampleBrowser.isVisible())
        {
            auto bounds = area;
            if (m_samplePreview.isVisible())
            {
                auto preview = area.removeFromBottom(180);
                m_samplePreview.setBounds(preview);
            }
            m_sampleBrowser.setBounds(area);
            m_headerName = "Samples";
            m_headerColour = m_appState.getSamplesColour();
        }
        else if (m_projectsBrowser.isVisible())
        {
            auto bounds = area;
            m_projectsBrowser.setBounds(bounds);
            m_headerColour = m_appState.getProjectsColour();
            m_headerName = "Projects";
        }
        else if (m_renderComponent != nullptr)
        {
            addAndMakeVisible(*m_renderComponent);
            auto bounds = area;
            m_renderComponent->setBounds(bounds);
            m_headerColour = m_appState.getRenderColour();
            m_headerName = "Render";
        }
    }
    repaint();
}
void SidebarComponent::mouseDown(const juce::MouseEvent &)
{
    if (m_projectsBrowser.isSaveAsWorkflowActive())
        dismissProjectSaveAs();
}

void SidebarComponent::buttonClicked(juce::Button *button)
{
    if (m_projectsBrowser.isSaveAsWorkflowActive())
    {
        dismissProjectSaveAs();
        return;
    }

    const auto buttonName = button->getName();
    const bool shouldCollapse = !m_appState.m_sidebarCollapsed && buttonName == m_activeButtonName;

    if (auto *drawableButton = dynamic_cast<juce::DrawableButton *>(button))
        drawableButton->getNormalImage()->replaceColour(juce::Colour(0xffffff), juce::Colours::greenyellow);

    if (shouldCollapse)
    {
        m_appState.m_sidebarCollapsed = true;
    }
    else
    {
        setAllVisibleOff();
        m_activeButtonName = buttonName;
        m_appState.m_sidebarWidth = SidebarLayout::getPreferredWidth((int)m_appState.m_sidebarWidth);
        m_appState.m_sidebarCollapsed = false;
        showViewForButton(buttonName);
    }

    if (auto *parent = dynamic_cast<MainComponent *>(getParentComponent()))
        parent->resized();

    resized();
}

void SidebarComponent::showViewForButton(const juce::String &buttonName)
{
    if (buttonName == "Settings")
    {
        m_settingsView.setVisible(true);
    }
    else if (buttonName == "Instruments")
    {
        m_instrumentList.setVisible(true);
    }
    else if (buttonName == "Samples")
    {
        m_sampleBrowser.setVisible(true);
        m_samplePreview.setVisible(true);
    }
    else if (buttonName == "Projects")
    {
        m_projectsBrowser.setVisible(true);
    }
    else if (buttonName == "Effects")
    {
        m_effectList.setVisible(true);
    }
    else if (buttonName == "Render")
    {
        m_renderComponent = std::make_unique<RenderDialog>(m_evs);
    }
    else if (buttonName == "Home")
    {
        m_fileListBrowser.setVisible(true);
        m_samplePreview.setVisible(true);
    }
}

void SidebarComponent::updateParentsListener()
{
    if (auto parent = dynamic_cast<MainComponent *>(getParentComponent()))
    {
        m_fileListBrowser.addChangeListener(parent);
        m_projectsBrowser.addChangeListener(parent);
    }
}

void SidebarComponent::refreshBrowsersFromAppState()
{
    const auto samplesRoot = juce::File(m_appState.m_samplesDir.get());
    const auto projectsRoot = juce::File(m_appState.m_projectsDir.get());
    const auto workRoot = juce::File(m_appState.m_workDir.get());

    m_sampleBrowser.setFileList(samplesRoot.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.wav;*.WAV;*.mp3;*.MP3;*.aiff;*.AIFF;*.flac;*.FLAC"));
    m_projectsBrowser.setFileList(projectsRoot.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.tracktionedit"));
    m_fileListBrowser.setDirecory(workRoot);
}

void SidebarComponent::refreshThemeFromAppState()
{
    m_settingsView.refreshThemeFromAppState();
    repaint();
}

void SidebarComponent::beginProjectSaveAs()
{
    if (!m_projectsBrowser.isVisible())
    {
        setAllVisibleOff();
        m_activeButtonName = "Projects";
        m_projectsBrowser.setVisible(true);
        m_appState.m_sidebarCollapsed = false;
    }
    m_projectsBrowser.beginSaveProjectAs();
    resized();
}

void SidebarComponent::dismissProjectSaveAs()
{
    m_projectsBrowser.dismissSaveProjectAs();
}

void SidebarComponent::projectWasSaved(const juce::File &file)
{
    m_projectsBrowser.projectWasSaved(file);
}

void SidebarComponent::showProjectError(const juce::String &message, const juce::File &file)
{
    if (!m_projectsBrowser.isVisible())
    {
        setAllVisibleOff();
        m_activeButtonName = "Projects";
        m_projectsBrowser.setVisible(true);
        m_appState.m_sidebarCollapsed = false;
    }
    m_projectsBrowser.showOperationError(message, file);
    resized();
}

void SidebarComponent::setAllVisibleOff()
{
    m_settingsView.setVisible(false);
    m_instrumentList.setVisible(false);
    m_effectList.setVisible(false);
    m_fileListBrowser.setVisible(false);
    m_projectsBrowser.setVisible(false);
    m_sampleBrowser.setVisible(false);
    m_samplePreview.setVisible(false);
    if (m_renderComponent != nullptr)
    {
        m_renderComponent.reset();
    }
}
