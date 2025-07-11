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

#include "SampleBrowser.h"
#include "Browser_Base.h"
#include "SearchFieldComponent.h"
#include "Utilities.h"


SampleBrowserComponent::SampleBrowserComponent(ApplicationViewState &avs, SamplePreviewComponent &spc)
    : BrowserBaseComponent(avs)
    , m_samplePreviewComponent(spc)
{
    setName("SampleBrowser");
    m_sortingBox.addItem(GUIHelpers::translate("by Name (a - z)", m_applicationViewState), 1);
    m_sortingBox.addItem(GUIHelpers::translate("by Name (z - a)", m_applicationViewState), 2);
    m_sortingBox.addItem(GUIHelpers::translate("Random", m_applicationViewState), 3);
    m_sortingBox.setSelectedId(1, juce::dontSendNotification);
}
void SampleBrowserComponent::resized() 
{
    auto area = getLocalBounds();
    auto sortcomp = area.removeFromTop(30);
    auto sortlabel = sortcomp.removeFromLeft(50);
    auto searchfield = area.removeFromBottom(30);
    area.removeFromTop(2);
    area.removeFromBottom(2);
    auto list = area;

    m_sortLabel.setBounds(sortlabel);
    m_sortingBox.setBounds(sortcomp);
    m_searchField.setBounds(searchfield);
    m_listBox.setBounds (list);
}

void SampleBrowserComponent::paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowNum < 0 || rowNum >= getNumRows())
    {
        return;
    }

    juce::Rectangle<int> bounds (0, 0, width, height);
    auto textColour = m_applicationViewState.getTextColour();
    g.setColour (rowNum%2==0 ? m_applicationViewState.getBackgroundColour2() : m_applicationViewState.getBackgroundColour2().brighter(0.05f));
    g.fillRect(bounds);
    g.setColour(m_applicationViewState.getBorderColour().withAlpha(0.3f));
    g.drawHorizontalLine(height - 1, 0, width);

    if (rowIsSelected)
    {
        g.setColour(m_applicationViewState.getPrimeColour());
        g.fillRect(bounds);
    }

    auto textArea = bounds.reduced(10, 0);

    if (m_searchTerm.isEmpty())
    {
        g.setColour(rowIsSelected ? m_applicationViewState.getPrimeColour().contrasting(.7f) : textColour);
        g.drawFittedText(m_contentList[rowNum].getFileName(), textArea, juce::Justification::left, 1);
    }
    else
    {
        auto text = m_contentList[rowNum].getFileName();

        juce::String preTerm, postTerm;
        int termStartIndex = text.indexOfIgnoreCase(m_searchTerm);
        juce::String searchTerm = text.substring(termStartIndex, termStartIndex + m_searchTerm.length());

        if (termStartIndex != -1 && m_searchTerm.length() > 0)
        {
            preTerm = text.substring(0, termStartIndex);
            postTerm = text.substring(termStartIndex + m_searchTerm.length());
            auto colour = rowIsSelected ? juce::Colours::black : textColour;

            // Breite der einzelnen Textteile berechnen
            int preTermWidth = g.getCurrentFont().getStringWidth(preTerm);
            int termWidth = g.getCurrentFont().getStringWidth(searchTerm);
            int postTermWidth = g.getCurrentFont().getStringWidth(postTerm);

            g.setColour(colour);
            g.setFont(juce::Font((float) height * 0.7f, juce::Font::bold));
            g.drawFittedText(preTerm, textArea.getX(), textArea.getY(), 
                             juce::jmin(preTermWidth, textArea.getWidth()), textArea.getHeight(), 
                             juce::Justification::centredLeft, 1, 0.9f);

            if (textArea.getWidth() > preTermWidth)
            {
                g.setColour(juce::Colours::coral);
                g.drawFittedText(searchTerm, textArea.getX() + preTermWidth, textArea.getY(),
                                 juce::jmin(termWidth, textArea.getWidth() - preTermWidth), textArea.getHeight(), 
                                 juce::Justification::centredLeft, 1, 0.9f);
            }

            if (textArea.getWidth() > preTermWidth + termWidth)
            {
                g.setColour(colour);
                g.drawFittedText(postTerm, textArea.getX() + preTermWidth + termWidth, textArea.getY(),
                                 juce::jmin(postTermWidth, textArea.getWidth() - preTermWidth - termWidth), textArea.getHeight(), 
                                 juce::Justification::centredLeft, 1, 0.9f);
            }
        }
    }
}
juce::var SampleBrowserComponent::getDragSourceDescription(const juce::SparseSet<int> &)
{
    return {"SampleBrowser"};
}

void SampleBrowserComponent::listBoxItemClicked(int row, const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown ())
    {
        juce::PopupMenu p;
        p.addItem (1, "Info");
        const int result = p.show();
        if(result == 1)
        {
        }
    }
}

void SampleBrowserComponent::selectedRowsChanged(int)
{
    previewSampleFile (m_contentList[m_listBox.getSelectedRow ()]);
}

void SampleBrowserComponent::previewSampleFile(const juce::File &file)
{
    if (m_samplePreviewComponent.setFile (file))
    {
        m_samplePreviewComponent.rewind();
        m_samplePreviewComponent.play();
    }
}

void SampleBrowserComponent::sortList(int selectedID)
{
    GUIHelpers::log("SELECTED ID: " , selectedID);
    juce::Array<juce::File> fileList;

    for (auto f : m_contentList)
        if (!f.isDirectory())
            fileList.add(f);

    if (selectedID == 1)
        sortByName(fileList, true);
    else if (selectedID == 2)
        sortByName(fileList, false);
    else if (selectedID == 3)
        shuffleFileArray(fileList);

    m_contentList.clear();
    m_contentList.addArray(fileList);

    getParentComponent()->resized(); 
}
void SampleBrowserComponent::sortByName(juce::Array<juce::File>& list, bool forward)
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
void SampleBrowserComponent::shuffleFileArray(juce::Array<juce::File>& fileList)
{
    juce::Random r;
    for (int i = fileList.size(); --i > 0;)
    {
        int newPos = r.nextInt(i + 1);
        if (newPos != i)
            fileList.swap(i, newPos);
    }
}
