#include "Browser_Base.h"
#include "SearchFieldComponent.h"
#include "Utilities.h"

juce::File BrowserListBox::getSelectedFile()
{
    auto row = getSelectedRows()[0];
    return m_browser.getContentList()[row]; 
}
BrowserBaseComponent::BrowserBaseComponent(ApplicationViewState &avs)
    : m_applicationViewState(avs)
    , m_listBox(*this, avs)
    , m_searchField(avs)
{
    addAndMakeVisible (m_listBox);
    m_listBox.setModel (this);
    m_listBox.setRowHeight (20);
    m_listBox.setColour (juce::ListBox::ColourIds::backgroundColourId, m_applicationViewState.getBackgroundColour());
    addAndMakeVisible(m_searchField);
    m_searchField.addChangeListener(this);
}

void BrowserBaseComponent::resized()
{
    auto area = getLocalBounds();
    m_searchField.setBounds(area.removeFromBottom(30));
    m_listBox.setBounds (area);
}


void BrowserBaseComponent::setFileList(const juce::Array<juce::File> &fileList)
{
    m_listBox.deselectAllRows ();
    m_fileList = fileList;

    updateContentList();
}


void BrowserBaseComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto sf = dynamic_cast<SearchFieldComponent*>(source))
    {
        m_searchTerm = sf->getText();
        updateContentList();
    }
}
void BrowserBaseComponent::updateContentList()
{
    m_contentList.clear();

    for (const auto& entry : m_fileList)
        if (entry.getFileName().containsIgnoreCase(m_searchTerm))
            m_contentList.add(entry);

    sortList();
    m_listBox.updateContent();
    repaint();
}
