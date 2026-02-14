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

#include "SideBrowser/ProjectsBrowser.h"
#include "MainComponent.h"
#include "SideBrowser/Browser_Base.h"
#include "SideBrowser/SearchFieldComponent.h"
#include "SongEditor/EditComponent.h"
#include "Utilities/Utilities.h"

const juce::DrawableButton::ButtonStyle buttonStyle{juce::DrawableButton::ButtonStyle::ImageAboveTextLabel};

ProjectsBrowserComponent::ProjectsBrowserComponent(EditViewState &evs, ApplicationViewState &avs)
    : BrowserBaseComponent(avs),
      m_evs(evs),
      m_avs(avs),
      m_newProjectButton("New", buttonStyle),
      m_loadProjectButton("Load", buttonStyle),
      m_saveProjectButton("Save", buttonStyle)
{
    const auto margin = 7;

    if (auto parent = dynamic_cast<MainComponent *>(getParentComponent()))
        addChangeListener(parent);

    addAndMakeVisible(m_projectsMenu);
    m_projectsMenu.addButton(&m_newProjectButton);

    GUIHelpers::setDrawableOnButton(m_newProjectButton, BinaryData::newProjectButton_svg, avs.getProjectsColour());
    m_newProjectButton.setTooltip(GUIHelpers::translate("handle projects", avs));
    m_newProjectButton.setEdgeIndent(margin);

    m_projectsMenu.addButton(&m_loadProjectButton);
    GUIHelpers::setDrawableOnButton(m_loadProjectButton, BinaryData::filedownload_svg, juce::Colours::lightcyan);
    m_loadProjectButton.setTooltip(GUIHelpers::translate("handle projects", avs));
    m_loadProjectButton.setEdgeIndent(margin);

    m_projectsMenu.addButton(&m_saveProjectButton);
    GUIHelpers::setDrawableOnButton(m_saveProjectButton, BinaryData::contentsaveedit_svg, juce::Colours::seagreen);
    m_saveProjectButton.setTooltip(GUIHelpers::translate("handle projects", avs));
    m_saveProjectButton.setEdgeIndent(margin);

    m_newProjectButton.onClick = [this]
    {
        m_evs.m_edit.engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
        m_projectToLoad = juce::File();
        sendChangeMessage();
    };

    m_loadProjectButton.onClick = [this]
    {
        juce::WildcardFileFilter wildcardFilter("*.tracktionedit", juce::String(), "Next Studio Project File");

        juce::FileBrowserComponent browser(juce::FileBrowserComponent::openMode + juce::FileBrowserComponent::canSelectFiles, juce::File(m_avs.m_projectsDir), &wildcardFilter, nullptr);

        juce::FileChooserDialogBox dialogBox("Load a project", "Please choose some kind of file that you want to load...", browser, true, juce::Colours::red);

        if (dialogBox.show())
        {
            m_projectToLoad = browser.getSelectedFile(0);
            sendChangeMessage();
        }
        sendChangeMessage();
    };

    m_saveProjectButton.onClick = [this] { GUIHelpers::saveEdit(m_evs, juce::File::createFileWithoutCheckingPath(m_avs.m_projectsDir)); };

    setName("ProjectBrowser");
    m_sortingBox.addItem(GUIHelpers::translate("by Name (a - z)", m_applicationViewState), 1);
    m_sortingBox.addItem(GUIHelpers::translate("by Name (z - a)", m_applicationViewState), 2);
    m_sortingBox.setSelectedId(1, juce::dontSendNotification);
}

void ProjectsBrowserComponent::paint(juce::Graphics &g)
{
    BrowserBaseComponent::paint(g);
    auto area = getLocalBounds();
    auto prjButtons = area.removeFromTop(m_projectsMenu.getHeight());
    g.drawHorizontalLine(prjButtons.getBottom(), 0, getWidth());
}

