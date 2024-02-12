#include "ProjectsBrowser.h"
#include "SearchFieldComponent.h"
#include "Utilities.h"

juce::File ProjectsListBox::getSelectedProject()
{
    auto row = getSelectedRows()[0];
    return m_projectsBrowser.getContentList()[row]; 
}

ProjectsBrowserComponent::ProjectsBrowserComponent(ApplicationViewState &avs)
    : m_applicationViewState(avs)
    , m_listBox(*this, avs)
    , m_searchField(avs)
{
    setName("ProjectsBrowser!");
    m_listBox.setName("ListBox");
    addAndMakeVisible (m_listBox);
    m_listBox.setModel (this);
    m_listBox.setRowHeight (20);
    m_listBox.setColour (juce::ListBox::ColourIds::backgroundColourId, m_applicationViewState.getBackgroundColour());
    addAndMakeVisible(m_searchField);
    m_searchField.addChangeListener(this);
}

void ProjectsBrowserComponent::resized()
{
    auto area = getLocalBounds();
    m_searchField.setBounds(area.removeFromBottom(30));
    m_listBox.setBounds (area);
}

void ProjectsBrowserComponent::paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected)
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
    bounds.reduce(4,0);
    if (m_searchTerm.isEmpty())
    {
        g.setColour(rowIsSelected ? juce::Colours::black : textColour);
        g.drawFittedText(m_contentList[rowNum].getFileName ().trimCharactersAtEnd(".tracktionedit"), bounds, juce::Justification::left, 1);
    }
    else
    {
        auto text = m_contentList[rowNum].getFileName ().trimCharactersAtEnd(".tracktionedit");

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

juce::var ProjectsBrowserComponent::getDragSourceDescription(const juce::SparseSet<int> &)
{
    return {"ProjectsBrowser"};
}

void ProjectsBrowserComponent::setFileList(const juce::Array<juce::File> &fileList)
{
    m_listBox.deselectAllRows ();
    m_fileList = fileList;

    updateContentList();
}

void ProjectsBrowserComponent::listBoxItemClicked(int row, const juce::MouseEvent &e)
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

void ProjectsBrowserComponent::selectedRowsChanged(int)
{
}


void ProjectsBrowserComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto sf = dynamic_cast<SearchFieldComponent*>(source))
    {
        m_searchTerm = sf->getText();
        updateContentList();
    }
}
void ProjectsBrowserComponent::updateContentList()
{
    m_contentList.clear();

    for (const auto& entry : m_fileList)
        if (entry.getFileName().trimCharactersAtEnd(".tracktionedit").containsIgnoreCase(m_searchTerm))
            m_contentList.add(entry);

    m_listBox.updateContent();
    repaint();
}
