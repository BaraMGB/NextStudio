#include "FileBrowser.h"
#include "ApplicationViewState.h"
#include "SearchFieldComponent.h"
#include "Utilities.h"

PathComponent::PathComponent(juce::File dir, ApplicationViewState& appState) 
    : m_currentPath(dir)
    , m_appState(appState)
{
    addAndMakeVisible(m_currentPathField);
    m_currentPathField.setText(m_currentPath.getFullPathName());
    m_currentPathField.onReturnKey = [this] {
        setDir(juce::File (m_currentPathField.getText()));
    };
    addAndMakeVisible(m_button);
    m_button.onClick = [this] 
    {
        GUIHelpers::log("button");
        setDir(m_currentPath.getParentDirectory()); 
    };
    m_currentPathField.setColour(juce::TextEditor::ColourIds::backgroundColourId, m_appState.getMenuBackgroundColour());
    m_currentPathField.setColour(juce::TextEditor::ColourIds::shadowColourId, m_appState.getMenuBackgroundColour().darker(0.3f));
    m_currentPathField.setColour(juce::TextEditor::ColourIds::outlineColourId, m_appState.getBorderColour());
    m_currentPathField.setColour(juce::TextEditor::ColourIds::textColourId, m_appState.getTextColour());
    m_currentPathField.setColour(juce::TextEditor::ColourIds::highlightColourId, m_appState.getPrimeColour());
}

void PathComponent::resized() 
{
    auto area = getLocalBounds();
    auto upButton = area.removeFromRight(getHeight());
    upButton.reduce(3, 3);
    area.reduce(3, 3);
    m_currentPathField.setBounds(area);
    m_button.setBounds(upButton);
}



void PathComponent::setDir(juce::File file)
{
    if (!file.exists() || !file.isDirectory())
        return;

    m_currentPath = file;
    m_currentPathField.setText(m_currentPath.getFullPathName());
    sendChangeMessage();
}


juce::File PathComponent::getCurrentPath()
{
    return m_currentPath;
}

// ----------------------------------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------------------------------
juce::File FileListBox::getSelectedFile()
{
    auto row = getSelectedRows()[0];
    return m_fileBrowser.getContentList()[row]; 
}

FileBrowserComponent::FileBrowserComponent(ApplicationViewState &avs, te::Engine& engine)
    : m_applicationViewState(avs)
    , m_samplePreviewComponent(engine, avs)
    , m_listBox(*this, m_applicationViewState)
    , m_searchField(m_applicationViewState)
    , m_currentPathField(juce::File(m_applicationViewState.m_workDir), avs)
{
    setName("SampleBrowser!");

    m_listBox.setName("ListBox");
    addAndMakeVisible (m_listBox);
    m_listBox.setModel (this);
    m_listBox.setRowHeight (20);
    m_listBox.setColour (juce::ListBox::ColourIds::backgroundColourId, m_applicationViewState.getBackgroundColour());

    addAndMakeVisible(m_searchField);
    m_searchField.addChangeListener(this);

    addAndMakeVisible(m_currentPathField);
    m_currentPathField.addChangeListener(this);
    m_currentPathField.setAlwaysOnTop(true);

    addAndMakeVisible(m_samplePreviewComponent);
    
}

FileBrowserComponent::~FileBrowserComponent() 
{
    m_currentPathField.removeChangeListener(this);
    m_searchField.removeChangeListener(this);
}
void FileBrowserComponent::resized()
{
    auto area = getLocalBounds();
    auto pathComp = area.removeFromTop(30);
    
    auto preview = area.removeFromBottom(50);
    auto searchfield = area.removeFromBottom(30);

    auto list = area;

    m_currentPathField.setBounds(pathComp);
    m_searchField.setBounds(searchfield);
    m_listBox.setBounds (list);
    m_samplePreviewComponent.setBounds(preview);
}