void ProjectsBrowserComponent::resized()
{
    auto area = getLocalBounds();
    auto prjButtons = area.removeFromTop(70);
    auto sortcomp = area.removeFromTop(30).reduced(2, 2);
    auto sortlabel = sortcomp.removeFromLeft(50);
    auto searchfield = area.removeFromBottom(30);
    auto list = area;

    m_projectsMenu.setBounds(prjButtons);
    m_sortLabel.setBounds(sortlabel);
    m_sortingBox.setBounds(sortcomp);
    m_searchField.setBounds(searchfield);
    m_listBox.setBounds(list);
}
juce::var ProjectsBrowserComponent::getDragSourceDescription(const juce::SparseSet<int> &) { return {"ProjectsBrowser"}; }

void ProjectsBrowserComponent::paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowNum < 0 || rowNum >= getNumRows())
    {
        return;
    }

    juce::Rectangle<int> bounds(0, 0, width, height);
    auto textColour = m_applicationViewState.getTextColour();
    g.setColour(rowNum % 2 == 0 ? m_applicationViewState.getBackgroundColour2() : m_applicationViewState.getBackgroundColour2().brighter(0.05f));
    g.fillRect(bounds);
    g.setColour(m_applicationViewState.getBorderColour().withAlpha(0.3f));
    g.drawHorizontalLine(height - 1, 0, width);

    if (rowIsSelected)
    {
        g.setColour(m_applicationViewState.getPrimeColour());
        g.fillRect(bounds);
    }
    bounds.reduce(4, 0);
    if (m_searchTerm.isEmpty())
    {
        g.setColour(rowIsSelected ? m_applicationViewState.getPrimeColour().contrasting(.7f) : textColour);
        g.drawFittedText(m_contentList[rowNum].getFileNameWithoutExtension(), bounds, juce::Justification::left, 1);
    }
    else
    {
        auto text = m_contentList[rowNum].getFileNameWithoutExtension();

        juce::String preTerm, postTerm;
        int termStartIndex = text.indexOfIgnoreCase(m_searchTerm);
        juce::String searchTerm = text.substring(termStartIndex, termStartIndex + m_searchTerm.length());

        if (termStartIndex != -1 && m_searchTerm.length() > 0)
        {
            preTerm = text.substring(0, termStartIndex);
            postTerm = text.substring(termStartIndex + m_searchTerm.length());
            auto colour = rowIsSelected ? juce::Colours::black : textColour;

            g.setColour(colour);
            g.setFont(juce::Font((float)height * 0.7f, juce::Font::bold));
            g.drawFittedText(preTerm, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 0.9f);

            int preTermWidth = g.getCurrentFont().getStringWidth(preTerm);

            g.setColour(juce::Colours::coral);
            g.drawFittedText(searchTerm, 4 + preTermWidth, 0, width - 6 - preTermWidth, height, juce::Justification::centredLeft, 1, 0.9f);

            int termWidth = g.getCurrentFont().getStringWidth(searchTerm);

            g.setColour(colour);
            g.drawFittedText(postTerm, 4 + preTermWidth + termWidth, 0, width - 6 - preTermWidth - termWidth, height, juce::Justification::centredLeft, 1, 0.9f);
        }
    }
}
void ProjectsBrowserComponent::listBoxItemClicked(int row, const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu p;
        p.addItem(1, "Info");
        const int result = p.show();
        if (result == 1)
        {
        }
    }
    else if (e.getNumberOfClicks() > 1)
    {
        m_projectToLoad = getContentList()[row];
        sendChangeMessage();
    }
}

void ProjectsBrowserComponent::selectedRowsChanged(int) {}

void ProjectsBrowserComponent::sortList(int selectedID)
{
    auto forward = selectedID == 1;
    juce::Array<juce::File> fileList;

    for (auto f : m_contentList)
        if (!f.isDirectory())
            fileList.add(f);

    sortByName(fileList, forward);

    m_contentList.clear();
    m_contentList.addArray(fileList);

    if (getParentComponent())
        getParentComponent()->resized();
}
void ProjectsBrowserComponent::sortByName(juce::Array<juce::File> &list, bool forward)
{
    if (list.size() > 1)
    {
        if (forward)
        {
            CompareNameForward cf;
            list.sort(cf);
        }
        else
        {
            CompareNameBackwards cb;
            list.sort(cb);
        }
    }
}
