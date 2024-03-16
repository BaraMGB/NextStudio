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
    auto sortcomp = area.removeFromTop(30).reduced(2,2);
    auto sortlabel = sortcomp.removeFromLeft(50);
    auto searchfield = area.removeFromBottom(30);
    auto list = area;

    m_sortLabel.setBounds(sortlabel);
    m_sortingBox.setBounds(sortcomp);
    m_searchField.setBounds(searchfield);
    m_listBox.setBounds (list);
}

void SampleBrowserComponent::paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowNum < 0|| rowNum >= getNumRows())
    {
        return;
    }

    juce::Rectangle<int> bounds (0,0, width, height);
    auto textColour = m_applicationViewState.getTextColour();
    g.setColour (rowNum%2==0 ? m_applicationViewState.getMenuBackgroundColour() : m_applicationViewState.getMenuBackgroundColour().brighter(0.05f));
    g.fillRect(bounds);
    g.setColour(m_applicationViewState.getBorderColour());
    g.drawHorizontalLine(height - 1, 0, width);

    if (rowIsSelected)
    {
        g.setColour(juce::Colour(0xff555555));
        g.fillRect(bounds);
    }
    bounds.reduce(10,0);
    if (m_searchTerm.isEmpty())
    {
        g.setColour(rowIsSelected ? juce::Colours::black : textColour);
        g.drawFittedText(m_contentList[rowNum].getFileName (), bounds, juce::Justification::left, 1);
    }
    else
    {
        auto text = m_contentList[rowNum].getFileName ();

        juce::String preTerm, postTerm;
        int termStartIndex = text.indexOfIgnoreCase(m_searchTerm);
        juce::String searchTerm = text.substring(termStartIndex, termStartIndex + m_searchTerm.length());

        if (termStartIndex != -1 && m_searchTerm.length() > 0)
        {
            preTerm = text.substring(0, termStartIndex);
            postTerm = text.substring(termStartIndex + m_searchTerm.length());
            auto colour = rowIsSelected ? juce::Colours::black : textColour;

            g.setColour(colour);
            g.setFont(juce::Font((float) height * 0.7f, juce::Font::bold));
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