void FileBrowserComponent::paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowNum < 0|| rowNum >= getNumRows())
    {
        return;
    }

    juce::Rectangle<int> bounds (0,0, width, height);
    auto textColour = m_applicationViewState.getTextColour();
    auto file = m_contentList[rowNum];
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
    auto icon = bounds.removeFromLeft(height).toFloat();
    bounds.reduce(10,0);
    icon.reduce(2,2);
    if (file.isDirectory())
        GUIHelpers::drawFromSvg(g, BinaryData::folder_svg, m_applicationViewState.getPrimeColour(), icon);
    else
    {
        GUIHelpers::drawFromSvg(g, BinaryData::file_svg, m_applicationViewState.getTextColour(), icon);
    };

    if (m_searchTerm.isEmpty())
    {
        g.setColour(rowIsSelected ? juce::Colours::black : textColour);
        g.drawFittedText(file.getFileName (), bounds, juce::Justification::left, 1);
    }
    else
    {
        auto text = file.getFileName ();

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
            g.drawFittedText(preTerm, bounds.getX(), bounds.getY(), bounds.getWidth() - 6, bounds.getHeight(), juce::Justification::centredLeft, 1, 0.9f);

            int preTermWidth = g.getCurrentFont().getStringWidth(preTerm);

            g.setColour(juce::Colours::coral);
            g.drawFittedText(searchTerm, bounds.getX() + preTermWidth, bounds.getY(), bounds.getWidth() - 6 - preTermWidth, bounds.getHeight(), juce::Justification::centredLeft, 1, 0.9f);

            int termWidth = g.getCurrentFont().getStringWidth(searchTerm);

            g.setColour(colour);
            g.drawFittedText(postTerm, bounds.getX() + preTermWidth + termWidth, bounds.getY(), bounds.getWidth() - 6 - preTermWidth - termWidth, bounds.getHeight(), juce::Justification::centredLeft, 1, 0.9f);
        }
    }
}

juce::var FileBrowserComponent::getDragSourceDescription(const juce::SparseSet<int> &)
{
    return {"FileBrowser"};
}

void FileBrowserComponent::setDirecory(const juce::File& dir)
{
    if (dir.exists() && dir.isDirectory())
    {
        GUIHelpers::log("FileBrowserComponent::setDirecory(): ");
        setFileList(dir.findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories , false ) );
    }
}

void FileBrowserComponent::setFileList(const juce::Array<juce::File> &fileList)
{
    m_listBox.deselectAllRows ();
    m_fileList = fileList;
    
    updateContentList();
}

void FileBrowserComponent::sortList(bool forward)
{
    juce::Array<juce::File> dirList;
    juce::Array<juce::File> fileList;

    for (auto f : m_contentList)
    {
        if (f.isDirectory())
            dirList.add(f);
        else
            fileList.add(f);
    }
    sortByName(dirList, forward);
    sortByName(fileList, forward);

    m_contentList.clear();
    if (forward)
    {
        m_contentList.addArray(dirList);
        m_contentList.addArray(fileList);
    }
    else
    {
        m_contentList.addArray(fileList);
        m_contentList.addArray(dirList);
    }
}

void FileBrowserComponent::listBoxItemClicked(int row, const juce::MouseEvent &e)
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
void FileBrowserComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent &e)
{
    auto clickedFile = m_contentList[row];
    if (e.mods.isRightButtonDown ())
    {
        juce::PopupMenu p;
        p.addItem (1, "Info");
        const int result = p.show();
        if(result == 1)
        {
        }
    }
    else if (clickedFile.isDirectory())
    {
        m_currentPathField.setDir(clickedFile);
    }
}

void FileBrowserComponent::selectedRowsChanged(int)
{
    previewSampleFile (m_contentList[m_listBox.getSelectedRow ()]);
}

void FileBrowserComponent::previewSampleFile(const juce::File &file)
{
    if (m_samplePreviewComponent.setFile (file))
    {
        m_samplePreviewComponent.rewind();
        m_samplePreviewComponent.play();
    }
}

void FileBrowserComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto sf = dynamic_cast<SearchFieldComponent*>(source))
    {
        m_searchTerm = sf->getText();
        updateContentList();
    }
    else if (source == &m_currentPathField)
    {
        GUIHelpers::log("FileBrowserComponent::setDir()");
        setDirecory(m_currentPathField.getCurrentPath());
    }
}
void FileBrowserComponent::sortByName(juce::Array<juce::File>& list, bool forward)
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
void FileBrowserComponent::updateContentList()
{
    m_contentList.clear();

    for (const auto& entry : m_fileList)
        if (entry.getFileName().containsIgnoreCase(m_searchTerm))
            m_contentList.add(entry);

    sortList();
    m_listBox.updateContent();
    repaint();
}
