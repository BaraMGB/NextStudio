#include "SampleBrowser.h"

juce::File SampleListBox::getSelectedSample()
{
    auto row = getSelectedRows()[0];
    return m_sampleBrowser.getFileList()[row]; 
}

SampleBrowserComponent::SampleBrowserComponent(ApplicationViewState &avs, SamplePreviewComponent &spc)
    : m_applicationViewState(avs)
    , m_samplePreviewComponent(spc)
    , m_listBox(*this)
{
    setName("SampleBrowser!");
    m_listBox.setName("ListBox");
    addAndMakeVisible (m_listBox);
    m_listBox.setModel (this);
    m_listBox.setRowHeight (20);
    m_listBox.setColour (
                juce::ListBox::ColourIds::backgroundColourId
                , juce::Colour(0xff171717));
}

void SampleBrowserComponent::resized()
{
    m_listBox.setBounds (getLocalBounds ());
}

void SampleBrowserComponent::paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowNum < 0|| rowNum >= getNumRows())
    {
        return;
    }

    juce::Rectangle<int> bounds (0,0, width, height);
    auto textColour = juce::Colours::white;
    g.setColour(juce::Colour(0xff171717));
    g.fillRect(bounds);


    if (rowIsSelected)
    {
        g.setColour(juce::Colour(0xff555555));
        g.fillRect(bounds);
    }
    bounds.reduce(10,0);
    g.setColour(textColour);
    g.drawFittedText(
                m_fileList[rowNum].getFileName ()
                , bounds, juce::Justification::left, 1);
}

juce::var SampleBrowserComponent::getDragSourceDescription(const juce::SparseSet<int> &)
{
    return {"SampleBrowser"};
}

void SampleBrowserComponent::setFileList(const juce::Array<juce::File> &fileList)
{
    m_listBox.deselectAllRows ();
    m_fileList = fileList;
    m_listBox.updateContent ();
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
    previewSampleFile (m_fileList[m_listBox.getSelectedRow ()]);
}

void SampleBrowserComponent::previewSampleFile(const juce::File &file)
{
    if (m_samplePreviewComponent.setFile (file))
    {
        m_samplePreviewComponent.rewind();
        m_samplePreviewComponent.play();
    }
}

void SampleBrowserComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
}
